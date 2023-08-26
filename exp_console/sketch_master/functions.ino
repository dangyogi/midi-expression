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
unsigned short Harmonic_bitmap;
byte Lowest_channel = 0xFF;   // 0-15, 0xFF when all switches off
unsigned short Channel_bitmap;
byte Buffer[NUM_FUNCTION_ENCODERS];

void load_function_encoder_values(void) {
  if (Lowest_channel != 0xFF) {
    byte fun = FUNCTION;
    byte req[6];
    byte bytes_sent, bytes_received;

    if (fun < NUM_CH_FUNCTIONS) {
      // load channel values
      req[0] = '$';
      req[1] = 'c';  // get_ch_values
      req[2] = SAVE_OR_SYNTH;
      req[3] = Lowest_channel;
      req[4] = fun;
      bytes_sent = Serial.write(req, 5);
      if (bytes_sent != 5) {
        Errno = 1;
        Err_data = bytes_sent;
        return;
      }

      bytes_received = Serial.readBytes((char *)Buffer, NUM_FUNCTION_ENCODERS);
      if (bytes_received != NUM_FUNCTION_ENCODERS) {
        Errno = 2;
        Err_data = bytes_received;
        return;
      } else {
        byte i;
        for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
          encoder_var_t *var = Encoders[Function_encoders[i]].var;
          if (var != NULL && (var->flags & 1)) {
            var->value = Buffer[i];
          } // end if (encoder enabled)
        } // end for (i)
      } // end if received whole response
    } else if (Lowest_harmonic != 0xFF) {
      // load harmonic values
      req[0] = '$';
      req[1] = 'h';  // get_hm_values
      req[2] = SAVE_OR_SYNTH;
      req[3] = Lowest_channel;
      req[4] = Lowest_harmonic;
      req[5] = fun;
      bytes_sent = Serial.write(req, 6);
      if (bytes_sent != 6) {
        Errno = 3;
        Err_data = bytes_sent;
        return;
      }

      bytes_received = Serial.readBytes((char *)Buffer, NUM_FUNCTION_ENCODERS);
      if (bytes_received != NUM_FUNCTION_ENCODERS) {
        Errno = 4;
        Err_data = bytes_received;
        return;
      } else {
        byte i;
        for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
          encoder_var_t *var = Encoders[Function_encoders[i]].var;
          if (var != NULL && (var->flags & 1)) {
            var->value = Buffer[i];
          } // end if (encoder enabled)
        } // end for (i)
      } // end if received whole response
    } // end if fun or hm
  } // end if (Lowest_channel set)
}

void save_function_encoder_values(void) {
  byte fun = FUNCTION;
  byte i;
  byte req[8 + NUM_FUNCTION_ENCODERS];
  byte bytes_sent;

  if (fun < NUM_CH_FUNCTIONS) {
    // save channel values
    req[0] = '$';
    req[1] = 'C';  // set_ch_values
    req[2] = SAVE_OR_SYNTH;
    req[3] = (byte)(Channel_bitmap >> 8);
    req[4] = (byte)Channel_bitmap;
    req[5] = fun;
    for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
      encoder_var_t *var = Encoders[Function_encoders[i]].var;
      if (var != NULL && (var->flags & 1)) req[6 + i] = var->value;
      else req[6 + i] = 0;
    }
    bytes_sent = Serial.write(req, 6 + NUM_FUNCTION_ENCODERS);
    if (bytes_sent != 6 + NUM_FUNCTION_ENCODERS) {
      Errno = 5;
      Err_data = bytes_sent;
      return;
    }
  } else {
    // save harmonic values
    req[0] = '$';
    req[1] = 'H';  // set_hm_values
    req[2] = SAVE_OR_SYNTH;
    req[3] = (byte)(Channel_bitmap >> 8);
    req[4] = (byte)Channel_bitmap;
    req[5] = (byte)(Harmonic_bitmap >> 8);
    req[6] = (byte)Harmonic_bitmap;
    req[7] = fun;
    for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
      encoder_var_t *var = Encoders[Function_encoders[i]].var;
      if (var != NULL && (var->flags & 1)) req[8 + i] = var->value;
      else req[8 + i] = 0;
    }
    bytes_sent = Serial.write(req, 7 + NUM_FUNCTION_ENCODERS);
    if (bytes_sent != 7 + NUM_FUNCTION_ENCODERS) {
      Errno = 6;
      Err_data = bytes_sent;
      return;
    }
  } // end if fun or hm
}

void harmonic_on(byte sw) {
  byte hm = sw - FIRST_HARMONIC_SWITCH;
  Harmonic_bitmap |= 1 << hm;
  if (hm < Lowest_harmonic) {
    Lowest_harmonic = hm;
    load_function_encoder_values();
  }
}

void harmonic_off(byte sw) {
  byte hm = sw - FIRST_HARMONIC_SWITCH;
  Harmonic_bitmap &= ~(1 << hm);
  if (hm == Lowest_harmonic) {
    byte i;
    for (i = hm + 1; i < NUM_HARMONICS; i++) {
      if (Switches[FIRST_HARMONIC_SWITCH + i].current) {
        Lowest_harmonic = i;
        load_function_encoder_values();
        return;
      }
    } // end for (i)
    // No harmonic switches on
    Lowest_harmonic = 0xFF;
    load_function_encoder_values();
  } // end if (Lowest_harmonic)
}

void channel_on(byte sw) {
  byte ch = sw - FIRST_CHANNEL_SWITCH;
  Channel_bitmap |= 1 << ch;
  if (ch < Lowest_channel) {
    Lowest_channel = ch;
    load_function_encoder_values();
  }
}

void channel_off(byte sw) {
  byte ch = sw - FIRST_CHANNEL_SWITCH;
  Channel_bitmap &= ~(1 << ch);
  if (ch == Lowest_channel) {
    byte i;
    for (i = ch + 1; i < NUM_CHANNELS; i++) {
      if (Switches[FIRST_CHANNEL_SWITCH + i].current) {
        Lowest_channel = i;
        load_function_encoder_values();
        return;
      }
    } // end for (i)
    // No channel switches on
    Lowest_channel = 0xFF;
    load_function_encoder_values();
  } // end if (Lowest_channel)
}

void reset_function_encoders(void) {
  // called with either SYNTH_OR_PROG or FUNCTION changes
  Encoders[FUNCTION_ENCODER].var = &Function_var[SAVE_OR_SYNTH];
  byte i;
  for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
    Encoders[Function_encoders[i]].var = &Functions[FUNCTION][i];
  } // end for (i)
  load_function_encoder_values();
}

byte setup_functions(byte EEPROM_offset) {
  // Returns num EEPROM bytes needed.
  EEPROM_functions_offset = EEPROM_offset;

  byte i;
  for (i = 0; i < NUM_CHANNELS; i++) {
    Switch_closed_event[FIRST_CHANNEL_SWITCH + i] = CHANNEL_ON;
    Switch_opened_event[FIRST_CHANNEL_SWITCH + i] = CHANNEL_OFF;
    if (Switches[FIRST_CHANNEL_SWITCH + i].current) {
      channel_on(FIRST_CHANNEL_SWITCH + i);
    }
  } // end for (i)

  for (i = 0; i < NUM_HARMONICS; i++) {
    Switch_closed_event[FIRST_HARMONIC_SWITCH + i] = HARMONIC_ON;
    Switch_opened_event[FIRST_HARMONIC_SWITCH + i] = HARMONIC_OFF;
    if (Switches[FIRST_HARMONIC_SWITCH + i].current) {
      harmonic_on(FIRST_HARMONIC_SWITCH + i);
    }
  } // end for (i)

  reset_function_encoders();

  return 0; // for now...
}

// vim: sw=2
