from dataclasses import dataclass
from abc import ABC, abstractmethod
from typing import Dict, Any


@dataclass
class AugmentedStatusReport:
    pos_x: float
    pos_y: float
    temperature: float
    smoke_detected: bool
    victim_status: str
    is_stuck: bool
    priority_level: int
    acoustic_hit: bool
    acoustic_angle: float


class IWebDashboard(ABC):
    @abstractmethod
    def start_server(self, host: str = "0.0.0.0", port: int = 5001) -> None:
        pass

    @abstractmethod
    def broadcast_telemetry(self, report: AugmentedStatusReport) -> None:
        pass

    @abstractmethod
    def stream_video_frame(self, jpeg_bytes: bytes) -> None:
        pass

    @abstractmethod
    def on_operator_command_received(self, command_payload: Dict[str, Any]) -> None:
        pass
