/**
 * @file      motor_control.c
 * @brief     L298N Motor Driver Control Implementation
 * @author    Implementation Team
 * @date      2026-05-17
 * @version   1.0
 * 
 * Features:
 * - PWM speed control with 0-100% mapping
 * - Emergency stop with active braking
 * - Soft deceleration with configurable ramp
 * - Individual motor control and trim adjustment
 */

#include "motor_control.h"
#include <string.h>

/* -- Private Constants ----------------------------------------------------- */
#define MOTOR_COUNT               4
#define PWM_RESOLUTION            1000    /**< PWM timer resolution for smooth control */
#define SOFT_STOP_STEP_MS         20      /**< Update interval for soft stop */

/* -- Private Types --------------------------------------------------------- */
typedef struct {
    GPIO_TypeDef* in1_port;
    uint16_t      in1_pin;
    GPIO_TypeDef* in2_port;
    uint16_t      in2_pin;
    TIM_TypeDef*  pwm_timer;
    uint32_t      pwm_channel;
    uint8_t       current_speed;
    motor_direction_t current_dir;
    int8_t        trim_adjustment;
} motor_instance_t;

/* -- Private Variables ----------------------------------------------------- */
static motor_instance_t motors[MOTOR_COUNT];
static bool initialized = false;

/* -- Private Function Prototypes ------------------------------------------- */
static void motor_apply_pwm(motor_id_t motor, uint8_t speed);
static void motor_apply_direction(motor_id_t motor, motor_direction_t dir);
static uint8_t motor_apply_trim(motor_id_t motor, uint8_t speed);
static void motor_gpio_init(void);
static void motor_pwm_init(void);

/* -- Public Function Implementations --------------------------------------- */

motor_status_t motor_control_init(void)
{
    if (initialized) {
        return MOTOR_OK;
    }

    // Initialize motor structures
    // Left Front Motor
    motors[MOTOR_LEFT_FRONT].in1_port = GPIOA;
    motors[MOTOR_LEFT_FRONT].in1_pin = MOTOR_LEFT_FRONT_IN1;
    motors[MOTOR_LEFT_FRONT].in2_port = GPIOA;
    motors[MOTOR_LEFT_FRONT].in2_pin = MOTOR_LEFT_FRONT_IN2;
    motors[MOTOR_LEFT_FRONT].pwm_timer = TIM2;
    motors[MOTOR_LEFT_FRONT].pwm_channel = 3;
    motors[MOTOR_LEFT_FRONT].current_speed = 0;
    motors[MOTOR_LEFT_FRONT].current_dir = MOTOR_DIRECTION_BRAKE;
    motors[MOTOR_LEFT_FRONT].trim_adjustment = 0;

    // Left Back Motor
    motors[MOTOR_LEFT_BACK].in1_port = GPIOA;
    motors[MOTOR_LEFT_BACK].in1_pin = MOTOR_LEFT_BACK_IN3;
    motors[MOTOR_LEFT_BACK].in2_port = GPIOA;
    motors[MOTOR_LEFT_BACK].in2_pin = MOTOR_LEFT_BACK_IN4;
    motors[MOTOR_LEFT_BACK].pwm_timer = TIM2;
    motors[MOTOR_LEFT_BACK].pwm_channel = 1;
    motors[MOTOR_LEFT_BACK].current_speed = 0;
    motors[MOTOR_LEFT_BACK].current_dir = MOTOR_DIRECTION_BRAKE;
    motors[MOTOR_LEFT_BACK].trim_adjustment = 0;

    // Right Front Motor
    motors[MOTOR_RIGHT_FRONT].in1_port = GPIOB;
    motors[MOTOR_RIGHT_FRONT].in1_pin = MOTOR_RIGHT_FRONT_IN1;
    motors[MOTOR_RIGHT_FRONT].in2_port = GPIOB;
    motors[MOTOR_RIGHT_FRONT].in2_pin = MOTOR_RIGHT_FRONT_IN2;
    motors[MOTOR_RIGHT_FRONT].pwm_timer = TIM2;
    motors[MOTOR_RIGHT_FRONT].pwm_channel = 3;
    motors[MOTOR_RIGHT_FRONT].current_speed = 0;
    motors[MOTOR_RIGHT_FRONT].current_dir = MOTOR_DIRECTION_BRAKE;
    motors[MOTOR_RIGHT_FRONT].trim_adjustment = 0;

    // Right Back Motor
    motors[MOTOR_RIGHT_BACK].in1_port = GPIOB;
    motors[MOTOR_RIGHT_BACK].in1_pin = MOTOR_RIGHT_BACK_IN3;
    motors[MOTOR_RIGHT_BACK].in2_port = GPIOB;
    motors[MOTOR_RIGHT_BACK].in2_pin = MOTOR_RIGHT_BACK_IN4;
    motors[MOTOR_RIGHT_BACK].pwm_timer = TIM3;
    motors[MOTOR_RIGHT_BACK].pwm_channel = 2;
    motors[MOTOR_RIGHT_BACK].current_speed = 0;
    motors[MOTOR_RIGHT_BACK].current_dir = MOTOR_DIRECTION_BRAKE;
    motors[MOTOR_RIGHT_BACK].trim_adjustment = 0;

    // Initialize hardware
    motor_gpio_init();
    motor_pwm_init();

    // Set all motors to safe state
    motor_emergency_stop();

    initialized = true;
    return MOTOR_OK;
}

motor_status_t motor_set_state(uint8_t direction, uint8_t speed_pwm)
{
    if (!initialized) {
        return MOTOR_ERR_INIT;
    }

    if (speed_pwm > MOTOR_SPEED_MAX) {
        return MOTOR_ERR_INVALID_SPD;
    }

    motor_status_t status = MOTOR_OK;

    switch (direction) {
        case UART_DIR_STOP:
            motor_emergency_stop();
            break;

        case UART_DIR_FORWARD:
            status |= motor_set_single(MOTOR_LEFT_FRONT, MOTOR_DIRECTION_FORWARD, speed_pwm);
            status |= motor_set_single(MOTOR_LEFT_BACK, MOTOR_DIRECTION_FORWARD, speed_pwm);
            status |= motor_set_single(MOTOR_RIGHT_FRONT, MOTOR_DIRECTION_FORWARD, speed_pwm);
            status |= motor_set_single(MOTOR_RIGHT_BACK, MOTOR_DIRECTION_FORWARD, speed_pwm);
            break;

        case UART_DIR_BACKWARD:
            status |= motor_set_single(MOTOR_LEFT_FRONT, MOTOR_DIRECTION_BACKWARD, speed_pwm);
            status |= motor_set_single(MOTOR_LEFT_BACK, MOTOR_DIRECTION_BACKWARD, speed_pwm);
            status |= motor_set_single(MOTOR_RIGHT_FRONT, MOTOR_DIRECTION_BACKWARD, speed_pwm);
            status |= motor_set_single(MOTOR_RIGHT_BACK, MOTOR_DIRECTION_BACKWARD, speed_pwm);
            break;

        case UART_DIR_LEFT:
            // Left motors backward, right motors forward
            {
                uint8_t turn_speed = (uint8_t)(speed_pwm * MOTOR_TURN_SPEED_RATIO);
                status |= motor_set_single(MOTOR_LEFT_FRONT, MOTOR_DIRECTION_BACKWARD, turn_speed);
                status |= motor_set_single(MOTOR_LEFT_BACK, MOTOR_DIRECTION_BACKWARD, turn_speed);
                status |= motor_set_single(MOTOR_RIGHT_FRONT, MOTOR_DIRECTION_FORWARD, turn_speed);
                status |= motor_set_single(MOTOR_RIGHT_BACK, MOTOR_DIRECTION_FORWARD, turn_speed);
            }
            break;

        case UART_DIR_RIGHT:
            // Left motors forward, right motors backward
            {
                uint8_t turn_speed = (uint8_t)(speed_pwm * MOTOR_TURN_SPEED_RATIO);
                status |= motor_set_single(MOTOR_LEFT_FRONT, MOTOR_DIRECTION_FORWARD, turn_speed);
                status |= motor_set_single(MOTOR_LEFT_BACK, MOTOR_DIRECTION_FORWARD, turn_speed);
                status |= motor_set_single(MOTOR_RIGHT_FRONT, MOTOR_DIRECTION_BACKWARD, turn_speed);
                status |= motor_set_single(MOTOR_RIGHT_BACK, MOTOR_DIRECTION_BACKWARD, turn_speed);
            }
            break;

        default:
            return MOTOR_ERR_INVALID_DIR;
    }

    return status;
}

motor_status_t motor_set_single(motor_id_t motor, motor_direction_t dir, uint8_t speed)
{
    if (!initialized) {
        return MOTOR_ERR_INIT;
    }

    if (motor >= MOTOR_COUNT) {
        return MOTOR_ERR_INVALID_DIR;
    }

    if (speed > MOTOR_SPEED_MAX) {
        return MOTOR_ERR_INVALID_SPD;
    }

    // Apply trim adjustment
    uint8_t adjusted_speed = motor_apply_trim(motor, speed);

    // Apply direction to GPIO pins
    motor_apply_direction(motor, dir);

    // Apply PWM speed
    if (dir != MOTOR_DIRECTION_BRAKE && dir != MOTOR_DIRECTION_COAST) {
        motor_apply_pwm(motor, adjusted_speed);
    } else {
        motor_apply_pwm(motor, 0);
    }

    // Update state
    motors[motor].current_speed = adjusted_speed;
    motors[motor].current_dir = dir;

    return MOTOR_OK;
}

void motor_emergency_stop(void)
{
    // Active braking: set both IN pins HIGH to brake
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        // Set both direction pins HIGH for active brake
        HAL_GPIO_WritePin(motors[i].in1_port, motors[i].in1_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(motors[i].in2_port, motors[i].in2_pin, GPIO_PIN_SET);
        
        // Set PWM to 100% for maximum braking force
        motor_apply_pwm((motor_id_t)i, MOTOR_SPEED_MAX);
        
        motors[i].current_speed = 0;
        motors[i].current_dir = MOTOR_DIRECTION_BRAKE;
    }

    // Hold brake for 50ms then coast
    HAL_Delay(50);

    // Switch to coast mode (both pins LOW, PWM off)
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        HAL_GPIO_WritePin(motors[i].in1_port, motors[i].in1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(motors[i].in2_port, motors[i].in2_pin, GPIO_PIN_RESET);
        motor_apply_pwm((motor_id_t)i, 0);
        motors[i].current_dir = MOTOR_DIRECTION_COAST;
    }
}

void motor_soft_stop(uint16_t decel_time_ms)
{
    if (decel_time_ms == 0) {
        motor_emergency_stop();
        return;
    }

    // Calculate number of steps for deceleration
    uint16_t steps = decel_time_ms / SOFT_STOP_STEP_MS;
    if (steps == 0) steps = 1;

    // Store initial speeds
    uint8_t initial_speeds[MOTOR_COUNT];
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        initial_speeds[i] = motors[i].current_speed;
    }

    // Gradually reduce speed
    for (uint16_t step = 0; step < steps; step++) {
        for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
            if (motors[i].current_dir != MOTOR_DIRECTION_BRAKE && 
                motors[i].current_dir != MOTOR_DIRECTION_COAST) {
                
                // Calculate new speed (linear ramp down)
                uint8_t new_speed = initial_speeds[i] * (steps - step) / steps;
                motor_apply_pwm((motor_id_t)i, new_speed);
                motors[i].current_speed = new_speed;
            }
        }
        HAL_Delay(SOFT_STOP_STEP_MS);
    }

    // Final coast
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        HAL_GPIO_WritePin(motors[i].in1_port, motors[i].in1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(motors[i].in2_port, motors[i].in2_pin, GPIO_PIN_RESET);
        motor_apply_pwm((motor_id_t)i, 0);
        motors[i].current_speed = 0;
        motors[i].current_dir = MOTOR_DIRECTION_COAST;
    }
}

uint8_t motor_get_current_speed(motor_id_t motor)
{
    if (motor >= MOTOR_COUNT) {
        return 0;
    }
    return motors[motor].current_speed;
}

void motor_set_trim(int8_t left_trim, int8_t right_trim)
{
    // Clamp trim values to -50 to +50
    if (left_trim < -50) left_trim = -50;
    if (left_trim > 50) left_trim = 50;
    if (right_trim < -50) right_trim = -50;
    if (right_trim > 50) right_trim = 50;

    motors[MOTOR_LEFT_FRONT].trim_adjustment = left_trim;
    motors[MOTOR_LEFT_BACK].trim_adjustment = left_trim;
    motors[MOTOR_RIGHT_FRONT].trim_adjustment = right_trim;
    motors[MOTOR_RIGHT_BACK].trim_adjustment = right_trim;
}

void motor_run_test_sequence(uint8_t test_speed, uint16_t duration_ms)
{
    if (!initialized || test_speed > MOTOR_SPEED_MAX) {
        return;
    }

    // Test each motor individually
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        // Forward
        motor_set_single((motor_id_t)i, MOTOR_DIRECTION_FORWARD, test_speed);
        HAL_Delay(duration_ms);
        
        // Stop
        motor_set_single((motor_id_t)i, MOTOR_DIRECTION_BRAKE, 0);
        HAL_Delay(500);
        
        // Backward
        motor_set_single((motor_id_t)i, MOTOR_DIRECTION_BACKWARD, test_speed);
        HAL_Delay(duration_ms);
        
        // Stop
        motor_set_single((motor_id_t)i, MOTOR_DIRECTION_BRAKE, 0);
        HAL_Delay(500);
    }

    motor_emergency_stop();
}

/* -- Private Function Implementations -------------------------------------- */

static void motor_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable GPIO clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Configure all direction pins as output
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        GPIO_InitStruct.Pin = motors[i].in1_pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(motors[i].in1_port, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = motors[i].in2_pin;
        HAL_GPIO_Init(motors[i].in2_port, &GPIO_InitStruct);

        // Initialize to coast (both LOW)
        HAL_GPIO_WritePin(motors[i].in1_port, motors[i].in1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(motors[i].in2_port, motors[i].in2_pin, GPIO_PIN_RESET);
    }
}

static void motor_pwm_init(void)
{
    TIM_HandleTypeDef htim2 = {0};
    TIM_HandleTypeDef htim3 = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    // Enable timer clocks
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    // Configure TIM2 (for left motors and right front)
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = (SystemCoreClock / (PWM_RESOLUTION * MOTOR_PWM_FREQUENCY_HZ)) - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = PWM_RESOLUTION - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    // Configure TIM3 (for right back motor)
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = (SystemCoreClock / (PWM_RESOLUTION * MOTOR_PWM_FREQUENCY_HZ)) - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = PWM_RESOLUTION - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim3);

    // Configure PWM channels
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    // Start all PWM channels with 0% duty cycle
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3);
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
}

static void motor_apply_pwm(motor_id_t motor, uint8_t speed)
{
    if (motor >= MOTOR_COUNT) return;

    // Convert 0-255 speed to PWM resolution (0-1000)
    uint32_t pulse = (uint32_t)(speed * PWM_RESOLUTION / 255);

    // Apply to appropriate timer channel
    switch (motors[motor].pwm_channel) {
        case 1:
            __HAL_TIM_SET_COMPARE(motors[motor].pwm_timer, TIM_CHANNEL_1, pulse);
            break;
        case 2:
            __HAL_TIM_SET_COMPARE(motors[motor].pwm_timer, TIM_CHANNEL_2, pulse);
            break;
        case 3:
            __HAL_TIM_SET_COMPARE(motors[motor].pwm_timer, TIM_CHANNEL_3, pulse);
            break;
    }
}

static void motor_apply_direction(motor_id_t motor, motor_direction_t dir)
{
    if (motor >= MOTOR_COUNT) return;

    switch (dir) {
        case MOTOR_DIRECTION_FORWARD:
            HAL_GPIO_WritePin(motors[motor].in1_port, motors[motor].in1_pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(motors[motor].in2_port, motors[motor].in2_pin, GPIO_PIN_RESET);
            break;

        case MOTOR_DIRECTION_BACKWARD:
            HAL_GPIO_WritePin(motors[motor].in1_port, motors[motor].in1_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(motors[motor].in2_port, motors[motor].in2_pin, GPIO_PIN_SET);
            break;

        case MOTOR_DIRECTION_BRAKE:
            HAL_GPIO_WritePin(motors[motor].in1_port, motors[motor].in1_pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(motors[motor].in2_port, motors[motor].in2_pin, GPIO_PIN_SET);
            break;

        case MOTOR_DIRECTION_COAST:
            HAL_GPIO_WritePin(motors[motor].in1_port, motors[motor].in1_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(motors[motor].in2_port, motors[motor].in2_pin, GPIO_PIN_RESET);
            break;
    }
}

static uint8_t motor_apply_trim(motor_id_t motor, uint8_t speed)
{
    if (motor >= MOTOR_COUNT) return speed;

    int16_t adjusted = speed + motors[motor].trim_adjustment;

    // Clamp to valid range
    if (adjusted < MOTOR_SPEED_MIN) adjusted = MOTOR_SPEED_MIN;
    if (adjusted > MOTOR_SPEED_MAX) adjusted = MOTOR_SPEED_MAX;

    return (uint8_t)adjusted;
}
