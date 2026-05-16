import json
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
        
        # Vosk küçük (small) modelleri için genel varsayılan örnekleme hızı 16000 Hz'dir.
        # İdeal olarak wav_bytes içinden wav header parse edilerek samplerate alınmalıdır,
        # ancak basitlik adına burada sabit 16000 Hz kabul ediyoruz.
        rec = KaldiRecognizer(self.model, 16000)
        
        # Gelen tüm ses verisini işleyiciye kabul ettiriyoruz
        rec.AcceptWaveform(wav_bytes)
        
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
