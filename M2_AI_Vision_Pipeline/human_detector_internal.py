"""
@file      human_detector_internal.py
@brief     Internal YOLO-based human detection layer for the Vision & AI Pipeline
@author    Gabil Rahimli
@date      2026-03-29
@version   0.1

Changelog:
v0.1 - Initial draft, defined human detection data classes and detector helper methods.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import List, Optional

from camera_internal import CameraFrame


@dataclass
class CameraBBox:
    """
    @brief Represents the bounding box of a detected human target.
    """
    x: int
    y: int
    width: int
    height: int


@dataclass
class HumanDetection:
    """
    @brief Stores one human detection candidate with confidence and bounding box data.
    """
    valid: bool
    confidence: float
    bbox: CameraBBox
    bbox_area: int


class HumanDetectorInternal:
    """
    @brief Internal helper class responsible for initializing the detector backend,
           detecting humans in frames, selecting the best candidate, and shutting down the detector.
    """

    def __init__(self) -> None:
        """
        @brief Creates the internal human detector object with default unloaded state.
        """
        self.model_loaded = False

    def vision_detector_init(self) -> int:
        """
        @brief Initializes the human detection backend and loads the detector model.
        @return 0 on success, negative value on failure.
        """
        # Burada gerçek projede YOLO modeli yüklenir.
        self.model_loaded = True
        return 0

    def vision_detector_detect(
        self,
        frame: CameraFrame,
        threshold: float,
    ) -> List[HumanDetection]:
        """
        @brief Runs human detection on the given frame.
        @param frame Input camera frame to analyze.
        @param threshold Minimum confidence threshold for valid detections.
        @return List of HumanDetection objects that satisfy the threshold condition.
        """
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
        """
        @brief Selects the best detection candidate from a list of detections.
        @param detections List of detected human candidates.
        @return Best HumanDetection object if available, None otherwise.
        """
        if not detections:
            return None

        # En büyük bbox_area'lı olanı seç
        return max(detections, key=lambda d: d.bbox_area)

    def vision_detector_shutdown(self) -> None:
        """
        @brief Releases detector resources and marks the backend as unloaded.
        """
        self.model_loaded = False
