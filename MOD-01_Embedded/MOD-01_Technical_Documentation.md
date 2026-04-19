# MOD-01 — Embedded & Hardware Control Module

## Module Overview

The **Embedded & Hardware Control Module (MOD-01)** serves as the "reflexive core" of the robot. It is responsible for low-level hardware abstraction, real-time motor control, environmental sensing, and safety failsafes. Built on the **STM32F103C8T6** (Blue Pill) microcontroller, this module ensures that high-level commands from the Raspberry Pi are translated into physical motion while simultaneously monitoring the robot's health and surroundings.

### Key Responsibilities
*   **4WD Motor Control:** Driving four DC motors via the L298N H-Bridge using high-frequency PWM.
*   **Stuck Detection:** Real-time IMU-based monitoring to detect and recover from mechanical obstructions.
*   **Environmental Sensing:** Polling DHT22 (Temp/Humidity) and MQ-2 (Smoke/Gas) sensors.
*   **Power Management:** Managing a dual-powerbank setup with a 12V decoy trigger and time-based operation limits.
*   **Telemetry Hub:** Aggregating all sensor data into a structured UART stream for the Raspberry Pi 5.

---

## Authors & Responsibility Matrix

| Name | Student ID | Primary Responsibility |
| :--- | :--- | :--- |
| **Ziya** | 210104004027 | Power Management, UART Protocol, System Integration |
| **Ömer** | 210104004814 | Motor Control Logic, Stuck Detection Algorithm, IMU Interfacing |
| **Gabil** | 230104004902 | Environmental Sensor Drivers (DHT22, MQ-2) |

---

## Hardware Specifications

### Core Components
*   **Microcontroller:** STM32F103C8T6 (72MHz ARM Cortex-M3, 64KB Flash, 20KB SRAM).
*   **Motor Driver:** L298N Dual H-Bridge (capable of driving 4 motors in pairs or independently).
*   **Chassis:** 4WD Robot Platform with integrated encoders (optional/future use).

### Sensors
*   **IMU:** MPU6050 (6-axis Accelerometer + Gyroscope) via I2C.
*   **Temperature & Humidity:** DHT22 Digital Sensor.
*   **Smoke/Gas:** MQ-2 Analog/Digital Sensor.
*   **Distance:** 4x HC-SR04 Ultrasonic Sensors (processed by MOD-01 and relayed via UART).

### Power Architecture
*   **Logic Power:** 5V provided by the primary Powerbank (shared with Raspberry Pi).
*   **Motor Power:** 12V provided via a USB-C PD Powerbank and a **12V Type-C Decoy Trigger**, controlled by the STM32 via a GPIO enable pin.

---

## Inter-Module Connections

The Embedded Module is the primary bridge between the physical world and the digital intelligence of the robot.

### 1. MOD-04 (Web Dashboard & STT)
*   **Communication:** UART (115200 Baud).
*   **Downlink:** MOD-04 sends motor direction (W,A,S,D), PWM speed, and auxiliary flags (Buzzer/Lights).
*   **Uplink:** MOD-01 sends a 10-Hz telemetry stream containing environmental data, stuck status, and IMU heading.

### 2. MOD-03 (Acoustics & Navigation)
*   **Logic Link:** MOD-03 uses MOD-01's telemetry to correlate acoustic hits with the robot's yaw angle.
*   **Command Path:** MOD-03 generates navigation vectors (Acoustic Homing) which are routed through MOD-04 down to MOD-01 for execution.

### 3. MOD-05 (Unity Digital Twin)
*   **Visualization:** All telemetry gathered by MOD-01 is eventually visualized in MOD-05's Top-Down Map and Status Dashboard.

---

## Technical Architecture

### UART Communication Protocol
The module uses a DMA-backed UART interface to ensure non-blocking communication at **115200 baud**.
*   **Buffer Size:** 128 bytes.
*   **Packet Structure:** Binary-packed structs for efficiency, terminated by `\n`.
*   **Safety:** If no command is received for >2000ms, the module enters a "Safe Stop" mode.

### Stuck Detection Logic
The module implements a proprietary algorithm to detect mechanical stalls:
1.  **Command Monitoring:** Checks if motors are currently set to a non-zero speed.
2.  **IMU Feedback:** Polls the MPU6050 for linear acceleration and angular rate.
3.  **Correlation:** If `Motors == ON` AND `IMU_Motion < Threshold` for **2 seconds**, the `is_stuck` flag is raised.
4.  **Recovery:** Triggers a `RECOVERY_REVERSE` sequence (1.5s backward movement) to clear the obstruction.

### Power Failsafe Logic
*   **12V Decoy Control:** The 12V motor rail is only enabled after successful initialization. It is automatically cut during emergency stops or critical failures.
*   **Time Limit:** A hardware timer tracks total operation time. If it exceeds **60 minutes**, the telemetry flag triggers a "Return to Home" (RTH) protocol on the Pi.

---

## Data Structures

### `uart_telemetry_t` (STM32 → Pi)
| Field | Type | Description |
| :--- | :--- | :--- |
| `temperature` | `float` | Current temperature in Celsius. |
| `smoke_detected` | `bool` | True if smoke level exceeds threshold. |
| `is_stuck` | `bool` | True if robot is currently stuck. |
| `acoustic_angle` | `float` | Reserved for acoustic bearing relay. |
| `imu_yaw_angle` | `float` | Current heading (0-360°). |
| `us_dist_front/back/left/right` | `uint8_t` | Ultrasonic distances in cm. |

### `uart_command_t` (Pi → STM32)
| Field | Type | Description |
| :--- | :--- | :--- |
| `direction` | `enum` | 0=STOP, 1=FWD, 2=BWD, 3=LFT, 4=RGT. |
| `speed_pwm` | `uint8_t` | Motor speed (0-255). |
| `buzzer_on` | `bool` | Toggle wakeup buzzer. |
| `lights_on` | `bool` | Toggle SOS/Flashlight. |

---

## API Summary

### Power Management (`pwr_management.h`)
| Function | Parameters | Return | Description |
| :--- | :--- | :--- | :--- |
| `pwr_init` | `void` | `pwr_status_t` | Initializes GPIOs and decoy controls. |
| `pwr_set_decoy_state` | `bool enable` | `void` | Enables/disables the 12V motor rail. |
| `pwr_is_time_limit_exceeded` | `void` | `bool` | Checks if 60-min limit is reached. |

### UART Communication (`uart_comm.h`)
| Function | Parameters | Return | Description |
| :--- | :--- | :--- | :--- |
| `uart_comm_init` | `void` | `int8_t` | Sets up UART DMA and buffers. |
| `uart_send_telemetry` | `const uart_telemetry_t*` | `void` | Transmits sensor data to Pi. |
| `uart_receive_command` | `uart_command_t*` | `bool` | Parses incoming control packets. |

### Motor Control (`motor_control.h`)
| Function | Parameters | Return | Description |
| :--- | :--- | :--- | :--- |
| `motor_control_init` | `void` | `motor_status_t` | Configures TIM PWM channels. |
| `motor_set_state` | `uint8_t dir, uint8_t spd` | `motor_status_t` | Sets global robot movement state. |
| `motor_emergency_stop` | `void` | `void` | Immediate halt of all motors. |
| `motor_set_trim` | `int8_t L, int8_t R` | `void` | Calibrates motor speed differences. |

### Stuck Detection (`stuck_detection.h`)
| Function | Parameters | Return | Description |
| :--- | :--- | :--- | :--- |
| `stuck_detection_init` | `void` | `stuck_status_t` | Initializes MPU6050 and I2C. |
| `stuck_check` | `bool active, result*` | `stuck_status_t` | Main algorithm comparison call. |
| `imu_get_yaw_angle` | `void` | `float` | Returns current heading estimate. |
| `stuck_execute_recovery` | `action_t` | `void` | Performs the recovery movement. |

### Environment Sensors (`environment_sensors.h`)
| Function | Parameters | Return | Description |
| :--- | :--- | :--- | :--- |
| `env_sensors_init` | `void` | `int` | Prepares DHT22 and MQ-2 GPIOs. |
| `env_read_all` | `env_data_t*` | `int` | Synchronous read of all sensors. |
| `env_set_smoke_threshold`| `uint16_t` | `void` | Adjusts MQ-2 sensitivity. |

---

## Integration Example

```c
#include "uart_comm.h"
#include "motor_control.h"
#include "stuck_detection.h"

int main(void) {
    // 1. Hardware Init
    pwr_init();
    uart_comm_init();
    motor_control_init();
    stuck_detection_init();
    
    uart_telemetry_t telemetry;
    uart_command_t cmd;
    stuck_detection_result_t stuck_res;
    
    while (1) {
        // 2. Control Loop
        if (uart_receive_command(&cmd)) {
            motor_set_state(cmd.direction, cmd.speed_pwm);
        }
        
        // 3. Safety Check
        stuck_check(cmd.direction != 0, &stuck_res);
        if (stuck_res.is_stuck) {
            stuck_execute_recovery(RECOVERY_REVERSE);
        }
        
        // 4. Telemetry Update
        telemetry.is_stuck = stuck_res.is_stuck;
        telemetry.imu_yaw_angle = imu_get_yaw_angle();
        uart_send_telemetry(&telemetry);
        
        HAL_Delay(50); // 20Hz Loop
    }
}
```

---

## Known Risks & Mitigations

*   **Motor Drift:** Mechanical differences in motors cause the robot to veer.
    *   **Mitigation:** Use `motor_set_trim()` to balance PWM signals.
*   **IMU Gyro Drift:** Yaw angle accumulates error over time.
    *   **Mitigation:** Periodic zero-point calibration and future sensor fusion.
*   **UART Packet Loss:** Electrical noise from motors can corrupt serial data.
    *   **Mitigation:** Twisted-pair wiring and software-side checksums (Planned).

---

## Version History

*   **v1.0 (2026-04-19)** — Final comprehensive technical documentation created for Module Submission.
*   **v0.4 (2026-03-29)** — Full synchronization with all headers; added environment_sensors.h.
*   **v0.3 (2026-03-29)** — Merged Power/UART interfaces with Motor/Stuck Logic.
*   **v0.1 (2026-03-18)** — Initial architecture drafts.
