# MOD-04 — Web Dashboard & STT Module

**Purpose:** This module acts as the central communication bridge (Operator Control Interface). It runs a Flask/WebSockets backend on the Raspberry Pi 5 to stream live video and telemetry to Unity, routes manual override commands to the FSM, and executes the offline Edge STT (Speech-to-Text) model to parse incoming voice commands.

## Authors

| Name | Student ID | Role |
|------|-----------|------|
| Ömer | [Öğrenci No] | Primary — Comms Bridging & JSON Telemetry |
| Tuana | 23104004903 | Primary — Offline STT Model Integration |
| Uğur | 210104004011 | Secondary — Video Streaming Pipeline |
| Fatma Öztürk | 230104004152 | Secondary — Operator Control Routing |

## Dependencies

- Python 3.9+
- `Flask`, `Flask-SocketIO` — WebSocket server and HTTP routing
- `pyserial` — Serial bridge with Mod 1 (Arduino / Sensor MCU)
- `vosk` or `whisper.cpp` — Offline Speech-to-Text engine

## Source Files

| File | Description |
|------|-------------|
| `comms_dashboard_interface.py` | Public interface contract for Flask/WebSocket dashboard and comms bridging |
| `stt_engine_interface.py` | Public interface contract for offline Speech-to-Text engine (Vosk/Whisper) |

## Data Structures

### `AugmentedStatusReport`

JSON-serializable telemetry payload sent to Unity via WebSockets.

| Field | Type | Description |
|-------|------|-------------|
| `pos_x` | `float` | Robot X coordinate on the map |
| `pos_y` | `float` | Robot Y coordinate on the map |
| `temperature` | `float` | Ambient temperature reading (°C) |
| `smoke_detected` | `bool` | Whether the smoke sensor is triggered |
| `victim_status` | `str` | One of `"NONE"`, `"STANDING"`, `"LYING"`, `"TRAPPED"` |
| `is_stuck` | `bool` | Whether the robot has detected a stuck condition |
| `priority_level` | `int` | Rescue priority ranking for the current target |
| `acoustic_hit` | `bool` | Whether the acoustic sensor detected a sound event |
| `acoustic_angle` | `float` | Estimated angle (°) of the detected acoustic source |

### `VoiceCommandData`

Parsed output from the STT pipeline.

| Field | Type | Description |
|-------|------|-------------|
| `raw_text` | `str` | Full transcription output from the STT model |
| `intent` | `str` | Recognized command, e.g. `"STOP"`, `"RETURN_HOME"`, `"ACTIVATE_BEACON"` |
| `confidence` | `float` | Model confidence score (0.0–1.0) |

## API Summary

### Dashboard Interface (`IWebDashboard`)

| Method | Signature | Description |
|--------|-----------|-------------|
| `start_server` | `(host: str = "0.0.0.0", port: int = 5000) -> None` | Initializes the Flask app and SocketIO server. Blocks the calling thread. |
| `broadcast_telemetry` | `(report: AugmentedStatusReport) -> None` | Emits the JSON status report to all connected WebSocket clients (Unity). |
| `stream_video_frame` | `(jpeg_bytes: bytes) -> None` | Streams a single compressed JPEG frame to the web dashboard. |
| `on_operator_command_received` | `(command_payload: Dict[str, Any]) -> None` | Callback triggered when an operator sends a manual override command. Must interrupt the autonomous FSM. |

### STT Interface (`ISTTEngine`)

| Method | Signature | Description |
|--------|-----------|-------------|
| `load_offline_model` | `(model_path: str) -> bool` | Loads the quantized Vosk/Whisper model into Raspberry Pi RAM. Returns `True` on success. |
| `process_audio_blob` | `(wav_bytes: bytes) -> VoiceCommandData` | Converts a raw PCM `.wav` byte array into a parsed voice command. **Warning:** This is a heavy blocking call — Mod 2 Vision must be paused before calling. |

## Quick-Start Integration Example

```python
from comms_dashboard_interface import AugmentedStatusReport
from stt_engine_interface import VoiceCommandData

# 1. Initialize STT and Server
# stt_engine.load_offline_model("/models/vosk-model-small")
# dashboard.start_server(port=5000)

# 2. Broadcast Telemetry
report = AugmentedStatusReport(
    pos_x=12.5, pos_y=8.0, temperature=25.1,
    smoke_detected=False, victim_status="TRAPPED", is_stuck=False,
    priority_level=1, acoustic_hit=True, acoustic_angle=45.0
)
# dashboard.broadcast_telemetry(report)

# 3. Process Incoming Audio from Unity
def handle_audio_event(wav_bytes: bytes):
    # DANGER: Pause vision pipeline before running STT to prevent OOM
    # mod2_vision.pause_vision_pipeline()

    command: VoiceCommandData = stt_engine.process_audio_blob(wav_bytes)
    print(f"Recognized Intent: {command.intent}")

    # mod2_vision.resume_vision_pipeline()
```

## Version History

| Version | Date | Notes |
|---------|------|-------|
| v0.2 | 2026-03-29 | Added `priority_level`, `acoustic_hit`, and `acoustic_angle` fields to `AugmentedStatusReport` for rescue prioritization and acoustic sensor data. |
| v0.1 | 2026-03-29 | Initial draft — defined `IWebDashboard` interface (`start_server`, `broadcast_telemetry`, `stream_video_frame`, `on_operator_command_received`), `ISTTEngine` interface (`load_offline_model`, `process_audio_blob`), and supporting data classes (`AugmentedStatusReport`, `VoiceCommandData`). |
