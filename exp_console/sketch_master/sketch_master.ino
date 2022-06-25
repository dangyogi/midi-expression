// sketch_master.ino

#include <MIDIUSB.h>
#include <Wire.h>
#include "flash_errno.h"
#include "switches.h"
#include "encoders.h"
#include "events.h"
#include "notes.h"
#include "functions.h"
#include "variable.h"

#define ERR_LED    13   // built-in LED

byte EEPROM_used;

unsigned long get_EEPROM(byte EEPROM_addr, byte len) {
  unsigned long ans;
  return 0xFFFFFFFFul;
}

#define NUM_PERIODICS           6

#define PULSE_NOTES_ON          0
#define PULSE_NOTES_OFF         1
#define UPDATE_LEDS             2
#define GET_POTS                3
#define SEND_MIDI               4
#define SWITCH_REPORT           5

unsigned short Periodic_period[NUM_PERIODICS];  // 0 to disable
unsigned short Period_offset[NUM_PERIODICS];

#define HUMAN_PERIOD     300

void setup() {
  // put your setup code here, to run once:
  err_led(ERR_LED);

  digitalWrite(ERR_LED, HIGH);

  Serial.begin(230400);

  Wire.begin();
  Wire.setClock(400000);

  byte EEPROM_used = setup_switches(0);
  EEPROM_used += setup_events(EEPROM_used);
  EEPROM_used += setup_encoders(EEPROM_used);
  EEPROM_used += setup_functions(EEPROM_used);
  EEPROM_used += setup_notes(EEPROM_used);

  Periodic_period[SWITCH_REPORT] = HUMAN_PERIOD;

  digitalWrite(ERR_LED, LOW);
} // end setup()

byte Debug = 0;

void running_help(void) {
  Serial.println("?: help");
  Serial.println("D: go into Debug mode");
  Serial.println("L - show Longest_scan");
  Serial.println("X<errno> - set Errno");
  Serial.println("E - show Errno, Err_data");
  Serial.println("T - toggle Trace_events");
  Serial.println("S<row> - dump switches on row");
  Serial.println("C - dump encoders");
}

void debug_help(void) {
  Serial.println("?: help");
  Serial.println("D: leave Debug mode");
  Serial.println("S: scan for shorts");
  Serial.println("On: turn on output n");
  Serial.println("In: turn on input n");
  Serial.println("F: turn off test pin");
}

byte Debug_pin_high = 0xFF;    // 0xFF means no pins high

void turn_off_test_pin(void) {
  if (Debug_pin_high != 0xFF) {
    pinMode(Debug_pin_high, INPUT_PULLDOWN);
    Debug_pin_high = 0xFF;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  byte b0, b1, b2, b3;

  if (!Debug) {
    scan_switches();

    byte i;
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
          //Serial.println("still running...");
          for (i = 0; i < NUM_SWITCHES; i++) {
            if (Close_counts[i]) {
              Serial.print("Switch row ");
              Serial.print(i / 9);
              Serial.print(" col ");
              Serial.print(i % 9);
              Serial.print(" closed ");
              Serial.print(Close_counts[i]);
              Serial.println(" times");
              Close_counts[i] = 0;
            } // end if (Close_counts)
          } // end for (i)
          for (i = 0; i < NUM_ENCODERS; i++) {
            if (Encoders[i].var == NULL) {
              Serial.print("Encoder ");
              Serial.print(i);
              Serial.println(": var is NULL");
            } else if (Encoders[i].var->changed) {
              Serial.print("Encoder ");
              Serial.print(i);
              Serial.print(" changed to ");
              Serial.println(Encoders[i].var->value);
              Encoders[i].var->changed = 0;
            } // end if (changed)
          } // end for (i)
          break;
        } // end switch (i)
      } // end if (period)
    } // for (i)
    if (Serial.available()) {
      b0 = toupper(Serial.read());
      switch (b0) {
      case '?': running_help(); break;
      case 'D':
        Debug = 1;
        for (b1 = 0; b1 < NUM_ROWS; b1++) {
          pinMode(Rows[b1], INPUT_PULLDOWN);
        } // end for (b1)
        Serial.println("Entering Debug mode");
        break;
      case 'L':
        Serial.print("Longest_scan: ");
        Serial.println(Longest_scan);
        Longest_scan = 0;
        break;
      case 'X': // set Errno
        Errno = Serial.parseInt(SKIP_WHITESPACE);
        Serial.print("Errno set to ");
        Serial.println(Errno);
        break;
      case 'E': // show Errno, Err_data
        Serial.print("Errno: ");
        Serial.print(Errno);
        Serial.print(", Err_data: ");
        Serial.println(Err_data);
        Errno = 0;
        Err_data = 0;
        break;
      case 'T': // toggle Trace_events
        Trace_events = 1 - Trace_events;
        Serial.print("Trace_events set to ");
        Serial.println(Trace_events);
        break;
      case 'S':  // dump switches on row
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= NUM_ROWS) {
          Serial.print("Invalid row, must be < ");
          Serial.println(NUM_ROWS);
        } else {
          for (b2 = 0; b2 < NUM_COLS; b2++) {
            b3 = SWITCH_NUM(b1, b2);
            Serial.print("Switch "); Serial.print(b3);
            Serial.print(", row "); Serial.print(b1);
            Serial.print(", col "); Serial.print(b2);
            Serial.print(": current "); Serial.print(Switches[b3].current);
            Serial.print(", closed_event "); Serial.print(Switch_closed_event[b3]);
            Serial.print(", opened_event "); Serial.println(Switch_opened_event[b3]);
          }
        }
        break;
      case 'C':  //dump encoders
        for (b1 = 0; b1 < NUM_ENCODERS; b1++) {
          b2 = Encoders[b1].A_sw;
          Serial.print("Encoder "); Serial.print(b1);
          Serial.print(": A_sw "); Serial.print(b2);
          Serial.print(", A closed_event "); Serial.print(Switch_closed_event[b2]);
          Serial.print(", A opened_event "); Serial.print(Switch_opened_event[b2]);
          Serial.print(", B closed_event "); Serial.print(Switch_closed_event[b2 + 1]);
          Serial.print(", B opened_event "); Serial.println(Switch_opened_event[b2 + 1]);
          Serial.print("           Encoder_event "); Serial.print(Encoder_event[b1]);
          if (Encoders[b1].var == NULL) {
            Serial.println(", var is NULL");
          } else {
            Serial.print(", var not NULL");
            if (Encoders[b1].var->flags & 0b1) Serial.print(", enabled");
            else  Serial.print(", not enabled");
            Serial.print(", value "); Serial.print(Encoders[b1].var->value);
            if (Encoders[b1].var->changed) Serial.print(", changed, var_num ");
            else Serial.print(", not changed, var_num ");
            Serial.println(Encoders[b1].var->var_num);
          }
        } // end for (b1)
        break;
      case ' ': case '\t': case '\n': case '\r': break;
      default: running_help(); break;
      } // end switch
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
        Serial.println("Leaving Debug mode");
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
              Serial.print("Possible short between O");
              Serial.print(b1);
              Serial.print(" and O");
              Serial.println(b2);
            }
          } // end for (b2)

          // Test Input pins
          for (b2 = 0; b2 < NUM_COLS; b2++) {
            if (digitalRead(Cols[b2])) {
              Serial.print("Possible short between O");
              Serial.print(b1);
              Serial.print(" and I");
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
              Serial.print("Possible short between I");
              Serial.print(b1);
              Serial.print(" and I");
              Serial.println(b2);
            }
          } // end for (b2)

          // Test Output pins
          for (b2 = 0; b2 < NUM_ROWS; b2++) {
            if (digitalRead(Rows[b2])) {
              Serial.print("Possible short between I");
              Serial.print(b1);
              Serial.print(" and O");
              Serial.println(b2);
            }
          } // end for (b2)
          pinMode(Cols[b1], INPUT_PULLDOWN);
        } // end for (b1)
        Serial.println("Scan complete");
        break;
      case 'O': // turn on output n
        turn_off_test_pin();     
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= NUM_ROWS) {
          Serial.print("Invalid output pin, must be < ");
          Serial.println(NUM_ROWS);
        } else {
          pinMode(Rows[b1], OUTPUT);
          digitalWrite(Rows[b1], HIGH);
          Debug_pin_high = Rows[b1];
        }
        Serial.print("O");
        Serial.print(b1);
        Serial.println(" on");
        break;
      case 'I': // turn on input n
        turn_off_test_pin();     
        b1 = Serial.parseInt(SKIP_WHITESPACE);
        if (b1 >= NUM_COLS) {
          Serial.print("Invalid input pin, must be < ");
          Serial.println(NUM_COLS);
        } else {
          pinMode(Cols[b1], OUTPUT);
          digitalWrite(Cols[b1], HIGH);
          Debug_pin_high = Cols[b1];
        }
        Serial.print("I");
        Serial.print(b1);
        Serial.println(" on");
        break;
      case 'F': // turn off test pin
        turn_off_test_pin();
        Serial.println("test pin off");
        break;
      case ' ': case '\t': case '\n': case '\r': break;
      default: debug_help(); break;
      } // end switch
    } // end if (Serial.available())
  }
  errno();
}

// vim: sw=2
