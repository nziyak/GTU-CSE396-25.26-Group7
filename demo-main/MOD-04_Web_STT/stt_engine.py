import json
import os
import io
import wave
# pyrefly: ignore [missing-import]
from vosk import Model, KaldiRecognizer
from stt_engine_interface import ISTTEngine, VoiceCommandData

class STTEngine(ISTTEngine):
    """
    @brief Vosk-based offline Speech-to-Text engine.
           Optimized for high performance on Raspberry Pi.
    """
    def __init__(self):
        self.model = None

    def load_offline_model(self, model_path: str) -> bool:
        """
        @brief Loads the Vosk model into memory.
        @param model_path Directory path to the model weights.
        @return True if loaded successfully.
        """
        try:
            resolved_path = model_path
            # If not found in the current working directory, try relative search based on this script's directory
            if not os.path.isabs(model_path) and not os.path.exists(model_path):
                possible_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), model_path)
                if os.path.exists(possible_path):
                    resolved_path = possible_path
            
            print(f"Loading: Vosk model ({resolved_path})...")
            self.model = Model(resolved_path)
            print("Model loaded successfully.")
            return True
        except Exception as e:
            print(f"Error loading model: {e}")
            return False

    def process_audio_blob(self, wav_bytes: bytes) -> VoiceCommandData:
        """
        @brief Takes a .wav byte array, converts the voice command to text, and extracts intent.
        @param wav_bytes Audio data to be processed.
        @return Parsed voice command data.
        """
        if not self.model:
            raise RuntimeError("Model is not loaded. Call load_offline_model() first.")
        
        # If the data contains a RIFF/WAVE header, parse it using the wave module and extract raw PCM bytes.
        if wav_bytes.startswith(b'RIFF'):
            try:
                with wave.open(io.BytesIO(wav_bytes), "rb") as wf:
                    samplerate = wf.getframerate()
                    pcm_data = wf.readframes(wf.getnframes())
                    print(f"[STT] WAV header detected. Samplerate={samplerate} Hz, DataSize={len(pcm_data)} bytes")
            except Exception as e:
                print(f"[STT] Error parsing WAV header: {e}. Defaulting to 16000 Hz and raw bytes.")
                pcm_data = wav_bytes
                samplerate = 16000
        else:
            pcm_data = wav_bytes
            samplerate = 16000

        rec = KaldiRecognizer(self.model, samplerate)
        
        # Accept the audio waveform into the recognizer
        rec.AcceptWaveform(pcm_data)
        
        # Extract the result from JSON format
        res = json.loads(rec.Result())
        text = res.get("text", "")
        
        # Simple Intent Extraction
        intent = "UNKNOWN"
        confidence = 1.0 # Vosk does not return a confidence score by default, so we hardcode it to 1.0
        
        # Rule-based simple intent recognition (can be expanded with an NLP model later)
        lower_text = text.lower()
        if "dur" in lower_text or "stop" in lower_text:
            intent = "STOP"
        elif "geri dön" in lower_text or "home" in lower_text:
            intent = "RETURN_HOME"
        elif "ilerle" in lower_text or "forward" in lower_text:
            intent = "MOVE_FORWARD"
        elif "fener" in lower_text or "beacon" in lower_text:
            intent = "ACTIVATE_BEACON"
            
        print(f"[STT] Recognized Text: '{text}' -> Extracted Intent: {intent}")
            
        return VoiceCommandData(raw_text=text, intent=intent, confidence=confidence)
