# Hardware Scheme v5.1 — Nihai Pin Atamaları

**Proje:** Autonomous First Responder Fire & Rescue Robot
**Grup:** 7 — CSE 396 Computer Engineering Project
**MCU:** **STM32F103C6** (Blue Pill — 32KB Flash, 10KB RAM)
**Programlama:** ST-Link V2 (SWD üzerinden — PA13/PA14)
**Tarih:** 17 Mayıs 2026
**Önceki sürüm:** v5

---

## 1. Sürüm Notları (v5 → v5.1)

| # | Değişiklik | Sebep |
|---|------------|-------|
| 1 | MCU `STM32F103C8T6` → **`STM32F103C6`** olarak düzeltildi. | Rapordaki C8 yanlış; fiziksel kart C6 (32KB Flash, 10KB RAM). |
| 2 | LED MOSFET sinyali `PC13` → **`PB1`** olarak taşındı. | PC13 fiziksel olarak çalışmıyor (Blue Pill onboard LED hasarı). PB1 boş, 25mA sürücü kapasiteli, ADC yetenekli (gerekirse). |
| 3 | Programlama yöntemi netleştirildi: **ST-Link V2 + SWD**. | PA11/PA12 L298N için kullanıldığından USB programlama zaten yapılamaz; ST-Link standart yöntem. |
| 4 | Pinout doğrulaması eklendi (Bölüm 17). | Tüm AF yetenekleri STM32F103C6 datasheet ile çapraz kontrol edildi. |

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

I2C1: PB6 (SCL), PB7 (SDA)
UART1: PA9 (TX), PA10 (RX)
USB: PA11 (D-), PA12 (D+)  [BU PROJEDE KULLANILMIYOR — L298N IN1/IN2'ye tahsisli]
SWD Debug: PA13 (SWDIO), PA14 (SWCLK)  [ST-Link bağlantısı]
JTAG (kullanılmıyor — SWD yeterli): PA15, PB3, PB4 GPIO olarak boşaltılabilir
```

> **Kart Notu:** Bu projedeki Blue Pill kartında **PC13 pini çalışmamaktadır** (donanımsal arıza). PC13/PC14/PC15 atamadan çıkarıldı; sadece PC14 ve PC15 yedek olarak listelendi (test edilmeli).

---

## 3. Tam Pin Atama Tablosu (Master Reference)

| Pin   | Fonksiyon                | Modül         | Tip           |
|-------|--------------------------|---------------|---------------|
| PA0   | MAX4466 #0 (mikrofon)    | Akustik       | ADC1_IN0      |
| PA1   | MAX4466 #1 (mikrofon)    | Akustik       | ADC1_IN1      |
| PA2   | MAX4466 #2 (mikrofon)    | Akustik       | ADC1_IN2      |
| PA3   | MAX4466 #3 (mikrofon)    | Akustik       | ADC1_IN3      |
| PA4   | MQ2 Analog Out           | Çevre Sensör  | ADC1_IN4      |
| PA5   | HC-SR04 #0 TRIG          | Mesafe        | GPIO Out      |
| PA6   | HC-SR04 #1 TRIG          | Mesafe        | GPIO Out      |
| PA7   | HC-SR04 #2 TRIG          | Mesafe        | GPIO Out      |
| PA8   | HC-SR04 #3 TRIG          | Mesafe        | GPIO Out      |
| PA9   | UART1 TX → Raspberry Pi  | Haberleşme    | AF Push-Pull  |
| PA10  | UART1 RX ← Raspberry Pi  | Haberleşme    | AF Input      |
| PA11  | L298N IN1                | Motor         | GPIO Out      |
| PA12  | L298N IN2                | Motor         | GPIO Out      |
| PA13  | SWDIO (ST-Link — REZERVE)| Sistem        | AF            |
| PA14  | SWCLK (ST-Link — REZERVE)| Sistem        | AF            |
| PA15  | BOŞ (JTAG remap gerekli) | —             | —             |
| PB0   | L298N IN4                | Motor         | GPIO Out      |
| PB1   | **LED MOSFET Sinyal**    | **Aydınlatma**| **GPIO Out**  |
| PB3   | BOŞ (JTAG remap gerekli) | —             | —             |
| PB4   | BOŞ (JTAG remap gerekli) | —             | —             |
| PB5   | MPU6050 INT              | IMU           | GPIO In (EXTI)|
| PB6   | I2C1 SCL (MPU6050)       | IMU           | AF Open-Drain |
| PB7   | I2C1 SDA (MPU6050)       | IMU           | AF Open-Drain |
| PB8   | L298N ENA (PWM)          | Motor         | AF Tim4_CH3   |
| PB9   | L298N ENB (PWM)          | Motor         | AF Tim4_CH4   |
| PB10  | L298N IN3                | Motor         | GPIO Out      |
| PB11  | DHT11 Data               | Çevre Sensör  | GPIO In/Out   |
| PB12  | HC-SR04 #0 ECHO          | Mesafe        | GPIO In (FT)  |
| PB13  | HC-SR04 #1 ECHO          | Mesafe        | GPIO In (FT)  |
| PB14  | HC-SR04 #2 ECHO          | Mesafe        | GPIO In (FT)  |
| PB15  | HC-SR04 #3 ECHO          | Mesafe        | GPIO In (FT)  |
| PC13  | ❌ ARIZALI — KULLANILMIYOR | —           | —             |
| PC14  | BOŞ (test edilmeli)      | —             | —             |
| PC15  | BOŞ (test edilmeli)      | —             | —             |

---

## 4. Motor Sürücü — L298N

| Sinyal | STM32 Pin | Notlar                  |
|--------|-----------|-------------------------|
| ENA    | PB8       | PWM (Tim4_CH3, sol motor hız) |
| ENB    | PB9       | PWM (Tim4_CH4, sağ motor hız) |
| IN1    | PA11      | Sol motor yön           |
| IN2    | PA12      | Sol motor yön           |
| IN3    | PB10      | Sağ motor yön           |
| IN4    | PB0       | Sağ motor yön           |

**Güç:** +12V (harici batarya), GND ortak
**Motorlar:** M1, M2 (sol grup), M3, M4 (sağ grup)

> **Not:** PA11/PA12 normalde USB D-/D+ pinleridir. Bu projede USB kullanılmadığı için (programlama ST-Link ile yapılıyor) çakışma yoktur.

---

## 5. IMU — MPU6050 (I2C)

| Sinyal | STM32 Pin | Notlar             |
|--------|-----------|--------------------|
| SCL    | PB6       | I2C1 saat (AF Open-Drain) |
| SDA    | PB7       | I2C1 veri (AF Open-Drain) |
| INT    | PB5       | Kesme — EXTI5      |

**Güç:** 3.3V, GND
**Kullanım:** Yaw açısı, sıkışma tespiti (stuck detection)
**Pull-up:** I2C hattında 4.7kΩ pull-up direnç gerekli (modül kartında genelde bulunur).

---

## 6. Mesafe Sensörleri — HC-SR04 ×4

| Sensör  | Konum  | TRIG Pin | ECHO Pin |
|---------|--------|----------|----------|
| SR04 #0 | Ön     | PA5      | PB12     |
| SR04 #1 | Arka   | PA6      | PB13     |
| SR04 #2 | Sol    | PA7      | PB14     |
| SR04 #3 | Sağ    | PA8      | PB15     |

**Güç:** Tüm sensörler ortak 5V, GND
**Seviye uyumu:** PB12-PB15 5V-tolerant FT pinleri olduğu için ECHO sinyali doğrudan bağlanabilir. Ekstra güvenlik için 1kΩ seri direnç önerilir.

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

> **Not:** Rapor `Platform Summary`'de 3 mikrofon yazıyor; donanım şeması ve bu sürüm 4 mikrofonu temel alır. MOD-03 ekibinin TDoA algoritmasını 4 mikrofona göre güncellemesi gerekebilir.

---

## 8. Sıcaklık/Nem Sensörü — DHT11

| Sinyal | STM32 Pin | Notlar                                |
|--------|-----------|---------------------------------------|
| DATA   | PB11      | Tek-hat protokol (10kΩ pull-up gerekli) |

**Güç:** 3.3V veya 5V, GND
**Örnekleme:** Maksimum 1 Hz (DHT11 sınırı).

> **Rapor uyumsuzluğu:** Rapor DHT22 diyor; bu projede **DHT11** kullanılıyor. Protokol benzer ama hassasiyet/aralık farklı (DHT11: ±2°C, ±5%RH).

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

> **v5.1 değişikliği:** PC13 fiziksel olarak çalışmadığı için LED sinyali **PB1**'e taşındı. PB1 boş, ADC yetenekli ama burada dijital çıkış olarak kullanılıyor (25mA max sürücü kapasiteli).

### LED
| Sinyal      | Bağlantı                   |
|-------------|----------------------------|
| Anot (+)    | 5V (MOSFET drain üzerinden)|
| Katot (−)   | GND                        |

### MOSFET (LED low-side switch)
| Pin     | Bağlantı       | Notlar                                |
|---------|----------------|---------------------------------------|
| Gate    | **PB1**        | STM32 dijital çıkış (PWM yetenekli)   |
| Drain   | LED katot      | Yük tarafı                            |
| Source  | GND            | Ortak toprak                          |
| VCC*    | 3.3V (lojik)   | Modül kart kullanılıyorsa             |
| VIN*    | 5V             | LED besleme hattı                     |

*VCC/VIN sadece hazır MOSFET modülü için geçerli. Çıplak N-kanal MOSFET'te yalnızca Gate/Drain/Source bağlanır; Gate'e 10kΩ pull-down direnç eklenmesi önerilir (boot sırasında istenmeyen tetikleme önlemek için).

> **Bonus:** PB1 PWM yetenekli (TIM3_CH4) olduğu için ileride LED parlaklık kontrolü eklenebilir.

---

## 11. Ses Çıkışı — PAM8403 Amplifikatör

| Sinyal       | Bağlantı                |
|--------------|-------------------------|
| L (Sol giriş)| Aux kablo — sol kanal   |
| R (Sağ giriş)| Aux kablo — sağ kanal   |
| L_OUT        | Hoparlör (+)            |
| GND_OUT      | Hoparlör (−)            |
| VCC          | 5V                      |
| GND          | GND                     |
| Aux Giriş    | Raspberry Pi USB ses kartı çıkışı |

**Sinyal yolu:** Pi 5 → USB ses kartı → PAM8403 Aux → Hoparlör. STM32 ses zincirine dahil değildir.

---

## 12. Güç Bağlantıları — Özet

| Hat   | Kaynak         | Kullanıcılar                                              |
|-------|----------------|-----------------------------------------------------------|
| 12V   | Harici batarya | L298N motor besleme                                       |
| 5V    | Buck regülatör | STM32 5V pin (dahili LDO→3.3V), MQ2 ısıtıcı, HC-SR04 ×4, PAM8403, LED hattı, MOSFET VIN |
| 3.3V  | STM32 LDO çıkışı | MPU6050, MAX4466 ×4, MOSFET lojik VCC, DHT11             |
| GND   | Ortak          | Tüm modüller (star grounding önerilir)                    |

> **Önemli:**
> - MQ2 ısıtıcı ~150mA çeker — ayrı 5V hattı veya bol akımlı regülatör kullanın.
> - MAX4466'lar gürültüye çok hassas — analog 3.3V hattı motor PWM hattından ayrılmalı.
> - Tüm GND'ler tek bir yıldız noktasında birleştirilmelidir.

---

## 13. Haberleşme & Programlama

| Bağlantı       | Açıklama                                  |
|----------------|-------------------------------------------|
| UART1 (PA9/PA10) | STM32 ↔ Raspberry Pi 5 — 115200 baud, 8N1 |
| I2C1 (PB6/PB7) | STM32 ↔ MPU6050                           |
| **SWD (PA13/PA14)** | **ST-Link V2 → STM32 programlama + debug** |
| USB Type-C (Pi 5) | Raspberry Pi 5 güç                      |
| USB (Pi 5)     | USB ses kartı → PAM8403                   |

> **Programlama:** ST-Link V2 + OpenOCD veya STM32CubeIDE. USB programlama YAPILMIYOR (PA11/PA12 motor pinlerine tahsis edildi).

---

## 14. Boş / Yedek Pinler

İleride sensör veya genişleme için müsait pinler:

| Pin   | Yetenek                       | Not                              |
|-------|-------------------------------|----------------------------------|
| PA15  | GPIO (JTAG remap gerekli)     | SWD aktif olduğu için AFIO remap gerekir |
| PB3   | GPIO (JTAG remap gerekli)     | AFIO remap gerektirir            |
| PB4   | GPIO (JTAG remap gerekli)     | AFIO remap gerektirir            |
| PC14  | GPIO (test edilmeli)          | Düşük akım — sadece lojik sinyal |
| PC15  | GPIO (test edilmeli)          | Düşük akım — sadece lojik sinyal |

> **Uyarı:** PC13 arızalı olduğu için PC14/PC15'in de test edilmesi önerilir (aynı kart kalitesinde olabilir).

---

## 15. Zamanlayıcı (Timer) Kullanımı

| Timer  | Kanal       | Kullanım                              |
|--------|-------------|---------------------------------------|
| TIM2   | —           | Sistem zamanı / HC-SR04 ECHO mikrosaniye ölçümü |
| TIM3   | CH4 (PB1)   | LED PWM (opsiyonel, parlaklık kontrolü) |
| TIM4   | CH3 (PB8), CH4 (PB9) | L298N ENA/ENB motor PWM      |

---

## 16. Bilinen Açık Konular ve Riskler

- **PC13 arızası:** Mevcut Blue Pill kartında PC13 ölü. Yedek kart varsa pin testi yapılmalı; ileride farklı bir kart kullanılırsa PC14/PC15 de test edilmeli.
- **MAX4466 sensör sayısı:** Rapor 3, donanım 4 — MOD-03 algoritması güncellenmeli.
- **DHT11 vs DHT22:** Rapor DHT22, donanım DHT11 — rapor güncellenmeli.
- **STM32F103C6 hafıza sınırı:** 32KB Flash + 10KB RAM. Tüm firmware modüllerinin (motor + sensör + UART + FSM) bu sınırlara sığması gerekir. STM32F103C8 (64KB Flash) gerekirse upgrade düşünülebilir.
- **HC-SR04 seviye uyumu:** PB12-PB15 5V-tolerant FT pinler — yine de 1kΩ seri direnç önerilir.
- **MQ2 ısınma süresi:** ~20 saniye, firmware başlangıçta beklemeli.
- **Star grounding:** Analog (MAX4466, MQ2) ve dijital güç hatları PCB'de ayrılmalı.

---

## 17. Pinout Doğrulaması (STM32F103C6 datasheet)

Tüm atamalar STM32F103C6 pinout şeması ile çapraz kontrol edildi:

| Kullanılan AF | Pin   | Datasheet Onayı |
|---------------|-------|-----------------|
| ADC1_IN0..IN4 | PA0-PA4 | ✓               |
| USART1_TX     | PA9   | ✓               |
| USART1_RX     | PA10  | ✓               |
| I2C1_SCL      | PB6   | ✓               |
| I2C1_SDA      | PB7   | ✓               |
| TIM4_CH3      | PB8   | ✓ (motor PWM)   |
| TIM4_CH4      | PB9   | ✓ (motor PWM)   |
| TIM3_CH4      | PB1   | ✓ (LED PWM opsiyonel) |
| SWDIO         | PA13  | ✓ (ST-Link)     |
| SWCLK         | PA14  | ✓ (ST-Link)     |
| 5V-tolerant FT | PB12-PB15 | ✓ (HC-SR04 ECHO için kritik) |

---

*Şema kaynak: HardwareScheme_v4.pdf (elle çizim) + STM32F103C6 mischianti pinout + grup içi netleştirme.*
*Bu sürüm üretim için onaylıdır.*
