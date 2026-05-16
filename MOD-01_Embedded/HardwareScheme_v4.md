# Hardware Scheme v4 — Pin Atamaları

---

## Genel GPIO Notları

```
x6 GPIO pins
  A: PA10 - PA15
  B: PB0  - PB11
  C: PC13 - PC15

** ADC (Analog to Digital Convert)
  A: PA0 - PA7
  B: PB0 - PB1
```

> Not: LED ve hoparlör fener direk bağlanıp gücüne kesilir.

---

## Mikroişlemci — STM32

| Fonksiyon       | Pin     |
|-----------------|---------|
| UART TX         | PA9     |
| UART RX         | PA10    |
| I2C SCL         | PB6     |
| I2C SDA         | PB7     |
| DHT11 Data      | PB11    |
| MQ2 Analog Out  | PA4     |
| Tim2            | —       |
| Tim3            | —       |
| Debug / Boot    | B3, B4, A13, A14 |

---

## Motor Sürücü — L298N

| Sinyal | STM32 Pin |
|--------|-----------|
| ENA    | PB8       |
| ENB    | PB9       |
| IN1    | PA11      |
| IN2    | PA12      |
| IN3    | PB10      |
| IN4    | PB0       |

**Güç:** +12V, GND  
**Motorlar:** M1, M2, M3, M4

---

## IMU — MPU6050 (I2C)

| Sinyal | STM32 Pin |
|--------|-----------|
| SCL    | PB6       |
| SDA    | PB7       |
| INT    | PB5       |

**Güç:** 3.3V, GND

---

## Mesafe Sensörleri — HC-SR04 x4

| Sensör  | VCC Pin | TRIG Pin |
|---------|---------|----------|
| SR04 #0 | PB12    | PA5      |
| SR04 #1 | PB13    | PA6      |
| SR04 #2 | PB14    | PA7      |
| SR04 #3 | PB15    | PA8      |

> Not: HC-SR04 "mesafe" (resafe) olarak belirtilmiş — muhtemelen "mesafe" kelimesinin kısaltması.

---

## Ses Sensörleri — MAX4466 x4 (ADC)

| Sensör   | STM32 ADC Pin |
|----------|---------------|
| MAX4466 #0 | A0          |
| MAX4466 #1 | A1          |
| MAX4466 #2 | A2          |
| MAX4466 #3 | A3          |

---

## Sıcaklık/Nem Sensörü — DHT11

| Sinyal | STM32 Pin |
|--------|-----------|
| DATA   | PB11      |

---

## Duman Sensörü — MQ2

| Sinyal | STM32 Pin | Notlar         |
|--------|-----------|----------------|
| AO     | PA4       | ADC girişi     |
| DO     | —         | Dijital çıkış  |
| VCC    | 5V        |                |
| GND    | GND       |                |

---

## Hoparlör — PAM8403

| Sinyal       | Bağlantı         |
|--------------|------------------|
| Sol (L)      | Hoparlör (L)     |
| Sağ (R)      | Hoparlör (R)     |
| VCC          | 5V               |
| GND          | GND              |
| Aux Giriş    | USB-SES kartı    |

---

## Mikrofon Amplifikatörü — DRV199 (comp)

- Bağlantı: STM32 ADC pinleri
- Güç: 5V

---

## LED

| Sinyal    | STM32 Pin |
|-----------|-----------|
| Dijital   | P11 (MOSFET üzerinden) |

---

## MOSFET (LED Sürücü)

| Sinyal | Bağlantı        |
|--------|-----------------|
| VCC    | 3.3V            |
| SIG    | STM32 P11       |
| VIN    | 5V              |
| GND    | GND             |

---

## Güç Bağlantıları — Özet

| Hat  | Kaynak   | Kullanım                          |
|------|----------|-----------------------------------|
| 5V   | Regülatör| STM32, MQ2, PAM8403, MOSFET VIN  |
| 3.3V | STM32    | MPU6050, MOSFET VCC               |
| 12V  | Harici   | L298N motor sürücü                |
| GND  | Ortak    | Tüm bileşenler                    |

---

## USB / Haberleşme

| Bağlantı       | Açıklama                      |
|----------------|-------------------------------|
| Type-C (PS1)   | Güç / programlama             |
| Type-C (PS2)   | İkinci port                   |
| USB → SES      | Ses kartı (probuf bağlantısı) |
| UART (TX/RX)   | PA9 / PA10                    |

---

*Şema: HardwareScheme_v4.pdf — elle çizim, 2 sayfa*
