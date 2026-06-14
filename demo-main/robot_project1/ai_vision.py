from __future__ import annotations

import copy
import json
import logging
import threading
import time
from collections import deque
from dataclasses import dataclass
from typing import Optional

import cv2
import numpy as np
import onnxruntime as ort
from picamera2 import Picamera2
from ultralytics import YOLO

from ai_vision_interface import IVisionPipeline, TargetData


logger = logging.getLogger("ai_vision")


@dataclass
class VisionPipelineConfig:
    exploration_fps: int = 5
    detection_threshold: float = 0.35
    frame_width: int = 640
    frame_height: int = 480
    detector_model_path: str = "yolov8n.pt"
    severity_model_path: str = "severity_model.onnx"
    class_names_path: str = "class_names.json"


class VisionPipeline(IVisionPipeline):
    DISTANCE_K = 30000.0
    CROP_EXPAND_RATIO = 0.35
    IMG_SIZE = (160, 160)
    DECISION_WINDOW_SECONDS = 2.0
    TRAPPED_MIN_PROB = 0.35
    TRAPPED_MAX_GAP_FROM_BEST = 0.20
    TRAPPED_PROTECTION_PROB = 0.30

    def __init__(self, config: Optional[VisionPipelineConfig] = None) -> None:
        self.config = config or VisionPipelineConfig()

        self._latest_target: Optional[TargetData] = None
        self._latest_frame_jpeg: Optional[bytes] = None
        self._latest_lock = threading.Lock()

        self._pause_event = threading.Event()
        self._stop_event = threading.Event()
        self._worker_thread: Optional[threading.Thread] = None
        self._initialized = False

        self._prediction_history = deque()
        self._picam2 = None
        self._yolo_model = None
        self._session = None
        self._input_name = None
        self._class_names = []

    def initialize_camera(self) -> bool:
        if self._initialized:
            return True

        try:
            self._yolo_model = YOLO(self.config.detector_model_path)
            self._session = ort.InferenceSession(
                self.config.severity_model_path,
                providers=["CPUExecutionProvider"],
            )
            self._input_name = self._session.get_inputs()[0].name

            with open(self.config.class_names_path, "r", encoding="utf-8") as handle:
                self._class_names = json.load(handle)

            self._picam2 = Picamera2()
            self._picam2.configure(
                self._picam2.create_preview_configuration(
                    main={
                        "format": "RGB888",
                        "size": (self.config.frame_width, self.config.frame_height),
                    }
                )
            )
            self._picam2.start()
            time.sleep(2)

            self._stop_event.clear()
            self._pause_event.clear()
            self._worker_thread = threading.Thread(target=self._run_loop, name="VisionLoop", daemon=True)
            self._worker_thread.start()
            self._initialized = True
            return True
        except Exception as exc:
            logger.exception("Vision init failed: %s", exc)
            return False

    def get_latest_target(self) -> Optional[TargetData]:
        with self._latest_lock:
            return copy.deepcopy(self._latest_target)

    def pause_vision_pipeline(self) -> None:
        self._pause_event.set()

    def resume_vision_pipeline(self) -> None:
        self._pause_event.clear()

    def shutdown(self) -> None:
        self._stop_event.set()
        if self._worker_thread and self._worker_thread.is_alive():
            self._worker_thread.join(timeout=2.0)

        if self._picam2 is not None:
            try:
                self._picam2.stop()
            except Exception:
                pass

    def get_latest_frame_jpeg(self) -> Optional[bytes]:
        with self._latest_lock:
            if self._latest_frame_jpeg is None:
                return None
            return bytes(self._latest_frame_jpeg)

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
        from comms_dashboard_interface import AugmentedStatusReport

        target = self.get_latest_target()
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
            return "NONE"
        return str(target.severity).upper()

    @staticmethod
    def target_to_priority_level(target: Optional[TargetData]) -> int:
        if target is None:
            return 0
        severity = str(target.severity).upper()
        if severity == "TRAPPED":
            return 1
        if severity == "LYING":
            return 2
        if severity == "STANDING":
            return 3
        return 0

    def _run_loop(self) -> None:
        while not self._stop_event.is_set():
            if self._pause_event.is_set():
                time.sleep(0.05)
                continue

            try:
                frame_rgb = self._picam2.capture_array()
                frame = cv2.cvtColor(frame_rgb, cv2.COLOR_RGB2BGR)
                img_h, img_w = frame.shape[:2]

                results = self._yolo_model(frame, verbose=False)
                best_person = self._select_best_person(results)

                if best_person is not None:
                    x1, y1, x2, y2, yolo_conf = best_person
                    ex1, ey1, ex2, ey2 = self._expand_bbox(x1, y1, x2, y2, img_w, img_h)
                    crop = frame[ey1:ey2, ex1:ex2]

                    if crop.size > 0:
                        preds = self._predict_severity_probs(crop)
                        self._update_prediction_history(preds)
                        severity, cnn_conf, stable_probs = self._get_stable_prediction()
                        severity, cnn_conf, bbox_ratio, _ = self._apply_bbox_posture_correction(
                            severity,
                            cnn_conf,
                            stable_probs,
                            x1,
                            y1,
                            x2,
                            y2,
                        )

                        distance_cm = self._estimate_distance_cm(y1, y2)
                        final_conf = (float(yolo_conf) + float(cnn_conf)) / 2.0

                        target = TargetData(
                            pos_x=int((x1 + x2) / 2),
                            pos_y=int((y1 + y2) / 2),
                            distance_cm=round(float(distance_cm), 2),
                            severity=severity.upper(),
                            confidence=round(float(final_conf), 3),
                        )

                        color = self._severity_to_color(severity)
                        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
                        cv2.putText(
                            frame,
                            f"{severity.upper()} {final_conf:.2f}",
                            (x1, max(30, y1 - 10)),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            0.65,
                            color,
                            2,
                        )
                        cv2.putText(
                            frame,
                            f"D:{distance_cm:.1f}cm ratio:{bbox_ratio:.2f}",
                            (x1, min(img_h - 20, y2 + 25)),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            0.55,
                            color,
                            2,
                        )

                        ok, buffer = cv2.imencode(".jpg", frame)
                        if ok:
                            with self._latest_lock:
                                self._latest_target = target
                                self._latest_frame_jpeg = buffer.tobytes()
                else:
                    self._prediction_history.clear()
                    cv2.putText(
                        frame,
                        "NO PERSON DETECTED",
                        (20, 40),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.8,
                        (255, 255, 255),
                        2,
                    )
                    ok, buffer = cv2.imencode(".jpg", frame)
                    if ok:
                        with self._latest_lock:
                            self._latest_target = None
                            self._latest_frame_jpeg = buffer.tobytes()

            except Exception as exc:
                logger.exception("Vision loop failed: %s", exc)
                time.sleep(0.1)

            time.sleep(1.0 / max(self.config.exploration_fps, 1))

    def _expand_bbox(self, x1, y1, x2, y2, img_w, img_h):
        width = x2 - x1
        height = y2 - y1
        pad_x = int(width * self.CROP_EXPAND_RATIO)
        pad_y = int(height * self.CROP_EXPAND_RATIO)
        nx1 = max(0, x1 - pad_x)
        ny1 = max(0, y1 - pad_y)
        nx2 = min(img_w, x2 + pad_x)
        ny2 = min(img_h, y2 + pad_y)
        return nx1, ny1, nx2, ny2

    def _estimate_distance_cm(self, y1, y2):
        bbox_height = max(1, y2 - y1)
        return self.DISTANCE_K / bbox_height

    def _predict_severity_probs(self, crop_bgr):
        resized = cv2.resize(crop_bgr, self.IMG_SIZE)
        rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
        arr = np.expand_dims(rgb.astype(np.float32), axis=0)
        preds = self._session.run(None, {self._input_name: arr})[0][0]
        return preds

    def _update_prediction_history(self, preds):
        now = time.time()
        self._prediction_history.append((now, preds))
        while self._prediction_history and now - self._prediction_history[0][0] > self.DECISION_WINDOW_SECONDS:
            self._prediction_history.popleft()

    def _get_stable_prediction(self):
        if not self._prediction_history:
            return "none", 0.0, {}

        all_preds = np.array([item[1] for item in self._prediction_history])
        avg_preds = np.mean(all_preds, axis=0)

        probs = {self._class_names[i]: round(float(avg_preds[i]), 3) for i in range(len(self._class_names))}
        class_id = int(np.argmax(avg_preds))
        severity = self._class_names[class_id]
        confidence = float(avg_preds[class_id])

        if "trapped" in self._class_names:
            trapped_idx = self._class_names.index("trapped")
            trapped_prob = float(avg_preds[trapped_idx])
            best_prob = float(np.max(avg_preds))
            if trapped_prob >= self.TRAPPED_MIN_PROB and (best_prob - trapped_prob) <= self.TRAPPED_MAX_GAP_FROM_BEST:
                severity = "trapped"
                confidence = trapped_prob

        return severity, confidence, probs

    def _select_best_person(self, results):
        best_box = None
        best_score = 0.0
        for result in results:
            for box in result.boxes:
                cls_id = int(box.cls[0])
                conf = float(box.conf[0])
                if cls_id != 0 or conf < self.config.detection_threshold:
                    continue

                x1, y1, x2, y2 = map(int, box.xyxy[0])
                area = max(1, (x2 - x1) * (y2 - y1))
                score = conf * area
                if score > best_score:
                    best_score = score
                    best_box = (x1, y1, x2, y2, conf)
        return best_box

    def _apply_bbox_posture_correction(self, severity, cnn_conf, stable_probs, x1, y1, x2, y2):
        width = max(1, x2 - x1)
        height = max(1, y2 - y1)
        ratio = width / height

        trapped_prob = stable_probs.get("trapped", 0.0)
        standing_prob = stable_probs.get("standing", 0.0)

        if ratio < 0.85 and standing_prob >= 0.20:
            return "standing", max(standing_prob, 0.82), ratio, "bbox_strong_corrected_to_standing"

        if severity == "trapped" or trapped_prob >= self.TRAPPED_PROTECTION_PROB:
            if ratio < 0.75 and standing_prob >= 0.25 and trapped_prob < 0.55:
                return "standing", max(standing_prob, 0.80), ratio, "standing_overrides_weak_trapped"
            return severity, cnn_conf, ratio, "trapped_protected"

        if ratio > 1.15 and cnn_conf < 0.80:
            return "lying", max(cnn_conf, 0.70), ratio, "bbox_corrected_to_lying"

        return severity, cnn_conf, ratio, "cnn_decision"

    def _severity_to_color(self, severity):
        sev = severity.lower()
        if sev == "trapped":
            return (0, 0, 255)
        if sev == "lying":
            return (0, 255, 255)
        if sev == "standing":
            return (0, 255, 0)
        return (255, 255, 255)
