#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <vector> // Bổ sung thư viện vector

class FingerprintManager {
public:
    FingerprintManager(HardwareSerial& serial, uint32_t baud = 57600, uint8_t rxPin = 15, uint8_t txPin = 16);

    bool begin();

    Adafruit_Fingerprint* getFingerprint() { return &_finger; }

    void setDebug(Stream* stream) { _debug = stream; }

    // BỔ SUNG: Hàm lấy danh sách toàn bộ ID đã đăng ký trong cảm biến
    std::vector<String> getRegisteredIds();

private:
    HardwareSerial& _serial;
    Adafruit_Fingerprint _finger;
    uint32_t _baud;
    uint8_t _rxPin, _txPin;
    Stream* _debug;
    bool _initialized;
};

#endif