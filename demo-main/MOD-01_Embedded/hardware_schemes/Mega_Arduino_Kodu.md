
--- Page 1 ---
           Arduino Mega 2560 - Tam Kod
CSE 396 Group 7 - Autonomous Fire & Rescue Robot

    Aciklama
    Asagidaki kod Arduino Mega 2560 uzerine yuklenir. Pi'den USB uzerinden gelen komutlari dinler ve her komut
    icin 2 saniye boyunca ilgili hareketi gerceklestirir. Ayni zamanda her 300 milisaniyede bir tum sensor verilerini
    JSON formatinda Pi'ye geri gonderir.
    Gerekli Kutuphaneler
    • Wire.h (Arduino ile birlikte gelir)
    • MPU6050_light by rfetick
    • DHT sensor library by Adafruit
    • Adafruit Unified Sensor
    Arduino IDE'de: Sketch → Include Library → Manage Libraries menusunden yuklenebilir.
    Tam Arduino Kodu

    #include <Wire.h>
    #include <MPU6050_light.h>
    #include <DHT.h>

    // Sensor pinleri
    #define TRIG_FRONT 9
    #define ECHO_FRONT 10
    #define TRIG_BACK  7
    #define ECHO_BACK  8
    #define MIC1_PIN A0
    #define MIC2_PIN A1
    #define MQ2_PIN A2
    #define DHT_PIN  4
    #define DHT_TYPE DHT11

    // Motor pinleri
    #define ENA 5
    #define IN1 22
    #define IN2 23
    #define IN3 24
    #define IN4 25
    #define ENB 6

    const int SPEED = 200;
    const unsigned long KOMUT_SURE_MS = 2000;

    MPU6050 mpu(Wire);
    DHT dht(DHT_PIN, DHT_TYPE);
    bool mpu_ok = false;

    unsigned long komutBaslangic = 0;
    bool komutAktif = false;

    // ==================== MOTOR ====================
    void setMotors(int leftDir, int rightDir) {
    if (leftDir > 0) {
digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    } else if (leftDir < 0) {
digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    } else {
 digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
    }

    if (rightDir > 0) {


--- Page 2 ---
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
 } else if (rightDir < 0) {
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
 } else {
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
 }

 analogWrite(ENA, leftDir == 0 ? 0 : SPEED);
 analogWrite(ENB, rightDir == 0 ? 0 : SPEED);
}

void ileri()   { setMotors(1, 1); }
void geri()    { setMotors(-1, -1); }
void dur()     { setMotors(0, 0); }
void sagaDon() { setMotors(1, -1); }
void solaDon() { setMotors(-1, 1); }

// ==================== SENSOR ====================
int readDistance(int trigPin, int echoPin) {
 digitalWrite(trigPin, LOW);
 delayMicroseconds(2);
 digitalWrite(trigPin, HIGH);
 delayMicroseconds(10);
 digitalWrite(trigPin, LOW);

 unsigned long timeout = micros() + 30000;
 while (digitalRead(echoPin) == LOW) {
  if (micros() > timeout) return 0;
 }
 unsigned long start = micros();
 while (digitalRead(echoPin) == HIGH) {
  if (micros() > timeout) return 0;
 }
 return (micros() - start) / 58;
}

void sendSensorData() {
 float yaw = 0.0, pitch = 0.0, roll = 0.0;
 if (mpu_ok) {
  mpu.update();
  yaw = mpu.getAngleZ();
  pitch = mpu.getAngleY();
  roll = mpu.getAngleX();
  if (isnan(yaw)) yaw = 0.0;
  if (isnan(pitch)) pitch = 0.0;
  if (isnan(roll)) roll = 0.0;
 }

 int front = readDistance(TRIG_FRONT, ECHO_FRONT);
 delay(10);
 int back = readDistance(TRIG_BACK, ECHO_BACK);

 int mic1 = analogRead(MIC1_PIN);
 int mic2 = analogRead(MIC2_PIN);
 int smoke = analogRead(MQ2_PIN);

 float temp = dht.readTemperature();
 float hum = dht.readHumidity();
 if (isnan(temp)) temp = 0.0;
 if (isnan(hum)) hum = 0.0;

 Serial.print("{\"dist_front\":");
 Serial.print(front);
 Serial.print(",\"dist_back\":");
 Serial.print(back);
 Serial.print(",\"mic1\":");
 Serial.print(mic1);
 Serial.print(",\"mic2\":");
 Serial.print(mic2);
 Serial.print(",\"smoke\":");
 Serial.print(smoke);


--- Page 3 ---
 Serial.print(",\"yaw\":");
 Serial.print(yaw, 1);
 Serial.print(",\"pitch\":");
 Serial.print(pitch, 1);
 Serial.print(",\"roll\":");
 Serial.print(roll, 1);
 Serial.print(",\"temp\":");
 Serial.print(temp, 1);
 Serial.print(",\"hum\":");
 Serial.print(hum, 1);
 Serial.print(",\"connected\":true}");
 Serial.println();
}

// ==================== KOMUT ====================
void komutuUygula(String cmd) {
 cmd.trim();
 cmd.toUpperCase();

 if (cmd == "FORWARD") {
  ileri();
  komutBaslangic = millis();
  komutAktif = true;
 } else if (cmd == "BACKWARD") {
  geri();
  komutBaslangic = millis();
  komutAktif = true;
 } else if (cmd == "LEFT") {
  solaDon();
  komutBaslangic = millis();
  komutAktif = true;
 } else if (cmd == "RIGHT") {
  sagaDon();
  komutBaslangic = millis();
  komutAktif = true;
 } else if (cmd == "STOP") {
  dur();
  komutAktif = false;
 }
}

// ==================== SETUP ====================
void setup() {
 Serial.begin(115200);

 pinMode(TRIG_FRONT, OUTPUT);
 pinMode(ECHO_FRONT, INPUT);
 pinMode(TRIG_BACK, OUTPUT);
 pinMode(ECHO_BACK, INPUT);
 digitalWrite(TRIG_FRONT, LOW);
 digitalWrite(TRIG_BACK, LOW);

 pinMode(ENA, OUTPUT);
 pinMode(ENB, OUTPUT);
 pinMode(IN1, OUTPUT);
 pinMode(IN2, OUTPUT);
 pinMode(IN3, OUTPUT);
 pinMode(IN4, OUTPUT);

 Wire.begin();
 byte status = mpu.begin();
 if (status == 0) {
  delay(1000);
  mpu.calcOffsets();
  mpu_ok = true;
 }

 dht.begin();
 delay(2000);

 dur();


--- Page 4 ---
}

// ==================== LOOP ====================
unsigned long sonSensorGonderim = 0;

void loop() {
 // Pi'den komut kontrol
 if (Serial.available() > 0) {
  String cmd = Serial.readStringUntil('\n');
  komutuUygula(cmd);
 }

 // Komut suresi doldu mu
 if (komutAktif && (millis() - komutBaslangic >= KOMUT_SURE_MS)) {
  dur();
  komutAktif = false;
 }

 // Sensor verisi gonder (300ms)
 if (millis() - sonSensorGonderim >= 300) {
  sendSensorData();
  sonSensorGonderim = millis();
 }
}


--- Page 5 ---
Yukleme Adimlari
1. Arduino IDE'yi ac (2.x veya sonrasi)
2. Tools → Board → Arduino AVR Boards → Arduino Mega or Mega 2560 sec
3. Tools → Processor → ATmega2560 (Mega 2560) sec
4. Tools → Port → Mega'nin bagli oldugu COM portunu sec
5. Gerekli kutuphaneleri yukle (Sketch → Include Library → Manage Libraries):
• MPU6050_light by rfetick
• DHT sensor library by Adafruit
• Adafruit Unified Sensor (otomatik istenir)
9. Yukaridaki kodu yeni bir sketch'e yapistir
10. Upload butonuna bas (yukari ok)
11. Yukleme tamamlandiktan sonra Mega'yi bilgisayardan cikar
12. Mega'yi Raspberry Pi'nin USB portuna tak
13. Pi'de Python scripti calistir, veriler akmaya baslamali
Test
Mega calisirken Pi'den asagidaki gibi komutlar gonderilebilir (Python ornegi):
import serial
import time
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
time.sleep(2) # Mega resetlenmesi icin

ser.write(b"FORWARD\n")
time.sleep(3)

ser.write(b"RIGHT\n")
time.sleep(3)

ser.write(b"STOP\n")