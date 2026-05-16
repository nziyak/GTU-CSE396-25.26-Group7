"""
@file      victim_analyzer_internal.py
@brief     Internal victim severity analysis layer for the Vision & AI Pipeline
@author    Gabil Rahimli
@date      2026-03-29
@version   0.2
"""

from __future__ import annotations

import json
import os
from typing import List, Optional, Tuple

from camera_internal import CameraFrame
from human_detector_internal import HumanDetection
from vision_types_internal import VisionPriority, VisionResult, VisionStatus

try:
    import cv2  # type: ignore[import-not-found]
except Exception:  # pragma: no cover - optional dependency
    cv2 = None

try:
    import numpy as np
except Exception:  # pragma: no cover - optional dependency
    np = None

try:
    import tensorflow as tf  # type: ignore[import-not-found]
    from tensorflow.keras.applications.mobilenet_v2 import preprocess_input  # type: ignore[import-not-found]
except Exception:  # pragma: no cover - optional dependency
    tf = None
    preprocess_input = None

try:
    import torch  # type: ignore[import-not-found]
except Exception:  # pragma: no cover - optional dependency
    torch = None

try:
    from ultralytics import YOLO  # type: ignore[import-not-found]
except Exception:  # pragma: no cover - optional dependency
    YOLO = None


class VictimAnalyzerInternal:
    """
    Internal helper responsible for victim severity classification,
    priority mapping, and analyzer shutdown.
    """

    def __init__(
        self,
        model_path: Optional[str] = None,
        class_names_path: Optional[str] = None,
        input_size: Tuple[int, int] = (160, 160),
    ) -> None:
        self.model_loaded = False
        self.model_path = model_path or os.getenv("MOD02_SEVERITY_MODEL_PATH")
        self.class_names_path = class_names_path or os.getenv("MOD02_CLASS_NAMES_PATH")
        self.input_size = input_size
        self.class_names = ["standing", "lying", "trapped"]
        self._backend = "unloaded"
        self._model = None

    def vision_analyzer_init(self) -> int:
        """Initializes the victim severity analysis backend."""
        self.class_names = self._load_class_names()

        if self.model_path and os.path.exists(self.model_path):
            suffix = os.path.splitext(self.model_path)[1].lower()
            if suffix in {".keras", ".h5"} and tf is not None:
                self._model = tf.keras.models.load_model(self.model_path)
                self._backend = "keras"
            elif suffix in {".pt", ".pth"} and YOLO is not None:
                self._model = YOLO(self.model_path)
                self._backend = "yolo-cls"
            elif suffix in {".pt", ".pth"} and torch is not None:
                self._model = torch.jit.load(self.model_path, map_location="cpu")
                self._model.eval()
                self._backend = "torchscript"

        if self._backend == "unloaded":
            self._backend = "heuristic"

        self.model_loaded = True
        return 0

    def vision_analyzer_classify(
        self,
        frame: CameraFrame,
        target: HumanDetection,
        target_id: int,
    ) -> VisionResult:
        """Classifies the selected human target as standing, lying, or trapped."""
        if not self.model_loaded:
            raise RuntimeError("Victim analyzer is not initialized.")

        crop = self._crop_target(frame, target)
        status, class_confidence = self._classify_crop(crop, frame, target)
        priority = self.vision_analyzer_map_priority(status)
        center_x = target.bbox.x + (target.bbox.width // 2)
        center_y = target.bbox.y + (target.bbox.height // 2)
        combined_confidence = max(0.0, min(1.0, (target.confidence + class_confidence) / 2.0))

        return VisionResult(
            target_id=target_id,
            status=status,
            priority=priority,
            confidence=combined_confidence,
            bbox_area=target.bbox_area,
            target_detected=True,
            pos_x=center_x,
            pos_y=center_y,
            distance_cm=self._estimate_distance_cm(target),
        )

    def vision_analyzer_map_priority(self, status: VisionStatus) -> VisionPriority:
        """Maps victim severity status to visualization / priority color."""
        if status == VisionStatus.TRAPPED:
            return VisionPriority.RED
        if status == VisionStatus.LYING:
            return VisionPriority.YELLOW
        if status == VisionStatus.STANDING:
            return VisionPriority.GREEN
        return VisionPriority.NONE

    def vision_analyzer_shutdown(self) -> None:
        """Releases analyzer resources and marks the backend as unloaded."""
        self.model_loaded = False
        self._backend = "unloaded"
        self._model = None

    def _load_class_names(self) -> List[str]:
        if self.class_names_path and os.path.exists(self.class_names_path):
            with open(self.class_names_path, "r", encoding="utf-8") as handle:
                loaded = json.load(handle)
            if isinstance(loaded, list) and loaded:
                return [str(item) for item in loaded]
        return ["standing", "lying", "trapped"]

    def _crop_target(self, frame: CameraFrame, target: HumanDetection):
        if frame.data is None or np is None:
            return None

        x1 = max(0, target.bbox.x)
        y1 = max(0, target.bbox.y)
        x2 = min(frame.width, target.bbox.x + target.bbox.width)
        y2 = min(frame.height, target.bbox.y + target.bbox.height)

        if x2 <= x1 or y2 <= y1:
            return None

        crop = frame.data[y1:y2, x1:x2]
        if crop.size == 0:
            return None
        return crop

    def _classify_crop(
        self,
        crop,
        frame: CameraFrame,
        target: HumanDetection,
    ) -> Tuple[VisionStatus, float]:
        if crop is None:
            return self._heuristic_classify(frame, target)

        if self._backend == "keras":
            return self._classify_with_keras(crop, frame, target)
        if self._backend == "yolo-cls":
            return self._classify_with_yolo(crop, frame, target)
        if self._backend == "torchscript":
            return self._classify_with_torchscript(crop, frame, target)
        return self._heuristic_classify(frame, target)

    def _classify_with_keras(
        self,
        crop,
        frame: CameraFrame,
        target: HumanDetection,
    ) -> Tuple[VisionStatus, float]:
        if tf is None or np is None:
            return self._heuristic_classify(frame, target)

        resized = self._resize_crop(crop)
        if resized is None:
            return self._heuristic_classify(frame, target)

        batch = np.expand_dims(resized.astype("float32"), axis=0)
        if preprocess_input is not None:
            batch = preprocess_input(batch)

        predictions = self._model.predict(batch, verbose=0)[0]
        index = int(np.argmax(predictions))
        confidence = float(predictions[index])
        label = self.class_names[index] if index < len(self.class_names) else "NONE"
        return self._normalize_status(label), confidence

    def _classify_with_yolo(
        self,
        crop,
        frame: CameraFrame,
        target: HumanDetection,
    ) -> Tuple[VisionStatus, float]:
        results = self._model.predict(source=crop, verbose=False)
        if not results:
            return self._heuristic_classify(frame, target)

        result = results[0]
        probs = getattr(result, "probs", None)
        if probs is None:
            return self._heuristic_classify(frame, target)

        top1 = int(probs.top1)
        confidence = float(probs.top1conf)
        names = getattr(result, "names", {}) or {}
        label = names.get(top1, self.class_names[top1] if top1 < len(self.class_names) else "NONE")
        return self._normalize_status(label), confidence

    def _classify_with_torchscript(
        self,
        crop,
        frame: CameraFrame,
        target: HumanDetection,
    ) -> Tuple[VisionStatus, float]:
        if torch is None or np is None:
            return self._heuristic_classify(frame, target)

        resized = self._resize_crop(crop)
        if resized is None:
            return self._heuristic_classify(frame, target)

        rgb = resized[:, :, ::-1].copy()
        tensor = torch.from_numpy(rgb).permute(2, 0, 1).float().unsqueeze(0) / 255.0
        with torch.no_grad():
            logits = self._model(tensor)
            if isinstance(logits, (tuple, list)):
                logits = logits[0]
            probabilities = torch.softmax(logits, dim=1)[0]
            index = int(torch.argmax(probabilities).item())
            confidence = float(probabilities[index].item())

        label = self.class_names[index] if index < len(self.class_names) else "NONE"
        return self._normalize_status(label), confidence

    def _heuristic_classify(
        self,
        frame: CameraFrame,
        target: HumanDetection,
    ) -> Tuple[VisionStatus, float]:
        aspect_ratio = target.bbox.width / max(target.bbox.height, 1)
        frame_area = max(frame.width * frame.height, 1)
        area_ratio = target.bbox_area / frame_area

        if area_ratio >= 0.28 and 0.75 <= aspect_ratio <= 1.35:
            return VisionStatus.TRAPPED, 0.55
        if aspect_ratio >= 1.15:
            return VisionStatus.LYING, 0.60
        return VisionStatus.STANDING, 0.65

    def _estimate_distance_cm(self, target: HumanDetection) -> float:
        reference_person_height_cm = 170.0
        focal_length_px = 700.0
        bbox_height = max(target.bbox.height, 1)
        distance_cm = (reference_person_height_cm * focal_length_px) / float(bbox_height)
        return round(distance_cm, 2)

    def _normalize_status(self, label: str) -> VisionStatus:
        normalized = str(label).strip().upper()
        alias_map = {
            "NONE": VisionStatus.NONE,
            "STANDING": VisionStatus.STANDING,
            "STAND": VisionStatus.STANDING,
            "UPRIGHT": VisionStatus.STANDING,
            "LYING": VisionStatus.LYING,
            "LAYING": VisionStatus.LYING,
            "DOWN": VisionStatus.LYING,
            "TRAPPED": VisionStatus.TRAPPED,
            "ENTRAPPED": VisionStatus.TRAPPED,
        }
        return alias_map.get(normalized, VisionStatus.NONE)

    def _resize_crop(self, crop):
        if cv2 is not None:
            return cv2.resize(crop, self.input_size)
        if tf is not None:
            return tf.image.resize(crop, self.input_size).numpy()
        return None
