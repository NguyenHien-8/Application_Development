#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

#include "FingerprintDetect.h"
#include "FingerprintEnroll.h"
#include "FingerprintDelete.h"

// Các chế độ hoạt động của hệ thống
enum OperationMode {
    MODE_DETECT,
    MODE_ENROLL,
    MODE_DELETE
};

// Trạng thái của quá trình đăng ký (Enroll)
enum EnrollState {
    STATE_IDLE,
    STATE_WAIT_FIRST_FINGER,
    STATE_WAIT_RELEASE,
    STATE_WAIT_SECOND_FINGER
};

// --- Định nghĩa các Callbacks ---
// Callbacks cho Detect
typedef void (*MatchCallback)(uint16_t id, uint16_t confidence);
typedef void (*NoMatchCallback)();
typedef void (*DetectErrorCallback)();

// Callbacks cho Enroll
typedef void (*PromptCallback)();
typedef void (*EnrollSuccessCallback)(uint16_t id);
typedef void (*EnrollErrorCallback)(const char* msg);

// Callbacks cho Delete
typedef void (*DeleteSuccessCallback)(uint8_t id);
typedef void (*DeleteErrorCallback)(uint8_t id, const char* msg);

class FingerprintManager {
public:
    FingerprintManager(HardwareSerial* serial);
    bool begin(uint8_t rxPin, uint8_t txPin, uint32_t baudRate = 57600);
    
    // Vòng lặp chính, gọi liên tục trong loop()
    void update(); 

    // Các hàm chuyển đổi chế độ
    void startEnrollment(uint8_t id);
    void requestDelete(uint8_t id);
    void cancelOperation(); // Hủy Enroll/Delete để quay về Detect

    // Setters cho Callbacks - Detect
    void setOnMatchCallback(MatchCallback cb);
    void setOnNoMatchCallback(NoMatchCallback cb);
    void setOnDetectErrorCallback(DetectErrorCallback cb);

    // Setters cho Callbacks - Enroll
    void setOnPromptFirstFinger(PromptCallback cb);
    void setOnPromptReleaseFinger(PromptCallback cb);
    void setOnPromptSecondFinger(PromptCallback cb);
    void setOnEnrollSuccess(EnrollSuccessCallback cb);
    void setOnEnrollError(EnrollErrorCallback cb);

    // Setters cho Callbacks - Delete
    void setOnDeleteSuccess(DeleteSuccessCallback cb);
    void setOnDeleteError(DeleteErrorCallback cb);

private:
    FingerprintDetect detector;
    FingerprintEnroll enroller;
    FingerprintDelete deleter;
    
    OperationMode currentMode;
    
    // Biến cho Detect
    bool isFingerOnSensor;
    
    // Biến cho Enroll & Delete
    EnrollState enrollState;
    uint8_t targetID;

    // Quản lý thời gian Non-blocking
    unsigned long lastCheckTime;
    const unsigned long checkInterval = 100; // 100ms

    // Con trỏ hàm Callbacks
    MatchCallback onMatch;
    NoMatchCallback onNoMatch;
    DetectErrorCallback onDetectError;

    PromptCallback onPromptFirst;
    PromptCallback onPromptRelease;
    PromptCallback onPromptSecond;
    EnrollSuccessCallback onEnrollSuccess;
    EnrollErrorCallback onEnrollError;

    DeleteSuccessCallback onDeleteSuccess;
    DeleteErrorCallback onDeleteError;

    void updateDetect();
    void updateEnroll();
    void updateDelete();
};

#endif