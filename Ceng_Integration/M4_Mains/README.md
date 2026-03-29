# MOD-04 — Central FSM, UART Bridge & WebSocket Server

**Purpose:** This module is the "brain" of the rescue robot running on the Raspberry Pi 5. It connects all other modules (M1 Embedded, M2 Vision, M3 Acoustics, M5 Unity) according to the interface rules defined in `modul_iletisim_arayuzleri.docx`.

**Authors:**
- Ceng_Integration (Auto-generated backbone)

**Dependencies:**
- Python 3.11+ on Raspberry Pi 5
- `pyserial` (UART communication with STM32)
- `flask`, `flask-socketio` (WebSocket server for Unity)
- `vosk` or `whisper.cpp` (Edge STT inference)
- M3 `acoustic_homing.py` (Acoustic Homing Bridge)

---

## Connection Map

| Link | Protocol | Docx Reference |
|------|----------|----------------|
| M1 (STM32) ↔ M4 | UART 115200 baud | docx §1 |
| M3 (Acoustics) → M4 | UART appendix packet | docx §4 |
| M2 (Vision) ↔ M4 | Local Python calls / threading | docx §3 |
| M5 (Unity) ↔ M4 | WebSocket (Flask-SocketIO) | docx §2 |

---

## File Contents

| Section | Scope | Functions |
|---------|-------|-----------|
| §1 UART | M4 ↔ M1/M3 | `serial_read_telemetry()`, `serial_read_acoustic()`, `serial_send_command()` |
| §2 WebSocket | M4 ↔ M5 | `ws_emit_telemetry_update()`, `ws_on_operator_command()`, `ws_on_audio_received()` |
| §3 Vision | M4 ↔ M2 | `get_vision_results()`, `pause_vision_pipeline()`, `resume_vision_pipeline()` |
| §4 STT Interrupt | M4 → M2 | `trigger_stt_interrupt()`, `run_stt_inference()` |
| §5 FSM | Core | `fsm_update()`, `fsm_transition()` |
| §6 Status Report | M4 → M5 | `build_augmented_status_report()` |
| §7 Failsafe | Core | `check_rth_timeout()`, `dead_mans_switch_check()`, `enter_beacon_mode()` |
| §8 Acoustic Bridge | M4 ↔ M3 | `process_acoustic_event()` |

---

## API Summary

- **`serial_read_telemetry()`** → Parses `T:<temp>|S:<smoke>|ST:<stuck>\n` into dict (docx §1A)
- **`serial_read_acoustic()`** → Parses `|A_Hit:<0/1>|A_Ang:<angle>\n` into dict (docx §4)
- **`serial_send_command(direction, speed, action_flag)`** → Sends `M:<dir>:<speed>|A:<flag>\n` (docx §1B)
- **`ws_emit_telemetry_update(payload)`** → Emits `telemetry_update` WebSocket event (docx §2A)
- **`ws_on_operator_command(data)`** → Receives `operator_command` WebSocket event (docx §2B)
- **`ws_on_audio_received(audio_blob)`** → Receives PTT audio blob (docx §2B)
- **`get_vision_results()`** → Retrieves `TargetData` list from M2 (docx §3A)
- **`pause_vision_pipeline()` / `resume_vision_pipeline()`** → STT resource management (docx §3B)
- **`fsm_update(vision_data, telemetry, acoustic)`** → Main FSM loop (docx §3A)
- **`build_augmented_status_report(...)`** → Generates JSON for Unity (docx §2A)

---

## Interface Integration Updates

This file was newly created as part of the `Ceng_Integration` workspace. It serves as the central integration backbone:

- **FSMState enum** maps 1:1 with `fsm_state_t` in `fsm_acoustic.h` (11 states, values 0–10).
- **VictimSeverity enum** maps to `vision_status_t` in `ai_vision.h` and `VictimStatus` in `DataContracts.cs`.
- **TargetData dataclass** mirrors `vision_result_t` / `vision_bbox_t` from `ai_vision.h` (bbox as tuple).
- **UART functions** produce/consume data matching `uart_telemetry_t`, `uart_acoustic_t`, and `uart_command_t` structs.
- **WebSocket JSON schema** matches `TelemetryData` and `CommandPacket` in `DataContracts.cs` field-for-field.
- **STT interrupt mechanism** calls `pause_vision_pipeline()` / `resume_vision_pipeline()` per docx §3B.
