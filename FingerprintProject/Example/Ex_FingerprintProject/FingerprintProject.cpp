#include <Arduino.h>
#include "FingerprintManager.h"


// Gõ E1 (Enter) để Đăng ký vân tay vào ID 1.
// Gõ D1 (Enter) để Xóa vân tay ID 1.
// Còn lại cứ chạm vân tay vào là tự động Nhận diện (Detect).

HardwareSerial mySerial(2); 
FingerprintManager fpManager(&mySerial);

// =========================================================
// CALLABACKS CHO CHẾ ĐỘ DETECT (Mở khóa)
// =========================================================
void onMatch(uint16_t id, uint16_t confidence) {
    Serial.printf("\n[UNLOCK] Access Granted! User ID: %d (Confidence: %d)\n", id, confidence);
}

void onNoMatch() {
    Serial.println("\n[DENIED] Unknown Fingerprint. Intruder alert!");
}

void onDetectError() {
    Serial.println("\n[ERROR] Sensor reading error. Try again.");
}

// =========================================================
// CALLABACKS CHO CHẾ ĐỘ ENROLL (Đăng ký)
// =========================================================
void onPromptFirst()   { Serial.println("\n[ENROLL] Step 1: Place your finger on the sensor..."); }
void onPromptRelease() { Serial.println("[ENROLL] Step 2: Image taken! Remove your finger."); }
void onPromptSecond()  { Serial.println("[ENROLL] Step 3: Place the SAME finger again..."); }

void onEnrollSuccess(uint16_t id) {
    Serial.printf("\n[SUCCESS] Fingerprint enrolled successfully at ID #%d!\n", id);
    Serial.println("-> System returned to Detect Mode.");
}

void onEnrollError(const char* msg) {
    Serial.printf("\n[ENROLL FAILED] %s\n", msg);
    Serial.println("-> System returned to Detect Mode.");
}

// =========================================================
// CALLABACKS CHO CHẾ ĐỘ DELETE (Xóa)
// =========================================================
void onDeleteSuccess(uint8_t id) {
    Serial.printf("\n[SUCCESS] Fingerprint ID #%d has been DELETED.\n", id);
    Serial.println("-> System returned to Detect Mode.");
}

void onDeleteError(uint8_t id, const char* msg) {
    Serial.printf("\n[DELETE FAILED] ID #%d - %s\n", id, msg);
    Serial.println("-> System returned to Detect Mode.");
}

// =========================================================
// SETUP VÀ VÒNG LẶP CHÍNH
// =========================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n--- FINGERPRINT ALL-IN-ONE SYSTEM ---");
    Serial.println("Commands: Type 'E<id>' to Enroll, 'D<id>' to Delete (e.g., E5 or D5)");

    if (fpManager.begin(16, 17, 57600)) {
        Serial.println("Sensor initialized successfully!");
    } else {
        Serial.println("ERROR: Sensor not found. Check wiring.");
        while(1) { delay(10); }
    }

    // Đăng ký các Callbacks
    fpManager.setOnMatchCallback(onMatch);
    fpManager.setOnNoMatchCallback(onNoMatch);
    fpManager.setOnDetectErrorCallback(onDetectError);

    fpManager.setOnPromptFirstFinger(onPromptFirst);
    fpManager.setOnPromptReleaseFinger(onPromptRelease);
    fpManager.setOnPromptSecondFinger(onPromptSecond);
    fpManager.setOnEnrollSuccess(onEnrollSuccess);
    fpManager.setOnEnrollError(onEnrollError);

    fpManager.setOnDeleteSuccess(onDeleteSuccess);
    fpManager.setOnDeleteError(onDeleteError);
}

void loop() {
    // 1. Nhạc trưởng liên tục chạy ngầm (Non-blocking)
    fpManager.update();

    // 2. Lắng nghe Serial Monitor để ra lệnh điều khiển
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // Xóa khoảng trắng và ký tự xuống dòng
        
        if (input.length() > 1) {
            char command = input.charAt(0);
            uint8_t id = input.substring(1).toInt();
            
            if (id > 0 && id <= 127) {
                if (command == 'E' || command == 'e') {
                    Serial.printf("\n>>> Starting Enrollment for ID #%d...\n", id);
                    fpManager.startEnrollment(id);
                } 
                else if (command == 'D' || command == 'd') {
                    Serial.printf("\n>>> Requesting Deletion for ID #%d...\n", id);
                    fpManager.requestDelete(id);
                }
            } else {
                Serial.println("\n[WARN] Invalid ID! Must be between 1 and 127.");
            }
        }
    }
}