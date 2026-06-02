import os
import threading
import time
from comms_dashboard import WebDashboard
from comms_dashboard_interface import AugmentedStatusReport
from stt_engine import STTEngine

def simulate_telemetry(dashboard: WebDashboard):
    """Background thread that broadcasts mock telemetry data every second."""
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
        print(f"[Simulation] Telemetry broadcasted: X={x}")
        x += 0.5
        time.sleep(2)

if __name__ == "__main__":
    print("=== MOD-04 Test System Starting ===")
    
    # 1. Initialize the STT Engine
    stt = STTEngine()
    model_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "vosk-model")
    if stt.load_offline_model(model_path):
        print("[STT] Engine ready!")
    else:
        print("[STT] Warning: Model could not be loaded. Running dashboard only.")
    
    # 2. Initialize the Web Dashboard
    dashboard = WebDashboard(stt_engine=stt)
    
    # Start telemetry simulation in the background
    sim_thread = threading.Thread(target=simulate_telemetry, args=(dashboard,), daemon=True)
    sim_thread.start()
    
    # Start the server blocking the main thread
    try:
        dashboard.start_server(port=5001)
    except KeyboardInterrupt:
        print("\nSystem shutting down...")
