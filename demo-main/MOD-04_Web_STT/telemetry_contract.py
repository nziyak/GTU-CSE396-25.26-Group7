from typing import Any, Dict

from comms_dashboard_interface import AugmentedStatusReport


def build_telemetry_payload(report: AugmentedStatusReport) -> Dict[str, Any]:
    """
    Convert the shared telemetry dataclass into the exact camelCase payload
    expected by MOD-05 Unity's TelemetryData contract.
    """
    return {
        "posX": report.pos_x,
        "posY": report.pos_y,
        "temperature": report.temperature,
        "smokeDetected": report.smoke_detected,
        "victimStatus": report.victim_status,
        "isStuck": report.is_stuck,
        "priorityLevel": report.priority_level,
        "acousticHit": report.acoustic_hit,
        "acousticAngle": report.acoustic_angle,
    }
