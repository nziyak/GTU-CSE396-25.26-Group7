# MOD-03 — Acoustics & Navigation Pipeline

**Purpose:** Provides the acoustic sensing and autonomous movement layer of the robot. It detects human distress calls via Acoustic Source Localization (ASL), filters motor noise using a Software-based Digital IIR Filter on the STM32, and manages spatial awareness through Spin-Scan Mapping and Acoustic Homing protocols.

**Authors:**
- Uğur Anıl Güney [Student ID TBD] (Primary — STM32 Firmware & Acoustic Processing)
- Evrim Doğa Solmaz 230104004042 (Secondary — Python Bridge & Navigation Interfacing)
- Tuana Melisa Aksoy [Student ID TBD] (Secondary — FSM Branching & Mode Transitions)
- Dicle Çoban [Student ID TBD] (Secondary — Unity Visualizer & Beam Mapping)

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
  - Description: `IIR_Filter_Apply()`, `Acoustic_ComputeBearing()`, `SpinScan_Execute()`, `Homing_Navigate()` etc. to be added.
- `TODO: [Evrim]`
  - Description: `acoustic_homing.py` bridge functions to be added.
- `TODO: [Tuana]`
  - Description: `fsm_update()` acoustic branching logic to be added.
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

- v0.2 (2026-03-28): Team list finalized. UART packet format (`|A_Hit:x|A_Ang:y|`) aligned with MOD-01/04 standards.
- v0.1 (2026-03-25): Initial architecture draft; IIR filter and Homing logic defined.

## Interface Integration Updates

The following structural changes were applied to the files in this folder as part of the `Ceng_Integration` docx compliance revision:

- **[R8] Added 4 new FSM states to `fsm_state_t` enum** (`fsm_acoustic.h`): `EVALUATE_VICTIM`, `WAKEUP_PROTOCOL`, `MANUAL_OVERRIDE`, and `BEACON_MODE`. Required by the docx FSM flow diagram, proposal §3.2.2, docx §2B (Unity override), and proposal §3.2.3 (power-saving beacon mode).
- **`acoustics_iir.h`**: No changes required — bearing range, IIR filter interface, and SpinScan already comply with docx §4.
- **`acoustic_homing.py`**: No changes required — `AcousticTelemetry(a_hit, a_ang)` dataclass already matches the docx §4 UART format.
- **`MapManager_AcousticBeam.cs`**: No changes required — acoustic visualization is outside docx interface scope.
