#include <SoftwareSerial.h>

// ... (keep all the existing constant definitions and pin configurations)
const int bientroX = A0 ;
const int bientroY = A1 ;
const int btn_bom = 9;
const int ena = 3;
const int in1 = 4;
const int in2_bom = 8;
const int role_led = 5;
bool is_bom = true;
bool is_led = true;
bool is_nhay = true;

int tocdo = 3;

//chân ST_CP của 74HC595
int latchPin = 10;
//chân SH_CP của 74HC595
int clockPin = 12;
//Chân DS của 74HC595
int dataPin = 11;
const int capBac[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const int tocDo[10] = {0, 167, 178, 189, 200, 211, 222, 233, 244, 255};
// Mảng cho LED 7 đoạn chung cực âm
// Các bit 1 sẽ làm các đoạn sáng
const int Seg[20] = {
  0b00111111,//0 - các thanh từ a-f sáng
  0b00000110,//1 - chỉ có 2 thanh b,c sáng
  0b01011011,//2
  0b01001111,//3
  0b01100110,//4
  0b01101101,//5
  0b01111101,//6
  0b00000111,//7
  0b01111111,//8
  0b01101111,//9
  0b10111111,//0 - các thanh từ a-f sáng và dấu chấm
  0b10000110,//1 - chỉ có 2 thanh b,c sáng và dấu chấm
  0b11011011,//2 và dấu chấm
  0b11001111,//3 và dấu chấm
  0b11100110,//4 và dấu chấm
  0b11101101,//5 và dấu chấm
  0b11111101,//6 và dấu chấm
  0b10000111,//7 và dấu chấm
  0b11111111,//8 và dấu chấm
  0b11101111,//9 và dấu chấm
};
#define RX_PIN 6  // Kết nối với chân TX của ESP32-CAM
#define TX_PIN 7  // Kết nối với chân RX của ESP32-CAM
SoftwareSerial espSerial(RX_PIN, TX_PIN);  // RX, TX

// ... (keep all the existing global variables)

void setup() {

  Serial.begin(9600);

  //  xf = analogRead(bientroX);
  //  yf = analogRead(bientroY);

  //brown switch
  pinMode(btn_bom, INPUT);
  pinMode(bientroX, INPUT);
  pinMode(bientroY, INPUT);

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(role_led, OUTPUT);
  digitalWrite (role_led, 1);

  pinMode (ena, OUTPUT);
  pinMode (in1, OUTPUT);
  analogWrite (ena, tocDo[tocdo]);
  digitalWrite (in1, HIGH);

  pinMode(in2_bom, OUTPUT);
  digitalWrite (in2_bom, HIGH);

  delay(1000);
  is_bom = true;


  espSerial.begin(9600);  // Start serial communication with ESP32-CAM
  Serial.println("đã khởi động");
}

void sendToESP32(int airPumpSpeed, bool bigLight, bool waterPump) {
  char buffer[20];
  sprintf(buffer, "TXofNANO(%d,%d,%d)", airPumpSpeed, bigLight, waterPump);
  espSerial.println(buffer);
  Serial.print("gửi esp"); Serial.println(buffer);
}

void receiveFromESP32() {
  if (espSerial.available()) {
    Serial.println("có tín hiệu gửi từ esp:");

    String data = espSerial.readStringUntil('\n');
    Serial.println(data);
    if (data.startsWith("TXofESP32(")) {
      int values[3];
      sscanf(data.c_str(), "TXofESP32(%d,%d,%d)", &values[0], &values[1], &values[2]);

      tocdo = values[0];
      is_led = values[1];
      is_bom = values[2];
      Serial.print("nhận của esp"); Serial.print(tocdo); Serial.print(is_led); Serial.print( is_bom); Serial.println();
      // Update device states based on received values
      if (tocdo == 0) {
        is_nhay = true;
        suc_khi_stop();
      }
      else {
        digitalWrite (in1, HIGH);
      }
      analogWrite(ena, tocDo[tocdo]);
      digitalWrite(role_led, is_led ? HIGH : LOW);
      digitalWrite(in2_bom, is_bom ? HIGH : LOW);
    }
  }
}
void suc_khi_stop()
{
  analogWrite (ena, 0);
  digitalWrite (in1, LOW);
}

void tangToc (int i)
{
  tocdo += i;
  if (tocdo > 9) {
    tocdo = 9;
  }
  analogWrite (ena, tocDo[tocdo]);
  Serial.print("tocDo[tocdo]"); Serial.println(tocDo[tocdo]);
  digitalWrite (in1, HIGH);
}

void giamToc(int i)
{
  tocdo -= i;
  if (tocdo < 0) {
    tocdo = 0;
    is_nhay = true;
    suc_khi_stop();
  }
  analogWrite (ena, tocDo[tocdo]);
  Serial.print("tocDo[tocdo]"); Serial.println(tocDo[tocdo]);
  digitalWrite (in1, HIGH);
}
void DK_bom_khi() {
  bool checkStatus = false;
  int x = analogRead(bientroX);
  while (x > 520 || x < 500) {
    x = analogRead(bientroX);
    //giam
    if (analogRead(bientroX) > 520) {
      if (analogRead(bientroX) < 650) {
        giamToc(1);
      } else if (analogRead(bientroX) < 1200) {
        giamToc(2);
      }
      delay(1000);
      checkStatus = true;
    }
    else if (analogRead(bientroX) < 500) {
      if (analogRead(bientroX) < 100) {
        tangToc(2);
      } else {
        tangToc(1);
      }
      delay(1000);
      checkStatus = true;
    }
    checkStatus = true;
    DK_Led(tocdo);
  }
  if (checkStatus == true) {
    checkStatus = false;
    // Send current state to ESP32-CAM
    sendToESP32(tocdo, is_led, is_bom);
  }
}
void nhayVaTatLed(int i) {
  is_nhay = false;
  delay(500);
  for (int k = 0; k < 5; k++) {
    digitalWrite(latchPin, LOW);
    //Xuất bảng ký tự ra cho Module LED
    shiftOut(dataPin, clockPin, MSBFIRST, Seg[i]);
    digitalWrite(latchPin, HIGH);
    delay(200);
    // Tắt tất cả các đoạn của LED
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
    digitalWrite(latchPin, HIGH);
    delay(200);
  }
}
void DK_Led(int i) {
  if (i == 0 && is_nhay == true) {
    nhayVaTatLed(i);
  } else if (i > 0) {
    digitalWrite(latchPin, LOW);
    //Xuất bảng ký tự ra cho Module LED
    shiftOut(dataPin, clockPin, MSBFIRST, Seg[i]);
    digitalWrite(latchPin, HIGH);
  }
}

void loop() {
  DK_bom_khi();
  DK_Led(tocdo);
  //Serial.print("=");Serial.print(digitalRead(btn_bom));Serial.print("=");Serial.println(digitalRead(button_led));

  if (analogRead(bientroY) > 1000) {
    if (is_led == true) {
      is_led = false;
      digitalWrite(role_led, 0);
    } else {
      is_led = true;
      digitalWrite(role_led, 1);
    }
    delay(500);
    Serial.println(is_led);
    // Send current state to ESP32-CAM
    sendToESP32(tocdo, is_led, is_bom);
  }
  if (digitalRead(btn_bom) == HIGH) {
    if (is_bom == true) {
      is_bom = false;
      digitalWrite(in2_bom, LOW);
    } else {
      is_bom = true;
      digitalWrite(in2_bom, 1);
    }
    delay(500);
    Serial.println(is_bom);
    // Send current state to ESP32-CAM
    sendToESP32(tocdo, is_led, is_bom);
  }

  // Check for commands from ESP32-CAM
  receiveFromESP32();

}
