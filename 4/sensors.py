import logging
import time
from abc import ABC, abstractmethod
from typing import Tuple, Union
import cv2


class Sensor(ABC):
    @abstractmethod
    def get(self):
        raise NotImplementedError("Subclasses must implement method get()")


class SensorX(Sensor):
    def __init__(self, delay: float):
        self._delay = delay
        self._data = 0

    def get(self) -> int:
        time.sleep(self._delay)
        self._data += 1
        return self._data


class SensorCam(Sensor):
    def __init__(self, camera_name: str, resolution: Tuple[int, int]):
        self._camera_name = camera_name
        self._capture = None

        source: Union[int, str] = int(camera_name) if str(camera_name).isdigit() else camera_name
        self._capture = cv2.VideoCapture(source)

        if not self._capture.isOpened():
            logging.error("Cannot open camera: %s", camera_name)
            raise RuntimeError(f"Cannot open camera: {camera_name}")

        width, height = resolution
        self._capture.set(cv2.CAP_PROP_FRAME_WIDTH, width)
        self._capture.set(cv2.CAP_PROP_FRAME_HEIGHT, height)

    def get(self):
        ok, frame = self._capture.read()
        if not ok or frame is None:
            logging.error("Cannot read frame from camera: %s", self._camera_name)
            raise RuntimeError("Cannot read frame from camera")
        return frame

    def __del__(self):
        if getattr(self, "_capture", None) is not None:
            self._capture.release()


class WindowImage:
    def __init__(self, fps: float, title: str = "Webcam"):
        if fps <= 0:
            raise ValueError("FPS must be positive")

        self._title = title
        self._wait_ms = max(1, round(1000 / fps))

        try:
            cv2.namedWindow(self._title, cv2.WINDOW_NORMAL)
        except cv2.error as exc:
            logging.exception("Cannot create OpenCV window")
            raise RuntimeError("Cannot create OpenCV window") from exc

    def show(self, img) -> int:
        try:
            cv2.imshow(self._title, img)
            return cv2.waitKey(self._wait_ms) & 0xFF
        except cv2.error as exc:
            logging.exception("Cannot display image")
            raise RuntimeError("Cannot display image") from exc

    def __del__(self):
        try:
            cv2.destroyWindow(self._title)
        except cv2.error:
            pass
