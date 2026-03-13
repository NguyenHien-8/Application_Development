#ifndef FINGERPRINT_ENROLL_H
#define FINGERPRINT_ENROLL_H

#include <Adafruit_Fingerprint.h>

// Các trạng thái khi thực hiện thao tác đăng ký
enum EnrollStatus {
    ENROLL_STATUS_OK,
    ENROLL_STATUS_NO_FINGER,
    ENROLL_STATUS_IMAGE_FAIL,
    ENROLL_STATUS_MATCH_FAIL, // Hai lần lấy mẫu vân tay không giống nhau
    ENROLL_STATUS_STORE_FAIL, // Lỗi khi lưu vào flash
    ENROLL_STATUS_ERROR       // Lỗi giao tiếp khác
};

class FingerprintEnroll {
public:
    FingerprintEnroll(HardwareSerial* serial);
    bool begin(uint8_t rxPin, uint8_t txPin, uint32_t baudRate = 57600);
    
    // Bước 1 & 3: Lấy hình ảnh và chuyển đổi (slot = 1 hoặc 2)
    EnrollStatus takeImageAndConvert(uint8_t slot);
    
    // Bước 2: Kiểm tra xem người dùng đã rút ngón tay ra chưa
    bool isFingerRemoved();
    
    // Bước 4: Tạo mô hình từ 2 slot và lưu vào ID chỉ định
    EnrollStatus createAndStore(uint16_t id);

private:
    Adafruit_Fingerprint* finger;
    HardwareSerial* hwSerial;
};

#endif