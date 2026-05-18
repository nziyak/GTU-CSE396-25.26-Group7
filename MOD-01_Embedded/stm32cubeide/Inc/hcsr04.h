#ifndef HCSR04_H
#define HCSR04_H

/**
 * @file      hcsr04.h
 * @brief     HC-SR04 Ultrasonic Distance Sensor Driver — 4 Sensor Array
 * @author    Grup 7
 * @date      2026-05-18
 * @version   1.0
 *
 * Hardware (v5.2):
 *   Sensor | Position | TRIG Pin | ECHO Pin | Notes
 *   -------|----------|----------|----------|---------------------------
 *     #0   |  Front   |  PA15    |  PB12    | JTAG remap required
 *     #1   |  Back    |  PB3     |  PB13    | JTAG remap required
 *     #2   |  Left    |  PA7     |  PB14    | Standard GPIO
 *     #3   |  Right   |  PA8     |  PB15    | Standard GPIO
 *
 * ECHO pins PB12-PB15 are 5V-tolerant (FT) — direct HC-SR04 connection OK.
 * A 1kΩ series resistor is recommended for additional protection.
 *
 * Timing: Uses DWT cycle counter for microsecond accuracy (no extra timer needed).
 * IMPORTANT: DWT must be enabled before first call (done in hcsr04_init()).
 *
 * CubeMX requirement: SYS → Debug = "Serial Wire" to free PA15 and PB3.
 */

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* -- Sensor Indices -------------------------------------------------------- */
#define HCSR04_FRONT    0   /**< Front  sensor */
#define HCSR04_BACK     1   /**< Back   sensor */
#define HCSR04_LEFT     2   /**< Left   sensor */
#define HCSR04_RIGHT    3   /**< Right  sensor */
#define HCSR04_COUNT    4   /**< Total sensor count */

/* -- Limits & Timing ------------------------------------------------------- */
#define HCSR04_MAX_DIST_CM    400u  /**< HC-SR04 max range (cm) */
#define HCSR04_TIMEOUT_US     25000u /**< Echo timeout (~400cm × 58µs/cm) */
#define HCSR04_TRIG_PULSE_US  10u   /**< TRIG pulse width (µs) */

/* -- Status Codes ---------------------------------------------------------- */
typedef enum {
    HCSR04_OK          =  0,
    HCSR04_ERR_TIMEOUT = -1,   /**< No echo received within timeout */
    HCSR04_ERR_INIT    = -2,   /**< DWT or GPIO init failed */
    HCSR04_ERR_PARAM   = -3    /**< Invalid sensor index */
} hcsr04_status_t;

/* -- Public API ------------------------------------------------------------ */

/**
 * @brief  Initialise DWT cycle counter and configure TRIG/ECHO GPIO pins.
 *         Must be called once before any hcsr04_read_cm() calls.
 * @return HCSR04_OK on success.
 */
hcsr04_status_t hcsr04_init(void);

/**
 * @brief  Measure distance from a single HC-SR04 sensor.
 *
 * Sends a 10µs TRIG pulse and measures ECHO pulse width via DWT.
 * Blocks until ECHO goes high then low (or timeout).
 *
 * @param  sensor_idx   Sensor index (HCSR04_FRONT..HCSR04_RIGHT)
 * @param  distance_cm  Output: measured distance in cm (capped at HCSR04_MAX_DIST_CM)
 * @return HCSR04_OK, HCSR04_ERR_TIMEOUT, or HCSR04_ERR_PARAM
 */
hcsr04_status_t hcsr04_read_cm(uint8_t sensor_idx, uint8_t *distance_cm);

/**
 * @brief  Read all 4 sensors in sequence and return distances.
 *         On timeout for any sensor, the corresponding output is set to 0.
 *
 * @param  front_cm   Output: front distance (cm)
 * @param  back_cm    Output: back  distance (cm)
 * @param  left_cm    Output: left  distance (cm)
 * @param  right_cm   Output: right distance (cm)
 * @return HCSR04_OK always (individual timeouts are handled gracefully)
 */
hcsr04_status_t hcsr04_read_all(uint8_t *front_cm, uint8_t *back_cm,
                                 uint8_t *left_cm,  uint8_t *right_cm);

#endif /* HCSR04_H */
