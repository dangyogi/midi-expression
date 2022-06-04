// test_LiquidCrystal_lib

#include <LiquidCrystal.h>

static const uint8_t RESET = 13;  // Built-in LED
static const uint8_t RS = 5;
static const uint8_t RW = 11;
static const uint8_t ENABLE = 12;
static const uint8_t DB0 = A3;
static const uint8_t RB0 = 6;
static const uint8_t DB1 = A2;
static const uint8_t RB1 = 7;
static const uint8_t DB2 = A1;
static const uint8_t RB2 = 8;
static const uint8_t DB3 = A0;
static const uint8_t RB3 = 9;
static const uint8_t DB4 = A6;
static const uint8_t RB4 = 3;
static const uint8_t DB5 = A7;
static const uint8_t RB5 = 2;
static const uint8_t DB6 = 4;
static const uint8_t RB6 = 0;   // RX
static const uint8_t DB7 = 1;   // TX, old 23 (AREF)
static const uint8_t RB7 = 10;

LiquidCrystal lcd(RS, RW, ENABLE, DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7);

void setup() {
  // put your setup code here, to run once:
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);

  // Doesn't work with or without these lines:
  delay(100);
  digitalWrite(RESET, LOW);
  delay(100);
  digitalWrite(RESET, HIGH);
  delay(100);
  
  lcd.begin(24, 1);
  lcd.print("Hello world!");

  Serial.begin(9600);
  Serial.setTimeout(5000);    // mSec
  Serial.println("setup done");
}

void loop() {
  // put your main code here, to run repeatedly:

}
