/**
 * @file    hcsr04.c
 * @brief   HC-SR04 Ultrasonic Distance Sensor Driver Implementation
 * @author  Grup 7
 * @date    2026-05-18
 * @version 1.0
 *
 * Uses DWT cycle counter for microsecond timing (no extra timer needed).
 *
 * Pin mapping (v5.2):
 *   TRIG0 (Front) = PA5    ECHO0 = PB12
 *   TRIG1 (Back)  = PA6    ECHO1 = PB13
 *   TRIG2 (Left)  = PA8    ECHO2 = PB15
 *   TRIG3 (Right) = PA7    ECHO3 = PB14
 */

#include "hcsr04.h"
#include "stm32f1xx_hal.h"

/* ===== Pin Table ========================================================= */

typedef struct {
    GPIO_TypeDef *trig_port;
    uint16_t      trig_pin;
    GPIO_TypeDef *echo_port;
    uint16_t      echo_pin;
} hcsr04_pin_t;

static const hcsr04_pin_t s_sensors[HCSR04_COUNT] = {
    /* #0 Front  */ { GPIOA, GPIO_PIN_5,  GPIOB, GPIO_PIN_12 },
    /* #1 Back   */ { GPIOA, GPIO_PIN_6,  GPIOB, GPIO_PIN_13 },
    /* #2 Left   */ { GPIOA, GPIO_PIN_8,  GPIOB, GPIO_PIN_15 },
    /* #3 Right  */ { GPIOA, GPIO_PIN_7,  GPIOB, GPIO_PIN_14 },
};

static bool s_initialized = false;

/* ===== DWT Helpers ======================================================= */

static void dwt_enable(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT  = 0;
    DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void delay_us(uint32_t us)
{
    uint32_t start  = DWT->CYCCNT;
    uint32_t ticks  = us * (HAL_RCC_GetHCLKFreq() / 1000000U);
    while ((DWT->CYCCNT - start) < ticks);
}

/**
 * @brief Wait for pin to reach expected_state, with timeout_us limit.
 * @return Elapsed microseconds, or UINT32_MAX on timeout.
 */
static uint32_t wait_for_pin(GPIO_TypeDef *port, uint16_t pin,
                              GPIO_PinState expected_state,
                              uint32_t timeout_us)
{
    uint32_t ticks_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
    uint32_t limit        = timeout_us * ticks_per_us;
    uint32_t start        = DWT->CYCCNT;

    while (HAL_GPIO_ReadPin(port, pin) != expected_state) {
        if ((DWT->CYCCNT - start) >= limit) {
            return UINT32_MAX;
        }
    }
    return (DWT->CYCCNT - start) / ticks_per_us;
}

/* ===== Public API ======================================================== */

hcsr04_status_t hcsr04_init(void)
{
    dwt_enable();

    /* GPIO clocks already enabled by MX_GPIO_Init().
     * TRIG pins configured as output, ECHO as input in MX_GPIO_Init().
     * Just verify DWT is running. */
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        return HCSR04_ERR_INIT;
    }

    /* Drive all TRIG pins low */
    for (uint8_t i = 0; i < HCSR04_COUNT; i++) {
        HAL_GPIO_WritePin(s_sensors[i].trig_port,
                          s_sensors[i].trig_pin,
                          GPIO_PIN_RESET);
    }

    s_initialized = true;
    return HCSR04_OK;
}

hcsr04_status_t hcsr04_read_cm(uint8_t sensor_idx, uint8_t *distance_cm)
{
    if (!s_initialized || distance_cm == NULL) {
        return HCSR04_ERR_INIT;
    }
    if (sensor_idx >= HCSR04_COUNT) {
        return HCSR04_ERR_PARAM;
    }

    const hcsr04_pin_t *s = &s_sensors[sensor_idx];

    /* 1. Send 10µs TRIG pulse */
    HAL_GPIO_WritePin(s->trig_port, s->trig_pin, GPIO_PIN_RESET);
    delay_us(2);
    HAL_GPIO_WritePin(s->trig_port, s->trig_pin, GPIO_PIN_SET);
    delay_us(HCSR04_TRIG_PULSE_US);
    HAL_GPIO_WritePin(s->trig_port, s->trig_pin, GPIO_PIN_RESET);

    /* 2. Wait for ECHO to go HIGH */
    if (wait_for_pin(s->echo_port, s->echo_pin,
                     GPIO_PIN_SET, HCSR04_TIMEOUT_US) == UINT32_MAX) {
        *distance_cm = 0;
        return HCSR04_ERR_TIMEOUT;
    }

    /* 3. Measure ECHO HIGH duration */
    uint32_t high_us = wait_for_pin(s->echo_port, s->echo_pin,
                                    GPIO_PIN_RESET, HCSR04_TIMEOUT_US);

    if (high_us == UINT32_MAX) {
        *distance_cm = 0;
        return HCSR04_ERR_TIMEOUT;
    }

    /* 4. Convert: distance = echo_us / 58 (speed of sound, round trip) */
    uint32_t dist = high_us / 58U;
    if (dist > HCSR04_MAX_DIST_CM) {
        dist = HCSR04_MAX_DIST_CM;
    }
    *distance_cm = (uint8_t)dist;

    return HCSR04_OK;
}

hcsr04_status_t hcsr04_read_all(uint8_t *front_cm, uint8_t *back_cm,
                                 uint8_t *left_cm,  uint8_t *right_cm)
{
    uint8_t *outs[HCSR04_COUNT] = { front_cm, back_cm, left_cm, right_cm };

    for (uint8_t i = 0; i < HCSR04_COUNT; i++) {
        if (outs[i] == NULL) continue;
        if (hcsr04_read_cm(i, outs[i]) != HCSR04_OK) {
            *outs[i] = 0;
        }
        HAL_Delay(5); /* Small gap between sensors to avoid interference */
    }

    return HCSR04_OK;
}
