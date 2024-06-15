// notes.h

// Synth is on cable 0
// Player is on cable 1

#define NUM_NOTES                6
#define FIRST_BUTTON             SWITCH_NUM(8, 0)
#define NOTE_BUTTON(n)           (FIRST_BUTTON + (n))
#define FIRST_SWITCH             SWITCH_NUM(5, 3)
#define NOTE_SWITCH(n)           (FIRST_SWITCH + (n))
#define SYNTH_CABLE              0
#define PLAYER_CABLE             1
#define PLAYER_CHANNEL           15
#define SYNTH_CHANNEL            15

#define CONTINUOUS_PULSE_SW      SWITCH_NUM(3, 8)
#define CONTINUOUS_ON            (Switches[CONTINUOUS_PULSE_SW].current)

extern byte MIDI_note[];         // MIDI note by note# (0-5)

extern byte Notes_currently_on;  // Set/reset by notes_on/notes_off

extern void note_on_by_bt(byte sw);
extern void note_off_by_bt(byte sw);
extern void note_on_by_sw(byte sw);
extern void note_off_by_sw(byte sw);
extern void check_pulse_on(void);
extern void check_pulse_off(void);
extern void note_on(byte note);
extern void note_off(byte note);
extern void notes_on(void);      // call every time pulsed notes should turned on
extern void notes_off(void);     // call every time pulsed notes should be turned off

// These take internal channel numbers (starting at 0 rather than 1)
extern void control_change(byte channel, byte control, byte value, byte cable);
extern void nrpn_change(byte channel, unsigned short param_num, unsigned short value, byte cable);

extern void flush_midi(void);

extern byte setup_notes(byte EEPROM_offset);
