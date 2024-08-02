// init_sketch.cpp

#define DELAY_0         20000
#define DELAY_1          1000

void
init_sketch(void) {
    EEPROM[0] = DELAY_0 >> 8;
    EEPROM[1] = DELAY_0 & 0xFF;
    EEPROM[2] = DELAY_1 >> 8;
    EEPROM[3] = DELAY_1 & 0xFF;
}
