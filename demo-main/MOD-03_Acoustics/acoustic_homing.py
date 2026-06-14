"""
File:    acoustic_homing.py
Brief:   MOD-03 Python Bridge — Acoustic Homing & FSM Integration
Author:  Evrim Doğa Solmaz 230104004042
Date:    2026-03-29
Version: 0.4
 
Changelog:
v0.4 (2026-04-20) - Added parse_uart_telemetry() to extract A_Hit/A_Ang from
                    raw MOD-01 UART pipe-delimited string.  Added convenience
                    method process_uart_line() on AcousticHomingBridge.
v0.3 (2026-03-29) - Aligned MotorDirection to M1 UART ints (0-4) and added explicit `buzzer_on` / `lights_on` booleans to NavCommand.
v0.2 (2026-03-29) - Aligned NavCommand with Modül 1 standard ('W, A, S, D, Q' and action flag).
v0.1 (2026-03-28) - Initial draft: interface stubs for acoustic bridge defined.

Consumed by: MOD-04 fsm_update loop on Raspberry Pi.
Depends on:  acoustics_iir.h (Uğur) for bearing data via UART,
             fsm_acoustic.h (Tuana) for FSM state transition signals.

Usage:
    from acoustic_homing import IAcousticHomingBridge, AcousticTelemetry, NavCommand, MotorDirection
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from typing import Optional
import re

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


# -- UART String Parser ------------------------------------------------------

# Precompiled regex patterns for the pipe-delimited telemetry format
# Expected format: ... |A_Hit:0| ... |A_Ang:-45.3| ...
_RE_A_HIT = re.compile(r'\|A_Hit:(\d)\|')
_RE_A_ANG = re.compile(r'\|A_Ang:([\-+]?\d+\.?\d*)\|')


def parse_uart_telemetry(raw_line: str) -> Optional['AcousticTelemetry']:
    """
    Parse the raw UART telemetry string from MOD-01/STM32 and extract
    acoustic fields (A_Hit, A_Ang) into an AcousticTelemetry dataclass.

    UART telemetry format (MOD-01 pipe-delimited, \n terminated):
        |Temp:25.3|Smoke:0|Stuck:0|A_Hit:1|A_Ang:45.0|Yaw:12.5|USF:30|USB:50|USL:20|USR:40|

    This function only extracts the acoustic-relevant fields:
        A_Hit  — 0 or 1  (mapped to bool)
        A_Ang  — float bearing in degrees, -180.0 to +180.0

    If either field is missing from the string, returns None so the
    caller can gracefully skip non-acoustic packets.

    @param  raw_line  Raw UART string received from STM32 via serial
    @return AcousticTelemetry if both fields found, None otherwise

    Usage:
        line = serial_port.readline().decode()
        telemetry = parse_uart_telemetry(line)
        if telemetry is not None:
            nav = bridge.process_telemetry(telemetry)
    """
    hit_match = _RE_A_HIT.search(raw_line)
    ang_match = _RE_A_ANG.search(raw_line)

    if hit_match is None or ang_match is None:
        return None

    try:
        a_hit = int(hit_match.group(1)) != 0
        a_ang = float(ang_match.group(1))
    except (ValueError, IndexError):
        return None

    return AcousticTelemetry(a_hit=a_hit, a_ang=a_ang)


class MotorDirection(int, Enum):
    """
    Direction Enum matching MOD-01 UART int format (uart_direction_t).
    0 = STOP, 1 = FORWARD, 2 = BACKWARD, 3 = LEFT, 4 = RIGHT
    """
    STOP = 0
    FORWARD = 1
    BACKWARD = 2
    LEFT = 3
    RIGHT = 4

@dataclass
class NavCommand:
    """
    Motor navigation command to be forwarded to MOD-01 via UART.

    Fields:
        direction (MotorDirection): Movement direction (0-4).
        speed (int):                PWM speed value (0-255).
        buzzer_on (bool):           Trigger wake-up buzzer.
        lights_on (bool):           Trigger SOS / Flashlight.
    """
    direction: MotorDirection
    speed: int
    buzzer_on: bool = False
    lights_on: bool = False

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


# -- Concrete Implementation -------------------------------------------------

# Additional constants for homing behaviour
# Aligned with fsm_acoustic.h FSM_ACOUSTIC_HOMING_TIMEOUT_MS = 30000
HOMING_TIMEOUT_SEC: float = 30.0
"""Maximum seconds in ACOUSTIC_HOMING before auto-reset."""

HOMING_DEFAULT_SPEED: int = 150
"""Default PWM speed (0-255) when driving toward the acoustic source."""

HOMING_APPROACH_SPEED: int = 100
"""Reduced PWM speed when bearing is within tolerance (careful approach)."""

import time
import logging

logger = logging.getLogger("MOD03.acoustic_homing")


class AcousticHomingBridge(IAcousticHomingBridge):
    """
    Concrete implementation of the Acoustic Homing Bridge.
    Author: Evrim Doğa Solmaz 230104004042

    Consumed by MOD-04's fsm_update loop on Raspberry Pi.

    Behaviour:
        1. Accumulates consecutive A_Hit=True readings.
        2. Once HOMING_MIN_HIT_CONFIRMS (3) consecutive hits are reached,
           computes a NavCommand based on the bearing angle and notifies
           MOD-04 FSM to transition EXPLORE -> ACOUSTIC_HOMING.
        3. Subsequent calls while in homing mode continue to refine the
           bearing and issue corrective motor commands.
        4. reset() is called by MOD-04 when homing times out or a false
           positive is identified.
    """

    def __init__(self, fsm_transition_callback=None):
        """
        @param fsm_transition_callback  Optional callable(bearing: float)
               provided by MOD-04 to trigger FSM state change.
               If None, notify_fsm_transition only logs the event.
        """
        self._hit_streak: int = 0
        self._last_bearing: float = 0.0
        self._is_homing: bool = False
        self._homing_start_time: float = 0.0
        self._fsm_callback = fsm_transition_callback

    def process_telemetry(self, telemetry: AcousticTelemetry) -> Optional[NavCommand]:
        """
        Process incoming acoustic telemetry from MOD-04.

        Decision flow (aligned with fsm_acoustic.h FSM_Acoustic_Update):
          1. If a_hit is False  -> reset streak, return None.
          2. If a_hit is True   -> increment streak.
             a. If streak < HOMING_MIN_HIT_CONFIRMS -> return None (wait).
             b. If streak >= threshold AND not yet homing ->
                notify FSM transition, enter homing mode.
             c. Compute NavCommand from bearing angle:
                - |bearing| <= tolerance  -> FORWARD  (source is ahead)
                - bearing > tolerance     -> RIGHT    (source is to the right)
                - bearing < -tolerance    -> LEFT     (source is to the left)
          3. If homing has timed out -> reset and return STOP.

        @param  telemetry  Parsed AcousticTelemetry from UART packet
        @return NavCommand to send to MOD-01, or None if no action needed
        """
        # --- No hit: reset streak and do nothing ---
        if not telemetry.a_hit:
            if self._hit_streak > 0:
                logger.debug("Acoustic hit streak broken at %d", self._hit_streak)
            self._hit_streak = 0
            return None

        # --- Validate bearing range (aligned with FSM_BEARING_MIN/MAX_DEG) ---
        if not (-180.0 <= telemetry.a_ang <= 180.0):
            logger.warning("Invalid bearing %.1f° — ignoring hit", telemetry.a_ang)
            return None

        # --- Accumulate hits ---
        self._hit_streak += 1
        self._last_bearing = telemetry.a_ang
        logger.debug("Acoustic hit streak: %d / %d  bearing: %.1f°",
                      self._hit_streak, HOMING_MIN_HIT_CONFIRMS, telemetry.a_ang)

        # --- Not enough confirmations yet ---
        if self._hit_streak < HOMING_MIN_HIT_CONFIRMS:
            return None

        # --- Check homing timeout ---
        if self._is_homing:
            elapsed = time.monotonic() - self._homing_start_time
            if elapsed >= HOMING_TIMEOUT_SEC:
                logger.info("Acoustic homing timed out after %.1fs — resetting", elapsed)
                self.reset()
                return NavCommand(
                    direction=MotorDirection.STOP,
                    speed=0,
                    buzzer_on=False,
                    lights_on=False,
                )

        # --- First time reaching threshold: trigger FSM transition ---
        if not self._is_homing:
            self._is_homing = True
            self._homing_start_time = time.monotonic()
            self.notify_fsm_transition(telemetry.a_ang)

        # --- Compute motor command from bearing ---
        return self._bearing_to_nav_command(telemetry.a_ang)

    def notify_fsm_transition(self, bearing: float) -> None:
        """
        Notify MOD-04 FSM to transition from EXPLORE to ACOUSTIC_HOMING.
        If a callback was provided at construction, it is invoked.
        Otherwise, only a log message is produced (MOD-04 polls the result).

        @param  bearing  Confirmed bearing angle to acoustic source (degrees)
        """
        logger.info("FSM transition requested: EXPLORE -> ACOUSTIC_HOMING  bearing=%.1f°", bearing)

        if self._fsm_callback is not None:
            try:
                self._fsm_callback(bearing)
            except Exception as exc:
                logger.error("FSM transition callback failed: %s", exc)

    def reset(self) -> None:
        """
        Reset internal hit counter and homing state.
        Called when FSM returns to EXPLORE after timeout or false positive.
        Aligned with fsm_acoustic.h FSM_Acoustic_ResetStreak().
        """
        logger.info("Acoustic homing bridge reset (was homing=%s, streak=%d)",
                     self._is_homing, self._hit_streak)
        self._hit_streak = 0
        self._last_bearing = 0.0
        self._is_homing = False
        self._homing_start_time = 0.0

    # -- Convenience: parse + process in one call -----------------------------

    def process_uart_line(self, raw_line: str) -> Optional[NavCommand]:
        """
        One-shot convenience: parse a raw UART string and process it.

        Combines parse_uart_telemetry() + process_telemetry() so that
        MOD-04's serial loop can call a single method per line.

        @param  raw_line  Raw UART string from STM32
        @return NavCommand if acoustic action needed, None otherwise
        """
        telemetry = parse_uart_telemetry(raw_line)
        if telemetry is None:
            return None
        return self.process_telemetry(telemetry)

    # -- Private Helpers ------------------------------------------------------

    def _bearing_to_nav_command(self, bearing_deg: float) -> NavCommand:
        """
        Convert a bearing angle to a NavCommand.

        Logic (aligned with fsm_acoustic.h FSM_BEARING_DEAD_ZONE_DEG = 10°):
          - |bearing| <= HOMING_BEARING_TOLERANCE_DEG -> FORWARD (source ahead)
          - bearing > 0 (positive = right)            -> RIGHT turn
          - bearing < 0 (negative = left)             -> LEFT turn

        Activates buzzer and lights when driving forward toward confirmed source.
        """
        if abs(bearing_deg) <= HOMING_BEARING_TOLERANCE_DEG:
            # Source is roughly ahead — drive forward carefully
            return NavCommand(
                direction=MotorDirection.FORWARD,
                speed=HOMING_APPROACH_SPEED,
                buzzer_on=True,
                lights_on=True,
            )
        elif bearing_deg > 0:
            # Source is to the right — turn right
            return NavCommand(
                direction=MotorDirection.RIGHT,
                speed=HOMING_DEFAULT_SPEED,
                buzzer_on=False,
                lights_on=True,
            )
        else:
            # Source is to the left — turn left
            return NavCommand(
                direction=MotorDirection.LEFT,
                speed=HOMING_DEFAULT_SPEED,
                buzzer_on=False,
                lights_on=True,
            )
