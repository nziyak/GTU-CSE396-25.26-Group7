"""
File:    acoustic_homing.py
Brief:   MOD-03 Python Bridge — Acoustic Homing & FSM Integration
Author:  Evrim Doga Solmaz [Student ID TBD]
Date:    2026-03-28
Version: 0.1

Changelog:
v0.1 (2026-03-28) - Initial draft: interface stubs for acoustic bridge defined.

Consumed by: MOD-04 fsm_update loop on Raspberry Pi.
Depends on:  acoustics_iir.h (Ugur) for bearing data via UART,
             fsm_acoustic.h (Tuana) for FSM state transition signals.

Usage:
    from acoustic_homing import IAcousticHomingBridge, AcousticTelemetry, NavCommand
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Optional

# -- Constants ---------------------------------------------------------------

HOMING_BEARING_TOLERANCE_DEG: float = 10.0
"""Degrees of tolerance before issuing a corrective turn command."""

HOMING_MIN_HIT_CONFIRMS: int = 3
"""Minimum consecutive A_Hit=1 readings before triggering FSM transition."""

# -- Data Types --------------------------------------------------------------

@dataclass
class AcousticTelemetry:
    """
    Parsed acoustic fields from the MOD-01/03 UART telemetry packet.
    Populated by MOD-04's serial_read_telemetry().

    Fields:
        a_hit (bool):  True if STM32 detected a distress call (A_Hit field).
        a_ang (float): Bearing to sound source in degrees (-180.0 to +180.0).
    """
    a_hit: bool
    a_ang: float


@dataclass
class NavCommand:
    """
    Motor navigation command to be forwarded to MOD-01 via UART.

    Fields:
        direction (str): One of 'FORWARD', 'LEFT', 'RIGHT', 'STOP'.
        speed (int):     PWM speed value (0-255).
    """
    direction: str
    speed: int

# -- Public Interface --------------------------------------------------------

class IAcousticHomingBridge(ABC):
    """
    Abstract interface for the Acoustic Homing Bridge.
    Evrim implements this class in acoustic_homing.py.
    """

    @abstractmethod
    def process_telemetry(self, telemetry: AcousticTelemetry) -> Optional[NavCommand]:
        """
        Process incoming acoustic telemetry from MOD-04.
        Accumulates A_Hit confirmations. Once HOMING_MIN_HIT_CONFIRMS is reached,
        returns a NavCommand and notifies FSM to transition to ACOUSTIC_HOMING.

        @param  telemetry  Parsed AcousticTelemetry from UART packet
        @return NavCommand to send to MOD-01, or None if no action needed
        """
        pass

    @abstractmethod
    def notify_fsm_transition(self, bearing: float) -> None:
        """
        Notify MOD-04 FSM to transition from EXPLORE to ACOUSTIC_HOMING.

        @param  bearing  Confirmed bearing angle to acoustic source (degrees)
        """
        pass

    @abstractmethod
    def reset(self) -> None:
        """
        Reset internal hit counter and state.
        Called when FSM returns to EXPLORE after false positive or timeout.
        """
        pass
