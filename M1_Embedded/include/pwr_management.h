#ifndef PWR_MANAGEMENT_H
#define PWR_MANAGEMENT_H

/**
 * @file      pwr_management.h
 * @brief     Power Management and Failsafe Driver for Dual Powerbank Architecture
 * @author    Ziya 210104004027
 * @date      2026-03-27
 * @version   0.1
 * * Changelog:
 * v0.1 (2026-03-18) - Initial draft, defined decoy pins and time-based failsafe constants.
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros ---------------------------------------------------- */
#define PWR_DECOY_ENABLE_PIN      GPIOA_PIN_5 /**< GPIO pin to enable 12V Decoy output */
#define PWR_PI_STATUS_PIN         GPIOA_PIN_6 /**< GPIO pin to read Pi 5 5V power status */
#define PWR_MAX_OPERATION_MINS    60          /**< Maximum operation time before forced RTH */

/* -- Data Types ------------------------------------------------------------ */
/**
 * @brief Status enumeration for power subsystem
 */
typedef enum {
    PWR_STATUS_OK             = 0,
    PWR_STATUS_DECOY_FAIL     = -1,
    PWR_STATUS_PI_POWER_FAIL  = -2,
    PWR_STATUS_TIME_LIMIT_HIT = -3
} pwr_status_t;

/* -- Public Functions ------------------------------------------------------ */
/**
 * @brief  Initialises the power management GPIO pins.
 * @return PWR_STATUS_OK on success, error code otherwise.
 */
pwr_status_t pwr_init(void);

/**
 * @brief  Enables or disables the 12V Decoy trigger for motor power.
 * @param  enable True to enable 12V output, False to cut motor power.
 */
void pwr_set_decoy_state(bool enable);

/**
 * @brief  Checks if the maximum operation time has been exceeded (Failsafe).
 * @return True if time limit is hit (trigger RTH), False otherwise.
 */
bool pwr_is_time_limit_exceeded(void);

#endif /* PWR_MANAGEMENT_H */