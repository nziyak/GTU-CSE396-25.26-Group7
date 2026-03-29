"""
@file      comms_dashboard_interface.py
@brief     Public Interface Contract for Flask/WebSockets Dashboard and Comms Bridging
@author    Ömer, Uğur, Fatma
@date      2026-03-29
@version   0.1

Changelog:
v0.1 - Initial draft: Defined WebSocket broadcasting, video streaming, and manual override handling.
"""

from dataclasses import dataclass
from abc import ABC, abstractmethod
from typing import Dict, Any

@dataclass
class AugmentedStatusReport:
    """
    @brief JSON-serializable structure for the Augmented Status Report sent to Unity.
    """
    pos_x: float
    pos_y: float
    temperature: float
    smoke_detected: bool
    victim_status: str       # "NONE", "STANDING", "LYING", "TRAPPED"
    is_stuck: bool


class IWebDashboard(ABC):
    """
    @brief Main interface for the Flask Web Server and WebSocket communications.
    """

    @abstractmethod
    def start_server(self, host: str = "0.0.0.0", port: int = 5000) -> None:
        """
        @brief Initializes the Flask app and SocketIO server. Blocks the thread.
        @param host Bind IP address.
        @param port Bind port number.
        """
        pass

    @abstractmethod
    def broadcast_telemetry(self, report: AugmentedStatusReport) -> None:
        """
        @brief Emits the JSON status report to all connected WebSocket clients (Unity).
        @param report AugmentedStatusReport dataclass instance.
        """
        pass

    @abstractmethod
    def stream_video_frame(self, jpeg_bytes: bytes) -> None:
        """
        @brief Streams a single compressed JPEG frame to the Web Dashboard.
        @param jpeg_bytes Encoded image bytes from Mod 2 (Vision).
        """
        pass

    @abstractmethod
    def on_operator_command_received(self, command_payload: Dict[str, Any]) -> None:
        """
        @brief Callback triggered when an operator sends a manual override command.
               Must interrupt the autonomous FSM.
        @param command_payload Dictionary containing direction and action triggers.
        """
        pass