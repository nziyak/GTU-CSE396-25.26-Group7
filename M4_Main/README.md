# MOD-04 — Central FSM, UART Bridge & WebSocket Server

**Purpose:** This module is the "brain" of the rescue robot running on the Raspberry Pi 5. It connects all other modules (M1 Embedded, M2 Vision, M3 Acoustics, M5 Unity) using their current interface definitions.

**Authors:**
- Group 7, CSE396

**Dependencies:**
- Python 3.11+ on Raspberry Pi 5
- `pyserial` (UART communication with STM32)
- `flask`, `flask-socketio` (WebSocket server for Unity)
- `vosk` or `whisper.cpp` (Edge STT inference)
- M3 `acoustic_homing.py` (Acoustic Homing Bridge, from `M3_Acoustics_Navigation/`)

---

## Connection Map

| Link | Protocol | Source Interface |
|------|----------|-----------------|
| M1 (STM32) <-> M4 | UART 115200 baud | `uart_comm.h` |
| M3 (Acoustics) <-> M4 | UART + Python bridge | `fsm_acoustic.h`, `acoustic_homing.py` |
| M2 (Vision) <-> M4 | Local Python calls / threading | `ai_vision.h` |
| M5 (Unity) <-> M4 | WebSocket (Flask-SocketIO) | `DataContracts.cs`, `INetworkClient.cs` |

---

## File Contents

| Section | Scope | Functions |
|---------|-------|-----------|
| §1 UART | M4 <-> M1 | `serial_init()`, `serial_read_telemetry()`, `serial_send_command()` |
| §2 WebSocket | M4 <-> M5 | `ws_emit_telemetry_update()`, `ws_on_operator_command()`, `ws_on_audio_received()` |
| §3 Vision | M4 <-> M2 | `vision_init()`, `vision_set_power_mode()`, `vision_process_frame()`, `pause_vision_pipeline()`, `resume_vision_pipeline()` |
| §4 STT Interrupt | M4 -> M2 | `trigger_stt_interrupt()`, `run_stt_inference()` |
| §5 FSM | Core | `fsm_update()`, `fsm_transition()`, `fsm_acoustic_update()` |
| §6 Status Report | M4 -> M5 | `build_augmented_status_report()` |
| §7 Failsafe | Core | `check_rth_timeout()`, `dead_mans_switch_check()` |
| §8 Acoustic Bridge | M4 <-> M3 | `process_acoustic_event()` |

---

## Data Type Alignment

| M4 Python Type | Matches | Source File |
|----------------|---------|-------------|
| `UartDirection` (Enum, int 0-4) | `uart_direction_t` | `uart_comm.h` |
| `UartTelemetry` (dataclass) | `uart_telemetry_t` | `uart_comm.h` |
| `UartCommand` (dataclass) | `uart_command_t` | `uart_comm.h` |
| `FSMState` (Enum, 7 states) | `fsm_state_t` | `fsm_acoustic.h` |
| `FSMAcousticEvent` (dataclass) | `fsm_acoustic_event_t` | `fsm_acoustic.h` |
| `FSMAcousticResult` (dataclass) | `fsm_acoustic_result_t` | `fsm_acoustic.h` |
| `VictimStatus` (Enum, int 0-3) | `vision_status_t` / `VictimStatus` | `ai_vision.h` / `DataContracts.cs` |
| `VisionPriority` (Enum) | `vision_priority_t` | `ai_vision.h` |
| `VisionResult` (dataclass) | `vision_result_t` | `ai_vision.h` |
| `TelemetryData` (dataclass) | `TelemetryData` struct | `DataContracts.cs` |

---

## API Summary

- **`serial_init()`** — Opens UART port (maps to `uart_comm_init()`)
- **`serial_read_telemetry()`** — Reads `uart_telemetry_t` (9 fields including imu_yaw, ultrasonic sensors)
- **`serial_send_command(cmd)`** — Sends `uart_command_t` with direction(int), speed, buzzer_on, lights_on
- **`ws_emit_telemetry_update(data)`** — Emits `TelemetryData` JSON for Unity (`posX`, `posY`, `temperature`, `smokeDetected`, `victimStatus`, `priorityLevel`)
- **`ws_on_operator_command(command)`** — Receives string command from `INetworkClient.SendOperatorCommand(string)`
- **`ws_on_audio_received(audio_blob)`** — Receives PTT audio from `INetworkClient.SendAudioBlob(byte[])`
- **`vision_init()`** — Initializes YOLO/VLM (maps to `ai_vision_init()`)
- **`vision_process_frame()`** — Single-frame analysis (maps to `ai_vision_process_frame()`)
- **`pause_vision_pipeline()` / `resume_vision_pipeline()`** — STT resource management (maps to `ai_vision_preemptive_pause()` / `ai_vision_resume()`)
- **`fsm_update()`** — Main FSM loop tick
- **`fsm_acoustic_update(state, event)`** — Acoustic FSM branching (maps to `FSM_Acoustic_Update()`)
- **`build_augmented_status_report(...)`** — Generates `TelemetryData` JSON for Unity
- **`process_acoustic_event(telemetry)`** — Acoustic bridge (uses `IAcousticHomingBridge`)
