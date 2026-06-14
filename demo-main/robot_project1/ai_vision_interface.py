from dataclasses import dataclass
from abc import ABC, abstractmethod
from typing import Optional


@dataclass
class TargetData:
    pos_x: int
    pos_y: int
    distance_cm: float
    severity: str
    confidence: float


class IVisionPipeline(ABC):
    @abstractmethod
    def initialize_camera(self) -> bool:
        pass

    @abstractmethod
    def get_latest_target(self) -> Optional[TargetData]:
        pass

    @abstractmethod
    def pause_vision_pipeline(self) -> None:
        pass

    @abstractmethod
    def resume_vision_pipeline(self) -> None:
        pass
