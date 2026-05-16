import os
import sys
import threading
import time

REPO_ROOT = os.path.dirname(__file__)
sys.path.append(os.path.join(REPO_ROOT, "MOD-02_Vision"))
sys.path.append(os.path.join(REPO_ROOT, "MOD-03_Acoustics"))
sys.path.append(os.path.join(REPO_ROOT, "MOD-04_Web_STT"))

from ai_vision import VisionPipeline
from acoustic_homing import AcousticHomingBridge, MotorDirection
from comms_dashboard import WebDashboard


def fsm_transition_callback(bearing: float):
    """Capture the MOD-03 request to transition into acoustic homing."""
    print(f"\n[FSM] EXPLORE -> ACOUSTIC_HOMING requested (bearing={bearing:.1f} deg)\n")


def simulate_robot_loop(
    dashboard: WebDashboard,
    acoustic_bridge: AcousticHomingBridge,
    vision: VisionPipeline,
):
    """
    End-to-end integration loop for MOD-02 + MOD-03 + MOD-04 + MOD-05.

    Telemetry source:
    - MOD-02 provides victim_status and priority_level
    - MOD-03 provides acoustic_hit and acoustic_angle
    - MOD-04 serializes and broadcasts the merged payload
    - MOD-05 consumes the resulting telemetry JSON
    """

    current_bearing = 45.0
    is_sound_active = True

    while True:
        a_hit = 1 if is_sound_active else 0
        uart_line = f"|Temp:25.3|Smoke:0|A_Hit:{a_hit}|A_Ang:{current_bearing:.1f}|\n"
        nav_command = acoustic_bridge.process_uart_line(uart_line)

        if nav_command:
            print(f"[MOTOR] MOD-03 command: dir={nav_command.direction.name}, speed={nav_command.speed}")

            if nav_command.direction == MotorDirection.RIGHT:
                current_bearing -= 15.0
                if current_bearing < 0:
                    current_bearing = 0.0
            elif nav_command.direction == MotorDirection.LEFT:
                current_bearing += 15.0
                if current_bearing > 0:
                    current_bearing = 0.0
            elif nav_command.direction == MotorDirection.FORWARD:
                print("[SIM] Robot aligned with sound source and moving forward.")
                is_sound_active = False
                acoustic_bridge.reset()
                current_bearing = 45.0
                time.sleep(5)
                is_sound_active = True
                print("\n[SIM] New acoustic event detected.")

        report = vision.build_augmented_status_report(
            pos_x=0.0,
            pos_y=0.0,
            temperature=25.3,
            smoke_detected=False,
            is_stuck=False,
            acoustic_hit=is_sound_active,
            acoustic_angle=current_bearing,
        )
        dashboard.broadcast_telemetry(report)

        latest_frame = vision.get_latest_frame_jpeg()
        if latest_frame:
            dashboard.stream_video_frame(latest_frame)

        latest_target = vision.get_latest_target()
        if latest_target is not None:
            print(
                "[VISION] "
                f"severity={latest_target.severity} "
                f"confidence={latest_target.confidence:.2f} "
                f"distance_cm={latest_target.distance_cm:.1f}"
            )

        time.sleep(1)


if __name__ == "__main__":
    print("=== MAIN SYSTEM STARTING (MOD-02 + MOD-03 + MOD-04 + MOD-05) ===")

    dashboard = WebDashboard()
    acoustic_bridge = AcousticHomingBridge(fsm_transition_callback=fsm_transition_callback)
    vision = VisionPipeline()

    if not vision.initialize_camera():
        raise RuntimeError("MOD-02 Vision could not be initialized.")

    sim_thread = threading.Thread(
        target=simulate_robot_loop,
        args=(dashboard, acoustic_bridge, vision),
        daemon=True,
    )
    sim_thread.start()

    try:
        dashboard.start_server(port=5001)
    except KeyboardInterrupt:
        print("\nSystem shutting down...")
    finally:
        vision.shutdown()
