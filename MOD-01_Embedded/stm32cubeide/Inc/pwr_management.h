#ifndef PWR_MANAGEMENT_H
#define PWR_MANAGEMENT_H

/**
 * @file      pwr_management.h
 * @brief     Power Management and Failsafe Driver for Dual Powerbank Architecture
 * @author    Ziya 210104004027
 * @date      2026-03-27
 * @version   1.0
 *
 * Changelog:
 *   v0.1 (2026-03-18) - Initial draft.
 *   v1.0 (2026-05-18) - Updated for HardwareScheme v5.2.
 *                       PA5 = PWR_DECOY_EN, PA6 = PWR_PI_STATUS.
 *                       Added pwr_is_pi_alive(), pwr_is_decoy_enabled(),
 *                       pwr_get_uptime_seconds() prototypes.
 *
 * Hardware (v5.2):
 *   PWR_DECOY_EN  = PA5 (GPIO Output) — 12V MOSFET gate → L298N VCC
 *   PWR_PI_STATUS = PA6 (GPIO Input, pull-down) — Pi 5V rail monitor
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants ------------------------------------------------------------- */
#define PWR_MAX_OPERATION_MINS    60    /**< 60-min RTH failsafe */

/* Abstract pin labels (concrete port/pin in pwr_management.c) */
#define PWR_DECOY_ENABLE_PIN      5     /**< PA5 — informational only */
#define PWR_PI_STATUS_PIN         6     /**< PA6 — informational only */

/* -- Status Codes ---------------------------------------------------------- */
typedef enum {
    PWR_STATUS_OK             =  0,
    PWR_STATUS_DECOY_FAIL     = -1,
    PWR_STATUS_PI_POWER_FAIL  = -2,
    PWR_STATUS_TIME_LIMIT_HIT = -3
} pwr_status_t;

/* -- Public API ------------------------------------------------------------ */

/**
 * @brief  Initialise PA5 (output) and PA6 (input pull-down).
 *         Records boot time for RTH failsafe. Decoy starts OFF.
 * @return PWR_STATUS_OK on success.
 */
pwr_status_t pwr_init(void);

/**
 * @brief  Enable or disable the 12V Decoy MOSFET (PA5).
 * @param  enable  true = 12V ON (motors can run), false = 12V OFF.
 */
void pwr_set_decoy_state(bool enable);

/**
 * @brief  Check 60-minute RTH time limit.
 * @return true if limit exceeded (trigger RTH).
 */
bool pwr_is_time_limit_exceeded(void);

/**
 * @brief  Check whether the Raspberry Pi 5V rail is alive (PA6 = HIGH).
 * @return true if Pi power OK, false if Pi lost power.
 */
bool pwr_is_pi_alive(void);

/**
 * @brief  Returns whether the 12V decoy line is currently active.
 * @return true if 12V motor power is enabled.
 */
bool pwr_is_decoy_enabled(void);

/**
 * @brief  Returns elapsed operation time in seconds since boot.
 */
uint32_t pwr_get_uptime_seconds(void);

#endif /* PWR_MANAGEMENT_H */