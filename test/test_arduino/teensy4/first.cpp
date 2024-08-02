// first.cpp

#include <Arduino.h>
#include <EEPROM.h>

byte EEPROM[EEPROM_SIZE];

void
init_EEPROM(void) {
    memset(EEPROM, 0xFF, EEPROM_SIZE);
}
