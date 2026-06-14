# MOD-01 Embedded & Hardware Module

**Purpose:** This module handles real-time hardware reflexes via the STM32 microcontroller. It drives the 4WD DC motors, polls environmental sensors, manages the Dual Powerbank setup (12V Decoy failsafes), implements IMU-based stuck detection, and ensures robust UART telemetry/command communication with the Raspberry Pi.

**Authors:**
* Ziya 210104004027 (Primary - Power & UART)
* Ömer 210104004814 (Secondary - Motors & Stuck Detection)
* Gabil 230104004902 (Secondary - Env. Sensors)

**Dependencies:** * STM32 HAL Library (`stm32f1xx_hal.h`, `_gpio.h`, `_tim.h`, `_i2c.h`)
* Standard C libraries (`<stdint.h>`, `<stdbool.h>`)

### Quick-Start Integration Example

```c
#include "uart_comm.h"
#include "motor_control.h"
#include "stuck_detection.h"
#include "pwr_management.h"
#include "environment_sensors.h"

int main(void) {
    // 1. Initialize all subsystems
    pwr_init();
    uart_comm_init();
    motor_control_init();
    stuck_detection_init();
    env_sensors_init();
    
    // 2. Enable 12V decoy for motors
    pwr_set_decoy_state(true);
    
    uart_telemetry_t telemetry;
    uart_command_t cmd;
    stuck_detection_result_t stuck_result;
    environment_data_t env_data;
    
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
        
        // 5. Read Environment Sensors
        env_read_all(&env_data);
        
        // 6. Populate Telemetry & Send
        telemetry.temperature = env_data.temperature_c;
        telemetry.smoke_detected = env_data.smoke_alert;
        telemetry.is_stuck = stuck_result.is_stuck;
        telemetry.imu_yaw_angle = imu_get_yaw_angle();
        uart_send_telemetry(&telemetry);
        
        // 7. Time-Based Power Failsafe
        if (pwr_is_time_limit_exceeded()) {
            // Trigger RTH protocol
        }
    }
}
API Summary
Power Management (pwr_management.h)

pwr_status_t pwr_init(void)

Description: Initializes power GPIO pins and sets up decoy states.

void pwr_set_decoy_state(bool enable)

Description: Enables/disables 12V output for motor driver.

bool pwr_is_time_limit_exceeded(void)

Description: Failsafe check for the 60-min RTH limit.

UART Communication (uart_comm.h)

int8_t uart_comm_init(void)

Description: Initializes UART peripheral and DMA buffers.

void uart_send_telemetry(const uart_telemetry_t* data)

Description: Sends packed telemetry struct to Pi 5.

bool uart_receive_command(uart_command_t* out_cmd)

Description: Parses incoming motor/light commands from Pi 5.

Motor Control (motor_control.h)

motor_status_t motor_control_init(void)

Description: Initializes L298N PWM timers and direction GPIOs.

motor_status_t motor_set_state(uint8_t direction, uint8_t speed_pwm)

Description: Sets the robot's movement state based on UART command.

motor_status_t motor_set_single(motor_id_t motor, motor_direction_t dir, uint8_t speed)

Description: Controls a single motor's direction and speed.

void motor_emergency_stop(void)

Description: Emergency stop - immediately halts all motors.

void motor_soft_stop(uint16_t decel_time_ms)

Description: Soft stop - gradually reduces speed to zero.

uint8_t motor_get_current_speed(motor_id_t motor)

Description: Gets the current PWM duty cycle of a specific motor.

void motor_set_trim(int8_t left_trim, int8_t right_trim)

Description: Calibrates motor speeds to compensate for mechanical differences.

void motor_run_test_sequence(uint8_t test_speed, uint16_t duration_ms)

Description: Runs all motors in sequence for hardware verification.

Stuck Detection (stuck_detection.h)

stuck_status_t stuck_detection_init(void)

Description: Initializes the MPU6050 IMU sensor and stuck detection module.

stuck_status_t imu_read_raw(imu_raw_data_t *data)

Description: Reads raw sensor data from the MPU6050.

void imu_process_data(const imu_raw_data_t *raw, imu_processed_data_t *processed)

Description: Converts raw IMU data to physical units.

stuck_status_t stuck_check(bool motors_active, stuck_detection_result_t *result)

Description: Main stuck detection function - compares motor state with IMU motion.

stuck_recovery_action_t stuck_get_recovery_action(const stuck_detection_result_t *result)

Description: Determines the recommended recovery action.

void stuck_execute_recovery(stuck_recovery_action_t action)

Description: Executes automatic recovery when stuck is detected.

stuck_status_t imu_calibrate(uint16_t sample_count)

Description: Calibrates the IMU sensors by computing zero-offset values.

void stuck_reset(void)

Description: Resets the stuck detection state machine.

float imu_get_yaw_angle(void)

Description: Gets the current IMU yaw angle (heading) in degrees.

bool imu_is_connected(void)

Description: Checks if the MPU6050 is responding on the I2C bus.

float stuck_compute_movement_magnitude(const imu_processed_data_t *data)

Description: Computes the magnitude of movement (combined accel + gyro).

Environment Sensors (environment_sensors.h)

int env_sensors_init(void)

Description: Initialize all environmental sensors used by the module.

int env_read_dht22(float *temperature_c_out, float *humidity_pct_out)

Description: Read DHT22 temperature and humidity values.

int env_read_mq2(uint16_t *smoke_raw_out)

Description: Read raw MQ-2 smoke/gas sensor value.

int env_read_all(environment_data_t *out_data)

Description: Read all environment sensors and fill a combined data structure.

void env_set_smoke_threshold(uint16_t threshold)

Description: Set MQ-2 smoke alert threshold.

bool env_is_smoke_alert(uint16_t smoke_raw)

Description: Check whether the given MQ-2 raw value exceeds current smoke threshold.

Known Limitations and Future Work
Mechanical Tolerances: Some motors may have different speeds. A motor_set_trim() software calibration function will be needed to ensure straight-line movement.

Terrain Variables: Bumpy terrain may cause false stuck detections. Thresholds (STUCK_ACCEL_THRESHOLD_MG) need empirical tuning in the arena.

Gyro Drift: Yaw angle drifts over time. A complementary filter using accelerometer tilt correction is planned for phase 2.

Payload Limit: UART DMA buffer size is strictly limited to 128 bytes.

Environmental Integration: Evaluate whether environmental sensor readings should contribute to higher-level decision-making or Return-to-Home / alert policies.

Hardware Testing Checklist
[ ] L298N: PWM speed control works across duty cycles (64, 128, 192, 255).

[ ] MPU6050: I2C connection verified (WHO_AM_I = 0x68).

[ ] Failsafe: Stuck flag triggers appropriately when motors run but the chassis is blocked.

[ ] Failsafe: Decoy power cuts off gracefully during emergency stops.

Version History
v0.4 (2026-03-29): Full synchronization with all headers; added environment_sensors.h and completed missing motor_control / stuck_detection API summaries.

v0.3 (2026-03-29): Merged Ziya's Power/UART interfaces with Ömer's Motor/Stuck Logic.

v0.2 (2026-03-27): Updated telemetry payload to include mapping variables (Yaw, US distances).

v0.1 (2026-03-18): Initial drafts created independently.