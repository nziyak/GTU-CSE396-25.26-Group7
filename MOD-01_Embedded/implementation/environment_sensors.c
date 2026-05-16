/**
 * @file    environment_sensors.c
 * @brief   DHT22 Temperature/Humidity and MQ-2 Smoke/Gas Sensor Implementation
 * @author  Gabil Rahimli 230104004902
 * @date    2026-05-16
 * @version 0.2
 *
 * Changelog:
 *   v0.1 - Header draft only.
 *   v0.2 - Full implementation: DHT22 single-wire protocol, MQ-2 ADC reading,
 *          combined read, and threshold-based smoke alert logic.
 *
 * Hardware Notes:
 *   - DHT22 data pin: PB12 (configurable via DHT22_GPIO_PORT / DHT22_GPIO_PIN)
 *   - MQ-2 analog out: PA7 / ADC1_IN7 (configurable via MQ2_ADC_CHANNEL)
 *   - DHT22 requires a 10k pull-up on the data line.
 *   - MQ-2 requires ~20s preheat after power-on for stable readings.
 */

#include "environment_sensors.h"
#include "stm32f1xx_hal.h"
#include <string.h>

/* ===== Configuration — adjust to match your wiring ====================== */

#define DHT22_GPIO_PORT           GPIOB
#define DHT22_GPIO_PIN            GPIO_PIN_12

#define MQ2_ADC_HANDLE            hadc1        /* extern declared below */
#define MQ2_ADC_CHANNEL           ADC_CHANNEL_7
#define MQ2_ADC_TIMEOUT_MS        50

/* Default smoke threshold (raw 12-bit ADC value).
 * Typical clean-air reading ~200-400, smoke present ~800+.
 * Tune empirically for the competition arena. */
#define MQ2_DEFAULT_THRESHOLD     800

/* DHT22 timing tolerances (microseconds) */
#define DHT22_START_LOW_US        1200
#define DHT22_START_HIGH_US       30
#define DHT22_RESPONSE_TIMEOUT_US 100
#define DHT22_BIT_THRESHOLD_US    40   /* >40us high = '1', <40us = '0' */

/* ===== External HAL Handles ============================================= */

extern ADC_HandleTypeDef MQ2_ADC_HANDLE;
extern TIM_HandleTypeDef htim6;   /* Microsecond delay timer (optional) */

/* ===== Module-level State =============================================== */

static uint16_t g_smoke_threshold = MQ2_DEFAULT_THRESHOLD;
static bool     g_initialized     = false;

/* ===== Internal Helpers ================================================= */

/**
 * @brief  Microsecond delay using DWT cycle counter (Cortex-M4).
 *         Falls back to HAL_Delay(1) if DWT is unavailable.
 */
static void delay_us(uint32_t us)
{
    /* Use DWT (Data Watchpoint and Trace) cycle counter for precise us delay */
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

/**
 * @brief  Enable DWT cycle counter for microsecond timing.
 */
static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  Set DHT22 data pin as output (push-pull).
 */
static void dht22_pin_output(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = DHT22_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &gpio);
}

/**
 * @brief  Set DHT22 data pin as input (floating, external pull-up expected).
 */
static void dht22_pin_input(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = DHT22_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &gpio);
}

/**
 * @brief  Read the DHT22 data pin state.
 */
static GPIO_PinState dht22_read_pin(void)
{
    return HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN);
}

/**
 * @brief  Wait for pin to reach expected state with a timeout.
 * @return Elapsed microseconds, or UINT32_MAX on timeout.
 */
static uint32_t dht22_wait_for_state(GPIO_PinState expected, uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks_per_us = HAL_RCC_GetHCLKFreq() / 1000000;
    uint32_t timeout_ticks = timeout_us * ticks_per_us;

    while (dht22_read_pin() != expected) {
        if ((DWT->CYCCNT - start) > timeout_ticks) {
            return UINT32_MAX; /* timeout */
        }
    }
    return (DWT->CYCCNT - start) / ticks_per_us;
}

/**
 * @brief  Read 40 bits from DHT22 (16-bit humidity + 16-bit temperature + 8-bit checksum).
 * @param  data  Output array of 5 bytes.
 * @return ENV_SENSOR_OK on success, negative error on failure.
 */
static int dht22_read_raw(uint8_t data[5])
{
    memset(data, 0, 5);

    /* ---- Host start signal ---- */
    dht22_pin_output();
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_RESET);
    delay_us(DHT22_START_LOW_US);  /* Hold low for ≥1ms */
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
    delay_us(DHT22_START_HIGH_US); /* Release for ~30us */
    dht22_pin_input();

    /* ---- DHT22 response signal ---- */
    /* Wait for DHT22 to pull low (~80us) */
    if (dht22_wait_for_state(GPIO_PIN_RESET, DHT22_RESPONSE_TIMEOUT_US) == UINT32_MAX) {
        return ENV_SENSOR_ERR_READ;
    }
    /* Wait for DHT22 to pull high (~80us) */
    if (dht22_wait_for_state(GPIO_PIN_SET, DHT22_RESPONSE_TIMEOUT_US) == UINT32_MAX) {
        return ENV_SENSOR_ERR_READ;
    }
    /* Wait for DHT22 to pull low again (start of first data bit) */
    if (dht22_wait_for_state(GPIO_PIN_RESET, DHT22_RESPONSE_TIMEOUT_US) == UINT32_MAX) {
        return ENV_SENSOR_ERR_READ;
    }

    /* ---- Read 40 data bits ---- */
    for (int i = 0; i < 40; i++) {
        /* Wait for rising edge (each bit starts with ~50us low) */
        if (dht22_wait_for_state(GPIO_PIN_SET, DHT22_RESPONSE_TIMEOUT_US) == UINT32_MAX) {
            return ENV_SENSOR_ERR_READ;
        }

        /* Measure high-time duration to determine bit value */
        uint32_t high_us = dht22_wait_for_state(GPIO_PIN_RESET, DHT22_RESPONSE_TIMEOUT_US);
        if (high_us == UINT32_MAX) {
            return ENV_SENSOR_ERR_READ;
        }

        /* >40us high = bit 1, <40us high = bit 0 */
        if (high_us > DHT22_BIT_THRESHOLD_US) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    /* ---- Verify checksum ---- */
    uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (checksum != data[4]) {
        return ENV_SENSOR_ERR_READ;
    }

    return ENV_SENSOR_OK;
}

/* ===== Public API ======================================================= */

/**
 * @brief  Initialize all environmental sensors.
 *
 * - Enables DWT cycle counter for microsecond timing.
 * - Enables GPIO clock for DHT22 pin.
 * - Configures and calibrates ADC for MQ-2 channel.
 *
 * @return ENV_SENSOR_OK on success, negative error code otherwise.
 */
int env_sensors_init(void)
{
    /* Enable DWT for microsecond timing */
    dwt_init();

    /* GPIO clock for DHT22 port should already be enabled by CubeMX.
     * Set pin to input (idle high via external pull-up). */
    dht22_pin_input();

    /* ADC for MQ-2 should already be initialised by CubeMX (MX_ADC1_Init).
     * We only verify the handle is non-null. */
    if (MQ2_ADC_HANDLE.Instance == NULL) {
        return ENV_SENSOR_ERR_INIT;
    }

    /* Calibrate ADC (STM32F4 does not have HAL_ADCEx_Calibration_Start,
     * but STM32F1 does — guard with ifdef) */
#if defined(STM32F1)
    HAL_ADCEx_Calibration_Start(&MQ2_ADC_HANDLE);
#endif

    g_smoke_threshold = MQ2_DEFAULT_THRESHOLD;
    g_initialized = true;

    return ENV_SENSOR_OK;
}

/**
 * @brief  Read DHT22 temperature and humidity.
 *
 * DHT22 specification:
 *   - Humidity range:    0 – 99.9 %RH
 *   - Temperature range: -40 – +80 °C
 *   - Resolution:        0.1 °C / 0.1 %RH
 *   - Minimum read interval: 2 seconds
 *
 * @param  temperature_c_out  Pointer to temperature output in °C.
 * @param  humidity_pct_out   Pointer to humidity output in %.
 * @return ENV_SENSOR_OK on success, negative error code otherwise.
 */
int env_read_dht22(float *temperature_c_out, float *humidity_pct_out)
{
    if (!g_initialized) {
        return ENV_SENSOR_ERR_INIT;
    }
    if (temperature_c_out == NULL || humidity_pct_out == NULL) {
        return ENV_SENSOR_ERR_INVALID;
    }

    uint8_t raw[5];
    int result = dht22_read_raw(raw);
    if (result != ENV_SENSOR_OK) {
        return result;
    }

    /* Parse humidity (raw[0]:raw[1]) — unsigned, 0.1 %RH resolution */
    uint16_t hum_raw = ((uint16_t)raw[0] << 8) | raw[1];
    *humidity_pct_out = hum_raw / 10.0f;

    /* Parse temperature (raw[2]:raw[3]) — MSB is sign bit, 0.1 °C resolution */
    uint16_t temp_raw = ((uint16_t)raw[2] << 8) | raw[3];
    if (temp_raw & 0x8000) {
        /* Negative temperature */
        temp_raw &= 0x7FFF;
        *temperature_c_out = -(temp_raw / 10.0f);
    } else {
        *temperature_c_out = temp_raw / 10.0f;
    }

    /* Sanity check */
    if (*temperature_c_out < ENV_SENSOR_MIN_TEMP_C ||
        *temperature_c_out > ENV_SENSOR_MAX_TEMP_C) {
        return ENV_SENSOR_ERR_READ;
    }

    return ENV_SENSOR_OK;
}

/**
 * @brief  Read raw MQ-2 smoke/gas sensor via ADC.
 *
 * Uses single-conversion mode. 12-bit ADC yields 0–4095.
 *
 * @param  smoke_raw_out  Pointer to raw ADC reading output.
 * @return ENV_SENSOR_OK on success, negative error code otherwise.
 */
int env_read_mq2(uint16_t *smoke_raw_out)
{
    if (!g_initialized) {
        return ENV_SENSOR_ERR_INIT;
    }
    if (smoke_raw_out == NULL) {
        return ENV_SENSOR_ERR_INVALID;
    }

    /* Configure ADC channel */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = MQ2_ADC_CHANNEL;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;

    if (HAL_ADC_ConfigChannel(&MQ2_ADC_HANDLE, &sConfig) != HAL_OK) {
        return ENV_SENSOR_ERR_READ;
    }

    /* Start conversion, wait for completion, read value */
    HAL_ADC_Start(&MQ2_ADC_HANDLE);

    if (HAL_ADC_PollForConversion(&MQ2_ADC_HANDLE, MQ2_ADC_TIMEOUT_MS) != HAL_OK) {
        HAL_ADC_Stop(&MQ2_ADC_HANDLE);
        return ENV_SENSOR_ERR_READ;
    }

    *smoke_raw_out = (uint16_t)HAL_ADC_GetValue(&MQ2_ADC_HANDLE);
    HAL_ADC_Stop(&MQ2_ADC_HANDLE);

    return ENV_SENSOR_OK;
}

/**
 * @brief  Read all environment sensors and fill a combined data structure.
 *
 * This is the primary function called from the main loop to populate
 * the environment_data_t struct for UART telemetry.
 *
 * @param  out_data  Pointer to caller-owned output structure.
 * @return ENV_SENSOR_OK on success, negative error code otherwise.
 */
int env_read_all(environment_data_t *out_data)
{
    if (!g_initialized) {
        return ENV_SENSOR_ERR_INIT;
    }
    if (out_data == NULL) {
        return ENV_SENSOR_ERR_INVALID;
    }

    int result;

    /* Read DHT22 */
    result = env_read_dht22(&out_data->temperature_c, &out_data->humidity_pct);
    if (result != ENV_SENSOR_OK) {
        /* On DHT22 failure, zero out fields but continue with MQ-2 */
        out_data->temperature_c = 0.0f;
        out_data->humidity_pct  = 0.0f;
    }

    /* Read MQ-2 */
    result = env_read_mq2(&out_data->smoke_raw);
    if (result != ENV_SENSOR_OK) {
        out_data->smoke_raw   = 0;
        out_data->smoke_alert = false;
    } else {
        out_data->smoke_alert = env_is_smoke_alert(out_data->smoke_raw);
    }

    /* Timestamp */
    out_data->timestamp_ms = HAL_GetTick();

    return ENV_SENSOR_OK;
}

/**
 * @brief  Set MQ-2 smoke alert threshold.
 * @param  threshold  Raw ADC threshold value (0–4095).
 */
void env_set_smoke_threshold(uint16_t threshold)
{
    g_smoke_threshold = threshold;
}

/**
 * @brief  Check whether the given MQ-2 raw value exceeds current smoke threshold.
 * @param  smoke_raw  Raw MQ-2 ADC reading.
 * @return true if smoke alert condition is detected, false otherwise.
 */
bool env_is_smoke_alert(uint16_t smoke_raw)
{
    return (smoke_raw >= g_smoke_threshold);
}
