/*
            JoyStick                Arduino
              VCC                     5V
              GND                    GND
              VRx                     A0
              VRy                     A1
              SW                      2

   Nạp code mở Serial Monitor chọn No line ending, baud 9600.

*/

const int bientroX = A0 ;
const int bientroY = A1 ;
const int button = 2;
const int ena = 3;
const int in1 = 4;
const int in2 = 5;
int tocdo=4;
int xf;
int yf;

//chân ST_CP của 74HC595
int latchPin = 8;
//chân SH_CP của 74HC595
int clockPin = 12;
//Chân DS của 74HC595
int dataPin = 11;
const int capBac[10] ={0,1,2,3,4,5,6,7,8,9};
const int tocDo[10] ={0,167,178,189,200,211,222,233,244,255};
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

void setup ()
{
  pinMode(2, INPUT);
  pinMode(bientroX, INPUT);
  pinMode(bientroY, INPUT);
  pinMode (ena, OUTPUT);
  pinMode (in1, OUTPUT);
  pinMode (in2, OUTPUT);
  xf = analogRead(bientroX);

  yf = analogRead(bientroY);

  Serial.begin(9600);
    //Bạn BUỘC PHẢI pinMode các chân này là OUTPUT
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}
void motor1_dung()
{
  digitalWrite (in1, LOW);
  digitalWrite (in2, LOW);
}
void tangtoc (int i)
{
  tocdo += i;
  if (tocdo > 9) {
    tocdo = 9;
  }
  analogWrite (ena, tocDo[tocdo]);
  Serial.print("tocDo[tocdo]"); Serial.println(tocDo[tocdo]);
  digitalWrite (in1, HIGH);
  digitalWrite (in2, LOW);
  //   delay (50);
}

void giamtoc(int i)
{
  tocdo -= i;
  if (tocdo < 0) {
    tocdo = 0;
   motor1_dung();
  }
  analogWrite (ena, tocDo[tocdo]);
  Serial.print("tocDo[tocdo]"); Serial.println(tocDo[tocdo]);
  digitalWrite (in1, HIGH);
  digitalWrite (in2, LOW);
  //   delay (0);

}
void controller(int x) {
  //giam
  if (xf > 520) {
    if (xf < 650) {
      giamtoc(1);
    } else if (xf < 1200) {
      giamtoc(2);
    }
  }
  else if (xf < 500) {
    if (xf < 100) {
      tangtoc(2);
    } else {
      tangtoc(1);
    }
  }
  Serial.println("-----------------");
  Serial.print (tocdo);
  Serial.print (" = ");
  Serial.println (tocDo[tocdo]);
  controllerLed(tocdo);
  delay(1000);
}
void controllerLed(int i){
  digitalWrite(latchPin, LOW);
  //Xuất bảng ký tự ra cho Module LED
  shiftOut(dataPin, clockPin, MSBFIRST, Seg[i]);  
  
  digitalWrite(latchPin, HIGH);
}
  void loop ()
  {
    xf = analogRead(bientroX);  // doc gia tri cua truc X
    yf = analogRead(bientroY);  // doc gia tri cua truc Y
//    Serial.print("X="); Serial.println(xf);
    controller(xf);
    int KEY = digitalRead(button);  // doc gia tri cua nut nhan

//    Serial.print("X="); Serial.print(xf); Serial.print(" "); Serial.println(xl);
    //  Serial.print("Y=");Serial.print(yf); Serial.println(yl);
      Serial.print("KEY="); Serial.print(KEY); Serial.println();

  
  //  delay(200);//delay để ổn định hơn

}
//int ena = 2;
//int in1 = 3;
//int in2 = 4;
//int in3 = 5;
//int in4 = 6;
//int enb = 8;
//
//int i;
//
//void setup()
//{
//  Serial. begin (9600);
//
//  pinMode (ena, OUTPUT);
//  pinMode (in1, OUTPUT);
//  pinMode (in2, OUTPUT);
//  pinMode (in3, OUTPUT);
//  pinMode (in4, OUTPUT);
//  pinMode (enb, OUTPUT);
//
//}
//
//void motor1_dung()
//{
//  digitalWrite (in1, LOW);
//  digitalWrite (in2, LOW);
//}
//void motor2_dung ()
//{
//  digitalWrite (in3, LOW);
//  digitalWrite (in4, LOW);
//}
//
//void tangtoc ()
//{
//
//  for (i=150; i<256; i+=1)
//  {
//     analogWrite (ena, i);
//     digitalWrite (in1, HIGH);
//     digitalWrite (in2, LOW);
////     analogWrite (enb, i);
//     digitalWrite (in3, LOW);
//     digitalWrite (in4, HIGH);
//     Serial.  println (i);
//     delay (50);
//  }
//}
//
//void giamtoc ()
//{
//
//  for (i=255; i>150; i-=1)
//  {
//     analogWrite (ena, i);
//     digitalWrite (in1, HIGH);
//     digitalWrite (in2, LOW);
////     analogWrite (enb, i);
//     digitalWrite (in3, LOW);
//     digitalWrite (in4, HIGH);
//     Serial.  println (i);
//     delay (50);
//  }
//}
//
//void loop()
//{
//  tangtoc ();
//  Serial. println ("Chạy 5 giây");
//  delay (5000);
//
//  giamtoc ();
//  digitalWrite (in3, LOW);
//  digitalWrite (in4, LOW);
//  digitalWrite (in2, LOW);
//  digitalWrite (in1, LOW);
//  Serial. println ("Dừng 5 giây");
//  delay (5000);
//
//}
