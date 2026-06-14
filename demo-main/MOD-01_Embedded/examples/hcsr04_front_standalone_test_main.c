/**
 * @file hcsr04_front_standalone_test_main.c
 * @brief HC-SR04 Ön Mesafe Sensörü (Front) Standalone Test Kodu
 *
 * CubeMX Konfigürasyonu:
 * 1. PA5  -> GPIO Output (Adı: FRONT_TRIG)
 * 2. PB12 -> GPIO Input  (Adı: FRONT_ECHO)
 * 3. TIM2 -> Internal Clock
 *    - Prescaler: (SystemCoreClock / 1000000) - 1 (Örneğin 72MHz için 71, 8MHz için 7)
 *    - Counter Period: 65535
 *
 * Bu kod main.c içerisine entegre edilebilir veya referans olarak kullanılabilir.
 * Okunan mesafe "distance_cm" değişkeninde tutulmaktadır, STM32CubeIDE "Live Expressions"
 * üzerinden izlenebilir.
 */

#include "stm32f1xx_hal.h"

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
uint32_t local_time = 0;
uint32_t sensor_time = 0;
uint32_t distance_cm = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN PFP */
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

void HCSR04_Read_Front(void)
{
    // 1. TRIG pinini low yap
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    delay_us(2);
    
    // 2. TRIG pinini 10 mikrosaniye high yap
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    
    // 3. ECHO pininin high olmasını bekle
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET)
    {
        // Zaman aşımı (timeout) kontrolü eklenebilir
        if (__HAL_TIM_GET_COUNTER(&htim2) > 60000) break;
    }
    
    // 4. ECHO pini high olduğu süreyi ölç
    local_time = 0;
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_SET)
    {
        local_time = __HAL_TIM_GET_COUNTER(&htim2);
        // Zaman aşımı (timeout) kontrolü
        if (local_time > 60000) break;
    }
    
    sensor_time = local_time;
    // Ses hızı ~343m/s -> 1 cm için gidiş-dönüş süresi ~58 mikrosaniye
    distance_cm = sensor_time / 58;
}
/* USER CODE END PFP */

int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM2_Init();

    /* USER CODE BEGIN 2 */
    // Timer'ı başlat
    HAL_TIM_Base_Start(&htim2);
    /* USER CODE END 2 */

    /* Infinite loop */
    while (1)
    {
        /* USER CODE BEGIN 3 */
        HCSR04_Read_Front();
        
        // Ölçümler arası bekleme (Sensör datasheet'ine göre min 60ms önerilir)
        HAL_Delay(100);
        /* USER CODE END 3 */
    }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  /* CubeMX tarafından üretilen saat konfigürasyon kodu buraya gelecek */
}

/**
  * @brief TIM2 Initialization Function
  */
static void MX_TIM2_Init(void)
{
  /* CubeMX tarafından üretilen TIM2 konfigürasyon kodu buraya gelecek */
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  /* CubeMX tarafından üretilen GPIO konfigürasyon kodu buraya gelecek */
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
