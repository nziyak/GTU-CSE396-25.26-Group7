# MOD-02 — AI & Vision Pipeline

## Purpose
This module manages the visual perception and classification layer on the Raspberry Pi 5.  
It integrates the Pi Camera V3 to perform real-time human detection (YOLO) and utilizes a quantized VLM (Moondream2) for victim severity assessment (Trapped / Lying / Standing).

---

## Authors
- **Fatma Öztürk** [230104004152] (Primary - AI Pipeline & Model Selection)
- **Gabil Rahimli** [230104004902] (Primary - AI Pipeline & Model Selection & Yolo Training)
- **Evrim Doğa Solmaz** [Öğrenci Nonu Yaz] (Primary - Vision Integration & Hardware Interfacing)
- **Tuana Melisa Aksoi** [Öğrenci Nonu Yaz] (Secondary - Model Testing & Severity Classification)
- **Uğur Anıl Güney** [210104004011] (Secondary - Dataset Preparation & YOLO Training)
- **Dicle Çoban** [Öğrenci Nonu Yaz] (Secondary - Performance Optimization & Resource Management)

---

## Dependencies
- **Hardware:** Raspberry Pi 5 (8GB) + Active Cooler  
- **Libraries:** OpenCV, Ultralytics YOLOv8, Moondream2 (Quantized VLM)  
- **Data Contracts:** Must strictly match `VictimStatus` enums in MOD-05 (Unity) for JSON serialization  

---

## Quick-Start Integration Example

```python
# Implementation of the Resource-Aware Interrupt protocol
# to prevent OOM during STT (MOD-04) operations.

from ai_vision import VisionPipeline, ResourceManager

vision = VisionPipeline()
manager = ResourceManager()

# Check for interrupt from MOD-04 (STT)
if stt_is_starting:
    manager.pause_vision_for_stt()  # Releases GPU/RAM for STT pipeline
    # Execute STT processing...
    manager.resume_vision()  # Resume YOLO/VLM inference
```
---

## API Summary

| Function                      | Parameters        | Return            | Description |
|-----------------------------|------------------|------------------|-------------|
| `ai_vision_init()`          | None             | int              | Initializes camera and loads AI models into Pi 5 memory |
| `ai_vision_set_power_mode()`| `bool high_perf` | void             | Toggles FPS (5 vs 25) to manage thermal load |
| `ai_vision_preemptive_pause()` | None          | void             | Pauses models to prioritize MOD-04 (STT) tasks |
| `ai_vision_process_frame()` | `vision_result_t*` | int            | Executes YOLO/VLM and returns victim status & priority |

---

## Known Risks & Open Questions

### Risks
- **Thermal:** Continuous VLM inference may cause the Pi 5 to overheat or trigger OOM errors  
  - *Mitigation:* Dynamic FPS management and preemptive scheduling  

- **Latency:** Moondream2 VLM might be too slow for real-time assessment  
  - *Mitigation:* Fallback to a custom CNN or YOLO-Pose engine if latency exceeds 5s  

### Open Question
- Should the **"Proximity Tie-Breaker"** logic be handled within the Vision pipeline or the FSM?

---

## Version History

- **v0.2 (2026-03-28):** Full team list updated. Aligned `VictimStatus` and `PriorityLevel` with MOD-05 Unity interface  
- **v0.1 (2026-03-25):** Initial architecture draft  
