#include "control_step.h"
#include "sensor.h"
const int pin_bao_mat_line = 45;
const int pin_dung = 46; 
const int pin_tin_hieu_15 = 44; 
const unsigned long time_bao_mat_line_lau = 30000;
const unsigned long time_hu_line = 50;
const unsigned long thoi_gian_tha_hang = 6000; 
const int time_tang_toc_case14_lan_1 = 8000;    
const unsigned long time_dung_lay_hang = 20000; 
const unsigned long THOI_GIAN_QUAY_MU = 4000;
const unsigned long thoi_gian_bao_chuong = 10000; //báo chuông
const unsigned long thoi_gian_15_giam_toc = 1500; // 
const unsigned long thoi_gian_15_dung = 1000;     // 
const unsigned long thoi_gian_15_lui = 3000;      // 
const int toc_do_lui = 3000;                      // 
const int speed = 1200;
const int slowSpeed = 12000;
const int startSpeed = 12000;
int speed_quay_dau = 4000;
const unsigned long thoi_gian_tang_toc = 9000;
const unsigned long thoi_gian_giam_toc = 6000;
const int BU_ZONE_123 = 0;   const float Kp_123 = 50.0;  const float Kd_123 = 500.0;
const int BU_ZONE_456 = 400; const float Kp_456 = 150.0; const float Kd_456 = 700.0;
const int BU_ZONE_789 = 500; const float Kp_789 = 200.0; const float Kd_789 = 800.0;
const float Ki = 0.01;
const float TIME_VAO_CUA = 200.0; 
const float TIME_RA_CUA  = 300.0; 
enum TrangThaiTram
{
    CHAY_BINH_THUONG = 0,
    BAT_DAU_GIAM_TOC = 1,
    CHO_DUNG = 2,
    DANG_DUNG_LAY_HANG = 3,
    ROI_KHOI_TRAM = 4,
    QUAY_DAU = 5,
    TAM_DUNG_DE_THA = 6,
    GIAM_TOC_15 = 7,      
    DUNG_CHO_LUI_15 = 8,   
    DANG_LUI_15 = 9,       
    DUNG_CHO_TIEN_15 = 10  
};

TrangThaiTram trang_thai_14 = CHAY_BINH_THUONG;

unsigned long thoi_gian_bat_dau = 0, thoi_gian_bat_dau_giam = 0, t_dung = 0;
unsigned long t_chong_doi_tram = 0;
unsigned long last_ramp_time = 0;
unsigned long thoi_gian_bat_dau_quay = 0;
unsigned long thoi_gian_mat_line = 0;
unsigned long t_bat_dau_tam_dung = 0;
unsigned long t_bat_dau_tha = 0;
unsigned long t_bat_dau_chuong = 0;
bool dang_bao_chuong = false;
unsigned long t_bat_dau_15_tien = 0;
unsigned long t_bat_dau_15_dung_1 = 0;
unsigned long t_bat_dau_15_lui = 0;
unsigned long t_bat_dau_15_dung_2 = 0;
bool dang_tha_hang = false;
bool dang_mat_line = false;
bool dang_dung_vi_vat_can = false;
bool dang_dung_vi_mat_line = false;
bool dang_tang_toc = true, dang_giam_toc = false;
bool is_special_code = false;
float toc_do_khi_bat_dau_giam = 0, toc_do_khi_bat_dau_tang = 0;
float currentSpeedVal = startSpeed, smoothBaseSpeed = 0;
float smoothedError = 0, lastError = 0, integral = 0, lastPidOutput = 0;
int raw_sensor = 0;
int steering_error = 0;
int last_valid_sensor = 0;
unsigned long thoi_gian_cap_nguon = 0;
void setup()
{
    pinMode(pin_dung, OUTPUT); digitalWrite(pin_dung, HIGH); 
    pinMode(pin_bao_mat_line, OUTPUT); digitalWrite(pin_bao_mat_line, HIGH);
    pinMode(pin_tin_hieu_15, OUTPUT); digitalWrite(pin_tin_hieu_15, HIGH);
    pinMode(PUL_TRAI, OUTPUT); pinMode(DIR_TRAI, OUTPUT); pinMode(ENA_TRAI, OUTPUT);
    pinMode(PUL_PHAI, OUTPUT); pinMode(DIR_PHAI, OUTPUT); pinMode(ENA_PHAI, OUTPUT);
    pinMode(T4, INPUT); pinMode(T3, INPUT); pinMode(T2, INPUT); pinMode(T1, INPUT);
    pinMode(TG, INPUT); pinMode(PG, INPUT);
    pinMode(P1, INPUT); pinMode(P2, INPUT); pinMode(P3, INPUT); pinMode(P4, INPUT);
    pinMode(cb1, INPUT_PULLUP);
    digitalWrite(ENA_TRAI, LOW); digitalWrite(ENA_PHAI, LOW);
    thoi_gian_cap_nguon = millis();
    KhoiTaoThongSoDeBa();
    if (In_SenSor() == 14)
    {
        trang_thai_14 = ROI_KHOI_TRAM;
    }
}
void loop()
{
    raw_sensor = In_SenSor();
    bool dung_vat_can = KiemTraAnToanGap(); 
    bool dung_mat_line = XuLyMatLine();
    if (dung_vat_can || dung_mat_line) return; 
    DocVaLocCamBien();
    XuLyMayTrangThai();
    XuLyNamCham();
    QuyetDinhXuatXung();
}
void KhoiTaoThongSoDeBa()
{
    currentSpeedVal = (float)startSpeed;
    smoothBaseSpeed = (float)startSpeed;
    toc_do_khi_bat_dau_tang = (float)startSpeed;
    thoi_gian_bat_dau = millis();
    last_ramp_time = millis();
    dang_tang_toc = true; dang_giam_toc = false;
    integral = 0; lastError = 0; smoothedError = 0;
    lastPidOutput = 0;
    last_valid_sensor = 0;
}
bool KiemTraAnToanGap()
{
    bool dang_lam_nhiem_vu = (trang_thai_14 == CHO_DUNG || trang_thai_14 == DANG_DUNG_LAY_HANG || trang_thai_14 == TAM_DUNG_DE_THA || trang_thai_14 >= GIAM_TOC_15);
    bool phai_dung_lai = false;
    if (digitalRead(cb1) == LOW && !dang_lam_nhiem_vu) 
    {
        step_dc(false, false, HIGH, LOW, 0, 0); 
        dang_dung_vi_vat_can = true; 
        phai_dung_lai = true;
    } 
    else 
    {
        if (dang_dung_vi_vat_can)
        {
            KhoiTaoThongSoDeBa(); 
            dang_dung_vi_vat_can = false;
        }
    }
    return phai_dung_lai;
}
bool XuLyMatLine()
{
    bool phai_dung = false;
    if (raw_sensor == 13) 
    {
        if (!dang_mat_line)
        {
            dang_mat_line = true; 
            thoi_gian_mat_line = millis(); 
        }
        if (millis() - thoi_gian_mat_line >= time_hu_line)
        {
            step_dc(false, false, HIGH, LOW, 0, 0);
            dang_dung_vi_mat_line = true; 
            phai_dung = true;
        }
        if (millis() - thoi_gian_mat_line >= time_bao_mat_line_lau)
        {
            digitalWrite(pin_bao_mat_line, LOW); 
        }
    }
    else if (abs(raw_sensor) <= 2 || raw_sensor == 14 || raw_sensor == 15) 
    {
        dang_mat_line = false;
        digitalWrite(pin_bao_mat_line, HIGH); 
        if (dang_dung_vi_mat_line)
        {
            KhoiTaoThongSoDeBa(); 
            dang_dung_vi_mat_line = false;
        }
    }
    else 
    {
        if (dang_dung_vi_mat_line) phai_dung = true; 
    }

    return phai_dung;
}
void DocVaLocCamBien()
{
    if ((raw_sensor == 14 || raw_sensor == 15) && trang_thai_14 == CHAY_BINH_THUONG)
    {
        if (abs(last_valid_sensor) >= 6 || abs(smoothedError) > 5.5)
        {
            raw_sensor = (last_valid_sensor > 0) ? 9 : -9;
        }
    }
    is_special_code = (raw_sensor == 12 || raw_sensor == 13 || raw_sensor == 14 || raw_sensor == 15);
    if (!is_special_code)
    {
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
void XuLyNamCham() 
{
    if (dang_tha_hang) 
    {
        if (millis() - t_bat_dau_tha >= thoi_gian_tha_hang) 
        {
            dang_tha_hang = false;
            digitalWrite(pin_dung, HIGH);
        }
    }
}
void XuLyMayTrangThai()
{
    unsigned long now = millis();
    if (now - thoi_gian_cap_nguon < thoi_gian_tang_toc) 
    {
        if(raw_sensor == 14 || raw_sensor == 15) return;
    }
    if (dang_bao_chuong && (now - t_bat_dau_chuong >= thoi_gian_bao_chuong)) 
    {
        dang_bao_chuong = false;
        digitalWrite(pin_tin_hieu_15, HIGH); // Tắt chuông
    }
    if (raw_sensor == 12 && trang_thai_14 == CHAY_BINH_THUONG)
    {
        trang_thai_14 = QUAY_DAU; thoi_gian_bat_dau_quay = now;
    }
    if (trang_thai_14 == QUAY_DAU)
    {
        if (now - thoi_gian_bat_dau_quay >= THOI_GIAN_QUAY_MU)
        {
            if (raw_sensor == 0)
            {
                trang_thai_14 = CHAY_BINH_THUONG; KhoiTaoThongSoDeBa();
            }
        }
        return;
    }
    if (trang_thai_14 == CHAY_BINH_THUONG) 
    {
        if (raw_sensor == 14)
        {
            trang_thai_14 = BAT_DAU_GIAM_TOC; t_chong_doi_tram = now;
            dang_tang_toc = false; dang_giam_toc = true;  
            thoi_gian_bat_dau_giam = now; toc_do_khi_bat_dau_giam = smoothBaseSpeed;
        } 
        else if (raw_sensor == 15) 
        {
            trang_thai_14 = GIAM_TOC_15;
            t_bat_dau_15_tien = now;
            dang_tang_toc = false; dang_giam_toc = true;
            thoi_gian_bat_dau_giam = now; toc_do_khi_bat_dau_giam = smoothBaseSpeed;
        }
    }
    if (trang_thai_14 == BAT_DAU_GIAM_TOC && !is_special_code && (now - t_chong_doi_tram > 150))
    {
        trang_thai_14 = CHO_DUNG;
    }
    else if (trang_thai_14 == CHO_DUNG && raw_sensor == 14)
    {
        if (now - t_chong_doi_tram > 500)
        {
            trang_thai_14 = DANG_DUNG_LAY_HANG; t_dung = now;
            
            dang_bao_chuong = true;
            t_bat_dau_chuong = now;
            digitalWrite(pin_tin_hieu_15, LOW); 
        }
    }
    if ((trang_thai_14 == BAT_DAU_GIAM_TOC || trang_thai_14 == CHO_DUNG) && (now - thoi_gian_bat_dau_giam > time_tang_toc_case14_lan_1))
    {
        trang_thai_14 = TAM_DUNG_DE_THA; 
        t_bat_dau_tam_dung = now; 
        dang_tha_hang = true; t_bat_dau_tha = now; digitalWrite(pin_dung, LOW); 
    }
    
    if (trang_thai_14 == TAM_DUNG_DE_THA && (now - t_bat_dau_tam_dung >= 500))
    {
        trang_thai_14 = CHAY_BINH_THUONG; KhoiTaoThongSoDeBa();
    }
    
    if (trang_thai_14 == DANG_DUNG_LAY_HANG && (now - t_dung >= time_dung_lay_hang))
    {
        trang_thai_14 = ROI_KHOI_TRAM; KhoiTaoThongSoDeBa();
    }
    if (trang_thai_14 == GIAM_TOC_15) {
        if (now - t_bat_dau_15_tien >= thoi_gian_15_giam_toc) {
            trang_thai_14 = DUNG_CHO_LUI_15;
            t_bat_dau_15_dung_1 = now;
        }
    }
    else if (trang_thai_14 == DUNG_CHO_LUI_15) {
        if (now - t_bat_dau_15_dung_1 >= thoi_gian_15_dung) { // Hết 1s dừng
            trang_thai_14 = DANG_LUI_15;
            t_bat_dau_15_lui = now;
        }
    }
    else if (trang_thai_14 == DANG_LUI_15) {
        if (now - t_bat_dau_15_lui >= thoi_gian_15_lui) { // Lùi đủ 3s
            trang_thai_14 = DUNG_CHO_TIEN_15;
            t_bat_dau_15_dung_2 = now;
        }
    }
    else if (trang_thai_14 == DUNG_CHO_TIEN_15) {
        if (now - t_bat_dau_15_dung_2 >= thoi_gian_15_dung) { // Hết 1s dừng
            trang_thai_14 = CHAY_BINH_THUONG;
            KhoiTaoThongSoDeBa(); // Đề ba đi tiếp
        }
    }
    if (trang_thai_14 == ROI_KHOI_TRAM && !is_special_code)
    {
        trang_thai_14 = CHAY_BINH_THUONG;
    }
}
void QuyetDinhXuatXung()
{

    if (trang_thai_14 == DANG_DUNG_LAY_HANG || trang_thai_14 == TAM_DUNG_DE_THA ||
        trang_thai_14 == DUNG_CHO_LUI_15 || trang_thai_14 == DUNG_CHO_TIEN_15)
    {
        step_dc(false, false, HIGH, LOW, 0, 0); last_ramp_time = millis(); return;
    }
    if (trang_thai_14 == DANG_LUI_15) 
    {
        step_dc(true, true, LOW, HIGH, toc_do_lui, toc_do_lui); last_ramp_time = millis(); return;
    }
    if (trang_thai_14 == QUAY_DAU)
    {
        step_dc(true, true, LOW, LOW, speed_quay_dau, speed_quay_dau); last_ramp_time = millis(); return;
    }
    
    TinhToanBiendangTocDo();
    TinhToanVaXuatPID();
}
void TinhToanBiendangTocDo()
{
    if (dang_tang_toc)
    {
        unsigned long t_chay = millis() - thoi_gian_bat_dau;
        if (t_chay < thoi_gian_tang_toc)
        {
            currentSpeedVal = toc_do_khi_bat_dau_tang - ((toc_do_khi_bat_dau_tang - speed) * ((float)t_chay / thoi_gian_tang_toc));
        } else {
            currentSpeedVal = (float)speed; dang_tang_toc = false;
        }
    }
    else if (dang_giam_toc)
    {
        unsigned long t_giam = millis() - thoi_gian_bat_dau_giam;
        if (t_giam < thoi_gian_giam_toc) {
            currentSpeedVal = toc_do_khi_bat_dau_giam + ((slowSpeed - toc_do_khi_bat_dau_giam) * ((float)t_giam / thoi_gian_giam_toc));
        } else {
            currentSpeedVal = (float)slowSpeed; dang_giam_toc = false;
        }
    }
    smoothBaseSpeed = (currentSpeedVal * 0.1) + (smoothBaseSpeed * 0.9);
}

void TinhToanVaXuatPID()
{
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
    
    if (millis() - last_ramp_time >= UPDATE_RATE)
    {
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

