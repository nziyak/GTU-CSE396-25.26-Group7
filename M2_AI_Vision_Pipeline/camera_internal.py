"""
@file      camera_internal.py
@brief     Internal camera handling layer for the Vision & AI Pipeline
@author    Gabil Rahimli
@date      2026-03-29
@version   0.1

Changelog:
v0.1 - Initial draft, defined CameraFrame data class and CameraInternal camera control methods.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional


@dataclass
class CameraFrame:
    """
    @brief Stores one captured camera frame and its metadata.
    """
    data: object
    width: int
    height: int
    channels: int


class CameraInternal:
    """
    @brief Internal helper class responsible for camera initialization, frame capture,
           FPS updates, and safe shutdown.
    """

    def __init__(self) -> None:
        """
        @brief Creates the internal camera controller object with default values.
        """
        self.fps = 5
        self.is_open = False

    def vision_camera_init(self, fps: int) -> int:
        """
        @brief Initializes the camera device with the requested FPS value.
        @param fps Target frames-per-second value for camera operation.
        @return 0 on success, negative value on failure.
        """
        self.fps = fps
        self.is_open = True
        return 0

    def vision_camera_capture_frame(self) -> Optional[CameraFrame]:
        """
        @brief Captures one frame from the active camera stream.
        @return CameraFrame object if capture is successful, None otherwise.
        """
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
        """
        @brief Updates the camera FPS during runtime.
        @param fps New frames-per-second value.
        @return 0 on success, negative value if the given FPS is invalid.
        """
        if fps <= 0:
            return -1
        self.fps = fps
        return 0

    def vision_camera_release_frame(self, frame: CameraFrame) -> None:
        """
        @brief Releases resources associated with a previously captured frame.
        @param frame Captured frame object to be released.
        """
        # Python'da çoğu durumda GC yeterli olur.
        # Ama burada açıkça bırakma mantığını koruyoruz.
        del frame

    def vision_camera_shutdown(self) -> None:
        """
        @brief Safely shuts down the camera interface.
        """
        self.is_open = False
