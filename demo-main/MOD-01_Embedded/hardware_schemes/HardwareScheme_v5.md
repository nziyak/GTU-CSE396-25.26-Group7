# Hardware Scheme v5.2 — Nihai Pin Atamaları

**Proje:** Autonomous First Responder Fire & Rescue Robot
**Grup:** 7 — CSE 396 Computer Engineering Project
**MCU:** **STM32F103C6** (Blue Pill — 32KB Flash, 10KB RAM)
**Programlama:** ST-Link V2 (SWD üzerinden — PA13/PA14)
**Tarih:** 17 Mayıs 2026
**Önceki sürüm:** v5.1

---

## 1. Sürüm Notları

### v5 → v5.1
| # | Değişiklik | Sebep |
|---|------------|-------|
| 1 | MCU `STM32F103C8T6` → `STM32F103C6` olarak düzeltildi. | Rapordaki C8 yanlış; fiziksel kart C6 (32KB Flash, 10KB RAM). |
| 2 | LED MOSFET sinyali `PC13` → `PB1` olarak taşındı. | PC13 fiziksel olarak çalışmıyor (Blue Pill onboard LED hasarı). |
| 3 | Programlama yöntemi netleştirildi: **ST-Link V2 + SWD**. | PA11/PA12 L298N için kullanıldığından USB programlama yapılamaz. |
| 4 | Pinout doğrulaması eklendi. | Tüm AF yetenekleri STM32F103C6 datasheet ile çapraz kontrol edildi. |

### v5.1 → v5.2
| # | Değişiklik | Sebep |
|---|------------|-------|
| 1 | HC-SR04 #0 TRIG: `PA5` → **`PA15`** olarak taşındı. | PA5, PWR_DECOY_EN ile çakışıyordu. |
| 2 | HC-SR04 #1 TRIG: `PA6` → **`PB3`** olarak taşındı. | PA6, PWR_PI_STATUS ile çakışıyordu. |
| 3 | `PA5` = PWR_DECOY_EN ve `PA6` = PWR_PI_STATUS master tabloya **geri eklendi**. | v5.1'de bu pinler tablodan eksik kalmıştı. |
| 4 | CubeMX'te `SWD only` modu zorunlu kılındı. | PA15 ve PB3'ü GPIO olarak kullanmak için JTAG remap şart. |

---

## 2. STM32F103C6 — Pin Yetenekleri

```
GPIO Pin Bankaları:
  Port A: PA0  - PA15  (16 pin)
  Port B: PB0  - PB15  (16 pin)
  Port C: PC13 - PC15  (3 pin — düşük akım, 3mA max)

ADC1 (12-bit) yetenekli pinler:
  PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7  → ADC1_IN0..IN7
  PB0, PB1                                → ADC1_IN8, ADC1_IN9

I2C1:  PB6 (SCL), PB7 (SDA)
UART1: PA9 (TX), PA10 (RX)
USB:   PA11 (D-), PA12 (D+)  [BU PROJEDE KULLANILMIYOR — L298N IN1/IN2'ye tahsisli]
SWD:   PA13 (SWDIO), PA14 (SWCLK)  [ST-Link bağlantısı — dokunma]

JTAG pinleri (SWD only modunda serbest kalır):
  PA15, PB3, PB4  →  CubeMX'te SYS → Debug → "Serial Wire" seçilince GPIO olarak kullanılabilir.
```

> **Kart Notu:** Bu projedeki Blue Pill kartında **PC13 pini çalışmamaktadır**. PC14 ve PC15 yedek olarak listelendi (test edilmeli).

---

## 3. Tam Pin Atama Tablosu — Master Reference (v5.2)

| Pin   | Fonksiyon                   | Modül          | Tip              |
|-------|-----------------------------|----------------|------------------|
| PA0   | MAX4466 #0 (mikrofon)       | Akustik        | ADC1_IN0         |
| PA1   | MAX4466 #1 (mikrofon)       | Akustik        | ADC1_IN1         |
| PA2   | MAX4466 #2 (mikrofon)       | Akustik        | ADC1_IN2         |
| PA3   | MAX4466 #3 (mikrofon)       | Akustik        | ADC1_IN3         |
| PA4   | MQ2 Analog Out              | Çevre Sensör   | ADC1_IN4         |
| PA5   | PWR_DECOY_EN (12V MOSFET)   | Güç Yönetimi   | GPIO Out         |
| PA6   | PWR_PI_STATUS (Pi izleme)   | Güç Yönetimi   | GPIO In          |
| PA7   | HC-SR04 #2 TRIG             | Mesafe         | GPIO Out         |
| PA8   | HC-SR04 #3 TRIG             | Mesafe         | GPIO Out         |
| PA9   | UART1 TX → Raspberry Pi     | Haberleşme     | AF Push-Pull     |
| PA10  | UART1 RX ← Raspberry Pi     | Haberleşme     | AF Input         |
| PA11  | L298N IN1                   | Motor          | GPIO Out         |
| PA12  | L298N IN2                   | Motor          | GPIO Out         |
| PA13  | SWDIO (ST-Link — REZERVE)   | Sistem         | AF               |
| PA14  | SWCLK (ST-Link — REZERVE)   | Sistem         | AF               |
| PA15  | HC-SR04 #0 TRIG             | Mesafe         | GPIO Out         |
| PB0   | L298N IN4                   | Motor          | GPIO Out         |
| PB1   | LED MOSFET Sinyal           | Aydınlatma     | GPIO Out         |
| PB2   | — (BOOT1 — dokunma)         | Sistem         | —                |
| PB3   | HC-SR04 #1 TRIG             | Mesafe         | GPIO Out         |
| PB4   | BOŞ (yedek)                 | —              | —                |
| PB5   | MPU6050 INT                 | IMU            | GPIO In (EXTI)   |
| PB6   | I2C1 SCL (MPU6050)          | IMU            | AF Open-Drain    |
| PB7   | I2C1 SDA (MPU6050)          | IMU            | AF Open-Drain    |
| PB8   | L298N ENA (PWM)             | Motor          | AF TIM4_CH3      |
| PB9   | L298N ENB (PWM)             | Motor          | AF TIM4_CH4      |
| PB10  | L298N IN3                   | Motor          | GPIO Out         |
| PB11  | DHT11 Data                  | Çevre Sensör   | GPIO In/Out      |
| PB12  | HC-SR04 #0 ECHO             | Mesafe         | GPIO In (FT)     |
| PB13  | HC-SR04 #1 ECHO             | Mesafe         | GPIO In (FT)     |
| PB14  | HC-SR04 #2 ECHO             | Mesafe         | GPIO In (FT)     |
| PB15  | HC-SR04 #3 ECHO             | Mesafe         | GPIO In (FT)     |
| PC13  | ❌ ARIZALI — KULLANILMIYOR  | —              | —                |
| PC14  | BOŞ (test edilmeli)         | —              | —                |
| PC15  | BOŞ (test edilmeli)         | —              | —                |

---

## 4. Motor Sürücü — L298N

| Sinyal | STM32 Pin | Notlar                          |
|--------|-----------|---------------------------------|
| ENA    | PB8       | PWM (TIM4_CH3, sol motor hız)   |
| ENB    | PB9       | PWM (TIM4_CH4, sağ motor hız)   |
| IN1    | PA11      | Sol motor yön                   |
| IN2    | PA12      | Sol motor yön                   |
| IN3    | PB10      | Sağ motor yön                   |
| IN4    | PB0       | Sağ motor yön                   |

**Güç:** +12V (harici batarya / PB2 powerbank), GND ortak
**Motorlar:** M1, M2 (sol grup), M3, M4 (sağ grup)

> **Not:** PA11/PA12 normalde USB D-/D+ pinleridir. Bu projede USB kullanılmadığı için çakışma yoktur.

---

## 5. IMU — MPU6050 (I2C)

| Sinyal | STM32 Pin | Notlar                      |
|--------|-----------|-----------------------------|
| SCL    | PB6       | I2C1 saat (AF Open-Drain)   |
| SDA    | PB7       | I2C1 veri (AF Open-Drain)   |
| INT    | PB5       | Kesme — EXTI5               |

**Güç:** 3.3V, GND
**Kullanım:** Yaw açısı, sıkışma tespiti (stuck detection)
**Pull-up:** I2C hattında 4.7kΩ pull-up direnç gerekli (modül kartında genelde bulunur).

---

## 6. Mesafe Sensörleri — HC-SR04 ×4

| Sensör  | Konum | TRIG Pin | ECHO Pin | Notlar                   |
|---------|-------|----------|----------|--------------------------|
| SR04 #0 | Ön    | **PA15** | PB12     | JTAG remap ile serbest   |
| SR04 #1 | Arka  | **PB3**  | PB13     | JTAG remap ile serbest   |
| SR04 #2 | Sol   | PA7      | PB14     | Temiz pin                |
| SR04 #3 | Sağ   | PA8      | PB15     | Temiz pin                |

**Güç:** Tüm sensörler ortak 5V, GND
**Seviye uyumu:** PB12-PB15 5V-tolerant FT pinler — ECHO sinyali doğrudan bağlanabilir. Ekstra güvenlik için 1kΩ seri direnç önerilir.

> **CubeMX zorunluluğu:** PA15 ve PB3'ü kullanabilmek için CubeMX'te `SYS → Debug → Serial Wire` seçili olmalı.

---

## 7. Ses Sensörleri — MAX4466 ×4 (ADC)

| Sensör     | STM32 Pin | ADC Kanalı |
|------------|-----------|------------|
| MAX4466 #0 | PA0       | ADC1_IN0   |
| MAX4466 #1 | PA1       | ADC1_IN1   |
| MAX4466 #2 | PA2       | ADC1_IN2   |
| MAX4466 #3 | PA3       | ADC1_IN3   |

**Güç:** 3.3V (gürültü için LC filtre önerilir), GND
**Kullanım:** Akustik kaynak yön kestirimi (MOD-03 IIR filtreleme + bearing hesaplama)

> **Not:** Rapor `Platform Summary`'de 3 mikrofon yazıyor; donanım şeması 4 mikrofonu temel alır. MOD-03 ekibinin TDoA algoritmasını 4 mikrofona göre güncellemesi gerekebilir.

---

## 8. Sıcaklık/Nem Sensörü — DHT11

| Sinyal | STM32 Pin | Notlar                                  |
|--------|-----------|------------------------------------------|
| DATA   | PB11      | Tek-hat protokol (10kΩ pull-up gerekli) |

**Güç:** 3.3V veya 5V, GND
**Örnekleme:** Maksimum 1 Hz (DHT11 sınırı).

> **Rapor uyumsuzluğu:** Rapor DHT22 diyor; bu projede **DHT11** kullanılıyor. Protokol benzer ama hassasiyet/aralık farklı (DHT11: ±2°C, ±5%RH). Rapor güncellenmeli.

---

## 9. Duman Sensörü — MQ2

| Sinyal | STM32 Pin | Notlar               |
|--------|-----------|----------------------|
| AO     | PA4       | ADC1_IN4 (analog)    |
| DO     | —         | Kullanılmıyor        |
| VCC    | 5V        | Isıtıcı için 5V şart |
| GND    | GND       |                      |

> **Isınma:** İlk açılışta ~20 saniye ısınma periyodu gerekir. Firmware bu sürede MQ2 okumalarını yok saymalı.

---

## 10. LED Aydınlatma + MOSFET Sürücü

### LED
| Sinyal   | Bağlantı                    |
|----------|-----------------------------|
| Anot (+) | 5V (MOSFET drain üzerinden) |
| Katot (−)| GND                         |

### MOSFET (LED low-side switch)
| Pin    | Bağlantı  | Notlar                                          |
|--------|-----------|-------------------------------------------------|
| Gate   | **PB1**   | STM32 dijital çıkış (TIM3_CH4 PWM yetenekli)   |
| Drain  | LED katot | Yük tarafı                                      |
| Source | GND       | Ortak toprak                                    |

> Gate'e 10kΩ pull-down direnç eklenmesi önerilir (boot sırasında istenmeyen tetikleme önlemek için).
> PB1, TIM3_CH4 yetenekli olduğu için ileride LED parlaklık kontrolü eklenebilir.

---

## 11. Güç Yönetimi — PWR

| Sinyal         | STM32 Pin | Tip       | Notlar                                     |
|----------------|-----------|-----------|--------------------------------------------|
| PWR_DECOY_EN   | **PA5**   | GPIO Out  | 12V motor hattı MOSFET gate tetikleme      |
| PWR_PI_STATUS  | **PA6**   | GPIO In   | Pi 5V hattı izleme (Pi crash tespiti)      |

**Mimari:**
- **PB1 (powerbank 1)** → Pi 5V → STM32 3.3V/5V besleme
- **PB2 (powerbank 2)** → 12V → L298N motor besleme (PA5 MOSFET üzerinden kontrol)

**Pi crash senaryosu:** Pi yazılımsal çökerse PB1 hattı hâlâ sağlıklı olduğu için STM32 çalışmaya devam eder. PA6 LOW'a düşünce STM32 motoru durdurabilir.

---

## 12. Ses Çıkışı — PAM8403 Amplifikatör

| Sinyal        | Bağlantı                          |
|---------------|-----------------------------------|
| L (Sol giriş) | Aux kablo — sol kanal             |
| R (Sağ giriş) | Aux kablo — sağ kanal             |
| L_OUT         | Hoparlör (+)                      |
| GND_OUT       | Hoparlör (−)                      |
| VCC           | 5V                                |
| GND           | GND                               |
| Aux Giriş     | Raspberry Pi USB ses kartı çıkışı |

**Sinyal yolu:** Pi 5 → USB ses kartı → PAM8403 Aux → Hoparlör. STM32 bu zincire dahil değildir.

---

## 13. Güç Bağlantıları — Özet

| Hat  | Kaynak            | Kullanıcılar                                                    |
|------|-------------------|-----------------------------------------------------------------|
| 12V  | PB2 (powerbank 2) | L298N motor besleme (PA5 MOSFET üzerinden açılır)              |
| 5V   | Buck regülatör    | STM32 5V pin, MQ2 ısıtıcı, HC-SR04 ×4, PAM8403, LED hattı     |
| 3.3V | STM32 LDO çıkışı  | MPU6050, MAX4466 ×4, MOSFET lojik, DHT11                       |
| GND  | Ortak             | Tüm modüller (star grounding önerilir)                          |

> **Önemli notlar:**
> - MQ2 ısıtıcı ~150mA çeker — bol akımlı regülatör kullanın.
> - MAX4466'lar gürültüye çok hassas — analog 3.3V hattı motor PWM hattından ayrılmalı.
> - Tüm GND'ler tek bir yıldız noktasında birleştirilmeli.

---

## 14. Haberleşme & Programlama

| Bağlantı            | Açıklama                                      |
|---------------------|-----------------------------------------------|
| UART1 (PA9/PA10)    | STM32 ↔ Raspberry Pi 5 — 115200 baud, 8N1    |
| I2C1 (PB6/PB7)      | STM32 ↔ MPU6050                               |
| **SWD (PA13/PA14)** | **ST-Link V2 → STM32 programlama + debug**    |
| USB Type-C (Pi 5)   | Raspberry Pi 5 güç                            |
| USB (Pi 5)          | USB ses kartı → PAM8403                       |

> **Programlama:** ST-Link V2 + OpenOCD veya STM32CubeIDE. USB programlama yapılmıyor (PA11/PA12 motor pinlerine tahsisli).

---

## 15. Boş / Yedek Pinler

| Pin  | Yetenek               | Not                         |
|------|-----------------------|-----------------------------|
| PB4  | GPIO (JTAG remap)     | SWD modunda otomatik serbest |
| PC14 | GPIO (test edilmeli)  | Düşük akım — sadece lojik   |
| PC15 | GPIO (test edilmeli)  | Düşük akım — sadece lojik   |

> PA15 ve PB3 artık HC-SR04 TRIG'e atandı — yedek listesinden çıkarıldı.

---

## 16. Zamanlayıcı (Timer) Kullanımı

| Timer | Kanal              | Kullanım                                   |
|-------|--------------------|--------------------------------------------|
| TIM2  | —                  | HC-SR04 ECHO mikrosaniye ölçümü (serbest sayaç) |
| TIM3  | CH4 (PB1)          | LED PWM — opsiyonel, parlaklık kontrolü    |
| TIM4  | CH3 (PB8), CH4 (PB9) | L298N ENA/ENB motor PWM                |

---

## 17. CubeMX Kontrol Listesi (v5.2 Hızlı Kurulum)

- [ ] **SYS → Debug: `Serial Wire`** ← PA15, PB3, PB4'ü serbest bırakır (JTAG remap)
- [ ] Clock Configuration → HCLK = 72 MHz (F103C6 max)
- [ ] **PA0–PA3** → ADC1_IN0..IN3 (MAX4466)
- [ ] **PA4** → ADC1_IN4 (MQ2)
- [ ] **PA5** → GPIO_Output (PWR_DECOY_EN)
- [ ] **PA6** → GPIO_Input Pull-down (PWR_PI_STATUS)
- [ ] **PA7, PA8** → GPIO_Output (HC-SR04 TRIG #2, #3)
- [ ] **USART1** → PA9 (TX) + PA10 (RX), 115200 baud, interrupt enabled
- [ ] **PA11, PA12** → GPIO_Output (L298N IN1, IN2)
- [ ] **PA15** → GPIO_Output (HC-SR04 TRIG #0) ← JTAG remap sayesinde kullanılabilir
- [ ] **PB0** → GPIO_Output (L298N IN4)
- [ ] **PB1** → GPIO_Output (LED MOSFET)
- [ ] **PB3** → GPIO_Output (HC-SR04 TRIG #1) ← JTAG remap sayesinde kullanılabilir
- [ ] **PB5** → GPIO_Input (MPU6050 INT, EXTI5)
- [ ] **I2C1** → PB6 (SCL) + PB7 (SDA)
- [ ] **TIM4** → CH3 (PB8) + CH4 (PB9) PWM Generation
- [ ] **PB10** → GPIO_Output (L298N IN3)
- [ ] **PB11** → GPIO_Input Pull-up (DHT11)
- [ ] **PB12–PB15** → GPIO_Input (HC-SR04 ECHO #0–#3)

---

## 18. Bilinen Açık Konular ve Riskler

| Konu | Açıklama |
|------|----------|
| PC13 arızası | Mevcut Blue Pill kartında PC13 ölü. PC14/PC15 de test edilmeli. |
| MAX4466 sensör sayısı | Rapor 3 yazıyor, donanım 4 — MOD-03 algoritması güncellenmeli. |
| DHT11 vs DHT22 | Rapor DHT22 yazıyor, donanım DHT11 — rapor güncellenmeli. |
| STM32F103C6 hafıza | 32KB Flash + 10KB RAM. Tüm firmware modülleri bu sınıra sığmalı. |
| HC-SR04 seviye uyumu | PB12-PB15 FT pinler — yine de 1kΩ seri direnç önerilir. |
| MQ2 ısınma süresi | ~20 saniye, firmware başlangıçta beklemeli. |
| Star grounding | Analog ve dijital güç hatları PCB'de ayrılmalı. |

---

## 19. Pinout Doğrulaması (STM32F103C6 datasheet)

| Kullanılan AF     | Pin      | Datasheet Onayı |
|-------------------|----------|-----------------|
| ADC1_IN0..IN4     | PA0-PA4  | ✓               |
| GPIO Out          | PA5, PA6 | ✓ (güç yönetimi)|
| GPIO Out          | PA7, PA8 | ✓ (HC-SR04 TRIG)|
| USART1_TX         | PA9      | ✓               |
| USART1_RX         | PA10     | ✓               |
| GPIO Out          | PA15     | ✓ (SWD modunda serbest) |
| GPIO Out          | PB3      | ✓ (SWD modunda serbest) |
| I2C1_SCL          | PB6      | ✓               |
| I2C1_SDA          | PB7      | ✓               |
| TIM4_CH3          | PB8      | ✓ (motor PWM)   |
| TIM4_CH4          | PB9      | ✓               |
| TIM3_CH4          | PB1      | ✓ (LED PWM opsiyonel) |
| SWDIO             | PA13     | ✓ (ST-Link)     |
| SWCLK             | PA14     | ✓ (ST-Link)     |
| 5V-tolerant FT    | PB12-PB15| ✓ (HC-SR04 ECHO)|

---

*Şema kaynak: HardwareScheme_v4.pdf (elle çizim) + STM32F103C6 datasheet + grup içi netleştirme.*
*Bu sürüm montaj için onaylıdır.*
