// sketch_leds.ino

// Arduino IDE Tools Settings:
//   Board: Nano Every
//   Registers Emulation: None

#include <EEPROM.h>
#include <Wire.h>
#include "flash_errno.h"

#include "step.h"
#include "numeric_displays.h"
#include "alpha_displays.h"

#define PROGRAM_ID          "LEDs V25"

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

  Serial.println(PROGRAM_ID);
  Serial.println();
  Serial.print("EEPROM_needed "); Serial.println(EEPROM_needed);
  Serial.print("NUM_EEPROM_USED "); Serial.println(EEPROM[NUM_EEPROM_USED]);
  Serial.print("EEPROM_AVAIL "); Serial.println(EEPROM_AVAIL);
} // end setup()

#define NUM_TIMEOUT_FUNS        3
#define ADVANCE_STRINGS         0
#define TEST_LED_ORDER          1
#define TEST_ALPHA_DECODER      2

// 0 means "off"
unsigned long Timeout_fun_runtime[NUM_TIMEOUT_FUNS];

#define STEP_RECEIVE_REQUEST    0
#define NUM_STEP_FUNS           1

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
unsigned long Timeout_cycle_time;  // uSec between timeout() calls
unsigned long Timeout_runtime;     // uSec consumed by timeout()
unsigned long Num_timeout_calls;
byte Trace;

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

void receiveRequest(int how_many) {  // Errnos 1-39
  // callback for requests from on high
  if (ReceiveRequest_running) {
    Errno = 35;
  } else {
    ReceiveRequest_running = 1;
    How_many = how_many;
    schedule_step_fun(STEP_RECEIVE_REQUEST);
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
    if (!check_eq(How_many, 1, 2)) Report = b0;
  } else if (b0 < 7) {
    // b0 == 6
    if (!check_eq(How_many, 2, 3)) {
      b1 = Wire.read();
      if (!check_ls(b1, EEPROM[NUM_EEPROM_USED], 4)) {
        Report = b0;
        Report_addr = b1;
      }
    }
  } else {
    Report = 0;
    switch (b0) {
    case 8:  // set num rows
      if (check_eq(How_many, 2, 5)) break;
      b1 = Wire.read();
      if (check_ls(b1, NUM_ROWS, 6)) break;
      Num_rows = b1;
      EEPROM[EEPROM_Num_rows] = b1;
      break;
    case 9:  // store EEPROM addr, value
      if (check_eq(How_many, 3, 7)) break;
      b1 = Wire.read();
      if (check_ls(b1, EEPROM_AVAIL, 8)) break;
      b2 = Wire.read();
      if (b1 > EEPROM[NUM_EEPROM_USED]) EEPROM[NUM_EEPROM_USED] = b1;
      EEPROM[EEPROM_storage_addr(b1)] = b2;
      break;
    case 10:  // set Num_numeric_displays
      if (check_eq(How_many, 2, 9)) break;
      b1 = Wire.read();
      if (check_ls(b1, MAX_NUMERIC_DISPLAYS, 10)) break;
      Num_numeric_displays = b1;
      EEPROM[EEPROM_Num_numeric_displays()] = b1;
      break;
    case 11:  // set <numeric_display>, size, offset
      if (check_eq(How_many, 4, 11)) break;
      b1 = Wire.read();
      if (check_ls(b1, Num_numeric_displays, 12)) break;
      b2 = Wire.read();
      if (check_ls(b2, MAX_NUMERIC_DISPLAY_SIZE + 1, 13)) break;
      b3 = Wire.read();
      if (check_ls(b3, Num_rows * NUM_COLS / 8 - 2*b2, 14)) break;
      Numeric_display_size[b1] = b2;
      EEPROM[EEPROM_Numeric_display_size(b1)] = b2;
      Numeric_display_offset[b1] = b3;
      EEPROM[EEPROM_Numeric_display_offset(b1)] = b3;
      break;
    case 12:  // set Num_alpha_strings
      if (check_eq(How_many, 2, 15)) break;
      b1 = Wire.read();
      if (check_ls(b1, MAX_NUM_STRINGS, 16)) break;
      Num_alpha_strings = b1;
      EEPROM[EEPROM_Num_alpha_strings()] = b1;
      break;
    case 13:  // set <string_num>, num_chars, index
      if (check_eq(How_many, 4, 17)) break;
      b1 = Wire.read();
      if (check_ls(b1, Num_alpha_strings, 18)) break;
      b2 = Wire.read();
      if (check_ls(b2, MAX_STRING_LEN + 1, 19)) break;
      b3 = Wire.read();
      if (check_ls(b3, Num_rows * NUM_COLS / 16, 20)) break;
      Alpha_num_chars[b1] = b2;
      EEPROM[EEPROM_Alpha_num_chars(b1)] = b2;
      Alpha_index[b1] = b3;
      EEPROM[EEPROM_Alpha_index(b1)] = b3;
      break;
    case 14:  // turn on led
      if (check_eq(How_many, 2, 21)) break;
      led_on(Wire.read());
      break;
    case 15:  // turn off led
      if (check_eq(How_many, 2, 22)) break;
      led_off(Wire.read());
      break;
    case 16:  // set 8 bits
      if (check_eq(How_many, 3, 23)) break;
      b1 = Wire.read();
      load_8(Wire.read(), b1);
      break;
    case 17:  // set 16 bits
      if (check_eq(How_many, 4, 24)) break;
      b1 = Wire.read();
      us0 = Wire.read();  // MSB first
      us0 = (us0 << 8) | Wire.read();
      load_16(us0, b1);
      break;
    case 18:  // load digit
      if (check_eq(How_many, 5, 25)) break;
      b1 = Wire.read();
      b2 = Wire.read();
      b3 = Wire.read();
      load_digit(b1, b2, b3, Wire.read());
      break;
    case 19:  // load numeric
      if (check_eq(How_many, 5, 26)) break;
      b1 = Wire.read();
      ss0 = Wire.read();
      ss0 = (ss0 << 8) | Wire.read();
      load_numeric(b1, ss0, Wire.read());
      break;
    case 20:  // load note
      if (check_eq(How_many, 4, 27)) break;
      b1 = Wire.read();
      b2 = Wire.read();
      load_note(b1, b2, Wire.read());
      break;
    case 21:  // load sharp_flat
      if (check_eq(How_many, 3, 28)) break;
      b1 = Wire.read();
      load_sharp_flat(b1, Wire.read());
      break;
    case 22:  // load string
      if (How_many < 2) {
        Errno = 29;
        Err_data = How_many;
        break;
      }
      if (check_ls(How_many, MAX_STRING_LEN + 3, 30)) break;
      b1 = Wire.read();
      for (b2 = 0; b2 < How_many - 2; b2++) s[b2] = Wire.read();
      s[b2] = 0;
      load_string(b1, s);
      break;
    case 23:  // test led order
      Timeout_fun_runtime[TEST_LED_ORDER] = 1;
      break;
    case 24:  // test numeric decoder
      test_numeric_decoder();
      break;
    case 25:  // test alpha decoder
      Timeout_fun_runtime[TEST_ALPHA_DECODER] = 1;
      break;
    case 26:  // set Errno, Err_data
      if (check_eq(How_many, 3, 31)) break;
      Errno = Wire.read();
      Err_data = Wire.read();
      break;
    default:
      Errno = 34;
      Err_data = b0;
      break;
    } // end switch (b0)
  }
  ReceiveRequest_running = 0;
}

void sendReport(void) {  // Errnos 41-59
  // callback for reports requested from on high
  byte i, len_written;
  switch (Report) {
  case 0:  // Errno, Err_data
    len_written = Wire.write(Errno);
    if (len_written != 1) {
      Errno = 43;
      Err_data = len_written;
      Report = 0;
      break;
    }
    len_written = Wire.write(Err_data);
    if (len_written != 1) {
      Errno = 44;
      Err_data = len_written;
      Report = 0;
      break;
    }
    Errno = 0;
    Err_data = 0;
    break;
  case 1:  // Num_rows, NUM_COLS, Num_numeric_displays, Num_alpha_displays, EEPROM_AVAIL,
           // EEPROM USED (6 bytes total)
    len_written = Wire.write(Num_rows);
    if (len_written != 1) {
      Errno = 45;
      Err_data = len_written;
      Report = 0;
      break;
    }
    len_written = Wire.write(byte(NUM_COLS));
    if (len_written != 1) {
      Errno = 46;
      Err_data = len_written;
      Report = 0;
      break;
    }
    len_written = Wire.write(Num_numeric_displays);
    if (len_written != 1) {
      Errno = 47;
      Err_data = len_written;
      Report = 0;
      break;
    }
    len_written = Wire.write(Num_alpha_strings);
    if (len_written != 1) {
      Errno = 48;
      Err_data = len_written;
      Report = 0;
      break;
    }
    len_written = Wire.write(byte(EEPROM_AVAIL));
    if (len_written != 1) {
      Errno = 49;
      Err_data = len_written;
      Report = 0;
      break;
    }
    len_written = Wire.write(EEPROM[NUM_EEPROM_USED]);
    if (len_written != 1) {
      Errno = 50;
      Err_data = len_written;
      Report = 0;
      break;
    }
    break;
  case 2:  // numeric_display_sizes (Num_numeric_displays bytes)
    for (i = 0; i < Num_numeric_displays; i++) {
      len_written = Wire.write(Numeric_display_size[i]);
      if (len_written != 1) {
        Errno = 51;
        Err_data = len_written;
        Report = 0;
        break;
      }
    }
    break;
  case 3:  // numeric_display_offsets (Num_numeric_displays bytes)
    for (i = 0; i < Num_numeric_displays; i++) {
      len_written = Wire.write(Numeric_display_offset[i]);
      if (len_written != 1) {
        Errno = 52;
        Err_data = len_written;
        Report = 0;
        break;
      }
    }
    break;
  case 4:  // Timeout_cycle_time (uSec, 4 bytes total)
    len_written = Wire.write(Timeout_cycle_time);
    if (len_written != 4) {
      Errno = 53;
      Err_data = len_written;
      Report = 0;
      break;
    }
    break;
  case 5:  // alpha_num_chars, alpha_index (2*Num_alpha_strings bytes)
    for (i = 0; i < Num_alpha_strings; i++) {
      len_written = Wire.write(Alpha_num_chars[i]);
      if (len_written != 1) {
        Errno = 54;
        Err_data = len_written;
        Report = 0;
        break;
      }
      len_written = Wire.write(Alpha_index[i]);
      if (len_written != 1) {
        Errno = 55;
        Err_data = len_written;
        Report = 0;
        break;
      }
    }
    break;
  case 6:  // next stored EEPROM byte (1 byte, EEPROM_addr auto-incremented)
    if (Report_addr < EEPROM[NUM_EEPROM_USED]) {
      len_written = Wire.write(EEPROM[EEPROM_storage_addr(Report_addr)]);
      if (len_written != 1) {
        Errno = 56;
        Err_data = len_written;
        Report = 0;
        break;
      }
      Report_addr++;
    } else {
      Errno = 42;
      Err_data = Report_addr;
    }
    break;
  default:
    Errno = 58;
    Err_data = Report;
    break;
  } // end switch (Report)
}

void help(void) {
  Serial.println();
  Serial.println(PROGRAM_ID);
  Serial.println();
  Serial.println("? - help");
  Serial.println("P - show Num_rows");
  Serial.println("Rn - set Num_rows to n");
  Serial.println("A - show Alpha_string settings");
  Serial.println("Sn - set Num_alpha_strings to n");
  Serial.println("C<str>,chars,index - set Alpha_num_chars, Alpha_index for <str>");
  Serial.println("D - show Numeric_display settings");
  Serial.println("Nn - set Num_numeric_displays to n");
  Serial.println(
    "U<disp>,size,offset - set Numeric_display_size, Numeric_display_offset for <disp>");
  Serial.println("T - show Timeout_cycle_time stats");
  Serial.println("Y - toggle trace mode");
  Serial.println("X<errno> - set Errno");
  Serial.println("E - show Errno, Err_data");
  Serial.println("V - Turn all LEDs off");
  Serial.println("L<bit> - led oN");
  Serial.println("F<bit> - led ofF");
  Serial.println("B<byte_addr>,<2hex> - set 8 bits");
  Serial.println("W<word_addr>,<4hex> - set 16 bits");
  Serial.println("G<disp_addr>,<digit_num>,<value>,<dp> - load digit, digit_num goes left to right");
  Serial.println("M<disp_addr>,<value>,<decimal_place> - load numeric");
  Serial.println("O<disp_addr>,<note>,<sharp_flat> - load note(0=A,6=G)/sharp_flat 0=nat, 1=#, 2=b");
  Serial.println("H<disp_addr>,<sharp_flat> - load sharp_flat, 0=nat, 1=sharp, 2=flat");
  Serial.println("I<str#>,<string> - load string");
  Serial.println("J - test led order");
  Serial.println("Q - test numeric decoder");
  Serial.println("Z - test alpha decoder");
  Serial.println("= - dump Col_ports (in hex)");
  Serial.println("K - Enter testing mode");
  Serial.println();
  //--- avail letters: <none>
}

void print_hex2(byte b) {
  // prints b as 2 digit hex number.
  if (b < 0x10) Serial.print(' ');
  Serial.print(b, HEX);
}

void dump_Col_ports(void) {
  byte i;
  Serial.println("Row   port_d  port_b  port_c  port_e");
  for (i = 0; i < Num_rows; i++) {
    Serial.print(' ');
    if (i < 10) Serial.print(' ');
    Serial.print(i); Serial.print(":    ");
    print_hex2(Col_ports[i].port_d); Serial.print("      ");
    print_hex2(Col_ports[i].port_b); Serial.print("      ");
    print_hex2(Col_ports[i].port_c); Serial.print("      ");
    print_hex2(Col_ports[i].port_e); Serial.println();
  }
  Serial.println();
}

byte from_hex(byte *ans, byte short_ok=0) {
  // Reads and returns a 2 hex digit byte into *ans.
  // Returns 1 on success, 0 on failure (after printing error message).
  *ans = 0;
  char ch;
  byte i;
  for (i = 0; i < 2; i++) {
    byte hex;
    if (Serial.readBytes(&ch, 1) != 1) {
      Serial.println("Premature end of line");
      return 0;  // error
    }
    if (short_ok && i && ch == '\n') {
      return 1;  // success!
    } 
    if (ch >= '0' && ch <= '9') hex = ch - '0';
    else {
      ch = toupper(ch);
      if (ch >= 'A' && ch <= 'F') hex = ch - 'A' + 10;
      else {
        Serial.print("Invalid hex digit ");
        Serial.println(ch);
        return 0;  // error
      }
    }
    *ans = (*ans << 4) | hex;
  }
  return 1;  // success!
}

byte read_comma(void) {
  // Returns 1 on success, 0 on failure.
  char ch;
  if (Serial.readBytes(&ch, 1) != 1) {
    Serial.println("Premature end of line");
    return 0;  // error
  }
  if (ch == ',') return 1;  // success!
  Serial.print("Expected ',' got '");
  Serial.print(ch); Serial.println("'");
  return 0;  // error
}

unsigned long Timeout_start_time;   // micros

byte Testing_mode;

void timeout(void) {
  // This is the slow loop.  It is run roughly every 1.7 mSec (with 13 rows), and takes 30 uSec to run (on avg).

  // Timeout funs are unsigned short foo(void), returning how many mSec to wait from the
  // start of this call, until the next call.  Returning 0xFFFF will disable the fun
  // (until somebody else sets its Timeout_fun_runtime).

  unsigned long now_micros = micros();
  Timeout_cycle_time += now_micros - Timeout_start_time;
  Timeout_start_time = now_micros;
  Num_timeout_calls += 1;
  unsigned long now = millis();

  byte i;
  for (i = 0; i < NUM_TIMEOUT_FUNS; i++) {
    if (now >= Timeout_fun_runtime[i] && Timeout_fun_runtime[i]) {
      unsigned short delay;  // mSec to next call, 0 means no next call (as of now)
      switch (i) {
      case ADVANCE_STRINGS: delay = advance_strings(); break;
      case TEST_LED_ORDER: delay = test_led_order(); break;
      case TEST_ALPHA_DECODER: delay = test_alpha_decoder(); break;
      } // end switch (i)
      if (delay == 0) {
        Timeout_fun_runtime[i] = 0;
      } else {
        Timeout_fun_runtime[i] = now + delay;
      }
      //Timeout_runtime += micros() - now_micros;
      //return;
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
        Serial.print("Num_rows set to ");
        Serial.println(b0);    
      }
      break;

    case 'A':
      Serial.print("Num_alpha_strings: ");
      Serial.println(Num_alpha_strings);
      for (b0 = 0; b0 < Num_alpha_strings; b0++) {
        Serial.print("Alpha display "); Serial.print(b0);
        Serial.print(": num_chars "); Serial.print(Alpha_num_chars[b0]);
        Serial.print(", word index "); Serial.println(Alpha_index[b0]);
      }
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
        Serial.print("Num_alpha_strings set to ");
        Serial.println(b0);    
      }
      break;
    case 'C':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_alpha_strings) {
        Serial.print("Invalid alpha string number ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_alpha_strings);    
      } else if (read_comma()) {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 > MAX_STRING_LEN) {
          Serial.print("Invalid Alpha_num_chars ");  
          Serial.print(b1);
          Serial.print(" must be <= ");
          Serial.println(MAX_STRING_LEN);    
        } else if (read_comma()) {
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
            Serial.print("For alpha_string "); Serial.println(b0);
            Serial.print("  Alpha_num_chars set to ");
            Serial.println(b1);
            Serial.print("  Alpha_index set to ");
            Serial.println(b2);
          }
        }
      }
      break;

    case 'D':
      Serial.print("Num_numeric_displays: ");
      Serial.println(Num_numeric_displays);
      for (b0 = 0; b0 < Num_numeric_displays; b0++) {
        Serial.print("Numeric display "); Serial.print(b0);
        Serial.print(": #digits "); Serial.print(Numeric_display_size[b0]);
        Serial.print(", byte offset "); Serial.println(Numeric_display_offset[b0]);
      }
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
        Serial.print("Num_numeric_displays set to ");
        Serial.println(b0);    
      }
      break;
    case 'U':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_numeric_displays) {
        Serial.print("Invalid numeric_display ");  
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_numeric_displays);    
      } else if (read_comma()) {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 > MAX_NUMERIC_DISPLAY_SIZE) {
          Serial.print("Invalid Numeric_display_size ");  
          Serial.print(b1);
          Serial.print(" must be <= ");
          Serial.println(MAX_NUMERIC_DISPLAY_SIZE);    
        } else if (read_comma()) {
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
            Serial.print("For numeric_display "); Serial.println(b0);
            Serial.print("  Numeric_display_size set to ");
            Serial.println(b1);
            Serial.print("  Numeric_display_offset set to ");
            Serial.println(b2);
          }
        }
      }
      break;
    case 'T':
      Serial.print("Num_timeout_calls "); Serial.print(Num_timeout_calls);
      Serial.print(", avg Timeout_cycle_time is "); Serial.print(Timeout_cycle_time / Num_timeout_calls); Serial.print(" uSec");
      Serial.print(", avg Timeout_runtime is "); Serial.print(Timeout_runtime / Num_timeout_calls); Serial.print(" uSec");
      Serial.print(", avg Step_runtime/row is "); Serial.print((Timeout_cycle_time - Timeout_runtime) / (Num_timeout_calls * Num_rows) - 100);
      Serial.println(" uSec");
      break;
    case 'Y':
      if (Trace) {
        Serial.println("Trace off");
        Trace = 0;
      } else {
        Serial.println("Trace on:");
        Trace = 1;
        for (b0 = 0; b0 < NUM_TIMEOUT_FUNS; b0++) {
          Serial.print("  Timeout_fun ");
          Serial.print(b0);
          Serial.print(": runtime ");
          Serial.println(Timeout_fun_runtime[b0]);
        }
      }
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
    case 'V': // all leds off
      for (b0 = 0; b0 < Num_rows * NUM_COLS; b0++) {
        led_off(b0);
      }
      Serial.println("All LEDs are off.");
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
        Serial.print("LED on: ");
        Serial.println(b0);    
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
        Serial.print("LED off: ");
        Serial.println(b0);    
      }
    case 'B':  // set 8 bits
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_rows * NUM_COLS / 8) {
        Serial.print("Invalid led byte number ");
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_rows * NUM_COLS / 8);    
      } else if (read_comma()) {
        if (from_hex(&b1, 1)) {
          load_8(b1, b0);
          Serial.print("8 bits at byte_num ");
          Serial.print(b0);    
          Serial.print(" set to ");
          Serial.println(b1, BIN);    
        }
      }
      break;
    case 'W':
      b0 = Serial.parseInt(SKIP_WHITESPACE);
      if (b0 >= Num_rows * NUM_COLS / 16) {
        Serial.print("Invalid led word number ");
        Serial.print(b0);
        Serial.print(" must be < ");
        Serial.println(Num_rows * NUM_COLS / 16);    
      } else if (read_comma()) {
        if (from_hex(&b1)) {    // must have at least 3 hex digits...
          if (from_hex(&b2)) {
            us0 = ((unsigned short)b1 << 8) | b2;
            load_16(us0, b0);
            Serial.print("16 bits on row ");
            Serial.print(b0);    
            Serial.print(" set to ");
            Serial.println(us0, BIN);
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
      } else if (read_comma()) {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= Numeric_display_size[b0]) {
          Serial.print("Invalid digit number ");  
          Serial.print(b1);
          Serial.print(" must be < ");
          Serial.println(Numeric_display_size[b0]);    
        } else if (read_comma()) {
          b2 = Serial.parseInt(SKIP_WHITESPACE);
          if (b2 >= 12) {
            Serial.print("Invalid value ");  
            Serial.print(b2);
            Serial.print(" must be < ");
            Serial.println(12);    
          } else if (read_comma()) {
            b3 = Serial.parseInt(SKIP_WHITESPACE);
            if (b3 >= 2) {
              Serial.print("Invalid dp ");  
              Serial.print(b3);
              Serial.print(" must be < ");
              Serial.println(2);    
            } else {
              load_digit(b0, b1, b2, b3);
              Serial.print("numeric_display ");
              Serial.print(b0);    
              Serial.print(", digit_num ");
              Serial.print(b1);
              Serial.print(" set to ");
              Serial.print(b2);
              if (b3) Serial.println(" without dp");
              else Serial.println(" with dp");
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
      } else if (read_comma()) {
        ss0 = Serial.parseInt(SKIP_WHITESPACE);
        if (read_comma()) {
          b1 = Serial.parseInt(SKIP_WHITESPACE);
          if (b1 >= Numeric_display_size[b0]) {
            Serial.print("Invalid digit ");  
            Serial.print(b1);
            Serial.print(" must be < ");
            Serial.println(Numeric_display_size[b0]);    
          } else {
            load_numeric(b0, ss0, b1);
            Serial.print("numeric_display ");
            Serial.print(b0);    
            Serial.print(" set to ");
            Serial.print(ss0);
            Serial.print(", with dp at ");
            Serial.println(b1);
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
      } else if (read_comma()) {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= 7) {
          Serial.print("Invalid note ");  
          Serial.print(b1);
          Serial.print(" must be < ");
          Serial.println(7);    
        } else if (read_comma()) {
          b2 = Serial.parseInt(SKIP_WHITESPACE);
          if (b2 >= 3) {
            Serial.print("Invalid sharp_flat ");  
            Serial.print(b2);
            Serial.print(" must be < ");
            Serial.println(3);    
          } else {
            load_note(b0, b1, b2);
            Serial.print("numeric_display ");
            Serial.print(b0);    
            Serial.print(" set to note ");
            Serial.print(b1);
            if (b2 == 1) Serial.println(" sharp");
            else if (b2 == 2) Serial.println(" flat");
            else Serial.println();
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
      } else if (read_comma()) {
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= 3) {
          Serial.print("Invalid sharp_flat ");  
          Serial.print(b1);
          Serial.print(" must be < ");
          Serial.println(3);    
        } else {
          load_sharp_flat(b0, b1);
          Serial.print("numeric_display ");
          Serial.print(b0);    
          Serial.print(" set");
          if (b2 == 1) Serial.println(" sharp");
          else if (b2 == 2) Serial.println(" flat");
          else Serial.println(" natural");
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
      } else if (read_comma()) {
        b1 = Serial.readBytesUntil('\n', s, MAX_STRING_LEN);
        s[b1] = 0;
        load_string(b0, s);
        Serial.print("alpha_string "); Serial.print(b0);
        Serial.print(", set to the "); Serial.print(strlen(s));    
        Serial.print(" chars: "); Serial.println(s);    
      }
      break;
    case 'J':  // test_led_order
      Timeout_fun_runtime[TEST_LED_ORDER] = 1;
      Serial.println("test_led_order started");
      break;
    case 'Q':  // test_numeric_decoder
      test_numeric_decoder();
      Serial.println("test_numeric_decoder done");
      break;
    case 'Z':  // test_alpha_decoder
      Timeout_fun_runtime[TEST_ALPHA_DECODER] = 1;
      Serial.println("test_alpha_decoder started");
      break;
    case '=':  // dump_Col_ports
      dump_Col_ports();
      break;
    case 'K':  // enter testing mode
      turn_off_all_columns();
      disable_all_rows();
      Testing_mode = 1;
      Serial.println("Entering testing mode!");
      break;
    case ' ': case '\t': case '\n': case '\r': break;
    default:
      Serial.print("Unrecognized command: '");
      Serial.print(char(b0));
      Serial.println("'");
      break;
    } // end switch (c)
  } // end if (Serial.available())

  while (Serial.available() && Serial.read() != '\n') ;

  errno();
  Timeout_runtime += micros() - now_micros;
}

byte Next_step_fun = 0xFF;
byte Step_fun_scheduled[NUM_STEP_FUNS];

void schedule_step_fun(byte step_fun) {
  if (step_fun >= NUM_STEP_FUNS) {
    Errno = 61;
    Err_data = step_fun;
  } else {
    Step_fun_scheduled[step_fun] = 1;
    if (step_fun < Next_step_fun) Next_step_fun = step_fun;
  }
}

void testing_help(void) {
  Serial.println();
  Serial.println(PROGRAM_ID);
  Serial.println("Testing mode");
  Serial.println();
  Serial.println("? - help");
  Serial.println("C - sequence through all columns 0-15");
  Serial.println("L<col> - turn on column");
  Serial.println("= - dump Col_ports (in hex)");
  Serial.println("T - (bi-T) sequence through led_on, then led_off twice (two different rows)");
  Serial.println("B<Mask> - (B-yte) load_8 twice (low/high) for two different rows");
  Serial.println("S<Mask> - (S-hort) scroll Mask right to left twice with load_16 (two rows),");
  Serial.println("          -Mask bits are flipped.");
  Serial.println("R - sequence through all rows 0-15");
  Serial.println("W<row> - turn on row");
  Serial.println("N - next row");
  Serial.println("D - disable all rows");
  Serial.println("E - enable all rows");
  Serial.println("K - terminate test mode");
}

void test_sequence(void fn(byte i)) {
  byte i;
  for (i = 0; i < 16; i++) {
    fn(i);
    delay(250);
  }
}

void test_turn_on_column(byte col) {
  turn_off_all_columns();
  turn_on_column(col);
  Serial.print("Column "); Serial.print(col); Serial.println(" on");
}

byte Test_row;
unsigned short Test_mask;
byte Negate;
byte Test_start_bit_num;

void test_led_on(byte col) {
  byte bit_num = col + Test_start_bit_num;
  led_on(bit_num);
  turn_off_all_columns();
  turn_on_led_columns(Test_row);
  Serial.print("len_on: bit "); Serial.println(bit_num);
}

void test_led_off(byte col) {
  byte bit_num = col + Test_start_bit_num;
  led_off(bit_num);
  turn_off_all_columns();
  turn_on_led_columns(Test_row);
  Serial.print("len_off: bit "); Serial.println(bit_num);
}

void test_load_16(byte col) {
  unsigned short bits = Test_mask << col;
  if (Negate) bits = ~bits;
  load_16(bits, Test_row);
  turn_off_all_columns();
  turn_on_led_columns(Test_row);
  Serial.print("Bits "); Serial.println(bits, BIN);
  Serial.print("Port D: "); Serial.println(Col_ports[Test_row].port_d, BIN);
  Serial.print("Port B: "); Serial.println(Col_ports[Test_row].port_b, BIN);
  Serial.print("Port C: "); Serial.println(Col_ports[Test_row].port_c, BIN);
  Serial.print("Port E: "); Serial.println(Col_ports[Test_row].port_e, BIN);
}

void test_row(byte i) {
  disable_all_rows();
  if (i) turn_on_next_row(NUM_ROWS);
  enable_all_rows();
  Serial.print("Row "); Serial.print(Current_row); Serial.println(" on");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Testing_mode) {
    if (Serial.available()) {
      char c = toupper(Serial.read());
      byte i, j, b0;
      long l0;
      switch (c) {
      case '?': testing_help(); break;
      case 'B':   // (B-yte) load_8 twice (low/high) for two different rows
        b0 = Serial.parseInt(SKIP_WHITESPACE);
        Serial.print("Bits "); Serial.println(b0, BIN);
        disable_all_rows();
        for (i = 0; i <= 1; i++) {
          Test_row = i * 5;
          Serial.print("Test_row "); Serial.println(Test_row);
          for (j = 0; j <= 1; j++) {
            clear_row(Test_row);
            load_8(b0, Test_row * 2 + j);
            turn_off_all_columns();
            turn_on_led_columns(Test_row);
            delay(250);
            turn_off_all_columns();
          }
          delay(500);
        }
        Serial.println("Test done"); Serial.println();
        break;
      case 'C':
        disable_all_rows();
        test_sequence(test_turn_on_column);
        turn_off_all_columns();
        Serial.println("Test done"); Serial.println();
        break;
      case 'D':
        disable_all_rows();
        Serial.println("All rows disabled");
        break;
      case 'E':
        enable_all_rows();
        Serial.println("All rows enabled");
        break;
      case 'K':  // enter normal mode
        turn_off_all_columns();
        disable_all_rows();
        Testing_mode = 0;
        On_start = On_end = 0;  // disable time expired check in step()
        Serial.println("Normal mode"); Serial.println();
        break;
      case 'N':
        disable_all_rows();
        turn_on_next_row(NUM_ROWS);
        enable_all_rows();
        Serial.print("Row "); Serial.print(Current_row); Serial.println(" on");
        break;
      case 'R':
        turn_off_all_columns();
        disable_all_rows();
        turn_on_first_row();    // still disabled, enabled by test_row
        test_sequence(test_row);
        disable_all_rows();
        Serial.println("Test done"); Serial.println();
        break;
      case 'S':
        l0 = Serial.parseInt(SKIP_WHITESPACE);
        if (l0 < 0) {
          Negate = 1;
          Test_mask = (unsigned short)-l0;
        } else {
          Negate = 0;
          Test_mask = (unsigned short)l0;
        }
        disable_all_rows();
        Test_row = 0;
        clear_row(Test_row);
        test_sequence(test_load_16);
        turn_off_all_columns();
        delay(500);
        Test_row = 4;
        clear_row(Test_row);
        test_sequence(test_load_16);
        turn_off_all_columns();
        Serial.println("Test done"); Serial.println();
        break;
      case 'T':   // sequence through led_on, then led_off
        disable_all_rows();
        for (i = 0; i <= 1; i++) {
          Test_row = i * 6;
          Test_start_bit_num = Test_row * NUM_COLS;
          clear_row(Test_row);
          test_sequence(test_led_on);
          test_sequence(test_led_off);
        }
        Serial.println("Test done"); Serial.println();
        break;
      case 'L':
        disable_all_rows();
        turn_off_all_columns();
        b0 = Serial.parseInt(SKIP_WHITESPACE);
        if (b0 > 15) {
          Serial.println("ERROR: column number > 15");
        } else {
          test_turn_on_column(b0);
        }
        break;
      case '=':  // dump_Col_ports
        dump_Col_ports();
        break;
      case 'W':
        b0 = Serial.parseInt(SKIP_WHITESPACE);
        if (b0 > 15) {
          Serial.println("ERROR: row number > 15");
        } else {
          turn_off_all_columns();
          disable_all_rows();
          turn_on_first_row();
          while (Current_row < b0) turn_on_next_row(NUM_ROWS);
          enable_all_rows();
          Serial.print("Row "); Serial.print(Current_row); Serial.println(" on");
        }
        break;
      case ' ': case '\t': case '\n': case '\r': break;
      default:
        Serial.print("Unknown testing command: '");
        Serial.print(c);
        Serial.println("'");
        break;
      } // end switch (c)
      while (Serial.available() && Serial.read() != '\n') ;
    }
  } else { // not Testing_mode
    // Run next step_fun:
    if (Next_step_fun != 0xFF) {
      byte next_step_fun = Next_step_fun;
      Step_fun_scheduled[next_step_fun] = 0;
      Next_step_fun = 0xFF;
      switch (next_step_fun) {
      case STEP_RECEIVE_REQUEST: step_receiveRequest(); break;
      } // end switch (next_step_fun)
      step(40*next_step_fun + 1);
      if (Next_step_fun > next_step_fun) {
        byte end = Next_step_fun == 0xFF ? NUM_STEP_FUNS : Next_step_fun;
        for (byte i = next_step_fun; i < end; i++) {
          if (Step_fun_scheduled[i]) {
            Next_step_fun = i;
            break;
          }
        } // end for (i)
        step(62);
      } // end if (Next_step_fun > next_step_fun)
    } else {  // Nothing else to run...
      step(63);
    } // end if (Next_step_fun != 0xFF)
  }
}

// vim: sw=2
