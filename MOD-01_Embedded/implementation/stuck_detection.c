/**
 * @file      stuck_detection.c
 * @brief     MPU6050 IMU-Based Stuck Detection with Gravity Compensation
 * @author    Implementation Team
 * @date      2026-05-17
 * @version   1.0
 * 
 * Features:
 * - IMU initialization and calibration
 * - Gravity compensation for accurate motion detection
 * - Movement magnitude calculation
 * - Multi-stage recovery strategy
 * - Yaw angle integration for navigation
 */

#include "stuck_detection.h"
#include "motor_control.h"
#include <math.h>
#include <string.h>

/* -- Private Constants ----------------------------------------------------- */
#define GRAVITY_MAGNITUDE_G       1.0f        /**< Earth's gravity in g */
#define DEG_TO_RAD                0.017453292f
#define RAD_TO_DEG                57.29577951f

/* -- Private Types --------------------------------------------------------- */
typedef struct {
    float accel_x_offset;
    float accel_y_offset;
    float accel_z_offset;
    float gyro_x_offset;
    float gyro_y_offset;
    float gyro_z_offset;
} imu_calibration_t;

typedef struct {
    float accel_history[IMU_FILTER_WINDOW_SIZE];
    float gyro_history[IMU_FILTER_WINDOW_SIZE];
    uint8_t history_index;
    bool history_filled;
} imu_filter_t;

/* -- Private Variables ----------------------------------------------------- */
static bool initialized = false;
static imu_calibration_t calibration = {0};
static imu_filter_t filter = {0};

static uint32_t stuck_start_time = 0;
static bool currently_stuck = false;
static float yaw_angle = 0.0f;
static uint32_t last_update_time = 0;

/* -- Private Function Prototypes ------------------------------------------- */
static stuck_status_t mpu6050_write_byte(uint8_t reg, uint8_t value);
static stuck_status_t mpu6050_read_bytes(uint8_t reg, uint8_t *data, uint16_t len);
static void compensate_gravity(imu_processed_data_t *data);
static float moving_average_filter(float new_value, float *history, uint8_t window_size);
static void update_yaw_angle(float gyro_z_dps, uint32_t dt_ms);

/* -- Public Function Implementations --------------------------------------- */

stuck_status_t stuck_detection_init(void)
{
    if (initialized) {
        return STUCK_OK;
    }

    // Initialize I2C peripheral (assuming HAL is used)
    // This should be done in your main initialization

    // Check if MPU6050 is connected
    if (!imu_is_connected()) {
        return STUCK_ERR_I2C;
    }

    // Wake up MPU6050 (reset sleep bit)
    if (mpu6050_write_byte(MPU6050_REG_PWR_MGMT_1, 0x00) != STUCK_OK) {
        return STUCK_ERR_INIT;
    }

    HAL_Delay(100); // Wait for sensor to stabilize

    // Set accelerometer range to ±2g
    if (mpu6050_write_byte(0x1C, 0x00) != STUCK_OK) {
        return STUCK_ERR_INIT;
    }

    // Set gyroscope range to ±250°/s
    if (mpu6050_write_byte(0x1B, 0x00) != STUCK_OK) {
        return STUCK_ERR_INIT;
    }

    // Set sample rate divider (1kHz / (1 + 19) = 50Hz)
    if (mpu6050_write_byte(0x19, 19) != STUCK_OK) {
        return STUCK_ERR_INIT;
    }

    // Enable low-pass filter (bandwidth ~21Hz)
    if (mpu6050_write_byte(0x1A, 0x04) != STUCK_OK) {
        return STUCK_ERR_INIT;
    }

    // Calibrate sensors
    if (imu_calibrate(200) != STUCK_OK) {
        return STUCK_ERR_CALIB;
    }

    // Initialize filter
    memset(&filter, 0, sizeof(filter));

    // Initialize state
    stuck_reset();
    last_update_time = HAL_GetTick();

    initialized = true;
    return STUCK_OK;
}

stuck_status_t imu_read_raw(imu_raw_data_t *data)
{
    if (!initialized || data == NULL) {
        return STUCK_ERR_INIT;
    }

    uint8_t buffer[14];

    // Read all sensor data in one burst (accelerometer + temperature + gyroscope)
    if (mpu6050_read_bytes(MPU6050_REG_ACCEL_XOUT_H, buffer, 14) != STUCK_OK) {
        return STUCK_ERR_I2C;
    }

    // Parse accelerometer data (big-endian)
    data->accel_x = (int16_t)((buffer[0] << 8) | buffer[1]);
    data->accel_y = (int16_t)((buffer[2] << 8) | buffer[3]);
    data->accel_z = (int16_t)((buffer[4] << 8) | buffer[5]);

    // Parse temperature
    data->temp = (int16_t)((buffer[6] << 8) | buffer[7]);

    // Parse gyroscope data (big-endian)
    data->gyro_x = (int16_t)((buffer[8] << 8) | buffer[9]);
    data->gyro_y = (int16_t)((buffer[10] << 8) | buffer[11]);
    data->gyro_z = (int16_t)((buffer[12] << 8) | buffer[13]);

    return STUCK_OK;
}

void imu_process_data(const imu_raw_data_t *raw, imu_processed_data_t *processed)
{
    if (raw == NULL || processed == NULL) {
        return;
    }

    // Convert accelerometer to g (apply calibration)
    processed->accel_x_g = (raw->accel_x / MPU6050_ACCEL_SCALE_FACTOR) - calibration.accel_x_offset;
    processed->accel_y_g = (raw->accel_y / MPU6050_ACCEL_SCALE_FACTOR) - calibration.accel_y_offset;
    processed->accel_z_g = (raw->accel_z / MPU6050_ACCEL_SCALE_FACTOR) - calibration.accel_z_offset;

    // Convert gyroscope to deg/s (apply calibration)
    processed->gyro_x_dps = (raw->gyro_x / MPU6050_GYRO_SCALE_FACTOR) - calibration.gyro_x_offset;
    processed->gyro_y_dps = (raw->gyro_y / MPU6050_GYRO_SCALE_FACTOR) - calibration.gyro_y_offset;
    processed->gyro_z_dps = (raw->gyro_z / MPU6050_GYRO_SCALE_FACTOR) - calibration.gyro_z_offset;

    // Convert temperature (formula from MPU6050 datasheet)
    processed->temp_celsius = (raw->temp / 340.0f) + 36.53f;

    // Apply gravity compensation
    compensate_gravity(processed);
}

stuck_status_t stuck_check(bool motors_active, stuck_detection_result_t *result)
{
    if (!initialized || result == NULL) {
        return STUCK_ERR_INIT;
    }

    // Read IMU data
    imu_raw_data_t raw_data;
    if (imu_read_raw(&raw_data) != STUCK_OK) {
        return STUCK_ERR_I2C;
    }

    // Process data
    imu_processed_data_t processed_data;
    imu_process_data(&raw_data, &processed_data);

    // Update yaw angle
    uint32_t current_time = HAL_GetTick();
    uint32_t dt = current_time - last_update_time;
    update_yaw_angle(processed_data.gyro_z_dps, dt);
    last_update_time = current_time;

    // Compute movement magnitude with filtering
    float magnitude = stuck_compute_movement_magnitude(&processed_data);

    // Apply moving average filter
    magnitude = moving_average_filter(magnitude, filter.accel_history, IMU_FILTER_WINDOW_SIZE);

    // Determine stuck condition
    bool moving = (magnitude > (STUCK_ACCEL_THRESHOLD_MG / 1000.0f));

    if (motors_active && !moving) {
        // Motors are on but no movement detected
        if (!currently_stuck) {
            stuck_start_time = current_time;
            currently_stuck = true;
        }

        uint32_t stuck_duration = current_time - stuck_start_time;

        if (stuck_duration >= STUCK_TIMEOUT_MS) {
            result->is_stuck = true;
            result->stuck_duration_ms = stuck_duration;
        } else {
            result->is_stuck = false;
            result->stuck_duration_ms = stuck_duration;
        }
    } else {
        // Either motors are off or movement is detected
        if (currently_stuck) {
            stuck_reset();
        }
        result->is_stuck = false;
        result->stuck_duration_ms = 0;
    }

    result->movement_magnitude = magnitude;
    result->timestamp_ms = current_time;

    return STUCK_OK;
}

stuck_recovery_action_t stuck_get_recovery_action(const stuck_detection_result_t *result)
{
    if (result == NULL || !result->is_stuck) {
        return RECOVERY_NONE;
    }

    // Multi-stage recovery strategy based on stuck duration
    if (result->stuck_duration_ms < 3000) {
        // Stage 1: Simple reverse (2-3 seconds stuck)
        return RECOVERY_REVERSE;
    } else if (result->stuck_duration_ms < 6000) {
        // Stage 2: Rotate to find new path (3-6 seconds stuck)
        return RECOVERY_ROTATE;
    } else {
        // Stage 3: Emergency stop and alert (>6 seconds stuck)
        return RECOVERY_EMERGENCY;
    }
}

void stuck_execute_recovery(stuck_recovery_action_t action)
{
    switch (action) {
        case RECOVERY_REVERSE:
            // Simple reverse maneuver
            motor_set_state(UART_DIR_BACKWARD, MOTOR_SPEED_DEFAULT);
            HAL_Delay(STUCK_RECOVERY_BACKOFF_MS);
            motor_soft_stop(500);
            break;

        case RECOVERY_ROTATE:
            // Rotate 90 degrees to find new path
            motor_set_state(UART_DIR_RIGHT, MOTOR_SPEED_DEFAULT);
            HAL_Delay(1000); // Approximate 90-degree turn time
            motor_soft_stop(500);
            break;

        case RECOVERY_EMERGENCY:
            // Emergency stop and hold
            motor_emergency_stop();
            // Alert should be sent via UART telemetry
            break;

        case RECOVERY_NONE:
        default:
            // No action needed
            break;
    }
}

stuck_status_t imu_calibrate(uint16_t sample_count)
{
    if (!initialized) {
        return STUCK_ERR_INIT;
    }

    if (sample_count == 0) {
        return STUCK_ERR_CALIB;
    }

    float accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
    float gyro_x_sum = 0, gyro_y_sum = 0, gyro_z_sum = 0;

    for (uint16_t i = 0; i < sample_count; i++) {
        imu_raw_data_t raw;
        if (imu_read_raw(&raw) != STUCK_OK) {
            return STUCK_ERR_I2C;
        }

        accel_x_sum += raw.accel_x / MPU6050_ACCEL_SCALE_FACTOR;
        accel_y_sum += raw.accel_y / MPU6050_ACCEL_SCALE_FACTOR;
        accel_z_sum += raw.accel_z / MPU6050_ACCEL_SCALE_FACTOR;

        gyro_x_sum += raw.gyro_x / MPU6050_GYRO_SCALE_FACTOR;
        gyro_y_sum += raw.gyro_y / MPU6050_GYRO_SCALE_FACTOR;
        gyro_z_sum += raw.gyro_z / MPU6050_GYRO_SCALE_FACTOR;

        HAL_Delay(5);
    }

    // Calculate offsets
    calibration.accel_x_offset = accel_x_sum / sample_count;
    calibration.accel_y_offset = accel_y_sum / sample_count;
    calibration.accel_z_offset = (accel_z_sum / sample_count) - GRAVITY_MAGNITUDE_G; // Remove gravity

    calibration.gyro_x_offset = gyro_x_sum / sample_count;
    calibration.gyro_y_offset = gyro_y_sum / sample_count;
    calibration.gyro_z_offset = gyro_z_sum / sample_count;

    return STUCK_OK;
}

void stuck_reset(void)
{
    currently_stuck = false;
    stuck_start_time = 0;
}

float imu_get_yaw_angle(void)
{
    return yaw_angle;
}

bool imu_is_connected(void)
{
    uint8_t who_am_i = 0;

    if (mpu6050_read_bytes(MPU6050_REG_WHO_AM_I, &who_am_i, 1) != STUCK_OK) {
        return false;
    }

    return (who_am_i == 0x68);
}

float stuck_compute_movement_magnitude(const imu_processed_data_t *data)
{
    if (data == NULL) {
        return 0.0f;
    }

    // Compute linear acceleration magnitude (gravity compensated)
    float accel_magnitude = sqrtf(
        data->accel_x_g * data->accel_x_g +
        data->accel_y_g * data->accel_y_g +
        data->accel_z_g * data->accel_z_g
    );

    // Compute angular velocity magnitude
    float gyro_magnitude = sqrtf(
        data->gyro_x_dps * data->gyro_x_dps +
        data->gyro_y_dps * data->gyro_y_dps +
        data->gyro_z_dps * data->gyro_z_dps
    );

    // Combine with weighting (prioritize gyro for rotation detection)
    // Scale gyro to comparable range (1 deg/s ~ 0.01 g of linear motion)
    float combined_magnitude = accel_magnitude + (gyro_magnitude * 0.01f);

    return combined_magnitude;
}

/* -- Private Function Implementations -------------------------------------- */

static stuck_status_t mpu6050_write_byte(uint8_t reg, uint8_t value)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        &hi2c1,                  // I2C handle (extern from main)
        MPU6050_I2C_ADDR << 1,   // Device address (shifted for HAL)
        reg,                     // Register address
        I2C_MEMADD_SIZE_8BIT,    // 8-bit register address
        &value,                  // Data to write
        1,                       // Number of bytes
        100                      // Timeout in ms
    );

    return (status == HAL_OK) ? STUCK_OK : STUCK_ERR_I2C;
}

static stuck_status_t mpu6050_read_bytes(uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        &hi2c1,                  // I2C handle (extern from main)
        MPU6050_I2C_ADDR << 1,   // Device address (shifted for HAL)
        reg,                     // Register address
        I2C_MEMADD_SIZE_8BIT,    // 8-bit register address
        data,                    // Buffer to store data
        len,                     // Number of bytes to read
        100                      // Timeout in ms
    );

    return (status == HAL_OK) ? STUCK_OK : STUCK_ERR_I2C;
}

static void compensate_gravity(imu_processed_data_t *data)
{
    if (data == NULL) {
        return;
    }

    // Compute current gravity vector magnitude
    float gravity_magnitude = sqrtf(
        data->accel_x_g * data->accel_x_g +
        data->accel_y_g * data->accel_y_g +
        data->accel_z_g * data->accel_z_g
    );

    // If close to 1g, we're stationary - subtract gravity vector
    if (fabsf(gravity_magnitude - GRAVITY_MAGNITUDE_G) < 0.2f) {
        // Assume gravity is primarily on Z-axis when stationary
        // For a robot on flat ground, Z should read ~1g
        float gravity_z = data->accel_z_g;
        
        // Remove gravity component from Z-axis
        data->accel_z_g = data->accel_z_g - (gravity_z / fabsf(gravity_z)) * GRAVITY_MAGNITUDE_G;
        
        // If robot is tilted, need more sophisticated compensation
        // For now, simple Z-axis compensation is sufficient for stuck detection
    }
}

static float moving_average_filter(float new_value, float *history, uint8_t window_size)
{
    // Shift history
    for (uint8_t i = window_size - 1; i > 0; i--) {
        history[i] = history[i - 1];
    }
    history[0] = new_value;

    // Compute average
    float sum = 0;
    for (uint8_t i = 0; i < window_size; i++) {
        sum += history[i];
    }

    return sum / window_size;
}

static void update_yaw_angle(float gyro_z_dps, uint32_t dt_ms)
{
    // Integrate gyroscope reading to get yaw angle
    float delta_angle = gyro_z_dps * (dt_ms / 1000.0f);
    
    yaw_angle += delta_angle;

    // Normalize to 0-360 degrees
    while (yaw_angle >= 360.0f) {
        yaw_angle -= 360.0f;
    }
    while (yaw_angle < 0.0f) {
        yaw_angle += 360.0f;
    }
}
