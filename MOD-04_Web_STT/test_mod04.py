import threading
import time
from comms_dashboard import WebDashboard
from comms_dashboard_interface import AugmentedStatusReport
from stt_engine import STTEngine

def simulate_telemetry(dashboard: WebDashboard):
    """Her saniye sahte (mock) telemetri verisi yayınlayan arkaplan thread'i."""
    x = 0.0
    while True:
        report = AugmentedStatusReport(
            pos_x=x,
            pos_y=5.0,
            temperature=24.5,
            smoke_detected=False,
            victim_status="LYING",
            is_stuck=False,
            priority_level=2,
            acoustic_hit=True,
            acoustic_angle=90.0
        )
        dashboard.broadcast_telemetry(report)
        print(f"[Simülasyon] Telemetri gönderildi: X={x}")
        x += 0.5
        time.sleep(2)

if __name__ == "__main__":
    print("=== MOD-04 Test Sistemi Başlatılıyor ===")
    
    # 1. STT Motorunun Başlatılması
    stt = STTEngine()
    if stt.load_offline_model("vosk-model"):
        print("[STT] Motor hazır!")
    else:
        print("[STT] Uyarı: Model yüklenemedi. Sadece Dashboard test edilecek.")
    
    # 2. Web Dashboard'un Başlatılması
    dashboard = WebDashboard()
    
    # Telemetri simülasyonunu arka planda başlat
    sim_thread = threading.Thread(target=simulate_telemetry, args=(dashboard,), daemon=True)
    sim_thread.start()
    
    # Sunucuyu ana thread'de bloklayarak başlat
    try:
        dashboard.start_server(port=5001)
    except KeyboardInterrupt:
        print("\nSistem kapatılıyor...")
