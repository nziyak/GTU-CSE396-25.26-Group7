#ifndef STUCK_DETECTION_H
#define STUCK_DETECTION_H

/**
 * @file      stuck_detection.h
 * @brief     MPU6050 IMU-Based Stuck Detection and Recovery Logic
 * @author    Ömer 210104004027
 * @date      2026-03-29
 * @version   1.0
 *
 * Changelog:
 *   v0.1 (2026-03-29) - Initial draft.
 *   v1.0 (2026-05-18) - Updated for HardwareScheme v5.2.
 *                       Added StuckDetection_t object-oriented API.
 *                       IMU on I2C1 (PB6/PB7), INT on PB5.
 */

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "motor_control.h"

/* -- MPU6050 Configuration ------------------------------------------------- */
#define MPU6050_I2C_ADDR            (0x68 << 1)  /**< 7-bit addr shifted for HAL */
#define MPU6050_REG_PWR_MGMT_1      0x6B
#define MPU6050_REG_ACCEL_XOUT_H    0x3B
#define MPU6050_REG_GYRO_XOUT_H     0x43
#define MPU6050_REG_WHO_AM_I        0x75

/* Scale factors */
#define MPU6050_ACCEL_SCALE_FACTOR  16384.0f   /**< LSB/g  at ±2g  */
#define MPU6050_ACCEL_SCALE         16384.0f   /**< Alias */
#define MPU6050_GYRO_SCALE_FACTOR   131.0f     /**< LSB/°/s at ±250°/s */
#define MPU6050_GYRO_SCALE          131.0f     /**< Alias */

/* -- Stuck Detection Thresholds -------------------------------------------- */
#define STUCK_ACCEL_THRESHOLD_MG    50          /**< Min dynamic accel (mg) */
#define STUCK_GYRO_THRESHOLD_DPS    5.0f        /**< Min angular velocity (°/s) */
#define STUCK_TIMEOUT_MS            2000        /**< No-motion timeout (ms) */
#define STUCK_RECOVERY_BACKOFF_MS   1500        /**< Original recovery backoff */
#define STUCK_RECOVERY_REVERSE_MS   1000        /**< Reverse phase duration */
#define STUCK_RECOVERY_TURN_MS      1000        /**< Turn phase duration */

/* -- IMU Sampling ---------------------------------------------------------- */
#define IMU_SAMPLE_RATE_HZ          50
#define IMU_FILTER_WINDOW_SIZE      5

/* -- Data Types ------------------------------------------------------------ */

/** @brief Raw IMU register data (14 bytes from ACCEL_XOUT_H) */
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temp;
} imu_raw_data_t;

/** @brief Processed IMU data in SI units */
typedef struct {
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;   /**< Yaw rate */
    float temp_celsius;
} imu_processed_data_t;

/** @brief Extended IMU data with pre-computed magnitudes (used by StuckDetection_t) */
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    float   accel_magnitude_mg;
    float   gyro_magnitude_dps;
} IMU_Data_t;

/** @brief Stuck detection FSM states */
typedef enum {
    STUCK_STATE_MONITORING = 0,
    STUCK_STATE_POSSIBLE   = 1,
    STUCK_STATE_CONFIRMED  = 2,
    STUCK_STATE_RECOVERING = 3
} StuckState_t;

/** @brief Recovery strategies */
typedef enum {
    RECOVERY_NONE        = 0,
    RECOVERY_REVERSE     = 1,
    RECOVERY_ROTATE      = 2,
    RECOVERY_EMERGENCY   = 3,
    RECOVERY_TURN_LEFT   = 4,
    RECOVERY_TURN_RIGHT  = 5,
    RECOVERY_FAILED      = 6
} stuck_recovery_action_t;

/** @brief Simple stuck detection result (used by legacy API) */
typedef struct {
    bool     is_stuck;
    uint32_t stuck_duration_ms;
    float    movement_magnitude;
    uint32_t timestamp_ms;
} stuck_detection_result_t;

/** @brief Full stuck detection instance */
typedef struct {
    I2C_HandleTypeDef *i2c;
    MotorControl_t    *motor;
    IMU_Data_t         imu_data;
    StuckState_t       state;
    int                current_strategy;
    uint32_t           stuck_timer_start;
    uint32_t           recovery_timer_start;
    uint32_t           total_stuck_events;
    uint32_t           successful_recoveries;
    MotorCommand_t     original_command;
    uint8_t            recovery_attempt_count;
} StuckDetection_t;

/** @brief Stuck detection status codes */
typedef enum {
    STUCK_OK          =  0,
    STUCK_ERR_INIT    = -1,
    STUCK_ERR_I2C     = -2,
    STUCK_ERR_TIMEOUT = -3,
    STUCK_ERR_CALIB   = -4
} stuck_status_t;

/* -- Object-Oriented API (main API) ---------------------------------------- */

HAL_StatusTypeDef StuckDetection_Init(StuckDetection_t *detector,
                                      I2C_HandleTypeDef *i2c,
                                      MotorControl_t *motor);
HAL_StatusTypeDef StuckDetection_Task(StuckDetection_t *detector);
bool              StuckDetection_IsRecovering(const StuckDetection_t *detector);
StuckState_t      StuckDetection_GetState(const StuckDetection_t *detector);
void              StuckDetection_Reset(StuckDetection_t *detector);
void              StuckDetection_GetStats(const StuckDetection_t *detector,
                                          uint32_t *total_events,
                                          uint32_t *successful_recoveries);

/* -- MPU6050 Low-Level API ------------------------------------------------- */

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *i2c);
HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef *i2c, IMU_Data_t *data);

/* -- Legacy API (kept for backward compatibility) -------------------------- */

stuck_status_t          stuck_detection_init(void);
stuck_status_t          imu_read_raw(imu_raw_data_t *data);
void                    imu_process_data(const imu_raw_data_t *raw,
                                         imu_processed_data_t *processed);
stuck_status_t          stuck_check(bool motors_active,
                                    stuck_detection_result_t *result);
stuck_recovery_action_t stuck_get_recovery_action(const stuck_detection_result_t *result);
void                    stuck_execute_recovery(stuck_recovery_action_t action);
stuck_status_t          imu_calibrate(uint16_t sample_count);
void                    stuck_reset(void);
float                   imu_get_yaw_angle(void);
bool                    imu_is_connected(void);
float                   stuck_compute_movement_magnitude(const imu_processed_data_t *data);

#endif /* STUCK_DETECTION_H */
