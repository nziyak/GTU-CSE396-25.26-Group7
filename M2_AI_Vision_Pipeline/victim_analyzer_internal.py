from __future__ import annotations

from human_detector_internal import HumanDetection
from camera_internal import CameraFrame
from ai_vision import VisionPriority, VisionResult, VisionStatus


class VictimAnalyzerInternal:
    def __init__(self) -> None:
        self.model_loaded = False

    def vision_analyzer_init(self) -> int:
        # Burada gerçek projede VLM / CNN backend yüklenir.
        self.model_loaded = True
        return 0

    def vision_analyzer_classify(
        self,
        frame: CameraFrame,
        target: HumanDetection,
        target_id: int,
    ) -> VisionResult:
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
        if status == VisionStatus.TRAPPED:
            return VisionPriority.RED
        if status == VisionStatus.LYING:
            return VisionPriority.YELLOW
        if status == VisionStatus.STANDING:
            return VisionPriority.GREEN
        return VisionPriority.GREEN

    def vision_analyzer_shutdown(self) -> None:
        self.model_loaded = False
