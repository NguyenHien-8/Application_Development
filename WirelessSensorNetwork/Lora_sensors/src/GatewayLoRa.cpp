#include "GatewayLoRa.h"
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

#define SCK_PIN 15 
#define MISO_PIN 16
#define MOSI_PIN 2
#define SS_PIN 42
#define RST_PIN 40
#define DIO0_PIN 41

void setupGatewayLoRa() {
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(433E6)) {
    Serial.println("Loi khoi tao LoRa Gateway!");
    while (1);
  }
}

String pollNode(int targetID) {
  // Gửi lệnh hỏi
  JsonDocument askDoc;
  askDoc["ask"] = targetID;
  String askString;
  serializeJson(askDoc, askString);

  LoRa.beginPacket();
  LoRa.print(askString);
  LoRa.endPacket();

  // Đợi Node trả lời (Timeout 800ms)
  long startWait = millis();
  while (millis() - startWait < 800) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String incoming = "";
      while (LoRa.available()) incoming += (char)LoRa.read();

      JsonDocument resDoc;
      DeserializationError err = deserializeJson(resDoc, incoming);
      
      // Nếu đúng JSON và đúng ID đang hỏi
      if (!err && resDoc["node"] == targetID) {
        float t = resDoc["temp"];
        float h = resDoc["hum"];
        return "ID " + String(targetID) + ": " + String(t, 1) + "C," + String(h, 1) + "%";
      }
    }
  }
  return "ID " + String(targetID) + ": N/A"; // Lỗi hoặc timeout
}