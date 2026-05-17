/**
 * @file      robot_state_machine.h
 * @brief     Main Robot Control State Machine
 * @author    Implementation Team
 * @date      2026-05-17
 * @version   1.0
 * 
 * Purpose:
 * Central state machine that coordinates all robot subsystems:
 * - Motor control and navigation
 * - Stuck detection and recovery
 * - Power management and failsafes
 * - Environmental monitoring
 * - UART communication with Raspberry Pi
 * 
 * State Flow:
 * INIT -> IDLE -> MANUAL/AUTO -> STUCK_RECOVERY -> EMERGENCY -> RTH
 */

#ifndef ROBOT_STATE_MACHINE_H
#define ROBOT_STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>
#include "motor_control.h"
#include "stuck_detection.h"
#include "uart_comm.h"
#include "pwr_management.h"
#include "environment_sensors.h"

/* -- State Machine States -------------------------------------------------- */
typedef enum {
    STATE_INIT              = 0,    /**< System initialization */
    STATE_IDLE              = 1,    /**< Waiting for commands */
    STATE_MANUAL_CONTROL    = 2,    /**< Manual control via UART */
    STATE_AUTO_NAVIGATE     = 3,    /**< Autonomous navigation */
    STATE_STUCK_RECOVERY    = 4,    /**< Executing stuck recovery */
    STATE_EMERGENCY_STOP    = 5,    /**< Emergency condition */
    STATE_RETURN_HOME       = 6,    /**< Returning to home position */
    STATE_ERROR             = 7     /**< Error state */
} robot_state_t;

/* -- State Machine Events -------------------------------------------------- */
typedef enum {
    EVENT_NONE              = 0,
    EVENT_UART_COMMAND      = 1,    /**< New command from Pi */
    EVENT_STUCK_DETECTED    = 2,    /**< IMU detected stuck */
    EVENT_RECOVERY_COMPLETE = 3,    /**< Recovery maneuver done */
    EVENT_SMOKE_ALERT       = 4,    /**< Smoke sensor triggered */
    EVENT_TIME_LIMIT        = 5,    /**< Operation time exceeded */
    EVENT_POWER_FAIL        = 6,    /**< Power system failure */
    EVENT_EMERGENCY_BUTTON  = 7,    /**< Emergency stop requested */
    EVENT_HOME_REACHED      = 8     /**< RTH complete */
} robot_event_t;

/* -- State Machine Configuration ------------------------------------------- */
#define STATE_MACHINE_UPDATE_RATE_MS    50      /**< Main loop rate (20 Hz) */
#define TELEMETRY_SEND_RATE_MS          100     /**< Telemetry rate (10 Hz) */
#define SENSOR_READ_RATE_MS             200     /**< Environmental sensor rate (5 Hz) */
#define STUCK_CHECK_RATE_MS             50      /**< Stuck detection rate (20 Hz) */

/* -- State Machine Context ------------------------------------------------- */
typedef struct {
    robot_state_t current_state;
    robot_state_t previous_state;
    robot_event_t pending_event;
    
    uint32_t state_entry_time;
    uint32_t last_telemetry_time;
    uint32_t last_sensor_read_time;
    uint32_t last_stuck_check_time;
    
    uart_command_t last_command;
    uart_telemetry_t telemetry_data;
    
    stuck_detection_result_t stuck_result;
    stuck_recovery_action_t recovery_action;
    
    environment_data_t env_data;
    
    bool motors_enabled;
    bool emergency_stop_active;
    
    uint8_t error_code;
} robot_context_t;

/* -- Public Functions ------------------------------------------------------ */

/**
 * @brief  Initialize the robot state machine and all subsystems.
 * @return 0 on success, negative error code otherwise.
 */
int8_t robot_state_machine_init(void);

/**
 * @brief  Main state machine update function - call periodically in main loop.
 *         Handles state transitions, event processing, and subsystem coordination.
 */
void robot_state_machine_update(void);

/**
 * @brief  Post an event to the state machine.
 * @param  event  Event to process.
 */
void robot_post_event(robot_event_t event);

/**
 * @brief  Get the current state of the robot.
 * @return Current state.
 */
robot_state_t robot_get_state(void);

/**
 * @brief  Force a state transition (for testing/debugging).
 * @param  new_state  Target state.
 */
void robot_force_state(robot_state_t new_state);

/**
 * @brief  Get the current context (for debugging/monitoring).
 * @return Pointer to read-only context.
 */
const robot_context_t* robot_get_context(void);

/**
 * @brief  Enable or disable motor control.
 * @param  enable  True to enable motors, false to disable.
 */
void robot_set_motors_enabled(bool enable);

#endif /* ROBOT_STATE_MACHINE_H */
