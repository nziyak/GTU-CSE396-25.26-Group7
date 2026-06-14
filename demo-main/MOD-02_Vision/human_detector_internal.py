"""
@file      human_detector_internal.py
@brief     Internal human detection layer for the Vision & AI Pipeline
@author    Gabil Rahimli
@date      2026-03-29
@version   0.2
"""

from __future__ import annotations

from dataclasses import dataclass
import math
import os
from typing import List, Optional

from camera_internal import CameraFrame

try:
    import cv2  # type: ignore[import-not-found]
except Exception:  # pragma: no cover - optional dependency
    cv2 = None

try:
    from ultralytics import YOLO  # type: ignore[import-not-found]
except Exception:  # pragma: no cover - optional dependency
    YOLO = None


@dataclass
class CameraBBox:
    """Bounding box of a detected human target."""

    x: int
    y: int
    width: int
    height: int


@dataclass
class HumanDetection:
    """One human detection candidate with confidence and bounding box data."""

    valid: bool
    confidence: float
    bbox: CameraBBox
    bbox_area: int


class HumanDetectorInternal:
    """
    Internal helper that loads a detection backend, extracts person detections,
    and selects the best candidate for severity analysis.
    """

    def __init__(self, model_path: Optional[str] = None) -> None:
        self.model_loaded = False
        self.model_path = model_path or os.getenv("MOD02_DETECTOR_MODEL_PATH")
        self._backend = "unloaded"
        self._detector = None

    def vision_detector_init(self) -> int:
        """Initializes the human detector backend."""
        if self.model_path and os.path.exists(self.model_path) and YOLO is not None:
            self._detector = YOLO(self.model_path)
            self._backend = "yolo"
            self.model_loaded = True
            return 0

        if cv2 is not None and hasattr(cv2, "HOGDescriptor"):
            hog = cv2.HOGDescriptor()
            hog.setSVMDetector(cv2.HOGDescriptor_getDefaultPeopleDetector())
            self._detector = hog
            self._backend = "hog"
            self.model_loaded = True
            return 0

        return -1

    def vision_detector_detect(
        self,
        frame: CameraFrame,
        threshold: float,
    ) -> List[HumanDetection]:
        """Runs human detection on the given frame."""
        if not self.model_loaded or frame.data is None:
            return []

        if self._backend == "yolo":
            return self._detect_with_yolo(frame, threshold)
        if self._backend == "hog":
            return self._detect_with_hog(frame, threshold)
        return []

    def vision_detector_select_best(
        self,
        detections: List[HumanDetection],
    ) -> Optional[HumanDetection]:
        """Selects the best detection candidate from a list of detections."""
        if not detections:
            return None
        return max(detections, key=lambda d: (d.confidence * d.bbox_area, d.confidence))

    def vision_detector_shutdown(self) -> None:
        """Releases detector resources and marks the backend as unloaded."""
        self.model_loaded = False
        self._backend = "unloaded"
        self._detector = None

    def _detect_with_yolo(
        self,
        frame: CameraFrame,
        threshold: float,
    ) -> List[HumanDetection]:
        detections: List[HumanDetection] = []
        results = self._detector.predict(
            source=frame.data,
            conf=threshold,
            classes=[0],
            verbose=False,
        )

        if not results:
            return detections

        boxes = getattr(results[0], "boxes", None)
        if boxes is None:
            return detections

        for box in boxes:
            coords = box.xyxy[0].tolist()
            x1, y1, x2, y2 = [int(value) for value in coords]
            width = max(0, x2 - x1)
            height = max(0, y2 - y1)
            confidence = float(box.conf[0]) if getattr(box, "conf", None) is not None else 0.0
            if confidence < threshold or width <= 0 or height <= 0:
                continue

            detections.append(
                HumanDetection(
                    valid=True,
                    confidence=confidence,
                    bbox=CameraBBox(x=x1, y=y1, width=width, height=height),
                    bbox_area=width * height,
                )
            )

        return detections

    def _detect_with_hog(
        self,
        frame: CameraFrame,
        threshold: float,
    ) -> List[HumanDetection]:
        detections: List[HumanDetection] = []
        rects, weights = self._detector.detectMultiScale(
            frame.data,
            winStride=(8, 8),
            padding=(8, 8),
            scale=1.05,
        )

        for idx, (x, y, width, height) in enumerate(rects):
            raw_weight = float(weights[idx]) if idx < len(weights) else 0.0
            confidence = 1.0 / (1.0 + math.exp(-raw_weight))
            if confidence < threshold:
                continue

            detections.append(
                HumanDetection(
                    valid=True,
                    confidence=confidence,
                    bbox=CameraBBox(x=int(x), y=int(y), width=int(width), height=int(height)),
                    bbox_area=int(width) * int(height),
                )
            )

        return detections
