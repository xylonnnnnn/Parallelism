import argparse
import logging
import queue
import signal
import threading
from pathlib import Path
from typing import Any, List, Optional, Tuple
import cv2
import numpy as np
from sensors import SensorCam, SensorX, WindowImage


ROOT_DIR = Path(__file__).resolve().parent
LOG_DIR = ROOT_DIR / "log"
LOG_DIR.mkdir(exist_ok=True)

logging.basicConfig(
    filename=LOG_DIR / "task4.log",
    level=logging.INFO,
    format="%(asctime)s | %(levelname)s | %(threadName)s | %(message)s",
)


def parse_resolution(text: str) -> Tuple[int, int]:
    try:
        width_text, height_text = text.lower().split("x", 1)
        width = int(width_text)
        height = int(height_text)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("Resolution must look like 640x480") from exc

    if width <= 0 or height <= 0:
        raise argparse.ArgumentTypeError("Resolution values must be positive")

    return width, height


def save_latest(target_queue: queue.Queue, value: Any) -> None:
    if target_queue.full():
        try:
            target_queue.get_nowait()
        except queue.Empty:
            pass
    target_queue.put_nowait(value)


def sensor_worker(sensor, output_queue: queue.Queue, stop_flag: threading.Event) -> None:
    while not stop_flag.is_set():
        try:
            value = sensor.get()
            save_latest(output_queue, value)
        except Exception:
            logging.exception("Sensor worker stopped because of an error")
            stop_flag.set()


def read_latest(source_queue: queue.Queue, old_value: Optional[Any]) -> Optional[Any]:
    value = old_value
    while True:
        try:
            value = source_queue.get_nowait()
        except queue.Empty:
            return value


def draw_values(frame, values: List[Optional[int]]):
    if frame is None:
        frame = np.zeros((480, 640, 3), dtype=np.uint8)

    image = frame.copy()
    height, width = image.shape[:2]

    box_width = 210
    box_height = 105
    left = max(10, width - box_width - 15)
    top = max(10, height - box_height - 15)
    right = left + box_width
    bottom = top + box_height

    overlay = image.copy()
    cv2.rectangle(overlay, (left, top), (right, bottom), (255, 255, 255), -1)
    image = cv2.addWeighted(overlay, 0.75, image, 0.25, 0)
    cv2.rectangle(image, (left, top), (right, bottom), (0, 0, 0), 1)

    for number, value in enumerate(values):
        text = f"Sensor{number}: {value if value is not None else '---'}"
        position = (left + 14, top + 30 + number * 25)
        cv2.putText(image, text, position, cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 0, 0), 1)

    return image


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Task 4: threaded sensors and camera")
    parser.add_argument("--camera", default="0", help="camera name/index, for example 0 or /dev/video0")
    parser.add_argument("--resolution", type=parse_resolution, default=parse_resolution("640x480"), help="camera resolution, for example 1280x720")
    parser.add_argument("--fps", type=float, default=30.0, help="image display frequency")
    return parser


def main() -> int:
    args = build_parser().parse_args()

    stop_flag = threading.Event()
    signal.signal(signal.SIGINT, lambda *_: stop_flag.set())
    signal.signal(signal.SIGTERM, lambda *_: stop_flag.set())

    sensors = [
        SensorCam(args.camera, args.resolution),
        SensorX(0.01),
        SensorX(0.1),
        SensorX(1),
    ]

    queues = [queue.Queue(maxsize=1) for _ in sensors]
    threads = []

    for index, sensor in enumerate(sensors):
        thread = threading.Thread(
            target=sensor_worker,
            args=(sensor, queues[index], stop_flag),
            name=f"sensor-worker-{index}",
            daemon=True,
        )
        threads.append(thread)
        thread.start()

    window = WindowImage(args.fps)
    latest = [None for _ in sensors]

    try:
        while not stop_flag.is_set():
            for index, current_queue in enumerate(queues):
                latest[index] = read_latest(current_queue, latest[index])

            camera_frame = latest[0]
            sensor_values = latest[1:]
            result_image = draw_values(camera_frame, sensor_values)

            key = window.show(result_image)
            if key == ord("q"):
                stop_flag.set()
    finally:
        stop_flag.set()
        for thread in threads:
            thread.join(timeout=2)
        del window
        for sensor in sensors:
            del sensor

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
