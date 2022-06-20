// sketch_numeric_displays.ino

#include "flash_errno.h"
//#include "step.h"
#include "sketch_numeric_displays.h"

static byte EEPROM_numeric_offset;

byte EEPROM_Num_numeric_displays(void) {
  return EEPROM_numeric_offset;
}

byte EEPROM_Numeric_display_size(byte numeric_display) {
  return 1 + numeric_display + EEPROM_numeric_offset;
}

byte EEPROM_Numeric_display_offset(byte numeric_display) {
  return 1 + MAX_NUMERIC_DISPLAYS + numeric_display + EEPROM_numeric_offset;
}

byte Num_numeric_displays;
byte Numeric_display_size[MAX_NUMERIC_DISPLAYS];    // number of digits
byte Numeric_display_offset[MAX_NUMERIC_DISPLAYS];  // byte_num of right-most digit

byte setup_numeric_displays(byte my_EEPROM_offset) {
  EEPROM_numeric_offset = my_EEPROM_offset;
  byte b = EEPROM[EEPROM_Num_numeric_displays()];
  if (b == 0xFF) {
    Serial.println("Num_numeric_displays not set in EEPROM");
  } else if (b > MAX_NUMERIC_DISPLAYS) {
    Errno = 100;
    Err_data = b;
  } else Num_numeric_displays = b;

  byte i;
  for (i = 0; i < Num_numeric_displays; i++) {
    b = EEPROM[EEPROM_Numeric_display_size(i)];
    if (b == 0xFF) {
      Serial.print("Numeric_display_size not set in EEPROM for ");
      Serial.println(i);
    } else if (b > MAX_NUMERIC_DISPLAY_SIZE) {
      Errno = 101;
      Err_data = b;
    } else Numeric_display_size[i] = b;

    b = EEPROM[EEPROM_Numeric_display_offset(i)];
    if (b == 0xFF) {
      Serial.print("Numeric_display_offset not set in EEPROM for ");
      Serial.println(i);
    } else if (b > Num_rows * NUM_COLS) {
      Errno = 102;
      Err_data = b;
    } else Numeric_display_offset[i] = b;
  } // end for (i)

  return 1 + 2*MAX_NUMERIC_DISPLAYS;  // EEPROM needed
} // end setup_numeric_displays()

const short Powers_of_ten[] = {0, 10, 100, 1000, 10000};

const byte Numeric_7_segment_decode[] = {
  //ABCDEFGDP
  0b11111100,    // 0           A
  0b01100000,    // 1          F B
  0b11011010,    // 2           G
  0b11110010,    // 3          E C
  0b11100110,    // 4           D
  0b10110110,    // 5
  0b10111110,    // 6
  0b11100000,    // 7
  0b11111110,    // 8
  0b11110110,    // 9
};

void load_numeric(byte display_num, short value, byte decimal_place) {
  // display_num indexes Numeric_display_size and Numeric_display_offset.
  // value must fit in the number of digits (reduced by 1 for '-' sign if value < 0).
  // decimal_place of 0, means no DP.  Otherwise it displayed on Nth digit from the right.
  if (display_num >= Num_numeric_displays) {
    Errno = 30;
    Err_data = display_num;
  } else if (decimal_place > Numeric_display_size[display_num]) {
    Errno = 31;
    Err_data = decimal_place;
  } else if (value >= Powers_of_ten[Numeric_display_size[display_num]] || 
             -value >= Powers_of_ten[Numeric_display_size[display_num] - 1]) {
    Errno = 32;
    Err_data = value/10;
  } else {
    byte i;
    // Turn off all segments on all digits
    for (i = 0; i < Numeric_display_size[display_num]; i++) {
      load_8(0, Numeric_display_offset[display_num] - i);
    }
    byte negative = value < 0;
    value = abs(value);
    byte addr = Numeric_display_offset[display_num];
    byte bits;
    i = 0;
    while (i == 0 || value) {
      bits = Numeric_7_segment_decode[value % 10];
      if (decimal_place == i + 1) bits |= 1;
      load_8(bits, addr - i);
      value /= 10;
      i++;
    } // end while
    if (negative) {
      bits = 0b10;
      if (decimal_place == i + 1) bits |= 1;
      load_8(bits, addr - i);
    } else if (decimal_place == i + 1) {
      load_8(0b1, addr - i);
    }
  } // end ifs
}
