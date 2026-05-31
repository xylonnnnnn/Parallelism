from __future__ import annotations

import argparse
import os
import queue
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import cv2
import numpy as np

WIDTH = 640
HEIGHT = 480
MODEL_PATH = "yolov8s-pose.pt"


class VideoCaptureResource:
    def __init__(self, source: int | str | Path):
        self.cap = cv2.VideoCapture(str(source) if isinstance(source, Path) else source)
        if not self.cap.isOpened():
            self.cap.release()
            raise RuntimeError(f"Не удалось открыть источник видео: {source}")

    def read(self) -> tuple[bool, np.ndarray]:
        return self.cap.read()

    def get(self, prop: int) -> float:
        return float(self.cap.get(prop))

    def set(self, prop: int, value: float) -> bool:
        return bool(self.cap.set(prop, value))

    def close(self) -> None:
        if self.cap is not None:
            self.cap.release()
            self.cap = None

    def __enter__(self) -> "VideoCaptureResource":
        return self

    def __exit__(self, exc_type: Any, exc: Any, tb: Any) -> None:
        self.close()

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass


class VideoWriterResource:
    def __init__(self, path: Path, fps: float, size: tuple[int, int]):
        path.parent.mkdir(parents=True, exist_ok=True)
        fourcc = cv2.VideoWriter_fourcc(*"mp4v")
        self.writer = cv2.VideoWriter(str(path), fourcc, fps, size)
        if not self.writer.isOpened():
            self.writer.release()
            raise RuntimeError(f"Не удалось открыть файл для записи: {path}")

    def write(self, frame: np.ndarray) -> None:
        self.writer.write(frame)

    def close(self) -> None:
        if self.writer is not None:
            self.writer.release()
            self.writer = None

    def __enter__(self) -> "VideoWriterResource":
        return self

    def __exit__(self, exc_type: Any, exc: Any, tb: Any) -> None:
        self.close()

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass


@dataclass(frozen=True)
class RunStats:
    mode: str
    workers: int
    frames: int
    elapsed: float

    @property
    def fps(self) -> float:
        return self.frames / self.elapsed if self.elapsed > 0 else 0.0

    @property
    def ms_per_frame(self) -> float:
        return self.elapsed * 1000.0 / self.frames if self.frames > 0 else 0.0


def frame_to_size(frame: np.ndarray, size: tuple[int, int]) -> np.ndarray:
    h, w = frame.shape[:2]
    if (w, h) == size:
        return frame
    return cv2.resize(frame, size, interpolation=cv2.INTER_AREA)


def cpu_count() -> int:
    return os.cpu_count() or 1


def default_workers() -> int:
    return max(1, min(cpu_count(), 8))


def torch_threads_for(workers: int, requested: int | None) -> int:
    if requested is not None:
        return max(1, requested)
    return max(1, cpu_count() // max(1, workers))


def configure_torch(torch_threads: int) -> None:
    import torch

    torch.set_num_threads(max(1, torch_threads))
    try:
        torch.set_num_interop_threads(1)
    except RuntimeError:
        pass


def make_model():
    from ultralytics import YOLO

    return YOLO(MODEL_PATH)


def predict_frame(model: Any, frame: np.ndarray, image_size: int) -> np.ndarray:
    result = model.predict(frame, imgsz=image_size, device="cpu", verbose=False)[0]
    return result.plot()


def read_video_info(video_path: Path) -> tuple[float, tuple[int, int]]:
    with VideoCaptureResource(video_path) as cap:
        fps = cap.get(cv2.CAP_PROP_FPS)
        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    if fps <= 0:
        fps = 25.0
    if width <= 0 or height <= 0:
        width, height = WIDTH, HEIGHT
    return fps, (width, height)


def process_single(video_path: Path, output_path: Path, image_size: int, size: tuple[int, int], torch_threads: int) -> RunStats:
    configure_torch(torch_threads)
    model = make_model()
    fps, _ = read_video_info(video_path)
    frames = 0
    started = time.perf_counter()
    with VideoCaptureResource(video_path) as cap, VideoWriterResource(output_path, fps, size) as writer:
        while True:
            ok, frame = cap.read()
            if not ok:
                break
            frame = frame_to_size(frame, size)
            writer.write(predict_frame(model, frame, image_size))
            frames += 1
    elapsed = time.perf_counter() - started
    if frames == 0:
        raise RuntimeError("Во входном видео не найдено кадров")
    return RunStats("single", 1, frames, elapsed)


def worker_loop(in_q: queue.Queue, out_q: queue.Queue, ready_q: queue.Queue, stop: threading.Event, workers: int, image_size: int, torch_threads: int) -> None:
    try:
        configure_torch(torch_threads)
        model = make_model()
        ready_q.put((True, ""))
        while not stop.is_set():
            item = in_q.get()
            if item is None:
                in_q.task_done()
                break
            idx, frame = item
            try:
                out_q.put((idx, predict_frame(model, frame, image_size)))
            finally:
                in_q.task_done()
    except Exception as exc:
        ready_q.put((False, str(exc)))
        stop.set()


def producer_loop(video_path: Path, in_q: queue.Queue, done: threading.Event, total: list[int], workers: int, size: tuple[int, int], stop: threading.Event) -> None:
    produced = 0
    try:
        with VideoCaptureResource(video_path) as cap:
            while not stop.is_set():
                ok, frame = cap.read()
                if not ok:
                    break
                in_q.put((produced, frame_to_size(frame, size)))
                produced += 1
    finally:
        total[0] = produced
        done.set()
        for _ in range(workers):
            in_q.put(None)


def drain_ordered_output(out_q: queue.Queue, writer: VideoWriterResource, producer_done: threading.Event, total: list[int], stop: threading.Event) -> int:
    pending: dict[int, np.ndarray] = {}
    next_idx = 0
    written = 0
    while not stop.is_set():
        if producer_done.is_set() and written >= total[0]:
            break
        try:
            idx, frame = out_q.get(timeout=0.05)
            pending[idx] = frame
        except queue.Empty:
            continue
        while next_idx in pending:
            writer.write(pending.pop(next_idx))
            next_idx += 1
            written += 1
    return written


def process_multi(video_path: Path, output_path: Path, workers: int, image_size: int, size: tuple[int, int], queue_size: int, torch_threads: int) -> RunStats:
    workers = max(1, workers)
    fps, _ = read_video_info(video_path)
    in_q: queue.Queue = queue.Queue(maxsize=max(1, queue_size))
    out_q: queue.Queue = queue.Queue(maxsize=max(1, queue_size * workers))
    ready_q: queue.Queue = queue.Queue()
    stop = threading.Event()
    producer_done = threading.Event()
    total = [0]
    threads = [threading.Thread(target=worker_loop, args=(in_q, out_q, ready_q, stop, workers, image_size, torch_threads), daemon=True) for _ in range(workers)]
    for thread in threads:
        thread.start()
    for _ in threads:
        ok, message = ready_q.get()
        if not ok:
            stop.set()
            raise RuntimeError(message)
    started = time.perf_counter()
    producer = threading.Thread(target=producer_loop, args=(video_path, in_q, producer_done, total, workers, size, stop), daemon=True)
    producer.start()
    with VideoWriterResource(output_path, fps, size) as writer:
        written = drain_ordered_output(out_q, writer, producer_done, total, stop)
    producer.join()
    in_q.join()
    for thread in threads:
        thread.join()
    elapsed = time.perf_counter() - started
    if stop.is_set() and written < total[0]:
        raise RuntimeError("Обработка остановлена из-за ошибки в рабочем потоке")
    if written == 0:
        raise RuntimeError("Во входном видео не найдено кадров")
    return RunStats("multi", workers, written, elapsed)


def benchmark(video_path: Path, output_dir: Path, thread_counts: list[int], image_size: int, size: tuple[int, int], queue_size: int, torch_threads: int | None) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    single_stats = process_single(video_path, output_dir / "single.mp4", image_size, size, torch_threads_for(1, torch_threads))
    rows = [single_stats]
    for workers in thread_counts:
        current_threads = torch_threads_for(workers, torch_threads)
        rows.append(process_multi(video_path, output_dir / f"multi_{workers}.mp4", workers, image_size, size, queue_size, current_threads))
    base = single_stats.elapsed
    print("mode\tworkers\ttorch_threads\tframes\telapsed_s\tfps\tms_per_frame\tspeedup")
    print(f"single\t1\t{torch_threads_for(1, torch_threads)}\t{single_stats.frames}\t{single_stats.elapsed:.4f}\t{single_stats.fps:.2f}\t{single_stats.ms_per_frame:.2f}\t1.00")
    best = single_stats
    for row in rows[1:]:
        speedup = base / row.elapsed if row.elapsed > 0 else 0.0
        current_threads = torch_threads_for(row.workers, torch_threads)
        print(f"multi\t{row.workers}\t{current_threads}\t{row.frames}\t{row.elapsed:.4f}\t{row.fps:.2f}\t{row.ms_per_frame:.2f}\t{speedup:.2f}")
        if row.elapsed < best.elapsed:
            best = row
    print(f"Лучший режим: {best.mode}, workers={best.workers}, elapsed={best.elapsed:.4f}s")


def parse_thread_list(value: str) -> list[int]:
    result = []
    for part in value.split(","):
        text = part.strip()
        if text:
            number = int(text)
            if number > 0:
                result.append(number)
    return sorted(set(result))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="YOLOv8s-pose CPU: single/multi-threaded frame processing")
    parser.add_argument("--video", type=Path)
    parser.add_argument("--mode", choices=("single", "multi"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--threads", type=int, default=default_workers())
    parser.add_argument("--benchmark", action="store_true")
    parser.add_argument("--benchmark-threads", default="1,2,3,4,6,8")
    parser.add_argument("--benchmark-dir", type=Path, default=Path("benchmark_out"))
    parser.add_argument("--queue-size", type=int, default=16)
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--width", type=int, default=WIDTH)
    parser.add_argument("--height", type=int, default=HEIGHT)
    parser.add_argument("--torch-threads", type=int)
    return parser


def print_stats(stats: RunStats, torch_threads: int, output: Path) -> None:
    print(f"Режим: {stats.mode}")
    print(f"Рабочих потоков: {stats.workers}")
    print(f"Потоков PyTorch на worker: {torch_threads}")
    print(f"Кадров: {stats.frames}")
    print(f"Время обработки всех кадров: {stats.elapsed:.4f} s")
    print(f"FPS обработки: {stats.fps:.2f}")
    print(f"ms/кадр: {stats.ms_per_frame:.2f}")
    print(f"Выходное видео: {output.resolve()}")


def main() -> int:
    args = build_parser().parse_args()
    size = (max(1, args.width), max(1, args.height))
    if args.video is None:
        raise SystemExit("Укажите --video")
    if args.benchmark:
        thread_counts = parse_thread_list(args.benchmark_threads)
        if not thread_counts:
            thread_counts = [1, 2, 4, default_workers()]
        benchmark(args.video, args.benchmark_dir, thread_counts, args.imgsz, size, args.queue_size, args.torch_threads)
        return 0
    if args.mode is None or args.output is None:
        raise SystemExit("Для обработки файла нужны --mode и --output")
    if args.mode == "single":
        torch_threads = torch_threads_for(1, args.torch_threads)
        stats = process_single(args.video, args.output, args.imgsz, size, torch_threads)
    else:
        workers = max(1, args.threads)
        torch_threads = torch_threads_for(workers, args.torch_threads)
        stats = process_multi(args.video, args.output, workers, args.imgsz, size, args.queue_size, torch_threads)
    print_stats(stats, torch_threads, args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
