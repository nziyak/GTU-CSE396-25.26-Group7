from __future__ import annotations

import logging
import time
from typing import Any, Dict

from ai_vision import VisionPipeline
from comms_dashboard import WebDashboard
from uart_reader import SerialTelemetryReader


logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger("pi_main")

SMOKE_THRESHOLD = 1500
ALLOWED_COMMANDS = {"FORWARD", "BACKWARD", "LEFT", "RIGHT", "STOP"}


class PiIntegratedSystem:
    def __init__(self) -> None:
        self.vision = VisionPipeline()
        self.dashboard = WebDashboard()
        self.uart = SerialTelemetryReader(port="/dev/ttyUSB0", baudrate=115200, timeout=1.0)

        self.dashboard.set_operator_command_callback(self.handle_operator_command)
        self.dashboard.set_vision_pipeline(self.vision)

    def start(self) -> None:
        if not self.vision.initialize_camera():
            raise RuntimeError("Vision init failed")
        self.uart.start()

    def stop(self) -> None:
        self.uart.stop()
        self.vision.shutdown()

    def run(self) -> None:
        self.dashboard.socketio.start_background_task(self._publish_loop)
        self.dashboard.start_server(port=5001)

    def _publish_loop(self) -> None:
        while True:
            stm = self.uart.get_latest()
            smoke_detected = stm.smoke >= SMOKE_THRESHOLD

            report = self.vision.build_augmented_status_report(
                pos_x=0.0,
                pos_y=0.0,
                temperature=stm.temp,
                smoke_detected=smoke_detected,
                is_stuck=False,
                acoustic_hit=False,
                acoustic_angle=0.0,
            )

            self.dashboard.broadcast_telemetry(report)

            latest_frame = self.vision.get_latest_frame_jpeg()
            if latest_frame:
                self.dashboard.stream_video_frame(latest_frame)

            self.dashboard.socketio.emit(
                "uart_debug",
                {
                    "distFront": stm.dist_front,
                    "distBack": stm.dist_back,
                    "mic1": stm.mic1,
                    "mic2": stm.mic2,
                    "smokeRaw": stm.smoke,
                    "yaw": stm.yaw,
                    "pitch": stm.pitch,
                    "roll": stm.roll,
                    "temp": stm.temp,
                    "hum": stm.hum,
                    "smokeDetected": smoke_detected,
                    "connected": stm.connected,
                },
            )

            logger.info(
                "STM front=%s back=%s mic1=%s mic2=%s smoke=%s yaw=%.1f pitch=%.1f roll=%.1f temp=%.1f hum=%.1f connected=%s",
                stm.dist_front,
                stm.dist_back,
                stm.mic1,
                stm.mic2,
                stm.smoke,
                stm.yaw,
                stm.pitch,
                stm.roll,
                stm.temp,
                stm.hum,
                stm.connected,
            )

            target = self.vision.get_latest_target()
            if target is not None:
                logger.info(
                    "VISION victim=%s conf=%.2f dist=%.1fcm",
                    target.severity,
                    target.confidence,
                    target.distance_cm,
                )

            time.sleep(0.25)

    def handle_operator_command(self, command_payload: Dict[str, Any]) -> None:
        logger.info("Operator command received from Unity: %s", command_payload)

        raw_cmd = str(command_payload.get("cmd", "")).strip().upper()
        if raw_cmd not in ALLOWED_COMMANDS:
            logger.warning("Rejected unknown command: %s", raw_cmd)
            self.dashboard.socketio.emit(
                "operator_command_ack",
                {
                    "requested": raw_cmd,
                    "sent": False,
                    "reason": "unknown_command",
                },
            )
            return

        ok = self.uart.send_command(raw_cmd)

        self.dashboard.socketio.emit(
            "operator_command_ack",
            {
                "requested": raw_cmd,
                "sent": ok,
            },
        )

    def main_loop(self) -> None:
        self.start()
        try:
            self.run()
        finally:
            self.stop()


def main() -> None:
    system = PiIntegratedSystem()
    try:
        system.main_loop()
    except KeyboardInterrupt:
        logger.info("Shutting down...")


if __name__ == "__main__":
    main()