// functions.ino

/**
#define NUM_CH_FUNCTIONS        8
#define NUM_HM_FUNCTIONS        7
#define NUM_FUNCTIONS           (NUM_CH_FUNCTIONS + NUM_HM_FUNCTIONS)
#define NUM_FUNCTION_ENCODERS   4
**/

byte EEPROM_functions_offset;

// Each variable_t.var_type has {min, max, flags, bt_mul[button_up], bt_mul[button_down], param_num}
// 
// flags are:
//
//    ENCODER_FLAGS_DISABLED      0b001
//    ENCODER_FLAGS_CYCLE         0b010
//    ENCODER_FLAGS_CHOICE_LEDS   0b100

// byte _choices_num, byte _choices_length, byte _bt_mul_down = 1, byte _additional_flags = 0
choices_t Major_minor(1, 2, 1, ENCODER_FLAGS_CYCLE);
choices_t Meantone(2, 2, 1, ENCODER_FLAGS_CYCLE);
choices_t Just_intonation(3, 14, 4);

sharps_flats_t Flats_and_sharps;
 
// byte _min, byte _max, long _offset = 0, byte _bt_mul_down = 1, byte _dp = 0, long _scale = 1,
// byte _trim = 0, byte _flags = 0
linear_number_t Cents_per_semitone(0, 20, 99858, 10, 2, 12, 1);
linear_number_t Max_octave_fudge(0, 127, 0, 10);  // What does "steps_per_value = 3" mean??

// byte _num_notes, char **_notes, byte _include_null = 0, byte _flags = 0, byte _min = 0
const char *Note_list_12[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
note_t Notes_12(12, Note_list_12);
note_t Notes_12_null(12, Note_list_12, 1);
const char *Note_list_2[] = {"Eb", "Ab"};
note_t Notes_Eb_Ab_null(2, Note_list_2, 1, ENCODER_FLAGS_CYCLE);


// {&var_type}
variable_t Functions[NUM_FUNCTIONS][NUM_FUNCTION_ENCODERS] = {
  {  // function 0: key_signature
    {&Flats_and_sharps}, {&Disabled}, {&Disabled}, {&Major_minor},
  },
  {  // function 1: tune absolute
    {&Disabled}, {&Flats_and_sharps/* 0, 11, 0b011, 1, 1 */}, {&Flats_and_sharps/* 0, 7, 0b001, 1, 1 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */},
  },
  {  // function 2: match tuning
    {&Disabled}, {&Flats_and_sharps/* 0, 11, 0b011, 1, 1 */}, {&Flats_and_sharps/* 0, 15, 0b011, 1, 5 */}, {&Disabled},
  },
  {  // function 3: equal_temperament
    {&Disabled}, {&Disabled}, {&Cents_per_semitone}, {&Disabled},
  },
  {  // function 4: well_tempered
    {&Notes_12}, {&Notes_12_null}, {&Disabled}, {&Disabled},
  },
  {  // function 5: meantone
    {&Notes_12}, {&Notes_Eb_Ab_null}, {&Max_octave_fudge}, {&Meantone},
  },
  {  // function 6: just_intonation
    {&Notes_12}, {&Notes_12_null}, {&Disabled}, {&Just_intonation},
  },
  {  // function 7: pythagorean
    {&Notes_12}, {&Disabled}, {&Max_octave_fudge}, {&Disabled},
  },
  {  // function 8: harmonic basics
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b011, 1, 10 */}, {&Disabled},
  },
  {  // function 9: freq env: ramp
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */},
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */},
  },
  {  // function 10: freq env: sine
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Disabled},
  },
  {  // function 11: attack
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */},
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */},
  },
  {  // function 12: decay
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Disabled}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */},
  },
  {  // function 13: sustain
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */},
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */},
  },
  {  // function 14: release
    {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Disabled}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */}, {&Flats_and_sharps/* 0, 127, 0b001, 1, 10 */},
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
          variable_t *var = Encoders[i].var;
          if (var != NULL && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {
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
          variable_t *var = Encoders[i].var;
          if (var != NULL && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {
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
      variable_t *var = Encoders[i].var;
      if (var != NULL && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) req[6 + i] = var->value;
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
      variable_t *var = Encoders[i].var;
      if (var != NULL && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) req[8 + i] = var->value;
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
  // called when FUNCTION changes
  byte i;
  for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
    if (Encoders[i].var && (Encoders[i].var->var_type->flags & ENCODER_FLAGS_CHOICE_LEDS)) {
      turn_off_choices_leds(i);
    } else {
      clear_numeric_display(i);  // display_num == encoder number
    }
    Encoders[i].var = &Functions[FUNCTION][i];
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
    //if (Switches[FIRST_CHANNEL_SWITCH + i].current) {
    //  channel_on(FIRST_CHANNEL_SWITCH + i);
    //}
  } // end for (i)

  for (i = 0; i < NUM_HARMONICS; i++) {
    Switch_closed_event[FIRST_HARMONIC_SWITCH + i] = HARMONIC_ON;
    Switch_opened_event[FIRST_HARMONIC_SWITCH + i] = HARMONIC_OFF;
    //if (Switches[FIRST_HARMONIC_SWITCH + i].current) {
    //  harmonic_on(FIRST_HARMONIC_SWITCH + i);
    //}
  } // end for (i)

  reset_function_encoders();

  return 0; // for now...
}

// vim: sw=2
