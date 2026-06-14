/**
 * @file    pwr_management.c
 * @brief   Power Management and Failsafe Driver Implementation
 * @author  Ziya 210104004027
 * @date    2026-05-16
 * @version 0.2
 *
 * Changelog:
 *   v0.1 - Header draft only.
 *   v0.2 - Full implementation: GPIO-based 12V decoy control, Pi status
 *          monitoring, and 60-minute time-based RTH failsafe.
 *
 * Hardware Notes:
 *   - 12V Decoy Enable: PA5 — drives a MOSFET gate or relay to connect/
 *     disconnect the 12V PD Decoy line feeding the L298N motor driver.
 *   - Pi Status Input:  PA6 — connected to a 3.3V rail indicator from
 *     the Raspberry Pi 5 to detect unexpected power loss.
 *   - Power architecture: Dual Powerbank
 *       • Bank A (5V USB-C) → Raspberry Pi 5 + STM32 logic
 *       • Bank B (12V PD Decoy) → L298N motor driver
 */

#include "pwr_management.h"
#include "stm32f4xx_hal.h"

/* ===== Pin Configuration ================================================ */
/* Remap the abstract header defines to concrete port/pin pairs.
 * Adjust these if the schematic changes. */

#define PWR_DECOY_PORT     GPIOA
#define PWR_DECOY_PIN      GPIO_PIN_5

#define PWR_PI_PORT        GPIOA
#define PWR_PI_PIN         GPIO_PIN_6

/* ===== Module-level State =============================================== */

static uint32_t g_boot_time_ms    = 0;   /* Captured at init */
static bool     g_decoy_enabled   = false;
static bool     g_initialized     = false;

/* ===== Public API ======================================================= */

/**
 * @brief  Initialises the power management GPIO pins.
 *
 * - Configures the 12V Decoy enable pin as push-pull output (initially OFF).
 * - Configures the Pi 5V status pin as input with pull-down.
 * - Records boot timestamp for the 60-min RTH failsafe.
 *
 * @return PWR_STATUS_OK on success, error code otherwise.
 */
pwr_status_t pwr_init(void)
{
    /* Enable GPIO clocks — CubeMX normally handles this, but be safe */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* ---- 12V Decoy Enable (Output) ---- */
    gpio.Pin   = PWR_DECOY_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PWR_DECOY_PORT, &gpio);

    /* Start with decoy OFF (motors unpowered until explicitly enabled) */
    HAL_GPIO_WritePin(PWR_DECOY_PORT, PWR_DECOY_PIN, GPIO_PIN_RESET);
    g_decoy_enabled = false;

    /* ---- Pi 5V Status (Input) ---- */
    gpio.Pin  = PWR_PI_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(PWR_PI_PORT, &gpio);

    /* Record boot time for RTH timeout calculation */
    g_boot_time_ms = HAL_GetTick();
    g_initialized  = true;

    return PWR_STATUS_OK;
}

/**
 * @brief  Enables or disables the 12V Decoy trigger for motor power.
 *
 * When enabled, the MOSFET/relay connects the PD Decoy powerbank's 12V
 * rail to the L298N VCC input.  When disabled, motor power is cut and
 * the robot coasts to a stop (gravity braking only).
 *
 * @param  enable  True to enable 12V output, False to cut motor power.
 */
void pwr_set_decoy_state(bool enable)
{
    if (!g_initialized) {
        return;
    }

    HAL_GPIO_WritePin(PWR_DECOY_PORT, PWR_DECOY_PIN,
                      enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
    g_decoy_enabled = enable;
}

/**
 * @brief  Checks if the maximum operation time has been exceeded (Failsafe).
 *
 * The robot must initiate Return-to-Home (RTH) after PWR_MAX_OPERATION_MINS
 * (60 minutes) of continuous operation to avoid draining the powerbanks
 * beyond safe discharge limits.
 *
 * @return True if time limit is hit (trigger RTH), False otherwise.
 */
bool pwr_is_time_limit_exceeded(void)
{
    if (!g_initialized) {
        return false;
    }

    uint32_t elapsed_ms = HAL_GetTick() - g_boot_time_ms;
    uint32_t limit_ms   = (uint32_t)PWR_MAX_OPERATION_MINS * 60U * 1000U;

    return (elapsed_ms >= limit_ms);
}

/**
 * @brief  Checks whether the Raspberry Pi 5V rail is still alive.
 *
 * If the Pi status pin reads LOW, the Pi may have crashed or lost power.
 * The STM32 should trigger a safe-stop procedure in this case.
 *
 * @return True if Pi power is OK, False if Pi power loss detected.
 */
bool pwr_is_pi_alive(void)
{
    if (!g_initialized) {
        return false;
    }

    return (HAL_GPIO_ReadPin(PWR_PI_PORT, PWR_PI_PIN) == GPIO_PIN_SET);
}

/**
 * @brief  Returns whether the 12V decoy line is currently active.
 * @return True if 12V motor power is enabled, False otherwise.
 */
bool pwr_is_decoy_enabled(void)
{
    return g_decoy_enabled;
}

/**
 * @brief  Returns elapsed operation time in seconds since boot.
 * @return Uptime in seconds.
 */
uint32_t pwr_get_uptime_seconds(void)
{
    if (!g_initialized) {
        return 0;
    }
    return (HAL_GetTick() - g_boot_time_ms) / 1000U;
}
