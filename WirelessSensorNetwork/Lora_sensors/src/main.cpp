#define BLYNK_TEMPLATE_ID "TMPL6u6CcU2bt"
#define BLYNK_TEMPLATE_NAME "Lora test"
#define BLYNK_AUTH_TOKEN "WvsHTNJ4uelyAP7MzFqNXpmrEhGDfN6s"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "wf_connect.h"
#include "GatewayLoRa.h"

// --- THÔNG TIN CỦA BẠN ---
const char *ssid = "Hoang Ngan"; 
const char *password = "hoilamgi1";

enum WifiState { WS_DISCONNECTED, WS_CONNECTING, WS_CONNECTED };
volatile WifiState currentWifiState = WS_DISCONNECTED;

// Mảng chứa 3 ID do người dùng nhập từ Serial
int nodeIDs[3]; 

// --- HAM XU LY SU KIEN WIFI (CALLBACK) GIỮ NGUYÊN ---
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      currentWifiState = WS_CONNECTING; break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      currentWifiState = WS_CONNECTING; break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      currentWifiState = WS_CONNECTED; break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (currentWifiState != WS_DISCONNECTED) {
        previousMillis_Disconnect = millis();
        previousMillis_Restart = millis();
        currentWifiState = WS_DISCONNECTED;
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  // 1. NHẬP ID CÁC NODE CẦN QUẢN LÝ TỪ SERIAL
  Serial.println("\n=== SETUP GATEWAY ===");
  Serial.println("Nhap 3 ID cua cac Node can thu thap (Vi du: 10,20,30):");
  while (Serial.available() == 0) { ConnectingEffect(); } // Hiệu ứng chờ nhập
  
  String input = Serial.readStringUntil('\n');
  sscanf(input.c_str(), "%d,%d,%d", &nodeIDs[0], &nodeIDs[1], &nodeIDs[2]);
  Serial.printf("Da luu ID cac Node: %d, %d, %d\n", nodeIDs[0], nodeIDs[1], nodeIDs[2]);

  // 2. SETUP PHẦN CỨNG
  for (uint8_t i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH);
  }

  setupGatewayLoRa();

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
}

void loop() {
  // YÊU CẦU: CHỈ CHÈN CODE VÀO TRONG CÁC CASE
  switch (currentWifiState) {
    case WS_CONNECTING:
      ConnectingEffect();
      break;

    case WS_CONNECTED: {
      int8_t rssi = WiFi.RSSI();
      showSignalLevel(rssi);
      
      Blynk.run(); // Duy trì Blynk

      // --- LOGIC GOM DỮ LIỆU LORA VÀ ĐẨY BLYNK ---
      static unsigned long lastPoll = 0;
      if (millis() - lastPoll > 5000) { // Mỗi 5 giây đi hỏi 3 Node một vòng
        String blynkPayload = "";
        
        Serial.println("\nDang lay du lieu tu cac Node...");
        for (int i = 0; i < 3; i++) {
          String nodeData = pollNode(nodeIDs[i]);
          blynkPayload += nodeData + " | ";
          Serial.println(nodeData);
        }

        // Đẩy chuỗi tổng hợp lên V0
        Blynk.virtualWrite(V0, blynkPayload);
        Serial.println("=> Da day len Blynk: " + blynkPayload);
        
        lastPoll = millis();
      }
      break;
    }

    case WS_DISCONNECTED:
      LostConnectEffect();
      Disconnect();
      RestartESP32();
      break;
  }
}