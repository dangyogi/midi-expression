// sketch_numeric_displays.h

#define MAX_NUMERIC_DISPLAYS            32
#define MAX_NUMERIC_DISPLAY_SIZE        4

extern byte EEPROM_Num_numeric_displays(void);
extern byte EEPROM_Numeric_display_size(byte numeric_display);
extern byte EEPROM_Numeric_display_offset(byte numeric_display);

extern byte setup_numeric_displays(byte my_EEPROM_offset);

extern byte Num_numeric_displays;
extern byte Numeric_display_size[MAX_NUMERIC_DISPLAYS];    // number of digits
extern byte Numeric_display_offset[MAX_NUMERIC_DISPLAYS];  // byte_num of right-most digit

extern void load_numeric(byte display_num, short value, byte decimal_place);

