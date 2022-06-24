// notes.ino

/**
#define NUM_NOTES                6
#define FIRST_BUTTON             (8 * 9)
#define NOTE_BUTTON(n)           (FIRST_BUTTON +  (n))
#define FIRST_SWITCH             (5 * 9 + 3)
#define NOTE_SWITCH(n)           (FIRST_SWITCH + (n))

#define CONTINUOUS_PULSE_SW      (3 * 9 + 8)
#define CONTINUOUS_ON            (Switches[CONTINUOUS_PULSE_SW].current)

#define PULSE_NOTES_PERIOD       500
#define PULSE_NOTES_ON_PERIOD    400
**/

#define SW_TO_NOTE(sw)  (Notes_by_sw[sw - (sw < FIRST_BUTTON ? FIRST_SWITCH : FIRST_BUTTON)])

byte EEPROM_notes_offset;

byte Notes_by_sw[] = {57, 60, 64, 67, 70, 72};  // A3, C4, E4, G4, Bb4, C5

void note_on_by_bt(byte sw) {
  byte note = SW_TO_NOTE(sw);
  // Switch takes precedence over button...
  if (!Switches[NOTE_SWITCH(note)].current) note_on(note);
}

void note_off_by_bt(byte sw) {
  byte note = SW_TO_NOTE(sw);
  // Switch takes precedence over button...
  if (!Switches[NOTE_SWITCH(note)].current) note_off(note);
}

void note_on_by_sw(byte sw) {
  byte note = SW_TO_NOTE(sw);
  if (!Switches[NOTE_BUTTON(note)].current) note_on(note);
  if (!CONTINUOUS_ON) {
    // Pulse mode!  Make sure pulsing is on...
    Periodic_period[PULSE_NOTES_ON] = PULSE_NOTES_PERIOD;
    Period_offset[PULSE_NOTES_ON] = 0;
    Periodic_period[PULSE_NOTES_OFF] = PULSE_NOTES_PERIOD;
    Period_offset[PULSE_NOTES_OFF] = PULSE_NOTES_ON_PERIOD;
  }
}

void note_off_by_sw(byte sw) {
  byte i;
  byte note = SW_TO_NOTE(sw);
  if (!Switches[NOTE_BUTTON(note)].current) note_off(note);
  if (!CONTINUOUS_ON) check_pulse_off();
}

void check_pulse_on(void) {
  // If any note switches are on, turn on pulsing!
  byte i;
  for (i = 0; i < NUM_NOTES; i++) {
    if (Switches[NOTE_SWITCH(i)].current) {
      // Some note switch is on, make sure pulsing is on
      Periodic_period[PULSE_NOTES_ON] = PULSE_NOTES_PERIOD;
      Period_offset[PULSE_NOTES_ON] = 0;
      Periodic_period[PULSE_NOTES_OFF] = PULSE_NOTES_PERIOD;
      Period_offset[PULSE_NOTES_OFF] = PULSE_NOTES_ON_PERIOD;
      break;
    }
  } // end for (i)
}

void check_pulse_off(void) {
  // If all note switches are off now, turn off pulsing.
  byte i;
  for (i = 0; i < NUM_NOTES; i++) {
    if (Switches[NOTE_SWITCH(i)].current) {
      // Some note switch is on, so don't turn off pulsing yet
      return;
    }
  } // end for (i)
  // No note switch is on, turn off pulsing
  Periodic_period[PULSE_NOTES_ON] = 0;
  Periodic_period[PULSE_NOTES_OFF] = 0;
}

void note_on(byte note) {
  midiEventPacket_t noteOn = {0x09, 0x90, note, 50};
  MidiUSB.sendMIDI(noteOn);
}

void note_off(byte note) {
  midiEventPacket_t noteOff = {0x08, 0x80, note, 0};
  MidiUSB.sendMIDI(noteOff);
}

void notes_on(void) {
  byte i;
  for (i = 0; i < NUM_NOTES; i++) {
    if (Switches[NOTE_SWITCH(i)].current) {
      note_on(i);
    }
  }
}

void notes_off(void) {
  byte i;
  for (i = 0; i < NUM_NOTES; i++) {
    if (Switches[NOTE_SWITCH(i)].current) {
      note_off(i);
    }
  }
}

void control_change(byte channel, byte control, byte value) {
  midiEventPacket_t controlChange = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(controlChange);
}

void flush(void) {
  MidiUSB.flush();
}

byte setup_notes(byte EEPROM_offset) {
  EEPROM_notes_offset = EEPROM_offset;
  byte i;
  for (i = 0; i < NUM_NOTES; i++) {
    Switch_closed_event[NOTE_BUTTON(i)] = NOTE_BT_ON;
    Switch_opened_event[NOTE_BUTTON(i)] = NOTE_BT_OFF;
    Switch_closed_event[NOTE_SWITCH(i)] = NOTE_SW_ON;
    Switch_opened_event[NOTE_SWITCH(i)] = NOTE_SW_OFF;
  }
  Switch_closed_event[CONTINUOUS_PULSE_SW] = CONTINUOUS;
  Switch_opened_event[CONTINUOUS_PULSE_SW] = PULSE;
  return 0;  // for now...
}

// vim: sw=2
