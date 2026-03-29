from __future__ import annotations

from dataclasses import dataclass
from typing import Optional


@dataclass
class CameraFrame:
    data: object
    width: int
    height: int
    channels: int


class CameraInternal:
    def __init__(self) -> None:
        self.fps = 5
        self.is_open = False

    def vision_camera_init(self, fps: int) -> int:
        self.fps = fps
        self.is_open = True
        return 0

    def vision_camera_capture_frame(self) -> Optional[CameraFrame]:
        if not self.is_open:
            return None

        # Burada gerçek projede OpenCV / Picamera2 ile frame alınır.
        dummy_frame = CameraFrame(
            data=None,
            width=640,
            height=480,
            channels=3,
        )
        return dummy_frame

    def vision_camera_set_fps(self, fps: int) -> int:
        if fps <= 0:
            return -1
        self.fps = fps
        return 0

    def vision_camera_release_frame(self, frame: CameraFrame) -> None:
        # Python'da çoğu durumda GC yeterli olur.
        # Ama burada açıkça bırakma mantığını koruyoruz.
        del frame

    def vision_camera_shutdown(self) -> None:
        self.is_open = False
