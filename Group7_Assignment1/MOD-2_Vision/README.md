# MOD-02 — AI & Vision Pipeline

## Purpose

This module manages the visual perception and classification layer on the Raspberry Pi 5.

It integrates the Pi Camera Module V3, performs real-time human detection using a YOLO-based detector, and classifies victim severity as `TRAPPED`, `LYING`, or `STANDING` using an edge-side analysis pipeline. The module also supports dynamic FPS switching and temporary pause/resume behavior during resource-heavy STT operations.

---

## Authors

* **Fatma Öztürk** `[230104004152]` — Primary — AI Pipeline & Model Selection
* **Gabil Rahimli** `[230104004902]` — Primary — AI Pipeline, Model Selection & YOLO Training
* **Evrim Doğa Solmaz** `[230104004042]` — Primary — Vision Integration & Hardware Interfacing
* **Tuana Melisa Aksoi** `[230104004903]` — Secondary — Model Testing & Severity Classification
* **Uğur Anıl Güney** `[210104004011]` — Secondary — Dataset Preparation & YOLO Training
* **Dicle Çoban** `[220104004088]` — Secondary — Performance Optimization & Resource Management

> Replace the placeholder student IDs before submission.

---

## Dependencies

### Hardware

* Raspberry Pi 5 (8GB)
* Active Cooler
* Pi Camera Module V3

### Libraries

* Python 3.10+
* OpenCV
* Ultralytics YOLOv8
* Moondream2 (or alternative edge-side classifier backend)

### Inter-Module Dependencies

* **MOD-04 (Web Dashboard & STT):** vision pipeline may be paused during STT execution
* **MOD-05 (Unity Digital Twin):** victim status / priority output must remain compatible with dashboard and pin-color visualization
* **FSM / higher-level control:** consumes final detection and victim analysis results

---

## Quick-Start Integration Example

```python
from ai_vision import VisionPipeline

vision = VisionPipeline()

status = vision.ai_vision_init()
if status < 0:
    raise RuntimeError("Vision pipeline initialization failed.")

vision.ai_vision_set_power_mode(False)  # Explore mode

status, result = vision.ai_vision_process_frame()
if status == 0 and result is not None and result.target_detected:
    print("Victim detected:", result.status, result.priority, result.confidence)

vision.ai_vision_preemptive_pause()
# STT or another high-priority task can run here
vision.ai_vision_resume()

vision.ai_vision_shutdown()
```

---

## API Summary

| Function / Method              | Parameters       | Return                             | Description                                                                      |
| ------------------------------ | ---------------- | ---------------------------------- | -------------------------------------------------------------------------------- |
| `ai_vision_init()`             | None             | `int`                              | Initializes camera-side and model-side resources for the vision pipeline         |
| `ai_vision_set_power_mode()`   | `bool high_perf` | `None`                             | Switches between low-power exploration mode and high-performance assessment mode |
| `ai_vision_preemptive_pause()` | None             | `None`                             | Temporarily pauses the vision pipeline to free compute / memory resources        |
| `ai_vision_resume()`           | None             | `None`                             | Resumes the vision pipeline after a temporary pause                              |
| `ai_vision_process_frame()`    | None             | `tuple[int, VisionResult \| None]` | Processes the current frame and returns status plus optional result object       |
| `ai_vision_shutdown()`         | None             | `None`                             | Releases camera / model resources and safely shuts down the module               |

---

## Public Data Types

### `VisionStatus`

Represents the victim status produced by the pipeline.

Possible values:

* `NONE`
* `STANDING`
* `LYING`
* `TRAPPED`

### `VisionPriority`

Represents the priority / pin color mapping used by upper layers.

Possible values:

* `GREEN`
* `YELLOW`
* `RED`

### `VisionResult`

Represents the final output of one frame-processing cycle.

Fields:

* `target_id: int`
* `status: VisionStatus`
* `priority: VisionPriority`
* `confidence: float`
* `bbox_area: int`
* `target_detected: bool`

---

## Internal Structure

The public integration point of MOD-02 is `ai_vision.py`.

Internally, the module is divided into helper files to improve readability, maintainability, and separation of responsibilities. These internal files are **not intended to be called directly by external modules**.

---

### `camera_internal.py`

**Purpose:**
Handles camera-side operations such as initialization, frame capture, FPS updates, frame cleanup, and shutdown.

| Internal Function / Method      | Parameters          | Return                | Description                                           |
| ------------------------------- | ------------------- | --------------------- | ----------------------------------------------------- |
| `vision_camera_init()`          | `int fps`           | `int`                 | Initializes the Pi Camera with the requested FPS      |
| `vision_camera_capture_frame()` | None                | `CameraFrame \| None` | Captures one frame from the camera stream             |
| `vision_camera_set_fps()`       | `int fps`           | `int`                 | Updates camera FPS according to runtime mode          |
| `vision_camera_release_frame()` | `CameraFrame frame` | `None`                | Releases resources associated with the captured frame |
| `vision_camera_shutdown()`      | None                | `None`                | Shuts down the camera interface safely                |

**What this internal layer does**

* opens and configures the Pi Camera
* provides the current frame to the pipeline
* applies low-power / high-performance FPS changes
* cleans temporary frame data after processing

---

### `human_detector_internal.py`

**Purpose:**
Handles YOLO-based human detection on camera frames and selects the best candidate target when multiple detections exist.

| Internal Function / Method      | Parameters                           | Return                   | Description                                            |
| ------------------------------- | ------------------------------------ | ------------------------ | ------------------------------------------------------ |
| `vision_detector_init()`        | None                                 | `int`                    | Initializes the human detector backend                 |
| `vision_detector_detect()`      | `CameraFrame frame, float threshold` | `list[HumanDetection]`   | Detects human candidates in the given frame            |
| `vision_detector_select_best()` | `list[HumanDetection] detections`    | `HumanDetection \| None` | Selects the best candidate from multiple detections    |
| `vision_detector_shutdown()`    | None                                 | `None`                   | Releases detector resources and shuts down the backend |

**What this internal layer does**

* runs person detection on the incoming frame
* filters weak detections using a confidence threshold
* keeps multiple candidates if more than one person is detected
* chooses the most relevant target for the next analysis stage

---

### `victim_analyzer_internal.py`

**Purpose:**
Classifies the selected target as `TRAPPED`, `LYING`, or `STANDING` and converts the result into the public `VisionResult` structure.

| Internal Function / Method       | Parameters                                                | Return           | Description                                                       |
| -------------------------------- | --------------------------------------------------------- | ---------------- | ----------------------------------------------------------------- |
| `vision_analyzer_init()`         | None                                                      | `int`            | Initializes the victim severity analysis backend                  |
| `vision_analyzer_classify()`     | `CameraFrame frame, HumanDetection target, int target_id` | `VisionResult`   | Classifies victim condition and produces the public result object |
| `vision_analyzer_map_priority()` | `VisionStatus status`                                     | `VisionPriority` | Maps victim status to dashboard / Unity priority color            |
| `vision_analyzer_shutdown()`     | None                                                      | `None`           | Releases analyzer resources and shuts down the backend            |

**What this internal layer does**

* analyzes the selected person candidate in more detail
* predicts whether the person is standing, lying, or trapped
* maps the output into priority / pin-color compatible form
* produces the final public result object used by upper modules

---

### Internal Runtime Logic

In addition to the internal files above, MOD-02 also contains runtime control logic inside `ai_vision.py`.

This runtime logic is responsible for:

* switching between exploration mode and assessment mode
* dynamic FPS switching
* pause / resume behavior during STT interrupts
* resource-aware execution on the Raspberry Pi 5

This logic may remain inside the main implementation file even if it is not separated into a dedicated scheduler file.

---

## Known Risks & Open Questions

### Risks

**Thermal Risk**
Continuous VLM inference may cause the Raspberry Pi 5 to overheat or trigger Out-Of-Memory errors.

**Mitigation:**
Dynamic FPS management and temporary pause/resume behavior are used to reduce thermal and memory pressure.

**Latency Risk**
Moondream2 may be too slow for real-time victim assessment on edge hardware.

**Mitigation:**
Fallback options such as a custom CNN or YOLO-Pose-based rule logic will be considered if latency exceeds the target threshold.

### Open Questions

* Should the proximity tie-breaker be handled inside the Vision pipeline or by the higher-level FSM?
* Should final victim-priority mapping remain fully inside MOD-02, or be normalized through a shared cross-module contract?

---

## Known Limitations and TODOs

### Known Limitations

* Final on-device benchmark comparison between VLM, custom CNN, and YOLO-Pose is still pending.
* The current pipeline is centered on single-frame processing; multi-target orchestration may later need extra FSM support.
* Some student IDs are still missing in this README draft.

### TODOs

* Complete Raspberry Pi 5 latency tests
* Finalize all student IDs
* Confirm final JSON / data-contract compatibility with MOD-04 and MOD-05
* Validate target tie-break behavior for multi-person scenes

---

## Version History

* **v0.4 (2026-03-29)** — README updated to reflect Python-based implementation structure and final public/internal module decomposition
* **v0.3 (2026-03-29)** — README updated to match final public API and internal module decomposition
* **v0.2 (2026-03-28)** — Full team list updated; victim status and priority mapping aligned with integration planning
* **v0.1 (2026-03-25)** — Initial module architecture draft
