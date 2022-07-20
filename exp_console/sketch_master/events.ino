// events.ino

byte EEPROM_events_offset;

void run_event(byte event_num, byte param) {
  // This runs Switch_closed_events, Switch_opened_events and Encoder_event events.
  if (event_num != 0xFF) {
    byte enc;
    byte adj;
    encoder_var_t *var;
    switch (event_num) {
    case ENC_A_CLOSED(0): case ENC_A_CLOSED(1): case ENC_A_CLOSED(2):
    case ENC_A_CLOSED(3): case ENC_A_CLOSED(4): case ENC_A_CLOSED(5):
      // switch closed for encoders A
      enc = event_num % NUM_ENCODERS;
      var = Encoders[enc].var;
      if (var != NULL && var->flags & 1) {  // enabled
        if (!(Encoders[enc].state & 2)) {
          // CW
          var->value += var->bt_mul[Switches[Encoders[enc].A_sw + 2].current];
          if (var->value >= var->max) {
            if (var->flags & 2) {
              // cycle
              var->value = var->min;
              var->changed = 1;
              encoder_changed(enc);
            }
          } else {
            var->changed = 1;
            encoder_changed(enc);
          }
        }
      } // end if (enabled)
      Encoders[enc].state |= 1;
      break;
    case ENC_B_CLOSED(0): case ENC_B_CLOSED(1): case ENC_B_CLOSED(2):
    case ENC_B_CLOSED(3): case ENC_B_CLOSED(4): case ENC_B_CLOSED(5):
      // switch closed for encoders B
      enc = event_num % NUM_ENCODERS;
      //var = Encoders[enc].var;
      //if (var != NULL && var->flags & 1) {  // enabled
      //} // end if (enabled)
      Encoders[enc].state |= 2;
      break;
    case ENC_A_OPENED(0): case ENC_A_OPENED(1): case ENC_A_OPENED(2):
    case ENC_A_OPENED(3): case ENC_A_OPENED(4): case ENC_A_OPENED(5):
      // switch opened for encoders A
      enc = event_num % NUM_ENCODERS;
      var = Encoders[enc].var;
      if (var != NULL && var->flags & 1) {  // enabled
        if (!(Encoders[enc].state & 2)) {
          // CCW
          adj = var->bt_mul[Switches[Encoders[enc].A_sw + 2].current];
          if (var->value <= var->min + adj) {
            if (var->flags & 2) {
              // cycle
              var->value = var->max;
              var->changed = 1;
              encoder_changed(enc);
            }
          } else {
            var->value -= adj;
            var->changed = 1;
            encoder_changed(enc);
          }
        }
      } // end if (enabled)
      Encoders[enc].state &= ~1;
      break;
    case ENC_B_OPENED(0): case ENC_B_OPENED(1): case ENC_B_OPENED(2):
    case ENC_B_OPENED(3): case ENC_B_OPENED(4): case ENC_B_OPENED(5):
      // switch opened for encoders B
      enc = event_num % NUM_ENCODERS;
      //var = Encoders[enc].var;
      //if (var != NULL && var->flags & 1) {  // enabled
      //} // end if (enabled)
      Encoders[enc].state &= ~2;
      break;
    case SYNTH_PROGRAM_OR_FUNCTION_CHANGED: // Synth_or_program or Function changed
      reset_function_encoders();
      break;
    case NOTE_BT_ON:   // tagged to note buttons
      note_on_by_bt(param);
      break;
    case NOTE_BT_OFF:  // tagged to note buttons
      note_off_by_bt(param);
      break;
    case NOTE_SW_ON:   // tagged to note switches
      note_on_by_sw(param);
      break;
    case NOTE_SW_OFF:  // tagged to note switches
      note_off_by_sw(param);
      break;
    case CONTINUOUS:
      // changing from pulse to continuous, notes may be off right now!
      Periodic_period[PULSE_NOTES_ON] = 0;
      Periodic_period[PULSE_NOTES_OFF] = 0;
      notes_on();
      break;
    case PULSE:
      // changing from continuous to pulse, no notes may be switched on now...
      check_pulse_on();
      break;
    case CHANNEL_ON:
      channel_on(param);
      break;
    case CHANNEL_OFF:
      channel_off(param);
      break;
    case HARMONIC_ON:
      harmonic_on(param);
      break;
    case HARMONIC_OFF:
      harmonic_off(param);
      break;
    } // end switch (event_num)
  } // end if (0xFF)
}

byte Switch_closed_event[NUM_SWITCHES]; // 0xFF is None
byte Switch_opened_event[NUM_SWITCHES]; // 0xFF is None

byte Trace_events = 0;

void switch_closed(byte sw) {
  if (Trace_events) {
    Serial.print(F("switch "));
    Serial.print(sw);
    Serial.print(F(" closed, event "));
    Serial.println(Switch_closed_event[sw]);
  }
  run_event(Switch_closed_event[sw], sw);
}

void switch_opened(byte sw) {
  if (Trace_events) {
    Serial.print(F("switch "));
    Serial.print(sw);
    Serial.print(F(" opened, event "));
    Serial.println(Switch_opened_event[sw]);
  }
  run_event(Switch_opened_event[sw], sw);
}

byte Encoder_event[NUM_ENCODERS]; // 0xFF is None

void encoder_changed(byte enc) {
  if (Trace_events) {
    Serial.print(F("encoder "));
    Serial.print(enc);
    Serial.print(F(" changed, event "));
    Serial.println(Encoder_event[enc]);
  }
  run_event(Encoder_event[enc], enc);
}

byte setup_events(byte EEPROM_offset) {
  EEPROM_events_offset = EEPROM_offset;
  byte i;
  for (i = 0; i < NUM_SWITCHES; i++) {
    Switch_closed_event[i] = 0xFF;
    Switch_opened_event[i] = 0xFF;
  }
  Switch_closed_event[SAVE_PROGRAM_SWITCH] = SYNTH_PROGRAM_OR_FUNCTION_CHANGED;
  Switch_opened_event[SAVE_PROGRAM_SWITCH] = SYNTH_PROGRAM_OR_FUNCTION_CHANGED;

  for (i = 0; i < NUM_ENCODERS; i++) {
    Encoder_event[i] = 0xFF;
  }
  Encoder_event[FUNCTION_ENCODER] = SYNTH_PROGRAM_OR_FUNCTION_CHANGED;

  // reset_function_encoders();  Moved to setup_functions()...

  return 0;  // for now...
}

// vim: sw=2
