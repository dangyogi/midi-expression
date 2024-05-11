// switches.h

#define NUM_ROWS         9
#define NUM_COLS         9

#define NUM_SWITCHES    81

#define SWITCH_NUM(row, col)    ((row) * NUM_COLS + (col))
#define SAVE_PROGRAM_SWITCH     SWITCH_NUM(1, 5)
#define SAVE_OR_SYNTH           (Switches[SAVE_PROGRAM_SWITCH].current)

#define FIRST_HARMONIC_SWITCH   SWITCH_NUM(4, 0)
#define FIRST_CHANNEL_SWITCH    SWITCH_NUM(6, 0)

typedef struct {
  byte current;                 // 0 == open, 1 == closed
  byte opening;                 // True or False, only when current is closed
  byte debounce_index;          // 0 (switch) or 1 (encoder)
  byte tag;                     // for use by other parts of the software
  unsigned long open_time;      // only valid when opening
} switch_t;

extern switch_t Switches[NUM_SWITCHES];

extern byte Rows[];  // Row pins
extern byte Cols[];  // Col pins

extern unsigned short Debounce_period[2];  // long (switches), short (encoders)

extern byte setup_switches(byte EEPROM_offset);

extern unsigned long Longest_scan;   // uSec

#define MAX_DEBOUNCE_COUNT    30
extern byte Debounce_delay_counts[2][MAX_DEBOUNCE_COUNT + 1];   // debounce_index, mSec

extern byte Close_counts[NUM_SWITCHES];

extern void scan_switches(byte trace=0);

// vim: sw=2
