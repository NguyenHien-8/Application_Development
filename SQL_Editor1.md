# Supabase Database

```sql
-- ==========================================
-- PHẦN CƠ BẢN: EXTENSION & TOKEN
-- ==========================================
-- Kích hoạt extension pgcrypto để mã hóa password
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- Khởi tạo các giá trị token mặc định cho user hiện tại (tránh lỗi null)
UPDATE auth.users 
SET 
    confirmation_token = COALESCE(confirmation_token, ''), 
    recovery_token = COALESCE(recovery_token, ''), 
    email_change_token_new = COALESCE(email_change_token_new, ''), 
    email_change = COALESCE(email_change, '');

-- ==========================================
-- PHẦN 1: CẤU TRÚC BẢNG VÀ USER AUTH
-- ==========================================

CREATE TABLE IF NOT EXISTS public.teachers (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    teacher_id TEXT UNIQUE NOT NULL,
    email TEXT UNIQUE NOT NULL,
    name TEXT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    user_id UUID UNIQUE REFERENCES auth.users(id) ON DELETE CASCADE
);

DROP TABLE IF EXISTS public.teacher_ids CASCADE;

CREATE TABLE IF NOT EXISTS public.teacher_ids (
    id SERIAL PRIMARY KEY,
    teacher_id TEXT UNIQUE NOT NULL,
    email TEXT UNIQUE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Cập nhật data mẫu cho giáo viên
INSERT INTO public.teacher_ids (teacher_id, email) VALUES
('TNHGV01', 'trannguyenhien29085@gmail.com'), 
('TNHGV02', 'nhanb2305247@student.ctu.edu.vn'), 
('TNHGV03', 'toanb2305266@student.ctu.edu.vn'), 
('TNHGV04', 'tilb2305264@student.ctu.edu.vn'), 
('TNHGV05', 'longb2305238@student.ctu.edu.vn')
ON CONFLICT (teacher_id) DO NOTHING;

DROP FUNCTION IF EXISTS public.signup_teacher(TEXT, TEXT, TEXT, TEXT);

-- HÀM SIGN UP
CREATE OR REPLACE FUNCTION public.signup_teacher(
    p_teacher_id TEXT, p_email TEXT, p_password TEXT, p_name TEXT
) RETURNS JSONB LANGUAGE plpgsql SECURITY DEFINER AS $$
DECLARE
    v_db_email TEXT; 
    v_db_teacher_id TEXT; 
    v_new_user_id UUID;
BEGIN
    SELECT email INTO v_db_email FROM public.teacher_ids WHERE teacher_id = p_teacher_id;
    SELECT teacher_id INTO v_db_teacher_id FROM public.teacher_ids WHERE email = p_email;

    IF v_db_email IS NULL AND v_db_teacher_id IS NULL THEN
        RETURN jsonb_build_object('success', false, 'message', 'Cả địa chỉ email và ID giáo viên đều không chính xác.');
    ELSIF v_db_email IS NOT NULL AND v_db_email != p_email THEN
        RETURN jsonb_build_object('success', false, 'message', 'Đăng ký email không chính xác.');
    ELSIF v_db_teacher_id IS NOT NULL AND v_db_teacher_id != p_teacher_id THEN
        RETURN jsonb_build_object('success', false, 'message', 'Đăng ký Teacher Id không chính xác.');
    ELSIF v_db_email = p_email AND v_db_teacher_id = p_teacher_id THEN
        IF EXISTS (SELECT 1 FROM auth.users WHERE email = p_email) OR 
           EXISTS (SELECT 1 FROM public.teachers WHERE teacher_id = p_teacher_id) THEN
            RETURN jsonb_build_object('success', false, 'message', 'Giáo viên đã được đăng ký.');
        END IF;

        v_new_user_id := gen_random_uuid();
        
        INSERT INTO auth.users (
            instance_id, id, aud, role, email, encrypted_password, email_confirmed_at, 
            raw_app_meta_data, raw_user_meta_data, created_at, updated_at,
            confirmation_token, recovery_token, email_change_token_new, email_change
        ) VALUES (
            '00000000-0000-0000-0000-000000000000', v_new_user_id, 'authenticated', 'authenticated', 
            p_email, crypt(p_password, gen_salt('bf')), NOW(), 
            '{"provider":"email","providers":["email"]}'::jsonb, 
            jsonb_build_object('teacher_id', p_teacher_id, 'name', p_name), NOW(), NOW(),
            '', '', '', ''
        );

        INSERT INTO auth.identities (
            id, user_id, provider_id, identity_data, provider, last_sign_in_at, created_at, updated_at
        ) VALUES (
            gen_random_uuid(), v_new_user_id, v_new_user_id::text, 
            jsonb_build_object('sub', v_new_user_id, 'email', p_email, 'email_verified', true, 'phone_verified', false),
            'email', NOW(), NOW(), NOW()
        );

        RETURN jsonb_build_object('success', true, 'message', 'Đăng ký thành công.');
    ELSE
        RETURN jsonb_build_object('success', false, 'message', 'Thông tin đăng nhập không hợp lệ.');
    END IF;
EXCEPTION WHEN OTHERS THEN
    RETURN jsonb_build_object('success', false, 'message', 'Database Error: ' || SQLERRM);
END;
$$;

DROP FUNCTION IF EXISTS public.signin_with_email(TEXT, TEXT);

-- HÀM SIGN IN
CREATE OR REPLACE FUNCTION public.signin_with_email(
    p_email TEXT, p_password TEXT
) RETURNS JSONB LANGUAGE plpgsql SECURITY DEFINER AS $$
DECLARE
    v_user_id UUID; v_encrypted_pw TEXT; v_teacher_id TEXT;
BEGIN
    SELECT u.id, u.encrypted_password, t.teacher_id 
    INTO v_user_id, v_encrypted_pw, v_teacher_id
    FROM auth.users u
    LEFT JOIN public.teachers t ON u.id = t.user_id
    WHERE u.email = p_email;

    IF v_user_id IS NULL THEN 
        RETURN jsonb_build_object('success', false, 'message', 'Đăng nhập thành công.'); 
    END IF;

    IF v_encrypted_pw = crypt(p_password, v_encrypted_pw) THEN
        RETURN jsonb_build_object('success', true, 'message', 'Đăng nhập thành công.', 'email', p_email, 'teacher_id', v_teacher_id);
    ELSE
        RETURN jsonb_build_object('success', false, 'message', 'Đăng nhập bằng mật khẩu không chính xác.');
    END IF;
END;
$$;

DROP TRIGGER IF EXISTS on_auth_user_created ON auth.users;

CREATE OR REPLACE FUNCTION public.handle_new_user() RETURNS TRIGGER LANGUAGE plpgsql SECURITY DEFINER AS $$
DECLARE
    v_teacher_id TEXT; v_name TEXT;
BEGIN
    v_teacher_id := NEW.raw_user_meta_data->>'teacher_id';
    v_name := NEW.raw_user_meta_data->>'name';
    INSERT INTO public.teachers (user_id, teacher_id, email, name)
    VALUES (NEW.id, v_teacher_id, NEW.email, v_name);
    RETURN NEW;
END;
$$;

CREATE TRIGGER on_auth_user_created
    AFTER INSERT ON auth.users
    FOR EACH ROW EXECUTE FUNCTION public.handle_new_user();

-- ==========================================
-- PHẦN 2: BẢNG LỚP HỌC VÀ HỌC SINH
-- ==========================================

CREATE TABLE IF NOT EXISTS public.classes (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    class_name TEXT NOT NULL,
    academic_year TEXT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    UNIQUE(class_name, academic_year)
);

CREATE TABLE IF NOT EXISTS public.teacher_class (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    teacher_id TEXT NOT NULL REFERENCES public.teachers(teacher_id) ON DELETE CASCADE,
    class_id UUID NOT NULL REFERENCES public.classes(id) ON DELETE CASCADE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    UNIQUE(teacher_id, class_id)
);

CREATE TABLE IF NOT EXISTS public.students (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    student_id TEXT UNIQUE NOT NULL, 
    name TEXT NOT NULL,
    gender TEXT CHECK (gender IN ('Male', 'Female', 'Other')),
    date_of_birth DATE,
    class_id UUID NOT NULL REFERENCES public.classes(id) ON DELETE CASCADE,
    fingerprint_id TEXT UNIQUE, 
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Ràng buộc định dạng mã học sinh
ALTER TABLE public.students DROP CONSTRAINT IF EXISTS chk_student_id_format;
ALTER TABLE public.students ADD CONSTRAINT chk_student_id_format CHECK (student_id ~ '^H[0-9]{6}$');

CREATE INDEX IF NOT EXISTS idx_students_class_id ON public.students(class_id);

-- TRIGGER KIỂM TRA ĐỊNH DẠNG MÃ HỌC SINH
CREATE OR REPLACE FUNCTION public.format_student_id_trigger()
RETURNS TRIGGER AS $$
BEGIN
    NEW.student_id := UPPER(TRIM(NEW.student_id));
    
    IF length(NEW.student_id) != 7 THEN
        RAISE EXCEPTION 'Mã số sinh viên phải có chính xác 7 ký tự.';
    END IF;
    
    IF NEW.student_id !~ '^H[0-9]{6}$' THEN
        RAISE EXCEPTION 'Mã số sinh viên phải có định dạng H + 6 chữ số (Ví dụ: H230501).';
    END IF;

    IF NEW.date_of_birth IS NOT NULL THEN
        IF SUBSTRING(NEW.student_id FROM 4 FOR 2) != TO_CHAR(NEW.date_of_birth, 'YY') THEN
            RAISE EXCEPTION 'Mã số sinh viên của thành phần X2 phải trùng khớp với hai chữ số cuối của năm sinh.';
        END IF;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_format_student_id ON public.students;
CREATE TRIGGER trg_format_student_id
BEFORE INSERT OR UPDATE ON public.students
FOR EACH ROW
EXECUTE FUNCTION public.format_student_id_trigger();

-- ==========================================
-- PHẦN 3: THIẾT LẬP PHÂN QUYỀN RLS
-- ==========================================
ALTER TABLE public.classes ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.teacher_class ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.students ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "Teacher views assigned classes" ON public.classes;
DROP POLICY IF EXISTS "Teacher views own assignments" ON public.teacher_class;
DROP POLICY IF EXISTS "Teacher views students in assigned classes" ON public.students;
DROP POLICY IF EXISTS "Teacher can insert students to assigned classes" ON public.students;
DROP POLICY IF EXISTS "Teacher can update students in assigned classes" ON public.students;
DROP POLICY IF EXISTS "Teacher can delete students in assigned classes" ON public.students;

CREATE POLICY "Teacher views assigned classes" ON public.classes FOR SELECT TO authenticated USING (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = classes.id AND t.user_id = auth.uid()));
CREATE POLICY "Teacher views own assignments" ON public.teacher_class FOR SELECT TO authenticated USING (EXISTS (SELECT 1 FROM public.teachers t WHERE t.teacher_id = teacher_class.teacher_id AND t.user_id = auth.uid()));
CREATE POLICY "Teacher views students in assigned classes" ON public.students FOR SELECT TO authenticated USING (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = students.class_id AND t.user_id = auth.uid()));

CREATE POLICY "Teacher can insert students to assigned classes" ON public.students FOR INSERT TO authenticated WITH CHECK (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = students.class_id AND t.user_id = auth.uid()));
CREATE POLICY "Teacher can update students in assigned classes" ON public.students FOR UPDATE TO authenticated USING (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = students.class_id AND t.user_id = auth.uid()));
CREATE POLICY "Teacher can delete students in assigned classes" ON public.students FOR DELETE TO authenticated USING (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = students.class_id AND t.user_id = auth.uid()));

-- ==========================================
-- PHẦN 4: HÀM ĐỒNG BỘ HỌC SINH (SYNC STUDENTS)
-- ==========================================
CREATE OR REPLACE FUNCTION public.sync_students(payload JSONB)
RETURNS JSONB
LANGUAGE plpgsql
SECURITY INVOKER
AS $$
DECLARE
    v_inferred_class_id UUID;
    v_academic_year TEXT;
    v_start_year_suffix TEXT;
    v_stolen_students TEXT;
    v_invalid_format_students TEXT;
    v_invalid_year_students TEXT;
    v_invalid_dob_students TEXT; 
    v_duplicate_students TEXT;  
    v_results JSONB;
BEGIN
    IF auth.uid() IS NULL THEN 
        RETURN jsonb_build_object('success', false, 'message', 'Người dùng trái phép.'); 
    END IF;

    SELECT tc.class_id, c.academic_year INTO v_inferred_class_id, v_academic_year
    FROM public.teacher_class tc
    JOIN public.teachers t ON tc.teacher_id = t.teacher_id
    JOIN public.classes c ON tc.class_id = c.id
    WHERE t.user_id = auth.uid()
    LIMIT 1;

    IF v_inferred_class_id IS NULL THEN
        RETURN jsonb_build_object('success', false, 'message', 'Giáo viên này hiện chưa được phân công phụ trách lớp nào.');
    END IF;

    SELECT string_agg(x.student_id, ', ') INTO v_invalid_format_students
    FROM jsonb_to_recordset(payload) AS x(action TEXT, student_id TEXT)
    WHERE x.action = 'save' AND UPPER(TRIM(x.student_id)) !~ '^H[0-9]{6}$';

    IF v_invalid_format_students IS NOT NULL THEN
        RETURN jsonb_build_object('success', false, 'message', 'Mã số sinh viên phải có định dạng H + 6 chữ số (Ví dụ: H230501). Mã lỗi: ' || v_invalid_format_students);
    END IF;

    v_start_year_suffix := SUBSTRING(SPLIT_PART(v_academic_year, '-', 1) FROM 3 FOR 2);
    
    SELECT string_agg(x.student_id, ', ') INTO v_invalid_year_students
    FROM jsonb_to_recordset(payload) AS x(action TEXT, student_id TEXT)
    WHERE x.action = 'save' AND SUBSTRING(UPPER(TRIM(x.student_id)) FROM 2 FOR 2) != v_start_year_suffix;

    IF v_invalid_year_students IS NOT NULL THEN
        RETURN jsonb_build_object('success', false, 'message', 'Mã số sinh viên của thành phần X1 không trùng khớp với năm bắt đầu của năm học (' || v_academic_year || '). Yêu cầu kiểm tra lại: ' || v_invalid_year_students);
    END IF;

    SELECT string_agg(x.student_id, ', ') INTO v_invalid_dob_students
    FROM jsonb_to_recordset(payload) AS x(action TEXT, student_id TEXT, date_of_birth DATE)
    WHERE x.action = 'save' AND x.date_of_birth IS NOT NULL AND SUBSTRING(UPPER(TRIM(x.student_id)) FROM 4 FOR 2) != TO_CHAR(x.date_of_birth, 'YY');

    IF v_invalid_dob_students IS NOT NULL THEN
        RETURN jsonb_build_object('success', false, 'message', 'Mã số sinh viên của thành phần X2 không trùng khớp với năm sinh. Yêu cầu kiểm tra lại: ' || v_invalid_dob_students);
    END IF;

    SELECT string_agg(x.student_id, ', ') INTO v_stolen_students
    FROM jsonb_to_recordset(payload) AS x(action TEXT, student_id TEXT)
    JOIN public.students s ON s.student_id = UPPER(TRIM(x.student_id))
    WHERE x.action = 'save' AND s.class_id != v_inferred_class_id;

    IF v_stolen_students IS NOT NULL THEN
        RETURN jsonb_build_object('success', false, 'message', 'Các mã số sinh viên sau hiện đang theo học lớp của giáo viên khác. Vui lòng xóa thông tin từ lớp cũ trước khi thêm lớp mới: ' || v_stolen_students);
    END IF;

    SELECT string_agg(x.student_id, ', ') INTO v_duplicate_students
    FROM jsonb_to_recordset(payload) AS x(action TEXT, student_id TEXT, name TEXT, fingerprint_id TEXT, gender TEXT, date_of_birth DATE)
    JOIN public.students s ON s.student_id = UPPER(TRIM(x.student_id))
    WHERE x.action = 'save' AND s.name = TRIM(x.name) AND s.gender IS NOT DISTINCT FROM TRIM(x.gender) AND s.date_of_birth IS NOT DISTINCT FROM x.date_of_birth AND s.fingerprint_id IS NOT DISTINCT FROM NULLIF(TRIM(x.fingerprint_id), '');

    IF v_duplicate_students IS NOT NULL THEN
        RETURN jsonb_build_object('success', false, 'message', 'Thông tin của sinh viên này đã có sẵn.');
    END IF;

    DELETE FROM public.students 
    WHERE student_id IN (
        SELECT UPPER(TRIM(student_id))
        FROM jsonb_to_recordset(payload) AS x(action TEXT, student_id TEXT)
        WHERE action = 'delete'
    );

    WITH parsed_data AS (
        SELECT 
            UPPER(TRIM(student_id)) AS student_id, 
            TRIM(name) AS name, 
            TRIM(gender) AS gender,
            date_of_birth,
            NULLIF(TRIM(fingerprint_id), '') AS fingerprint_id
        FROM jsonb_to_recordset(payload) AS x(action TEXT, student_id TEXT, name TEXT, fingerprint_id TEXT, gender TEXT, date_of_birth DATE)
        WHERE action = 'save'
    ),
    upserted AS (
        INSERT INTO public.students (student_id, name, gender, date_of_birth, class_id, fingerprint_id)
        SELECT student_id, name, gender, date_of_birth, v_inferred_class_id, fingerprint_id
        FROM parsed_data
        ON CONFLICT (student_id) 
        DO UPDATE SET 
            name = EXCLUDED.name,
            gender = EXCLUDED.gender,
            date_of_birth = EXCLUDED.date_of_birth,
            fingerprint_id = EXCLUDED.fingerprint_id
        RETURNING student_id
    )
    SELECT jsonb_agg(jsonb_build_object('student_id', student_id, 'status', 'processed', 'message', 'Sinh viên đã được xử lý thành công.')) 
    INTO v_results
    FROM upserted;

    RETURN jsonb_build_object('success', true, 'data', COALESCE(v_results, '[]'::jsonb));
EXCEPTION WHEN unique_violation THEN
    RETURN jsonb_build_object('success', false, 'message', 'Mã vân tay đã bị trùng khớp ở nơi khác.');
END;
$$;

-- ==========================================
-- BỔ SUNG 1: BẢNG QUERY_FINGERPRINT
-- ==========================================
CREATE TABLE IF NOT EXISTS public.query_fingerprint (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    student_id TEXT UNIQUE NOT NULL REFERENCES public.students(student_id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    fingerprint_id TEXT UNIQUE NOT NULL,
    fingerprint_data TEXT DEFAULT NULL,
    status TEXT DEFAULT 'pending' CHECK (status IN ('pending', 'waiting_scan_1', 'waiting_scan_2', 'completed', 'failed')),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Cập nhật cột status nếu đã tạo bảng trước đó
ALTER TABLE public.query_fingerprint 
ADD COLUMN IF NOT EXISTS status TEXT DEFAULT 'pending' 
CHECK (status IN ('pending', 'waiting_scan_1', 'waiting_scan_2', 'completed', 'failed'));

ALTER TABLE public.query_fingerprint ENABLE ROW LEVEL SECURITY;
DROP POLICY IF EXISTS "Teacher views query_fingerprint of assigned classes" ON public.query_fingerprint;

CREATE POLICY "Teacher views query_fingerprint of assigned classes" ON public.query_fingerprint 
FOR SELECT TO authenticated 
USING (
    EXISTS (
        SELECT 1 
        FROM public.students s
        JOIN public.teacher_class tc ON s.class_id = tc.class_id
        JOIN public.teachers t ON tc.teacher_id = t.teacher_id
        WHERE s.student_id = query_fingerprint.student_id 
          AND t.user_id = auth.uid()
    )
);

CREATE OR REPLACE FUNCTION public.sync_sensor_fingerprints(sensor_ids TEXT[])
RETURNS JSONB 
LANGUAGE plpgsql 
SECURITY DEFINER 
AS $$
BEGIN
    INSERT INTO public.query_fingerprint (student_id, name, fingerprint_id)
    SELECT student_id, name, fingerprint_id FROM public.students
    ON CONFLICT (student_id) DO UPDATE
    SET name = EXCLUDED.name, fingerprint_id = EXCLUDED.fingerprint_id;

    UPDATE public.query_fingerprint
    SET fingerprint_data = 'Registered'
    WHERE fingerprint_id = ANY(sensor_ids);

    UPDATE public.query_fingerprint
    SET fingerprint_data = 'Not Registered'
    WHERE NOT (fingerprint_id = ANY(sensor_ids)) OR fingerprint_data IS NULL;

    RETURN jsonb_build_object('success', true, 'message', 'Trạng thái vân tay đã được đồng bộ hóa thành công.');
END;
$$;

-- ==========================================
-- BỔ SUNG 2: BẢNG LƯU LỆNH ĐIỀU KHIỂN & BẬT REALTIME 
-- ==========================================
CREATE TABLE IF NOT EXISTS public.device_commands (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    command TEXT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_publication WHERE pubname = 'supabase_realtime') THEN
        CREATE PUBLICATION supabase_realtime;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_publication_tables WHERE pubname = 'supabase_realtime' AND tablename = 'device_commands') THEN
        ALTER PUBLICATION supabase_realtime ADD TABLE public.device_commands;
    END IF;
END
$$;

CREATE OR REPLACE FUNCTION public.request_sync()
RETURNS JSONB
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
BEGIN
    INSERT INTO public.device_commands (command) VALUES ('SYNC_FINGERPRINTS');
    RETURN jsonb_build_object('success', true, 'message', 'Yêu cầu đồng bộ hóa dấu vân tay đã được gửi thành công.');
END;
$$;

-- ==========================================
-- BỔ SUNG 3: TÍNH NĂNG ENROLL VÂN TAY TỪ XA
-- ==========================================
CREATE OR REPLACE FUNCTION public.request_enroll(
    p_student_id TEXT, 
    p_fingerprint_id TEXT
)
RETURNS JSONB
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM public.students WHERE student_id = p_student_id) THEN
        RETURN jsonb_build_object('success', false, 'message', 'Không tìm thấy mã số sinh viên.');
    END IF;

    -- SỬA ĐỔI TẠI ĐÂY: Khi App gọi, trạng thái tự động chuyển thành waiting_scan_1
    INSERT INTO public.query_fingerprint (student_id, name, fingerprint_id, status)
    SELECT student_id, name, p_fingerprint_id, 'waiting_scan_1'
    FROM public.students WHERE student_id = p_student_id
    ON CONFLICT (student_id) DO UPDATE
    SET fingerprint_id = EXCLUDED.fingerprint_id, status = 'waiting_scan_1';

    INSERT INTO public.device_commands (command) 
    VALUES ('ENROLL_' || p_fingerprint_id);

    RETURN jsonb_build_object('success', true, 'message', 'Yêu cầu đăng ký đã được gửi thành công.', 'fingerprint_id', p_fingerprint_id);
END;
$$;

CREATE OR REPLACE FUNCTION public.update_enroll_status(
    p_fingerprint_id TEXT, 
    p_status TEXT, 
    p_message TEXT DEFAULT ''
)
RETURNS JSONB
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
BEGIN
    UPDATE public.query_fingerprint
    SET 
        status = p_status,
        fingerprint_data = CASE WHEN p_status = 'completed' THEN 'Registered' ELSE fingerprint_data END
    WHERE fingerprint_id = p_fingerprint_id;

    RETURN jsonb_build_object('success', true, 'message', 'Trạng thái đã được cập nhật thành công.');
END;
$$;

-- ==========================================
-- BỔ SUNG 4: TÍNH NĂNG DELETE VÂN TAY TỪ XA
-- ==========================================
CREATE OR REPLACE FUNCTION public.request_delete(
    p_student_id TEXT, 
    p_fingerprint_id TEXT
)
RETURNS JSONB
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM public.students WHERE student_id = p_student_id) THEN
        RETURN jsonb_build_object('success', false, 'message', 'Không tìm thấy mã số sinh viên.');
    END IF;

    INSERT INTO public.device_commands (command) 
    VALUES ('DELETE_' || p_fingerprint_id);

    RETURN jsonb_build_object('success', true, 'message', 'Yêu cầu xóa đã được gửi thành công.', 'fingerprint_id', p_fingerprint_id);
END;
$$;

CREATE OR REPLACE FUNCTION public.update_delete_status(
    p_fingerprint_id TEXT, 
    p_status TEXT, 
    p_message TEXT DEFAULT ''
)
RETURNS JSONB
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
BEGIN
    IF p_status = 'completed' THEN
        UPDATE public.query_fingerprint
        SET fingerprint_data = 'Not Registered'
        WHERE fingerprint_id = p_fingerprint_id;
    END IF;

    RETURN jsonb_build_object('success', true, 'message', 'Trạng thái xóa đã được cập nhật thành công.');
END;
$$;

-- ==========================================
-- BỔ SUNG 5: TÍNH NĂNG ĐIỂM DANH (ATTENDANCE)
-- ==========================================

-- 1. Tạo bảng Student Attendance
CREATE TABLE IF NOT EXISTS public.student_attendance (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    student_id TEXT NOT NULL REFERENCES public.students(student_id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    class_id UUID NOT NULL REFERENCES public.classes(id) ON DELETE CASCADE,
    date_of_school DATE NOT NULL,
    time TIME NOT NULL,
    session TEXT CHECK (session IN ('Morning', 'Afternoon', 'Evening')),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Bật bảo mật RLS cơ bản
ALTER TABLE public.student_attendance ENABLE ROW LEVEL SECURITY;

-- THÊM DÒNG DROP POLICY ĐỂ TRÁNH LỖI KHI CHẠY LẠI
DROP POLICY IF EXISTS "Teacher views attendance" ON public.student_attendance;

CREATE POLICY "Teacher views attendance" ON public.student_attendance FOR SELECT TO authenticated USING (true);

-- 2. Hàm RPC để ESP32 gọi khi quét vân tay điểm danh thành công
CREATE OR REPLACE FUNCTION public.log_attendance_by_fingerprint(
    p_fingerprint_id TEXT
)
RETURNS JSONB
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
DECLARE
    v_student_id TEXT;
    v_name TEXT;
    v_class_id UUID;
    v_session TEXT;
    v_current_tz TIMESTAMP := NOW() AT TIME ZONE 'Asia/Ho_Chi_Minh'; -- Lấy giờ thực tế tại Việt Nam
    v_time TIME := v_current_tz::TIME;
    v_date DATE := v_current_tz::DATE;
BEGIN
    -- 1. Tìm thông tin học sinh dựa vào mã vân tay
    SELECT student_id, name, class_id INTO v_student_id, v_name, v_class_id
    FROM public.students
    WHERE fingerprint_id = p_fingerprint_id
    LIMIT 1;

    -- Nếu không tìm thấy vân tay trong CSDL
    IF NOT FOUND THEN
        RETURN jsonb_build_object('success', false, 'message', 'Không tìm thấy học sinh với vân tay này.');
    END IF;

    -- 2. Xác định buổi học (Session) dựa vào thời gian
    IF v_time < '12:00:00'::TIME THEN
        v_session := 'Morning';
    ELSIF v_time < '18:00:00'::TIME THEN
        v_session := 'Afternoon';
    ELSE
        v_session := 'Evening';
    END IF;

    -- LƯU Ý: Đã gỡ bỏ logic "Chống Spam" cũ.
    -- Giờ đây, cứ mỗi lần ESP32 gọi hàm này là sẽ tạo ra 1 Record điểm danh mới, 
    -- không chặn, không ghi đè dữ liệu cũ.
    
    -- 3. Ghi nhận điểm danh vào bảng
    INSERT INTO public.student_attendance (student_id, name, class_id, date_of_school, time, session)
    VALUES (v_student_id, v_name, v_class_id, v_date, v_time, v_session);

    RETURN jsonb_build_object('success', true, 'message', 'Điểm danh thành công.', 'student_id', v_student_id, 'session', v_session, 'time', v_time);
END;
$$;

-- ==========================================
-- KÍCH HOẠT REALTIME CHO BẢNG query_fingerprint
-- (BỔ SUNG ĐỂ APP CÓ THỂ LẮNG NGHE THAY ĐỔI TRẠNG THÁI)
-- ==========================================
DO $$
BEGIN
    -- Tạo publication nếu chưa có
    IF NOT EXISTS (SELECT 1 FROM pg_publication WHERE pubname = 'supabase_realtime') THEN
        CREATE PUBLICATION supabase_realtime;
    END IF;

    -- Thêm bảng query_fingerprint vào publication nếu chưa có
    IF NOT EXISTS (
        SELECT 1 FROM pg_publication_tables 
        WHERE pubname = 'supabase_realtime' 
          AND tablename = 'query_fingerprint'
          AND schemaname = 'public'
    ) THEN
        ALTER PUBLICATION supabase_realtime ADD TABLE public.query_fingerprint;
    END IF;
END
$$;
```