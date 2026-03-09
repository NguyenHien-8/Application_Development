# Supabase Database Schema

```sql
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

CREATE TABLE IF NOT EXISTS public.teacher_ids (
    id SERIAL PRIMARY KEY,
    teacher_id TEXT UNIQUE NOT NULL,
    is_registered BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

INSERT INTO public.teacher_ids (teacher_id) VALUES
('TNHGV01'), ('TNHGV02'), ('TNHGV03'), ('TNHGV04'), ('TNHGV05')
ON CONFLICT (teacher_id) DO NOTHING;

CREATE OR REPLACE FUNCTION public.check_teacher_before_signup(
    p_teacher_id TEXT, p_email TEXT
) RETURNS JSONB LANGUAGE plpgsql SECURITY DEFINER AS $$
DECLARE
    v_valid_count INT; v_teacher_exists INT; v_email_exists INT;
BEGIN
    SELECT COUNT(*) INTO v_valid_count FROM public.teacher_ids WHERE teacher_id = p_teacher_id AND is_registered = FALSE;
    SELECT COUNT(*) INTO v_teacher_exists FROM public.teachers WHERE teacher_id = p_teacher_id;
    SELECT COUNT(*) INTO v_email_exists FROM public.teachers WHERE email = p_email;

    IF v_valid_count = 0 THEN
        IF EXISTS (SELECT 1 FROM public.teacher_ids WHERE teacher_id = p_teacher_id AND is_registered = TRUE) OR v_teacher_exists > 0 THEN
            RETURN jsonb_build_object('success', false, 'message', 'The teacher is ID has been registered.');
        ELSE
            RETURN jsonb_build_object('success', false, 'message', 'The teacher ID is invalid.');
        END IF;
    ELSIF v_email_exists > 0 THEN
        RETURN jsonb_build_object('success', false, 'message', 'Email has already been used.');
    ELSE
        RETURN jsonb_build_object('success', true, 'message', 'Registration is available.');
    END IF;
END;
$$;

-- XÓA BỎ HÀM LOGIN CŨ (theo teacher_id)
DROP FUNCTION IF EXISTS public.signin_with_teacher_id(TEXT, TEXT);

-- THÊM HÀM MỚI: KIỂM TRA ĐĂNG NHẬP BẰNG EMAIL & PASSWORD
CREATE OR REPLACE FUNCTION public.signin_with_email(
    p_email TEXT, p_password TEXT
) RETURNS JSONB LANGUAGE plpgsql SECURITY DEFINER AS $$
DECLARE
    v_user_id UUID; 
    v_encrypted_pw TEXT;
    v_teacher_id TEXT;
BEGIN
    -- Lấy thông tin user từ bảng auth.users (Bảng nội bộ của Supabase)
    SELECT u.id, u.encrypted_password, t.teacher_id 
    INTO v_user_id, v_encrypted_pw, v_teacher_id
    FROM auth.users u
    LEFT JOIN public.teachers t ON u.id = t.user_id
    WHERE u.email = p_email;

    -- 1. Nếu email không tồn tại -> Sai Email
    IF v_user_id IS NULL THEN 
        RETURN jsonb_build_object('success', false, 'message', 'Incorrect Email.'); 
    END IF;

    -- 2. Nếu email tồn tại -> Kiểm tra Password
    IF v_encrypted_pw = crypt(p_password, v_encrypted_pw) THEN
        -- 3. Đúng cả hai
        RETURN jsonb_build_object(
            'success', true, 
            'message', 'Login Success.', 
            'email', p_email, 
            'teacher_id', v_teacher_id
        );
    ELSE
        -- 4. Sai Password
        RETURN jsonb_build_object('success', false, 'message', 'Incorrect Password.');
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

    UPDATE public.teacher_ids SET is_registered = TRUE WHERE teacher_id = v_teacher_id;
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
    class_id UUID NOT NULL REFERENCES public.classes(id) ON DELETE CASCADE,
    fingerprint_id TEXT UNIQUE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

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
    item JSONB;
    v_action TEXT;
    v_student_id TEXT;
    v_name TEXT;
    v_class_id UUID;
    v_fingerprint_id TEXT;
    
    v_existing_id UUID;
    v_existing_name TEXT;
    v_existing_fingerprint TEXT;
    v_existing_class_id UUID;
    
    results JSONB := '[]'::JSONB;
BEGIN
    IF auth.uid() IS NULL THEN
        RETURN jsonb_build_object('success', false, 'message', 'Unauthorized user.');
    END IF;

    FOR item IN SELECT * FROM jsonb_array_elements(payload)
    LOOP
        v_action := item->>'action'; 
        v_student_id := item->>'student_id';
        v_name := item->>'name';
        v_class_id := (item->>'class_id')::UUID;
        v_fingerprint_id := item->>'fingerprint_id';

        IF v_action = 'delete' THEN
            DELETE FROM public.students WHERE student_id = v_student_id;
            results := results || jsonb_build_object('student_id', v_student_id, 'status', 'deleted', 'message', 'The student has been removed.');
            
        ELSIF v_action = 'save' THEN
            SELECT id, name, fingerprint_id, class_id
            INTO v_existing_id, v_existing_name, v_existing_fingerprint, v_existing_class_id
            FROM public.students WHERE student_id = v_student_id;

            IF NOT FOUND THEN
                BEGIN
                    INSERT INTO public.students (student_id, name, class_id, fingerprint_id)
                    VALUES (v_student_id, v_name, v_class_id, v_fingerprint_id);
                    results := results || jsonb_build_object('student_id', v_student_id, 'status', 'inserted', 'message', 'Student information has been added.');
                EXCEPTION WHEN unique_violation THEN
                    IF SQLERRM LIKE '%student_id%' THEN
                        results := results || jsonb_build_object('student_id', v_student_id, 'status', 'error', 'message', 'The student code has already been duplicated');
                    ELSE
                        results := results || jsonb_build_object('student_id', v_student_id, 'status', 'error', 'message', 'The fingerprint code has already been duplicated.');
                    END IF;
                END;
            ELSE
                IF v_existing_name = v_name AND v_existing_fingerprint = v_fingerprint_id AND v_existing_class_id = v_class_id THEN
                    results := results || jsonb_build_object('student_id', v_student_id, 'status', 'unchanged', 'message', 'The student is information is already in the system.');
                ELSE
                    BEGIN
                        UPDATE public.students
                        SET name = v_name, class_id = v_class_id, fingerprint_id = v_fingerprint_id
                        WHERE student_id = v_student_id;
                        results := results || jsonb_build_object('student_id', v_student_id, 'status', 'updated', 'message', 'Student information has been updated.');
                    EXCEPTION WHEN unique_violation THEN
                        IF SQLERRM LIKE '%student_id%' THEN
                            results := results || jsonb_build_object('student_id', v_student_id, 'status', 'error', 'message', 'The student code has already been duplicated');
                        ELSE
                            results := results || jsonb_build_object('student_id', v_student_id, 'status', 'error', 'message', 'The fingerprint code has already been duplicated.');
                        END IF;
                    END;
                END IF;
            END IF;
        END IF;
    END LOOP;

    RETURN jsonb_build_object('success', true, 'data', results);
END;
$$;

-- ==========================================
-- BỔ SUNG: BẢNG QUERY_FINGERPRINT
-- ==========================================
CREATE TABLE IF NOT EXISTS public.query_fingerprint (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    student_id TEXT UNIQUE NOT NULL REFERENCES public.students(student_id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    fingerprint_id TEXT UNIQUE NOT NULL,
    fingerprint_data TEXT DEFAULT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Bật RLS cho bảng mới
ALTER TABLE public.query_fingerprint ENABLE ROW LEVEL SECURITY;
CREATE POLICY "Cho phép đọc dữ liệu query_fingerprint" ON public.query_fingerprint FOR SELECT USING (true);

-- ==========================================
-- HÀM RPC ĐỂ ESP32 GỌI LÊN ĐỒNG BỘ TRẠNG THÁI
-- ==========================================
CREATE OR REPLACE FUNCTION public.sync_sensor_fingerprints(sensor_ids TEXT[])
RETURNS JSONB 
LANGUAGE plpgsql 
SECURITY DEFINER 
AS $$
BEGIN
    -- 1. Tự động đồng bộ học sinh từ bảng students sang query_fingerprint 
    -- (Đảm bảo danh sách luôn mới nhất trước khi check vân tay)
    INSERT INTO public.query_fingerprint (student_id, name, fingerprint_id)
    SELECT student_id, name, fingerprint_id FROM public.students
    ON CONFLICT (student_id) DO UPDATE
    SET name = EXCLUDED.name,
        fingerprint_id = EXCLUDED.fingerprint_id;

    -- 2. Cập nhật thành 'Registered' cho các ID CÓ TRONG mảng sensor_ids do ESP32 gửi lên
    UPDATE public.query_fingerprint
    SET fingerprint_data = 'Registered'
    WHERE fingerprint_id = ANY(sensor_ids);

    -- 3. Cập nhật thành 'Not Registered' cho các ID KHÔNG CÓ TRONG mảng sensor_ids
    UPDATE public.query_fingerprint
    SET fingerprint_data = 'Not Registered'
    WHERE NOT (fingerprint_id = ANY(sensor_ids)) OR fingerprint_data IS NULL;

    RETURN jsonb_build_object('success', true, 'message', 'Đã đồng bộ trạng thái vân tay thành công.');
END;
$$;

-- ==========================================
-- PHẦN 6: BẢNG NHẬN LỆNH TỪ APP XUỐNG ESP32 (REALTIME)
-- ==========================================
CREATE TABLE IF NOT EXISTS public.device_commands (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    command TEXT NOT NULL,          -- Tên lệnh, ví dụ: 'SYNC_FINGERPRINTS'
    status TEXT DEFAULT 'pending',  -- Trạng thái: pending, completed...
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Cấu hình Row Level Security (RLS) để cho phép đọc/ghi đối với bảng lệnh
ALTER TABLE public.device_commands ENABLE ROW LEVEL SECURITY;
CREATE POLICY "Allow full access to device_commands" 
ON public.device_commands FOR ALL USING (true) WITH CHECK (true);

-- ==========================================
-- BẬT TÍNH NĂNG SUPABASE REALTIME CHO BẢNG NÀY
-- ==========================================
-- Supabase yêu cầu phải add bảng vào publication 'supabase_realtime' thì WebSockets mới hoạt động
BEGIN;
  DROP PUBLICATION IF EXISTS supabase_realtime;
  CREATE PUBLICATION supabase_realtime;
COMMIT;
ALTER PUBLICATION supabase_realtime ADD TABLE public.device_commands;
```