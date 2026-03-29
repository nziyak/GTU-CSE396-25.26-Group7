# MOD-03 — Acoustics & Navigation Pipeline

**Purpose:** Provides the acoustic sensing and autonomous movement layer of the robot. It detects human distress calls via Acoustic Source Localization (ASL), filters motor noise using a Software-based Digital IIR Filter on the STM32, and manages spatial awareness through Spin-Scan Mapping and Acoustic Homing protocols.

**Authors:**
- Uğur Anıl Güney [Öğrenci No Yaz] (Primary — STM32 Firmware & Acoustic Processing)
- Evrim Doğa Solmaz 230104004042 (Secondary — Python Bridge & Navigation Interfacing)
- Tuana Melisa Aksoı 230104004903 (Secondary — FSM Branching & Mode Transitions)
- Dicle Çoban [Öğrenci No Yaz] (Secondary — Unity Visualizer & Beam Mapping)

**Dependencies:**
- STM32 HAL Library + CMSIS-DSP (for IIR Filtering)
- Hardware: 3x MAX4466 Microphones, HC-SR04 Ultrasonic Sensors
- Python: `RPi.GPIO`, `math` (for triangulation)
- Data Contract: UART packet must include `|A_Hit:x|A_Ang:y|` appended to MOD-01's telemetry string

## Quick-Start Integration Example

```python
# Acoustic Homing & FSM Transition — Python side (Evrim)
from acoustics import AcousticProcessor, NavigationFSM

acoustics = AcousticProcessor()
fsm = NavigationFSM()

# Check for acoustic hit from STM32 via UART (parsed by MOD-04)
if telemetry_data['A_Hit'] == 1:
    # Trigger transition from EXPLORE to ACOUSTIC_HOMING
    fsm.transition_to_homing(bearing=telemetry_data['A_Ang'])
    # Execute motor commands to rotate towards sound source
    acoustics.align_to_source()
```

## API Summary

- `TODO: [Uğur]`
  - Description: `acoustics_iir_filter_apply()`, `acoustics_compute_bearing()`, `acoustics_spinscan_execute()`, `acoustics_homing_navigate()` etc. to be added.
- **Evrim — `acoustic_homing.py`** (Acoustic Homing Bridge & MOD-01 Navigation Interfacing)

| Function | Parameters | Return | Description |
|---|---|---|---|
| `process_telemetry()` | `telemetry: AcousticTelemetry` | `Optional[NavCommand]` | Parses A_Hit/A_Ang UART telemetry. Returns a `NavCommand` with `MotorDirection` (W,A,S,D,Q) and `action_flag`. |
| `notify_fsm_transition()` | `bearing: float` | `None` | Notifies MOD-04 FSM to transition from EXPLORE to ACOUSTIC_HOMING. |
| `reset()` | `None` | `None` | Resets internal acoustic hit counter when FSM returns to EXPLORE (false positive/timeout). |
- **Tuana — `fsm_acoustic.h`** (FSM Branching & Mode Transitions)

| Function | Parameters | Return | Description |
|---|---|---|---|
| `FSM_Acoustic_Init()` | `void` | `void` | Resets internal hit streak, cooldown, and timers. Call once before FSM loop. |
| `FSM_Acoustic_Update()` | `fsm_state_t current_state, const fsm_acoustic_event_t *event, fsm_acoustic_result_t *result` | `bool` | Core branching logic. Accumulates 3 consecutive A_Hit confirmations, then fires EXPLORE → ACOUSTIC_HOMING transition. |
| `FSM_Acoustic_IsHomingTimedOut()` | `uint32_t now_ms` | `bool` | Returns true if ACOUSTIC_HOMING exceeded 30s timeout. |
| `FSM_Acoustic_ShouldInterruptExplore()` | `fsm_state_t current_state, const fsm_acoustic_event_t *event` | `bool` | Quick check for Evrim's bridge to pre-stop motors before formal state change. |
| `FSM_Acoustic_ResetStreak()` | `void` | `void` | Resets hit counter and cooldown on timeout or false positive. |
| `FSM_Acoustic_GetStateName()` | `fsm_state_t state` | `const char*` | Returns state label string for M4/M5 display. |

- `TODO: [Dicle]`
  - Description: `MapManager.ShowAcousticBeam()` etc. to be added.

## Known Risks & Open Questions

**Risks:**
- **Acoustic Noise:** Motor/gearbox noise may interfere with microphone sensitivity.
  - Mitigation: Software-based Digital IIR Filter on STM32 to attenuate motor frequency band.
- **Reflection/Echo:** Indoor arenas may cause acoustic ghosting leading to incorrect bearing calculation.
  - Mitigation: Multiple bearing samples will be averaged before triggering FSM transition.

**Open Question:**
- Should `SpinScan_Execute()` run only once at startup, or be re-triggered periodically if no victims are found within a set time?

## Version History

- v0.5 (2026-03-29): `acoustics_iir.h` API updated. Aligned `acoustics_nav_cmd_t` with Modül 1 UART standard ('W', 'S', 'A', 'D', 'Q') and applied `acoustics_` prefix to C functions.
- v0.4 (2026-03-29): `acoustic_homing.py` API added. Aligned `NavCommand` with Modül 1 (W,A,S,D,Q and action flags) and updated the README contract.
- v0.3 (2026-03-29): `fsm_acoustic.h` API added. Signatures aligned with M4_MainFSM.py and acoustic_homing.py.
- v0.2 (2026-03-28): Team list finalized. UART packet format (`|A_Hit:x|A_Ang:y|`) aligned with MOD-01/04 standards.
- v0.1 (2026-03-25): Initial architecture draft; IIR filter and Homing logic defined.