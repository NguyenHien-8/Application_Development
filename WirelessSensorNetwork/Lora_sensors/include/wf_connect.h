#pragma once
#include <Arduino.h>
#include <WiFi.h>

extern const char *ssid;
extern const char *password;

// Bổ sung khai báo led từ file main của bạn để hàm hiểu được
const uint8_t ledPins[] = {7, 6, 5, 4}; 
const uint8_t numLeds = 4;

// --- BÊ Y NGUYÊN CODE CỦA BẠN DƯỚI ĐÂY ---
// --- KHAI BAO BIEN THOI GIAN ---
unsigned long previousMillis_Signal = 0;     
unsigned long previousMillis_Disconnect = 0;
unsigned long previousMillis_Restart = 0; 

const uint16_t interval_SignalLevel = 500;
const uint16_t interval_disconnect = 20000;
const uint16_t interval_restartEsp32 = 50000;

void Disconnect() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis_Disconnect >= interval_disconnect) {
    Serial.println(">> [TIMEOUT 20s] Thu Reset WiFi va Ket noi lai...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    previousMillis_Disconnect = currentMillis;
  }
}

void RestartESP32() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis_Restart >= interval_restartEsp32) {
    Serial.println(">> [TIMEOUT 50s] KHONG THE KET NOI. KHOI DONG LAI ESP32...");
    ESP.restart();
  }
}

// --- HIEN THI CONG SUAT MUC SONG ---
void showSignalLevel(int rssi) {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis_Signal >= interval_SignalLevel) {
    Serial.printf("RSSI = %d", rssi);
    for (uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
    if (rssi <= -90) { // Lost Connect
      for (uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
    }

    if (rssi > -90 && rssi <= -70) digitalWrite(ledPins[0], LOW);
// Connect Yeu    
      else if (rssi > -70 && rssi <= -50) {
        for(uint8_t i = 0; i < 2; i++) digitalWrite(ledPins[i], LOW);
// Connect TB 
    } else if (rssi > -50 && rssi <= -30) {
        for(uint8_t i = 0; i < 3; i++) digitalWrite(ledPins[i], LOW);
// Connect Tot 
    } else if (rssi > -30) {
        for(uint8_t i = 0; i < 4; i++) digitalWrite(ledPins[i], LOW);
// Connect Cuc Manh
    }   

    previousMillis_Signal = currentMillis;
  }
}

// --- HIEU UNG LED "LOADING" ---
void ConnectingEffect() {
  static uint8_t Positions_Led = 0;
  for(uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
  digitalWrite(ledPins[Positions_Led], LOW);
  Positions_Led++;
  if(Positions_Led >= numLeds) Positions_Led = 0;
  delay(150);
}

// --- HIEU UNG MAT KET NOI ---
void LostConnectEffect() {
  for(uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
  delay(300);
  for(uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], LOW);
  delay(300);
}