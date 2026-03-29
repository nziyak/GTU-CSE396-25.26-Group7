# MOD-01 Embedded & Hardware Module

**Purpose:** This module handles real-time hardware reflexes via the STM32 microcontroller. It drives the 4WD DC motors, polls environmental sensors, manages the Dual Powerbank setup (12V Decoy failsafes), implements IMU-based stuck detection, and ensures robust UART telemetry/command communication with the Raspberry Pi.

**Authors:**
* Ziya 210104004027 (Primary - Power & UART)
* Ömer 210104004814 (Secondary - Motors & Stuck Detection)
* Gabil [Öğrenci Nonu Yaz] (Secondary - Env. Sensors)

**Dependencies:** * STM32 HAL Library (`stm32f1xx_hal.h`, `_gpio.h`, `_tim.h`, `_i2c.h`)
* Standard C libraries (`<stdint.h>`, `<stdbool.h>`)

### Quick-Start Integration Example

```c
#include "uart_comm.h"
#include "motor_control.h"
#include "stuck_detection.h"
#include "pwr_management.h"

int main(void) {
    // 1. Initialize all subsystems
    pwr_init();
    uart_comm_init();
    motor_control_init();
    stuck_detection_init();
    
    // 2. Enable 12V decoy for motors
    pwr_set_decoy_state(true);
    
    uart_telemetry_t telemetry;
    uart_command_t cmd;
    stuck_detection_result_t stuck_result;
    
    while (1) {
        // 3. Receive Commands & Drive Motors
        if (uart_receive_command(&cmd)) {
            motor_set_state(cmd.direction, cmd.speed_pwm);
        }
        
        // 4. Stuck Detection & Recovery Failsafe
        bool motors_active = (cmd.direction != UART_DIR_STOP);
        stuck_check(motors_active, &stuck_result);
        
        if (stuck_result.is_stuck) {
            stuck_recovery_action_t action = stuck_get_recovery_action(&stuck_result);
            stuck_execute_recovery(action);
        }
        
        // 5. Populate Telemetry & Send
        telemetry.is_stuck = stuck_result.is_stuck;
        telemetry.imu_yaw_angle = imu_get_yaw_angle();
        uart_send_telemetry(&telemetry);
        
        // 6. Time-Based Power Failsafe
        if (pwr_is_time_limit_exceeded()) {
            // Trigger RTH protocol
        }
    }
}
```

API Summary
pwr_status_t pwr_init(void)

Description: Initializes power GPIO pins and sets up decoy states.

void pwr_set_decoy_state(bool enable)

Description: Enables/disables 12V output for motor driver.

bool pwr_is_time_limit_exceeded(void)

Description: Failsafe check for the 60-min RTH limit.

int8_t uart_comm_init(void)

Description: Initializes UART peripheral and DMA buffers.

void uart_send_telemetry(const uart_telemetry_t* data)

Description: Sends packed telemetry struct to Pi 5.

bool uart_receive_command(uart_command_t* out_cmd)

Description: Parses incoming motor/light commands from Pi 5.

motor_status_t motor_control_init(void)

Description: Initializes L298N PWM timers and direction GPIOs.

motor_status_t motor_set_state(uint8_t direction, uint8_t speed_pwm)

Description: Main driver function to move the chassis.

stuck_status_t stuck_detection_init(void)

Description: Initializes I2C communication with MPU6050.

stuck_status_t stuck_check(bool motors_active, stuck_detection_result_t *result)

Description: Evaluates accelerometer/gyro data against motor states.

float imu_get_yaw_angle(void)

Description: Returns the integrated Z-axis rotation for dead-reckoning mapping.

TODO: [Gabil]

Description: Sensor_ReadDHT22, Sensor_ReadMQ2 functions to be added.

Known Limitations and Future Work
Mechanical Tolerances: Some motors may have different speeds. A motor_set_trim() software calibration function will be needed to ensure straight-line movement.

Terrain Variables: Bumpy terrain may cause false stuck detections. Thresholds (STUCK_ACCEL_THRESHOLD_MG) need empirical tuning in the arena.

Gyro Drift: Yaw angle drifts over time. A complementary filter using accelerometer tilt correction is planned for phase 2.

Payload Limit: UART DMA buffer size is strictly limited to 128 bytes.

Hardware Testing Checklist
[ ] L298N: PWM speed control works across duty cycles (64, 128, 192, 255).

[ ] MPU6050: I2C connection verified (WHO_AM_I = 0x68).

[ ] Failsafe: Stuck flag triggers appropriately when motors run but the chassis is blocked.

[ ] Failsafe: Decoy power cuts off gracefully during emergency stops.

Version History
v0.3 (2026-03-29): Merged Ziya's Power/UART interfaces with Ömer's Motor/Stuck Logic.

v0.2 (2026-03-27): Updated telemetry payload to include mapping variables (Yaw, US distances).

v0.1 (2026-03-18): Initial drafts created independently.