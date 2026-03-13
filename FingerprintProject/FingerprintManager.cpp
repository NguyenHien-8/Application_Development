#include "FingerprintManager.h"

FingerprintManager::FingerprintManager(HardwareSerial* serial) 
    : detector(serial), enroller(serial), deleter(serial) {
    
    currentMode = MODE_DETECT;
    isFingerOnSensor = false;
    enrollState = STATE_IDLE;
    targetID = 0;
    lastCheckTime = 0;

    // Reset Callbacks
    onMatch = nullptr; onNoMatch = nullptr; onDetectError = nullptr;
    onPromptFirst = nullptr; onPromptRelease = nullptr; onPromptSecond = nullptr;
    onEnrollSuccess = nullptr; onEnrollError = nullptr;
    onDeleteSuccess = nullptr; onDeleteError = nullptr;
}

bool FingerprintManager::begin(uint8_t rxPin, uint8_t txPin, uint32_t baudRate) {
    // Khởi tạo phần cứng (chỉ cần lấy trạng thái của 1 module là đủ biết cảm biến đã nhận)
    bool ok = detector.begin(rxPin, txPin, baudRate);
    enroller.begin(rxPin, txPin, baudRate);
    deleter.begin(rxPin, txPin, baudRate);
    return ok;
}

// --- Đăng ký Callbacks ---
void FingerprintManager::setOnMatchCallback(MatchCallback cb) { onMatch = cb; }
void FingerprintManager::setOnNoMatchCallback(NoMatchCallback cb) { onNoMatch = cb; }
void FingerprintManager::setOnDetectErrorCallback(DetectErrorCallback cb) { onDetectError = cb; }

void FingerprintManager::setOnPromptFirstFinger(PromptCallback cb) { onPromptFirst = cb; }
void FingerprintManager::setOnPromptReleaseFinger(PromptCallback cb) { onPromptRelease = cb; }
void FingerprintManager::setOnPromptSecondFinger(PromptCallback cb) { onPromptSecond = cb; }
void FingerprintManager::setOnEnrollSuccess(EnrollSuccessCallback cb) { onEnrollSuccess = cb; }
void FingerprintManager::setOnEnrollError(EnrollErrorCallback cb) { onEnrollError = cb; }

void FingerprintManager::setOnDeleteSuccess(DeleteSuccessCallback cb) { onDeleteSuccess = cb; }
void FingerprintManager::setOnDeleteError(DeleteErrorCallback cb) { onDeleteError = cb; }

// --- Điều hướng lệnh ---
void FingerprintManager::startEnrollment(uint8_t id) {
    targetID = id;
    currentMode = MODE_ENROLL;
    enrollState = STATE_WAIT_FIRST_FINGER;
    if (onPromptFirst) onPromptFirst();
}

void FingerprintManager::requestDelete(uint8_t id) {
    targetID = id;
    currentMode = MODE_DELETE; // Sẽ xử lý ngay trong chu kỳ update tiếp theo
}

void FingerprintManager::cancelOperation() {
    currentMode = MODE_DETECT;
    enrollState = STATE_IDLE;
}

// --- Update Logic (Non-blocking) ---
void FingerprintManager::update() {
    if (millis() - lastCheckTime < checkInterval) return;
    lastCheckTime = millis();

    switch (currentMode) {
        case MODE_DETECT: updateDetect(); break;
        case MODE_ENROLL: updateEnroll(); break;
        case MODE_DELETE: updateDelete(); break;
    }
}

void FingerprintManager::updateDetect() {
    FingerprintStatus status = detector.scan();

    if (status == FP_STATUS_NO_FINGER) {
        isFingerOnSensor = false;
        return;
    }

    if (isFingerOnSensor) return; // Chống spam khi giữ ngón tay
    isFingerOnSensor = true;

    switch (status) {
        case FP_STATUS_MATCHED:
            if (onMatch) onMatch(detector.getMatchedID(), detector.getConfidence());
            break;
        case FP_STATUS_NOT_FOUND:
            if (onNoMatch) onNoMatch();
            break;
        case FP_STATUS_ERROR:
            if (onDetectError) onDetectError();
            break;
        default: break;
    }
}

void FingerprintManager::updateEnroll() {
    EnrollStatus status;

    switch (enrollState) {
        case STATE_WAIT_FIRST_FINGER:
            status = enroller.takeImageAndConvert(1);
            if (status == ENROLL_STATUS_OK) {
                enrollState = STATE_WAIT_RELEASE;
                if (onPromptRelease) onPromptRelease();
            } else if (status != ENROLL_STATUS_NO_FINGER) {
                if (onEnrollError) onEnrollError("Failed to read first image.");
                cancelOperation(); // Trở về Detect
            }
            break;

        case STATE_WAIT_RELEASE:
            if (enroller.isFingerRemoved()) {
                enrollState = STATE_WAIT_SECOND_FINGER;
                if (onPromptSecond) onPromptSecond();
            }
            break;

        case STATE_WAIT_SECOND_FINGER:
            status = enroller.takeImageAndConvert(2);
            if (status == ENROLL_STATUS_OK) {
                EnrollStatus finalStatus = enroller.createAndStore(targetID);
                if (finalStatus == ENROLL_STATUS_OK) {
                    if (onEnrollSuccess) onEnrollSuccess(targetID);
                } else if (finalStatus == ENROLL_STATUS_MATCH_FAIL) {
                    if (onEnrollError) onEnrollError("Fingerprints did not match.");
                } else {
                    if (onEnrollError) onEnrollError("Failed to store model in memory.");
                }
                cancelOperation(); // Trở về Detect
            } else if (status != ENROLL_STATUS_NO_FINGER) {
                if (onEnrollError) onEnrollError("Failed to read second image.");
                cancelOperation(); // Trở về Detect
            }
            break;
            
        default: break;
    }
}

void FingerprintManager::updateDelete() {
    DeleteStatus status = deleter.remove(targetID);
    
    if (status == DELETE_STATUS_OK) {
        if (onDeleteSuccess) onDeleteSuccess(targetID);
    } else {
        if (onDeleteError) {
            const char* msg = "Unknown Error";
            if (status == DELETE_STATUS_BAD_LOCATION) msg = "Empty ID or Bad Location";
            else if (status == DELETE_STATUS_COMM_ERR) msg = "Communication Error";
            onDeleteError(targetID, msg);
        }
    }
    
    // Xóa xong, tự động trở về chế độ Detect
    cancelOperation(); 
}