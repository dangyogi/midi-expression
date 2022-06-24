// notes.h

#define NUM_NOTES                6
#define FIRST_BUTTON             (8 * 9)
#define NOTE_BUTTON(n)           (FIRST_BUTTON +  (n))
#define FIRST_SWITCH             (5 * 9 + 3)
#define NOTE_SWITCH(n)           (FIRST_SWITCH + (n))

#define CONTINUOUS_PULSE_SW      (3 * 9 + 8)
#define CONTINUOUS_ON            (Switches[CONTINUOUS_PULSE_SW].current)

#define PULSE_NOTES_PERIOD       500
#define PULSE_NOTES_ON_PERIOD    400

extern void note_on_by_bt(byte sw);
extern void note_off_by_bt(byte sw);
extern void note_on_by_sw(byte sw);
extern void note_off_by_sw(byte sw);
extern void check_pulse_on(void);
extern void check_pulse_off(void);
extern void note_on(byte note);
extern void note_off(byte note);
extern void notes_on(byte note);
extern void notes_off(byte note);
extern void control_change(byte channel, byte control, byte value);
extern void flush(void);

extern byte setup_notes(byte EEPROM_offset);
