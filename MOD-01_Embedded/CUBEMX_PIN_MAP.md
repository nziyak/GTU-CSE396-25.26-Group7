# MOD-01 STM32CubeMX Pin Konfigürasyon Haritası

> **Bu doküman kodla birebir eşleşir.**  
> STM32CubeMX'te aşağıdaki tabloyu takip ederek pin konfigürasyonunu yapın.  
> MCU: **STM32F401 / STM32F411** (veya F103 — aşağıdaki notlara bakın)

---

## 1. GPIO — Motor Kontrolü (L298N)

Kaynak dosyalar: `main.c` → `MotorConfig_t`, `motor_control.h`

| STM32 Pin | CubeMX Label     | Mod            | Açıklama                        |
|-----------|------------------|----------------|----------------------------------|
| **PA0**   | `MOTOR_A_IN1`    | GPIO_Output    | Sol Motor — Yön 1 (L298N IN1)   |
| **PA1**   | `MOTOR_A_IN2`    | GPIO_Output    | Sol Motor — Yön 2 (L298N IN2)   |
| **PA4**   | `MOTOR_B_IN3`    | GPIO_Output    | Sağ Motor — Yön 1 (L298N IN3)   |
| **PA5**   | `MOTOR_B_IN4`    | GPIO_Output    | Sağ Motor — Yön 2 (L298N IN4)   |

### PWM (Motor Hız Kontrolü)

Kaynak: `main.c` → `pwm_timer = &htim3`

| STM32 Pin | CubeMX Label     | Mod                  | Açıklama                  |
|-----------|------------------|----------------------|---------------------------|
| **PA6**   | `MOTOR_A_ENA`    | TIM3_CH1 (PWM)       | Sol Motor PWM Enable      |
| **PA7**   | `MOTOR_B_ENB`    | TIM3_CH2 (PWM)       | Sağ Motor PWM Enable      |

**TIM3 Ayarları:**
- Prescaler: `83` (84MHz saat için → 1MHz counter)
- Counter Period (ARR): `999` (→ 1kHz PWM frekansı)
- Mode: PWM Generation CH1 + CH2
- Polarity: High

---

## 2. UART — Raspberry Pi 5 İletişimi

Kaynak dosya: `uart_comm.c`, `uart_comm.h`

| STM32 Pin | CubeMX Label | Mod         | Açıklama                    |
|-----------|-------------|-------------|------------------------------|
| **PA2**   | `USART2_TX` | USART2_TX   | STM32 → Pi (Telemetri)      |
| **PA3**   | `USART2_RX` | USART2_RX   | Pi → STM32 (Komutlar)       |

**USART2 Ayarları:**
- Baud Rate: `115200`
- Word Length: 8 Bits
- Stop Bits: 1
- Parity: None
- Mode: TX + RX
- ☑ NVIC → USART2 global interrupt: **Enabled** (RX interrupt için şart!)

---

## 3. I2C — MPU6050 IMU Sensörü

Kaynak dosya: `main.c` → `&hi2c1`, `stuck_detection.c`

| STM32 Pin | CubeMX Label | Mod      | Açıklama           |
|-----------|-------------|----------|---------------------|
| **PB6**   | `I2C1_SCL`  | I2C1_SCL | MPU6050 Clock       |
| **PB7**   | `I2C1_SDA`  | I2C1_SDA | MPU6050 Data        |

**I2C1 Ayarları:**
- Speed Mode: Standard Mode (100 kHz) veya Fast Mode (400 kHz)
- MPU6050 Slave Address: `0x68` (AD0=GND) veya `0x69` (AD0=VCC)

**Donanım Notu:** SDA ve SCL hatlarına **4.7kΩ pull-up dirençleri** (3.3V'a) gereklidir.

---

## 4. ADC — MQ-2 Duman Sensörü

Kaynak dosya: `environment_sensors.c`

| STM32 Pin | CubeMX Label | Mod            | Açıklama                    |
|-----------|-------------|----------------|------------------------------|
| **PA7**   | `MQ2_ANALOG`| ADC1_IN7       | MQ-2 Analog çıkışı           |

> ⚠️ **UYARI:** PA7 hem TIM3_CH2 (Motor B PWM) hem de ADC1_IN7 olarak kullanılıyor!  
> Bu çakışma varsa MQ-2'yi farklı bir ADC pinine taşıyın. Öneriler:
> - **PC0** → ADC1_IN10
> - **PC1** → ADC1_IN11
> - **PB0** → ADC1_IN8
>
> Değiştirirseniz `environment_sensors.c` dosyasında `MQ2_ADC_CHANNEL` define'ını güncelleyin.

**ADC1 Ayarları:**
- Resolution: 12-bit
- Scan Conversion: Disabled
- Continuous Conversion: Disabled
- Discontinuous Conversion: Disabled
- End of Conversion: Single

---

## 5. GPIO — DHT22 Sıcaklık/Nem Sensörü

Kaynak dosya: `environment_sensors.c`

| STM32 Pin | CubeMX Label   | Mod            | Açıklama                        |
|-----------|---------------|----------------|----------------------------------|
| **PB12**  | `DHT22_DATA`  | GPIO_Input     | DHT22 tek-kablo veri hattı       |

**CubeMX'te Ayar:**
- Mode: GPIO_Input (Pull-up)
- Not: Kod runtime'da output/input arasında geçiş yapıyor, CubeMX'te **input** olarak başlatmak yeterli.

**Donanım Notu:** DHT22 data pinine **10kΩ pull-up direnci** (3.3V'a) bağlayın.

---

## 6. GPIO — Güç Yönetimi

Kaynak dosya: `pwr_management.c`

| STM32 Pin | CubeMX Label      | Mod           | Açıklama                              |
|-----------|-------------------|---------------|----------------------------------------|
| **PA5**   | `PWR_DECOY_EN`    | GPIO_Output   | 12V Decoy MOSFET/Röle tetikleme        |
| **PA6**   | `PWR_PI_STATUS`   | GPIO_Input    | Pi 5V durumu izleme                    |

> ⚠️ **UYARI:** PA5 hem Motor B IN4 hem de PWR Decoy olarak tanımlı!  
> PA6 hem Motor A ENA (TIM3_CH1) hem de Pi Status olarak tanımlı!  
>
> **Çözüm:** Güç yönetimi pinlerini başka pinlere taşıyın. Öneriler:
> - `PWR_DECOY_EN` → **PC13** (GPIO Output)
> - `PWR_PI_STATUS` → **PC14** (GPIO Input)
>
> Değiştirirseniz `pwr_management.c` dosyasında `PWR_DECOY_PORT/PIN` define'larını güncelleyin.

---

## 7. Periferik Özeti (CubeMX Connectivity/Analog Sekmesi)

| Periferik | Durum    | Kullanım Amacı                    |
|-----------|----------|-----------------------------------|
| **USART2** | ✅ Enable | Pi ↔ STM32 haberleşme             |
| **I2C1**   | ✅ Enable | MPU6050 IMU sensörü               |
| **TIM3**   | ✅ Enable | Motor PWM (CH1 + CH2)             |
| **ADC1**   | ✅ Enable | MQ-2 duman sensörü                |
| **SYS**    | ✅ Debug  | Serial Wire (SWD) debug           |
| **RCC**    | ✅ HSE    | Harici kristal osilatör (varsa)    |

---

## 8. NVIC (Interrupt) Ayarları

| Interrupt               | Durum    | Neden                                    |
|--------------------------|----------|------------------------------------------|
| USART2 global interrupt  | ✅ Enable | `uart_comm.c` byte-by-byte RX interrupt  |
| TIM3 global interrupt    | ❌ Disable | PWM için interrupt gerekmez              |
| I2C1 event interrupt     | ❌ Disable | Polling kullanıyoruz                     |

---

## 9. Saat Ağacı (Clock Configuration)

- **HCLK:** 84 MHz (STM32F401) veya 100 MHz (STM32F411)
- **APB1:** 42 MHz (USART2 + TIM3 + I2C1 bu bus'ta)
- **APB2:** 84 MHz (ADC1 bu bus'ta)

---

## 🔴 Bilinen Pin Çakışmaları ve Çözümleri

| Çakışma | Pin | Kullanım 1 | Kullanım 2 | Çözüm |
|---------|-----|-----------|------------|-------|
| #1 | PA5 | Motor B IN4 | PWR Decoy Enable | Decoy'u **PC13**'e taşı |
| #2 | PA6 | TIM3_CH1 (Motor PWM) | Pi Status Input | Pi Status'u **PC14**'e taşı |
| #3 | PA7 | TIM3_CH2 (Motor PWM) | ADC1_IN7 (MQ-2) | MQ-2'yi **PB0** (ADC1_IN8)'e taşı |

> Bu çakışmaları çözdükten sonra ilgili `.c` dosyalarındaki `#define` satırlarını güncelleyin.

---

## ✅ CubeMX Kontrol Listesi

Aşağıdaki adımları sırayla yapın:

- [ ] Yeni proje aç → MCU: STM32F401CCU6 (veya kullandığınız chip)
- [ ] SYS → Debug: Serial Wire
- [ ] RCC → HSE: Crystal/Ceramic Resonator (harici kristal varsa)
- [ ] Clock Configuration → HCLK = 84 MHz
- [ ] PA0, PA1, PA4 → GPIO_Output (Motor yön pinleri)
- [ ] PA5 → **Çakışma var!** Motor IN4 için kullanmayın, farklı pin seçin VEYA Decoy'u PC13'e taşıyın
- [ ] TIM3 → CH1 (PA6) + CH2 (PA7) PWM Generation — **ama PA6/PA7 çakışıyorsa** alternate pinlere taşıyın
- [ ] USART2 → PA2 (TX) + PA3 (RX), 115200 baud, interrupt enabled
- [ ] I2C1 → PB6 (SCL) + PB7 (SDA), 100kHz
- [ ] ADC1 → IN8 (PB0) — MQ-2 (çakışma çözümü sonrası)
- [ ] PB12 → GPIO_Input, Pull-up — DHT22
- [ ] PC13 → GPIO_Output — PWR Decoy (çakışma çözümü)
- [ ] PC14 → GPIO_Input, Pull-down — Pi Status (çakışma çözümü)
- [ ] NVIC → USART2 interrupt: Enable
- [ ] Generate Code → Proje adı: MOD01_Firmware
