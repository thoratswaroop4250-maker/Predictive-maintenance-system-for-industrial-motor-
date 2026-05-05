#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// WiFi
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

// ThingSpeak
String apiKey = "YOUR_WRITE_API_KEY";
const char* server = "http://api.thingspeak.com/update";

// Pins
#define DHTPIN 4
#define DHTTYPE DHT11
#define CURRENT_PIN 34
#define VOLTAGE_PIN 35
#define RELAY_PIN 26
#define SD_CS 5

// Objects
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Calibration
float currentCal = 0.1;
float voltageCal = 0.1;

// Thresholds
float maxTemp = 50;
float maxCurrent = 10;
float maxVibration = 15;

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // LOW trigger relay OFF

  lcd.init();
  lcd.backlight();
  lcd.print("Starting...");

  dht.begin();
  Wire.begin(21, 22);
  accel.begin();

  SD.begin(SD_CS);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  lcd.clear();
  lcd.print("Connected");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  int rawCurrent = analogRead(CURRENT_PIN);
  float current = (rawCurrent - 2048) * currentCal;

  int rawVoltage = analogRead(VOLTAGE_PIN);
  float voltage = rawVoltage * voltageCal;

  sensors_event_t event;
  accel.getEvent(&event);

  float vibration = sqrt(
    event.acceleration.x * event.acceleration.x +
    event.acceleration.y * event.acceleration.y +
    event.acceleration.z * event.acceleration.z
  );

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:"); lcd.print(temp);
  lcd.print(" V:"); lcd.print(voltage);

  lcd.setCursor(0,1);
  lcd.print("I:"); lcd.print(current);
  lcd.print(" Vib:"); lcd.print(vibration);

  File file = SD.open("/data.txt", FILE_APPEND);
  if (file) {
    file.print(temp); file.print(",");
    file.print(hum); file.print(",");
    file.print(current); file.print(",");
    file.print(voltage); file.print(",");
    file.println(vibration);
    file.close();
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = server;
    url += "?api_key=" + apiKey;
    url += "&field1=" + String(temp);
    url += "&field2=" + String(current);
    url += "&field3=" + String(voltage);
    url += "&field4=" + String(vibration);
    url += "&field5=" + String(hum);

    http.begin(url);
    http.GET();
    http.end();
  }

  if (temp > maxTemp || current > maxCurrent || vibration > maxVibration) {
    digitalWrite(RELAY_PIN, HIGH); // OFF
  } else {
    digitalWrite(RELAY_PIN, LOW); // ON
  }

  delay(15000);
}
