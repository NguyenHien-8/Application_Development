#ifndef QUERY_DATABASE_H
#define QUERY_DATABASE_H

#include <Arduino.h>
#include <ESPSupabase.h>

class QueryDatabase
{
private:
    Supabase db;

public:
    void begin(String url, String key);
    
    // --- Các hàm gọi API ---
    String getPendingCommand();
    bool updateCommandStatus(String command_id, String new_status);
    bool updateStudentFingerprint(String student_id, String fingerprint_id);
    bool clearStudentFingerprint(String student_id);
    bool insertAttendance(String student_id, String fingerprint_id);
    String getStudentIdByFingerprint(String fingerprint_id);

    // --- Hàm Tự viết: Bóc tách JSON siêu nhẹ ---
    String extractJsonValue(String data, String key);
};

#endif