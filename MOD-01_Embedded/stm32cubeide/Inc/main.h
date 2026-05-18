/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  *                   Updated for HardwareScheme v5.2 — STM32F103C6
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* ---- Motor L298N (v5.2) ------------------------------------------------- */
#define MOTOR_A_IN1_Pin          GPIO_PIN_11     /* PA11 — Left  IN1 */
#define MOTOR_A_IN1_GPIO_Port    GPIOA
#define MOTOR_A_IN2_Pin          GPIO_PIN_12     /* PA12 — Left  IN2 */
#define MOTOR_A_IN2_GPIO_Port    GPIOA
#define MOTOR_B_IN3_Pin          GPIO_PIN_10     /* PB10 — Right IN3  ← FIXED (was PB13) */
#define MOTOR_B_IN3_GPIO_Port    GPIOB
#define MOTOR_B_IN4_Pin          GPIO_PIN_0      /* PB0  — Right IN4 */
#define MOTOR_B_IN4_GPIO_Port    GPIOB
/* ENA = PB8 (TIM4_CH3), ENB = PB9 (TIM4_CH4) — configured by HAL_TIM_MspPostInit */

/* ---- LED MOSFET (v5.2) -------------------------------------------------- */
#define LED_MOSFET_Pin           GPIO_PIN_1      /* PB1  — LED low-side MOSFET  ← FIXED (was PA5) */
#define LED_MOSFET_GPIO_Port     GPIOB

/* ---- Power Management (v5.2) -------------------------------------------- */
#define PWR_DECOY_EN_Pin         GPIO_PIN_5      /* PA5  — 12V MOSFET gate */
#define PWR_DECOY_EN_GPIO_Port   GPIOA
#define PWR_PI_STATUS_Pin        GPIO_PIN_6      /* PA6  — Pi 5V rail monitor */
#define PWR_PI_STATUS_GPIO_Port  GPIOA

/* ---- HC-SR04 TRIG (v5.2) ------------------------------------------------ */
#define HCSR04_TRIG0_Pin         GPIO_PIN_15     /* PA15 — Front  TRIG (JTAG remap) */
#define HCSR04_TRIG0_GPIO_Port   GPIOA
#define HCSR04_TRIG1_Pin         GPIO_PIN_3      /* PB3  — Back   TRIG (JTAG remap) */
#define HCSR04_TRIG1_GPIO_Port   GPIOB
#define HCSR04_TRIG2_Pin         GPIO_PIN_7      /* PA7  — Left   TRIG */
#define HCSR04_TRIG2_GPIO_Port   GPIOA
#define HCSR04_TRIG3_Pin         GPIO_PIN_8      /* PA8  — Right  TRIG */
#define HCSR04_TRIG3_GPIO_Port   GPIOA

/* ---- HC-SR04 ECHO (v5.2) — FT pins, 5V tolerant ------------------------ */
#define HCSR04_ECHO0_Pin         GPIO_PIN_12     /* PB12 — Front  ECHO */
#define HCSR04_ECHO0_GPIO_Port   GPIOB
#define HCSR04_ECHO1_Pin         GPIO_PIN_13     /* PB13 — Back   ECHO */
#define HCSR04_ECHO1_GPIO_Port   GPIOB
#define HCSR04_ECHO2_Pin         GPIO_PIN_14     /* PB14 — Left   ECHO */
#define HCSR04_ECHO2_GPIO_Port   GPIOB
#define HCSR04_ECHO3_Pin         GPIO_PIN_15     /* PB15 — Right  ECHO */
#define HCSR04_ECHO3_GPIO_Port   GPIOB

/* ---- IMU (v5.2) --------------------------------------------------------- */
#define MPU6050_INT_Pin          GPIO_PIN_5      /* PB5  — MPU6050 interrupt */
#define MPU6050_INT_GPIO_Port    GPIOB
/* I2C1: PB6 (SCL), PB7 (SDA) — configured by HAL_I2C_MspInit */

/* ---- DHT11 (v5.2) ------------------------------------------------------- */
#define DHT11_DATA_Pin           GPIO_PIN_11     /* PB11 — DHT11 single-wire */
#define DHT11_DATA_GPIO_Port     GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
