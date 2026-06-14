# MOD-01 STM32CubeMX Pin Konfigürasyon Haritası (v4)

> **Bu doküman kodla birebir eşleşir (Hardware Scheme v4).**  
> STM32CubeMX'te aşağıdaki tabloyu takip ederek pin konfigürasyonunu yapın.  

---

## 1. GPIO — Motor Kontrolü (L298N)

Kaynak dosyalar: `main.c` → `MotorConfig_t`

| STM32 Pin | CubeMX Label     | Mod            | Açıklama                        |
|-----------|------------------|----------------|----------------------------------|
| **PA11**  | `MOTOR_A_IN1`    | GPIO_Output    | Sol Motor — Yön 1 (L298N IN1)   |
| **PA12**  | `MOTOR_A_IN2`    | GPIO_Output    | Sol Motor — Yön 2 (L298N IN2)   |
| **PB10**  | `MOTOR_B_IN3`    | GPIO_Output    | Sağ Motor — Yön 1 (L298N IN3)   |
| **PB0**   | `MOTOR_B_IN4`    | GPIO_Output    | Sağ Motor — Yön 2 (L298N IN4)   |

### PWM (Motor Hız Kontrolü)

Kaynak: `main.c` → `pwm_timer = &htim4`

| STM32 Pin | CubeMX Label     | Mod                  | Açıklama                  |
|-----------|------------------|----------------------|---------------------------|
| **PB8**   | `MOTOR_A_ENA`    | TIM4_CH3 (PWM)       | Sol Motor PWM Enable      |
| **PB9**   | `MOTOR_B_ENB`    | TIM4_CH4 (PWM)       | Sağ Motor PWM Enable      |

**TIM4 Ayarları:**
- Prescaler: `83` (84MHz saat için → 1MHz counter)
- Counter Period (ARR): `999` (→ 1kHz PWM frekansı)
- Mode: PWM Generation CH3 + CH4
- Polarity: High

---

## 2. UART — Raspberry Pi 5 İletişimi

Kaynak dosya: `uart_comm.c`

| STM32 Pin | CubeMX Label | Mod         | Açıklama                    |
|-----------|-------------|-------------|------------------------------|
| **PA9**   | `USART1_TX` | USART1_TX   | STM32 → Pi (Telemetri)      |
| **PA10**  | `USART1_RX` | USART1_RX   | Pi → STM32 (Komutlar)       |

**USART1 Ayarları:**
- Baud Rate: `115200`
- Mode: TX + RX
- ☑ NVIC → USART1 global interrupt: **Enabled** (RX interrupt için şart!)

---

## 3. I2C — MPU6050 IMU Sensörü

Kaynak dosya: `main.c` → `&hi2c1`

| STM32 Pin | CubeMX Label | Mod      | Açıklama           |
|-----------|-------------|----------|---------------------|
| **PB6**   | `I2C1_SCL`  | I2C1_SCL | MPU6050 Clock       |
| **PB7**   | `I2C1_SDA`  | I2C1_SDA | MPU6050 Data        |
| **PB5**   | `MPU_INT`   | GPIO_Input| MPU6050 Interrupt   |

---

## 4. ADC — MQ-2 Duman Sensörü

Kaynak dosya: `environment_sensors.c`

| STM32 Pin | CubeMX Label | Mod            | Açıklama                    |
|-----------|-------------|----------------|------------------------------|
| **PA4**   | `MQ2_ANALOG`| ADC1_IN4       | MQ-2 Analog çıkışı           |

---

## 5. GPIO — DHT11 Sıcaklık/Nem Sensörü

Kaynak dosya: `environment_sensors.c`

| STM32 Pin | CubeMX Label   | Mod            | Açıklama                        |
|-----------|---------------|----------------|----------------------------------|
| **PB11**  | `DHT11_DATA`  | GPIO_Input     | DHT11 tek-kablo veri hattı       |

**CubeMX'te Ayar:** Mode: GPIO_Input (Pull-up)

---

## 6. GPIO — Güç Yönetimi (Opsiyonel Failsafe)

Kaynak dosya: `pwr_management.c`
*(Not: v4 şemasında bunlar doğrudan listelenmediği için eski varsayılan pinler boşta kaldığından onlara atandı)*

| STM32 Pin | CubeMX Label      | Mod           | Açıklama                              |
|-----------|-------------------|---------------|----------------------------------------|
| **PA5**   | `PWR_DECOY_EN`    | GPIO_Output   | 12V Decoy MOSFET tetikleme             |
| **PA6**   | `PWR_PI_STATUS`   | GPIO_Input    | Pi 5V durumu izleme                    |

*(Artık motor pinleri değiştiği için PA5/PA6'da **hiçbir çakışma YOKTUR**.)*

---

## ✅ CubeMX Kontrol Listesi (v4 Hızlı Kurulum)

- [ ] SYS → Debug: Serial Wire
- [ ] Clock Configuration → HCLK = 84 MHz
- [ ] **PA11, PA12, PB10, PB0** → GPIO_Output (Motor yön)
- [ ] **TIM4** → CH3 (PB8) + CH4 (PB9) PWM Generation
- [ ] **USART1** → PA9 (TX) + PA10 (RX), 115200 baud, *Interrupt enabled!*
- [ ] **I2C1** → PB6 (SCL) + PB7 (SDA)
- [ ] **ADC1** → IN4 (PA4)
- [ ] **PB11** → GPIO_Input (Pull-up, DHT11)
- [ ] **PB5** → GPIO_Input (MPU INT)
- [ ] **PA5** → GPIO_Output (Decoy Power Enable)
- [ ] **PA6** → GPIO_Input (Pi Status)
