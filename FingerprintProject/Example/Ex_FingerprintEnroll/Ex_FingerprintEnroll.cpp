#include <Arduino.h>
#include "FingerprintManager.h"

HardwareSerial mySerial(2); 
FingerprintManager fpManager(&mySerial);

// --- CALLBACKS: QUẢN LÝ GIAO DIỆN (UI/UX) ---
void promptFirst() {
    Serial.println("\n[ENROLL] Step 1: Please place your finger on the sensor...");
}

void promptRelease() {
    Serial.println("[ENROLL] Step 2: Image taken! Now, remove your finger.");
}

void promptSecond() {
    Serial.println("[ENROLL] Step 3: Place the SAME finger again...");
}

void enrollSuccess(uint16_t id) {
    Serial.println("=====================================");
    Serial.print("[SUCCESS] Fingerprint enrolled successfully at ID #");
    Serial.println(id);
    Serial.println("=====================================\n");
}

void enrollError(const char* msg) {
    Serial.print("\n[ERROR] ");
    Serial.println(msg);
    Serial.println("Enrollment aborted.\n");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- Fingerprint Enrollment System ---");

    if (fpManager.begin(16, 17, 57600)) {
        Serial.println("Sensor initialized!");
    } else {
        Serial.println("ERROR: Sensor not found.");
        while (1) { delay(1); } 
    }

    // Đăng ký Callback
    fpManager.setOnPromptFirstFinger(promptFirst);
    fpManager.setOnPromptReleaseFinger(promptRelease);
    fpManager.setOnPromptSecondFinger(promptSecond);
    fpManager.setOnEnrollSuccess(enrollSuccess);
    fpManager.setOnEnrollError(enrollError);

    Serial.println("\nType an ID (1-127) and press Enter to start enrolling:");
}

void loop() {
    // Luôn gọi hàm update để duy trì máy trạng thái (Non-blocking)
    fpManager.update();

    // Lắng nghe lệnh từ Serial để bắt đầu đăng ký
    if (Serial.available() > 0) {
        int targetID = Serial.parseInt();
        // Đọc nốt ký tự thừa (như \n)
        while(Serial.available() > 0) Serial.read(); 
        
        if (targetID > 0 && targetID <= 127) {
            Serial.print("\nStarting enrollment for ID #");
            Serial.println(targetID);
            fpManager.startEnrollment(targetID);
        } else if (targetID != 0) {
            Serial.println("Invalid ID. Please enter a number between 1 and 127.");
        }
    }
}