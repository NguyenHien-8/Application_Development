#include "NodeLoRa.h"
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// Bạn tự đổi chân này nếu dùng Arduino Uno/Nano nhé
#define SCK_PIN 12
#define MISO_PIN 13
#define MOSI_PIN 11
#define SS_PIN 10
#define RST_PIN 9
#define DIO0_PIN 14

int myNodeID = 0;

void setupNodeLoRa() {
  Serial.begin(115200);

  // Nhập ID từ Serial
  Serial.println("\n=== SETUP NODE PHAT ===");
  Serial.println("Nhap ID cho Node nay (Vi du: 10):");
  while (Serial.available() == 0) {}
  myNodeID = Serial.parseInt();
  Serial.printf("Da luu ID cua Node: %d\n", myNodeID);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(433E6)) {
    Serial.println("Loi LoRa Node!");
    while(1);
  }
  Serial.println("Sẵn sàng lắng nghe Master...");
}


/*Day la ham gui du lieu, 
nhietDo va doAm la 2 bien du lieu tu cam bien cua team ban,
guiDuLieu se tu dong kiem tra xem co Master nao goi ID cua minh khong,
neu co thi se gui du lieu, neu khong thi se im lang cho den khi co Master goi tiep.
Ban chi can goi ham nay lien tuc trong vong loop, no se tu dong xu ly viec gui du lieu khi co Master goi.*/

int guiDuLieu(float nhietDo = 11, float doAm = 15) {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) incoming += (char)LoRa.read();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, incoming);

    // Nếu không lỗi và Master gọi đúng tên (ID) của mình
    if (!err && doc["ask"] == myNodeID) {
      Serial.println("Master goi! Dang gui du lieu...");
      
      JsonDocument response;
      response["node"] = myNodeID;
      response["temp"] = nhietDo;
      response["hum"] = doAm;

      String resString;
      serializeJson(response, resString);

      LoRa.beginPacket();
      LoRa.print(resString);
      LoRa.endPacket();
      return 1; // Báo cho team biết là đã gửi xong
    }
  }
  return 0; // Đang yên lặng chờ đợi
}