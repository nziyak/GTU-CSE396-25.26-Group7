from dataclasses import dataclass


@dataclass
class MockTelemetryState:
    """Temporary Pi-side telemetry until STM32 is integrated."""

    pos_x: float = 0.0
    pos_y: float = 0.0
    temperature: float = 25.0
    smoke_detected: bool = False
    is_stuck: bool = False
    acoustic_hit: bool = False
    acoustic_angle: float = 0.0
