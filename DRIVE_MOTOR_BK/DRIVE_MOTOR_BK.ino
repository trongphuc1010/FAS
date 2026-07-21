#include <LiquidCrystal.h>
#include <TimerOne.h> 
#include <EEPROM.h> 

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// --- MOTOR QUẤN (TRỤC CHÍNH) ---
#define PUL_TRAI  A0
#define DIR_TRAI  A1
#define ENA_TRAI  A2
#define CB_CHOT_VITRI A15 

// --- NÚT NHẤN ---
#define NUT_CHAY 22
#define NUT_TANG 24
#define NUT_GIAM 26

int Chieu_quay = LOW;
int tocdo_nhanh = 110; 
int tocdo_cham = 350;  
int TAT_DRIVE = HIGH; 
int BAT_DRIVE = LOW;  

volatile bool trang_thai_xung_trai = false;
volatile bool dang_chay = false;         

const int SO_KICH_1_VONG = 1; 
int so_vong_cai_dat = 1; 
const int DIA_CHI_NHO = 0; 
long tong_so_lan_kich = 0; 
long dem_so_lan_kich = 0;  

bool trang_thai_cb_truoc = HIGH; 
bool da_thoat_home = false; 

// Biến chống dội nút nhấn
bool tt_chay_truoc = HIGH; 
bool tt_tang_truoc = HIGH;
bool tt_giam_truoc = HIGH;

unsigned long thoi_gian_chot_nhieu = 0; 
const int DO_TRE_CHONG_NHIEU = 50; 

// ==========================================
// HÀM NGẮT TIMER: BĂM XUNG MOTOR QUẤN
// ==========================================
void phat_xung_motor() 
{
  if (dang_chay == true) 
  {
    trang_thai_xung_trai = !trang_thai_xung_trai;
    digitalWrite(PUL_TRAI, trang_thai_xung_trai);
  }
}

// ==========================================
// HÀM: ĐƯA MOTOR QUẤN VỀ GỐC
// ==========================================
void ve_goc_motor_quan() 
{
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Tim mat doc A15.");
  lcd.setCursor(0, 1); lcd.print("Vui long cho!   ");

  digitalWrite(DIR_TRAI, Chieu_quay);
  digitalWrite(ENA_TRAI, BAT_DRIVE); 
  
  // 1. Nhích ra khỏi mắt đọc nếu đang bị đè trúng
  while (digitalRead(CB_CHOT_VITRI) == LOW) 
  {
    digitalWrite(PUL_TRAI, HIGH); delayMicroseconds(tocdo_cham);
    digitalWrite(PUL_TRAI, LOW); delayMicroseconds(tocdo_cham);
  }
  
  delay(100); 

  // 2. Chạy từ từ tìm mắt đọc và chốt cứng khi gặp (LOW)
  while (digitalRead(CB_CHOT_VITRI) == HIGH) 
  {
    digitalWrite(PUL_TRAI, HIGH); delayMicroseconds(tocdo_cham);
    digitalWrite(PUL_TRAI, LOW); delayMicroseconds(tocdo_cham);
  }
  
  digitalWrite(ENA_TRAI, BAT_DRIVE); 
}

void setup() 
{
  pinMode(PUL_TRAI, OUTPUT); pinMode(DIR_TRAI, OUTPUT); pinMode(ENA_TRAI, OUTPUT);
  pinMode(CB_CHOT_VITRI, INPUT_PULLUP); 
  pinMode(NUT_CHAY, INPUT_PULLUP); pinMode(NUT_TANG, INPUT_PULLUP); pinMode(NUT_GIAM, INPUT_PULLUP);
  
  digitalWrite(DIR_TRAI, Chieu_quay);
  digitalWrite(ENA_TRAI, TAT_DRIVE); 
  
  EEPROM.get(DIA_CHI_NHO, so_vong_cai_dat);
  if (so_vong_cai_dat < 1 || so_vong_cai_dat > 9999) { so_vong_cai_dat = 1; }
  
  lcd.begin(16, 2); 
  
  // Tự động tìm vị trí xỏ dây lúc vừa bật máy
  ve_goc_motor_quan();
  
  cap_nhat_man_hinh_cho(); 
  
  Timer1.initialize(tocdo_nhanh); 
  Timer1.attachInterrupt(phat_xung_motor); 
}

void loop() 
{
  // ==========================================
  // 1. VÙNG ĐỌC NÚT NHẤN CÀI ĐẶT
  // ==========================================
  if (dang_chay == false) 
  {
    // NÚT TĂNG
    bool tt_tang_hien_tai = digitalRead(NUT_TANG);
    if (tt_tang_truoc == HIGH && tt_tang_hien_tai == LOW) 
    {
      delay(20); 
      if(digitalRead(NUT_TANG) == LOW) {
        so_vong_cai_dat++;
        EEPROM.put(DIA_CHI_NHO, so_vong_cai_dat); 
        cap_nhat_man_hinh_cho(); 
      }
    }
    tt_tang_truoc = tt_tang_hien_tai;
    
    // NÚT GIẢM
    bool tt_giam_hien_tai = digitalRead(NUT_GIAM);
    if (tt_giam_truoc == HIGH && tt_giam_hien_tai == LOW) 
    {
      delay(20); 
      if(digitalRead(NUT_GIAM) == LOW) {
        if (so_vong_cai_dat > 1) { 
          so_vong_cai_dat--;
          EEPROM.put(DIA_CHI_NHO, so_vong_cai_dat); 
          cap_nhat_man_hinh_cho();
        }
      }
    }
    tt_giam_truoc = tt_giam_hien_tai;
    
    // NÚT CHẠY 
    bool tt_chay_hien_tai = digitalRead(NUT_CHAY);
    if (tt_chay_truoc == HIGH && tt_chay_hien_tai == LOW) 
    {
      delay(20); 
      if(digitalRead(NUT_CHAY) == LOW) 
      {
        digitalWrite(ENA_TRAI, BAT_DRIVE); 
        
        tong_so_lan_kich = so_vong_cai_dat * SO_KICH_1_VONG; 
        dem_so_lan_kich = 0;
        da_thoat_home = false; 
        
        if (tong_so_lan_kich == 1) { Timer1.setPeriod(tocdo_cham); } 
        else { Timer1.setPeriod(tocdo_nhanh); }
        
        lcd.clear(); 
        lcd.setCursor(0, 0); lcd.print("Set: "); lcd.print(so_vong_cai_dat);
        lcd.setCursor(9, 0); lcd.print("| QUAN ");
        lcd.setCursor(0, 1); lcd.print("Thuc te: 0      "); 
        
        dang_chay = true;
      }
    }
    tt_chay_truoc = tt_chay_hien_tai; 
  }

  // ==========================================
  // 2. VÙNG THỰC THI QUẤN
  // ==========================================
  if (dang_chay == true) 
  {
    bool trang_thai_cb_hien_tai = digitalRead(CB_CHOT_VITRI);
    
    // Nhận diện lúc vấu qua khỏi mắt đọc ở đầu vòng quay
    if (da_thoat_home == false) 
    {
      if (trang_thai_cb_hien_tai == HIGH) { da_thoat_home = true; trang_thai_cb_truoc = HIGH; }
    } 
    else 
    {
      // Nhận diện lúc vấu đập lại vào mắt đọc (hoàn thành 1 vòng)
      if (trang_thai_cb_truoc == HIGH && trang_thai_cb_hien_tai == LOW) 
      {
        if (millis() - thoi_gian_chot_nhieu > DO_TRE_CHONG_NHIEU) 
        {
          dem_so_lan_kich++; 
          thoi_gian_chot_nhieu = millis(); 
          
          lcd.setCursor(9, 1); 
          lcd.print(dem_so_lan_kich);
          
          // Hoàn thành đủ số vòng cài đặt -> DỪNG VÀ CHỜ LƯỢT MỚI
          if (dem_so_lan_kich >= tong_so_lan_kich) 
          {
            dang_chay = false;
            cap_nhat_man_hinh_cho(); // Cập nhật lại giao diện chờ chạy
          }
          // Chuẩn bị vào vòng cuối -> giảm tốc độ để tránh trượt trớn
          else if (dem_so_lan_kich == (tong_so_lan_kich - 1)) 
          {
            Timer1.setPeriod(tocdo_cham); 
          }
        }
      }
      trang_thai_cb_truoc = trang_thai_cb_hien_tai;
    }
  }
}

void cap_nhat_man_hinh_cho() 
{
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Set: "); lcd.print(so_vong_cai_dat);
  lcd.setCursor(9, 0); lcd.print("| QUAN ");
  lcd.setCursor(0, 1); lcd.print("San sang QUAN!  ");
}