// sketch_master.ino

// Arduino IDE Tools settings:
//   Board: Teensy LC
//   USB Type: Serial + MIDI*4

#include <EEPROM.h>
#include <Wire.h>
#include "flash_errno.h"
#include "switches.h"
#include "encoders.h"
#include "events.h"
#include "notes.h"
#include "functions.h"

#define PROGRAM_ID    "Master V10"

#define ERR_LED      13   // built-in LED
#define ERR_LED_2     1

// Indexed by I2C_addr - I2C_BASE
#define NUM_REMOTES   3
byte Remote_Errno[NUM_REMOTES];
byte Remote_Err_data[NUM_REMOTES];
byte Remote_char[NUM_REMOTES] = {'P', 'L'};

#define I2C_BASE              0x31
#define I2C_POT_CONTROLLER    0x31
#define I2C_LED_CONTROLLER    0x32
#define I2C_RAM_CONTROLLER    0x33

byte EEPROM_used;

void set_EEPROM(byte EEPROM_addr, byte value) {
  EEPROM[EEPROM_addr] = value;
}

byte get_EEPROM(byte EEPROM_addr) {
  return EEPROM[EEPROM_addr];
}

#define NUM_PERIODICS           6

// Periodic functions:
#define PULSE_NOTES_ON          0
#define PULSE_NOTES_OFF         1
#define UPDATE_LEDS             2
#define GET_POTS                3
#define SEND_MIDI               4
#define SWITCH_REPORT           5

unsigned short Periodic_period[NUM_PERIODICS];  // 0 to disable
unsigned short Period_offset[NUM_PERIODICS];

#define HUMAN_PERIOD     300

byte Driver_up;

void setup() {
  // put your setup code here, to run once:
  err_led(ERR_LED, ERR_LED_2);

  digitalWrite(ERR_LED, HIGH);
  digitalWrite(ERR_LED_2, HIGH);

  /***
  for (int i = 0; i < 100; i++) {
    delay(500);
    digitalWrite(ERR_LED, LOW);
    digitalWrite(ERR_LED_2, LOW);
    delay(500);
    digitalWrite(ERR_LED, HIGH);
    digitalWrite(ERR_LED_2, HIGH);
  }
  ***/
  
  Serial.begin(230400);
  Serial.println(PROGRAM_ID);

  Wire.begin();
  Wire.setClock(400000);

  byte EEPROM_used = setup_switches(0);
  EEPROM_used += setup_events(EEPROM_used);
  EEPROM_used += setup_encoders(EEPROM_used);
  EEPROM_used += setup_functions(EEPROM_used);
  EEPROM_used += setup_notes(EEPROM_used);

  Serial.print("EEPROM_used: "); Serial.println(EEPROM_used);

  Periodic_period[SWITCH_REPORT] = HUMAN_PERIOD;

  digitalWrite(ERR_LED, LOW);
  digitalWrite(ERR_LED_2, LOW);
} // end setup()

byte Debug = 0;

void running_help(void) {
  Serial.println();
  Serial.println(PROGRAM_ID);
  Serial.println();
  Serial.println(F("running:"));
  Serial.println(F("? - help"));
  Serial.println(F("I - initialize driver"));
  Serial.println(F("D - go into Debug mode"));
  Serial.println(F("L - show Longest_scan"));
  Serial.println(F("X<errno> - set Errno"));
  Serial.println(F("E - show Errno, Err_data"));
  Serial.println(F("S - show settings"));
  Serial.println(F("P<debounce_period_0>,<debounce_period_1> - set debounce_periods in EEPROM"));
  Serial.println(F("T - toggle Trace_events"));
  Serial.println(F("U - toggle Trace_encoders"));
  Serial.println(F("R<row> - dump switches on row"));
  Serial.println(F("C - dump encoders"));
  Serial.println(F("B - dump Debounce_delay_counts"));
  Serial.println(F("M<controller:(P|L|R)>,<len_expected>,<comma_sep_bytes> - send I2C message to <controller>"));
  Serial.println(F("G<first>,<last> - generate raw bytes in the range first-last (inclusive)"));
  Serial.println(F("V<first>,<last>\\n<raw_bytes> - verify that raw_bytes are in the range first-last"));
  Serial.println();
  Serial.print(F("sizeof(encoder_var_t) is ")); Serial.println(sizeof(encoder_var_t));
  Serial.println();
}

void debug_help(void) {
  Serial.println();
  Serial.println(F("debug:"));
  Serial.println(F("? - help"));
  Serial.println(F("D - leave Debug mode"));
  Serial.println(F("E - show Errno, Err_data"));
  Serial.println(F("S - scan for shorts"));
  Serial.println(F("On - turn on output n"));
  Serial.println(F("In - turn on input n"));
  Serial.println(F("F - turn off test pin"));
  Serial.println(F("P<data> - send I2C command to pot_controller"));
  Serial.println(F("Q<len_expected> - receive I2C report from pot_controller"));
  Serial.println(F("L<data> - send I2C command to led_controller"));
  Serial.println(F("M<len_expected> - receive I2C report from led_controller"));
  Serial.println();
}

byte Debug_pin_high = 0xFF;    // 0xFF means no pins high

void turn_off_test_pin(void) {
  if (Debug_pin_high != 0xFF) {
    pinMode(Debug_pin_high, INPUT_PULLDOWN);
    Debug_pin_high = 0xFF;
  }
}

unsigned long I2C_send_time;    // uSec

void sendRequest(byte i2c_addr, byte *data, byte data_len) {
  byte b0, status;
  unsigned long start_time = micros();
  Wire.beginTransmission(i2c_addr);
  b0 = Wire.write(data, data_len);
  if (b0 != data_len) {
    Errno = 20;
    Err_data = b0;
  }
  status = Wire.endTransmission();
  if (status) {
    Errno = 20 + status;        // 21 to 25
    Err_data = i2c_addr;
  }
  unsigned long elapsed_time = micros() - start_time;
  if (elapsed_time > I2C_send_time) I2C_send_time = elapsed_time;
}

unsigned long I2C_request_from_time;    // uSec
unsigned long I2C_read_time;            // uSec

byte ResponseData[32];

byte getResponse(byte i2c_addr, byte data_len, byte check_errno) {
  // Returns bytes received.  (0 if error)
  // Data in ResponseData.
  byte i;
  unsigned long start_time = micros();
  if (data_len > 32) {
    Errno = 10;
    Err_data = data_len;
    return 0;
  }
  byte bytes_received = Wire.requestFrom(i2c_addr, data_len);
  unsigned long mid_time = micros();
  unsigned long elapsed_time = mid_time - start_time;
  if (elapsed_time > I2C_request_from_time) I2C_request_from_time = elapsed_time;
  if (bytes_received > data_len) {
    if (Serial) {
      Serial.print("Bytes_received too long, got "); Serial.print(bytes_received);
      Serial.print("expected "); Serial.println(data_len);
    }
    Errno = 11;
    Err_data = bytes_received;
    return 0;
  }
  if (bytes_received != Wire.available()) {
    if (Serial) {
      Serial.print(F("I2C.requestFrom: bytes_received, ")); Serial.print(bytes_received);
      Serial.print(F(", != available(), ")); Serial.println(Wire.available());
    }
    Errno = 12;
    Err_data = Wire.available();
    return 0;
  }
  for (i = 0; i < bytes_received; i++) {
    ResponseData[i] = Wire.read();
  }
  unsigned long read_time = micros() - mid_time;
  if (read_time > I2C_read_time) I2C_read_time = read_time;
  if (bytes_received == 0) {
    Errno = 13;
    Err_data = i2c_addr;
    return bytes_received;
  }
  if (bytes_received >= 2) {
    if ((check_errno || (bytes_received < data_len && bytes_received == 2))
        && ResponseData[0] != 0
    ) {
      if (Driver_up) {
        report_remote_error(Remote_char[i2c_addr - I2C_BASE],
                            ResponseData[0], ResponseData[1]);
      } else {
        Remote_Errno[i2c_addr - I2C_BASE] = ResponseData[0];
        Remote_Err_data[i2c_addr - I2C_BASE] = ResponseData[1];
      }
    } // end error check
  } // end if received at least 2 bytes
  return bytes_received;
}

void skip_ws(void) {
  // skips whitespace on Serial.
  while (isspace(Serial.peek())) Serial.read();
}

void report_error() {
  Serial.print("$E");
  Serial.write('M');
  Serial.write(Errno);
  Serial.write(Err_data);
  Errno = Err_data = 0;
}

void report_remote_error(byte remote_index, byte errno, byte err_data) {
  Serial.print("$E");
  Serial.write(Remote_char[remote_index]);
  Serial.write(errno);
  Serial.write(err_data);
}

void loop() {
  // put your main code here, to run repeatedly:
  byte b0, b1, b2, b3, b4, i;
  unsigned short us0, us1;
  byte buffer[32];

  if (!Debug) {
    scan_switches();   // takes optional trace param...

    unsigned long now = millis();
    for (i = 0; i < NUM_PERIODICS; i++) {
      if (Periodic_period[i] && (now % Periodic_period[i]) == Period_offset[i]) {
        switch (i) {
        case PULSE_NOTES_ON:
          notes_on();
          break;
        case PULSE_NOTES_OFF:
          notes_off();
          break;
        case UPDATE_LEDS:
          break;
        case GET_POTS:
          break;
        case SEND_MIDI:
          break;
        case SWITCH_REPORT:
          if (Serial) {
            //Serial.println(F("still running..."));
            if (Trace_events) {
              for (i = 0; i < NUM_SWITCHES; i++) {
                if (Close_counts[i]) {
                  Serial.print(F("Switch "));
                  Serial.print(i);
                  Serial.print(F(" closed "));
                  Serial.print(Close_counts[i]);
                  Serial.println(F(" times"));
                  Close_counts[i] = 0;
                } // end if (Close_counts)
              } // end for (i)
            } // end if (Trace_events)
            if (Trace_encoders) {
              for (i = 0; i < NUM_ENCODERS; i++) {
                if (Encoders[i].var == NULL) {
                  Serial.print(F("Encoder "));
                  Serial.print(i);
                  Serial.println(F(": var is NULL"));
                } else if (Encoders[i].var->changed) {
                  Serial.print(F("Encoder "));
                  Serial.print(i);
                  Serial.print(F(" changed to "));
                  Serial.println(Encoders[i].var->value);
                  Encoders[i].var->changed = 0;
                } // end if (changed)
              } // end for (i)
            } // end if (Trace_events)
          } // end if (Serial)
          break;
        default:
          Errno = 16;
          Err_data = i;
          break;
        } // end switch (i)
      } // end if (period)
    } // for (i)
    if (Serial.available()) {
      b0 = toupper(Serial.read());
      switch (b0) {
      case '?': running_help(); break;
      case 'I':
        // initialize driver
        Serial.print("$I");   // init
        Serial.write(NUM_CHANNELS);
        Serial.write(NUM_CH_FUNCTIONS);
        Serial.write(NUM_FUNCTION_ENCODERS);
        Serial.write(NUM_HARMONICS);
        Serial.write(NUM_HM_FUNCTIONS);
        Driver_up = 1;
        if (Errno) {
          report_error();
          Errno = Err_data = 0;
        }
        for (i = 0; i < NUM_REMOTES; i++) {
          if (Remote_Errno[i]) {
            report_remote_error(i, Remote_Errno[i], Remote_Err_data[i]);
            Remote_Errno[i] = 0;
            Remote_Err_data[i] = 0;
          }
        }
        break;
      case 'D':
        Debug = 1;
        for (b1 = 0; b1 < NUM_ROWS; b1++) {
          pinMode(Rows[b1], INPUT_PULLDOWN);
        } // end for (b1)
        Serial.println(F("Entering Debug mode"));
        break;
      case 'L':
        Serial.print(F("Longest_scan: "));
        Serial.println(Longest_scan);
        Longest_scan = 0;
        break;
      case 'X': // set Errno
        skip_ws();
        Errno = Serial.parseInt();
        Serial.print(F("Errno set to "));
        Serial.println(Errno);
        break;
      case 'E': // show Errno, Err_data
        Serial.print(F("Errno: "));
        Serial.print(Errno);
        Serial.print(F(", Err_data: "));
        Serial.println(Err_data);
        Errno = 0;
        Err_data = 0;
        for (i = 0; i < NUM_REMOTES; i++) {
          Serial.print(F("Remote_Errno[")); Serial.print(i); Serial.print("]: ");
          Serial.print(Remote_Errno[i]);
          Serial.print(F(", Remote_Err_data: "));
          Serial.println(Remote_Err_data[i]);
          Remote_Errno[i] = 0;
          Remote_Err_data[i] = 0;
        }
        break;
      case 'S':  // show settings
        Serial.print(F("Switch debounce_period is ")); Serial.print(Debounce_period[0]);
        Serial.println(F(" uSec"));
        Serial.print(F("Encoder debounce_period is ")); Serial.print(Debounce_period[1]);
        Serial.println(F(" uSec"));
        break;
      case 'P':  // <debounce_period> - set debounce_period in EEPROM
        skip_ws();
        us0 = Serial.parseInt();
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("P: expected ',' after debounce_period[0], got ");
          Serial.println(b3);
          break;
        }
        us1 = Serial.parseInt();
        set_debounce_period(0, us0);
        Serial.print(F("Switch debounce_period set to ")); Serial.print(us0);
        Serial.println(F(" uSec in EEPROM"));
        set_debounce_period(1, us1);
        Serial.print(F("Encoder debounce_period set to ")); Serial.print(us1);
        Serial.println(F(" uSec in EEPROM"));
        break;
      case 'T': // toggle Trace_events
        Trace_events = 1 - Trace_events;
        Serial.print(F("Trace_events set to "));
        Serial.println(Trace_events);
        break;
      case 'U': // toggle Trace_encoders
        Trace_encoders = 1 - Trace_encoders;
        Serial.print(F("Trace_encoders set to "));
        Serial.println(Trace_encoders);
        break;
      case 'R':  // dump switches on row
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 >= NUM_ROWS) {
          Serial.print(F("Invalid row, must be < "));
          Serial.println(NUM_ROWS);
        } else {
          for (b2 = 0; b2 < NUM_COLS; b2++) {
            b3 = SWITCH_NUM(b1, b2);
            Serial.print(F("Switch ")); Serial.print(b3);
            Serial.print(F(", row ")); Serial.print(b1);
            Serial.print(F(", col ")); Serial.print(b2);
            Serial.print(F(": current ")); Serial.print(Switches[b3].current);
            Serial.print(F(", closed_event ")); Serial.print(Switch_closed_event[b3]);
            Serial.print(F(", opened_event ")); Serial.println(Switch_opened_event[b3]);
          }
        }
        break;
      case 'C':  // dump encoders
        for (b1 = 0; b1 < NUM_ENCODERS; b1++) {
          b2 = Encoders[b1].A_sw;
          Serial.print(F("Encoder ")); Serial.print(b1);
          Serial.print(F(": A_sw ")); Serial.print(b2);
          Serial.print(F(", A closed_event ")); Serial.print(Switch_closed_event[b2]);
          Serial.print(F(", A opened_event ")); Serial.print(Switch_opened_event[b2]);
          Serial.print(F(", B closed_event ")); Serial.print(Switch_closed_event[b2 + 1]);
          Serial.print(F(", B opened_event ")); Serial.println(Switch_opened_event[b2 + 1]);
          Serial.print(F("           Encoder_event ")); Serial.print(Encoder_event[b1]);
          if (Encoders[b1].var == NULL) {
            Serial.println(F(", var is NULL"));
          } else {
            Serial.print(F(", var not NULL"));
            if (Encoders[b1].var->flags & 0b1) Serial.print(F(", enabled"));
            else  Serial.print(F(", not enabled"));
            Serial.print(F(", value ")); Serial.print(Encoders[b1].var->value);
            if (Encoders[b1].var->changed) Serial.println(F(", changed"));
            else Serial.println(F(", not changed"));
          }
        } // end for (b1)
        break;
      case 'B':  // dump debounce_delay_counts
        for (b1 = 0; b1 < 2; b1 += 1) {
          if (b1) {
            Serial.println("Dumping Encoder debounce_delay_counts:");
          } else {
            Serial.println("Dumping Switch debounce_delay_counts:");
          }
          for (b2 = 0; b2 <= MAX_DEBOUNCE_COUNT; b2 += 1) {
            byte count = Debounce_delay_counts[b1][b2];
            if (count) {
              Serial.print("  ");
              if (b2 == MAX_DEBOUNCE_COUNT) {
                Serial.print("overflow: ");
              } else {
                Serial.print(b2); Serial.print(" mSec: ");
              }
              Serial.println(count);
              Debounce_delay_counts[b1][b2] = 0;
            }
          }
          Serial.println();
        }
        break;
      case 'M':  // send I2C message to controller
        // M<controller>,<len_expected>,<comma_seperated_bytes> - send I2C message to <controller>
        skip_ws();
        b1 = Serial.read();     // controller
        switch (b1) {
        case 'p':
        case 'P':
          b1 = I2C_POT_CONTROLLER;
          break;
        case 'l':
        case 'L':
          b1 = I2C_LED_CONTROLLER;
          break;
        case 'r':
        case 'R':
          b1 = I2C_RAM_CONTROLLER;
          break;
        default:
          Serial.print("M: unrecognized controller ");
          Serial.println(b1);
          goto error;
        }
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("M: expected ',' after controller, got ");
          Serial.println(b3);
          goto error;
        }
        skip_ws();
        b2 = Serial.parseInt(); // len_expected
        if (b2 > 32) {
          Serial.print("M: len_expected > 32, got ");
          Serial.println(b2);
          goto error;
        }
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("M: expected ',' after len_expected, got ");
          Serial.println(b3);
          goto error;
        }
        skip_ws();
        buffer[0] = Serial.parseInt();
        for (i = 1; i < 32; i++) {
          b3 = Serial.read();  // delimiter char
          if (b3 == '\n') {
            sendRequest(b1, buffer, i);
            I2C_send_time = 0;
            break;
          } else if (b3 != ',') {
            Serial.print("M: expected ',' after byte ");
            Serial.print(i);
            Serial.print(", got ");
            Serial.println(b3);
            goto error;
          }
          skip_ws();
          buffer[i] = Serial.parseInt();
        } // end for (i)
        if (b2 == 0) {
          Serial.println("Message sent");
        } else {
          skip_ws();
          b3 = getResponse(b1, b2, 0);
          I2C_request_from_time = 0;
          I2C_read_time = 0;
          if (b3 == 0) {
            Serial.println("no response");
          } else {
            Serial.print(ResponseData[0]);
            for (i = 1; i < b3; i++) {
              Serial.print(", "); Serial.print(ResponseData[i]);
            }
            Serial.println();
          }
        }
       error:
        break;
      case 'G': // G<first>,<last> - generate raw bytes in the range first-last (inclusive)
        b1 = Serial.parseInt(); // first
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("G: expected ',' after first, got ");
          Serial.println(b3);
          goto error;
        }
        skip_ws();
        b2 = Serial.parseInt(); // last
        b3 = Serial.read();     // \n
        if (b3 != '\n') {
          Serial.print("G: expected '\\n' after last, got ");
          Serial.println(b3);
          goto error;
        }
        for (i = b1; i <= b2; i++) {
          Serial.write(i);
          if (i == 255) break;  // so i doesn't get incremented back to 0
        }
        break;
      case 'V': // V<first>,<last>\n<raw_bytes> - verify that raw_bytes are in the range first-last
        b1 = Serial.parseInt(); // first
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("V: expected ',' after first, got ");
          Serial.println(b3);
          goto error;
        }
        skip_ws();
        b2 = Serial.parseInt(); // last
        b3 = Serial.read();     // comma delimiter
        if (b3 != '\n') {
          Serial.print("V: expected '\\n' after last, got ");
          Serial.println(b3);
          goto error;
        }
        b4 = 0;  // num errors
        for (i = b1; i <= b2; i++) {
          b3 = Serial.read();
          if (b3 != i) {
            if (b4 == 0) {
              Serial.print("V ERRORS: ");
            } else {
              Serial.print(", ");
            }
            Serial.print("expected ");
            Serial.print(i);
            Serial.print(" got ");
            Serial.print(b3);
            b4 += 1;
          }
          if (i == 255) break;  // so i doesn't get incremented back to 0
        }
        if (b4 == 0) {
          Serial.println("V: no errors");
        } else {
          Serial.println();
        }
        break;
      case ' ': case '\t': case '\n': case '\r': break;
      default: running_help(); break;
      } // end switch

      while (Serial.available() && Serial.read() != '\n') ;
    } // end if (Serial.available())
  } else { // Debug
    if (Serial.available()) {
      b0 = toupper(Serial.read());
      switch (b0) {
      case '?': debug_help(); break;
      case 'D':
        Debug = 0;
        turn_off_test_pin();     
        for (b1 = 0; b1 < NUM_ROWS; b1++) {
          pinMode(Rows[b1], OUTPUT);
          digitalWrite(Rows[b1], LOW);
        } // end for (b1)
        Serial.println(F("Leaving Debug mode"));
        break;
      case 'E': // show Errno, Err_data
        Serial.print(F("Errno: "));
        Serial.print(Errno);
        Serial.print(F(", Err_data: "));
        Serial.println(Err_data);
        Errno = 0;
        Err_data = 0;
        for (i = 0; i < NUM_REMOTES; i++) {
          Serial.print(F("Remote_Errno[")); Serial.print(i); Serial.print("]: ");
          Serial.print(Remote_Errno[i]);
          Serial.print(F(", Remote_Err_data: "));
          Serial.println(Remote_Err_data[i]);
          Remote_Errno[i] = 0;
          Remote_Err_data[i] = 0;
        }
        break;
      case 'S': // scan for shorts
        turn_off_test_pin();     

        // Test output pins high
        for (b1 = 0; b1 < NUM_ROWS; b1++) {
          pinMode(Rows[b1], OUTPUT);
          digitalWrite(Rows[b1], HIGH);

          // Test other Output pins
          for (b2 = 0; b2 < NUM_ROWS; b2++) {
            if (b2 != b1 && digitalRead(Rows[b2])) {
              Serial.print(F("Possible short between O"));
              Serial.print(b1);
              Serial.print(F(" and O"));
              Serial.println(b2);
            }
          } // end for (b2)

          // Test Input pins
          for (b2 = 0; b2 < NUM_COLS; b2++) {
            if (digitalRead(Cols[b2])) {
              Serial.print(F("Possible short between O"));
              Serial.print(b1);
              Serial.print(F(" and I"));
              Serial.println(b2);
            }
          } // end for (b2)
          pinMode(Rows[b1], INPUT_PULLDOWN);
        } // end for (b1)

        // Test input pins high
        for (b1 = 0; b1 < NUM_COLS; b1++) {
          pinMode(Cols[b1], OUTPUT);
          digitalWrite(Cols[b1], HIGH);

          // Test other Input pins
          for (b2 = 0; b2 < NUM_COLS; b2++) {
            if (b2 != b1 && digitalRead(Cols[b2])) {
              Serial.print(F("Possible short between I"));
              Serial.print(b1);
              Serial.print(F(" and I"));
              Serial.println(b2);
            }
          } // end for (b2)

          // Test Output pins
          for (b2 = 0; b2 < NUM_ROWS; b2++) {
            if (digitalRead(Rows[b2])) {
              Serial.print(F("Possible short between I"));
              Serial.print(b1);
              Serial.print(F(" and O"));
              Serial.println(b2);
            }
          } // end for (b2)
          pinMode(Cols[b1], INPUT_PULLDOWN);
        } // end for (b1)
        Serial.println(F("Scan complete"));
        break;
      case 'O': // turn on output n
        turn_off_test_pin();     
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 >= NUM_ROWS) {
          Serial.print(F("Invalid output pin, must be < "));
          Serial.println(NUM_ROWS);
        } else {
          pinMode(Rows[b1], OUTPUT);
          digitalWrite(Rows[b1], HIGH);
          Debug_pin_high = Rows[b1];
        }
        Serial.print(F("O"));
        Serial.print(b1);
        Serial.println(F(" on"));
        break;
      case 'I': // turn on input n
        turn_off_test_pin();     
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 >= NUM_COLS) {
          Serial.print(F("Invalid input pin, must be < "));
          Serial.println(NUM_COLS);
        } else {
          pinMode(Cols[b1], OUTPUT);
          digitalWrite(Cols[b1], HIGH);
          Debug_pin_high = Cols[b1];
        }
        Serial.print(F("I"));
        Serial.print(b1);
        Serial.println(F(" on"));
        break;
      case 'F': // turn off test pin
        turn_off_test_pin();
        Serial.println(F("test pin off"));
        break;
      case 'P': // send I2C command to pot_controller
        skip_ws();
        buffer[0] = Serial.parseInt();
        for (b1 = 1; b1 < 32; b1++) {
          b2 = Serial.read();
          if (b2 == '\n') {
            sendRequest(I2C_POT_CONTROLLER, buffer, b1);
            Serial.print(b1);
            Serial.println(" bytes sent to Pot Controller");
            for (b2 = 0; b2 < b1; b2++) {
              Serial.print(buffer[b2]);
              if (b2 + 1 < b1) {
                Serial.print(", ");
              }
            }
            Serial.println();
            Serial.println();
            Serial.print(F("I2C_send_time ")); Serial.print(I2C_send_time);
            Serial.println(F(" uSec"));
            I2C_send_time = 0;
            break;
          } else if (b2 != ',') {
            Serial.println(F("comma expected between bytes"));
            break;
          }
          skip_ws();
          buffer[b1] = Serial.parseInt();
        } // end for (b1)

        break;
      case 'Q': // receive I2C report from pot_controller
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 > 32) {
          Serial.println(F("ERROR: len_expected > 32"));
        } else {
          b2 = getResponse(I2C_POT_CONTROLLER, b1, 0);
          Serial.print(F("I2C_request_from_time ")); Serial.print(I2C_request_from_time);
          Serial.println(F(" uSec"));
          I2C_request_from_time = 0;
          Serial.print(F("I2C_read_time ")); Serial.print(I2C_read_time);
          Serial.println(F(" uSec"));
          I2C_read_time = 0;
          if (b2 == 0) {
            Serial.print("got Errno "); Serial.print(Errno);
            Serial.print(", Err_data "); Serial.println(Err_data);
          } else {
            Serial.print(ResponseData[0]);
            for (i = 1; i < b2; i++) {
              Serial.print(", "); Serial.print(ResponseData[i]);
            }
            Serial.println();
          }
        }
        break;
      case 'L': // send I2C command to led_controller
        skip_ws();
        buffer[0] = Serial.parseInt();
        for (b1 = 1; b1 < 32; b1++) {
          b2 = Serial.read();
          if (b2 == '\n') {
            sendRequest(I2C_LED_CONTROLLER, buffer, b1);
            Serial.print(b1);
            Serial.println(" bytes sent to LED Controller");
            for (b2 = 0; b2 < b1; b2++) {
              Serial.print(buffer[b2]);
              if (b2 + 1 < b1) {
                Serial.print(", ");
              }
            }
            Serial.println();
            Serial.println();
            Serial.print(F("I2C_send_time ")); Serial.print(I2C_send_time);
            Serial.println(F(" uSec"));
            I2C_send_time = 0;
            break;
          } else if (b2 != ',') {
            Serial.println(F("comma expected between bytes"));
            break;
          }
          skip_ws();
          buffer[b1] = Serial.parseInt();
        } // end for (b1)
        break;
      case 'M': // receive I2C report from led_controller
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 > 32) {
          Serial.println(F("ERROR: len_expected > 32"));
        } else {
          b2 = getResponse(I2C_LED_CONTROLLER, b1, 0);
          Serial.print(F("I2C_request_from_time ")); Serial.print(I2C_request_from_time);
          Serial.println(F(" uSec"));
          I2C_request_from_time = 0;
          Serial.print(F("I2C_read_time ")); Serial.print(I2C_read_time);
          Serial.println(F(" uSec"));
          I2C_read_time = 0;
          if (b2 == 0) {
            Serial.print("got Errno "); Serial.print(Errno);
            Serial.print(", Err_data "); Serial.println(Err_data);
          } else {
            Serial.print(ResponseData[0]);
            for (i = 1; i < b2; i++) {
              Serial.print(", "); Serial.print(ResponseData[i]);
            }
            Serial.println();
          }
        }
        break;
      case ' ': case '\t': case '\n': case '\r': break;
      default: debug_help(); break;
      } // end switch

      while (Serial.available() && Serial.read() != '\n') ;
    } // end if (Serial.available())
  } // end else if (!Debug)

  // MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) ;  // read & ignore incoming messages

  errno();
}

// vim: sw=2
