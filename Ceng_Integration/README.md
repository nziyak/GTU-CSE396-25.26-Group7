# Ceng_Integration — Centralized Integration Report

**Project:** GTU CSE396 Group 7 — Search & Rescue Robot
**Date:** 2026-03-29
**Reference:** `modul_iletisim_arayuzleri (1).docx`

This report summarizes every change applied to the original module interfaces when creating the unified `Ceng_Integration` workspace. Each entry lists the file, what changed, and the specific docx rule that motivated the change.

---

## M1_Embedded

- **uart_comm.h**
  - **Changed:** `uart_direction_t` enum values from integers (0–4) to ASCII chars ('W','A','S','D','Q')
  - **Why:** Docx §1B specifies UART command format `M:<W|A|S|D|Q>:<speed>` using character direction codes

  - **Changed:** Merged `buzzer_on` and `lights_on` fields in `uart_command_t` into a single `uint8_t action_flag`
  - **Why:** Docx §1B defines `A:<flag>` as a single action field, not separate buzzer/lights toggles

  - **Changed:** Removed `float acoustic_angle` from `uart_telemetry_t`; created new `uart_acoustic_t` struct with `bool audio_hit` and `float audio_angle`
  - **Why:** Docx §4 defines acoustic data as a separate appendix (`|A_Hit:<0/1>|A_Ang:<angle>`) independent of base telemetry

  - **Added:** `void uart_send_acoustic(const uart_acoustic_t* data)` function
  - **Why:** Docx §4 requires a dedicated transmission path for acoustic bearing packets

  - **Added:** `void UART_ReceiveCommand(uint8_t* buffer)` and `void Motor_SetState(uart_direction_t dir, uint8_t speed)` functions
  - **Why:** Docx §1B requires STM32-side command reception and motor actuation endpoints

  - **Added:** `#define UART_SendTelemetry(data_ptr) uart_send_telemetry(data_ptr)` macro alias
  - **Why:** Docx §1A references this exact function name as the M1 sending interface

- **pwr_management.h**
  - **Changed:** Nothing — file is identical to original
  - **Why:** Already compliant with integration rules; no docx requirements affected this interface

---

## M2_Vision

- **ai_vision.h**
  - **Changed:** Replaced `uint16_t bbox_area` in `vision_result_t` with a new `vision_bbox_t` struct containing `uint16_t x, y, w, h`
  - **Why:** Docx §3A defines `TargetData(bbox=(x,y,w,h), ...)` — a single area integer cannot convey bounding box coordinates needed by M4

  - **Added:** `int get_vision_results(vision_result_t *out_results, uint8_t max_results, uint8_t *out_count)` function
  - **Why:** Docx §3A names `get_vision_results()` as the primary data-provider function from M2 to M4, supporting multiple detections per frame

  - **Added:** `#define pause_vision_pipeline()` and `#define resume_vision_pipeline()` macro aliases
  - **Why:** Docx §3B specifies these exact function names as M2 receiving functions for the Resource-Aware STT Interrupt protocol

---

## M3_Acoustics

- **fsm_acoustic.h**
  - **Added:** Four new FSM states to `fsm_state_t`: `FSM_STATE_EVALUATE_VICTIM` (6), `FSM_STATE_WAKEUP_PROTOCOL` (7), `FSM_STATE_MANUAL_OVERRIDE` (8), `FSM_STATE_BEACON_MODE` (9); `FSM_STATE_RTH` shifted from 6 to 10
  - **Why:** Docx flow diagram defines 11 FSM states; the original enum only had 6 and was missing victim evaluation, wakeup, manual override, and beacon states

- **acoustics_iir.h**
  - **Changed:** Translated Turkish author placeholder to English (`[Student ID TBD]`)
  - **Why:** English-only documentation policy for the integrated workspace

- **acoustic_homing.py**
  - **Changed:** Translated Turkish author and dependency placeholders to English
  - **Why:** English-only documentation policy for the integrated workspace

- **MapManager_AcousticBeam.cs**
  - **Changed:** Translated Turkish author placeholder to English
  - **Why:** English-only documentation policy for the integrated workspace

---

## M4_Main *(New Module)*

- **M4_MainFSM.py**
  - **Added:** Entire file created from scratch — central FSM backbone with 8 interface sections: §1 UART, §2 WebSocket, §3 Vision, §4 STT, §5 FSM, §6 Report, §7 Failsafe, §8 Acoustic; plus `main()` entry point
  - **Why:** Docx requires a central Raspberry Pi coordinator that wires together all module interfaces (M1–M5); no such file existed in the original project

- **README.md**
  - **Added:** Entire file created from scratch with connection map, file table, API summary, and integration update log
  - **Why:** M4 is a new module and requires its own documentation for the integration workspace

---

## M5_Unity

- **DataContracts.cs**
  - **Changed:** Renamed `TelemetryData` fields: `posX` → `pos_x`, `posY` → `pos_y`, `temperature` → `temp`, `smokeDetected` → `smoke`, `victimStatus` → `victim_status` (type changed from `VictimStatus` enum to `string`), `priorityLevel` → `priority`
  - **Why:** Docx §2A defines WebSocket JSON keys as `{pos_x, pos_y, temp, smoke, victim_status, priority}` — Unity's `JsonUtility.FromJson` requires exact field name matching

  - **Added:** `[Serializable] public class CommandPacket { public bool @override; public string cmd; }`
  - **Why:** Docx §2B defines the operator command payload as `{override: bool, cmd: string}`

- **INetworkClient.cs**
  - **Changed:** `SendOperatorCommand(string command)` parameter type changed to `SendOperatorCommand(CommandPacket cmd)`
  - **Why:** Docx §2B requires the full `CommandPacket` object (with override flag) to be sent, not a raw string

- **MapManager.cs**
  - **Changed:** Updated docstring field references from `posX/posY/victimStatus/priorityLevel` to `pos_x/pos_y/victim_status/priority`
  - **Why:** Must match the renamed `TelemetryData` fields from DataContracts.cs per docx §2A

- **UIManager.cs**
  - **Changed:** Updated internal flow comments from `data.temperature/data.smokeDetected/data.victimStatus` to `data.temp/data.smoke/data.victim_status`
  - **Why:** Must match the renamed `TelemetryData` fields from DataContracts.cs per docx §2A

- **AudioManager.cs**
  - **Changed:** Translated Turkish author placeholder to English
  - **Why:** English-only documentation policy for the integrated workspace

---

## Change Code Summary

| Code | File | Description |
|------|------|-------------|
| R1 | `uart_comm.h` | Direction enum: int → char ('W','A','S','D','Q') |
| R2 | `uart_comm.h` | Merged buzzer/lights into single `action_flag` |
| R3 | `uart_comm.h` | Separated acoustic data into dedicated `uart_acoustic_t` struct |
| R4 | `uart_comm.h` | Added `uart_send_acoustic()`, `UART_ReceiveCommand()`, `Motor_SetState()` |
| R5 | `ai_vision.h` | Replaced `bbox_area` with `vision_bbox_t` struct (x, y, w, h) |
| R6 | `ai_vision.h` | Added `get_vision_results()` multi-detection function |
| R7 | `ai_vision.h` | Added `pause_vision_pipeline()` / `resume_vision_pipeline()` aliases |
| R8 | `fsm_acoustic.h` | Added 4 missing FSM states (EVALUATE_VICTIM, WAKEUP, MANUAL, BEACON) |
| R9 | `DataContracts.cs` | Renamed all `TelemetryData` fields to match docx §2A JSON keys |
| R10 | `DataContracts.cs` | Added `CommandPacket` class per docx §2B |
| R11 | `INetworkClient.cs` | `SendOperatorCommand` now takes `CommandPacket` instead of `string` |
| R12 | `MapManager.cs` | Docstring field references updated to new names |
| R13 | `UIManager.cs` | Internal comments updated to new field names |
