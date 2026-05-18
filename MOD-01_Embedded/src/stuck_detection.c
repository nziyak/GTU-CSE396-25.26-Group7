/**
 * @file stuck_detection.c
 * @brief Stuck Detection and Recovery Implementation
 */

#include "stuck_detection.h"
#include <math.h>
#include <string.h>

/* MPU6050 Register Addresses */
#define MPU6050_REG_PWR_MGMT_1      0x6B
#define MPU6050_REG_ACCEL_XOUT_H    0x3B
#define MPU6050_REG_GYRO_XOUT_H     0x43
#define MPU6050_REG_WHO_AM_I        0x75

/* Internal helper functions */
static HAL_StatusTypeDef StuckDetection_UpdateIMU(StuckDetection_t *detector);
static bool StuckDetection_CheckMotionThreshold(const IMU_Data_t *imu);
static HAL_StatusTypeDef StuckDetection_ExecuteRecovery(StuckDetection_t *detector);
static void StuckDetection_AdvanceRecoveryStrategy(StuckDetection_t *detector);

/**
 * @brief Initialize stuck detection system
 */
HAL_StatusTypeDef StuckDetection_Init(StuckDetection_t *detector,
                                      I2C_HandleTypeDef *i2c,
                                      MotorControl_t *motor)
{
    if (!detector || !i2c || !motor) {
        return HAL_ERROR;
    }
    
    // Clear structure
    memset(detector, 0, sizeof(StuckDetection_t));
    
    // Store handles
    detector->i2c = i2c;
    detector->motor = motor;
    
    // Initialize MPU6050
    if (MPU6050_Init(i2c) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // Set initial state
    detector->state = STUCK_STATE_MONITORING;
    detector->current_strategy = RECOVERY_NONE;
    
    return HAL_OK;
}

/**
 * @brief Main stuck detection task
 */
HAL_StatusTypeDef StuckDetection_Task(StuckDetection_t *detector)
{
    if (!detector) {
        return HAL_ERROR;
    }
    
    uint32_t current_time = HAL_GetTick();
    
    // Update IMU readings
    if (StuckDetection_UpdateIMU(detector) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // State machine
    switch (detector->state) {
        case STUCK_STATE_MONITORING:
            // Check if motors are active
            if (Motor_IsActive(detector->motor)) {
                // Motors running - check for motion
                if (!StuckDetection_CheckMotionThreshold(&detector->imu_data)) {
                    // Low motion detected - start timer
                    detector->stuck_timer_start = current_time;
                    detector->state = STUCK_STATE_POSSIBLE;
                }
            }
            break;
            
        case STUCK_STATE_POSSIBLE:
            // Timer running - check if still stuck
            if (!Motor_IsActive(detector->motor)) {
                // Motors stopped - cancel stuck detection
                detector->state = STUCK_STATE_MONITORING;
            }
            else if (StuckDetection_CheckMotionThreshold(&detector->imu_data)) {
                // Motion detected - false alarm
                detector->state = STUCK_STATE_MONITORING;
            }
            else if ((current_time - detector->stuck_timer_start) >= STUCK_TIMEOUT_MS) {
                // Stuck timeout exceeded - confirm stuck
                detector->state = STUCK_STATE_CONFIRMED;
                detector->total_stuck_events++;
                
                // Store original command
                detector->original_command = Motor_GetCurrentCommand(detector->motor);
                
                // Start recovery
                detector->current_strategy = RECOVERY_REVERSE;
                detector->recovery_attempt_count = 0;
                detector->recovery_timer_start = current_time;
                detector->state = STUCK_STATE_RECOVERING;
                
                StuckDetection_ExecuteRecovery(detector);
            }
            break;
            
        case STUCK_STATE_RECOVERING:
            // Execute recovery sequence
            if (StuckDetection_ExecuteRecovery(detector) == HAL_OK) {
                // Recovery complete - verify motion
                if (StuckDetection_CheckMotionThreshold(&detector->imu_data)) {
                    // Success! Resume original command
                    Motor_ExecuteCommand(detector->motor, detector->original_command);
                    detector->successful_recoveries++;
                    detector->state = STUCK_STATE_MONITORING;
                    detector->current_strategy = RECOVERY_NONE;
                }
                else {
                    // Still stuck - try next strategy
                    StuckDetection_AdvanceRecoveryStrategy(detector);
                    
                    if (detector->current_strategy == RECOVERY_FAILED) {
                        // All strategies failed - stop motors
                        Motor_EmergencyStop(detector->motor);
                        detector->state = STUCK_STATE_MONITORING;
                    }
                }
            }
            break;
            
        default:
            detector->state = STUCK_STATE_MONITORING;
            break;
    }
    
    return HAL_OK;
}

/**
 * @brief Check if recovering
 */
bool StuckDetection_IsRecovering(const StuckDetection_t *detector)
{
    return (detector && detector->state == STUCK_STATE_RECOVERING);
}

/**
 * @brief Get current state
 */
StuckState_t StuckDetection_GetState(const StuckDetection_t *detector)
{
    return detector ? detector->state : STUCK_STATE_MONITORING;
}

/**
 * @brief Reset stuck detection
 */
void StuckDetection_Reset(StuckDetection_t *detector)
{
    if (!detector) {
        return;
    }
    
    detector->state = STUCK_STATE_MONITORING;
    detector->current_strategy = RECOVERY_NONE;
    detector->stuck_timer_start = 0;
    detector->recovery_timer_start = 0;
}

/**
 * @brief Get statistics
 */
void StuckDetection_GetStats(const StuckDetection_t *detector,
                            uint32_t *total_events,
                            uint32_t *successful_recoveries)
{
    if (!detector) {
        return;
    }
    
    if (total_events) {
        *total_events = detector->total_stuck_events;
    }
    if (successful_recoveries) {
        *successful_recoveries = detector->successful_recoveries;
    }
}

/* ===== Internal Helper Functions ===== */

/**
 * @brief Update IMU data and calculate magnitudes
 */
static HAL_StatusTypeDef StuckDetection_UpdateIMU(StuckDetection_t *detector)
{
    if (MPU6050_ReadData(detector->i2c, &detector->imu_data) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // Calculate acceleration magnitude in milli-g
    float ax = detector->imu_data.accel_x / MPU6050_ACCEL_SCALE;
    float ay = detector->imu_data.accel_y / MPU6050_ACCEL_SCALE;
    float az = detector->imu_data.accel_z / MPU6050_ACCEL_SCALE;
    detector->imu_data.accel_magnitude_mg = sqrtf(ax*ax + ay*ay + az*az) * 1000.0f;
    
    // Calculate gyroscope magnitude in deg/s
    float gx = detector->imu_data.gyro_x / MPU6050_GYRO_SCALE;
    float gy = detector->imu_data.gyro_y / MPU6050_GYRO_SCALE;
    float gz = detector->imu_data.gyro_z / MPU6050_GYRO_SCALE;
    detector->imu_data.gyro_magnitude_dps = sqrtf(gx*gx + gy*gy + gz*gz);
    
    return HAL_OK;
}

/**
 * @brief Check if motion exceeds thresholds
 */
static bool StuckDetection_CheckMotionThreshold(const IMU_Data_t *imu)
{
    // Subtract gravity (1g = 1000mg) from acceleration magnitude
    float dynamic_accel = fabsf(imu->accel_magnitude_mg - 1000.0f);
    
    // Motion detected if either threshold exceeded
    return (dynamic_accel > STUCK_ACCEL_THRESHOLD_MG) ||
           (imu->gyro_magnitude_dps > STUCK_GYRO_THRESHOLD_DPS);
}

/**
 * @brief Execute recovery sequence based on current strategy
 */
static HAL_StatusTypeDef StuckDetection_ExecuteRecovery(StuckDetection_t *detector)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed = current_time - detector->recovery_timer_start;
    
    switch (detector->current_strategy) {
        case RECOVERY_REVERSE:
            // Reverse for STUCK_RECOVERY_REVERSE_MS
            if (elapsed == 0) {
                Motor_ExecuteCommand(detector->motor, MOTOR_CMD_BACKWARD);
            }
            if (elapsed >= STUCK_RECOVERY_REVERSE_MS) {
                return HAL_OK;  // Recovery step complete
            }
            break;
            
        case RECOVERY_TURN_LEFT:
            // Turn left for STUCK_RECOVERY_TURN_MS
            if (elapsed == 0) {
                Motor_ExecuteCommand(detector->motor, MOTOR_CMD_LEFT);
            }
            if (elapsed >= STUCK_RECOVERY_TURN_MS) {
                return HAL_OK;
            }
            break;
            
        case RECOVERY_TURN_RIGHT:
            // Turn right for STUCK_RECOVERY_TURN_MS
            if (elapsed == 0) {
                Motor_ExecuteCommand(detector->motor, MOTOR_CMD_RIGHT);
            }
            if (elapsed >= STUCK_RECOVERY_TURN_MS) {
                return HAL_OK;
            }
            break;
            
        case RECOVERY_FAILED:
        case RECOVERY_NONE:
        default:
            return HAL_OK;
    }
    
    return HAL_BUSY;  // Recovery in progress
}

/**
 * @brief Advance to next recovery strategy
 */
static void StuckDetection_AdvanceRecoveryStrategy(StuckDetection_t *detector)
{
    detector->recovery_attempt_count++;
    detector->recovery_timer_start = HAL_GetTick();
    
    switch (detector->current_strategy) {
        case RECOVERY_REVERSE:
            detector->current_strategy = RECOVERY_TURN_LEFT;
            break;
        case RECOVERY_TURN_LEFT:
            detector->current_strategy = RECOVERY_TURN_RIGHT;
            break;
        case RECOVERY_TURN_RIGHT:
        default:
            detector->current_strategy = RECOVERY_FAILED;
            break;
    }
}

/* ===== MPU6050 Low-Level Functions ===== */

/**
 * @brief Initialize MPU6050
 */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *i2c)
{
    uint8_t check;
    uint8_t data;
    
    // Check WHO_AM_I (should return 0x68)
    if (HAL_I2C_Mem_Read(i2c, MPU6050_I2C_ADDR, MPU6050_REG_WHO_AM_I, 
                         1, &check, 1, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }
    
    if (check != 0x68) {
        return HAL_ERROR;  // Wrong device
    }
    
    // Wake up MPU6050 (clear sleep bit)
    data = 0x00;
    if (HAL_I2C_Mem_Write(i2c, MPU6050_I2C_ADDR, MPU6050_REG_PWR_MGMT_1,
                          1, &data, 1, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }
    
    HAL_Delay(100);  // Wait for sensor to stabilize
    
    return HAL_OK;
}

/**
 * @brief Read IMU data
 */
HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef *i2c, IMU_Data_t *data)
{
    uint8_t buffer[14];
    
    // Read 14 bytes starting from ACCEL_XOUT_H
    if (HAL_I2C_Mem_Read(i2c, MPU6050_I2C_ADDR, MPU6050_REG_ACCEL_XOUT_H,
                         1, buffer, 14, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // Parse accelerometer data (high byte first)
    data->accel_x = (int16_t)((buffer[0] << 8) | buffer[1]);
    data->accel_y = (int16_t)((buffer[2] << 8) | buffer[3]);
    data->accel_z = (int16_t)((buffer[4] << 8) | buffer[5]);
    // buffer[6-7] is temperature (not used)
    
    // Parse gyroscope data
    data->gyro_x = (int16_t)((buffer[8] << 8) | buffer[9]);
    data->gyro_y = (int16_t)((buffer[10] << 8) | buffer[11]);
    data->gyro_z = (int16_t)((buffer[12] << 8) | buffer[13]);
    
    return HAL_OK;
}
