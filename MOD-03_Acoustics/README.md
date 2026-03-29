# MOD-03 — Acoustics & Navigation Pipeline

## Purpose

Provides the acoustic sensing and autonomous movement layer of the robot. It detects human distress calls via Acoustic Source Localization (ASL), filters motor noise using a Software-based Digital IIR Filter on the STM32, and manages spatial awareness through Spin-Scan Mapping and Acoustic Homing protocols.

---

## Authors

* **Uğur Anıl Güney** `[210104004011]` — Primary — STM32 Firmware & Acoustic Processing
* **Evrim Doğa Solmaz** `[230104004042]` — Secondary — Python Bridge & Navigation Interfacing
* **Tuana Melisa Aksoı** `[230104004903]` — Secondary — FSM Branching & Mode Transitions
* **Dicle Çoban** `[220104004088]` — Secondary — Unity Visualizer & Beam Mapping

---

## Dependencies

### Hardware

* 3x MAX4466 Microphones
* 4x HC-SR04 Ultrasonic Sensors

### Software / Libraries

* STM32 HAL Library + CMSIS-DSP (for IIR Filtering)
* Python: `RPi.GPIO`, `math` (for sound triangulation)

### Inter-Module Dependencies

* **MOD-01 (Embedded & Hardware):** UART packet must include `|A_Hit:x|A_Ang:y|` appended to MOD-01's telemetry string.
* **MOD-04 (Web Dashboard & STT):** M4 FSM dictates the high-level transition into Acoustic Homing.
* **MOD-05 (Unity Digital Twin):** Visually models the acoustic hit and bearing via the Top-Down Map.

---

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

---

## Data Structures

### `AcousticTelemetry`

Dataclass parsed from the incoming M1 UART string.
* `a_hit: bool` — True if a distinct loud sound (clap/shout) was confirmed.
* `a_ang: float` — Bearing from -180° to 180° representing source direction.

### `NavCommand`

Dataclass representing a navigation instruction sent down to M1.
* `direction: MotorDirection` — Values 0-4 mapping to STOP, FWD, BWD, LFT, RGT.
* `speed: int` — PWM driving speed (0-255).
* `buzzer_on: bool` — Triggers wakeup buzzer sequence.
* `lights_on: bool` — Triggers strobe / SOS LEDs.

---

## API Summary

### `acoustics_iir.h`

| Function | Parameters | Return | Description |
|---|---|---|---|
| `acoustics_iir_filter_apply()` | `samples: const float*`<br>`length: uint16_t`<br>`out: float*` | `acoustics_status_t` | Applies a 4th-order Digital IIR filter to raw ADC microphone array samples to attenuate gearbox/motor noise interference. |
| `acoustics_compute_bearing()` | `mic_buffers: const float*[]`<br>`length: uint16_t`<br>`out: acoustics_result_t*` | `acoustics_status_t` | Computes source bearing (-180° to 180°) via phase-difference among the 3 MAX4466 microphones. |
| `acoustics_spinscan_execute()` | `grid: acoustics_grid_t*` | `acoustics_status_t` | Executes a complete 360-degree rotation, pinging ultrasonics to populate the 2D arena occupancy grid. |
| `acoustics_homing_navigate()` | `bearing_deg: float`<br>`cmd: acoustics_nav_cmd_t*` | `acoustics_status_t` | Maps a confirmed bearing angle to a Modül 1 integer motor command (0=STOP, 1=FWD, 2=BWD, 3=LFT, 4=RGT). |

### `acoustic_homing.py`

| Function | Parameters | Return | Description |
|---|---|---|---|
| `process_telemetry()` | `telemetry: AcousticTelemetry` | `Optional[NavCommand]` | Parses A_Hit/A_Ang UART telemetry. Returns a `NavCommand` with `MotorDirection` (0-4), `buzzer_on`, and `lights_on`. |
| `notify_fsm_transition()` | `bearing: float` | `None` | Notifies MOD-04 FSM to transition from EXPLORE to ACOUSTIC_HOMING. |
| `reset()` | `None` | `None` | Resets internal acoustic hit counter when FSM returns to EXPLORE (false positive/timeout). |

### `fsm_acoustic.h`

| Function | Parameters | Return | Description |
|---|---|---|---|
| `FSM_Acoustic_Init()` | `void` | `void` | Resets internal hit streak, cooldown, and timers. Call once before FSM loop. |
| `FSM_Acoustic_Update()` | `fsm_state_t current_state, const fsm_acoustic_event_t *event, fsm_acoustic_result_t *result` | `bool` | Core branching logic. Accumulates 3 consecutive A_Hit confirmations, then fires EXPLORE → ACOUSTIC_HOMING transition. |
| `FSM_Acoustic_IsHomingTimedOut()` | `uint32_t now_ms` | `bool` | Returns true if ACOUSTIC_HOMING exceeded 30s timeout. |
| `FSM_Acoustic_ShouldInterruptExplore()` | `fsm_state_t current_state, const fsm_acoustic_event_t *event` | `bool` | Quick check for Evrim's bridge to pre-stop motors before formal state change. |
| `FSM_Acoustic_ResetStreak()` | `void` | `void` | Resets hit counter and cooldown on timeout or false positive. |
| `FSM_Acoustic_GetStateName()` | `fsm_state_t state` | `const char*` | Returns state label string for M4/M5 display. |

### `MapManager_AcousticBeam.cs`

| Function | Parameters | Return | Description |
|---|---|---|---|
| `ShowAcousticBeam()` | `data: AcousticBeamData`, `style: AcousticBeamStyle` | `void` | Renders a directional arrow or radar sweep on the 2D Top-Down Unity map when an acoustic hit is detected. |
| `HideAcousticBeam()` | `None` | `void` | Removes the acoustic beam visualization when homing times out or finishes. |
| `UpdateAcousticBeamAngle()` | `newBearingDeg: float` | `void` | Instantly adjusts the bearing of an already visible acoustic beam indicator (used for iterative bearing refinement). |

---

## Known Risks & Open Questions

### Risks

* **Acoustic Noise:** Motor/gearbox noise may interfere with microphone sensitivity.
  * **Mitigation:** Software-based Digital IIR Filter on STM32 to attenuate motor frequency band.
* **Reflection/Echo:** Indoor arenas may cause acoustic ghosting leading to incorrect bearing calculation.
  * **Mitigation:** Multiple bearing samples will be averaged before triggering FSM transition.

### Open Questions

* Should `SpinScan_Execute()` run only once at startup, or be re-triggered periodically if no victims are found within a set time?

---

## Version History

* **v0.6 (2026-03-29)** — Strict M1/M4 Sync implemented. `fsm_state_t` expanded from 7 to 11 states to match `M4_MainFSM.py`. `NavCommand` and `acoustics_nav_cmd_t` reverted to integer Enums (0-4) matching M1 UART, replacing `action_flag` with `buzzer_on`/`lights_on`.
* **v0.5 (2026-03-29)** — `acoustics_iir.h` API updated. Aligned `acoustics_nav_cmd_t` with Modül 1 UART standard ('W', 'S', 'A', 'D', 'Q') and applied `acoustics_` prefix to C functions.
* **v0.4 (2026-03-29)** — `acoustic_homing.py` API added. Aligned `NavCommand` with Modül 1 (W,A,S,D,Q and action flags) and updated the README contract.
* **v0.3 (2026-03-29)** — `fsm_acoustic.h` API added. Signatures aligned with M4_MainFSM.py and acoustic_homing.py.
* **v0.2 (2026-03-28)** — Team list finalized. UART packet format (`\|A_Hit:x\|A_Ang:y\|`) aligned with MOD-01/04 standards.
* **v0.1 (2026-03-25)** — Initial architecture draft; IIR filter and Homing logic defined.