import json

from vosk import Model, KaldiRecognizer

from stt_engine_interface import ISTTEngine, VoiceCommandData


class STTEngine(ISTTEngine):
    def __init__(self):
        self.model = None

    def load_offline_model(self, model_path: str) -> bool:
        try:
            self.model = Model(model_path)
            return True
        except Exception:
            return False

    def process_audio_blob(self, wav_bytes: bytes) -> VoiceCommandData:
        if not self.model:
            raise RuntimeError("STT model not loaded")

        rec = KaldiRecognizer(self.model, 16000)
        rec.AcceptWaveform(wav_bytes)
        res = json.loads(rec.Result())
        text = res.get("text", "")

        intent = "UNKNOWN"
        lower_text = text.lower()

        if "dur" in lower_text or "stop" in lower_text:
            intent = "STOP"
        elif "geri dön" in lower_text or "home" in lower_text:
            intent = "RETURN_HOME"
        elif "ilerle" in lower_text or "forward" in lower_text:
            intent = "MOVE_FORWARD"
        elif "fener" in lower_text or "beacon" in lower_text:
            intent = "ACTIVATE_BEACON"

        return VoiceCommandData(raw_text=text, intent=intent, confidence=1.0)
