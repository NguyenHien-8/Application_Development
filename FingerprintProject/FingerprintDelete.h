#ifndef FINGERPRINT_DELETE_H
#define FINGERPRINT_DELETE_H

#include <Adafruit_Fingerprint.h>

// Các trạng thái khi thực thi lệnh xóa vân tay
enum DeleteStatus {
    DELETE_STATUS_OK,
    DELETE_STATUS_COMM_ERR,      // Lỗi giao tiếp
    DELETE_STATUS_BAD_LOCATION,  // Vị trí ID không hợp lệ hoặc trống
    DELETE_STATUS_FLASH_ERR,     // Lỗi ghi bộ nhớ Flash
    DELETE_STATUS_UNKNOWN        // Lỗi không xác định
};

class FingerprintDelete {
public:
    FingerprintDelete(HardwareSerial* serial);
    bool begin(uint8_t rxPin, uint8_t txPin, uint32_t baudRate = 57600);
    
    // Thực thi lệnh xóa theo ID
    DeleteStatus remove(uint8_t id);

private:
    Adafruit_Fingerprint* finger;
    HardwareSerial* hwSerial;
};

#endif