"""
@file      ai_vision_interface.py
@brief     Public Interface Contract for the Vision & AI Pipeline (YOLO + VLM)
@author    Evrim & Fatma
@date      2026-03-29
@version   0.1

Changelog:
v0.1 - Initial draft, defined Data Classes and IVisionPipeline Abstract Class.
"""

from dataclasses import dataclass
from abc import ABC, abstractmethod
from typing import Optional

# -- Data Types -------------------------------

@dataclass
class TargetData:
    """
    @brief Holds bounding box and severity classification of a detected human.
    """
    pos_x: int            # Pixel X coordinate
    pos_y: int            # Pixel Y coordinate
    distance_cm: float    # Estimated distance from camera
    severity: str         # "TRAPPED", "LYING", "STANDING", "NONE"
    confidence: float     # AI confidence score (0.0 to 1.0)


# -- Interface Contract ---------------------------------------------------

class IVisionPipeline(ABC):
    """
    @brief Main contract for Mod 2. Other modules will call these methods.
    """

    @abstractmethod
    def initialize_camera(self) -> bool:
        """
        @brief Warms up the Pi Camera V3 and loads YOLO/VLM weights into RAM.
        @return True if initialization is successful, False otherwise.
        """
        pass

    @abstractmethod
    def get_latest_target(self) -> Optional[TargetData]:
        """
        @brief Retrieves the most recent AI classification result.
        @return TargetData object if a human is detected, None otherwise.
        """
        pass

    @abstractmethod
    def pause_vision_pipeline(self) -> None:
        """
        @brief STRICT REQUIREMENT: Pauses all camera framing and AI inference.
               Must be called by Mod 4 (STT) to free up RAM/CPU before voice processing.
        """
        pass

    @abstractmethod
    def resume_vision_pipeline(self) -> None:
        """
        @brief Resumes AI inference after STT processing is complete.
        """
        pass