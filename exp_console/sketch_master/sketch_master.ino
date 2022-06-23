// sketch_master.ino

#include <Wire.h>
#include "flash_errno.h"
#include "switches.h"
#include "events.h"

#define ERR_LED    13   // built-in LED

byte EEPROM_used;

unsigned long get_EEPROM(byte EEPROM_addr, byte len) {
  unsigned long ans;
  return 0xFFFFFFFFul;
}

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

unsigned long Last_human_time;

#define HUMAN_PERIOD     300

byte Showed_stats;

void loop() {
  // put your main code here, to run repeatedly:
  scan_switches();

  if (millis() >= Last_human_time + HUMAN_PERIOD) {
    //Serial.println("still running...");
    byte i;
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
      }
    }
    Last_human_time = millis();
  }
  
  if (!Showed_stats && millis() >= 10000) {
    help();
    Serial.print("Longest_scan ");
    Serial.println(Longest_scan);
    Showed_stats = 1;
  }

  errno();
}

// vim: sw=2
