#include <Arduino.h>
#include "NodeLoRa.h"

// Các biến của thành viên phụ trách cảm biến
float nhietDoHienTai = 0.0;
float doAmHienTai = 0.0;

void setup() {
  // Chỉ gọi 1 hàm setup LoRa duy nhất (trong này đã bao gồm chọn ID)
  setupNodeLoRa();
  
  // Khởi tạo cảm biến của team ở đây...
}

void loop() {
  // --- 1. CODE ĐỌC CẢM BIẾN CỦA TEAM ---
  // Giả lập đọc cảm biến
  nhietDoHienTai = random(250, 350) / 10.0; 
  doAmHienTai = random(600, 800) / 10.0;

  // --- 2. CODE GỬI DỮ LIỆU ---
  // Gọi hàm ở vòng loop liên tục, nó sẽ tự động chờ Master gọi thì mới gửi
  int ketQua = guiDuLieu(nhietDoHienTai, doAmHienTai);
  
  if (ketQua == 1) {
    Serial.println(">>> Bao cao xong: Nhiet do va Do am!");
  }
}