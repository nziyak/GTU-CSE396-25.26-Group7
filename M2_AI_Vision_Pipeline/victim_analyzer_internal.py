"""
@file      victim_analyzer_internal.py
@brief     Internal victim severity analysis layer for the Vision & AI Pipeline
@author    Gabil Rahimli
@date      2026-03-29
@version   0.1

Changelog:
v0.1 - Initial draft, defined victim severity analysis helper methods and priority mapping.
"""

from __future__ import annotations

from human_detector_internal import HumanDetection
from camera_internal import CameraFrame
from ai_vision import VisionPriority, VisionResult, VisionStatus


class VictimAnalyzerInternal:
    """
    @brief Internal helper class responsible for victim severity classification,
           priority mapping, and analyzer shutdown.
    """

    def __init__(self) -> None:
        """
        @brief Creates the internal victim analyzer object with default unloaded state.
        """
        self.model_loaded = False

    def vision_analyzer_init(self) -> int:
        """
        @brief Initializes the victim severity analysis backend.
        @return 0 on success, negative value on failure.
        """
        # Burada gerçek projede VLM / CNN backend yüklenir.
        self.model_loaded = True
        return 0

    def vision_analyzer_classify(
        self,
        frame: CameraFrame,
        target: HumanDetection,
        target_id: int,
    ) -> VisionResult:
        """
        @brief Classifies the selected human target as standing, lying, or trapped.
        @param frame Full camera frame containing the target.
        @param target Selected human detection candidate.
        @param target_id Unique ID assigned to the detected target.
        @return VisionResult object containing final victim analysis output.
        """
        if not self.model_loaded:
            raise RuntimeError("Victim analyzer is not initialized.")

        # Dummy classification:
        # Gerçek projede burada crop alınır, VLM/CNN çalıştırılır.
        status = VisionStatus.LYING
        priority = self.vision_analyzer_map_priority(status)

        return VisionResult(
            target_id=target_id,
            status=status,
            priority=priority,
            confidence=target.confidence,
            bbox_area=target.bbox_area,
            target_detected=True,
        )

    def vision_analyzer_map_priority(self, status: VisionStatus) -> VisionPriority:
        """
        @brief Maps victim severity status to visualization / priority color.
        @param status Victim severity output from the analyzer.
        @return Corresponding VisionPriority value.
        """
        if status == VisionStatus.TRAPPED:
            return VisionPriority.RED
        if status == VisionStatus.LYING:
            return VisionPriority.YELLOW
        if status == VisionStatus.STANDING:
            return VisionPriority.GREEN
        return VisionPriority.GREEN

    def vision_analyzer_shutdown(self) -> None:
        """
        @brief Releases analyzer resources and marks the backend as unloaded.
        """
        self.model_loaded = False
