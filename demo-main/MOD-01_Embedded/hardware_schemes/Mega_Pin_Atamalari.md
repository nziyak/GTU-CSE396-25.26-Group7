
--- Page 1 ---
        Arduino Mega 2560 - Pin Atamalari
CSE 396 Group 7 - Autonomous Fire & Rescue Robot

    Genel Bilgi

    Mikrodenetleyici                Arduino Mega 2560
    Onceki MCU                      STM32F103C6 (SWD pinleri hasar gordugu icin degistirildi)
    Pi Baglantisi                   USB kablo (/dev/ttyUSB0)
    Baud Rate                       115200
    Veri Formati                    JSON, \n terminator

    Sensor Pin Atamalari

    Sensor                          Pin                                VCC      GND

    HC-SR04 On Mesafe               TRIG: 9, ECHO: 10                  5V       GND
    HC-SR04 Arka Mesafe             TRIG: 7, ECHO: 8                   5V       GND
    MAX4466 Mikrofon 1              A0 (analog)                        3.3V     GND
    MAX4466 Mikrofon 2              A1 (analog)                        3.3V     GND
    MQ-2 Duman Sensoru              A2 (analog)                        5V       GND
    DHT11 Sicaklik/Nem              Pin 4 (dijital)                    5V       GND
    MPU6050 IMU                     SDA: 20, SCL: 21 (I2C)             3.3V     GND

    Motor Surucu (L298N) Pin Atamalari

    L298N Pini            Mega Pin                      Aciklama

    ENA                   Pin 5 (PWM)                   Sol motor hiz kontrolu
    IN1                   Pin 22                        Sol motor yon 1
    IN2                   Pin 23                        Sol motor yon 2
    IN3                   Pin 24                        Sag motor yon 1
    IN4                   Pin 25                        Sag motor yon 2
    ENB                   Pin 6 (PWM)                   Sag motor hiz kontrolu

    Guc Baglantilari

    Bilesen                         Baglanti


--- Page 2 ---
Arduino Mega          USB (Pi'den) veya harici 7-12V
L298N VS (12V)        Harici pil/powerbank (9-12V onerilir)
L298N GND             Pil GND ve Mega GND ortak olmali
HC-SR04 (on/arka)     Mega 5V pini
MAX4466 mikrofonlar   Mega 3.3V pini
MQ-2 duman sensoru    Mega 5V pini
DHT11                 Mega 5V pini
MPU6050               Mega 3.3V pini (5V tolere etmiyor)


--- Page 3 ---
Haberlesme Protokolu
Komutlar (Pi → Mega)
Pi'den Mega'ya tek satir, ASCII komut, satir sonunda \n karakteri gonderilir. Mega her komutu aldiginda 2
saniye boyunca o yonde hareket eder, sonra otomatik durur.
Komut                Davranis

FORWARD\n            2 saniye ileri git
BACKWARD\n           2 saniye geri git
LEFT\n               2 saniye sola don (tank metodu)
RIGHT\n              2 saniye saga don (tank metodu)
STOP\n               Hemen durdur

Telemetri (Mega → Pi)
Mega, her 300 milisaniyede bir asagidaki formatta JSON satiri gonderir:
{"dist_front": 45, "dist_back": 80, "mic1": 340,
"mic2": 335, "smoke": 420, "yaw": 1.2, "pitch": -0.5,
"roll": 0.3, "temp": 24.5, "hum": 45.0, "connected": true}

JSON Alan Aciklamalari
Alan         Tip      Aciklama

dist_front   int      On mesafe sensoru olcumu (cm)
dist_back    int      Arka mesafe sensoru olcumu (cm)
mic1         int      Mikrofon 1 analog deger (0-1023)
mic2         int      Mikrofon 2 analog deger (0-1023)
smoke        int      MQ-2 duman sensoru analog deger (0-1023)
yaw          float    MPU6050 Z ekseni acisi (derece)
pitch        float    MPU6050 Y ekseni acisi (derece)
roll         float    MPU6050 X ekseni acisi (derece)
temp         float    DHT11 sicaklik (Celsius)
hum          float    DHT11 nem (%)
connected    bool     Mega baglantisi aktif (her zaman true)

Onemli Notlar
• STM32F103C6 (Blue Pill) ile baslanan donanim, SWD pinlerinin elektriksel hasari sebebiyle programlanamaz hale
geldi. Demo oncesi Arduino Mega 2560'a gecildi.


--- Page 4 ---
• DHT22 yerine DHT11 kullanildi. (Modul uzerinde DHT11 yazdigi tespit edildi.)
• L298N icin 9V kaynak yetersiz kaldi, motorlar dusuk gucte dondu. 12V kaynak onerilir.
• MPU6050 baslangicta SCL/SDA baglantilari ters yapilmisti, fiziksel olarak duzeltildi.
• MPU6050 5V tolere etmedigi icin VCC 3.3V'a baglanmistir.
• Arduino AVR mimaride snprintf %f formatlamasini desteklemedigi icin float degerler Serial.print(value, 1) ile
yazdirilmistir.
• Mesafe sensorlerinin pulseIn timing'i MPU6050 I2C iletisimine duyarli oldugundan, MPU once okunup ardindan
mesafe okumasi yapilmaktadir.
• Motorlar baslangicta hareket etmez; sadece Pi'den komut geldiginde 2 saniye sureyle calisir, sonra otomatik durur.