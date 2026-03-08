#ifndef QUERY_DATABASE_H
#define QUERY_DATABASE_H

#include <Arduino.h>
#include <ESPSupabase.h>

class QueryDatabase
{
private:
    Supabase db; // Khởi tạo đối tượng Supabase

public:
    // Khởi tạo kết nối
    void begin(String url, String key);

    // 1. Hàm lắng nghe lệnh từ App (Lấy lệnh có status = 'PENDING')
    String getPendingCommand();

    // 2. Hàm cập nhật trạng thái lệnh (Ví dụ: từ PENDING -> SUCCESS)
    bool updateCommandStatus(String command_id, String new_status);

    // 3. Hàm gán ID vân tay mới cho học sinh (Chế độ Đăng ký)
    bool updateStudentFingerprint(String student_id, String fingerprint_id);

    // 4. Hàm xóa vân tay của học sinh (Chế độ Xóa)
    bool clearStudentFingerprint(String student_id);

    // 5. Hàm đẩy log điểm danh lên server (Chế độ Check-in)
    bool insertAttendance(String student_id, String fingerprint_id);
    
    // 6. Lấy student_id từ fingerprint_id (Dùng khi quét vân tay check-in)
    String getStudentIdByFingerprint(String fingerprint_id);
};

#endif