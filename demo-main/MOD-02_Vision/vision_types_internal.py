"""
@file      vision_types_internal.py
@brief     Shared internal enums and result types for MOD-02 Vision
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, IntEnum


class VisionStatus(str, Enum):
    """Internal normalized victim status labels."""

    NONE = "NONE"
    STANDING = "STANDING"
    LYING = "LYING"
    TRAPPED = "TRAPPED"


class VisionPriority(IntEnum):
    """Priority values aligned with MOD-04/MOD-05 telemetry contract."""

    NONE = 0
    RED = 1
    YELLOW = 2
    GREEN = 3


@dataclass
class VisionResult:
    """Internal output of one completed detection + classification cycle."""

    target_id: int
    status: VisionStatus
    priority: VisionPriority
    confidence: float
    bbox_area: int
    target_detected: bool
    pos_x: int
    pos_y: int
    distance_cm: float
