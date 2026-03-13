#include <Arduino.h>
#include "FingerprintManager.h"

HardwareSerial mySerial(2); 
FingerprintManager fpManager(&mySerial);

// --- CÁC HÀM XỬ LÝ SỰ KIỆN CALLBACKS ---
void onDeleteSuccess(uint8_t id) {
    Serial.println("=====================================");
    Serial.print("[SUCCESS] Fingerprint ID #");
    Serial.print(id);
    Serial.println(" has been permanently DELETED.");
    Serial.println("=====================================\n");
    Serial.println("Type next ID to delete (1-127):");
}

void onDeleteError(uint8_t id, const char* msg) {
    Serial.print("\n[ERROR] Failed to delete ID #");
    Serial.print(id);
    Serial.print(". Reason: ");
    Serial.println(msg);
    Serial.println("Type next ID to delete (1-127):");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- Fingerprint Deletion System ---");

    if (fpManager.begin(16, 17, 57600)) {
        Serial.println("Sensor initialized successfully!");
    } else {
        Serial.println("ERROR: Sensor not found.");
        while (1) { delay(1); } 
    }

    // Đăng ký Callback
    fpManager.setOnDeleteSuccess(onDeleteSuccess);
    fpManager.setOnDeleteError(onDeleteError);

    Serial.println("\nType an ID (1-127) and press Enter to delete it:");
}

void loop() {
    // Duy trì tính năng xử lý ngầm trong Lớp 2
    fpManager.update();

    // Đọc Serial không sử dụng vòng lặp while block hệ thống
    if (Serial.available() > 0) {
        int targetID = Serial.parseInt();
        
        // Đọc nốt và xóa các ký tự thừa (như \r, \n) trong buffer
        while(Serial.available() > 0) Serial.read(); 
        
        if (targetID > 0 && targetID <= 127) {
            Serial.print("\nRequesting deletion for ID #");
            Serial.println(targetID);
            
            // Lên lịch xóa thay vì xóa trực tiếp
            fpManager.requestDelete(targetID);
        } else if (targetID != 0) {
            Serial.println("Invalid ID. Please enter a number between 1 and 127.");
        }
    }
}