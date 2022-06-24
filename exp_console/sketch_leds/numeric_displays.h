// sketch_numeric_displays.h

#define MAX_NUMERIC_DISPLAYS            32
#define MAX_NUMERIC_DISPLAY_SIZE        4

extern byte EEPROM_Num_numeric_displays(void);
extern byte EEPROM_Numeric_display_size(byte numeric_display);
extern byte EEPROM_Numeric_display_offset(byte numeric_display);

extern byte setup_numeric_displays(byte my_EEPROM_offset);

extern byte Num_numeric_displays;
extern byte Numeric_display_size[MAX_NUMERIC_DISPLAYS];    // number of digits
extern byte Numeric_display_offset[MAX_NUMERIC_DISPLAYS];  // byte_num of left-most digit

// value of 10 produces '-'
extern void load_digit(byte display_num, byte digit_num, byte value, byte dp);

// value may be < 0, but then is limited to one less digit.
// decimal_place of 0 means no decimal.  Otherwise it is the digit number for the DP.
extern void load_numeric(byte display_num, short value, byte decimal_place);

// note must be 0-6 (for A-G).
// sharp_flat of 0 means natural, 1 means sharp, 2 means flat.
extern void load_note(byte display_num, byte note, byte sharp_flat);

// vim: sw=2
