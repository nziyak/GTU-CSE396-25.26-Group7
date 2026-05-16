import json
import io
import wave
# pyrefly: ignore [missing-import]
from vosk import Model, KaldiRecognizer
from stt_engine_interface import ISTTEngine, VoiceCommandData

class STTEngine(ISTTEngine):
    """
    @brief Vosk tabanlı çevrimdışı Speech-to-Text motoru.
           Raspberry Pi üzerinde yüksek performans için optimize edilmiştir.
    """
    def __init__(self):
        self.model = None

    def load_offline_model(self, model_path: str) -> bool:
        """
        @brief Vosk modelini belleğe yükler.
        @param model_path Model dosyalarının bulunduğu klasör yolu
        @return Yükleme başarılıysa True
        """
        try:
            print(f"Yükleniyor: Vosk modeli ({model_path})...")
            self.model = Model(model_path)
            print("Model başarıyla yüklendi.")
            return True
        except Exception as e:
            print(f"Model yüklenirken hata oluştu: {e}")
            return False

    def process_audio_blob(self, wav_bytes: bytes) -> VoiceCommandData:
        """
        @brief .wav bayt dizisini alır ve içindeki sesli komutu metne dönüştürüp niyet (intent) çıkarır.
        @param wav_bytes İşlenecek ses verisi
        @return Çözümlenmiş komut verisi
        """
        if not self.model:
            raise RuntimeError("Model henüz yüklenmedi. Önce load_offline_model() çağrılmalıdır.")
        
        pcm_bytes, sample_rate = self._extract_pcm_from_wav(wav_bytes)
        rec = KaldiRecognizer(self.model, sample_rate)
        
        # Gelen tüm ses verisini işleyiciye kabul ettiriyoruz
        rec.AcceptWaveform(pcm_bytes)
        
        # Sonucu JSON formatından ayıklıyoruz
        res = json.loads(rec.Result())
        text = res.get("text", "")
        
        # Basit Intent (Niyet) Çıkarımı
        intent = "UNKNOWN"
        confidence = 1.0 # Vosk, varsayılan olarak basit bir güven skoru dönmez, burayı sabitledik
        
        # Basit kural tabanlı intent tespiti (gelecekte NLP modeliyle genişletilebilir)
        lower_text = text.lower()
        if "dur" in lower_text or "stop" in lower_text:
            intent = "STOP"
        elif "geri dön" in lower_text or "home" in lower_text:
            intent = "RETURN_HOME"
        elif "ilerle" in lower_text or "forward" in lower_text:
            intent = "MOVE_FORWARD"
        elif "fener" in lower_text or "beacon" in lower_text:
            intent = "ACTIVATE_BEACON"
            
        print(f"[STT] Algılanan Metin: '{text}' -> Çıkarılan Niyet: {intent}")
            
        return VoiceCommandData(raw_text=text, intent=intent, confidence=confidence)

    def _extract_pcm_from_wav(self, wav_bytes: bytes) -> tuple[bytes, int]:
        """
        @brief Unity'den gelen RIFF/WAV paketinden PCM frame'lerini ve sample rate'i çıkarır.
        """
        try:
            with wave.open(io.BytesIO(wav_bytes), "rb") as wav_file:
                sample_rate = wav_file.getframerate()
                channels = wav_file.getnchannels()
                sample_width = wav_file.getsampwidth()
                pcm_bytes = wav_file.readframes(wav_file.getnframes())

            if sample_width != 2:
                raise ValueError(f"Only 16-bit PCM WAV is supported, got {sample_width * 8}-bit.")

            if channels > 1:
                pcm_bytes = self._take_first_channel_pcm16(pcm_bytes, channels)

            return pcm_bytes, sample_rate
        except wave.Error:
            # Backward-compatible fallback for callers that already send raw 16 kHz PCM.
            return wav_bytes, 16000

    def _take_first_channel_pcm16(self, pcm_bytes: bytes, channels: int) -> bytes:
        frame_size = channels * 2
        mono = bytearray()

        for index in range(0, len(pcm_bytes), frame_size):
            mono.extend(pcm_bytes[index:index + 2])

        return bytes(mono)
