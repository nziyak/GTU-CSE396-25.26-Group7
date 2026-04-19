# MOD-01 Embedded & Hardware Module - Ömer's Contributions

**Author:** Ömer [Student ID]  
**Role:** Secondary (Motors & Stuck Detection)  
**Date:** 2026-03-29  
**Version:** 0.1

---

## Overview

This section documents Ömer's contributions to Module 1, specifically:
1. **`motor_control.h`** - L298N motor driver control interface
2. **`stuck_detection.h`** - MPU6050 IMU-based stuck detection logic

These headers integrate with Ziya's UART communication layer (`uart_comm.h`) to provide complete motor control and failsafe detection for the search & rescue robot.

---

## File 1: motor_control.h

### Purpose
Controls the L298N H-Bridge motor driver to manage the 4WD chassis. Receives direction commands from UART and translates them into PWM signals for DC motors.

### Key Features
- **Direction Control:** Forward, Backward, Left, Right, Stop
- **PWM Speed Control:** 0-255 duty cycle
- **Individual Motor Control:** Control each of the 4 motors independently
- **Safety Features:** Emergency stop, soft stop, trim calibration
- **Integration:** Maps directly to `uart_direction_t` from `uart_comm.h`

### Hardware Interface
| Component | Connection |
|-----------|------------|
| L298N IN1-IN4 | STM32 GPIO (direction control) |
| L298N ENA, ENB | STM32 PWM Timers (TIM2, TIM3) |
| Power Supply | 12V from PD Powerbank with Type-C Decoy |
| Motors | 4x DC Motors (4WD Chassis) |

### Main Functions

```c
// Initialize motor control system
motor_status_t motor_control_init(void);

// Set robot movement based on UART command (main function)
motor_status_t motor_set_state(uint8_t direction, uint8_t speed_pwm);

// Emergency stop all motors
void motor_emergency_stop(void);

// Control individual motor
motor_status_t motor_set_single(motor_id_t motor, motor_direction_t dir, uint8_t speed);
```

### Integration with UART
The `motor_set_state()` function is called by the UART receive handler when a command is received from the Raspberry Pi:

```c
// In main.c or uart handler:
uart_command_t cmd;
if (uart_receive_command(&cmd)) {
    motor_set_state(cmd.direction, cmd.speed_pwm);
}
```

---

## File 2: stuck_detection.h

### Purpose
Detects when the robot is stuck by comparing commanded motor state with actual IMU motion data. Triggers recovery protocols when motors are running but no movement is detected.

### Detection Logic
**Stuck Condition:**
```
(Motors Active) AND (No IMU Movement for 2+ seconds) = STUCK
```

**Movement Detection:**
- Monitors accelerometer for linear motion
- Monitors gyroscope for rotation
- Combines both measurements into movement magnitude
- Compares against thresholds: 50mg acceleration, 5°/s rotation

### Hardware Interface
| Component | Connection |
|-----------|------------|
| MPU6050 IMU | STM32 I2C1 (address 0x68) |
| SDA/SCL | I2C1 pins on STM32 |
| Power | 3.3V from STM32 |

### Main Functions

```c
// Initialize IMU and stuck detection
stuck_status_t stuck_detection_init(void);

// Main stuck detection function (call every 20-50ms)
stuck_status_t stuck_check(bool motors_active, stuck_detection_result_t *result);

// Get recovery action recommendation
stuck_recovery_action_t stuck_get_recovery_action(const stuck_detection_result_t *result);

// Execute automatic recovery
void stuck_execute_recovery(stuck_recovery_action_t action);

// Get current yaw angle for navigation
float imu_get_yaw_angle(void);
```

### Integration with UART Telemetry

The stuck detection module updates the `is_stuck` flag in `uart_telemetry_t`:

```c
// In main.c loop:
stuck_detection_result_t stuck_result;
bool motors_active = (current_direction != UART_DIR_STOP);

// Check if stuck
stuck_check(motors_active, &stuck_result);

// Update telemetry
uart_telemetry_t telemetry;
telemetry.is_stuck = stuck_result.is_stuck;
telemetry.imu_yaw_angle = imu_get_yaw_angle();

// Send to Raspberry Pi
uart_send_telemetry(&telemetry);
```

### Recovery Actions

| Action | Description | Trigger Condition |
|--------|-------------|-------------------|
| `RECOVERY_NONE` | No action needed | Robot is moving normally |
| `RECOVERY_REVERSE` | Reverse for 1.5s | Stuck for 2-3 seconds |
| `RECOVERY_ROTATE` | Rotate in place | Stuck after reverse failed |
| `RECOVERY_EMERGENCY` | Stop & alert | Stuck >10 seconds (critical) |

---

## Integration Example

### Complete Main Loop Integration

```c
#include "uart_comm.h"
#include "motor_control.h"
#include "stuck_detection.h"
#include "pwr_management.h"

int main(void) {
    // Initialize all subsystems
    HAL_Init();
    SystemClock_Config();
    
    pwr_init();
    uart_comm_init();
    motor_control_init();
    stuck_detection_init();
    
    // Enable 12V for motors
    pwr_set_decoy_state(true);
    
    uart_telemetry_t telemetry;
    uart_command_t cmd;
    stuck_detection_result_t stuck_result;
    
    while (1) {
        // 1. Check for new UART commands from Pi
        if (uart_receive_command(&cmd)) {
            motor_set_state(cmd.direction, cmd.speed_pwm);
            
            // Handle buzzer and lights
            if (cmd.buzzer_on) {
                // Activate buzzer
            }
            if (cmd.lights_on) {
                // Activate SOS lights
            }
        }
        
        // 2. Check if motors are currently active
        bool motors_active = (cmd.direction != UART_DIR_STOP);
        
        // 3. Run stuck detection
        stuck_check(motors_active, &stuck_result);
        
        // 4. Execute recovery if stuck
        if (stuck_result.is_stuck) {
            stuck_recovery_action_t action = stuck_get_recovery_action(&stuck_result);
            stuck_execute_recovery(action);
        }
        
        // 5. Read sensors and populate telemetry
        telemetry.temperature = /* Read DHT22 */;
        telemetry.smoke_detected = /* Read MQ-2 */;
        telemetry.is_stuck = stuck_result.is_stuck;
        telemetry.acoustic_angle = /* Read from acoustics module */;
        telemetry.imu_yaw_angle = imu_get_yaw_angle();
        telemetry.us_dist_front = /* Read HC-SR04 front */;
        telemetry.us_dist_back = /* Read HC-SR04 back */;
        telemetry.us_dist_left = /* Read HC-SR04 left */;
        telemetry.us_dist_right = /* Read HC-SR04 right */;
        
        // 6. Send telemetry to Pi
        uart_send_telemetry(&telemetry);
        
        // 7. Check failsafe conditions
        if (pwr_is_time_limit_exceeded()) {
            // Trigger RTH protocol
        }
        
        HAL_Delay(20);  // 50Hz main loop
    }
}
```

---

## Dependencies

### Ziya's Files (Module 1 Primary)
- `uart_comm.h` - UART communication protocol
- `pwr_management.h` - Power management and failsafe

### STM32 HAL Libraries
- `stm32f1xx_hal_gpio.h` - GPIO control
- `stm32f1xx_hal_tim.h` - PWM timers
- `stm32f1xx_hal_i2c.h` - I2C for MPU6050
- `stm32f1xx_hal.h` - System initialization

### External Hardware
- L298N Dual H-Bridge Motor Driver
- MPU6050 6-Axis IMU
- 4x DC Motors (4WD Chassis)
- 12V PD Powerbank with Type-C Decoy

---

## Testing Checklist

### Motor Control Tests
- [ ] All 4 motors spin correctly in FORWARD direction
- [ ] All 4 motors spin correctly in BACKWARD direction
- [ ] LEFT turn rotates robot counter-clockwise
- [ ] RIGHT turn rotates robot clockwise
- [ ] STOP command halts all motors immediately
- [ ] PWM speed control works (test at 64, 128, 192, 255)
- [ ] Emergency stop function works
- [ ] Trim calibration ensures straight-line movement

### Stuck Detection Tests
- [ ] MPU6050 I2C connection verified (WHO_AM_I = 0x68)
- [ ] IMU calibration completes successfully
- [ ] Stuck flag triggers when motors run but robot is blocked
- [ ] Stuck flag clears when robot moves freely
- [ ] Yaw angle updates correctly during rotation
- [ ] Recovery actions execute when stuck
- [ ] Telemetry includes correct is_stuck flag
- [ ] Telemetry includes correct imu_yaw_angle

---

## Known Issues & Future Work

### Motor Control
- **Issue:** Some motors may have different speeds due to mechanical tolerances
  - **Solution:** Use `motor_set_trim()` to calibrate left/right balance

### Stuck Detection
- **Issue:** Bumpy terrain may cause false stuck detections
  - **Solution:** Increase `STUCK_ACCEL_THRESHOLD_MG` or add terrain-adaptive thresholds
  
- **Issue:** Yaw angle drifts over time (gyro integration error)
  - **Future:** Implement complementary filter with accelerometer-based tilt correction

---

## Version History

- **v0.1 (2026-03-29):** Initial header files created
  - `motor_control.h`: L298N driver interface defined
  - `stuck_detection.h`: MPU6050 stuck detection logic defined

---

## Contact

For questions about motor control or stuck detection:
- **Ömer** - Module 1 Secondary (Motors & Stuck Detection)
- **Ziya** - Module 1 Primary (Power & UART) - Contact for integration issues
