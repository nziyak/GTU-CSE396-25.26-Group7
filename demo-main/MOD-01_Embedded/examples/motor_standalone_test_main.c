/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : motor_standalone_test_main.c
  * @brief          : Standalone Motor Test Program (GPIO Only - No PWM)
  * 
  * STM32F103C6 islemcisinde TIM4 bulunmadigi anlasildigi icin bu test kodunda
  * ENA ve ENB pinleri standart GPIO Output olarak kullanilmistir. Hiz kontrolu 
  * yoktur (sadece Tam Guc ve Sifir Guc kullanilabilir).
  * 
  * Kullanimi: CubeMX'te asagidaki pinleri GPIO_Output yapip label atayin:
  * PA11 (MOTOR_A_IN1), PA12 (MOTOR_A_IN2), PB10 (MOTOR_B_IN3), PB0 (MOTOR_B_IN4)
  * PB8 (MOTOR_ENA), PB9 (MOTOR_ENB)
  * Sonra kodu generate edip bu dosyanin icerigini main.c'ye tamamen yapistirin.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */
  
  /* Test baslangici - ENA ve ENB pinlerine lojik 1 (Tam guc) verilir */
  /* NOT: CubeMX'te bu pinleri tanimlarken etiket adini MOTOR_ENA yapmalisiniz, 
   * aksi halde GPIO_PIN_8 yazarak da direkt mudehale edebilirsiniz. */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);

  /* USER CODE END 2 */

  while (1)
  {
      /* 1. Senaryo: 3 saniye İleri (Tam Güç) */
      // Sol Motor İleri (IN1=1, IN2=0)
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
      // Sağ Motor İleri (IN3=1, IN4=0)
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
      
      HAL_Delay(3000);

      /* 2. Senaryo: 2 saniye Ani Fren (Tüm yön pinleri = 1) */
      // L298N'de tüm IN pinlerini HIGH yapmak motoru frenler
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10 | GPIO_PIN_0, GPIO_PIN_SET);
      
      HAL_Delay(2000);

      /* 3. Senaryo: 3 saniye Geri (Tam Güç) */
      // Sol Motor Geri (IN1=0, IN2=1)
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
      // Sağ Motor Geri (IN3=0, IN4=1)
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
      
      HAL_Delay(3000);

      /* 4. Senaryo: 2 saniye Tamamen Dur (Tüm pinler 0, ENA/ENB=0) */
      // Motorlara güç gitmesin ve boşta dursun
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10 | GPIO_PIN_0, GPIO_PIN_RESET);
      // PWM(ENA/ENB) pinlerini de sıfıra çekiyoruz (Güç = 0)
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);
      
      HAL_Delay(2000);

      /* Döngü başa dönerken tekrar gücü (ENA/ENB) aktif et */
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);
  }
}

/* (SystemClock_Config ve MX_GPIO_Init fonksiyonları CubeMX tarafından oluşturulacaktır) */
