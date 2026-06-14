/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.c  —  MOD-01 Full System (STM32CubeIDE version, v5.2)
  * @brief   HardwareScheme v5.2 + Proje Raporu gereksinimleri:
  *          - PWM speed control (0-100%)
  *          - Emergency stop with active braking
  *          - Multi-stage stuck recovery (REVERSE → TURN_LEFT → TURN_RIGHT → FAIL)
  *          - IMU magnitude with gravity compensation
  *          - Robot state machine (INIT→IDLE→MANUAL→STUCK_RECOVERY→RTH→EMERGENCY)
  *          - AND-logic obstacle avoidance (HC-SR04)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <string.h>

/* USER CODE BEGIN Includes */
#include "pwr_management.h"
#include "uart_comm.h"
#include "motor_control.h"
#include "stuck_detection.h"
#include "environment_sensors.h"
#include "hcsr04.h"
#include "robot_state_machine.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef  hadc1;
I2C_HandleTypeDef  hi2c1;
TIM_HandleTypeDef  htim4;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* ---- Subsystem instances ------------------------------------------------ */
static MotorControl_t     g_motor;
static StuckDetection_t   g_stuck;
static environment_data_t g_env_data;
static uart_telemetry_t   g_telemetry;
static uart_command_t     g_last_cmd;

/* ---- State machine ------------------------------------------------------- */
static robot_state_t      g_state           = STATE_INIT;
static uint32_t           g_state_entry_ms  = 0;

/* ---- Timing trackers ---------------------------------------------------- */
static uint32_t t_stuck   = 0;
static uint32_t t_env     = 0;
static uint32_t t_telem   = 0;
static uint32_t t_pwr     = 0;
static uint32_t t_hcsr04  = 0;

/* ---- AND-logic obstacle avoidance thresholds (cm) ----------------------- */
#define OBSTACLE_FRONT_CM   25u  /**< Stop if front sensor < 25 cm */
#define OBSTACLE_SIDE_CM    15u  /**< AND-logic: also a side < 15 cm */

/* ---- Idle timeout: revert to IDLE if no command for N ms ---------------- */
#define IDLE_TIMEOUT_MS     3000u

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
static void System_Init(void);
static void FSM_TransitionTo(robot_state_t new_state);
static bool Avoidance_AndLogic_ShouldStop(void);
static void Task_FSM_Update(uint32_t now);
/* USER CODE END PFP */

/* =========================================================================
 * USER CODE BEGIN 0
 * =========================================================================*/

/* ---------- System Init -------------------------------------------------- */
static void System_Init(void)
{
    if (pwr_init() != PWR_STATUS_OK) Error_Handler();
    pwr_set_decoy_state(true);

    if (uart_comm_init() != 0) Error_Handler();

    /* Motor — v5.2 pin mapping */
    MotorConfig_t mcfg = {
        .motorA_IN1_port    = GPIOA, .motorA_IN1_pin     = GPIO_PIN_11,
        .motorA_IN2_port    = GPIOA, .motorA_IN2_pin     = GPIO_PIN_12,
        .motorB_IN3_port    = GPIOB, .motorB_IN3_pin     = GPIO_PIN_10,
        .motorB_IN4_port    = GPIOB, .motorB_IN4_pin     = GPIO_PIN_0,
        .pwm_timer          = &htim4,
        .motorA_pwm_channel = TIM_CHANNEL_3,   /* ENA — PB8 */
        .motorB_pwm_channel = TIM_CHANNEL_4    /* ENB — PB9 */
    };
    if (Motor_Init(&g_motor, &mcfg) != HAL_OK) Error_Handler();
    Motor_SetSpeed(&g_motor, 70);   /* Default 70% */

    if (StuckDetection_Init(&g_stuck, &hi2c1, &g_motor) != HAL_OK) Error_Handler();

    env_sensors_init();   /* Non-critical */
    hcsr04_init();

    memset(&g_env_data,  0, sizeof(g_env_data));
    memset(&g_telemetry, 0, sizeof(g_telemetry));
    memset(&g_last_cmd,  0, sizeof(g_last_cmd));
}

/* ---------- State Machine Transition ------------------------------------- */
static void FSM_TransitionTo(robot_state_t new_state)
{
    g_state          = new_state;
    g_state_entry_ms = HAL_GetTick();
}

/* ---------- AND-Logic Obstacle Avoidance --------------------------------- */
/**
 * @brief Report requires "AND-Logic obstacle avoidance":
 *        Stop only when BOTH conditions hold:
 *        (1) front sensor blocked  AND
 *        (2) at least one side sensor also blocked.
 *        This prevents false stops from single-sensor noise.
 * @return true if motors should be overridden to STOP.
 */
static bool Avoidance_AndLogic_ShouldStop(void)
{
    bool front_blocked = (g_telemetry.us_dist_front > 0 &&
                          g_telemetry.us_dist_front < OBSTACLE_FRONT_CM);
    bool side_blocked  = ((g_telemetry.us_dist_left  > 0 &&
                           g_telemetry.us_dist_left  < OBSTACLE_SIDE_CM) ||
                          (g_telemetry.us_dist_right > 0 &&
                           g_telemetry.us_dist_right < OBSTACLE_SIDE_CM));
    return (front_blocked && side_blocked);
}

/* ---------- State Machine Update ----------------------------------------- */
/**
 * @brief  Central FSM tick — called every main loop iteration.
 *         Implements:
 *           INIT → IDLE → MANUAL_CONTROL → STUCK_RECOVERY → EMERGENCY_STOP
 *                                        ↘ RETURN_HOME (time limit)
 */
static void Task_FSM_Update(uint32_t now)
{
    switch (g_state)
    {
    /* ---- INIT ----------------------------------------------------------- */
    case STATE_INIT:
        /* System_Init() was already called in main() before super-loop.
         * Immediately transition to IDLE. */
        Motor_EmergencyStop(&g_motor);
        FSM_TransitionTo(STATE_IDLE);
        break;

    /* ---- IDLE ----------------------------------------------------------- */
    case STATE_IDLE:
        /* Motors stopped. Waiting for UART commands from Pi. */
        if (g_last_cmd.direction != UART_DIR_STOP) {
            FSM_TransitionTo(STATE_MANUAL_CONTROL);
        }
        break;

    /* ---- MANUAL CONTROL ------------------------------------------------- */
    case STATE_MANUAL_CONTROL:
    {
        uart_command_t cmd;
        bool got_cmd = uart_receive_command(&cmd);

        if (got_cmd) {
            g_last_cmd = cmd;
            g_state_entry_ms = now;   /* Reset idle timeout on each command */
        }

        /* AND-logic obstacle avoidance override */
        if (g_last_cmd.direction == UART_DIR_FORWARD &&
            Avoidance_AndLogic_ShouldStop())
        {
            Motor_EmergencyStop(&g_motor);
            /* Notify Pi that obstacle blocked us */
            g_telemetry.is_stuck = true;
            /* Stay in MANUAL_CONTROL — Pi can send a different direction */
        }
        else if (!StuckDetection_IsRecovering(&g_stuck))
        {
            Motor_ExecuteCommand(&g_motor, (MotorCommand_t)g_last_cmd.direction);
            Motor_SetSpeed(&g_motor, (uint8_t)(g_last_cmd.speed_pwm * 100U / 255U));

            /* LED MOSFET (PB1) */
            HAL_GPIO_WritePin(LED_MOSFET_GPIO_Port, LED_MOSFET_Pin,
                              g_last_cmd.lights_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }

        /* Transition to STUCK_RECOVERY if IMU detected stuck */
        if (StuckDetection_GetState(&g_stuck) == STUCK_STATE_CONFIRMED ||
            StuckDetection_GetState(&g_stuck) == STUCK_STATE_RECOVERING)
        {
            FSM_TransitionTo(STATE_STUCK_RECOVERY);
        }

        /* Idle timeout: no meaningful command → go back to IDLE */
        if (g_last_cmd.direction == UART_DIR_STOP &&
            (now - g_state_entry_ms) >= IDLE_TIMEOUT_MS)
        {
            Motor_EmergencyStop(&g_motor);
            FSM_TransitionTo(STATE_IDLE);
        }
        break;
    }

    /* ---- STUCK RECOVERY ------------------------------------------------- */
    case STATE_STUCK_RECOVERY:
        /* StuckDetection_Task() drives recovery internally (multi-stage):
         *   Stage 1: REVERSE    (STUCK_RECOVERY_REVERSE_MS = 1000 ms)
         *   Stage 2: TURN_LEFT  (STUCK_RECOVERY_TURN_MS    = 1000 ms)
         *   Stage 3: TURN_RIGHT (STUCK_RECOVERY_TURN_MS    = 1000 ms)
         *   Stage 4: FAILED     → Emergency stop */
        if (StuckDetection_GetState(&g_stuck) == STUCK_STATE_MONITORING)
        {
            /* Recovery finished successfully */
            FSM_TransitionTo(STATE_MANUAL_CONTROL);
        }
        else if (g_stuck.current_strategy == RECOVERY_FAILED)
        {
            FSM_TransitionTo(STATE_EMERGENCY_STOP);
        }
        break;

    /* ---- EMERGENCY STOP ------------------------------------------------- */
    case STATE_EMERGENCY_STOP:
        Motor_EmergencyStop(&g_motor);
        /* Stay here — Pi must send an explicit STOP+restart command or operator
         * physically resets. Telemetry keeps broadcasting is_stuck=true. */
        break;

    /* ---- RETURN HOME ---------------------------------------------------- */
    case STATE_RETURN_HOME:
        Motor_EmergencyStop(&g_motor);
        pwr_set_decoy_state(false);
        g_telemetry.is_stuck = true;
        uart_send_telemetry(&g_telemetry);
        while (1) { HAL_Delay(1000); }   /* Halt — production: trigger RTH FSM */

    /* ---- ERROR ---------------------------------------------------------- */
    case STATE_ERROR:
    default:
        Error_Handler();
        break;
    }
}

/* USER CODE END 0 */

/* =========================================================================
 * Application entry point
 * =========================================================================*/
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_TIM4_Init();
    MX_USART1_UART_Init();

    /* USER CODE BEGIN 2 */
    System_Init();
    FSM_TransitionTo(STATE_INIT);
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* ---- Core FSM tick (every loop) --------------------------------- */
        Task_FSM_Update(now);

        /* ---- HC-SR04 distance sensing @ 10 Hz --------------------------- */
        if ((now - t_hcsr04) >= TICK_HCSR04_MS) {
            hcsr04_read_all(&g_telemetry.us_dist_front, &g_telemetry.us_dist_back,
                            &g_telemetry.us_dist_left,  &g_telemetry.us_dist_right);
            t_hcsr04 = now;
        }

        /* ---- Stuck detection @ 10 Hz ------------------------------------ */
        if ((now - t_stuck) >= TICK_STUCK_MS) {
            StuckDetection_Task(&g_stuck);
            t_stuck = now;
        }

        /* ---- Environment sensors @ 0.5 Hz ------------------------------- */
        if ((now - t_env) >= TICK_ENV_MS) {
            env_read_all(&g_env_data);
            t_env = now;
        }

        /* ---- Telemetry TX @ 2 Hz ---------------------------------------- */
        if ((now - t_telem) >= TICK_TELEMETRY_MS) {
            g_telemetry.temperature    = g_env_data.temperature_c;
            g_telemetry.smoke_detected = g_env_data.smoke_alert;
            g_telemetry.is_stuck = (StuckDetection_GetState(&g_stuck) == STUCK_STATE_CONFIRMED ||
                                    StuckDetection_GetState(&g_stuck) == STUCK_STATE_RECOVERING ||
                                    g_state == STATE_EMERGENCY_STOP);
            uart_send_telemetry(&g_telemetry);
            t_telem = now;
        }

        /* ---- Power failsafe @ 1 Hz -------------------------------------- */
        if ((now - t_pwr) >= TICK_POWER_MS) {
            if (pwr_is_time_limit_exceeded()) {
                FSM_TransitionTo(STATE_RETURN_HOME);
            }
            if (!pwr_is_pi_alive() && g_state == STATE_MANUAL_CONTROL) {
                Motor_EmergencyStop(&g_motor);
                FSM_TransitionTo(STATE_EMERGENCY_STOP);
            }
            t_pwr = now;
        }
    /* USER CODE END WHILE */
    }
}

/* =========================================================================
 * Peripheral Init (CubeMX style)
 * =========================================================================*/

/** SystemClock: HSI + PLL → 72 MHz */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef       osc = {0};
    RCC_ClkInitTypeDef       clk = {0};
    RCC_PeriphCLKInitTypeDef pclk= {0};

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;  /* 4 MHz */
    osc.PLL.PLLMUL          = RCC_PLL_MUL18;            /* 72 MHz */
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

    clk.ClockType      = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|
                         RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;   /* HCLK  = 72 MHz */
    clk.APB1CLKDivider = RCC_HCLK_DIV2;     /* PCLK1 = 36 MHz */
    clk.APB2CLKDivider = RCC_HCLK_DIV1;     /* PCLK2 = 72 MHz */
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) Error_Handler();

    pclk.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    pclk.AdcClockSelection    = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) Error_Handler();
}

/** ADC1: PA4 / IN4 (MQ-2) */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef ch = {0};
    hadc1.Instance                   = ADC1;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();
    ch.Channel      = ADC_CHANNEL_4;
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &ch) != HAL_OK) Error_Handler();
}

/** I2C1: 100 kHz (MPU6050) */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 100000;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
}

/** TIM4: PWM CH3(PB8/ENA) + CH4(PB9/ENB), 1 kHz @ 72 MHz */
static void MX_TIM4_Init(void)
{
    TIM_MasterConfigTypeDef mc = {0};
    TIM_OC_InitTypeDef      oc = {0};
    htim4.Instance               = TIM4;
    htim4.Init.Prescaler         = 71;     /* 72 MHz / 72 = 1 MHz */
    htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim4.Init.Period            = 999;    /* 1 MHz / 1000 = 1 kHz */
    htim4.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK) Error_Handler();
    mc.MasterOutputTrigger = TIM_TRGO_RESET;
    mc.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &mc) != HAL_OK) Error_Handler();
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_3) != HAL_OK) Error_Handler();
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_4) != HAL_OK) Error_Handler();
    HAL_TIM_MspPostInit(&htim4);
}

/** USART1: 115200 8N1 (Pi ↔ STM32) */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl   = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

/** GPIO: all v5.2 pins */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Initial output levels: LOW */
    HAL_GPIO_WritePin(GPIOA,
        MOTOR_A_IN1_Pin|MOTOR_A_IN2_Pin|
        HCSR04_TRIG0_Pin|HCSR04_TRIG1_Pin|HCSR04_TRIG2_Pin|HCSR04_TRIG3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB,
        MOTOR_B_IN3_Pin|MOTOR_B_IN4_Pin|LED_MOSFET_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, PWR_DECOY_EN_Pin, GPIO_PIN_RESET);

    /* GPIOA Outputs */
    g.Pin   = MOTOR_A_IN1_Pin|MOTOR_A_IN2_Pin|
              HCSR04_TRIG0_Pin|HCSR04_TRIG1_Pin|HCSR04_TRIG2_Pin|HCSR04_TRIG3_Pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);

    /* GPIOB Outputs */
    g.Pin   = MOTOR_B_IN3_Pin|MOTOR_B_IN4_Pin|LED_MOSFET_Pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    /* GPIOB Inputs: ECHO x4, MPU6050 INT */
    g.Pin  = HCSR04_ECHO0_Pin|HCSR04_ECHO1_Pin|HCSR04_ECHO2_Pin|HCSR04_ECHO3_Pin|
             MPU6050_INT_Pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &g);

    /* GPIOC Output: PWR_DECOY_EN */
    g.Pin   = PWR_DECOY_EN_Pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &g);

    /* GPIOC Input: PWR_PI_STATUS */
    g.Pin  = PWR_PI_STATUS_Pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOC, &g);

    /* DHT11 data pin — pull-up input (reconfigured during comms) */
    g.Pin  = DHT11_DATA_Pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &g);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    Motor_EmergencyStop(&g_motor);
    pwr_set_decoy_state(false);
    __disable_irq();
    while (1) {}
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
