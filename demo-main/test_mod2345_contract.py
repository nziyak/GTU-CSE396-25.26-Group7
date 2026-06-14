import os
import sys
import time

REPO_ROOT = os.path.dirname(__file__)
sys.path.append(os.path.join(REPO_ROOT, "MOD-02_Vision"))
sys.path.append(os.path.join(REPO_ROOT, "MOD-03_Acoustics"))
sys.path.append(os.path.join(REPO_ROOT, "MOD-04_Web_STT"))

from ai_vision import VisionPipeline
from acoustic_homing import AcousticHomingBridge
from telemetry_contract import build_telemetry_payload


ALLOWED_VICTIM_STATUSES = {"NONE", "STANDING", "LYING", "TRAPPED"}
EXPECTED_PRIORITY_BY_STATUS = {
    "NONE": 0,
    "STANDING": 3,
    "LYING": 2,
    "TRAPPED": 1,
}
EXPECTED_PAYLOAD_KEYS = {
    "posX",
    "posY",
    "temperature",
    "smokeDetected",
    "victimStatus",
    "isStuck",
    "priorityLevel",
    "acousticHit",
    "acousticAngle",
}


def main():
    os.environ.setdefault("MOD02_ALLOW_MOCK_CAMERA", "1")

    vision = VisionPipeline()
    if not vision.initialize_camera():
        raise RuntimeError("MOD-02 Vision initialization failed.")

    bridge = AcousticHomingBridge()

    try:
        deadline = time.time() + 3.0
        report = None

        while time.time() < deadline:
            bridge.process_uart_line("|Temp:25.3|Smoke:0|A_Hit:1|A_Ang:45.0|\n")
            report = vision.build_augmented_status_report(
                pos_x=1.0,
                pos_y=2.0,
                temperature=25.3,
                smoke_detected=False,
                is_stuck=False,
                acoustic_hit=True,
                acoustic_angle=45.0,
            )

            payload = build_telemetry_payload(report)
            if payload["victimStatus"] != "NONE":
                break
            time.sleep(0.2)

        if report is None:
            raise AssertionError("No integration report could be built.")

        payload = build_telemetry_payload(report)

        assert set(payload.keys()) == EXPECTED_PAYLOAD_KEYS, payload
        assert payload["victimStatus"] in ALLOWED_VICTIM_STATUSES, payload
        assert payload["priorityLevel"] == EXPECTED_PRIORITY_BY_STATUS[payload["victimStatus"]], payload
        assert isinstance(payload["acousticHit"], bool), payload
        assert isinstance(payload["smokeDetected"], bool), payload
        assert isinstance(payload["isStuck"], bool), payload
        assert isinstance(payload["posX"], float), payload
        assert isinstance(payload["posY"], float), payload
        assert isinstance(payload["temperature"], float), payload
        assert isinstance(payload["acousticAngle"], float), payload

        latest_target = vision.get_latest_target()
        print("MOD-02 latest target:", latest_target)
        print("MOD-03/04/05 payload:", payload)
        print("Integration contract test passed.")
    finally:
        vision.shutdown()


if __name__ == "__main__":
    main()
