# MOD-04 Web Dashboard & STT Module

**Purpose:** This module acts as the central communication bridge (Operator Control Interface). It runs a Flask/WebSockets backend on the Raspberry Pi 5 to stream live video/telemetry to Unity, routes manual override commands to the FSM, and executes the offline Edge STT (Speech-to-Text) model to parse incoming voice commands.

**Authors:**
* Ömer [Öğrenci No] (Primary - Comms Bridging & JSON Telemetry)
* Tuana [Öğrenci No] (Primary - Offline STT Model Integration)
* Uğur [Öğrenci No] (Secondary - Video Streaming Pipeline)
* Fatma Öztürk [230104004152] (Secondary - Operator Control Routing)

**Dependencies:**
* Python 3.9+
* `Flask`, `Flask-SocketIO` (for WebSockets)
* `pyserial` (for bridging with Mod 1)
* `vosk` or `whisper.cpp` (for Offline STT)

### Quick-Start Integration Example

```python
from comms_dashboard_interface import AugmentedStatusReport
from stt_engine_interface import VoiceCommandData

# 1. Initialize STT and Server
# stt_engine.load_offline_model("/models/vosk-model-small")
# dashboard.start_server(port=5000)

# 2. Broadcast Telemetry Example
report = AugmentedStatusReport(
    pos_x=12.5, pos_y=8.0, temperature=25.1, 
    smoke_detected=False, victim_status="TRAPPED", is_stuck=False
)
# dashboard.broadcast_telemetry(report)

# 3. Processing Incoming Audio from Unity
def handle_audio_event(wav_bytes: bytes):
    # DANGER: Pause vision pipeline before running STT to prevent OOM
    # mod2_vision.pause_vision_pipeline()
    
    command: VoiceCommandData = stt_engine.process_audio_blob(wav_bytes)
    print(f"Recognized Intent: {command.intent}")
    
    # mod2_vision.resume_vision_pipeline()
API Summary
void start_server(str host, int port)

Description: Initializes the Flask app and SocketIO server.

void broadcast_telemetry(AugmentedStatusReport report)

Description: Emits the JSON status report to Unity via WebSockets.

void stream_video_frame(bytes jpeg_bytes)

Description: Streams a single compressed frame to the Dashboard.

void on_operator_command_received(dict command_payload)

Description: Callback for incoming manual override commands.

bool load_offline_model(str model_path)

Description: Loads the Vosk/Whisper model into Pi RAM.

VoiceCommandData process_audio_blob(bytes wav_bytes)

Description: Converts raw .wav bytes into parsed text commands.

Known Limitations and TODOS
TODO: Define exact WebSocket event names (e.g., telemetry_update, audio_stream).

Limitation: STT processing is highly CPU-intensive. The Pi 5 may throttle if the Vision module (YOLO) is not explicitly paused during process_audio_blob() execution.

Limitation: Video streaming via WebSockets will be capped at 5-10 FPS to reserve bandwidth for critical telemetry and audio data.

Version History
v0.1 (2026-03-29): Initial draft, defined Dashboard interface, JSON structuring, and STT engine contracts.
