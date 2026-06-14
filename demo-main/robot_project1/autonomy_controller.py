from __future__ import annotations

from dataclasses import dataclass
from typing import Optional

from ai_vision_interface import TargetData
from uart_reader import UartTelemetry


@dataclass
class AutonomyDecision:
    mode: str
    command: str
    obstacle_detected: bool
    target_locked: bool
    target_severity: str
    reason: str


class AutonomyController:
    def __init__(
        self,
        *,
        frame_center_x: int = 320,
        center_tolerance_px: int = 70,
        obstacle_stop_cm: int = 18,
        target_stop_cm: int = 60,
    ) -> None:
        self.frame_center_x = frame_center_x
        self.center_tolerance_px = center_tolerance_px
        self.obstacle_stop_cm = obstacle_stop_cm
        self.target_stop_cm = target_stop_cm
        self._next_turn = "LEFT"

    def decide(self, telemetry: UartTelemetry, target: Optional[TargetData]) -> AutonomyDecision:
        front_distance = telemetry.dist_front
        obstacle_detected = front_distance > 0 and front_distance <= self.obstacle_stop_cm

        if obstacle_detected:
            turn_command = self._consume_turn_direction()
            return AutonomyDecision(
                mode="AUTO_AVOID_OBSTACLE",
                command=turn_command,
                obstacle_detected=True,
                target_locked=False,
                target_severity="NONE",
                reason=f"front_obstacle_{front_distance}cm",
            )

        if target is not None and str(target.severity).upper() in {"TRAPPED", "LYING"}:
            severity = str(target.severity).upper()

            if target.distance_cm <= self.target_stop_cm:
                return AutonomyDecision(
                    mode="AUTO_APPROACH_TARGET",
                    command="STOP",
                    obstacle_detected=False,
                    target_locked=True,
                    target_severity=severity,
                    reason=f"target_close_{target.distance_cm:.1f}cm",
                )

            if target.pos_x < self.frame_center_x - self.center_tolerance_px:
                return AutonomyDecision(
                    mode="AUTO_APPROACH_TARGET",
                    command="LEFT",
                    obstacle_detected=False,
                    target_locked=True,
                    target_severity=severity,
                    reason="target_left_of_center",
                )

            if target.pos_x > self.frame_center_x + self.center_tolerance_px:
                return AutonomyDecision(
                    mode="AUTO_APPROACH_TARGET",
                    command="RIGHT",
                    obstacle_detected=False,
                    target_locked=True,
                    target_severity=severity,
                    reason="target_right_of_center",
                )

            return AutonomyDecision(
                mode="AUTO_APPROACH_TARGET",
                command="FORWARD",
                obstacle_detected=False,
                target_locked=True,
                target_severity=severity,
                reason="target_centered",
            )

        return AutonomyDecision(
            mode="AUTO_PATROL",
            command="FORWARD",
            obstacle_detected=False,
            target_locked=False,
            target_severity=str(target.severity).upper() if target else "NONE",
            reason="clear_path_patrol",
        )

    def _consume_turn_direction(self) -> str:
        current = self._next_turn
        self._next_turn = "RIGHT" if current == "LEFT" else "LEFT"
        return current
