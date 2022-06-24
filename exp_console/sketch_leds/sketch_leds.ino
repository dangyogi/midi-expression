// sketch_leds.ino

#include <EEPROM.h>
#include <Wire.h>
#include "flash_errno.h"

#include "step.h"
#include "numeric_displays.h"
#include "alpha_displays.h"

#define NUM_EEPROM_USED     EEPROM_needed
#define EEPROM_SIZE         (EEPROM.length())
#define EEPROM_AVAIL        (EEPROM_SIZE - EEPROM_needed - 1)

#define EEPROM_storage_addr(addr) (EEPROM_needed + 1 + (addr))

#define ERR_LED    13   // built-in LED
#define ERR_LED2    7   // LED on panel

byte EEPROM_needed;

void setup() {
  // put your setup code here, to run once:
  err_led(ERR_LED, ERR_LED2);

  digitalWrite(ERR_LED, HIGH);
  digitalWrite(ERR_LED2, HIGH);

  Serial.begin(230400);

  EEPROM_needed = setup_step();
  EEPROM_needed += setup_numeric_displays(EEPROM_needed);
  EEPROM_needed += setup_alpha_displays(EEPROM_needed);

  Wire.begin(0x32);
  Wire.setClock(400000);
  
  if (EEPROM[NUM_EEPROM_USED] == 0xFF) {
    EEPROM[NUM_EEPROM_USED] = 0;
  }

  Wire.onReceive(receiveRequest);  // callback for requests from on high
  Wire.onRequest(sendReport);      // callback for reports to on high
  
  digitalWrite(ERR_LED, LOW);
  digitalWrite(ERR_LED2, LOW);
} // end setup()

#define STEP_RECEIVE_REPORT     0
#define STEP_SEND_REPORT        1
#define NUM_STEP_FUNS           2

// 0 = ??
// 1 = num_rows, num_a_pins, num_EEPROMS_avail, num_EEPROMS_used
// 2 = errno, err_data
// 3 = num_pots for each a_pin
// 4 = cycle_time
// 5 = ??
// 6 = EEPROM <addr> (one byte at a time, auto increments)
// 0xFF = report nothing (no data returned)
byte Report = 0xFF;
byte Report_addr = 0;
unsigned long Cycle_time;  // uSec

byte check_eq(byte b, byte n, byte errno) {
  if (b != n) {
    Errno = errno;
    Err_data = b;
    return 1;
  }
  return 0;
}

byte check_ls(byte b, byte limit, byte errno) {
  if (b >= limit) {
    Errno = errno;
    Err_data = b;
    return 1;
  }
  return 0;
}

int How_many;
byte ReceiveRequest_running;

void receiveRequest(int how_many) {
  // callback for requests from on high
  if (ReceiveRequest_running) {
    Errno = 70;
  } else {
    ReceiveRequest_running = 1;
    How_many = how_many;
    schedule_step_fun(STEP_RECEIVE_REPORT);
  }
}

void step_receiveRequest(void) {
  // request from on high
  byte b0, b1, b2, b3;
  unsigned short us0;
  short ss0;
  char s[MAX_STRING_LEN + 1];
  b0 = Wire.read();
  if (b0 < 6) {
    if (!check_eq(How_many, 1, 1)) Report = b0;
  } else if (b0 < 7) {
    // b0 == 6
    if (!check_eq(How_many, 2, 2)) {
      b1 = Wire.read();
      if (!check_ls(b1, EEPROM[NUM_EEPROM_USED], 4)) {
        Report = b0;
        Report_addr = b1;
      }
    }
  } else {
    switch (b0) {
    case 8:  // set num rows
      if (check_eq(How_many, 2, 8)) break;
      b1 = Wire.read();
      if (check_ls(b1, NUM_ROWS, 9)) break;
      Num_rows = b1;
      EEPROM[EEPROM_Num_rows] = b1;
      break;
    case 9:  // store EEPROM addr, value
      if (check_eq(How_many, 3, 20)) break;
      b1 = Wire.read();
      if (check_ls(b1, EEPROM_AVAIL, 21)) break;
      b2 = Wire.read();
      if (b1 > EEPROM[NUM_EEPROM_USED]) EEPROM[NUM_EEPROM_USED] = b1;
      EEPROM[EEPROM_storage_addr(b1)] = b2;
      break;
    case 10:  // set Num_numeric_displays
      if (check_eq(How_many, 2, 8)) break;
      b1 = Wire.read();
      if (check_ls(b1, MAX_NUMERIC_DISPLAYS, 9)) break;
      Num_numeric_displays = b1;
      EEPROM[EEPROM_Num_numeric_displays()] = b1;
      break;
    case 11:  // set <numeric_display>, size, offset
      if (check_eq(How_many, 4, 8)) break;
      b1 = Wire.read();
      if (check_ls(b1, Num_numeric_displays, 9)) break;
      b2 = Wire.read();
      if (check_ls(b2, MAX_NUMERIC_DISPLAY_SIZE + 1, 9)) break;
      b3 = Wire.read();
      if (check_ls(b3, Num_rows * NUM_COLS / 8 - 2*b2, 9)) break;
      Numeric_display_size[b1] = b2;
      EEPROM[EEPROM_Numeric_display_size(b1)] = b2;
      Numeric_display_offset[b1] = b3;
      EEPROM[EEPROM_Numeric_display_offset(b1)] = b3;
      break;
    case 12:  // set Num_alpha_strings
      if (check_eq(How_many, 2, 8)) break;
      b1 = Wire.read();
      if (check_ls(b1, MAX_NUM_STRINGS, 9)) break;
      Num_alpha_strings = b1;
      EEPROM[EEPROM_Num_alpha_strings()] = b1;
      break;
    case 13:  // set <string_num>, num_chars, index
      if (check_eq(How_many, 4, 8)) break;
      b1 = Wire.read();
      if (check_ls(b1, Num_alpha_strings, 9)) break;
      b2 = Wire.read();
      if (check_ls(b2, MAX_STRING_LEN, 9)) break;
      b3 = Wire.read();
      if (check_ls(b3, Num_rows * NUM_COLS / 16, 9)) break;
      Alpha_num_chars[b1] = b2;
      EEPROM[EEPROM_Alpha_num_chars(b1)] = b2;
      Alpha_index[b1] = b3;
      EEPROM[EEPROM_Alpha_index(b1)] = b3;
      break;
    case 14:  // turn on led
      if (check_eq(How_many, 2, 9)) break;
      led_on(Wire.read());
      break;
    case 15:  // turn off led
      if (check_eq(How_many, 2, 10)) break;
      led_off(Wire.read());
      break;
    case 16:  // set 8 bits
      if (check_eq(How_many, 3, 11)) break;
      b1 = Wire.read();
      load_8(Wire.read(), b1);
      break;
    case 17:  // set 16 bits
      if (check_eq(How_many, 4, 12)) break;
      b1 = Wire.read();
      us0 = Wire.read();  // MSB first
      us0 = (us0 << 8) | Wire.read();
      load_16(us0, b1);
      break;
    case 18:  // load digit
      if (check_eq(How_many, 5, 13)) break;
      b1 = Wire.read();
      b2 = Wire.read();
      b3 = Wire.read();
      load_digit(b1, b2, b3, Wire.read());
      break;
    case 19:  // load numeric
      if (check_eq(How_many, 5, 14)) break;
      b1 = Wire.read();
      ss0 = Wire.read();
      ss0 = (ss0 << 8) | Wire.read();
      load_numeric(b1, ss0, Wire.read());
      break;
    case 20:  // load note
      if (check_eq(How_many, 4, 15)) break;
      b1 = Wire.read();
      b2 = Wire.read();
      load_note(b1, b2, Wire.read());
      break;
    case 21:  // load sharp_flat
      if (check_eq(How_many, 3, 16)) break;
      b1 = Wire.read();
      load_sharp_flat(b1, Wire.read());
      break;
    case 22:  // load string
      if (How_many < 2) {
        Errno = 17;
        Err_data = How_many;
        break;
      }
      if (check_ls(How_many, MAX_STRING_LEN + 3, 18)) break;
      b1 = Wire.read();
      for (b2 = 0; b2 < How_many - 2; b2++) s[b2] = Wire.read();
      s[b2] = 0;
      load_string(b1, s);
      break;
    default:
      Errno = 110;
      Err_data = b0;
      break;
    } // end switch (b0)
  }
  ReceiveRequest_running = 0;
}

byte SendReport_running;

void sendReport(void) {
  // callback for reports requested from on high
  if (SendReport_running) {
    Errno = 80;
  } else {
    SendReport_running = 1;
    schedule_step_fun(STEP_SEND_REPORT);
  }
}

void step_sendReport(void) {
  byte i;
  switch (Report) {
  case 0:  // Errno, Err_data
    Wire.write(Errno);
    Wire.write(Err_data);
    Errno = 0;
    Err_data = 0;
    break;
  case 1:  // Num_rows, NUM_COLS, Num_numeric_displays, Num_alpha_displays, EEPROM_AVAIL,
           // EEPROM USED (6 bytes total)
    Wire.write(Num_rows);
    Wire.write(byte(NUM_COLS));
    Wire.write(Num_numeric_displays);
    Wire.write(Num_alpha_strings);
    Wire.write(byte(EEPROM_AVAIL));
    Wire.write(EEPROM[NUM_EEPROM_USED]);
    break;
  case 2:  // numeric_display_size (Num_numeric_displays bytes)
    for (i = 0; i < Num_numeric_displays; i++) {
      Wire.write(Numeric_display_size[i]);
    }
    break;
  case 3:  // numeric_display_offset (Num_numeric_displays bytes)
    for (i = 0; i < Num_numeric_displays; i++) {
      Wire.write(Numeric_display_offset[i]);
    }
    break;
  case 4:  // Cycle_time (uSec, 4 bytes total)
    Wire.write(Cycle_time);
    break;
  case 5:  // alpha_num_chars, alpha_index (2*Num_alpha_strings bytes)
    for (i = 0; i < Num_alpha_strings; i++) {
      Wire.write(Alpha_num_chars[i]);
      Wire.write(Alpha_index[i]);
    }
    break;
  case 6:  // next stored EEPROM byte (1 byte, EEPROM_addr auto-incremented)
    if (Report_addr < EEPROM[NUM_EEPROM_USED]) {
      Wire.write(EEPROM[EEPROM_storage_addr(Report_addr)]);
      Report_addr++;
    } else {
      Errno = 4;
      Err_data = Report_addr;
    }
    break;
  } // end switch (Report)
}

void help(void) {
  Serial.println();
  Serial.println("? - help");
  Serial.println("P - show Num_rows");
  Serial.println("Rn - set Num_rows to n");
  Serial.println("A - show Alpha_string settings");
  Serial.println("Sn - set Num_alpha_strings to n");
  Serial.println("C<str>,chars,index - set Alpha_num_chars, Alpha_index for <str>");
  Serial.println("D - show Num_numeric_displays");
  Serial.println("Nn - set Num_numeric_displays to n");
  Serial.println(
    "U<disp>,size,offset - set Numeric_display_size, Numeric_display_offset for <disp>");
  Serial.println("T - show Cycle_time");
  Serial.println("X<errno> - set Errno");
  Serial.println("E - show Errno, Err_data");
  Serial.println("L<bit> - led oN");
  Serial.println("F<bit> - led ofF");
  Serial.println("B<byte_addr>,<2hex> - set 8 bits");
  Serial.println("W<word_addr>,<4hex> - set 16 bits");
  Serial.println("G<disp_addr>,<digit>,<value>,<dp> - load digit");
  Serial.println("M<disp_addr>,<value>,<decimal_place> - load numeric");
  Serial.println("O<disp_addr>,<note>,<sharp_flat> - load note, sharp_flat 1=sharp, 2=flat");
  Serial.println("H<disp_addr>,<sharp_flat> - load shart_flat, 1=sharp, 2=flat");
  Serial.println("I<str#>,<string> - load string");
}

#define NUM_TIMEOUT_FUNS        3
#define ADVANCE_STRINGS         0
#define TEST_LED_ORDER          1
#define TEST_NUMERIC_DECODER    2

// 0 means "off"
unsigned long Timeout_fun_runtime[NUM_TIMEOUT_FUNS];

byte from_hex(byte ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  ch = toupper(ch);
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  return 0xFF;  // invalid hex digit
}

void timeout(void) {
  // This is the slow loop.  It is run roughly every 1.6 mSec.

  // Timeout funs are unsigned short foo(void), returning how many mSec to wait from the
  // start of this call, until the next call.  Returning 0xFFFF will disable the fun
  // (until somebody else sets its Timeout_fun_runtime).

  unsigned long now = millis();

  byte i;
  for (i = 0; i < NUM_TIMEOUT_FUNS; i++) {
    if (millis >= Timeout_fun_runtime[i] && Timeout_fun_runtime[i]) {
      unsigned short delay;  // mSec to next call, 0 means no next call (as of now)
      switch (i) {
      case ADVANCE_STRINGS: delay = advance_strings(); break;
      case TEST_LED_ORDER: delay = test_led_order(); break;
      } // end switch (i)
      if (delay == 0) {
        Timeout_fun_runtime[i] = 0;
      } else {
        Timeout_fun_runtime[i] = now + delay;
      }
      return;
    } // end if (Timeout_fun_scheduled[i])
  } // end for (i)

  if (Serial.available()) {
    char c = toupper(Serial.read());
    byte b0, b1, b2, b3;
    unsigned short us0;
    signed short ss0;
    char s[MAX_STRING_LEN + 1];
    switch (c) {
    case '?': help(); break;
    case 'P':
      Serial.print("Num_rows: ");
      Serial.println(Num_rows);
      break;
    case 'R':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= NUM_ROWS) {
        Serial.print("Invalid num_rows ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(NUM_ROWS);    
      } else {
        Num_rows = b0;
        EEPROM[EEPROM_Num_rows] = b0;
      }
      break;

    case 'A':
      Serial.print("Num_alpha_strings: ");
      Serial.println(Num_alpha_strings);
      break;
    case 'S':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= MAX_NUM_STRINGS) {
        Serial.print("Invalid Num_alpha_strings ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(MAX_NUM_STRINGS);    
      } else {
        Num_alpha_strings = b0;
        EEPROM[EEPROM_Num_alpha_strings()] = b0;
      }
      break;
    case 'C':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= NUM_ROWS) {
        Serial.print("Invalid num_rows ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(NUM_ROWS);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'C' command -- aborted");
      } else {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= MAX_STRING_LEN) {
          Serial.print("Invalid Alpha_num_chars ");  
          Serial.print(b1);
          Serial.print(" must be < ");
          Serial.println(MAX_STRING_LEN);    
        } else if (Serial.read() != ',') {
          Serial.println("Missing ',' in 'C' command -- aborted");
        } else {
          b2 = Serial.parseInt(SKIP_WHITESPACE);
          if (b2 >= Num_rows * NUM_COLS / 16) {
            Serial.print("Invalid Alpha_index ");  
            Serial.print(b2);
            Serial.print(" must be < ");
            Serial.println(Num_rows * NUM_COLS / 16);    
          } else {
            Alpha_num_chars[b0] = b1;
            EEPROM[EEPROM_Alpha_num_chars(b0)] = b1;
            Alpha_index[b0] = b2;
            EEPROM[EEPROM_Alpha_index(b0)] = b2;
          }
        }
      }
      break;

    case 'D':
      Serial.print("Num_numeric_displays: ");
      Serial.println(Num_numeric_displays);
      break;
    case 'N':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= MAX_NUMERIC_DISPLAYS) {
        Serial.print("Invalid Num_numeric_displays ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(MAX_NUMERIC_DISPLAYS);    
      } else {
        Num_numeric_displays = b0;
        EEPROM[EEPROM_Num_numeric_displays()] = b0;
      }
      break;
    case 'U':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_numeric_displays) {
        Serial.print("Invalid numeric_display ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_numeric_displays);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'U' command -- aborted");
      } else {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 > MAX_NUMERIC_DISPLAY_SIZE) {
          Serial.print("Invalid Numeric_display_size ");  
          Serial.print(b1);
          Serial.print(" must be <= ");
          Serial.println(MAX_NUMERIC_DISPLAY_SIZE);    
        } else if (Serial.read() != ',') {
          Serial.println("Missing ',' in 'U' command -- aborted");
        } else {
          b2 = Serial.parseInt(SKIP_WHITESPACE);
          if (b2 >= Num_rows * NUM_COLS / 8 - 2*b1) {
            Serial.print("Invalid Numeric_display_offset ");  
            Serial.print(b2);
            Serial.print(" must be < ");
            Serial.println(Num_rows * NUM_COLS / 8 - 2*b1);
          } else {
            Numeric_display_size[b0] = b1;
            EEPROM[EEPROM_Numeric_display_size(b0)] = b1;
            Numeric_display_offset[b0] = b2;
            EEPROM[EEPROM_Numeric_display_offset(b0)] = b2;
          }
        }
      }
      break;

    case 'T':
      Serial.print("Cycle_time is ");
      Serial.print(Cycle_time);
      Serial.println(" uSec");
      break;
    case 'X':
      Errno = Serial.parseInt(SKIP_WHITESPACE);
      Err_data = 0;
      Serial.print("Errno set to ");
      Serial.println(Errno);
      break;
    case 'E':
      Serial.print("Errno is ");
      Serial.print(Errno);
      Serial.print(", Err_data is ");
      Serial.println(Err_data);
      Errno = 0;
      Err_data = 0;
      break;
    case 'L': // led on
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_rows * NUM_COLS) {
        Serial.print("Invalid led number ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_rows * NUM_COLS);    
      } else {
        led_on(b0);
      }
      break;
    case 'F': // led off
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_rows * NUM_COLS) {
        Serial.print("Invalid led number ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_rows * NUM_COLS);    
      } else {
        led_off(b0);
      }
    case 'B':  // set 8 bits
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_rows * NUM_COLS / 8) {
        Serial.print("Invalid led byte number ");
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_rows * NUM_COLS / 8);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'B' command -- aborted");
      } else {
        b1 = from_hex(Serial.read());
        if (b1 == 0xFF) {
          Serial.print("Invalid hex digit ");  
          Serial.println(b1);
        } else {
          b2 = from_hex(Serial.read());
          if (b2 == 0xFF) {
            Serial.print("Invalid hex digit ");  
            Serial.println(b2);
          } else {
            b1 = (b1 << 4) | b2;
            load_8(b1, b0);
          }
        }
      }
      break;
    case 'W':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_rows * NUM_COLS / 16) {
        Serial.print("Invalid led byte number ");
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_rows * NUM_COLS / 16);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'W' command -- aborted");
      } else {
        b1 = from_hex(Serial.read());
        if (b1 == 0xFF) {
          Serial.print("Invalid hex digit ");  
          Serial.println(b1);
        } else {
          b2 = from_hex(Serial.read());
          if (b2 == 0xFF) {
            Serial.print("Invalid hex digit ");  
            Serial.println(b2);
          } else {
            b2 = from_hex(Serial.read());
            if (b2 == 0xFF) {
              Serial.print("Invalid hex digit ");  
              Serial.println(b2);
            } else {
              us0 = (us0 << 4) | b2;
              b2 = from_hex(Serial.read());
              if (b2 == 0xFF) {
                Serial.print("Invalid hex digit ");  
                Serial.println(b2);
              } else {
                us0 = (us0 << 4) | b2;
                load_16(us0, b0);
              }
            }
          }
        }
      }
      break;
    case 'G':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_numeric_displays) {
        Serial.print("Invalid numeric display number ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_numeric_displays);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'G' command -- aborted");
      } else {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= Numeric_display_size[b0]) {
          Serial.print("Invalid digit ");  
          Serial.print(b1);
          Serial.print(" must be < ");
          Serial.println(Numeric_display_size[b0]);    
        } else if (Serial.read() != ',') {
          Serial.println("Missing ',' in 'G' command -- aborted");
        } else {
          b2 = Serial.parseInt(SKIP_WHITESPACE);
          if (b2 >= 12) {
            Serial.print("Invalid value ");  
            Serial.print(b2);
            Serial.print(" must be < ");
            Serial.println(12);    
          } else if (Serial.read() != ',') {
            Serial.println("Missing ',' in 'G' command -- aborted");
          } else {
            b3 = Serial.parseInt(SKIP_WHITESPACE);
            if (b3 >= 2) {
              Serial.print("Invalid dp ");  
              Serial.print(b3);
              Serial.print(" must be < ");
              Serial.println(2);    
            } else {
              load_digit(b0, b1, b2, b3);
            }
          }
        }
      }
      break;
    case 'M':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_numeric_displays) {
        Serial.print("Invalid numeric display number ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_numeric_displays);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'M' command -- aborted");
      } else {
        ss0 = Serial.parseInt(SKIP_WHITESPACE);
        if (Serial.read() != ',') {
          Serial.println("Missing ',' in 'M' command -- aborted");
        } else {
          b1 = Serial.parseInt(SKIP_WHITESPACE);
          if (b1 >= Numeric_display_size[b0]) {
            Serial.print("Invalid digit ");  
            Serial.print(b1);
            Serial.print(" must be < ");
            Serial.println(Numeric_display_size[b0]);    
          } else {
            load_numeric(b0, ss0, b1);
          }
        }
      }
      break;

    case 'O':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_numeric_displays) {
        Serial.print("Invalid numeric display number ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_numeric_displays);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'O' command -- aborted");
      } else {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= 7) {
          Serial.print("Invalid note ");  
          Serial.print(b1);
          Serial.print(" must be < ");
          Serial.println(7);    
        } else if (Serial.read() != ',') {
          Serial.println("Missing ',' in 'O' command -- aborted");
        } else {
          b2 = Serial.parseInt(SKIP_WHITESPACE);
          if (b2 >= 3) {
            Serial.print("Invalid sharp_flat ");  
            Serial.print(b2);
            Serial.print(" must be < ");
            Serial.println(3);    
          } else {
            load_note(b0, b1, b2);
          }
        }
      }
      break;
    case 'H':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_numeric_displays) {
        Serial.print("Invalid numeric display number ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_numeric_displays);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'H' command -- aborted");
      } else {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= 3) {
          Serial.print("Invalid sharp_flat ");  
          Serial.print(b1);
          Serial.print(" must be < ");
          Serial.println(3);    
        } else {
          load_sharp_flat(b0, b1);
        }
      }
      break;
    case 'I':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_alpha_strings) {
        Serial.print("Invalid alpha string number ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_alpha_strings);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'I' command -- aborted");
      } else {
        b1 = Serial.readBytesUntil('\n', s, MAX_STRING_LEN);
        s[b1] = 0;
        load_string(b0, s);
      }
      break;
    case ' ': case '\t': case '\n': case '\r': break;
    default: help(); break;
    } // end switch (c)
  } // end if (Serial.available())

  errno();
}

byte Next_step_fun = 0xFF;
byte Step_fun_scheduled[NUM_STEP_FUNS];

void schedule_step_fun(byte step_fun) {
  if (step_fun >= NUM_STEP_FUNS) {
    Errno = 60;
    Err_data = step_fun;
  } else {
    Step_fun_scheduled[step_fun] = 1;
    if (step_fun < Next_step_fun) Next_step_fun = step_fun;
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  // Run next step_fun:
  if (Next_step_fun != 0xFF) {
    byte next_step_fun = Next_step_fun;
    Step_fun_scheduled[next_step_fun] = 0;
    Next_step_fun = 0xFF;
    switch (next_step_fun) {
    case 0: break;  // FIX
    } // end switch (next_step_fun)
    step(10*(next_step_fun+1));
    if (Next_step_fun > next_step_fun) {
      byte end = Next_step_fun == 0xFF ? NUM_STEP_FUNS : Next_step_fun;
      for (byte i = next_step_fun; i < end; i++) {
        if (Step_fun_scheduled[i]) {
          Next_step_fun = i;
          break;
        }
      } // end for (i)
      step(50);
    } // end if (Next_step_fun > next_step_fun)
  } else {  // Nothing else to run...
    step(51);
  } // end if (Next_step_fun != 0xFF)
}

// vim: sw=2
