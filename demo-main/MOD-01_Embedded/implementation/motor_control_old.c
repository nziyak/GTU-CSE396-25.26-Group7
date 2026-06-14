/**
 * @file motor_control.c
 * @brief L298N Motor Driver Implementation
 */

#include "motor_control.h"
#include <string.h>

/* L298N Truth Table for Differential Drive:
 * 
 * Motor A (Left):  IN1 | IN2 | Action
 *                  ----+-----+--------
 *                   1  |  0  | Forward
 *                   0  |  1  | Backward
 *                   0  |  0  | Stop
 *                   1  |  1  | Brake
 * 
 * Motor B (Right): IN3 | IN4 | Action
 *                  ----+-----+--------
 *                   1  |  0  | Forward
 *                   0  |  1  | Backward
 *                   0  |  0  | Stop
 *                   1  |  1  | Brake
 */

/* Internal helper functions */
static void Motor_SetPins(MotorControl_t *motor, 
                          GPIO_PinState in1, GPIO_PinState in2,
                          GPIO_PinState in3, GPIO_PinState in4);
static void Motor_UpdatePWM(MotorControl_t *motor);

/**
 * @brief Initialize motor control system
 */
HAL_StatusTypeDef Motor_Init(MotorControl_t *motor, const MotorConfig_t *config)
{
    if (!motor || !config) {
        return HAL_ERROR;
    }
    
    // Copy configuration
    memcpy(&motor->config, config, sizeof(MotorConfig_t));
    
    // Initialize state
    motor->current_command = MOTOR_CMD_STOP;
    motor->status = MOTOR_STATUS_IDLE;
    motor->speed_percent = 50;  // Default to 50% speed
    motor->last_command_time = HAL_GetTick();
    
    // Ensure motors are stopped
    Motor_EmergencyStop(motor);
    
    // Start PWM if configured
    if (motor->config.pwm_timer) {
        HAL_TIM_PWM_Start(motor->config.pwm_timer, motor->config.motorA_pwm_channel);
        HAL_TIM_PWM_Start(motor->config.pwm_timer, motor->config.motorB_pwm_channel);
        Motor_UpdatePWM(motor);
    }
    
    return HAL_OK;
}

/**
 * @brief Execute motor command
 */
HAL_StatusTypeDef Motor_ExecuteCommand(MotorControl_t *motor, MotorCommand_t command)
{
    if (!motor) {
        return HAL_ERROR;
    }
    
    motor->current_command = command;
    motor->last_command_time = HAL_GetTick();
    
    switch (command) {
        case MOTOR_CMD_FORWARD:
            // Both motors forward
            Motor_SetPins(motor, 
                         GPIO_PIN_SET, GPIO_PIN_RESET,    // Motor A forward
                         GPIO_PIN_SET, GPIO_PIN_RESET);   // Motor B forward
            motor->status = MOTOR_STATUS_RUNNING;
            break;
            
        case MOTOR_CMD_BACKWARD:
            // Both motors backward
            Motor_SetPins(motor,
                         GPIO_PIN_RESET, GPIO_PIN_SET,    // Motor A backward
                         GPIO_PIN_RESET, GPIO_PIN_SET);   // Motor B backward
            motor->status = MOTOR_STATUS_RUNNING;
            break;
            
        case MOTOR_CMD_LEFT:
            // Left motor backward, right motor forward (pivot left)
            Motor_SetPins(motor,
                         GPIO_PIN_RESET, GPIO_PIN_SET,    // Motor A backward
                         GPIO_PIN_SET, GPIO_PIN_RESET);   // Motor B forward
            motor->status = MOTOR_STATUS_RUNNING;
            break;
            
        case MOTOR_CMD_RIGHT:
            // Left motor forward, right motor backward (pivot right)
            Motor_SetPins(motor,
                         GPIO_PIN_SET, GPIO_PIN_RESET,    // Motor A forward
                         GPIO_PIN_RESET, GPIO_PIN_SET);   // Motor B backward
            motor->status = MOTOR_STATUS_RUNNING;
            break;
            
        case MOTOR_CMD_STOP:
        default:
            // All pins low = coast stop
            Motor_SetPins(motor,
                         GPIO_PIN_RESET, GPIO_PIN_RESET,
                         GPIO_PIN_RESET, GPIO_PIN_RESET);
            motor->status = MOTOR_STATUS_IDLE;
            break;
    }
    
    return HAL_OK;
}

/**
 * @brief Set motor speed via PWM
 */
HAL_StatusTypeDef Motor_SetSpeed(MotorControl_t *motor, uint8_t speed_percent)
{
    if (!motor) {
        return HAL_ERROR;
    }
    
    // Clamp to 0-100%
    motor->speed_percent = (speed_percent > 100) ? 100 : speed_percent;
    
    // Update PWM duty cycle
    Motor_UpdatePWM(motor);
    
    return HAL_OK;
}

/**
 * @brief Emergency stop
 */
void Motor_EmergencyStop(MotorControl_t *motor)
{
    if (!motor) {
        return;
    }
    
    // Active brake: all pins high
    Motor_SetPins(motor,
                 GPIO_PIN_SET, GPIO_PIN_SET,
                 GPIO_PIN_SET, GPIO_PIN_SET);
    
    motor->current_command = MOTOR_CMD_STOP;
    motor->status = MOTOR_STATUS_IDLE;
}

/**
 * @brief Get motor status
 */
MotorStatus_t Motor_GetStatus(const MotorControl_t *motor)
{
    return motor ? motor->status : MOTOR_STATUS_ERROR;
}

/**
 * @brief Check if motors are active
 */
bool Motor_IsActive(const MotorControl_t *motor)
{
    if (!motor) {
        return false;
    }
    return (motor->status == MOTOR_STATUS_RUNNING);
}

/**
 * @brief Get current command
 */
MotorCommand_t Motor_GetCurrentCommand(const MotorControl_t *motor)
{
    return motor ? motor->current_command : MOTOR_CMD_STOP;
}

/* ===== Internal Helper Functions ===== */

/**
 * @brief Set all motor control pins
 */
static void Motor_SetPins(MotorControl_t *motor,
                          GPIO_PinState in1, GPIO_PinState in2,
                          GPIO_PinState in3, GPIO_PinState in4)
{
    // Motor A (Left)
    HAL_GPIO_WritePin(motor->config.motorA_IN1_port, 
                      motor->config.motorA_IN1_pin, in1);
    HAL_GPIO_WritePin(motor->config.motorA_IN2_port,
                      motor->config.motorA_IN2_pin, in2);
    
    // Motor B (Right)
    HAL_GPIO_WritePin(motor->config.motorB_IN3_port,
                      motor->config.motorB_IN3_pin, in3);
    HAL_GPIO_WritePin(motor->config.motorB_IN4_port,
                      motor->config.motorB_IN4_pin, in4);
}

/**
 * @brief Update PWM duty cycle based on speed percentage
 */
static void Motor_UpdatePWM(MotorControl_t *motor)
{
    if (!motor->config.pwm_timer) {
        return;  // PWM not configured
    }
    
    // Calculate duty cycle (assuming ARR = 999 for 0.1% resolution)
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(motor->config.pwm_timer);
    uint32_t pulse = (arr + 1) * motor->speed_percent / 100;
    
    // Set PWM duty cycle for both motors
    __HAL_TIM_SET_COMPARE(motor->config.pwm_timer, 
                          motor->config.motorA_pwm_channel, pulse);
    __HAL_TIM_SET_COMPARE(motor->config.pwm_timer,
                          motor->config.motorB_pwm_channel, pulse);
}
