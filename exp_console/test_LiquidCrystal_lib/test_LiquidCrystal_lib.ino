// test_LiquidCrystal_lib

#include <LiquidCrystal.h>

static const uint8_t RESET = 13;  // Built-in LED
static const uint8_t RS = 5;
static const uint8_t RW = 11;
static const uint8_t ENABLE = 12;
static const uint8_t DB0 = 2;
//static const uint8_t RB0 = 6;
static const uint8_t DB1 = 3;
//static const uint8_t RB1 = 7;
static const uint8_t DB2 = 6;
//static const uint8_t RB2 = 8;
static const uint8_t DB3 = 7;
//static const uint8_t RB3 = 9;
static const uint8_t DB4 = 8;
//static const uint8_t RB4 = 3;
static const uint8_t DB5 = 9;
//static const uint8_t RB5 = 2;
static const uint8_t DB6 = 4;
//static const uint8_t RB6 = 0;   // RX
static const uint8_t DB7 = 10;
//static const uint8_t RB7 = 10;

//LiquidCrystal lcd(RS, RW, ENABLE, DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7);
//LiquidCrystal lcd(RS, ENABLE, DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7);
LiquidCrystal lcd(RS, ENABLE, DB4, DB5, DB6, DB7);  // Tie RW to GND

void setup() {
  // put your setup code here, to run once:

  lcd.begin(16, 2);
  //lcd.noAutoScroll();
  //lcd.noCursor();
  //lcd.setCursor(col, row);   sets location where subsequent text written to the LCD will be displayed
  lcd.print("Hello World!");

  Serial.begin(9600);
  Serial.setTimeout(500);    // mSec
  Serial.println("setup done");
  help();
}

void help(void) {
  Serial.println();
  Serial.println("H - help");
  Serial.println("C - clear");
  Serial.println("P<col>,<row> - position to (setCursor)");
  Serial.println("W<text> - write");
  Serial.println();
}

byte swallow(byte c) {
  // Returns 1 on success.
  byte b = Serial.read();
  if (b == c) return 1;
  Serial.print("Expected '");
  Serial.print(c);
  Serial.print("', got '");
  Serial.print(b);
  Serial.println("'");
  return 0;
}

void position(void) {
  byte col = Serial.parseInt(SKIP_WHITESPACE);
  if (!swallow(',')) return;
  byte row = Serial.parseInt(SKIP_WHITESPACE);
  lcd.setCursor(col, row);
  Serial.print("Did setCursor to ");
  Serial.print(col);
  Serial.print(", ");
  Serial.println(row);
}

void write(void) {
  String text = Serial.readStringUntil('\n');
  Serial.print("Writing ");
  Serial.print(text.length());
  Serial.print(" chars: ");
  Serial.println(text.c_str());
  lcd.print(text.c_str());
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    byte c = Serial.read();
    switch (toupper(c)) {
    case 'H': help(); break;
    case 'C': lcd.clear(); Serial.println("LCD cleared"); break;
    case 'P': position(); break;
    case 'W': write(); break;
    case ' ': case '\t': case '\r': case '\n': break;
    default: help(); break;
    } // end switch
  } // end if (Serial.available())
}
