#include <WiFi.h>
#include <HTTPClient.h>
#include <MPU6050_tockn.h> 
#include <Wire.h>
#include "time.h"

// ===== WiFi & LINE =====2
const char* ssid = "Rose";
const char* password = "0618191505";
String LINE_TOKEN  = "4tnSOX0nG7Pos4UPhCY1bkqDO5G26ifTdj7g8B4u8pvXNlE9MQLI6wwPzbI3X5ewrOntR/AsE/LDXOjpte45mgxaOWO++YpvuxK6PGhqvbbq6Mnslo1JrdHZgYOlWIgSjP5NYpvtPOCnwdxFzzSNIQdB04t89/1O/w1cDnyilFU="; 
String USER_ID = "Ua83d2b28993c87037252bd8a34220d77";

// ===== LED =====
#define LED_READY 19   
#define LED_VIB   18   

MPU6050 mpu6050(Wire);

float vibrationThreshold = 0.5;
bool alreadyNotified = false;

// --- ย้ายมาไว้ตรงนี้เพื่อให้ loop() มองเห็น ---
unsigned long lastNotifyTime = 0;
const unsigned long cooldownInterval = 10000; 
// ---------------------------------------

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;

void sendLine(String msg) {
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi not connected");
    return;
  }

  // ✅ แทนที่ \n ด้วย \\n เพื่อให้ JSON ถูกต้อง
  msg.replace("\n", "\\n");

  HTTPClient http;
  http.begin("https://api.line.me/v2/bot/message/push");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + LINE_TOKEN);

  String body =
    "{\"to\":\"" + USER_ID + "\","
    "\"messages\":[{\"type\":\"text\",\"text\":\"" + msg + "\"}]}";

  int code = http.POST(body);

  Serial.print("LINE HTTP Code: ");
  Serial.println(code);

  if(code > 0){
    Serial.println(http.getString());
  }

  http.end();
}



String getDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "Unknown Time";
  char buffer[30];
  strftime(buffer, 30, "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_READY, OUTPUT);
  pinMode(LED_VIB, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  digitalWrite(LED_READY, HIGH); 

  configTime(gmtOffset_sec, 0, ntpServer);

  Wire.begin(21, 22);
  mpu6050.begin();
  Serial.println("Calibrating...");
  mpu6050.calcGyroOffsets(true); 
  Serial.println("✅ ระบบพร้อมทำงาน!");
  sendLine("ESP32 พร้อมทำงานแล้ว ✅");
  sendLine("ทดสอบจาก ESP32");
}

void loop() {
  mpu6050.update();
  
  float aX = mpu6050.getAccX();
  float aY = mpu6050.getAccY();
  float aZ = mpu6050.getAccZ();
  
  float totalAcc = sqrt(aX*aX + aY*aY + aZ*aZ);
  float vibration = abs(totalAcc - 1.0); 
  

  if (vibration > vibrationThreshold) {
  digitalWrite(LED_VIB, HIGH);

  if (!alreadyNotified && (millis() - lastNotifyTime > cooldownInterval)) {
    String msg = "🚨 ตรวจพบแรงสั่นสะเทือน!\nเวลา: " + getDateTime() +
                 "\nค่าความแรง: " + String(vibration, 2);

    Serial.println(msg);
    sendLine(msg);   // ✅ ต้องอยู่ใน if
    alreadyNotified = true;
    lastNotifyTime = millis();
  }
}

   else {
    digitalWrite(LED_VIB, LOW);
    alreadyNotified = false; 
  }
  delay(50); 
}