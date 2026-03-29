from __future__ import annotations

from dataclasses import dataclass
from typing import List, Optional

from camera_internal import CameraFrame


@dataclass
class CameraBBox:
    x: int
    y: int
    width: int
    height: int


@dataclass
class HumanDetection:
    valid: bool
    confidence: float
    bbox: CameraBBox
    bbox_area: int


class HumanDetectorInternal:
    def __init__(self) -> None:
        self.model_loaded = False

    def vision_detector_init(self) -> int:
        # Burada gerçek projede YOLO modeli yüklenir.
        self.model_loaded = True
        return 0

    def vision_detector_detect(
        self,
        frame: CameraFrame,
        threshold: float,
    ) -> List[HumanDetection]:
        if not self.model_loaded:
            return []

        # Dummy detection örneği
        detection = HumanDetection(
            valid=True,
            confidence=0.87,
            bbox=CameraBBox(x=100, y=120, width=80, height=160),
            bbox_area=80 * 160,
        )

        if detection.confidence < threshold:
            return []

        return [detection]

    def vision_detector_select_best(
        self,
        detections: List[HumanDetection],
    ) -> Optional[HumanDetection]:
        if not detections:
            return None

        # En büyük bbox_area'lı olanı seç
        return max(detections, key=lambda d: d.bbox_area)

    def vision_detector_shutdown(self) -> None:
        self.model_loaded = False
