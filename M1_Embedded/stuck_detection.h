#ifndef STUCK_DETECTION_H
#define STUCK_DETECTION_H

/**
 * @file      stuck_detection.h
 * @brief     MPU6050 IMU-Based Stuck Detection and Recovery Logic
 * @author    Ömer [Student ID]
 * @date      2026-03-29
 * @version   0.1
 * 
 * Changelog:
 * v0.1 (2026-03-29) - Initial draft, defined stuck detection logic and IMU interface.
 * 
 * Purpose:
 * This module detects when the robot is stuck by comparing commanded motor state
 * with actual IMU motion data from the MPU6050. When motors are running but the
 * IMU shows no movement, the robot is considered stuck and triggers recovery protocols.
 * 
 * Hardware Dependencies:
 * - MPU6050 6-axis IMU (Accelerometer + Gyroscope)
 * - I2C1 interface on STM32F103C8T6
 * - Motor control feedback from motor_control.h
 * 
 * Detection Logic:
 * The module monitors:
 * 1. Motor command state (from uart_comm.h)
 * 2. Linear acceleration (accelerometer data)
 * 3. Angular velocity (gyroscope data)
 * 
 * Stuck condition = (Motors ON) AND (No significant IMU movement for STUCK_TIMEOUT_MS)
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros ---------------------------------------------------- */

/** MPU6050 I2C Configuration */
#define MPU6050_I2C_ADDR            0x68        /**< MPU6050 default I2C address */
#define MPU6050_I2C_SPEED_HZ        100000      /**< I2C clock speed (100 kHz) */

/** MPU6050 Register Addresses */
#define MPU6050_REG_PWR_MGMT_1      0x6B        /**< Power management register */
#define MPU6050_REG_ACCEL_XOUT_H    0x3B        /**< Accelerometer X-axis high byte */
#define MPU6050_REG_GYRO_XOUT_H     0x43        /**< Gyroscope X-axis high byte */
#define MPU6050_REG_WHO_AM_I        0x75        /**< Device ID register */

/** Stuck Detection Thresholds */
#define STUCK_ACCEL_THRESHOLD_MG    50          /**< Min acceleration change to consider moving (mg) */
#define STUCK_GYRO_THRESHOLD_DPS    5.0f        /**< Min angular velocity to consider rotating (deg/s) */
#define STUCK_TIMEOUT_MS            2000        /**< Time with no movement to trigger stuck flag (ms) */
#define STUCK_RECOVERY_BACKOFF_MS   1500        /**< Duration to reverse when stuck (ms) */

/** IMU Sampling Configuration */
#define IMU_SAMPLE_RATE_HZ          50          /**< IMU data sampling frequency */
#define IMU_FILTER_WINDOW_SIZE      5           /**< Moving average filter window */

/** Accelerometer Sensitivity (±2g range) */
#define MPU6050_ACCEL_SCALE_FACTOR  16384.0f    /**< LSB per g at ±2g range */

/** Gyroscope Sensitivity (±250°/s range) */
#define MPU6050_GYRO_SCALE_FACTOR   131.0f      /**< LSB per °/s at ±250°/s range */

/* -- Data Types ------------------------------------------------------------ */

/**
 * @brief Stuck detection status codes
 */
typedef enum {
    STUCK_OK              =  0,    /**< Operation successful */
    STUCK_ERR_INIT        = -1,    /**< IMU initialization failed */
    STUCK_ERR_I2C         = -2,    /**< I2C communication error */
    STUCK_ERR_TIMEOUT     = -3,    /**< IMU read timeout */
    STUCK_ERR_CALIB       = -4     /**< Calibration failed */
} stuck_status_t;

/**
 * @brief Raw IMU sensor data
 */
typedef struct {
    int16_t accel_x;    /**< Raw accelerometer X-axis */
    int16_t accel_y;    /**< Raw accelerometer Y-axis */
    int16_t accel_z;    /**< Raw accelerometer Z-axis */
    int16_t gyro_x;     /**< Raw gyroscope X-axis */
    int16_t gyro_y;     /**< Raw gyroscope Y-axis */
    int16_t gyro_z;     /**< Raw gyroscope Z-axis */
    int16_t temp;       /**< Raw temperature reading */
} imu_raw_data_t;

/**
 * @brief Processed IMU data in physical units
 */
typedef struct {
    float accel_x_g;        /**< X acceleration in g */
    float accel_y_g;        /**< Y acceleration in g */
    float accel_z_g;        /**< Z acceleration in g */
    float gyro_x_dps;       /**< X angular velocity in deg/s */
    float gyro_y_dps;       /**< Y angular velocity in deg/s */
    float gyro_z_dps;       /**< Z angular velocity in deg/s (yaw rate) */
    float temp_celsius;     /**< Temperature in Celsius */
} imu_processed_data_t;

/**
 * @brief Stuck detection result
 */
typedef struct {
    bool    is_stuck;               /**< True if robot is detected as stuck */
    uint32_t stuck_duration_ms;     /**< Duration of stuck condition (ms) */
    float   movement_magnitude;     /**< Combined accel + gyro magnitude */
    uint32_t timestamp_ms;          /**< Timestamp of detection (ms since boot) */
} stuck_detection_result_t;

/**
 * @brief Stuck recovery action recommendations
 */
typedef enum {
    RECOVERY_NONE       = 0,    /**< No action needed, robot is moving */
    RECOVERY_REVERSE    = 1,    /**< Reverse motors for STUCK_RECOVERY_BACKOFF_MS */
    RECOVERY_ROTATE     = 2,    /**< Rotate in place to find new path */
    RECOVERY_EMERGENCY  = 3     /**< Stop motors, alert operator (critical stuck) */
} stuck_recovery_action_t;

/* -- Public Functions ------------------------------------------------------ */

/**
 * @brief  Initializes the MPU6050 IMU sensor and stuck detection module.
 *         Configures I2C, wakes up MPU6050, and performs initial calibration.
 * @return STUCK_OK on success, negative error code otherwise.
 */
stuck_status_t stuck_detection_init(void);

/**
 * @brief  Reads raw sensor data from the MPU6050.
 * @param  data  Pointer to imu_raw_data_t struct to store raw readings.
 * @return STUCK_OK on success, negative error code otherwise.
 */
stuck_status_t imu_read_raw(imu_raw_data_t *data);

/**
 * @brief  Converts raw IMU data to physical units (g, deg/s, °C).
 * @param  raw        Pointer to raw sensor data.
 * @param  processed  Pointer to store processed data.
 */
void imu_process_data(const imu_raw_data_t *raw, imu_processed_data_t *processed);

/**
 * @brief  Main stuck detection function - compares motor state with IMU motion.
 *         Should be called periodically (every 20-50 ms) in the main loop.
 * @param  motors_active  True if motors are currently commanded to move.
 * @param  result         Pointer to stuck_detection_result_t to store detection result.
 * @return STUCK_OK on success, negative error code otherwise.
 * 
 * @note This function updates the is_stuck flag in uart_telemetry_t.
 *       The flag is then sent to the Raspberry Pi 5 via uart_send_telemetry().
 */
stuck_status_t stuck_check(bool motors_active, stuck_detection_result_t *result);

/**
 * @brief  Determines the recommended recovery action based on stuck detection.
 * @param  result  Pointer to the stuck detection result.
 * @return Recommended recovery action (RECOVERY_NONE, RECOVERY_REVERSE, etc.)
 */
stuck_recovery_action_t stuck_get_recovery_action(const stuck_detection_result_t *result);

/**
 * @brief  Executes automatic recovery when stuck is detected.
 *         Called by the main loop when stuck_check() returns is_stuck = true.
 * @param  action  Recovery action to execute (from stuck_get_recovery_action).
 */
void stuck_execute_recovery(stuck_recovery_action_t action);

/**
 * @brief  Calibrates the IMU sensors by computing zero-offset values.
 *         Robot must be stationary during calibration.
 * @param  sample_count  Number of samples to average (recommended: 100-200).
 * @return STUCK_OK on success, negative error code otherwise.
 */
stuck_status_t imu_calibrate(uint16_t sample_count);

/**
 * @brief  Resets the stuck detection state machine.
 *         Clears accumulated stuck duration and movement history.
 */
void stuck_reset(void);

/**
 * @brief  Gets the current IMU yaw angle (heading) in degrees.
 *         Integrates gyro_z over time to estimate robot orientation.
 * @return Current yaw angle in degrees (0-360).
 * 
 * @note This value is included in uart_telemetry_t.imu_yaw_angle
 *       for mapping and navigation purposes.
 */
float imu_get_yaw_angle(void);

/**
 * @brief  Checks if the MPU6050 is responding on the I2C bus.
 * @return True if MPU6050 WHO_AM_I register reads 0x68, false otherwise.
 */
bool imu_is_connected(void);

/**
 * @brief  Computes the magnitude of movement (combined accel + gyro).
 * @param  data  Pointer to processed IMU data.
 * @return Movement magnitude (arbitrary units, used for threshold comparison).
 */
float stuck_compute_movement_magnitude(const imu_processed_data_t *data);

#endif /* STUCK_DETECTION_H */
