//CT8 - FULL: CHỐNG KẸT TRẠM KHI ĐỀ BA + LƯỚT RÁCH LINE 0.5S + ĐỨNG IM KHÔNG GIẬT
#include "control_step.h"
#include "sensor.h"

const int pin_bao_mat_line = 45;
const int pin_dung = 46;         

const unsigned long time_bao_mat_line_lau = 10000; 
const unsigned long time_cho_vach_2 = 8000;        
const unsigned long time_dung_lay_hang = 20000;    
const unsigned long thoi_gian_bo_qua_mat_line = 500; // THÊM: 0.5s để lướt qua chỗ rách line

const int speed = 1200;
const int slowSpeed = 12000;
const int startSpeed = 12000;
const unsigned long thoi_gian_tang_toc = 9000;
const unsigned long thoi_gian_giam_toc = 6000;

const int BU_ZONE_123 = 130;   const float Kp_123 = 100.0;  const float Kd_123 = 100.0;
const int BU_ZONE_456 = 400; const float Kp_456 = 150.0; const float Kd_456 = 600.0;
const int BU_ZONE_789 = 500; const float Kp_789 = 200.0; const float Kd_789 = 700.0;
const float Ki = 0.01;
const float TIME_VAO_CUA = 200.0; 
const float TIME_RA_CUA  = 300.0; 

int raw_sensor = 0;
int steering_error = 0;
int last_valid_sensor = 0;
bool is_special_code = false;
bool dang_gap_su_co = false;
unsigned long t_bat_dau_su_co = 0;
int trang_thai_tram = 0; 
// Ý nghĩa trang_thai_tram:
// 0: BÌNH THƯỜNG
// 1: TÌM VẠCH LẦN 2 (Đã qua vạch 1, đang giảm tốc)
// 2: THẢ HÀNG (Hết 8s, kích chân 46)
// 3: LẤY HÀNG (Gặp vạch 2, đếm 20s)
// 4: RỜI TRẠM (Đề ba đi tiếp)

unsigned long t_gap_vach_1 = 0;
unsigned long t_bat_dau_xa_hang = 0;
unsigned long t_bat_dau_dung_20s = 0;

float currentSpeedVal = startSpeed, smoothBaseSpeed = startSpeed;
float toc_do_khi_bat_dau_giam = startSpeed, toc_do_khi_bat_dau_tang = startSpeed;
unsigned long thoi_gian_bat_dau = 0, thoi_gian_bat_dau_giam = 0, last_ramp_time = 0;
bool dang_tang_toc = true, dang_giam_toc = false;
float smoothedError = 0, lastError = 0, integral = 0, lastPidOutput = 0;

void setup() {
    pinMode(pin_dung, OUTPUT); digitalWrite(pin_dung, HIGH); 
    pinMode(pin_bao_mat_line, OUTPUT); digitalWrite(pin_bao_mat_line, HIGH);
    
    pinMode(PUL_TRAI, OUTPUT); pinMode(DIR_TRAI, OUTPUT); pinMode(ENA_TRAI, OUTPUT);
    pinMode(PUL_PHAI, OUTPUT); pinMode(DIR_PHAI, OUTPUT); pinMode(ENA_PHAI, OUTPUT);
    
    pinMode(T4, INPUT); pinMode(T3, INPUT); pinMode(T2, INPUT); pinMode(T1, INPUT);
    pinMode(TG, INPUT); pinMode(PG, INPUT);
    pinMode(P1, INPUT); pinMode(P2, INPUT); pinMode(P3, INPUT); pinMode(P4, INPUT);
    pinMode(cb1, INPUT_PULLUP); // Vật cản
    
    digitalWrite(ENA_TRAI, LOW); digitalWrite(ENA_PHAI, LOW);
    KhoiTaoThongSoDeBa();
}

void loop() 
{
    raw_sensor = In_SenSor();
    
    // 1. Kiểm tra an toàn (Vật cản hoặc Mất line)
    if (KiemTraAnToan()) return;
    
    // 2. Lọc mã đặc biệt
    DocVaLocCamBien();
    
    // 3. Xử lý Trạm
    if (XuLyTram()) return;
    
    // 4. Cấp xung di chuyển
    QuyetDinhXuatXung();
}

void KhoiTaoThongSoDeBa() 
{
    currentSpeedVal = (float)startSpeed;
    smoothBaseSpeed = (float)startSpeed;
    toc_do_khi_bat_dau_tang = (float)startSpeed;
    thoi_gian_bat_dau = millis();
    last_ramp_time = millis();
    dang_tang_toc = true; 
    dang_giam_toc = false;
    integral = 0; lastError = 0; smoothedError = 0;
    lastPidOutput = 0;
}

// ĐÃ SỬA: Tách riêng Vật cản và Mất line, khóa bánh chặt không cho giật
bool KiemTraAnToan() {
    bool co_vat_can = (digitalRead(cb1) == LOW); 
    bool mat_line = (raw_sensor == 13); // Mất line (Toàn bộ mắt bằng 1)

    // 1. NẾU CÓ VẬT CẢN -> DỪNG NGAY LẬP TỨC 0 GIÂY
    if (co_vat_can) 
    {    
        step_dc(false, false, HIGH, LOW, 0, 0); // Phanh cứng
        if (!dang_gap_su_co) {
            dang_gap_su_co = true;
            t_bat_dau_su_co = millis(); 
        } else if (millis() - t_bat_dau_su_co >= time_bao_mat_line_lau) {
            digitalWrite(pin_bao_mat_line, LOW); // Đứng 10s còi kêu
        }
        return true; // Trả về true để văng khỏi loop, không chạy PID -> Không giật
    } 
    // 2. NẾU MẤT LINE (Rách băng keo) -> LƯỚT 0.5 GIÂY RỒI MỚI DỪNG HẲN
    else if (mat_line) 
    {
        if (!dang_gap_su_co) {
            dang_gap_su_co = true;
            t_bat_dau_su_co = millis(); // Bắt đầu bấm giờ 0.5s
        }
        
        if (millis() - t_bat_dau_su_co < thoi_gian_bo_qua_mat_line) {
            // Dưới 0.5s: Bỏ qua lỗi, xe lướt thẳng bằng quán tính
            return false; 
        } else {
            // Quá 0.5s (rớt line thật): Khóa bánh đứng im phăng phắc
            step_dc(false, false, HIGH, LOW, 0, 0); 
            if (millis() - t_bat_dau_su_co >= time_bao_mat_line_lau) {
                digitalWrite(pin_bao_mat_line, LOW); 
            }
            return true; // Trả về true để cô lập PID -> Đứng im tuyệt đối
        }
    } 
    // 3. TRẠNG THÁI BÌNH THƯỜNG (Có line lại / Hết vật cản)
    else {
        if (dang_gap_su_co) {
            dang_gap_su_co = false;
            digitalWrite(pin_bao_mat_line, HIGH); 
            KhoiTaoThongSoDeBa(); // Reset để đề ba êm ái
        }
        return false;
    }
}

void DocVaLocCamBien() {
    is_special_code = (raw_sensor == 12 || raw_sensor == 13 || raw_sensor == 14);
    if (!is_special_code) {
        steering_error = raw_sensor;
        last_valid_sensor = raw_sensor;
    } else {
        steering_error = 0; 
    }
}

bool XuLyTram() {
    unsigned long now = millis();

    // CHIÊU CHỐNG KẸT TRẠM KHI BẬT NGUỒN: Bỏ qua 9s đầu
    if (now - thoi_gian_bat_dau < thoi_gian_tang_toc && dang_tang_toc) {
        return false; 
    }

    if (trang_thai_tram == 0) {
        if (raw_sensor == 14) {
            trang_thai_tram = 1;
            t_gap_vach_1 = now;
            dang_tang_toc = false;
            dang_giam_toc = true;
            thoi_gian_bat_dau_giam = now;
            toc_do_khi_bat_dau_giam = smoothBaseSpeed;
        }
    }
    else if (trang_thai_tram == 1) {
        if (now - t_gap_vach_1 > 500) {
            if (raw_sensor == 14) {
                trang_thai_tram = 3;
                t_bat_dau_dung_20s = now;
            }
            else if (now - t_gap_vach_1 >= time_cho_vach_2) { 
                trang_thai_tram = 2;
                t_bat_dau_xa_hang = now;
                digitalWrite(pin_dung, LOW); 
            }
        }
    }
    else if (trang_thai_tram == 2) {
        step_dc(false, false, HIGH, LOW, 0, 0); 
        if (now - t_bat_dau_xa_hang >= 500) {
            digitalWrite(pin_dung, HIGH); 
            trang_thai_tram = 4; 
            KhoiTaoThongSoDeBa();
        }
        return true; 
    }
    else if (trang_thai_tram == 3) {
        step_dc(false, false, HIGH, LOW, 0, 0); 
        if (now - t_bat_dau_dung_20s >= time_dung_lay_hang) { 
            trang_thai_tram = 4; 
            KhoiTaoThongSoDeBa();
        }
        return true; 
    }
    else if (trang_thai_tram == 4) {
        if (raw_sensor != 14 && raw_sensor != 13 && raw_sensor != 12) {
            trang_thai_tram = 0;
        }
    }
    return false; 
}

void QuyetDinhXuatXung() {
    TinhToanBiendangTocDo();
    TinhToanVaXuatPID();
}

void TinhToanBiendangTocDo() {
    if (dang_tang_toc) {
        unsigned long t_chay = millis() - thoi_gian_bat_dau;
        if (t_chay < thoi_gian_tang_toc) {
            currentSpeedVal = toc_do_khi_bat_dau_tang - ((toc_do_khi_bat_dau_tang - speed) * ((float)t_chay / thoi_gian_tang_toc));
        } else {
            currentSpeedVal = (float)speed; 
            dang_tang_toc = false;
        }
    }
    else if (dang_giam_toc) {
        unsigned long t_giam = millis() - thoi_gian_bat_dau_giam;
        if (t_giam < thoi_gian_giam_toc) {
            currentSpeedVal = toc_do_khi_bat_dau_giam + ((slowSpeed - toc_do_khi_bat_dau_giam) * ((float)t_giam / thoi_gian_giam_toc));
        } else {
            currentSpeedVal = (float)slowSpeed; 
            dang_giam_toc = false;
        }
    }
    smoothBaseSpeed = (currentSpeedVal * 0.1) + (smoothBaseSpeed * 0.9);
}

void TinhToanVaXuatPID() {
    int baseSpeed = (int)smoothBaseSpeed;
    smoothedError = ((float)steering_error * 0.35) + (smoothedError * 0.65);
    
    float target_Kp, target_Kd;
    int target_offset_raw;
    int absError = abs(steering_error);
    
    if (absError == 0)      { target_Kp = Kp_123; target_Kd = Kd_123; target_offset_raw = 0; }
    else if (absError <= 3) { target_Kp = Kp_123; target_Kd = Kd_123; target_offset_raw = BU_ZONE_123; }
    else if (absError <= 6) { target_Kp = Kp_456; target_Kd = Kd_456; target_offset_raw = BU_ZONE_456; }
    else                    { target_Kp = Kp_789; target_Kd = Kd_789; target_offset_raw = BU_ZONE_789; }
    
    float speed_ratio = (float)(startSpeed - baseSpeed) / (startSpeed - speed);
    speed_ratio = constrain(speed_ratio, 0.0, 1.0);
    int target_offset = (int)(target_offset_raw * speed_ratio);
    
    static float current_Kp = Kp_123;
    static float current_Kd = Kd_123;
    static float current_offset = 0.0;
    int UPDATE_RATE = 5;
    
    if (millis() - last_ramp_time >= UPDATE_RATE) {
        last_ramp_time = millis();
        float time_vao = max(TIME_VAO_CUA, 1.0f);
        float time_ra  = max(TIME_RA_CUA,  1.0f);
        float step_offset_vao = ((float)BU_ZONE_789 / time_vao) * UPDATE_RATE;
        float step_offset_ra  = ((float)BU_ZONE_789 / time_ra) * UPDATE_RATE;
        float step_Kp_vao = ((Kp_789 - Kp_123) / time_vao) * UPDATE_RATE;
        float step_Kp_ra  = ((Kp_789 - Kp_123) / time_ra) * UPDATE_RATE;
        float step_Kd_vao = ((Kd_789 - Kd_123) / time_vao) * UPDATE_RATE;
        float step_Kd_ra  = ((Kd_789 - Kd_123) / time_ra) * UPDATE_RATE;
        
        if (current_offset < target_offset) current_offset = min(current_offset + step_offset_vao, (float)target_offset);
        else if (current_offset > target_offset) current_offset = max(current_offset - step_offset_ra, (float)target_offset);
        if (current_Kp < target_Kp) current_Kp = min(current_Kp + step_Kp_vao, target_Kp);
        else if (current_Kp > target_Kp) current_Kp = max(current_Kp - step_Kp_ra, target_Kp);
        if (current_Kd < target_Kd) current_Kd = min(current_Kd + step_Kd_vao, target_Kd);
        else if (current_Kd > target_Kd) current_Kd = max(current_Kd - step_Kd_ra, target_Kd);
    }
    
    int finalBaseSpeed = baseSpeed + (int)current_offset;
    finalBaseSpeed = constrain(finalBaseSpeed, speed, startSpeed);
    integral = constrain(integral + smoothedError, -100, 100);
    float derivative = smoothedError - lastError;
    float rawPid = (current_Kp * smoothedError) + (Ki * integral) + (current_Kd * derivative);
    lastError = smoothedError;
    float filteredPid = (rawPid * 0.2) + (lastPidOutput * 0.8);
    lastPidOutput = filteredPid;
    
    int PHUC_DEPTRAI = constrain(finalBaseSpeed - (int)filteredPid, speed, startSpeed);
    int PHUC_KDEP = constrain(finalBaseSpeed + (int)filteredPid, speed, startSpeed);
    step_dc(true, true, HIGH, LOW, PHUC_KDEP, PHUC_DEPTRAI);
}