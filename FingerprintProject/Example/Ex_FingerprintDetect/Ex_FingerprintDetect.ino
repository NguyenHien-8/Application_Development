#include <Arduino.h>
#include "FingerprintManager.h"

// Khởi tạo HardwareSerial 2 của ESP32
HardwareSerial mySerial(2); 
FingerprintManager fpManager(&mySerial);

// --- CÁC HÀM XỬ LÝ SỰ KIỆN (CALLBACKS) ---
void onFingerMatch(uint16_t id, uint16_t confidence) {
    Serial.println("=====================================");
    Serial.println("[SUCCESS] Access Granted!");
    Serial.print("User ID: "); Serial.println(id);
    Serial.print("Confidence: "); Serial.println(confidence);
    Serial.println("=====================================\n");
}

void onFingerNoMatch() {
    Serial.println("[DENIED] Unknown Fingerprint. Intruder alert!");
}

void onFingerError() {
    Serial.println("[ERROR] Sensor error or dirty image. Please try again.");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- Fingerprint Security System ---");

    // Khởi tạo cảm biến với RX=16, TX=17, Baud=57600
    if (fpManager.begin(16, 17, 57600)) {
        Serial.println("Fingerprint sensor initialized successfully!");
    } else {
        Serial.println("ERROR: Did not find fingerprint sensor.");
        while (1) { delay(1); } // Dừng nếu lỗi phần cứng
    }

    fpManager.setOnMatchCallback(onFingerMatch);
    fpManager.setOnNoMatchCallback(onFingerNoMatch);
    fpManager.setOnDetectErrorCallback(onFingerError);
    
    Serial.println("System is ready. Waiting for valid finger...");
}

void loop() {
    fpManager.update();
}