
const int TG = 31;      const int PG = 30;
const int T1 = 32;      const int P1 = 29;
const int T2 = 33;      const int P2 = 28;
const int T3 = 34;      const int P3 = 27;
const int T4 = 35;      const int P4 = 26;
void setup() {
  Serial.begin(115200);
  
  pinMode(T4, INPUT); pinMode(T3, INPUT); pinMode(T2, INPUT); 
  pinMode(T1, INPUT); pinMode(TG, INPUT); pinMode(PG, INPUT); 
  pinMode(P1, INPUT); pinMode(P2, INPUT); pinMode(P3, INPUT); 
  pinMode(P4, INPUT);
  Serial.println("   [■] = Thấy vạch | [  .  ] = Trống          ");
  Serial.println("===========================================");
  delay(2000);
}

void loop() 
{
  int s[10];
  s[0] = digitalRead(T4); s[1] = digitalRead(T3); s[2] = digitalRead(T2);
  s[3] = digitalRead(T1); s[4] = digitalRead(TG); s[5] = digitalRead(PG);
  s[6] = digitalRead(P1); s[7] = digitalRead(P2); s[8] = digitalRead(P3);
  s[9] = digitalRead(P4);
  Serial.print("SENSORS: | ");
  
  for (int i = 0; i < 10; i++) {
    if (i == 5) Serial.print("  |  "); 
    if (s[i] == 0) {
      Serial.print("[■]");
    } else {
      Serial.print("  .  "); 
    }
  }
  int error = In_SenSor_Simple(s); 
  
  Serial.print(" | -> ERROR: ");
  Serial.println(error);

  delay(20);
}
int In_SenSor_Simple(int s[]) 
{
  if (s[4] == 0 && s[5] == 0) return 0;
  if (s[0] == 0) return -9;
  if (s[9] == 0) return 9;
  return 13;
}