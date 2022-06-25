// notes.h

#define NUM_NOTES                6
#define FIRST_BUTTON             SWITCH_NUM(8, 0)
#define NOTE_BUTTON(n)           (FIRST_BUTTON +  (n))
#define FIRST_SWITCH             SWITCH_NUM(5, 3)
#define NOTE_SWITCH(n)           (FIRST_SWITCH + (n))

#define CONTINUOUS_PULSE_SW      SWITCH_NUM(3, 8)
#define CONTINUOUS_ON            (Switches[CONTINUOUS_PULSE_SW].current)

#define PULSE_NOTES_PERIOD       500
#define PULSE_NOTES_ON_PERIOD    400

extern byte Notes_by_sw[];       // MIDI note by note sw#

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
