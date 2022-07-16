// functions.ino

/**
#define NUM_CH_FUNCTIONS        8
#define NUM_HM_FUNCTIONS        7
#define NUM_FUNCTIONS           (NUM_CH_FUNCTIONS + NUM_HM_FUNCTIONS)
#define NUM_FUNCTION_ENCODERS   4
**/

byte EEPROM_functions_offset;

byte Function_encoders[] = {1, 2, 3, 4};

encoder_var_t Functions[NUM_FUNCTIONS][NUM_FUNCTION_ENCODERS] = {
  {  // function 0: key_signature
    {0, 14, 0b01, 1, 1}, {0, 0, 0b00}, {0, 0, 0b00}, {0, 1, 0b11, 1, 1},
  },
  {  // function 1: tune absolute
    {0, 0, 0b00}, {0, 11, 0b11, 1, 1}, {0, 7, 0b01, 1, 1}, {0, 127, 0b01, 1, 10},
  },
  {  // function 2: match tuning
    {0, 0, 0b00}, {0, 11, 0b11, 1, 1}, {0, 15, 0b11, 1, 5}, {0, 0, 0b00},
  },
  {  // function 3: equal_temperament
    {0, 0, 0b00}, {0, 0, 0b00}, {0, 127, 0b01, 1, 10}, {0, 0, 0b00},
  },
  {  // function 4: well_tempered
    {0, 11, 0b11, 1, 1}, {0, 11, 0b11, 1, 1}, {0, 0, 0b00}, {0, 0, 0b00},
  },
  {  // function 5: meantone
    {0, 11, 0b11, 1, 1}, {0, 11, 0b11, 1, 1}, {0, 127, 0b01, 1, 10}, {0, 1, 0b11, 1, 1},
  },
  {  // function 6: just_intonation
    {0, 11, 0b11, 1, 1}, {0, 11, 0b11, 1, 1}, {0, 0, 0b00}, {0, 13, 0b11, 1, 3},
  },
  {  // function 7: pythagorean
    {0, 11, 0b11, 1, 1}, {0, 0, 0b00}, {0, 127, 0b01, 1, 10}, {0, 0, 0b00},
  },
  {  // function 8: harmonic basics
    {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10}, {0, 127, 0b11, 1, 10}, {0, 0, 0b00},
  },
  {  // function 9: freq env: ramp
    {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10},
    {0, 127, 0b01, 1, 10},
  },
  {  // function 10: freq env: sine
    {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10}, {0, 0, 0b00},
  },
  {  // function 11: attack
    {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10},
    {0, 127, 0b01, 1, 10},
  },
  {  // function 12: decay
    {0, 127, 0b01, 1, 10}, {0, 0, 0b00}, {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10},
  },
  {  // function 13: sustain
    {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10},
    {0, 127, 0b01, 1, 10},
  },
  {  // function 14: release
    {0, 127, 0b01, 1, 10}, {0, 0, 0b00}, {0, 127, 0b01, 1, 10}, {0, 127, 0b01, 1, 10},
  },
};

byte Lowest_harmonic = 0xFF;  // 0-9, 0xFF when all switches off
byte Lowest_channel = 0xFF;   // 0-15, 0xFF when all switches off

void set_function_encoder_values(void) {
  if (Lowest_channel != 0xFF) {
    byte fun = FUNCTION;
    byte i;
    for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
      encoder_var_t *var = Encoders[Function_encoders[i]].var;
      if (var != NULL && (var->flags & 1)) {
        if (fun < NUM_CH_FUNCTIONS) {
          var->value = Channel_values[SAVE_OR_SYNTH][Lowest_channel][var->var_num];
        } else if (Lowest_harmonic != 0xFF) {
          var->value = Harmonic_values[SAVE_OR_SYNTH][Lowest_channel]
                                                     [Lowest_harmonic][var->var_num];
        } // end if (channel or harmonic)
      } // end if (encoder enabled)
    } // end for (i)
  } // end if (Lowest_channel set)
}

void copy_function_encoder_values(void) {
  byte fun = FUNCTION;
  byte i;
  for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
    encoder_var_t *var = Encoders[Function_encoders[i]].var;
    if (var != NULL && (var->flags & 1)) {
      byte ch, hm;
      for (ch = 0; ch < NUM_CHANNELS; ch++) {
        if (Switches[FIRST_CHANNEL_SWITCH + ch].current) {
          if (fun < NUM_CH_FUNCTIONS) {
            Channel_values[SAVE_OR_SYNTH][ch][var->var_num] = var->value;
          } else {
            for (hm = 0; hm < NUM_HARMONICS; hm++) {
              if (Switches[FIRST_HARMONIC_SWITCH + hm].current) {
                Harmonic_values[SAVE_OR_SYNTH][ch][hm][var->var_num] = var->value;
              } // end if (hm switch on)
            } // end for (hm)
          } // end if (channel or harmonic)
        } // end if (ch switch on)
      } // end for (ch)
    } // end if (encoder enabled)
  } // end for (i)
}

void harmonic_on(byte sw) {
  byte hm = sw - FIRST_HARMONIC_SWITCH;
  if (hm < Lowest_harmonic) {
    Lowest_harmonic = hm;
    set_function_encoder_values();
  }
}

void harmonic_off(byte sw) {
  byte hm = sw - FIRST_HARMONIC_SWITCH;
  if (hm == Lowest_harmonic) {
    byte i;
    for (i = hm + 1; i < NUM_HARMONICS; i++) {
      if (Switches[FIRST_HARMONIC_SWITCH + i].current) {
        Lowest_harmonic = i;
        set_function_encoder_values();
        return;
      }
    } // end for (i)
    // No harmonic switches on
    Lowest_harmonic = 0xFF;
    set_function_encoder_values();
  } // end if (Lowest_harmonic)
}

void channel_on(byte sw) {
  byte ch = sw - FIRST_CHANNEL_SWITCH;
  if (ch < Lowest_channel) {
    Lowest_channel = ch;
    set_function_encoder_values();
  }
}

void channel_off(byte sw) {
  byte ch = sw - FIRST_CHANNEL_SWITCH;
  if (ch == Lowest_channel) {
    byte i;
    for (i = ch + 1; i < NUM_CHANNELS; i++) {
      if (Switches[FIRST_CHANNEL_SWITCH + i].current) {
        Lowest_channel = i;
        set_function_encoder_values();
        return;
      }
    } // end for (i)
    // No channel switches on
    Lowest_channel = 0xFF;
    set_function_encoder_values();
  } // end if (Lowest_channel)
}

void reset_function_encoders(void) {
  Encoders[FUNCTION_ENCODER].var = &Function_var[SAVE_OR_SYNTH];
  byte i;
  for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
    Encoders[Function_encoders[i]].var = &Functions[FUNCTION][i];
  } // end for (i)
  set_function_encoder_values();
}

byte setup_functions(byte EEPROM_offset) {
  // Returns num EEPROM bytes needed.
  EEPROM_functions_offset = EEPROM_offset;
  byte fun, enc, var_num;

  // assign var_nums to CH FUNS
  var_num = 0;
  for (fun = 0; fun < NUM_CH_FUNCTIONS; fun++) {
    for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
      if (Functions[fun][enc].flags & 1) { // enabled
        Functions[fun][enc].var_num = var_num++;
      } // end if (enabled)
    } // end for (enc)
  } // end for (fun)

  if (var_num != NUM_CHANNEL_VARS) {
    Errno = 10;
    Err_data = var_num;
  }

  // assign var_nums to HM FUNS
  var_num = 0;
  for (fun = NUM_CH_FUNCTIONS; fun < NUM_FUNCTIONS; fun++) {
    for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
      if (Functions[fun][enc].flags & 1) { // enabled
        Functions[fun][enc].var_num = var_num++;
      } // end if (enabled)
    } // end for (enc)
  } // end for (fun)

  if (var_num != NUM_HARMONIC_VARS) {
    Errno = 11;
    Err_data = var_num;
  }

  byte i;
  for (i = 0; i < NUM_CHANNELS; i++) {
    Switch_closed_event[FIRST_CHANNEL_SWITCH + i] = CHANNEL_ON;
    Switch_opened_event[FIRST_CHANNEL_SWITCH + i] = CHANNEL_OFF;
    if (Lowest_channel == 0xFF && Switches[FIRST_CHANNEL_SWITCH + i].current) {
      channel_on(FIRST_CHANNEL_SWITCH + i);
    }
  } // end for (i)

  for (i = 0; i < NUM_HARMONICS; i++) {
    Switch_closed_event[FIRST_HARMONIC_SWITCH + i] = HARMONIC_ON;
    Switch_opened_event[FIRST_HARMONIC_SWITCH + i] = HARMONIC_OFF;
    if (Lowest_harmonic == 0xFF && Switches[FIRST_HARMONIC_SWITCH + i].current) {
      harmonic_on(FIRST_HARMONIC_SWITCH + i);
    }
  } // end for (i)

  reset_function_encoders();

  return 0; // for now...
}

// vim: sw=2
