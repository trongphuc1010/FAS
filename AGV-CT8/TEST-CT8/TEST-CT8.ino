//CT8 
#include "control_step.h"
#include "sensor.h"
const int pin_bao_mat_line = 45;
const int pin_dung = 46;         
const unsigned long time_bao_mat_line_lau = 10000; 
const unsigned long time_cho_vach_2 = 8000;        
const unsigned long time_dung_lay_hang = 20000;    
const unsigned long thoi_gian_bo_qua_mat_line = 500; 
const int speed = 1200;
const int slowSpeed = 12000;
const int startSpeed = 12000;
const unsigned long thoi_gian_tang_toc = 9000;
const unsigned long thoi_gian_giam_toc = 6000;
const int BU_ZONE_123 = 100;   const float Kp_123 = 100.0;  const float Kd_123 = 100.0;
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
bool xe_dang_bi_khoa = false; 

// --- THÊM: Biến lưu thời điểm bật công tắc nguồn ---
unsigned long thoi_gian_cap_nguon = 0; 

int trang_thai_tram = 0; 
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
    pinMode(cb1, INPUT_PULLUP); 
    
    digitalWrite(ENA_TRAI, LOW); digitalWrite(ENA_PHAI, LOW);
    
    // Đánh dấu thời điểm bật điện
    thoi_gian_cap_nguon = millis(); 
    KhoiTaoThongSoDeBa();
}

void loop() 
{
    raw_sensor = In_SenSor();
    
    if (KiemTraAnToan()) return;
    DocVaLocCamBien();
    if (XuLyTram()) return;
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

bool KiemTraAnToan() {
    bool co_vat_can = (digitalRead(cb1) == LOW); 
    bool mat_line = (raw_sensor == 13); 

    if (co_vat_can) 
    {    
        step_dc(false, false, HIGH, LOW, 0, 0); 
        xe_dang_bi_khoa = true; 
        
        if (!dang_gap_su_co) {
            dang_gap_su_co = true;
            t_bat_dau_su_co = millis(); 
        } else if (millis() - t_bat_dau_su_co >= time_bao_mat_line_lau) 
        {
            digitalWrite(pin_bao_mat_line, LOW); 
        }
        return true; 
    } 
    else if (mat_line) 
    {
        if (!dang_gap_su_co) {
            dang_gap_su_co = true;
            t_bat_dau_su_co = millis(); 
        }
        
        if (millis() - t_bat_dau_su_co < thoi_gian_bo_qua_mat_line) 
        {
            return false; 
        } else {
            step_dc(false, false, HIGH, LOW, 0, 0); 
            xe_dang_bi_khoa = true; 
            
            if (millis() - t_bat_dau_su_co >= time_bao_mat_line_lau) 
            {
                digitalWrite(pin_bao_mat_line, LOW); 
            }
            return true; 
        }
    } 
    else {
        if (dang_gap_su_co) 
        {
            dang_gap_su_co = false;
            digitalWrite(pin_bao_mat_line, HIGH); 
            if (xe_dang_bi_khoa) {
                KhoiTaoThongSoDeBa(); 
                trang_thai_tram = 0; 
                xe_dang_bi_khoa = false;
            }
        }
        return false;
    }
}
void DocVaLocCamBien() {
    if (raw_sensor == 14 && trang_thai_tram == 0) {
        if (abs(last_valid_sensor) >= 5 || abs(smoothedError) > 4.5) 
        {
            raw_sensor = (last_valid_sensor > 0) ? 9 : -9;
        }
    }
    is_special_code = (raw_sensor == 12 || raw_sensor == 13 || raw_sensor == 14);
    if (!is_special_code) {
        steering_error = raw_sensor;
        last_valid_sensor = raw_sensor;
    } else {
        if (raw_sensor == 13) {
            steering_error = last_valid_sensor; 
        } else {
            steering_error = 0; 
        }
    }
}
bool XuLyTram() {
    unsigned long now = millis();
    if (now - thoi_gian_cap_nguon < thoi_gian_tang_toc) 
    {
        return false; 
    }
    if (trang_thai_tram == 0) 
    {
        if (raw_sensor == 14) 
        {
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
            else if (now - t_gap_vach_1 >= time_cho_vach_2) 
            { 
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
void QuyetDinhXuatXung() 
{
    TinhToanBiendangTocDo();
    TinhToanVaXuatPID();
}
void TinhToanBiendangTocDo() 
{
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
    smoothedError = ((float)steering_error * 0.15) + (smoothedError * 0.85); 
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
    derivative = constrain(derivative, -2.0, 2.0); 
    float rawPid = (current_Kp * smoothedError) + (Ki * integral) + (current_Kd * derivative);
    lastError = smoothedError;
    float filteredPid = (rawPid * 0.05) + (lastPidOutput * 0.95);
    if (filteredPid > lastPidOutput + 100) filteredPid = lastPidOutput + 100;
    else if (filteredPid < lastPidOutput - 100) filteredPid = lastPidOutput - 100;
    lastPidOutput = filteredPid;
    int PHUC_DEPTRAI = constrain(finalBaseSpeed - (int)filteredPid, speed, startSpeed);
    int PHUC_KDEP = constrain(finalBaseSpeed + (int)filteredPid, speed, startSpeed);
    step_dc(true, true, HIGH, LOW, PHUC_KDEP, PHUC_DEPTRAI);
}