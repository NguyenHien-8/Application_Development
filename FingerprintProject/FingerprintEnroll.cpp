#include "FingerprintEnroll.h"

FingerprintEnroll::FingerprintEnroll(HardwareSerial* serial) {
    hwSerial = serial;
    finger = new Adafruit_Fingerprint(hwSerial);
}

bool FingerprintEnroll::begin(uint8_t rxPin, uint8_t txPin, uint32_t baudRate) {
    hwSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
    finger->begin(baudRate);
    delay(5);
    return finger->verifyPassword();
}

EnrollStatus FingerprintEnroll::takeImageAndConvert(uint8_t slot) {
    uint8_t p = finger->getImage();
    if (p == FINGERPRINT_NOFINGER) return ENROLL_STATUS_NO_FINGER;
    if (p != FINGERPRINT_OK) return ENROLL_STATUS_IMAGE_FAIL;

    p = finger->image2Tz(slot);
    if (p != FINGERPRINT_OK) return ENROLL_STATUS_ERROR;

    return ENROLL_STATUS_OK;
}

bool FingerprintEnroll::isFingerRemoved() {
    uint8_t p = finger->getImage();
    return (p == FINGERPRINT_NOFINGER);
}

EnrollStatus FingerprintEnroll::createAndStore(uint16_t id) {
    // Tạo mô hình từ 2 mẫu đã lưu ở buffer 1 và 2
    uint8_t p = finger->createModel();
    if (p == FINGERPRINT_ENROLLMISMATCH) return ENROLL_STATUS_MATCH_FAIL;
    if (p != FINGERPRINT_OK) return ENROLL_STATUS_ERROR;

    // Lưu mô hình vào ID tương ứng
    p = finger->storeModel(id);
    if (p != FINGERPRINT_OK) return ENROLL_STATUS_STORE_FAIL;

    return ENROLL_STATUS_OK;
}