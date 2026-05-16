/**
 * @file main_integration_example.c
 * @brief Example integration of Motor Control + Stuck Detection
 * @details Shows how to integrate UART command processing with IMU-based
 *          stuck detection and recovery
 */

#include "motor_control.h"
#include "stuck_detection.h"
#include "stm32f4xx_hal.h"

/* Global handles (initialized in main) */
extern UART_HandleTypeDef huart2;  // UART for commands
extern I2C_HandleTypeDef hi2c1;    // I2C for MPU6050
extern TIM_HandleTypeDef htim3;    // Timer for PWM

/* Global instances */
MotorControl_t g_motor;
StuckDetection_t g_stuck_detector;

/* UART receive buffer */
uint8_t uart_rx_buffer[1];

/**
 * @brief System initialization
 */
void System_Init(void)
{
    /* Configure Motor Control */
    MotorConfig_t motor_config = {
        // Motor A (Left) - L298N IN1 and IN2
        .motorA_IN1_port = GPIOA,
        .motorA_IN1_pin = GPIO_PIN_0,
        .motorA_IN2_port = GPIOA,
        .motorA_IN2_pin = GPIO_PIN_1,
        
        // Motor B (Right) - L298N IN3 and IN4
        .motorB_IN3_port = GPIOA,
        .motorB_IN3_pin = GPIO_PIN_4,
        .motorB_IN4_port = GPIOA,
        .motorB_IN4_pin = GPIO_PIN_5,
        
        // PWM configuration (optional)
        .pwm_timer = &htim3,
        .motorA_pwm_channel = TIM_CHANNEL_1,  // ENA on TIM3 CH1
        .motorB_pwm_channel = TIM_CHANNEL_2   // ENB on TIM3 CH2
    };
    
    /* Initialize motor control */
    if (Motor_Init(&g_motor, &motor_config) != HAL_OK) {
        Error_Handler();
    }
    
    /* Set initial speed to 70% */
    Motor_SetSpeed(&g_motor, 70);
    
    /* Initialize stuck detection */
    if (StuckDetection_Init(&g_stuck_detector, &hi2c1, &g_motor) != HAL_OK) {
        Error_Handler();
    }
    
    /* Start UART interrupt reception */
    HAL_UART_Receive_IT(&huart2, uart_rx_buffer, 1);
}

/**
 * @brief UART receive callback - process motor commands
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        // Map received byte to motor command
        MotorCommand_t cmd;
        
        switch (uart_rx_buffer[0]) {
            case 'F':
            case 'f':
            case '1':
                cmd = MOTOR_CMD_FORWARD;
                break;
                
            case 'B':
            case 'b':
            case '2':
                cmd = MOTOR_CMD_BACKWARD;
                break;
                
            case 'L':
            case 'l':
            case '3':
                cmd = MOTOR_CMD_LEFT;
                break;
                
            case 'R':
            case 'r':
            case '4':
                cmd = MOTOR_CMD_RIGHT;
                break;
                
            case 'S':
            case 's':
            case '0':
            default:
                cmd = MOTOR_CMD_STOP;
                break;
        }
        
        // Execute command (unless in recovery)
        if (!StuckDetection_IsRecovering(&g_stuck_detector)) {
            Motor_ExecuteCommand(&g_motor, cmd);
        }
        
        // Re-enable UART reception
        HAL_UART_Receive_IT(&huart2, uart_rx_buffer, 1);
    }
}

/**
 * @brief Main loop
 */
int main(void)
{
    /* STM32 HAL initialization */
    HAL_Init();
    SystemClock_Config();
    
    /* Initialize peripherals (GPIO, UART, I2C, TIM) */
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    MX_TIM3_Init();
    
    /* Initialize system */
    System_Init();
    
    /* Main loop */
    while (1)
    {
        /* Run stuck detection task every 100ms */
        static uint32_t last_detection_time = 0;
        uint32_t current_time = HAL_GetTick();
        
        if ((current_time - last_detection_time) >= 100) {
            StuckDetection_Task(&g_stuck_detector);
            last_detection_time = current_time;
        }
        
        /* Optional: Send status via UART for debugging */
        static uint32_t last_status_time = 0;
        if ((current_time - last_status_time) >= 1000) {
            Send_Status_Update();
            last_status_time = current_time;
        }
    }
}

/**
 * @brief Send status update over UART (debugging)
 */
void Send_Status_Update(void)
{
    char msg[100];
    uint32_t total_stuck, successful_recoveries;
    
    StuckDetection_GetStats(&g_stuck_detector, &total_stuck, &successful_recoveries);
    
    sprintf(msg, "State: %d | Stuck Events: %lu | Recoveries: %lu\r\n",
            StuckDetection_GetState(&g_stuck_detector),
            total_stuck,
            successful_recoveries);
    
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

/**
 * @brief Error handler
 */
void Error_Handler(void)
{
    Motor_EmergencyStop(&g_motor);
    while (1) {
        // Blink LED or halt
    }
}
