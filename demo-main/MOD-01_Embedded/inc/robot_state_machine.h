#ifndef ROBOT_STATE_MACHINE_H
#define ROBOT_STATE_MACHINE_H

/**
 * @file      robot_state_machine.h
 * @brief     Main Robot Control State Machine
 * @author    Grup 7
 * @date      2026-05-18
 * @version   1.0
 *
 * Purpose:
 *   Central state machine coordinating all MOD-01 subsystems.
 *   State machine logic is implemented inside main.c (not a separate .c)
 *   to save Flash on the STM32F103C6 (32KB limit).
 *
 * State Flow:
 *   INIT → IDLE → MANUAL_CONTROL / AUTO_NAVIGATE
 *                     ↓ (stuck)
 *               STUCK_RECOVERY → IDLE
 *                     ↓ (critical)
 *               EMERGENCY_STOP
 *                     ↓ (time/power)
 *               RETURN_HOME → IDLE
 */

#include <stdint.h>
#include <stdbool.h>
#include "motor_control.h"
#include "stuck_detection.h"
#include "uart_comm.h"
#include "pwr_management.h"
#include "environment_sensors.h"
#include "hcsr04.h"

/* -- States ---------------------------------------------------------------- */
typedef enum {
    STATE_INIT           = 0,
    STATE_IDLE           = 1,
    STATE_MANUAL_CONTROL = 2,
    STATE_AUTO_NAVIGATE  = 3,
    STATE_STUCK_RECOVERY = 4,
    STATE_EMERGENCY_STOP = 5,
    STATE_RETURN_HOME    = 6,
    STATE_ERROR          = 7
} robot_state_t;

/* -- Events ---------------------------------------------------------------- */
typedef enum {
    EVENT_NONE             = 0,
    EVENT_UART_COMMAND     = 1,
    EVENT_STUCK_DETECTED   = 2,
    EVENT_RECOVERY_COMPLETE= 3,
    EVENT_SMOKE_ALERT      = 4,
    EVENT_TIME_LIMIT       = 5,
    EVENT_POWER_FAIL       = 6,
    EVENT_EMERGENCY_BUTTON = 7,
    EVENT_HOME_REACHED     = 8
} robot_event_t;

/* -- Timing (ms) ----------------------------------------------------------- */
#define TICK_STUCK_DETECTION_MS    100   /**< 10 Hz stuck check */
#define TICK_ENV_SENSOR_MS         2000  /**< 0.5 Hz env sensors (DHT11 limit) */
#define TICK_TELEMETRY_TX_MS       500   /**< 2 Hz telemetry to Pi */
#define TICK_POWER_CHECK_MS        1000  /**< 1 Hz power failsafe */
#define TICK_HCSR04_MS             100   /**< 10 Hz distance sensing */

/* -- Context --------------------------------------------------------------- */
typedef struct {
    robot_state_t    current_state;
    robot_state_t    previous_state;
    robot_event_t    pending_event;

    uint32_t         state_entry_time;
    uint32_t         last_telemetry_time;
    uint32_t         last_sensor_read_time;
    uint32_t         last_stuck_check_time;
    uint32_t         last_hcsr04_time;

    uart_command_t   last_command;
    uart_telemetry_t telemetry_data;

    StuckDetection_t stuck_detector;
    environment_data_t env_data;

    bool             motors_enabled;
    bool             emergency_stop_active;
    uint8_t          error_code;
} robot_context_t;

#endif /* ROBOT_STATE_MACHINE_H */
