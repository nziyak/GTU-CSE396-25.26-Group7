import sys
import os
import threading
import time

# MOD-03 ve MOD-04 klasörlerini Python yoluna ekliyoruz ki import yapabilelim
sys.path.append(os.path.join(os.path.dirname(__file__), 'MOD-03_Acoustics'))
sys.path.append(os.path.join(os.path.dirname(__file__), 'MOD-04_Web_STT'))

from acoustic_homing import AcousticHomingBridge, MotorDirection
from comms_dashboard import WebDashboard
from comms_dashboard_interface import AugmentedStatusReport
from stt_engine import STTEngine

def fsm_transition_callback(bearing: float):
    """MOD-03'ten gelen FSM durum değiştirme isteğini yakalar."""
    print(f"\n[FSM] *** DURUM DEĞİŞİKLİĞİ: EXPLORE -> ACOUSTIC_HOMING (Açı: {bearing:.1f}°) ***\n")

def simulate_robot_loop(dashboard: WebDashboard, acoustic_bridge: AcousticHomingBridge):
    """
    Sanal bir donanım (UART) simülasyonu.
    Robotun sağından (+45 derece) sürekli ses geldiğini varsayar.
    Robot sağa döndükçe bu açıyı 0'a doğru azaltırız.
    """
    
    current_bearing = 45.0 # Başlangıçta ses sağdan 45 derece açıyla geliyor
    is_sound_active = True
    
    while True:
        # 1. Sahte UART String'i oluştur (STM32'den geliyormuş gibi)
        a_hit = 1 if is_sound_active else 0
        uart_line = f"|Temp:25.3|Smoke:0|A_Hit:{a_hit}|A_Ang:{current_bearing:.1f}|\n"
        
        # 2. MOD-03 Akustik Köprüsü üzerinden UART verisini işle
        # Bu fonksiyon üst üste 3 hit alana kadar None döner.
        nav_command = acoustic_bridge.process_uart_line(uart_line)
        
        # 3. Eğer akustik köprü robota hareket komutu verirse:
        if nav_command:
            print(f"[MOTOR] MOD-03'ten komut geldi: Yön={nav_command.direction.name}, Hız={nav_command.speed}")
            
            # Robotun sese doğru döndüğünü simüle edelim:
            if nav_command.direction == MotorDirection.RIGHT:
                current_bearing -= 15.0  # Robot sağa döndükçe sesin açısı küçülür
                if current_bearing < 0: current_bearing = 0.0
            elif nav_command.direction == MotorDirection.LEFT:
                current_bearing += 15.0
                if current_bearing > 0: current_bearing = 0.0
            elif nav_command.direction == MotorDirection.FORWARD:
                # Açı 0'a yaklaştığında ileri komutu gelir
                print("[SİMÜLASYON] Robot ses kaynağına başarıyla hizalandı ve ilerliyor!")
                is_sound_active = False # Hedefe ulaştık, sesi kes
                acoustic_bridge.reset() # Köprüyü sıfırla
                current_bearing = 45.0 # Bir sonraki test için tekrar sağa al
                time.sleep(5) # 5 saniye bekle
                is_sound_active = True
                print("\n[SİMÜLASYON] Yeni bir ses duyuldu!")
                
        # 4. Elde ettiğimiz durumu Unity'ye (MOD-05) gönder
        report = AugmentedStatusReport(
            pos_x=0.0,  # Sabit tutuyoruz
            pos_y=0.0,
            temperature=25.3,
            smoke_detected=False,
            victim_status="NONE",
            is_stuck=False,
            priority_level=0,
            acoustic_hit=is_sound_active,
            acoustic_angle=current_bearing
        )
        dashboard.broadcast_telemetry(report)
        
        time.sleep(1) # Her 1 saniyede bir döngü çalışır

if __name__ == "__main__":
    print("=== ANA SİSTEM BAŞLATILIYOR (MOD-03 + MOD-04 + MOD-05) ===")
    
    # 1. Dashboard Başlat
    dashboard = WebDashboard()
    
    # 2. Akustik Köprüyü Başlat (Callback bağlayarak)
    acoustic_bridge = AcousticHomingBridge(fsm_transition_callback=fsm_transition_callback)
    
    # 3. Simülasyon Thread'ini Başlat
    sim_thread = threading.Thread(
        target=simulate_robot_loop, 
        args=(dashboard, acoustic_bridge), 
        daemon=True
    )
    sim_thread.start()
    
    # 4. Sunucuyu Ana Thread'de Bloklayarak Çalıştır
    try:
        dashboard.start_server(port=5001)
    except KeyboardInterrupt:
        print("\nSistem kapatılıyor...")
