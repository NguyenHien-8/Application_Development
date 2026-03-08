#include "QueryDatabase.h"

void QueryDatabase::begin(String url, String key) {
    db.begin(url, key);
    Serial.println("Supabase initialized!");
}

String QueryDatabase::getPendingCommand() {
    // Tìm lệnh đang chờ xử lý, lấy 1 lệnh cũ nhất
    String result = db.from("device_commands")
                      .select("*")
                      .eq("status", "PENDING")
                      .limit(1)
                      .doSelect();
    return result; // Trả về chuỗi JSON để hàm main xử lý
}

bool QueryDatabase::updateCommandStatus(String command_id, String new_status) {
    String json_data = "{\"status\": \"" + new_status + "\"}";
    int httpCode = db.update("device_commands").eq("id", command_id).doUpdate(json_data);
    return (httpCode == 200 || httpCode == 204);
}

bool QueryDatabase::updateStudentFingerprint(String student_id, String fingerprint_id) {
    String json_data = "{\"fingerprint_id\": \"" + fingerprint_id + "\"}";
    int httpCode = db.update("students").eq("student_id", student_id).doUpdate(json_data);
    return (httpCode == 200 || httpCode == 204);
}

bool QueryDatabase::clearStudentFingerprint(String student_id) {
    // Set fingerprint_id về null
    String json_data = "{\"fingerprint_id\": null}"; 
    int httpCode = db.update("students").eq("student_id", student_id).doUpdate(json_data);
    return (httpCode == 200 || httpCode == 204);
}

bool QueryDatabase::insertAttendance(String student_id, String fingerprint_id) {
    // Không cần gửi check_in_time vì database đã set default là NOW()
    String json_data = "{\"student_id\": \"" + student_id + "\", \"fingerprint_id\": \"" + fingerprint_id + "\", \"status\": \"SUCCESS\"}";
    int httpCode = db.insert("attendance_logs", json_data, false);
    return (httpCode == 201); // 201 là mã Created
}

String QueryDatabase::getStudentIdByFingerprint(String fingerprint_id) {
    String result = db.from("students")
                      .select("student_id")
                      .eq("fingerprint_id", fingerprint_id)
                      .doSelect();
    return result; // Trả về JSON chứa student_id
}

String QueryDatabase::extractJsonValue(String data, String key) {
    // 1. Tìm vị trí của từ khóa (ví dụ: "command_type")
    String searchKey = "\"" + key + "\"";
    int keyIndex = data.indexOf(searchKey);
    if (keyIndex == -1) return ""; // Không tìm thấy

    // 2. Tìm dấu hai chấm ':' ngay sau từ khóa
    int colonIndex = data.indexOf(":", keyIndex);
    if (colonIndex == -1) return "";

    // 3. Tìm dấu ngoặc kép '"' mở đầu của giá trị
    int startQuote = data.indexOf("\"", colonIndex);
    if (startQuote == -1) return "";

    // 4. Tìm dấu ngoặc kép '"' kết thúc của giá trị
    int endQuote = data.indexOf("\"", startQuote + 1);
    if (endQuote == -1) return "";

    // 5. Cắt chuỗi lấy kết quả
    return data.substring(startQuote + 1, endQuote);
}