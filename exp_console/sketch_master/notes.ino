// notes.ino

#define SW_TO_NOTE_NUM(sw)   (sw - (sw < FIRST_BUTTON ? FIRST_SWITCH : FIRST_BUTTON))

byte EEPROM_notes_offset;

byte MIDI_note[] = {57, 60, 64, 67, 70, 72};  // A3, C4, E4, G4, Bb4, C5

byte Notes_currently_on;  // Set/Reset during pulsing

void note_on_by_bt(byte sw) {
  byte note = SW_TO_NOTE_NUM(sw);
  // Switch takes precedence over button...
  if (!Switches[NOTE_SWITCH(note)].current) note_on(note);
}

void note_off_by_bt(byte sw) {
  byte note = SW_TO_NOTE_NUM(sw);
  // Switch takes precedence over button...
  if (!Switches[NOTE_SWITCH(note)].current) note_off(note);
}

void note_on_by_sw(byte sw) {
  byte note = SW_TO_NOTE_NUM(sw);
  if (CONTINUOUS_ON) {
    if (!Switches[NOTE_BUTTON(note)].current) note_on(note);
  } else {
    // Pulse mode!  Make sure pulsing is on...
    Periodic_period[PULSE_NOTES_ON] = PULSE_NOTES_PERIOD;
    Period_offset[PULSE_NOTES_ON] = 0;
    Periodic_period[PULSE_NOTES_OFF] = PULSE_NOTES_PERIOD;
    Period_offset[PULSE_NOTES_OFF] = PULSE_NOTES_ON_PERIOD;
    if (Notes_currently_on) note_on(note);
  }
}

void note_off_by_sw(byte sw) {
  byte note = SW_TO_NOTE_NUM(sw);
  if (!Switches[NOTE_BUTTON(note)].current && (CONTINUOUS_ON || Notes_currently_on)) {
    note_off(note);
  }
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
  usbMIDI.send(usbMIDI.NoteOn, MIDI_note[note], 50, 1, 0);
}

void note_off(byte note) {
  usbMIDI.send(usbMIDI.NoteOff, MIDI_note[note], 0, 1, 0);
}

void notes_on(void) {
  byte i;
  if (!Notes_currently_on) {
    for (i = 0; i < NUM_NOTES; i++) {
      if (Switches[NOTE_SWITCH(i)].current) {
        note_on(i);
      }
    }
    Notes_currently_on = 1;
  }
}

void notes_off(void) {
  byte i;
  if (Notes_currently_on) {
    for (i = 0; i < NUM_NOTES; i++) {
      if (Switches[NOTE_SWITCH(i)].current) {
        note_off(i);
      }
    }
    Notes_currently_on = 0;
  }
}

void control_change(byte channel, byte control, byte value, byte cable) {
  usbMIDI.send(usbMIDI.ControlChange, control, value, channel, cable);
}

/***** Not needed any more, now that we have multiple virtual MIDI cables!
void system_common(byte code, byte b1, byte b2, byte cable) {
  // FIX: looks like this needs to be a call to usb_midi_write_packed.
  // see: ~/arduino-1,8.19/hardware/teensy/avr/cores/teensy[34]/usb_midi.h, search for 'void send('
  usbMIDI.send(usbMIDI.SystemCommon, code, b1, b2, cable);
}
******/

void flush_midi(void) {
  usbMIDI.send_now();
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
