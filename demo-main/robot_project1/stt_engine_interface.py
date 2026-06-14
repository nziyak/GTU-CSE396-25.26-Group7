from dataclasses import dataclass
from abc import ABC, abstractmethod


@dataclass
class VoiceCommandData:
    raw_text: str
    intent: str
    confidence: float


class ISTTEngine(ABC):
    @abstractmethod
    def load_offline_model(self, model_path: str) -> bool:
        pass

    @abstractmethod
    def process_audio_blob(self, wav_bytes: bytes) -> VoiceCommandData:
        pass
