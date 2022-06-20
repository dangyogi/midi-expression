// sketch_alpha_displays.h

#define MAX_NUM_STRINGS     4
#define MAX_STRING_LEN      30
#define END_GAP             3
#define SCROLL_DELAY        250  /* mSec */

extern byte EEPROM_Num_alpha_strings(void);
extern byte EEPROM_Alpha_num_chars(byte string_num);
extern byte EEPROM_Alpha_index(byte string_num);

extern byte setup_alpha_displays(byte my_EEPROM_offset);

extern void load_string(byte string_num, char *s);

extern byte advance_strings(void);  // Returns 1 if scrolling done, else 0.
