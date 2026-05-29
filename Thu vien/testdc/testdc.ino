#define PUL_TRAI  A0
#define DIR_TRAI  A1
#define ENA_TRAI  A2

#define PUL_PHAI  A3
#define DIR_PHAI  A4
#define ENA_PHAI  A5

unsigned long prevMicrosTrai = 0;
unsigned long prevMicrosPhai = 0;

int toc_do_trai = 1500;   // microseconds
int toc_do_phai = 1500;   // microseconds

bool trang_thai_xung_trai = false;
bool trang_thai_xung_phai = false;

// Biến đếm số bước để dễ theo dõi
long countTrai = 0;
long countPhai = 0;

void setup() {
  // Khởi tạo Serial với baudrate cao để giảm độ trễ
  Serial.begin(115200);
  
  pinMode(PUL_TRAI, OUTPUT);
  pinMode(DIR_TRAI, OUTPUT);
  pinMode(ENA_TRAI, OUTPUT);

  pinMode(PUL_PHAI, OUTPUT);
  pinMode(DIR_PHAI, OUTPUT);
  pinMode(ENA_PHAI, OUTPUT);

  digitalWrite(DIR_TRAI, HIGH);
  digitalWrite(DIR_PHAI, HIGH);
  digitalWrite(ENA_TRAI, LOW);
  digitalWrite(ENA_PHAI, LOW);
  
  Serial.println("He thong bat dau...");
}

void loop() {
  unsigned long currentMicros = micros();

  // Động cơ trái
  if (currentMicros - prevMicrosTrai >= toc_do_trai) 
  {
    prevMicrosTrai = currentMicros;
    trang_thai_xung_trai = !trang_thai_xung_trai;
    digitalWrite(PUL_TRAI, trang_thai_xung_trai);
    
    // In ra khi bắt đầu một xung mới (mức HIGH)
    if (trang_thai_xung_trai) 
    {
      countTrai++;
      Serial.print("L: ");
      Serial.println(countTrai);
    }
  }

  // Động cơ phải
  if (currentMicros - prevMicrosPhai >= toc_do_phai) 
  {
    prevMicrosPhai = currentMicros;
    trang_thai_xung_phai = !trang_thai_xung_phai;
    digitalWrite(PUL_PHAI, trang_thai_xung_phai);
    
    // In ra khi bắt đầu một xung mới (mức HIGH)
    if (trang_thai_xung_phai) {
      countPhai++;
      Serial.print("\t\t R: ");
      Serial.println(countPhai);
    }
  }
}