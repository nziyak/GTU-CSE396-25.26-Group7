"""
@file      ai_vision.py
@brief     Concrete MOD-02 vision pipeline implementation
"""

from __future__ import annotations

import copy
from dataclasses import dataclass
import logging
import os
import threading
import time
from typing import Dict, Optional

from ai_vision_interface import IVisionPipeline, TargetData
from camera_internal import CameraFrame, CameraInternal
from human_detector_internal import HumanDetection, HumanDetectorInternal
from victim_analyzer_internal import VictimAnalyzerInternal
from vision_types_internal import VisionPriority, VisionResult, VisionStatus

try:
    import cv2  # type: ignore[import-not-found]
except Exception:  # pragma: no cover - optional dependency
    cv2 = None


logger = logging.getLogger("MOD02.ai_vision")


@dataclass
class VisionPipelineConfig:
    """Runtime configuration for the MOD-02 pipeline."""

    exploration_fps: int = 5
    assessment_fps: int = 12
    detection_threshold: float = 0.35
    allow_mock_camera: bool = True
    camera_source: Optional[str] = None
    detector_model_path: Optional[str] = None
    severity_model_path: Optional[str] = None
    class_names_path: Optional[str] = None
    frame_width: int = 640
    frame_height: int = 480


class VisionPipeline(IVisionPipeline):
    """
    Public implementation consumed by MOD-04 and higher-level control.
    Runs a background loop and caches the latest target result.
    """

    def __init__(self, config: Optional[VisionPipelineConfig] = None) -> None:
        self.config = config or self._build_default_config()

        self.camera = CameraInternal(
            device_source=self.config.camera_source,
            frame_width=self.config.frame_width,
            frame_height=self.config.frame_height,
            allow_mock_camera=self.config.allow_mock_camera,
        )
        self.detector = HumanDetectorInternal(model_path=self.config.detector_model_path)
        self.analyzer = VictimAnalyzerInternal(
            model_path=self.config.severity_model_path,
            class_names_path=self.config.class_names_path,
        )

        self._latest_target: Optional[TargetData] = None
        self._latest_result: Optional[VisionResult] = None
        self._latest_frame_jpeg: Optional[bytes] = None
        self._latest_frame_timestamp: float = 0.0

        self._target_id_counter = 1
        self._latest_lock = threading.Lock()
        self._pause_event = threading.Event()
        self._stop_event = threading.Event()
        self._worker_thread: Optional[threading.Thread] = None
        self._initialized = False

    def initialize_camera(self) -> bool:
        """Warms up the camera and loads the inference backends."""
        if self._initialized:
            return True

        if self.camera.vision_camera_init(self.config.exploration_fps) != 0:
            logger.error("MOD-02 camera initialization failed.")
            return False

        if self.detector.vision_detector_init() != 0:
            logger.error("MOD-02 human detector initialization failed.")
            self.camera.vision_camera_shutdown()
            return False

        if self.analyzer.vision_analyzer_init() != 0:
            logger.error("MOD-02 victim analyzer initialization failed.")
            self.detector.vision_detector_shutdown()
            self.camera.vision_camera_shutdown()
            return False

        self._stop_event.clear()
        self._pause_event.clear()
        self._worker_thread = threading.Thread(
            target=self._run_loop,
            name="MOD02VisionWorker",
            daemon=True,
        )
        self._worker_thread.start()
        self._initialized = True
        return True

    def get_latest_target(self) -> Optional[TargetData]:
        """Retrieves the most recent AI classification result."""
        with self._latest_lock:
            return copy.deepcopy(self._latest_target)

    def pause_vision_pipeline(self) -> None:
        """Pauses camera framing and AI inference."""
        self._pause_event.set()
        self.camera.vision_camera_set_fps(1)

    def resume_vision_pipeline(self) -> None:
        """Resumes AI inference after a pause."""
        self.camera.vision_camera_set_fps(self.config.exploration_fps)
        self._pause_event.clear()

    def shutdown(self) -> None:
        """Stops the worker and releases camera / model resources."""
        self._stop_event.set()
        if self._worker_thread is not None and self._worker_thread.is_alive():
            self._worker_thread.join(timeout=2.0)
        self.analyzer.vision_analyzer_shutdown()
        self.detector.vision_detector_shutdown()
        self.camera.vision_camera_shutdown()
        self._initialized = False

    def get_latest_frame_jpeg(self) -> Optional[bytes]:
        """Optional helper for MOD-04 video streaming."""
        with self._latest_lock:
            if self._latest_frame_jpeg is None:
                return None
            return bytes(self._latest_frame_jpeg)

    def build_dashboard_fields(self) -> Dict[str, object]:
        """Normalized status fields directly compatible with MOD-04 telemetry."""
        target = self.get_latest_target()
        return {
            "victim_status": self.target_to_victim_status(target),
            "priority_level": self.target_to_priority_level(target),
        }

    def build_augmented_status_report(
        self,
        *,
        pos_x: float,
        pos_y: float,
        temperature: float,
        smoke_detected: bool,
        is_stuck: bool,
        acoustic_hit: bool,
        acoustic_angle: float,
    ):
        """Convenience adapter for MOD-04/MOD-05 integration."""
        target = self.get_latest_target()

        try:
            from comms_dashboard_interface import AugmentedStatusReport
        except Exception:
            return {
                "posX": pos_x,
                "posY": pos_y,
                "temperature": temperature,
                "smokeDetected": smoke_detected,
                "victimStatus": self.target_to_victim_status(target),
                "isStuck": is_stuck,
                "priorityLevel": self.target_to_priority_level(target),
                "acousticHit": acoustic_hit,
                "acousticAngle": acoustic_angle,
            }

        return AugmentedStatusReport(
            pos_x=pos_x,
            pos_y=pos_y,
            temperature=temperature,
            smoke_detected=smoke_detected,
            victim_status=self.target_to_victim_status(target),
            is_stuck=is_stuck,
            priority_level=self.target_to_priority_level(target),
            acoustic_hit=acoustic_hit,
            acoustic_angle=acoustic_angle,
        )

    @staticmethod
    def target_to_victim_status(target: Optional[TargetData]) -> str:
        if target is None:
            return VisionStatus.NONE.value
        return str(target.severity).upper()

    @staticmethod
    def target_to_priority_level(target: Optional[TargetData]) -> int:
        if target is None:
            return int(VisionPriority.NONE)
        severity = str(target.severity).upper()
        if severity == VisionStatus.TRAPPED.value:
            return int(VisionPriority.RED)
        if severity == VisionStatus.LYING.value:
            return int(VisionPriority.YELLOW)
        if severity == VisionStatus.STANDING.value:
            return int(VisionPriority.GREEN)
        return int(VisionPriority.NONE)

    def _run_loop(self) -> None:
        while not self._stop_event.is_set():
            if self._pause_event.is_set():
                time.sleep(0.05)
                continue

            cycle_start = time.monotonic()
            frame = self.camera.vision_camera_capture_frame()
            if frame is None:
                time.sleep(0.1)
                continue

            try:
                detections = self.detector.vision_detector_detect(frame, self.config.detection_threshold)
                best_target = self.detector.vision_detector_select_best(detections)
                if best_target is None:
                    self._publish_no_target(frame)
                else:
                    result = self.analyzer.vision_analyzer_classify(frame, best_target, self._target_id_counter)
                    self._target_id_counter += 1
                    self._publish_target(frame, best_target, result)
            except Exception as exc:
                logger.exception("MOD-02 pipeline cycle failed: %s", exc)
                self._publish_no_target(frame)
            finally:
                self.camera.vision_camera_release_frame(frame)

            desired_interval = 1.0 / max(self.camera.fps, 1)
            remaining = desired_interval - (time.monotonic() - cycle_start)
            if remaining > 0:
                time.sleep(remaining)

    def _publish_target(
        self,
        frame: CameraFrame,
        detection: HumanDetection,
        result: VisionResult,
    ) -> None:
        target = TargetData(
            pos_x=result.pos_x,
            pos_y=result.pos_y,
            distance_cm=result.distance_cm,
            severity=result.status.value,
            confidence=result.confidence,
        )

        frame_bytes = self._encode_preview_frame(frame, detection, target)
        with self._latest_lock:
            self._latest_target = target
            self._latest_result = result
            self._latest_frame_jpeg = frame_bytes
            self._latest_frame_timestamp = time.time()

        self.camera.vision_camera_set_fps(self.config.assessment_fps)

    def _publish_no_target(self, frame: CameraFrame) -> None:
        frame_bytes = self._encode_preview_frame(frame, None, None)
        with self._latest_lock:
            self._latest_target = None
            self._latest_result = None
            self._latest_frame_jpeg = frame_bytes
            self._latest_frame_timestamp = time.time()

        self.camera.vision_camera_set_fps(self.config.exploration_fps)

    def _encode_preview_frame(
        self,
        frame: CameraFrame,
        detection: Optional[HumanDetection],
        target: Optional[TargetData],
    ) -> Optional[bytes]:
        if cv2 is None or frame.data is None:
            return None

        preview = frame.data.copy()
        if detection is not None:
            x1 = detection.bbox.x
            y1 = detection.bbox.y
            x2 = detection.bbox.x + detection.bbox.width
            y2 = detection.bbox.y + detection.bbox.height
            cv2.rectangle(preview, (x1, y1), (x2, y2), (0, 255, 0), 2)

        if target is not None:
            label = f"{target.severity} {target.confidence:.2f}"
            origin = (max(8, target.pos_x - 60), max(24, target.pos_y - 8))
            cv2.putText(
                preview,
                label,
                origin,
                cv2.FONT_HERSHEY_SIMPLEX,
                0.55,
                (0, 255, 255),
                2,
                cv2.LINE_AA,
            )

        ok, encoded = cv2.imencode(".jpg", preview)
        if not ok:
            return None
        return encoded.tobytes()

    def _build_default_config(self) -> VisionPipelineConfig:
        module_dir = os.path.dirname(__file__)
        repo_root = os.path.dirname(module_dir)

        return VisionPipelineConfig(
            exploration_fps=int(os.getenv("MOD02_EXPLORATION_FPS", "5")),
            assessment_fps=int(os.getenv("MOD02_ASSESSMENT_FPS", "12")),
            detection_threshold=float(os.getenv("MOD02_DETECTION_THRESHOLD", "0.35")),
            allow_mock_camera=os.getenv("MOD02_ALLOW_MOCK_CAMERA", "1") != "0",
            camera_source=os.getenv("MOD02_CAMERA_SOURCE"),
            detector_model_path=self._find_existing_file(
                explicit=os.getenv("MOD02_DETECTOR_MODEL_PATH"),
                candidates=(
                    os.path.join(repo_root, "best.pt"),
                    os.path.join(repo_root, "person_detector.pt"),
                    os.path.join(module_dir, "best.pt"),
                ),
            ),
            severity_model_path=self._find_existing_file(
                explicit=os.getenv("MOD02_SEVERITY_MODEL_PATH"),
                candidates=(
                    os.path.join(repo_root, "severity_model.keras"),
                    os.path.join(repo_root, "severity_model.h5"),
                    os.path.join(module_dir, "severity_model.keras"),
                    os.path.join(module_dir, "severity_model.h5"),
                    os.path.join(repo_root, "severity_model.pt"),
                    os.path.join(module_dir, "severity_model.pt"),
                ),
            ),
            class_names_path=self._find_existing_file(
                explicit=os.getenv("MOD02_CLASS_NAMES_PATH"),
                candidates=(
                    os.path.join(repo_root, "class_names.json"),
                    os.path.join(module_dir, "class_names.json"),
                ),
            ),
        )

    @staticmethod
    def _find_existing_file(*, explicit: Optional[str], candidates) -> Optional[str]:
        if explicit:
            return explicit
        for candidate in candidates:
            if candidate and os.path.exists(candidate):
                return candidate
        return None


__all__ = [
    "VisionPipeline",
    "VisionPipelineConfig",
    "VisionPriority",
    "VisionResult",
    "VisionStatus",
]
