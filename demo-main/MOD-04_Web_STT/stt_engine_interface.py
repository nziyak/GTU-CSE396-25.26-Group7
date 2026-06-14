"""
@file      stt_engine_interface.py
@brief     Public Interface Contract for Offline Speech-to-Text (Vosk/Whisper)
@author    Tuana
@date      2026-03-29
@version   0.1

Changelog:
v0.1 - Initial draft: Defined STT initialization and audio blob processing.
"""

from dataclasses import dataclass
from abc import ABC, abstractmethod
from typing import Optional

@dataclass
class VoiceCommandData:
    """
    @brief Parsed command extracted from the raw STT text output.
    """
    raw_text: str
    intent: str           # e.g., "STOP", "RETURN_HOME", "ACTIVATE_BEACON"
    confidence: float


class ISTTEngine(ABC):
    """
    @brief Interface for the Edge Speech-to-Text processing pipeline.
    """

    @abstractmethod
    def load_offline_model(self, model_path: str) -> bool:
        """
        @brief Loads the quantized Vosk/Whisper model into Raspberry Pi RAM.
        @param model_path Local directory path to the model weights.
        @return True if loaded successfully.
        """
        pass

    @abstractmethod
    def process_audio_blob(self, wav_bytes: bytes) -> VoiceCommandData:
        """
        @brief Converts a raw PCM .wav byte array into parsed text commands.
               NOTE: This is a heavy blocking call. Mod 2 Vision must be paused before calling!
        @param wav_bytes Audio data received via WebSocket from Unity.
        @return VoiceCommandData containing the recognized intent.
        """
        pass