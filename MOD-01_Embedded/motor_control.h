#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

/**
 * @file      motor_control.h
 * @brief     L298N Motor Driver Control Interface for 4WD Chassis
 * @author    Ömer 210104004027
 * @date      2026-03-29
 * @version   0.1
 * 
 * Changelog:
 * v0.1 (2026-03-29) - Initial draft, defined motor control functions and PWM interface.
 * 
 * Purpose:
 * This module controls the L298N H-Bridge motor driver to manage the 4WD chassis.
 * It receives commands from the UART interface (uart_comm.h) and translates them
 * into PWM signals and direction control for the DC motors.
 * 
 * Hardware Dependencies:
 * - L298N Dual H-Bridge Motor Driver
 * - 4x DC Motors (4WD Chassis)
 * - STM32F103C8T6 PWM timers (TIM2, TIM3)
 * - 12V power from PD Powerbank with Type-C Decoy
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros ---------------------------------------------------- */

/** L298N Pin Definitions (STM32 GPIO) */
#define MOTOR_LEFT_FRONT_IN1      GPIOA_PIN_0   /**< Left front motor direction 1 */
#define MOTOR_LEFT_FRONT_IN2      GPIOA_PIN_1   /**< Left front motor direction 2 */
#define MOTOR_LEFT_FRONT_EN       GPIOA_PIN_2   /**< Left front motor PWM enable (TIM2_CH3) */

#define MOTOR_LEFT_BACK_IN3       GPIOA_PIN_3   /**< Left back motor direction 1 */
#define MOTOR_LEFT_BACK_IN4       GPIOA_PIN_4   /**< Left back motor direction 2 */
#define MOTOR_LEFT_BACK_EN        GPIOA_PIN_5   /**< Left back motor PWM enable (TIM2_CH1) */

#define MOTOR_RIGHT_FRONT_IN1     GPIOB_PIN_0   /**< Right front motor direction 1 */
#define MOTOR_RIGHT_FRONT_IN2     GPIOB_PIN_1   /**< Right front motor direction 2 */
#define MOTOR_RIGHT_FRONT_EN      GPIOB_PIN_10  /**< Right front motor PWM enable (TIM2_CH3) */

#define MOTOR_RIGHT_BACK_IN3      GPIOB_PIN_3   /**< Right back motor direction 1 */
#define MOTOR_RIGHT_BACK_IN4      GPIOB_PIN_4   /**< Right back motor direction 2 */
#define MOTOR_RIGHT_BACK_EN       GPIOB_PIN_5   /**< Right back motor PWM enable (TIM3_CH2) */

/** PWM Configuration */
#define MOTOR_PWM_FREQUENCY_HZ    1000          /**< PWM frequency (1 kHz) */
#define MOTOR_PWM_MAX_DUTY        255           /**< Maximum PWM duty cycle (8-bit) */
#define MOTOR_PWM_MIN_DUTY        0             /**< Minimum PWM duty cycle */

/** Speed Limits */
#define MOTOR_SPEED_MAX           255           /**< Maximum motor speed */
#define MOTOR_SPEED_MIN           0             /**< Minimum motor speed (stop) */
#define MOTOR_SPEED_DEFAULT       128           /**< Default cruising speed (50%) */

/** Turning Configuration */
#define MOTOR_TURN_SPEED_RATIO    0.7f          /**< Speed reduction for turning (70%) */

/* -- Data Types ------------------------------------------------------------ */

/**
 * @brief Motor control status codes
 */
typedef enum {
    MOTOR_OK              =  0,    /**< Operation successful */
    MOTOR_ERR_INIT        = -1,    /**< Initialization failed */
    MOTOR_ERR_INVALID_DIR = -2,    /**< Invalid direction parameter */
    MOTOR_ERR_INVALID_SPD = -3,    /**< Invalid speed parameter */
    MOTOR_ERR_PWM_FAIL    = -4     /**< PWM timer configuration failed */
} motor_status_t;

/**
 * @brief Individual motor identifiers
 */
typedef enum {
    MOTOR_LEFT_FRONT  = 0,
    MOTOR_LEFT_BACK   = 1,
    MOTOR_RIGHT_FRONT = 2,
    MOTOR_RIGHT_BACK  = 3
} motor_id_t;

/**
 * @brief Motor direction for individual motor control
 */
typedef enum {
    MOTOR_DIRECTION_FORWARD  = 0,
    MOTOR_DIRECTION_BACKWARD = 1,
    MOTOR_DIRECTION_BRAKE    = 2,
    MOTOR_DIRECTION_COAST    = 3   /**< Free-wheeling stop */
} motor_direction_t;

/* -- Public Functions ------------------------------------------------------ */

/**
 * @brief  Initializes the motor control subsystem.
 *         Configures GPIO pins, PWM timers, and sets motors to idle state.
 * @return MOTOR_OK on success, negative error code otherwise.
 */
motor_status_t motor_control_init(void);

/**
 * @brief  Sets the robot's movement state based on UART command.
 *         This is the main function called by the UART receive handler.
 * @param  direction  Target direction from uart_direction_t (0=STOP, 1=FORWARD, 2=BACKWARD, 3=LEFT, 4=RIGHT)
 * @param  speed_pwm  Motor speed PWM value (0-255)
 * @return MOTOR_OK on success, negative error code otherwise.
 * 
 * @note This function maps to uart_comm.h uart_direction_t enum:
 *       - UART_DIR_STOP (0)     -> All motors stop
 *       - UART_DIR_FORWARD (1)  -> All motors forward
 *       - UART_DIR_BACKWARD (2) -> All motors backward
 *       - UART_DIR_LEFT (3)     -> Left motors backward, right motors forward
 *       - UART_DIR_RIGHT (4)    -> Left motors forward, right motors backward
 */
motor_status_t motor_set_state(uint8_t direction, uint8_t speed_pwm);

/**
 * @brief  Controls a single motor's direction and speed.
 * @param  motor    Motor identifier (MOTOR_LEFT_FRONT, etc.)
 * @param  dir      Direction (FORWARD, BACKWARD, BRAKE, COAST)
 * @param  speed    PWM duty cycle (0-255)
 * @return MOTOR_OK on success, negative error code otherwise.
 */
motor_status_t motor_set_single(motor_id_t motor, motor_direction_t dir, uint8_t speed);

/**
 * @brief  Emergency stop - immediately halts all motors.
 *         Sets all motors to BRAKE mode with PWM = 0.
 */
void motor_emergency_stop(void);

/**
 * @brief  Soft stop - gradually reduces speed to zero.
 * @param  decel_time_ms  Time in milliseconds to decelerate (0 = immediate stop)
 */
void motor_soft_stop(uint16_t decel_time_ms);

/**
 * @brief  Gets the current PWM duty cycle of a specific motor.
 * @param  motor  Motor identifier
 * @return Current PWM value (0-255), or 0 if motor ID is invalid.
 */
uint8_t motor_get_current_speed(motor_id_t motor);

/**
 * @brief  Calibrates motor speeds to compensate for mechanical differences.
 *         Adjusts PWM values to ensure straight-line movement.
 * @param  left_trim   Trim adjustment for left motors (-50 to +50)
 * @param  right_trim  Trim adjustment for right motors (-50 to +50)
 */
void motor_set_trim(int8_t left_trim, int8_t right_trim);

/**
 * @brief  Test function - runs all motors in sequence for hardware verification.
 * @param  test_speed  Speed to use during test (0-255)
 * @param  duration_ms Duration for each motor test in milliseconds
 */
void motor_run_test_sequence(uint8_t test_speed, uint16_t duration_ms);

#endif /* MOTOR_CONTROL_H */
