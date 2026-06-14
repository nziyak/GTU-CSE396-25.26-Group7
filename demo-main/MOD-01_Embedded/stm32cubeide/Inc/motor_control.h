#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

/**
 * @file      motor_control.h
 * @brief     L298N Motor Driver Control Interface for 4WD Chassis
 * @author    Ömer 210104004027
 * @date      2026-03-29
 * @version   1.0
 *
 * Changelog:
 *   v0.1 (2026-03-29) - Initial draft.
 *   v1.0 (2026-05-18) - Updated for HardwareScheme v5.2.
 *                       Added MotorConfig_t / MotorControl_t object-oriented API.
 *                       Pin assignments now dynamic via MotorConfig_t (no hardcoded defines).
 *
 * Hardware (v5.2):
 *   IN1 = PA11, IN2 = PA12  (Left  motor group — Motor A)
 *   IN3 = PB10, IN4 = PB0   (Right motor group — Motor B)
 *   ENA = PB8  (TIM4_CH3 PWM)
 *   ENB = PB9  (TIM4_CH4 PWM)
 */

#include <stdint.h>
#include <stdbool.h>
#include "main.h"   /* HAL types, GPIO_PIN_*, TIM_HandleTypeDef */

/* -- PWM Configuration ---------------------------------------------------- */
#define MOTOR_PWM_FREQUENCY_HZ    1000     /**< PWM frequency (1 kHz) */
#define MOTOR_SPEED_DEFAULT       70       /**< Default cruising speed (70%) */
#define MOTOR_TURN_SPEED_RATIO    0.7f     /**< Speed reduction factor for turns */

/* -- Data Types ------------------------------------------------------------ */

/** @brief High-level movement commands */
typedef enum {
    MOTOR_CMD_STOP     = 0,
    MOTOR_CMD_FORWARD  = 1,
    MOTOR_CMD_BACKWARD = 2,
    MOTOR_CMD_LEFT     = 3,
    MOTOR_CMD_RIGHT    = 4
} MotorCommand_t;

/** @brief Motor subsystem status */
typedef enum {
    MOTOR_STATUS_IDLE    = 0,
    MOTOR_STATUS_RUNNING = 1,
    MOTOR_STATUS_ERROR   = 2
} MotorStatus_t;

/**
 * @brief Hardware pin/timer configuration for L298N driver.
 *        Populated once in System_Init(), then passed to Motor_Init().
 */
typedef struct {
    GPIO_TypeDef      *motorA_IN1_port;     /**< Left  IN1 port (PA) */
    uint16_t           motorA_IN1_pin;      /**< Left  IN1 pin  (GPIO_PIN_11) */
    GPIO_TypeDef      *motorA_IN2_port;     /**< Left  IN2 port (PA) */
    uint16_t           motorA_IN2_pin;      /**< Left  IN2 pin  (GPIO_PIN_12) */
    GPIO_TypeDef      *motorB_IN3_port;     /**< Right IN3 port (PB) */
    uint16_t           motorB_IN3_pin;      /**< Right IN3 pin  (GPIO_PIN_10) */
    GPIO_TypeDef      *motorB_IN4_port;     /**< Right IN4 port (PB) */
    uint16_t           motorB_IN4_pin;      /**< Right IN4 pin  (GPIO_PIN_0)  */
    TIM_HandleTypeDef *pwm_timer;           /**< &htim4 */
    uint32_t           motorA_pwm_channel;  /**< TIM_CHANNEL_3 (ENA — PB8) */
    uint32_t           motorB_pwm_channel;  /**< TIM_CHANNEL_4 (ENB — PB9) */
} MotorConfig_t;

/** @brief Motor control instance — one global g_motor in main.c */
typedef struct {
    MotorConfig_t   config;
    MotorCommand_t  current_command;
    MotorStatus_t   status;
    uint8_t         speed_percent;      /**< 0-100 */
    uint32_t        last_command_time;  /**< HAL_GetTick() at last command */
} MotorControl_t;

/* Legacy status codes (kept for backward compatibility) */
typedef enum {
    MOTOR_OK              =  0,
    MOTOR_ERR_INIT        = -1,
    MOTOR_ERR_INVALID_DIR = -2,
    MOTOR_ERR_INVALID_SPD = -3,
    MOTOR_ERR_PWM_FAIL    = -4
} motor_status_t;

/* -- Public API ------------------------------------------------------------ */

HAL_StatusTypeDef Motor_Init(MotorControl_t *motor, const MotorConfig_t *config);
HAL_StatusTypeDef Motor_ExecuteCommand(MotorControl_t *motor, MotorCommand_t command);
HAL_StatusTypeDef Motor_SetSpeed(MotorControl_t *motor, uint8_t speed_percent);
void              Motor_EmergencyStop(MotorControl_t *motor);
MotorStatus_t     Motor_GetStatus(const MotorControl_t *motor);
bool              Motor_IsActive(const MotorControl_t *motor);
MotorCommand_t    Motor_GetCurrentCommand(const MotorControl_t *motor);

#endif /* MOTOR_CONTROL_H */
