#ifndef ENVIRONMENT_SENSORS_H
#define ENVIRONMENT_SENSORS_H

/**
 * @file    environment_sensors.h
 * @brief   Environmental sensor interface for DHT11 and MQ-2 readings
 * @author  Gabil Rahimli 230104004902
 * @date    2026-03-29
 * @version 0.1
 *
 * Changelog:
 *   v0.1 - Initial draft for DHT11 temperature/humidity and MQ-2 smoke sensor interface.
 *   v0.2 - Updated DHT22 to DHT11 for v4 hardware scheme.
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants ------------------------------------------------------------ */

#define ENV_SENSOR_OK              0
#define ENV_SENSOR_ERR_INIT       -1
#define ENV_SENSOR_ERR_READ       -2
#define ENV_SENSOR_ERR_INVALID    -3

#define ENV_SENSOR_DHT11_ID        1
#define ENV_SENSOR_MQ2_ID          2

#define ENV_SENSOR_MAX_TEMP_C      80.0f
#define ENV_SENSOR_MIN_TEMP_C     -40.0f

/* -- Data Types ----------------------------------------------------------- */

/**
 * @brief Supported environment sensor identifiers.
 */
typedef enum
{
    ENV_SENSOR_TYPE_DHT11 = 0,
    ENV_SENSOR_TYPE_MQ2   = 1
} env_sensor_type_t;

/**
 * @brief Combined environment sensor data packet.
 *
 * This structure can be sent to higher-level modules through UART or
 * other module interfaces.
 */
typedef struct
{
    float    temperature_c;   /**< Temperature in Celsius from DHT11 */
    float    humidity_pct;    /**< Relative humidity percentage from DHT11 */
    uint16_t smoke_raw;       /**< Raw analog smoke/gas reading from MQ-2 */
    bool     smoke_alert;     /**< Threshold-based smoke alert flag */
    uint32_t timestamp_ms;    /**< Timestamp in milliseconds since boot */
} environment_data_t;

/* -- Public Functions ----------------------------------------------------- */

/**
 * @brief  Initialize all environmental sensors used by the module.
 *
 * This function prepares DHT11 and MQ-2 related hardware resources.
 *
 * @return ENV_SENSOR_OK on success, negative error code otherwise.
 */
int env_sensors_init(void);

/**
 * @brief  Read DHT11 temperature and humidity values.
 *
 * @param  temperature_c_out Pointer to temperature output in Celsius.
 * @param  humidity_pct_out Pointer to humidity output in percentage.
 * @return ENV_SENSOR_OK on success, negative error code otherwise.
 */
int env_read_dht11(float *temperature_c_out, float *humidity_pct_out);

/**
 * @brief  Read raw MQ-2 smoke/gas sensor value.
 *
 * @param  smoke_raw_out Pointer to raw MQ-2 ADC reading output.
 * @return ENV_SENSOR_OK on success, negative error code otherwise.
 */
int env_read_mq2(uint16_t *smoke_raw_out);

/**
 * @brief  Read all environment sensors and fill a combined data structure.
 *
 * @param  out_data Pointer to caller-owned output structure.
 * @return ENV_SENSOR_OK on success, negative error code otherwise.
 */
int env_read_all(environment_data_t *out_data);

/**
 * @brief  Set MQ-2 smoke alert threshold.
 *
 * @param  threshold Raw ADC threshold value.
 */
void env_set_smoke_threshold(uint16_t threshold);

/**
 * @brief  Check whether the given MQ-2 raw value exceeds current smoke threshold.
 *
 * @param  smoke_raw Raw MQ-2 ADC reading.
 * @return true if smoke alert condition is detected, false otherwise.
 */
bool env_is_smoke_alert(uint16_t smoke_raw);

#endif /* ENVIRONMENT_SENSORS_H */
