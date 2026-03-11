#include "FingerprintManager.h"

FingerprintManager::FingerprintManager(HardwareSerial& serial, uint32_t baud, uint8_t rxPin, uint8_t txPin)
    : _serial(serial), _baud(baud), _rxPin(rxPin), _txPin(txPin), _debug(nullptr), _initialized(false), _finger(&serial) {
}

bool FingerprintManager::begin() {
    _serial.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    _finger.begin(_baud);

    if (!_finger.verifyPassword()) {
        if (_debug) _debug->println("Fingerprint sensor not found!");
        return false;
    }

    _initialized = true;
    if (_debug) _debug->println("Fingerprint manager initialized.");
    return true;
}

// BỔ SUNG: Quét toàn bộ ID để xem ID nào đã có vân tay
std::vector<String> FingerprintManager::getRegisteredIds() {
    std::vector<String> registeredIds;
    if (!_initialized) return registeredIds;

    if (_debug) _debug->println("Dang quet cac ID da dang ky tren AS608...");
    
    // Quét từ ID 1 đến 127 (dung lượng phổ biến của AS608)
    for (uint16_t id = 1; id <= 127; id++) {
        // loadModel trả về FINGERPRINT_OK nếu template tồn tại ở vị trí ID đó
        if (_finger.loadModel(id) == FINGERPRINT_OK) {
            registeredIds.push_back(String(id));
        }
    }
    
    if (_debug) {
        _debug->print("Tim thay ");
        _debug->print(registeredIds.size());
        _debug->println(" van tay da dang ky.");
    }
    return registeredIds;
}