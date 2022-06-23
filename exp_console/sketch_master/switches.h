// switches.h

#define NUM_SWITCHES    81

#define SAVE_PROGRAM_SWITCH     (1 * 9 + 5)
#define SYNTH_OR_PROGRAM        (Switches[SAVE_PROGRAM_SWITCH].current)

typedef struct {
  byte current;                 // 0 == open, 1 == closed
  byte opening;                 // True or False, only when current is closed
  unsigned long open_time;      // only valid when opening
} switch_t;

extern switch_t Switches[NUM_SWITCHES];

extern byte setup_switches(byte EEPROM_offset);

extern unsigned long Longest_scan;   // uSec

extern byte Close_counts[NUM_SWITCHES];

extern void scan_switches(byte trace=0);

// vim: sw=2
