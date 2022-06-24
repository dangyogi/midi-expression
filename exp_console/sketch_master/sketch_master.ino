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

#define NUM_PERIODICS           7

#define PULSE_NOTES_ON          0
#define PULSE_NOTES_OFF         1
#define UPDATE_LEDS             2
#define GET_POTS                3
#define SEND_MIDI               4
#define SWITCH_REPORT           5
#define ONE_TIME_REPORT         6

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
  EEPROM_used += setup_encoders(EEPROM_used);
  EEPROM_used += setup_functions(EEPROM_used);
  EEPROM_used += setup_events(EEPROM_used);
  EEPROM_used += setup_notes(EEPROM_used);

  Periodic_period[SWITCH_REPORT] = HUMAN_PERIOD;
  Periodic_period[ONE_TIME_REPORT] = 5000;
  
  digitalWrite(ERR_LED, LOW);
} // end setup()

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


void help(void) {
  Serial.println();
  Serial.println("? - help");
  Serial.println("T - show Cycle_time");
  Serial.println("X<errno> - set Errno");
  Serial.println("E - show Errno, Err_data");
  Serial.println();
  Serial.print("sizeof(unsigned long): ");
  Serial.println(sizeof(unsigned long));
  Serial.print("-1ul: ");
  Serial.println(-1ul, HEX);
}

void loop() {
  // put your main code here, to run repeatedly:
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
          if (Encoders[i].var != NULL && Encoders[i].var->changed) {
            Serial.print("Encoder ");
            Serial.print(i);
            Serial.print(" now ");
            Serial.println(Encoders[i].var->value);
          } // end if (encoder changed)
        } // end for (i)
        break;
      case ONE_TIME_REPORT:
        help();
        Serial.print("Longest_scan ");
        Serial.println(Longest_scan);
        Periodic_period[ONE_TIME_REPORT] = 0;  // make this a one-time thing...
        break;
      } // end switch (i)
    } // end if (period)
  } // for (i)

  errno();
}

// vim: sw=2
