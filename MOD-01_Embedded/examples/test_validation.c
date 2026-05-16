/**
 * @file test_validation.c
 * @brief Test cases and validation functions for motor control and stuck detection
 * @details Use these functions to validate your hardware setup
 */

#include "motor_control.h"
#include "stuck_detection.h"
#include <stdio.h>

extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim3;

/**
 * @brief Test 1: Basic GPIO Control
 * @details Verify L298N pins toggle correctly
 */
void Test_GPIO_Control(void)
{
    printf("\r\n=== Test 1: GPIO Control ===\r\n");
    
    MotorControl_t motor;
    MotorConfig_t config = {
        .motorA_IN1_port = GPIOA, .motorA_IN1_pin = GPIO_PIN_0,
        .motorA_IN2_port = GPIOA, .motorA_IN2_pin = GPIO_PIN_1,
        .motorB_IN3_port = GPIOA, .motorB_IN3_pin = GPIO_PIN_4,
        .motorB_IN4_port = GPIOA, .motorB_IN4_pin = GPIO_PIN_5,
        .pwm_timer = NULL  // No PWM for this test
    };
    
    Motor_Init(&motor, &config);
    
    // Test each direction
    printf("Testing FORWARD (2 sec)...\r\n");
    Motor_ExecuteCommand(&motor, MOTOR_CMD_FORWARD);
    HAL_Delay(2000);
    
    printf("Testing BACKWARD (2 sec)...\r\n");
    Motor_ExecuteCommand(&motor, MOTOR_CMD_BACKWARD);
    HAL_Delay(2000);
    
    printf("Testing LEFT (2 sec)...\r\n");
    Motor_ExecuteCommand(&motor, MOTOR_CMD_LEFT);
    HAL_Delay(2000);
    
    printf("Testing RIGHT (2 sec)...\r\n");
    Motor_ExecuteCommand(&motor, MOTOR_CMD_RIGHT);
    HAL_Delay(2000);
    
    printf("Testing STOP...\r\n");
    Motor_ExecuteCommand(&motor, MOTOR_CMD_STOP);
    
    printf("GPIO Test Complete!\r\n");
}

/**
 * @brief Test 2: PWM Speed Control
 * @details Verify PWM signals are generated correctly
 */
void Test_PWM_Control(void)
{
    printf("\r\n=== Test 2: PWM Speed Control ===\r\n");
    
    MotorControl_t motor;
    MotorConfig_t config = {
        .motorA_IN1_port = GPIOA, .motorA_IN1_pin = GPIO_PIN_0,
        .motorA_IN2_port = GPIOA, .motorA_IN2_pin = GPIO_PIN_1,
        .motorB_IN3_port = GPIOA, .motorB_IN3_pin = GPIO_PIN_4,
        .motorB_IN4_port = GPIOA, .motorB_IN4_pin = GPIO_PIN_5,
        .pwm_timer = &htim3,
        .motorA_pwm_channel = TIM_CHANNEL_1,
        .motorB_pwm_channel = TIM_CHANNEL_2
    };
    
    Motor_Init(&motor, &config);
    Motor_ExecuteCommand(&motor, MOTOR_CMD_FORWARD);
    
    // Test different speeds
    printf("Speed 25%% (2 sec)...\r\n");
    Motor_SetSpeed(&motor, 25);
    HAL_Delay(2000);
    
    printf("Speed 50%% (2 sec)...\r\n");
    Motor_SetSpeed(&motor, 50);
    HAL_Delay(2000);
    
    printf("Speed 75%% (2 sec)...\r\n");
    Motor_SetSpeed(&motor, 75);
    HAL_Delay(2000);
    
    printf("Speed 100%% (2 sec)...\r\n");
    Motor_SetSpeed(&motor, 100);
    HAL_Delay(2000);
    
    Motor_EmergencyStop(&motor);
    printf("PWM Test Complete!\r\n");
}

/**
 * @brief Test 3: MPU6050 Communication
 * @details Verify I2C connection and data reading
 */
void Test_MPU6050_Communication(void)
{
    printf("\r\n=== Test 3: MPU6050 I2C Communication ===\r\n");
    
    // Initialize MPU6050
    if (MPU6050_Init(&hi2c1) != HAL_OK) {
        printf("ERROR: MPU6050 initialization failed!\r\n");
        printf("Check:\r\n");
        printf("  - I2C wiring (SDA/SCL)\r\n");
        printf("  - Pull-up resistors (4.7k to 3.3V)\r\n");
        printf("  - I2C address (0xD0 or 0xD1)\r\n");
        return;
    }
    
    printf("MPU6050 initialized successfully!\r\n");
    
    // Read data 10 times
    IMU_Data_t imu;
    for (int i = 0; i < 10; i++) {
        if (MPU6050_ReadData(&hi2c1, &imu) == HAL_OK) {
            printf("Sample %d: Accel(%d, %d, %d) Gyro(%d, %d, %d)\r\n",
                   i+1,
                   imu.accel_x, imu.accel_y, imu.accel_z,
                   imu.gyro_x, imu.gyro_y, imu.gyro_z);
        } else {
            printf("ERROR: Failed to read IMU data\r\n");
        }
        HAL_Delay(200);
    }
    
    printf("MPU6050 Test Complete!\r\n");
}

/**
 * @brief Test 4: Stuck Detection Logic
 * @details Simulate stuck condition (hold robot while motors run)
 */
void Test_StuckDetection_Logic(void)
{
    printf("\r\n=== Test 4: Stuck Detection ===\r\n");
    printf("This test requires MANUAL INTERVENTION:\r\n");
    printf("1. Motors will start moving forward\r\n");
    printf("2. HOLD THE ROBOT to prevent movement\r\n");
    printf("3. After 2 seconds, recovery should trigger\r\n");
    printf("4. Release the robot to verify recovery\r\n");
    printf("\r\nPress any key to start...\r\n");
    
    // Wait for keypress
    uint8_t dummy;
    HAL_UART_Receive(&huart2, &dummy, 1, HAL_MAX_DELAY);
    
    MotorControl_t motor;
    StuckDetection_t detector;
    
    MotorConfig_t motor_config = {
        .motorA_IN1_port = GPIOA, .motorA_IN1_pin = GPIO_PIN_0,
        .motorA_IN2_port = GPIOA, .motorA_IN2_pin = GPIO_PIN_1,
        .motorB_IN3_port = GPIOA, .motorB_IN3_pin = GPIO_PIN_4,
        .motorB_IN4_port = GPIOA, .motorB_IN4_pin = GPIO_PIN_5,
        .pwm_timer = &htim3,
        .motorA_pwm_channel = TIM_CHANNEL_1,
        .motorB_pwm_channel = TIM_CHANNEL_2
    };
    
    Motor_Init(&motor, &motor_config);
    Motor_SetSpeed(&motor, 50);
    StuckDetection_Init(&detector, &hi2c1, &motor);
    
    printf("\r\nStarting motors... HOLD THE ROBOT NOW!\r\n");
    Motor_ExecuteCommand(&motor, MOTOR_CMD_FORWARD);
    
    // Run detection loop for 10 seconds
    uint32_t start_time = HAL_GetTick();
    uint32_t last_task_time = start_time;
    
    while ((HAL_GetTick() - start_time) < 10000) {
        // Run stuck detection every 100ms
        if ((HAL_GetTick() - last_task_time) >= 100) {
            StuckDetection_Task(&detector);
            last_task_time = HAL_GetTick();
            
            // Print status
            static StuckState_t last_state = STUCK_STATE_MONITORING;
            StuckState_t current_state = StuckDetection_GetState(&detector);
            
            if (current_state != last_state) {
                switch (current_state) {
                    case STUCK_STATE_MONITORING:
                        printf("State: MONITORING\r\n");
                        break;
                    case STUCK_STATE_POSSIBLE:
                        printf("State: POSSIBLE STUCK (timer started)\r\n");
                        break;
                    case STUCK_STATE_CONFIRMED:
                        printf("State: STUCK CONFIRMED!\r\n");
                        break;
                    case STUCK_STATE_RECOVERING:
                        printf("State: RECOVERING (release robot now)\r\n");
                        break;
                }
                last_state = current_state;
            }
        }
    }
    
    Motor_EmergencyStop(&motor);
    
    uint32_t total_stuck, recoveries;
    StuckDetection_GetStats(&detector, &total_stuck, &recoveries);
    
    printf("\r\nTest Complete!\r\n");
    printf("Total stuck events: %lu\r\n", total_stuck);
    printf("Successful recoveries: %lu\r\n", recoveries);
}

/**
 * @brief Test 5: Full System Integration
 * @details Complete operational test
 */
void Test_Full_System_Integration(void)
{
    printf("\r\n=== Test 5: Full System Integration ===\r\n");
    printf("This test runs the complete system.\r\n");
    printf("Send UART commands (F/B/L/R/0) to control the robot.\r\n");
    printf("Stuck detection is active.\r\n");
    printf("Test will run for 30 seconds.\r\n");
    
    // Initialize system (same as main)
    extern MotorControl_t g_motor;
    extern StuckDetection_t g_stuck_detector;
    
    uint32_t start_time = HAL_GetTick();
    uint32_t last_task_time = start_time;
    uint32_t last_status_time = start_time;
    
    while ((HAL_GetTick() - start_time) < 30000) {
        // Run stuck detection
        if ((HAL_GetTick() - last_task_time) >= 100) {
            StuckDetection_Task(&g_stuck_detector);
            last_task_time = HAL_GetTick();
        }
        
        // Print status every second
        if ((HAL_GetTick() - last_status_time) >= 1000) {
            uint32_t total, recoveries;
            StuckDetection_GetStats(&g_stuck_detector, &total, &recoveries);
            printf("Status: State=%d | Active=%d | Stuck=%lu | Recovered=%lu\r\n",
                   StuckDetection_GetState(&g_stuck_detector),
                   Motor_IsActive(&g_motor),
                   total, recoveries);
            last_status_time = HAL_GetTick();
        }
    }
    
    Motor_EmergencyStop(&g_motor);
    printf("\r\nFull System Test Complete!\r\n");
}

/**
 * @brief Run all tests
 */
void Run_All_Tests(void)
{
    printf("\r\n");
    printf("================================================\r\n");
    printf("   Motor Control & Stuck Detection Test Suite\r\n");
    printf("================================================\r\n");
    
    Test_GPIO_Control();
    HAL_Delay(1000);
    
    Test_PWM_Control();
    HAL_Delay(1000);
    
    Test_MPU6050_Communication();
    HAL_Delay(1000);
    
    Test_StuckDetection_Logic();
    HAL_Delay(1000);
    
    Test_Full_System_Integration();
    
    printf("\r\n");
    printf("================================================\r\n");
    printf("   All Tests Complete!\r\n");
    printf("================================================\r\n");
}
