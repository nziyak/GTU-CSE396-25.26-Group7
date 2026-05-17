/**
 * @file      robot_state_machine.c
 * @brief     Main Robot Control State Machine Implementation
 * @author    Implementation Team
 * @date      2026-05-17
 * @version   1.0
 */

#include "robot_state_machine.h"
#include <string.h>

/* -- Private Variables ----------------------------------------------------- */
static robot_context_t context;
static bool initialized = false;

/* -- State Handler Function Prototypes ------------------------------------- */
static void state_init_handler(void);
static void state_idle_handler(void);
static void state_manual_control_handler(void);
static void state_auto_navigate_handler(void);
static void state_stuck_recovery_handler(void);
static void state_emergency_stop_handler(void);
static void state_return_home_handler(void);
static void state_error_handler(void);

/* -- Helper Function Prototypes -------------------------------------------- */
static void transition_to_state(robot_state_t new_state);
static void process_uart_commands(void);
static void update_telemetry(void);
static void read_sensors(void);
static void check_stuck_condition(void);
static void check_failsafes(void);

/* -- Public Function Implementations --------------------------------------- */

int8_t robot_state_machine_init(void)
{
    if (initialized) {
        return 0;
    }

    // Clear context
    memset(&context, 0, sizeof(robot_context_t));

    // Initialize all subsystems
    motor_status_t motor_status = motor_control_init();
    if (motor_status != MOTOR_OK) {
        context.error_code = 1;
        return -1;
    }

    stuck_status_t stuck_status = stuck_detection_init();
    if (stuck_status != STUCK_OK) {
        context.error_code = 2;
        return -2;
    }

    int8_t uart_status = uart_comm_init();
    if (uart_status != 0) {
        context.error_code = 3;
        return -3;
    }

    pwr_status_t pwr_status = pwr_init();
    if (pwr_status != PWR_STATUS_OK) {
        context.error_code = 4;
        return -4;
    }

    int env_status = env_sensors_init();
    if (env_status != ENV_SENSOR_OK) {
        context.error_code = 5;
        return -5;
    }

    // Initialize state machine
    context.current_state = STATE_INIT;
    context.previous_state = STATE_INIT;
    context.pending_event = EVENT_NONE;
    context.state_entry_time = HAL_GetTick();
    context.motors_enabled = false;
    context.emergency_stop_active = false;

    // Enable power decoy for motors
    pwr_set_decoy_state(true);

    initialized = true;
    return 0;
}

void robot_state_machine_update(void)
{
    if (!initialized) {
        return;
    }

    uint32_t current_time = HAL_GetTick();

    // Check for UART commands
    process_uart_commands();

    // Periodic sensor reading
    if (current_time - context.last_sensor_read_time >= SENSOR_READ_RATE_MS) {
        read_sensors();
        context.last_sensor_read_time = current_time;
    }

    // Periodic stuck detection check
    if (current_time - context.last_stuck_check_time >= STUCK_CHECK_RATE_MS) {
        check_stuck_condition();
        context.last_stuck_check_time = current_time;
    }

    // Check critical failsafes
    check_failsafes();

    // Execute current state handler
    switch (context.current_state) {
        case STATE_INIT:
            state_init_handler();
            break;

        case STATE_IDLE:
            state_idle_handler();
            break;

        case STATE_MANUAL_CONTROL:
            state_manual_control_handler();
            break;

        case STATE_AUTO_NAVIGATE:
            state_auto_navigate_handler();
            break;

        case STATE_STUCK_RECOVERY:
            state_stuck_recovery_handler();
            break;

        case STATE_EMERGENCY_STOP:
            state_emergency_stop_handler();
            break;

        case STATE_RETURN_HOME:
            state_return_home_handler();
            break;

        case STATE_ERROR:
            state_error_handler();
            break;

        default:
            transition_to_state(STATE_ERROR);
            break;
    }

    // Periodic telemetry transmission
    if (current_time - context.last_telemetry_time >= TELEMETRY_SEND_RATE_MS) {
        update_telemetry();
        uart_send_telemetry(&context.telemetry_data);
        context.last_telemetry_time = current_time;
    }

    // Clear processed event
    context.pending_event = EVENT_NONE;
}

void robot_post_event(robot_event_t event)
{
    context.pending_event = event;
}

robot_state_t robot_get_state(void)
{
    return context.current_state;
}

void robot_force_state(robot_state_t new_state)
{
    transition_to_state(new_state);
}

const robot_context_t* robot_get_context(void)
{
    return &context;
}

void robot_set_motors_enabled(bool enable)
{
    context.motors_enabled = enable;
    if (!enable) {
        motor_emergency_stop();
    }
}

/* -- State Handler Implementations ----------------------------------------- */

static void state_init_handler(void)
{
    // Initialization complete, move to idle
    transition_to_state(STATE_IDLE);
}

static void state_idle_handler(void)
{
    // Wait for commands or events
    
    if (context.pending_event == EVENT_UART_COMMAND) {
        if (context.last_command.direction != UART_DIR_STOP) {
            transition_to_state(STATE_MANUAL_CONTROL);
        }
    }
    
    if (context.pending_event == EVENT_EMERGENCY_BUTTON || 
        context.pending_event == EVENT_SMOKE_ALERT) {
        transition_to_state(STATE_EMERGENCY_STOP);
    }
    
    if (context.pending_event == EVENT_TIME_LIMIT) {
        transition_to_state(STATE_RETURN_HOME);
    }
}

static void state_manual_control_handler(void)
{
    // Execute motor commands from UART
    
    if (context.motors_enabled && !context.emergency_stop_active) {
        motor_set_state(
            context.last_command.direction,
            context.last_command.speed_pwm
        );
    }
    
    // Check for stuck condition
    if (context.pending_event == EVENT_STUCK_DETECTED) {
        transition_to_state(STATE_STUCK_RECOVERY);
        return;
    }
    
    // Check for emergency conditions
    if (context.pending_event == EVENT_EMERGENCY_BUTTON || 
        context.pending_event == EVENT_SMOKE_ALERT ||
        context.pending_event == EVENT_POWER_FAIL) {
        transition_to_state(STATE_EMERGENCY_STOP);
        return;
    }
    
    // Check for stop command
    if (context.pending_event == EVENT_UART_COMMAND && 
        context.last_command.direction == UART_DIR_STOP) {
        transition_to_state(STATE_IDLE);
        return;
    }
    
    // Check time limit
    if (context.pending_event == EVENT_TIME_LIMIT) {
        transition_to_state(STATE_RETURN_HOME);
        return;
    }
}

static void state_auto_navigate_handler(void)
{
    // Autonomous navigation (placeholder for future implementation)
    // Would integrate with path planning and obstacle avoidance
    
    // For now, treat like manual control
    state_manual_control_handler();
}

static void state_stuck_recovery_handler(void)
{
    static uint32_t recovery_start_time = 0;
    static bool recovery_executed = false;
    
    uint32_t current_time = HAL_GetTick();
    
    // On entry, determine recovery action
    if (current_time - context.state_entry_time < 100 && !recovery_executed) {
        context.recovery_action = stuck_get_recovery_action(&context.stuck_result);
        stuck_execute_recovery(context.recovery_action);
        recovery_start_time = current_time;
        recovery_executed = true;
    }
    
    // Wait for recovery to complete
    uint32_t recovery_duration = current_time - recovery_start_time;
    
    if (recovery_duration > STUCK_RECOVERY_BACKOFF_MS + 1000) {
        // Recovery complete
        recovery_executed = false;
        stuck_reset();
        
        if (context.recovery_action == RECOVERY_EMERGENCY) {
            transition_to_state(STATE_EMERGENCY_STOP);
        } else {
            robot_post_event(EVENT_RECOVERY_COMPLETE);
            transition_to_state(STATE_IDLE);
        }
    }
    
    // Allow emergency override
    if (context.pending_event == EVENT_EMERGENCY_BUTTON) {
        recovery_executed = false;
        transition_to_state(STATE_EMERGENCY_STOP);
    }
}

static void state_emergency_stop_handler(void)
{
    // Execute emergency stop
    motor_emergency_stop();
    context.emergency_stop_active = true;
    context.motors_enabled = false;
    
    // Hold in emergency state until manual reset
    // In a real system, this would require operator intervention
    
    // For simulation, allow transition after 5 seconds
    if (HAL_GetTick() - context.state_entry_time > 5000) {
        context.emergency_stop_active = false;
        transition_to_state(STATE_IDLE);
    }
}

static void state_return_home_handler(void)
{
    // Return to home functionality
    // This would use GPS or dead reckoning in a real system
    
    // Simple implementation: stop and wait
    motor_soft_stop(1000);
    
    // Simulate RTH completion after 10 seconds
    if (HAL_GetTick() - context.state_entry_time > 10000) {
        robot_post_event(EVENT_HOME_REACHED);
        transition_to_state(STATE_IDLE);
    }
}

static void state_error_handler(void)
{
    // Error state - all motors stopped
    motor_emergency_stop();
    context.motors_enabled = false;
    
    // Flash error code or send error telemetry
    // In production, would log error details
    
    // Allow manual reset after 3 seconds
    if (HAL_GetTick() - context.state_entry_time > 3000) {
        if (context.pending_event == EVENT_UART_COMMAND) {
            context.error_code = 0;
            transition_to_state(STATE_IDLE);
        }
    }
}

/* -- Helper Function Implementations --------------------------------------- */

static void transition_to_state(robot_state_t new_state)
{
    if (new_state == context.current_state) {
        return; // No change
    }

    // Exit current state (cleanup actions)
    switch (context.current_state) {
        case STATE_MANUAL_CONTROL:
        case STATE_AUTO_NAVIGATE:
            // Stop motors when leaving navigation states
            motor_soft_stop(500);
            break;

        default:
            break;
    }

    // Update state
    context.previous_state = context.current_state;
    context.current_state = new_state;
    context.state_entry_time = HAL_GetTick();

    // Entry actions for new state
    switch (new_state) {
        case STATE_IDLE:
            context.motors_enabled = true; // Allow motor commands
            break;

        case STATE_MANUAL_CONTROL:
            context.motors_enabled = true;
            break;

        case STATE_STUCK_RECOVERY:
            // Prepare for recovery
            break;

        case STATE_EMERGENCY_STOP:
            motor_emergency_stop();
            context.motors_enabled = false;
            break;

        default:
            break;
    }
}

static void process_uart_commands(void)
{
    uart_command_t new_command;
    
    if (uart_receive_command(&new_command)) {
        // Store command
        context.last_command = new_command;
        
        // Post event
        robot_post_event(EVENT_UART_COMMAND);
        
        // Handle buzzer and lights (immediate actions)
        if (new_command.buzzer_on) {
            // Activate buzzer GPIO
            // HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
        }
        
        if (new_command.lights_on) {
            // Activate lights GPIO
            // HAL_GPIO_WritePin(LIGHTS_GPIO_Port, LIGHTS_Pin, GPIO_PIN_SET);
        }
    }
}

static void update_telemetry(void)
{
    // Populate telemetry structure with latest data
    context.telemetry_data.temperature = context.env_data.temperature_c;
    context.telemetry_data.smoke_detected = context.env_data.smoke_alert;
    context.telemetry_data.is_stuck = context.stuck_result.is_stuck;
    context.telemetry_data.imu_yaw_angle = imu_get_yaw_angle();
    
    // Ultrasonic sensors would be read here
    // context.telemetry_data.us_dist_front = read_ultrasonic_front();
    // For now, use placeholder values
    context.telemetry_data.us_dist_front = 100;
    context.telemetry_data.us_dist_back = 100;
    context.telemetry_data.us_dist_left = 100;
    context.telemetry_data.us_dist_right = 100;
    
    // Acoustic angle would come from acoustic module
    context.telemetry_data.acoustic_angle = 0.0f;
}

static void read_sensors(void)
{
    // Read environmental sensors
    int status = env_read_all(&context.env_data);
    
    if (status == ENV_SENSOR_OK) {
        // Check for smoke alert
        if (context.env_data.smoke_alert) {
            robot_post_event(EVENT_SMOKE_ALERT);
        }
    }
}

static void check_stuck_condition(void)
{
    // Only check stuck if motors are active
    bool motors_active = (context.current_state == STATE_MANUAL_CONTROL || 
                         context.current_state == STATE_AUTO_NAVIGATE) &&
                         context.motors_enabled;
    
    if (motors_active) {
        stuck_status_t status = stuck_check(motors_active, &context.stuck_result);
        
        if (status == STUCK_OK && context.stuck_result.is_stuck) {
            robot_post_event(EVENT_STUCK_DETECTED);
        }
    }
}

static void check_failsafes(void)
{
    // Check operation time limit
    if (pwr_is_time_limit_exceeded()) {
        robot_post_event(EVENT_TIME_LIMIT);
    }
    
    // Check power status (if implemented in pwr_management)
    // pwr_status_t pwr_status = pwr_check_status();
    // if (pwr_status != PWR_STATUS_OK) {
    //     robot_post_event(EVENT_POWER_FAIL);
    // }
}
