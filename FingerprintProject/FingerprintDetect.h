#ifndef FINGERPRINT_DETECT_H
#define FINGERPRINT_DETECT_H

#include <Adafruit_Fingerprint.h>

// Các trạng thái dễ hiểu được trừu tượng hóa từ thư viện gốc
enum FingerprintStatus {
    FP_STATUS_IDLE,       // Không có gì xảy ra
    FP_STATUS_MATCHED,    // Vân tay khớp
    FP_STATUS_NOT_FOUND,  // Quét thành công nhưng không có trong cơ sở dữ liệu
    FP_STATUS_NO_FINGER,  // Không có ngón tay trên cảm biến
    FP_STATUS_ERROR       // Lỗi giao tiếp hoặc hình ảnh quá mờ
};

class FingerprintDetect {
public:
    FingerprintDetect(HardwareSerial* serial);
    bool begin(uint8_t rxPin, uint8_t txPin, uint32_t baudRate = 57600);
    
    // Hàm thực hiện quét và trả về trạng thái rút gọn
    FingerprintStatus scan();
    
    // Lấy thông tin sau khi quét thành công
    uint16_t getMatchedID();
    uint16_t getConfidence();

private:
    Adafruit_Fingerprint* finger;
    HardwareSerial* hwSerial;
    uint16_t matchedID;
    uint16_t matchedConfidence;
};

#endif