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