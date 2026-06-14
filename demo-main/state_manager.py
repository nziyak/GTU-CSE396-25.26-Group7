import threading

# Import the shared telemetry dataclass
import sys
import os

# Ensure MOD-04 is in path so we can import the interface
REPO_ROOT = os.path.dirname(__file__)
if os.path.join(REPO_ROOT, "MOD-04_Web_STT") not in sys.path:
    sys.path.append(os.path.join(REPO_ROOT, "MOD-04_Web_STT"))

from comms_dashboard_interface import AugmentedStatusReport

class RobotStateManager:
    """
    Thread-safe central state manager for merging telemetry from all modules 
    (STM32, Vision, Acoustics) before broadcasting to Unity.
    """
    def __init__(self):
        self._lock = threading.Lock()
        self._state = AugmentedStatusReport(
            pos_x=0.0,
            pos_y=0.0,
            temperature=20.0,
            smoke_detected=False,
            victim_status="NONE",
            is_stuck=False,
            priority_level=0,
            acoustic_hit=False,
            acoustic_angle=0.0
        )

    def update_from_stm32(self, temp: float, smoke: bool, pos_x: float, pos_y: float):
        with self._lock:
            self._state.temperature = temp
            self._state.smoke_detected = smoke
            self._state.pos_x = pos_x
            self._state.pos_y = pos_y

    def update_from_vision(self, victim_status: str, priority_level: int):
        with self._lock:
            self._state.victim_status = victim_status
            self._state.priority_level = priority_level

    def update_from_acoustic(self, acoustic_hit: bool, acoustic_angle: float):
        with self._lock:
            self._state.acoustic_hit = acoustic_hit
            self._state.acoustic_angle = acoustic_angle

    def update_stuck_status(self, is_stuck: bool):
        with self._lock:
            self._state.is_stuck = is_stuck

    def get_report(self) -> AugmentedStatusReport:
        with self._lock:
            # Return a new instance to prevent thread mutation of the broadcasted object
            return AugmentedStatusReport(
                pos_x=self._state.pos_x,
                pos_y=self._state.pos_y,
                temperature=self._state.temperature,
                smoke_detected=self._state.smoke_detected,
                victim_status=self._state.victim_status,
                is_stuck=self._state.is_stuck,
                priority_level=self._state.priority_level,
                acoustic_hit=self._state.acoustic_hit,
                acoustic_angle=self._state.acoustic_angle
            )
