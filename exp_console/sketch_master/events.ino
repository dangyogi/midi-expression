// events.ino

byte EEPROM_events_offset;

void run_event(byte event_num, byte param) {
  if (event_num != 0xFF) {
    byte enc;
    byte adj;
    encoder_var_t *var;
    switch (event_num) {
    case 0: case 1: case 2: case 3: case 4: case 5:
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
    case 6: case 7: case 8: case 9: case 10: case 11:
      // switch closed for encoders B
      enc = event_num % NUM_ENCODERS;
      //var = Encoders[enc].var;
      //if (var != NULL && var->flags & 1) {  // enabled
      //} // end if (enabled)
      Encoders[enc].state |= 2;
      break;
    case 12: case 13: case 14: case 15: case 16: case 17:
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
    case 18: case 19: case 20: case 21: case 22: case 23:
      // switch opened for encoders B
      enc = event_num % NUM_ENCODERS;
      //var = Encoders[enc].var;
      //if (var != NULL && var->flags & 1) {  // enabled
      //} // end if (enabled)
      Encoders[enc].state &= ~2;
      break;
    case 24: // Synth_or_program or Function changed
      reset_function_encoders();
      break;
    } // end switch (event_num)
  } // end if (0xFF)
}

byte Switch_closed_event[NUM_SWITCHES]; // 0xFF is None
byte Switch_opened_event[NUM_SWITCHES]; // 0xFF is None

void switch_closed(byte sw) {
  run_event(Switch_closed_event[sw], sw);
}

void switch_opened(byte sw) {
  run_event(Switch_opened_event[sw], sw);
}

byte Encoder_event[NUM_ENCODERS]; // 0xFF is None

void encoder_changed(byte enc) {
  run_event(Encoder_event[enc], enc);
}

byte setup_events(byte EEPROM_offset) {
  EEPROM_events_offset = EEPROM_offset;
  byte i;
  for (i = 0; i < NUM_SWITCHES; i++) {
    Switch_closed_event[i] = 0xFF;
    Switch_opened_event[i] = 0xFF;
  }
  Switch_closed_event[SAVE_PROGRAM_SWITCH] = 24;
  Switch_opened_event[SAVE_PROGRAM_SWITCH] = 24;

  for (i = 0; i < NUM_ENCODERS; i++) {
    Encoder_event[i] = 0xFF;
  }
  Encoder_event[0] = 24;
  reset_function_encoders();

  return 0;  // for now...
}

// vim: sw=2
