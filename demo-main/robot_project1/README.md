# Robot Project MVP

This folder contains the Raspberry Pi MVP integration for:

- `MOD-02`: live vision pipeline
- `MOD-04`: Flask + Socket.IO bridge
- `MOD-05`: Unity dashboard client

## What runs where

- Raspberry Pi:
  - `robot_project/pi_main.py`
  - `MOD-02_Vision/*`
  - `MOD-04_Web_STT/*`
- Unity PC:
  - `MOD-05_Unity/*`

## MVP scope

This MVP does not require STM32 yet.
The following fields are mocked on Pi for now:

- `pos_x`
- `pos_y`
- `temperature`
- `smoke_detected`
- `is_stuck`
- `acoustic_hit`
- `acoustic_angle`

The following fields are real:

- `victim_status`
- `priority_level`
- `video_frame`

## Start on Pi

1. Install Python dependencies from `robot_project/requirements-pi.txt`
2. Place your models where `MOD-02_Vision/ai_vision.py` can find them, or set:
   - `MOD02_DETECTOR_MODEL_PATH`
   - `MOD02_SEVERITY_MODEL_PATH`
   - `MOD02_CLASS_NAMES_PATH`
3. Run:

```powershell
python robot_project/pi_main.py
```

The server starts on port `5001`.

## Unity connection

In `RobotManager` set:

- `serverUrl = ws://<RASPBERRY_PI_IP>:5001`

Keep `useMockFileData = false`.
