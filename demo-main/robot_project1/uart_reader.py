from __future__ import annotations

import copy
import json
import logging
import threading
import time
from dataclasses import dataclass
from typing import Optional

import serial


logger = logging.getLogger("uart_reader")


@dataclass
class UartTelemetry:
    dist_front: int = 0
    dist_back: int = 0
    mic1: int = 0
    mic2: int = 0
    smoke: int = 0
    yaw: float = 0.0
    pitch: float = 0.0
    roll: float = 0.0
    temp: float = 0.0
    hum: float = 0.0
    raw_line: str = ""
    connected: bool = False


class SerialTelemetryReader:
    def __init__(self, port: str = "/dev/ttyUSB0", baudrate: int = 115200, timeout: float = 1.0) -> None:
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self._serial: Optional[serial.Serial] = None
        self._latest = UartTelemetry()
        self._lock = threading.Lock()
        self._serial_lock = threading.Lock()
        self._stop_event = threading.Event()
        self._thread: Optional[threading.Thread] = None

    def start(self) -> None:
        self._serial = serial.Serial(self.port, self.baudrate, timeout=self.timeout)
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._read_loop, name="UartReader", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop_event.set()
        if self._thread is not None and self._thread.is_alive():
            self._thread.join(timeout=2.0)

        if self._serial is not None:
            try:
                self._serial.close()
            except Exception:
                pass

    def get_latest(self) -> UartTelemetry:
        with self._lock:
            return copy.deepcopy(self._latest)

    def send_command(self, command: str) -> bool:
        if self._serial is None:
            logger.warning("UART send failed: serial is not open")
            return False

        normalized = command.strip().upper()
        if not normalized:
            logger.warning("UART send failed: empty command")
            return False

        try:
            packet = f"{normalized}\n".encode("utf-8")
            with self._serial_lock:
                self._serial.write(packet)
                self._serial.flush()
            logger.info("UART command sent to Mega: %s", normalized)
            return True
        except Exception as exc:
            logger.exception("UART send failed: %s", exc)
            return False

    def _read_loop(self) -> None:
        while not self._stop_event.is_set():
            try:
                with self._serial_lock:
                    line = self._serial.readline().decode("utf-8", errors="ignore").strip()

                if not line:
                    continue

                try:
                    data = json.loads(line)
                except json.JSONDecodeError:
                    logger.warning("Bad UART JSON: %s", line)
                    continue

                telemetry = UartTelemetry(
                    dist_front=int(data.get("dist_front", 0)),
                    dist_back=int(data.get("dist_back", 0)),
                    mic1=int(data.get("mic1", 0)),
                    mic2=int(data.get("mic2", 0)),
                    smoke=int(data.get("smoke", 0)),
                    yaw=float(data.get("yaw", 0.0)),
                    pitch=float(data.get("pitch", 0.0)),
                    roll=float(data.get("roll", 0.0)),
                    temp=float(data.get("temp", 0.0)),
                    hum=float(data.get("hum", 0.0)),
                    raw_line=line,
                    connected=True,
                )

                with self._lock:
                    self._latest = telemetry

            except Exception as exc:
                logger.exception("UART read failed: %s", exc)
                time.sleep(0.5)