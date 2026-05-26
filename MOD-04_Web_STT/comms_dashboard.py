import base64
import time
from typing import Any, Dict

# pyrefly: ignore [missing-import]
from flask import Flask, request
from flask_socketio import SocketIO

from comms_dashboard_interface import AugmentedStatusReport, IWebDashboard
from telemetry_contract import build_telemetry_payload


class WebDashboard(IWebDashboard):
    """
    Flask + Socket.IO dashboard for Unity and operator-side communications.
    """

    def __init__(self):
        self.app = Flask(__name__)
        self.socketio = SocketIO(self.app, cors_allowed_origins="*")
        self.connected_clients = set()
        self._last_broadcast_log_ts = 0.0
        self._setup_routes()

    def _setup_routes(self):
        @self.app.route("/")
        def index():
            return "MOD-04 Communications Dashboard is running. Waiting for Unity/Operator connection..."

        @self.socketio.on("connect")
        def handle_connect():
            self.connected_clients.add(request.sid)
            print(
                f"[WebDashboard] Client connected: {request.sid} "
                f"(total={len(self.connected_clients)})"
            )

        @self.socketio.on("disconnect")
        def handle_disconnect():
            self.connected_clients.discard(request.sid)
            print(
                f"[WebDashboard] Client disconnected: {request.sid} "
                f"(total={len(self.connected_clients)})"
            )

        @self.socketio.on("operator_command")
        def handle_operator_command(data):
            print(f"[WebDashboard] 'operator_command' event received: {data}")
            self.on_operator_command_received(data)

        @self.socketio.on("unity_ping")
        def handle_unity_ping(data):
            self.socketio.emit("unity_pong", data, to=request.sid)

    def start_server(self, host: str = "0.0.0.0", port: int = 5000) -> None:
        print(f"[WebDashboard] Starting server on {host}:{port} ...")
        self.socketio.run(self.app, host=host, port=port, allow_unsafe_werkzeug=True)

    def broadcast_telemetry(self, report: AugmentedStatusReport) -> None:
        payload = build_telemetry_payload(report)
        self.socketio.emit("telemetry_update", payload)
        now = time.time()
        if now - self._last_broadcast_log_ts >= 5.0:
            self._last_broadcast_log_ts = now
            print(
                "[WebDashboard] Telemetry broadcast "
                f"victimStatus={payload['victimStatus']} "
                f"priorityLevel={payload['priorityLevel']} "
                f"clients={len(self.connected_clients)}"
            )

    def stream_video_frame(self, jpeg_bytes: bytes) -> None:
        b64_img = base64.b64encode(jpeg_bytes).decode("utf-8")
        data_url = f"data:image/jpeg;base64,{b64_img}"
        self.socketio.emit("video_frame", {"image": data_url})

    def on_operator_command_received(self, command_payload: Dict[str, Any]) -> None:
        print(f"[Dashboard Callback] Operator override command received: {command_payload}")

    def get_connected_client_count(self) -> int:
        return len(self.connected_clients)
