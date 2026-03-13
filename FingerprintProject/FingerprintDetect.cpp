#include "FingerprintDetect.h"

FingerprintDetect::FingerprintDetect(HardwareSerial* serial) {
    hwSerial = serial;
    finger = new Adafruit_Fingerprint(hwSerial);
    matchedID = 0;
    matchedConfidence = 0;
}

bool FingerprintDetect::begin(uint8_t rxPin, uint8_t txPin, uint32_t baudRate) {
    hwSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
    finger->begin(baudRate);
    delay(5);
    return finger->verifyPassword();
}

FingerprintStatus FingerprintDetect::scan() {
    uint8_t p = finger->getImage();
    if (p == FINGERPRINT_NOFINGER) return FP_STATUS_NO_FINGER;
    if (p != FINGERPRINT_OK) return FP_STATUS_ERROR;

    p = finger->image2Tz();
    if (p != FINGERPRINT_OK) return FP_STATUS_ERROR;

    // Sử dụng fingerFastSearch() để tối ưu tốc độ đọc
    p = finger->fingerFastSearch();
    if (p == FINGERPRINT_OK) {
        matchedID = finger->fingerID;
        matchedConfidence = finger->confidence;
        return FP_STATUS_MATCHED;
    } else if (p == FINGERPRINT_NOTFOUND) {
        return FP_STATUS_NOT_FOUND;
    } else {
        return FP_STATUS_ERROR;
    }
}

uint16_t FingerprintDetect::getMatchedID() { 
    return matchedID; 
}

uint16_t FingerprintDetect::getConfidence() { 
    return matchedConfidence; 
}