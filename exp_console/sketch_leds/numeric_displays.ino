// numeric_displays.ino

byte EEPROM_numeric_offset;

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
byte Numeric_display_offset[MAX_NUMERIC_DISPLAYS];  // byte_num of left-most digit

byte setup_numeric_displays(byte my_EEPROM_offset) {
  EEPROM_numeric_offset = my_EEPROM_offset;
  byte b = EEPROM[EEPROM_Num_numeric_displays()];
  if (b == 0xFF) {
    Serial.println("Num_numeric_displays not set in EEPROM");
  } else if (b > MAX_NUMERIC_DISPLAYS) {
    Errno = 91;
    Err_data = b;
  } else Num_numeric_displays = b;

  byte i;
  for (i = 0; i < Num_numeric_displays; i++) {
    b = EEPROM[EEPROM_Numeric_display_size(i)];
    if (b == 0xFF) {
      Serial.print("Numeric_display_size not set in EEPROM for ");
      Serial.println(i);
    } else if (b > MAX_NUMERIC_DISPLAY_SIZE) {
      Errno = 92;
      Err_data = b;
    } else Numeric_display_size[i] = b;

    b = EEPROM[EEPROM_Numeric_display_offset(i)];
    if (b == 0xFF) {
      Serial.print("Numeric_display_offset not set in EEPROM for ");
      Serial.println(i);
    } else if (b > Num_rows * NUM_COLS) {
      Errno = 93;
      Err_data = b;
    } else Numeric_display_offset[i] = b;
  } // end for (i)

  return 1 + 2*MAX_NUMERIC_DISPLAYS;  // EEPROM needed
} // end setup_numeric_displays()

const short Powers_of_ten[] = {0, 10, 100, 1000, 10000};  // must be signed! (I think)

const byte Numeric_7_segment_decode[] = {
  //ABCDEFGDP
  0b11111100,    // 0           A
  0b01100000,    // 1          F B
  0b11011010,    // 2           G
  0b11110010,    // 3          E C
  0b01100110,    // 4           D
  0b10110110,    // 5
  0b10111110,    // 6
  0b11100000,    // 7
  0b11111110,    // 8
  0b11110110,    // 9
  0b00000010,    // 10 (minus '-')
  0b00000000,    // 11 (all segments off)
  0b11101110,    // 12 'A'
  0b00111110,    // 13 'b'
  0b10011100,    // 14 'C'
  0b01111010,    // 15 'd'
  0b10011110,    // 16 'E'
  0b10001110,    // 17 'F'
  0b10111100,    // 18 'G'
  0b10110110,    // 19 'S'
  0b01101110,    // 20 'H'
  0b00011100,    // 21 'L'
};

void load_digit(byte display_num, byte digit_num, byte value, byte dp) {
  // digit_nums go left to right, starting with 0 as the left-most digit.
  // value of 10 produces a '-', 11 turns all segments off
  if (display_num >= Num_numeric_displays) {
    Errno = 94;
    Err_data = display_num;
  } else if (digit_num >= Numeric_display_size[display_num]) {
    Errno = 95;
    Err_data = digit_num;
  } else if (value > 11) {
    Errno = 96;
    Err_data = value;
  } else {
    byte bits = Numeric_7_segment_decode[value];
    if (dp) bits |= 1;
    load_8(bits, Numeric_display_offset[display_num] + 2*digit_num);
  }
}

void test_numeric_decoder(void) {
  // load 12 numeric digits into 4 x 3-digit displays, with alternating DPs
  byte i;
  for (i = 0; i < 12; i++) {
    load_digit(i/3, i % 3, i, i % 2);
  }
}

void load_numeric(byte display_num, short value, byte decimal_place) {
  // display_num indexes Numeric_display_size and Numeric_display_offset.
  // value must fit in the number of digits (reduced by 1 for '-' sign if value < 0).
  // decimal_place of 0, means no DP.  Otherwise it's the number of digits to the
  // right of the decimal point.
  byte display_size = Numeric_display_size[display_num];
  if (display_num >= Num_numeric_displays) {
    Errno = 97;
    Err_data = display_num;
  } else if (decimal_place >= display_size) {
    Errno = 98;
    Err_data = decimal_place;
  } else if (value >= Powers_of_ten[Numeric_display_size[display_num]] || 
             -value >= Powers_of_ten[Numeric_display_size[display_num] - 1]) {
    Errno = 99;
    Err_data = value/10;
  } else {
    byte addr = Numeric_display_offset[display_num];
    byte negative = value < 0;
    value = abs(value);
    byte bits;
    byte i;
    for (i = 0; i < display_size; i++) { // going from right to left
      if (negative && i + 1 == display_size) {
        // display a minus sign
        bits = 0b10;   // G segment (middle horz)
      } else if (!value && i && decimal_place <= i) {
        // display a blank
        bits = 0;
      } else {
        // display next digit
        bits = Numeric_7_segment_decode[value % 10];
        value /= 10;
      }
      if (decimal_place && decimal_place == i) bits |= 1;
      load_8(bits, addr + 2*(2 - i));
    } // end for (i)
  } // end ifs
}

void load_sharp_flat(byte display_num, byte sharp_flat) {
  // sharp_flat: 0 = nat (blank), 1 = SH, 2 = FL
  if (display_num >= Num_numeric_displays) {
    Errno = 100;
    Err_data = display_num;
  } else if (sharp_flat > 2) {
    Errno = 101;
    Err_data = sharp_flat;
  } else {
    byte addr = Numeric_display_offset[display_num];
    switch (sharp_flat) {
    case 0: // natural
      load_8(0b0, addr + 2*1);
      load_8(0b0, addr + 2*2);
      break;
    case 1: // sharp
      load_8(Numeric_7_segment_decode[19], addr + 2*1);
      load_8(Numeric_7_segment_decode[20], addr + 2*2);
      break;
    case 2: // flat
      load_8(Numeric_7_segment_decode[17], addr + 2*1);
      load_8(Numeric_7_segment_decode[21], addr + 2*2);
      break;
    } // end switch
  } // end ifs
}

void load_note(byte display_num, byte note, byte sharp_flat) {
  // display_num indexes Numeric_display_size and Numeric_display_offset.
  // note must be 0-6 for A-G
  // sharp_flat is 0 for natural, 1 for sharp, 2 for flat
  if (display_num >= Num_numeric_displays) {
    Errno = 102;
    Err_data = display_num;
  } else if (note > 6) {
    Errno = 103;
    Err_data = note;
  } else if (sharp_flat > 2) {
    Errno = 104;
    Err_data = sharp_flat;
  } else {
    byte addr = Numeric_display_offset[display_num];
    byte bits = Numeric_7_segment_decode[note + 12];
    if (sharp_flat) bits |= 0b1;  // turn on DP
    load_8(bits, addr);
    load_sharp_flat(display_num, sharp_flat);
  } // end ifs
}

void clear_display(byte display_num) {
  load_digit(display_num, 0, 11, 0);
  load_digit(display_num, 1, 11, 0);
  load_digit(display_num, 2, 11, 0);
}

// vim: sw=2
