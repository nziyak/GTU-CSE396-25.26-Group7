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

if not vision.initialize_camera():
    raise RuntimeError("Vision pipeline initialization failed.")

target = vision.get_latest_target()
if target is not None and target.severity != "NONE":
    print(f"Victim detected — severity: {target.severity}, confidence: {target.confidence:.2f}, distance: {target.distance_cm} cm")

vision.pause_vision_pipeline()
# STT or another high-priority task can run here
vision.resume_vision_pipeline()
```

---

## API Summary

> **Note:** The public integration contract is defined in `ai_vision_interface.py` via the `IVisionPipeline` abstract class. The concrete implementation (`VisionPipeline` in `ai_vision.py`) must implement all four methods listed below.

| Method (`IVisionPipeline`)    | Return                    | Description                                                                                   |
| ----------------------------- | ------------------------- | --------------------------------------------------------------------------------------------- |
| `initialize_camera()`         | `bool`                    | Warms up the Pi Camera V3 and loads YOLO / VLM weights into RAM. Returns `True` on success.  |
| `get_latest_target()`         | `TargetData \| None`      | Returns the most recent AI classification result, or `None` if no human is currently detected.|
| `pause_vision_pipeline()`     | `None`                    | Pauses all camera framing and AI inference. **Must** be called by Mod 4 (STT) before voice processing to free RAM / CPU. |
| `resume_vision_pipeline()`    | `None`                    | Resumes AI inference after STT processing is complete.                                        |

---

## Public Data Types

### `TargetData`

The single public data structure returned by `get_latest_target()`. Defined in `ai_vision_interface.py`.

Fields:

* `pos_x: int` — Pixel X coordinate of the detected target
* `pos_y: int` — Pixel Y coordinate of the detected target
* `distance_cm: float` — Estimated distance from the camera in centimetres
* `severity: str` — Victim condition classification; one of `"TRAPPED"`, `"LYING"`, `"STANDING"`, or `"NONE"`
* `confidence: float` — AI confidence score in the range `[0.0, 1.0]`

---

> **Compatibility note for MOD-04 / MOD-05:** Upper layers consuming victim status and priority colour must map `TargetData.severity` to their own priority enum. A shared cross-module contract for this mapping is an open question — see Known Risks section below.

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
Classifies the selected target as `TRAPPED`, `LYING`, or `STANDING` and converts the result into the internal `VisionResult` structure, which is then translated into the public `TargetData` format by the main pipeline.

> **Internal types used only within MOD-02:**
> * `VisionStatus` — `NONE`, `STANDING`, `LYING`, `TRAPPED`
> * `VisionPriority` — `GREEN`, `YELLOW`, `RED`
> * `VisionResult` — internal output of one frame-processing cycle (fields: `target_id`, `status`, `priority`, `confidence`, `bbox_area`, `target_detected`)

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

* **v0.5 (2026-03-29)** — README corrected to match `ai_vision_interface.py` contract: API table updated to reflect `IVisionPipeline` abstract methods, Quick-Start example fixed, public data type updated from `VisionResult` to `TargetData`, internal types (`VisionStatus`, `VisionPriority`, `VisionResult`) moved under `victim_analyzer_internal.py` section
* **v0.4 (2026-03-29)** — README updated to reflect Python-based implementation structure and final public/internal module decomposition
* **v0.3 (2026-03-29)** — README updated to match final public API and internal module decomposition
* **v0.2 (2026-03-28)** — Full team list updated; victim status and priority mapping aligned with integration planning
* **v0.1 (2026-03-25)** — Initial module architecture draft
