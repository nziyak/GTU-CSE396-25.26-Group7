"""
@file      camera_internal.py
@brief     Internal camera handling layer for the Vision & AI Pipeline
@author    Gabil Rahimli
@date      2026-03-29
@version   0.2
"""

from __future__ import annotations

from dataclasses import dataclass
import os
import sys
import time
from typing import Optional

try:
    import cv2  # type: ignore[import-not-found]
except Exception:  # pragma: no cover - optional dependency
    cv2 = None

try:
    import numpy as np
except Exception:  # pragma: no cover - optional dependency
    np = None


@dataclass
class CameraFrame:
    """Stores one captured camera frame and its metadata."""

    data: object
    width: int
    height: int
    channels: int


class CameraInternal:
    """
    Internal helper responsible for camera initialization, frame capture,
    FPS changes, and safe shutdown.
    """

    def __init__(
        self,
        device_source: Optional[str] = None,
        frame_width: int = 640,
        frame_height: int = 480,
        allow_mock_camera: bool = True,
    ) -> None:
        self.fps = 5
        self.is_open = False
        self.frame_width = frame_width
        self.frame_height = frame_height
        self.allow_mock_camera = allow_mock_camera
        self.device_source = device_source or os.getenv("MOD02_CAMERA_SOURCE", "0")
        self._capture = None
        self._mock_mode = False

    def vision_camera_init(self, fps: int) -> int:
        """Initializes the camera device with the requested FPS value."""
        self.fps = fps

        if cv2 is None:
            if self.allow_mock_camera:
                self._mock_mode = True
                self.is_open = True
                return 0
            return -1

        source: object = self.device_source
        if isinstance(source, str) and source.isdigit():
            source = int(source)

        capture = self._open_capture(source)
        if capture is not None and capture.isOpened():
            capture.set(cv2.CAP_PROP_FRAME_WIDTH, float(self.frame_width))
            capture.set(cv2.CAP_PROP_FRAME_HEIGHT, float(self.frame_height))
            capture.set(cv2.CAP_PROP_FPS, float(fps))
            self._capture = capture
            self._mock_mode = False
            self.is_open = True
            return 0

        if capture is not None:
            capture.release()

        if self.allow_mock_camera:
            self._mock_mode = True
            self.is_open = True
            return 0

        return -1

    def vision_camera_capture_frame(self) -> Optional[CameraFrame]:
        """Captures one frame from the active camera stream."""
        if not self.is_open:
            return None

        if self._mock_mode:
            return self._build_mock_frame()

        if self._capture is None:
            return None

        ok, frame = self._capture.read()
        if not ok or frame is None:
            if self.allow_mock_camera:
                self._mock_mode = True
                return self._build_mock_frame()
            return None

        height, width = frame.shape[:2]
        channels = frame.shape[2] if len(frame.shape) > 2 else 1
        return CameraFrame(data=frame, width=width, height=height, channels=channels)

    def vision_camera_set_fps(self, fps: int) -> int:
        """Updates the camera FPS during runtime."""
        if fps <= 0:
            return -1
        self.fps = fps
        if self._capture is not None and cv2 is not None:
            self._capture.set(cv2.CAP_PROP_FPS, float(fps))
        return 0

    def vision_camera_release_frame(self, frame: CameraFrame) -> None:
        """Releases resources associated with a previously captured frame."""
        del frame

    def vision_camera_shutdown(self) -> None:
        """Safely shuts down the camera interface."""
        if self._capture is not None:
            self._capture.release()
            self._capture = None
        self.is_open = False
        self._mock_mode = False

    def _open_capture(self, source: object):
        """
        Prefer DirectShow for numeric camera indices on Windows because MSMF
        frequently emits noisy warnings and can fail intermittently.
        """
        if cv2 is None:
            return None

        if isinstance(source, int) and sys.platform.startswith("win"):
            preferred_backends = []
            if hasattr(cv2, "CAP_DSHOW"):
                preferred_backends.append(cv2.CAP_DSHOW)
            if hasattr(cv2, "CAP_MSMF"):
                preferred_backends.append(cv2.CAP_MSMF)

            for backend in preferred_backends:
                capture = cv2.VideoCapture(source, backend)
                if capture is not None and capture.isOpened():
                    return capture
                if capture is not None:
                    capture.release()

        return cv2.VideoCapture(source)

    def _build_mock_frame(self) -> CameraFrame:
        """Produces a synthetic frame when no real camera is available."""
        if np is None:
            return CameraFrame(
                data=None,
                width=self.frame_width,
                height=self.frame_height,
                channels=3,
            )

        frame = np.zeros((self.frame_height, self.frame_width, 3), dtype=np.uint8)
        tick = int(time.time() * 10) % max(self.frame_width, 1)
        frame[:, :, 0] = 24
        frame[:, :, 1] = 32
        frame[:, :, 2] = 40
        frame[:, tick:tick + 24, 1] = 220
        return CameraFrame(
            data=frame,
            width=self.frame_width,
            height=self.frame_height,
            channels=3,
        )
