#include "FingerprintDelete.h"

FingerprintDelete::FingerprintDelete(HardwareSerial* serial) {
    hwSerial = serial;
    finger = new Adafruit_Fingerprint(hwSerial);
}

bool FingerprintDelete::begin(uint8_t rxPin, uint8_t txPin, uint32_t baudRate) {
    hwSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
    finger->begin(baudRate);
    delay(5);
    return finger->verifyPassword();
}

DeleteStatus FingerprintDelete::remove(uint8_t id) {
    // Gọi lệnh xóa mô hình vân tay từ thư viện Adafruit
    uint8_t p = finger->deleteModel(id);
    
    // Che giấu mã lỗi thư viện bằng các trạng thái dễ hiểu hơn
    if (p == FINGERPRINT_OK) return DELETE_STATUS_OK;
    if (p == FINGERPRINT_PACKETRECIEVEERR) return DELETE_STATUS_COMM_ERR;
    if (p == FINGERPRINT_BADLOCATION) return DELETE_STATUS_BAD_LOCATION;
    if (p == FINGERPRINT_FLASHERR) return DELETE_STATUS_FLASH_ERR;
    
    return DELETE_STATUS_UNKNOWN;
}