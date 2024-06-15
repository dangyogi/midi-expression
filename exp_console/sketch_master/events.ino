// events.ino

byte EEPROM_events_offset;

byte Trace_events = 0;
byte Trace_encoders = 0;

void inc_encoder(byte enc) {
  // Called each time both encoders switches become open (detent position reached).
  // Only called if encoder is enabled.
  byte new_value;
  if (Trace_encoders) {
    Serial.print("inc_encoder "); Serial.print(enc);
    Serial.print(", count "); Serial.print(Encoders[enc].count);
    Serial.print(", value "); Serial.print(Encoders[enc].var->value);
    Serial.print(", max "); Serial.print(Encoders[enc].var->var_type->max);
    Serial.print(", min "); Serial.print(Encoders[enc].var->var_type->min);
    Serial.print(", flags 0b"); Serial.println(Encoders[enc].var->var_type->flags, BIN);
  }
  if (Encoders[enc].count >= 2) {
    // inc encoder value
    variable_t *var = Encoders[enc].var;
    new_value = var->value + var->var_type->bt_mul[Switches[Encoders[enc].A_sw + 2].current];
    if (new_value > var->var_type->max) {
      if (Trace_encoders) {
        Serial.print("  new_value ("); Serial.print(new_value); Serial.println(") > max");
      }
      if (var->var_type->flags & ENCODER_FLAGS_CYCLE) {
        // cycle
        if (Trace_encoders) {
          Serial.println("  CYCLE on");
        }
        var->value = new_value - (var->var_type->max + 1 - var->var_type->min);
        var->changed = 1;
        Function_changed = 1;
        encoder_changed(enc);
      } else if (var->value != var->var_type->max) {
        if (Trace_encoders) {
          Serial.println("  current value != max");
        }
        var->value = var->var_type->max;
        var->changed = 1;
        Function_changed = 1;
        encoder_changed(enc);
      }
    } else {
      var->value = new_value;
      var->changed = 1;
      Function_changed = 1;
      encoder_changed(enc);
    }
  } else if (Encoders[enc].count <= -2) {
    // dec encoder value
    variable_t *var = Encoders[enc].var;
    byte adj = var->var_type->bt_mul[Switches[Encoders[enc].A_sw + 2].current];
    if (var->value < var->var_type->min + adj) {
      if (Trace_encoders) {
        Serial.print("  value < min + adj ("); Serial.print(adj); Serial.println(")");
      }
      if (var->var_type->flags & ENCODER_FLAGS_CYCLE) {
        // cycle
        if (Trace_encoders) {
          Serial.println("  CYCLE on");
        }
        var->value = (var->value + var->var_type->max + 1) - var->var_type->min - adj;
        var->changed = 1;
        Function_changed = 1;
        encoder_changed(enc);
      } else if (var->value != var->var_type->min) {
        if (Trace_encoders) {
          Serial.println("  current value != min");
        }
        var->value = var->var_type->min;
        var->changed = 1;
        Function_changed = 1;
        encoder_changed(enc);
      }
    } else {
      var->value -= adj;
      var->changed = 1;
      Function_changed = 1;
      encoder_changed(enc);
    }
  }
  Encoders[enc].count = 0;
}

void run_event(byte event_num, byte param) {
  // This runs Switch_closed_events, Switch_opened_events and Encoder display_value and encoder_events.
  if (event_num != 0xFF) {
    byte enc, pots_index, pot;
    trigger_t *trig;
    variable_t *var;
    switch (event_num) {
    case ENC_A_CLOSED(0): case ENC_A_CLOSED(1): case ENC_A_CLOSED(2):
    case ENC_A_CLOSED(3): case ENC_A_CLOSED(4): case ENC_A_CLOSED(5):
      // switch A closed for encoders
      enc = EVENT_ENCODER_NUM(event_num);
      var = Encoders[enc].var;
      if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {  // enabled
        byte B_sw = Encoders[enc].A_sw + 1;
        if (!Switches[B_sw].current) {  // B open
          // CW
          Encoders[enc].count += 1;
        } else {
          // CCW
          Encoders[enc].count -= 1;
        }
      } // end if (enabled)
      break;
    case ENC_B_CLOSED(0): case ENC_B_CLOSED(1): case ENC_B_CLOSED(2):
    case ENC_B_CLOSED(3): case ENC_B_CLOSED(4): case ENC_B_CLOSED(5):
      // switch closed for encoders B
      enc = EVENT_ENCODER_NUM(event_num);
      var = Encoders[enc].var;
      if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {  // enabled
        byte A_sw = Encoders[enc].A_sw;
        if (Switches[A_sw].current) {   // A closed
          // CW
          Encoders[enc].count += 1;
        } else {
          // CCW
          Encoders[enc].count -= 1;
        }
      } // end if (enabled)
      break;
    case ENC_A_OPENED(0): case ENC_A_OPENED(1): case ENC_A_OPENED(2):
    case ENC_A_OPENED(3): case ENC_A_OPENED(4): case ENC_A_OPENED(5):
      // switch A opened for encoders
      enc = EVENT_ENCODER_NUM(event_num);
      var = Encoders[enc].var;
      if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {  // enabled
        byte B_sw = Encoders[enc].A_sw + 1;
        if (Switches[B_sw].current) {   // B closed
          // CW
          Encoders[enc].count += 1;
        } else {
          // CCW, back at detent
          Encoders[enc].count -= 1;
          inc_encoder(enc);
        }
      } // end if (enabled)
      break;
    case ENC_B_OPENED(0): case ENC_B_OPENED(1): case ENC_B_OPENED(2):
    case ENC_B_OPENED(3): case ENC_B_OPENED(4): case ENC_B_OPENED(5):
      // switch B opened for encoders
      enc = EVENT_ENCODER_NUM(event_num);
      var = Encoders[enc].var;
      if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {  // enabled
        byte A_sw = Encoders[enc].A_sw;
        if (!Switches[A_sw].current) {   // A open
          // CW, back at detent
          Encoders[enc].count += 1;
          inc_encoder(enc);
        } else {
          // CCW
          Encoders[enc].count -= 1;
        }
      } // end if (enabled)
      break;
    case FUNCTION_CHANGED: // Function changed
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
      turn_off_periodic_fun(PULSE_NOTES_ON);
      turn_off_periodic_fun(PULSE_NOTES_OFF);
      notes_on();
      break;
    case PULSE:
      // changing from continuous to pulse, but no notes may be switched on now...
      check_pulse_on();
      break;
    case CHANNEL_ON:
      channel_on(param);
      disable_triggers();
      break;
    case CHANNEL_OFF:
      channel_off(param);
      disable_triggers();
      break;
    case HARMONIC_ON:
      harmonic_on(param);
      disable_triggers();
      break;
    case HARMONIC_OFF:
      harmonic_off(param);
      disable_triggers();
      break;
    case UPDATE_CHOICES:  // enc
      select_led(param);
      break;
    case UPDATE_LINEAR_NUM:  // enc
      display_linear_number(param);
      break;
    case UPDATE_GEOMETRIC_NUM:  // enc
      display_geometric_number(param);
      break;
    case UPDATE_NOTE:  // enc
      display_note(param);
      break;
    case UPDATE_SHARPS_FLATS:  // enc
      display_sharps_flats(param);
      break;
    case TRIGGER_SW_ON:   // trigger switch
      trig = &Triggers[Switches[param].tag];
      if (Lowest_channel != 0xFF) {
        trig->continuous = 1;
        led_on(trig->led);
      }
      break;
    case TRIGGER_SW_OFF:  // trigger switch
      trig = &Triggers[Switches[param].tag];
      trig->continuous = 0;
      led_off(trig->led);
      break;
    case TRIGGER_BT_PRESSED:  // trigger button
      trig = &Triggers[Switches[param].tag];
      if (!trig->continuous && Lowest_channel != 0xFF) {
        led_on(trig->led);
        check_trigger(Switches[param].tag);
      }
      break;
    case TRIGGER_BT_RELEASED: // trigger button
      trig = &Triggers[Switches[param].tag];
      if (!trig->continuous) {
        led_off(trig->led);
      }
      break;
    case CHECK_POTS: // trigger_num, called when trigger button is pressed
      for (pots_index = 0; pots_index < Num_pots[param]; pots_index++) {
        pot = Pots[param][pots_index];
        if (Synced_pot_value[pot] != Current_pot_value[pot]) {
          send_pot(pot);
        }
      }
      break;
    case CHECK_FUNCTIONS: // trigger_num, called when trigger button is pressed
      if (Function_changed) {
        send_function();
      }
      break;
    case FUN_PARAM_CHANGED: // enc, called when fun parameter encoder changes
      if (Triggers[FUNCTIONS_TRIGGER].continuous) {
        send_function();
      }
      break;
    default:
      Errno = 30;
      Err_data = event_num;
      break;
    } // end switch (event_num)
  } // end if (0xFF)
} // end run_event()

byte Switch_closed_event[NUM_SWITCHES]; // 0xFF is None
byte Switch_opened_event[NUM_SWITCHES]; // 0xFF is None

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

void encoder_changed(byte enc) {
  if (Trace_encoders) {
    Serial.print(F("encoder "));
    Serial.print(enc);
    Serial.print(F(" changed, display_value "));
    Serial.print(Encoders[enc].var->var_type->display_value);
    Serial.print(F(", new value "));
    Serial.print(Encoders[enc].var->value);
    Serial.print(F(", encoder_event "));
    Serial.println(Encoders[enc].encoder_event);
  }
  run_event(Encoders[enc].var->var_type->display_value, enc);
  run_event(Encoders[enc].encoder_event, enc);
}

byte setup_events(byte EEPROM_offset) {
  EEPROM_events_offset = EEPROM_offset;
  byte i;
  for (i = 0; i < NUM_SWITCHES; i++) {
    Switch_closed_event[i] = 0xFF;
    Switch_opened_event[i] = 0xFF;
  }

  return 0;  // for now...
}

// vim: sw=2
