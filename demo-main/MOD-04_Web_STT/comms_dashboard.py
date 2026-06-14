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

    def __init__(self, stt_engine=None):
        self.app = Flask(__name__)
        self.socketio = SocketIO(self.app, cors_allowed_origins="*")
        self.connected_clients = set()
        self._last_broadcast_log_ts = 0.0
        self.stt_engine = stt_engine
        self.vision_pipeline = None
        self._setup_routes()

    def set_stt_engine(self, stt_engine) -> None:
        """Registers the STT engine."""
        self.stt_engine = stt_engine

    def set_vision_pipeline(self, vision_pipeline) -> None:
        """Registers the MOD-02 Vision pipeline (for pause/resume during STT)."""
        self.vision_pipeline = vision_pipeline

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

        @self.socketio.on("audio_received")
        def handle_audio_received(data):
            print(f"[WebDashboard] 'audio_received' event received. Size: {data.get('byteCount')} bytes")
            try:
                base64_data = data.get("data", "")
                wav_bytes = base64.b64decode(base64_data)
            except Exception as e:
                print(f"[WebDashboard] Base64 error decoding audio: {e}")
                return

            if self.stt_engine:
                # Pause the vision pipeline to prevent OOM
                if self.vision_pipeline:
                    print("[WebDashboard] Pausing vision pipeline before STT processing...")
                    try:
                        self.vision_pipeline.pause_vision_pipeline()
                    except Exception as e:
                        print(f"[WebDashboard] Failed to pause vision pipeline: {e}")

                try:
                    # Process the audio file and extract the intent
                    command = self.stt_engine.process_audio_blob(wav_bytes)
                    print(f"[WebDashboard] STT Completed. Text: '{command.raw_text}' -> Intent: {command.intent}")
                    
                    # Forward the extracted intent as a manual operator override command to the FSM
                    self.on_operator_command_received({
                        "override": True, 
                        "cmd": command.intent,
                        "raw_text": command.raw_text
                    })
                    
                    # Send feedback to the client (Unity/Web)
                    self.socketio.emit("speech_command_parsed", {
                        "rawText": command.raw_text,
                        "intent": command.intent,
                        "confidence": command.confidence
                    })
                except Exception as e:
                    print(f"[WebDashboard] STT error during audio processing: {e}")
                finally:
                    # Resume the vision pipeline
                    if self.vision_pipeline:
                        print("[WebDashboard] Resuming vision pipeline...")
                        try:
                            self.vision_pipeline.resume_vision_pipeline()
                        except Exception as e:
                            print(f"[WebDashboard] Failed to resume vision pipeline: {e}")
            else:
                print("[WebDashboard] Warning: Audio command could not be processed because STT engine is not loaded.")

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
