import threading
import time
import serial
from typing import Dict, Any, Optional

from comms_dashboard import WebDashboard
from comms_dashboard_interface import AugmentedStatusReport

class STM32Bridge:
    """
    Bridge between the STM32 (Bluepill) and the Web Dashboard.
    Reads serial telemetry asynchronously and forwards operator commands to STM32.
    """
    
    def __init__(self, port: str = '/dev/ttyACM0', baudrate: int = 115200, dashboard: Optional[WebDashboard] = None, state_manager=None):
        self.port = port
        self.baudrate = baudrate
        self.dashboard = dashboard
        self.state_manager = state_manager
        self.serial_conn: Optional[serial.Serial] = None
        self._running = False
        
        # Override dashboard's callback if dashboard is provided
        if self.dashboard:
            self.dashboard.on_operator_command_received = self.custom_operator_callback

    def connect(self) -> bool:
        """Establishes connection to the STM32 serial port."""
        try:
            self.serial_conn = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"[STM32Bridge] Connected to STM32 on {self.port} at {self.baudrate} baud.")
            return True
        except Exception as e:
            print(f"[STM32Bridge] Serial connection error on {self.port}: {e}")
            self.serial_conn = None
            return False

    def start(self):
        """Starts the background thread to read from STM32."""
        if not self.serial_conn:
            print("[STM32Bridge] Cannot start read loop, serial not connected.")
            return
        
        self._running = True
        read_thread = threading.Thread(target=self._read_loop, daemon=True)
        read_thread.start()

    def stop(self):
        """Stops the background thread and closes serial connection."""
        self._running = False
        if self.serial_conn:
            self.serial_conn.close()
            print("[STM32Bridge] Serial connection closed.")

    def _read_loop(self):
        """Background loop reading from STM32 and broadcasting to Unity."""
        print("[STM32Bridge] Background read thread started.")
        while self._running:
            if not self.serial_conn or not self.serial_conn.is_open:
                print("[STM32Bridge] Serial disconnected. Attempting to reconnect...")
                if self.connect():
                    time.sleep(1)  # Give it a second to stabilize after connecting
                else:
                    time.sleep(2)  # Wait before retrying
                continue

            try:
                if self.serial_conn.in_waiting > 0:
                    # Expected format: Temp,Smoke,Victim,PosX,PosY
                    # Example: 25.5,1,NONE,12.0,8.5
                    line = self.serial_conn.readline().decode('utf-8').strip()
                    if not line:
                        continue
                        
                    data_parts = line.split(',')
                    if len(data_parts) >= 5:
                        if self.state_manager:
                            self.state_manager.update_from_stm32(
                                temp=float(data_parts[0]),
                                smoke=bool(int(data_parts[1])),
                                pos_x=float(data_parts[3]),
                                pos_y=float(data_parts[4])
                            )
                        else:
                            # Fallback to direct broadcast if state_manager is not provided
                            report = AugmentedStatusReport(
                                temperature=float(data_parts[0]),
                                smoke_detected=bool(int(data_parts[1])),
                                victim_status=data_parts[2],
                                pos_x=float(data_parts[3]),
                                pos_y=float(data_parts[4]),
                                is_stuck=False,
                                priority_level=1,
                                acoustic_hit=False,
                                acoustic_angle=0.0
                            )
                            if self.dashboard:
                                self.dashboard.broadcast_telemetry(report)
                    else:
                        print(f"[STM32Bridge] Malformed data received: {line}")
                        
            except serial.SerialException as se:
                print(f"[STM32Bridge] Serial connection lost: {se}")
                self.serial_conn.close()
                self.serial_conn = None
            except Exception as e:
                print(f"[STM32Bridge] Error reading/parsing data: {e}")
            
            time.sleep(0.05)  # Small sleep to prevent 100% CPU usage

    def custom_operator_callback(self, command_payload: Dict[str, Any]):
        """Callback triggered when an operator command is received from Unity."""
        print(f"[STM32Bridge] Emergency! Operator FSM override: {command_payload}")
        if self.serial_conn and self.serial_conn.is_open:
            try:
                # Extract intent/cmd/action from payload.
                intent = command_payload.get('cmd', command_payload.get('intent', command_payload.get('action', 'UNKNOWN')))
                
                # Send the command to the STM32 via Serial
                cmd_str = f"OVERRIDE:{intent}\n"
                self.serial_conn.write(cmd_str.encode('utf-8'))
                print(f"[STM32Bridge] Sent to STM32: {cmd_str.strip()}")
            except serial.SerialException as se:
                print(f"[STM32Bridge] Serial connection lost while writing: {se}")
                self.serial_conn.close()
                self.serial_conn = None
            except Exception as e:
                print(f"[STM32Bridge] Failed to write operator command to serial: {e}")
        else:
            print("[STM32Bridge] Could not send override command. STM32 is not connected.")
