#include "LiquidCrystal_I2C.h" // Library LCD
#include "PZEM004Tv30.h" // Library PZEM004Tv30
#include <stdlib.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Setting alamat LCD 0x27
#define Relay1 6 // Init pin Relay1
#define Relay2 7 // Init pin Relay2
#define Relay3 8 // Init pin Relay3
#define Relay4 9 // Init pin Relay4

// SoftwareSerial komunikasi Arduino - PZEM004Tv30
PZEM004Tv30 pzem(2, 3);

unsigned long previousmillis = 0;
#define ssid "Pohon Belimbing" // Nama Wifi
#define password "klentang25" // Password Wifi
bool TimeToSend = false;
bool Cap1 = false, Cap2 = false, Cap3 = false, Cap4 = false;
char datacommand[100] = "";
float voltage, powerfactor, current;

#define detik 1000
uint8_t Detik = 0;

// Fungsi untuk membaca tegangan dari PZEM004Tv30
float cekvolt() {
  float voltage = pzem.voltage();
  if (voltage >= 0.0) {
  }
  else {
    voltage = 0.0;
  }
  return voltage;  
}

// Fungsi untuk membaca tegangan dari PZEM004Tv30
float cekcurrent() {
  float current = pzem.current();
  if (current >= 0.0) {
  }
  else {
    current = 0.0;
  }
  return current;  
}

// Fungsi untuk membaca power faktor dari PZEM004Tv30
float cekpf() {
  float pf = pzem.pf();
  if (pf >= 0.0) {
  }
  else {
    pf = 0.0;
  }
  return pf;  
}

uint8_t currentcount = 0, lastcurrentcount = 0;
// Fungsi untuk cek power faktor dan aktifkan relay jika diperlukan
void CekPF(float powfac) {
  if (powfac > 0.1 && powfac < 0.85) {
//    if (currentcount < 4) currentcount++;
    current = cekcurrent();
    if(current > 0.00) {
      if(current < 1.00) {
        currentcount = 1;
      }
      else if(current < 2.00) {
        currentcount = 2;
      }
      else if(current < 3.00) {
        currentcount = 3;
      }
      else if(current < 4.00) {
        currentcount = 4;
      }
    }
  }
  else if (powfac <= 0.1) { currentcount = 0; }

  if (lastcurrentcount != currentcount) {
    TimeToSend = true;
    lastcurrentcount = currentcount;
    switch (currentcount) {
      case 0 :
        Cap1 = 0;
        Cap2 = 0;
        Cap3 = 0;
        Cap4 = 0;
        digitalWrite(Relay1, LOW);
        digitalWrite(Relay2, LOW);
        digitalWrite(Relay3, LOW);
        digitalWrite(Relay4, LOW);
        break;
      case 1 :
        Cap1 = 1;
        Cap2 = 0;
        Cap3 = 0;
        Cap4 = 0;
        digitalWrite(Relay1, HIGH);
        digitalWrite(Relay2, LOW);
        digitalWrite(Relay3, LOW);
        digitalWrite(Relay4, LOW);
        break;
      case 2:
        Cap1 = 0;
        Cap2 = 1;
        Cap3 = 0;
        Cap4 = 0;
        digitalWrite(Relay1, LOW);
        digitalWrite(Relay2, HIGH);
        digitalWrite(Relay3, LOW);
        digitalWrite(Relay4, LOW);
        break;
      case 3:
        Cap1 = 0;
        Cap2 = 0;
        Cap3 = 1;
        Cap4 = 0;
        digitalWrite(Relay1, LOW);
        digitalWrite(Relay2, LOW);
        digitalWrite(Relay3, HIGH);
        digitalWrite(Relay4, LOW);
        break;
      case 4:
        Cap1 = 0;
        Cap2 = 0;
        Cap3 = 0;
        Cap4 = 1;
        digitalWrite(Relay1, LOW);
        digitalWrite(Relay2, LOW);
        digitalWrite(Relay3, LOW);
        digitalWrite(Relay4, HIGH);
        break;
    }
  }
}

bool found = false;
// Fungsi untuk kirim perintah AT Command dari Arduino ke ESP8266
bool sendcommand(const char* comm, int maxTime, char readReplay[], int millisecond) {
  uint8_t countTimeCommand = 0;
  uint8_t countTrueCommand = 0;
  found = false;
  while(countTimeCommand < (maxTime*1)) {
    Serial.println(comm);
    unsigned long startTime = millis();
    int milliseconds = millisecond*1000;
    while (millis() - startTime < milliseconds) {
      if(Serial.find(readReplay)) {
        found = true;
        break;
      }
    }
    if(found == true) { break; }
    countTimeCommand++;
  }
  
  if(found == true) {
    countTrueCommand++;
    countTimeCommand = 0;
  }
  
  if(found == false) {
    countTrueCommand = 0;
    countTimeCommand = 0;
  }
  return found;
}

// Fungsi untuk kirim data dari Arduino ke Thingspeak;
void updateTemp(float Data1, float Data2, uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6) {
  sendcommand("AT+CIPMUX=0",5,"OK",2);
  memset(datacommand, 0, sizeof(datacommand));
  sprintf(datacommand, "AT+CIPSTART=\"TCP\",\"184.106.153.149\",80"); delay(10);
  sendcommand(datacommand,5,"OK",2);

  char result[7]="", results[7]="";
  dtostrf(Data1, 3, 1, result);
  dtostrf(Data2, 3, 1, results);
  sprintf(datacommand, "GET /update?api_key=ZU9ZYS8CO24B3IN3&field1=%s&field2=%s&field3=%d&field4=%d&field5=%d&field6=%d\r\n",
                        result, results, Data3, Data4, Data5, Data6);
  char cipsend[20] = "";
  sprintf(cipsend, "AT+CIPSEND=%d", strlen(datacommand));
  sendcommand(cipsend,5,">",2);

  if (found) {
    delay(1000); sendcommand(datacommand,5,"OK",2);
  }
  else {
    sendcommand("AT+CIPCLOSE",5,"OK",2);
  }
}

void setup() {
  Serial.begin(115200);
  lcd.begin();
  delay(1000);

  lcd.setCursor(0, 0); lcd.print("   CONNECTING   ");
  lcd.setCursor(0, 1); lcd.print("    TO  WIFI    ");

  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(Relay3, OUTPUT);
  pinMode(Relay4, OUTPUT);

  delay(500);
  digitalWrite(Relay1, LOW);
  digitalWrite(Relay2, LOW);
  digitalWrite(Relay3, LOW);
  digitalWrite(Relay4, LOW);

  delay(500);
  sendcommand("AT",5,"OK",2);
  sendcommand("AT+CWMODE=1",5,"OK",2);
  sendcommand("AT+CWJAP=\"OPPO F7\",\"natathecoco\"",20,"OK",17);
  delay(1000);
  if(found) { updateTemp(voltage, powerfactor, Cap1, Cap2, Cap3, Cap4); delay(500); }
  previousmillis = millis();
  TimeToSend = false;
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("  FAKTOR  DAYA  ");
  lcd.setCursor(0, 1); lcd.print("V:       PF:");

}

void loop() {
  if (millis() - previousmillis >= detik) {
    previousmillis = millis();
    voltage = cekvolt();
    powerfactor = cekpf();
    CekPF(powerfactor);
    lcd.setCursor(2, 1); lcd.print("      ");
    lcd.setCursor(12, 1); lcd.print("    ");
    lcd.setCursor(2, 1); lcd.print(voltage);
    lcd.setCursor(12, 1); lcd.print(powerfactor);
    Detik++;
    if (Detik >= 60) {
      TimeToSend = true;
      Detik = 0;
    }
  }

  if (TimeToSend) {
    TimeToSend = false;
    if (cekkoneksi()) {
      updateTemp(voltage, powerfactor, Cap1, Cap2, Cap3, Cap4);
    }
    else {
    }
  }
}

// Fungsi untuk cekkoneksi antara ES8266 ke Wifi apakah masih terhubung atau tidak
bool cekkoneksi() {
  sendcommand("AT",5,"OK",2);
  sendcommand("AT+CIPSTATUS",1,":2",2);
  if (!found) { sendcommand("AT+CWJAP=\"OPPO F7\",\"natathecoco\"",20,"OK",17); }
  else if (found) { return true; }
}
