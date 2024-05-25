// functions.ino

/**
#define NUM_CH_FUNCTIONS        8
#define NUM_HM_FUNCTIONS        7
#define NUM_FUNCTIONS           (NUM_CH_FUNCTIONS + NUM_HM_FUNCTIONS)
#define NUM_FUNCTION_ENCODERS   4
**/

byte EEPROM_functions_offset;

// 512 bytes
byte Channel_memory[NUM_CHANNELS][NUM_CH_FUNCTIONS][NUM_FUNCTION_ENCODERS];

// 4480 bytes
byte Harmonic_memory[NUM_CHANNELS][NUM_HARMONICS][NUM_HM_FUNCTIONS][NUM_FUNCTION_ENCODERS];

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
 
// number displayed is value*10^(dp+extra_10s) / scale + offset
// byte _min, byte _max, long _offset = 0, byte _bt_mul_down = 1, byte _dp = 0, long _scale = 1,
// byte extra_10s = 0, byte _trim = 0, byte _flags = 0
linear_number_t Cents_per_semitone(0, 20, 99858, 10, 2, 12, 0, 1);
linear_number_t Max_octave_fudge(0, 127, 0, 10, 1, 3);
linear_number_t Channel_num(1, 16, 0, 4);
linear_number_t Octave(0, 7);
linear_number_t Frequency_offset(0, 127, -318, 10, 1, 2);
linear_number_t Scale_3(0, 5);
linear_number_t Freq_ramp_start(0, 15, -400, 4, 0, 2, 2);
linear_number_t Attack_start(0, 15, -40, 4, 0, 3, 1);
linear_number_t Bend(0, 7, -100, 1, 0, 35, 3);
linear_number_t Cycle_time(0, 15, 5, 1, 2, 20);
linear_number_t Sine_ampl_swing(0, 127, 0, 10, 0, 3333, 4);
linear_number_t Sustain_ampl_swing(0, 7, 0, 1, 1, 7, 1);
linear_number_t Center_ampl(0, 15, 5, 4);
linear_number_t Ampl_offset(0, 127, 0, 10, 1, 1411, 3);
linear_number_t Freq_course(0, 127, 0, 10, 1, 10);
linear_number_t Freq_fine(0, 99, 0, 10);

// byte _max, float _limit, float _b, float _start = 0.0, byte _bt_mul_down = 1
// dur = exp(0.049957498*value + 1.5) - 4.48168907
// value = (log(dur + 4.48168907) - 1.5)/0.049957498
geometric_number_s Duration_5(15, 5.0, 1.5);
// dur = exp(0.068489354*value + 1) - 2.718281828
// value = (log(dur + 2.718281828) - 1)/0.068489354
geometric_number_s Duration_20(31, 20.0, 1.0);

// byte _num_notes, char **_notes, byte _include_null = 0, byte _flags = 0, byte _min = 0
const char *Note_list_12[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
note_t Notes_12(12, Note_list_12);
note_t Notes_12_null(12, Note_list_12, 1); // 12 is null
const char *Note_list_2[] = {"Eb", "Ab"};
note_t Notes_Eb_Ab_null(2, Note_list_2, 1, ENCODER_FLAGS_CYCLE);  // 2 is null


// {&var_type}
variable_t Functions[NUM_FUNCTIONS][NUM_FUNCTION_ENCODERS] = {
  {  // function 0: key_signature
    {&Flats_and_sharps, 7}, {&Disabled}, {&Disabled}, {&Major_minor},
  },
  {  // function 1: tune absolute
    {&Disabled}, {&Notes_12, 9}, {&Octave, 4}, {&Frequency_offset, 64},
  },
  {  // function 2: match tuning
    {&Disabled}, {&Notes_12, 9}, {&Channel_num, 1}, {&Disabled},
  },
  {  // function 3: equal_temperament
    {&Disabled}, {&Disabled}, {&Cents_per_semitone, 17}, {&Disabled},
  },
  {  // function 4: well_tempered
    {&Notes_12}, {&Notes_12_null, 12}, {&Disabled}, {&Disabled},
  },
  {  // function 5: meantone
    {&Notes_12}, {&Notes_Eb_Ab_null, 2}, {&Max_octave_fudge}, {&Meantone},
  },
  {  // function 6: just_intonation
    {&Notes_12}, {&Notes_12_null, 12}, {&Disabled}, {&Just_intonation},
  },
  {  // function 7: pythagorean
    {&Notes_12}, {&Disabled}, {&Max_octave_fudge}, {&Disabled},
  },
  {  // function 8: harmonic basics
    {&Ampl_offset, 4}, {&Freq_course}, {&Freq_fine}, {&Disabled},
  },
  {  // function 9: freq env: ramp
    {&Scale_3}, {&Freq_ramp_start}, {&Duration_5, 1}, {&Bend, 4},
  },
  {  // function 10: freq env: sine
    {&Scale_3}, {&Cycle_time, 3}, {&Sine_ampl_swing, 15}, {&Disabled},
  },
  {  // function 11: attack
    {&Scale_3}, {&Attack_start}, {&Duration_5, 1}, {&Bend, 4},
  },
  {  // function 12: decay
    {&Scale_3}, {&Disabled}, {&Duration_20, 1}, {&Bend, 4},
  },
  {  // function 13: sustain
    {&Scale_3}, {&Cycle_time, 7}, {&Sustain_ampl_swing, 4}, {&Center_ampl, 7},
  },
  {  // function 14: release
    {&Scale_3}, {&Disabled}, {&Duration_5, 3}, {&Bend, 4},
  },
};

byte Lowest_harmonic = 0xFF;  // 0-9, 0xFF when all switches off
unsigned short Harmonic_bitmap;
byte Lowest_channel = 0xFF;   // 0-15, 0xFF when all switches off
unsigned short Channel_bitmap;
byte Buffer[NUM_FUNCTION_ENCODERS];

void load_functions(byte skip_ch_functions) {
  // Loads Function.values from Function_memory and Harmonic_memory.
  // Called when the Lowest_channel or Lowest_harmonic changes.
  // Resets Functions.changed.
  // If current FUNCTION changed, updates displays.

  if (Lowest_channel != 0xFF) {
    byte fun, enc;
    if (!skip_ch_functions) {
      for (fun = 0; fun < NUM_CH_FUNCTIONS; fun++) {
        for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
          Functions[fun][enc].value = Channel_memory[Lowest_channel][fun][enc];
          Functions[fun][enc].changed = 0;
        }
      } // end for (fun)
    } // end if (!skip_ch_functions)
    if (Lowest_harmonic != 0xFF) {
      for (fun = NUM_CH_FUNCTIONS; fun < NUM_FUNCTIONS; fun++) {
        for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
          Functions[fun][enc].value = 
            Harmonic_memory[Lowest_channel][Lowest_harmonic][fun - NUM_CH_FUNCTIONS][enc];
          Functions[fun][enc].changed = 0;
        }
      } // end for (fun)
    } // end if (Lowest_harmonic set)
    if (!skip_ch_functions || FUNCTION >= NUM_CH_FUNCTIONS) {
      update_displays();
    }
  } // end if (Lowest_channel set)
}

void load_encoders(void) {
  // Loads Encoders from Functions.
  // Called when the FUNCTION changes.
  // Does not reset Functions.changed.
  // Clears displays prior to update.
  // Does not update displays afterwards.

  clear_displays();

  for (byte enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
    Encoders[enc].var = &Functions[FUNCTION][enc];
  } // end for (enc)
}

void clear_displays(void) {
  // Clears all Encoder displays.

  for (byte enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
    variable_t *var = Encoders[enc].var;
    if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {
      if (var->var_type->flags & ENCODER_FLAGS_CHOICE_LEDS) {
        turn_off_choices_leds(enc);
      } else {
        clear_numeric_display(enc);  // display_num == encoder number
      }
    }
  } // end for (enc)
}

void update_displays(void) {
  // Updates displays to match Encoder values.

  for (byte enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
    variable_t *var = Encoders[enc].var;
    if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {
      run_event(var->var_type->display_value, enc); // display value
    }
  } // end for (enc)
}

void send_functions_to_synth(void) {
  // Sends Functions to synth and save Functions in Function_memory and Harmonic_memory.
  // Called when send is triggered.
  // Resets Functions.changed.

  if (Lowest_channel != 0xFF) {
    byte fun, enc;
    for (byte ch = 0; ch < NUM_CHANNELS; ch++) {
      if (Channel_bitmap & (1 << ch)) {
        for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
          for (fun = 0; fun < NUM_CH_FUNCTIONS; fun++) {
            if (Functions[fun][enc].changed) {
              // FIX: Send value to synth
              Channel_memory[ch][fun][enc] = Functions[fun][enc].value;
            }
          }
          if (Lowest_harmonic != 0xFF) {
            for (byte hm = 0; hm < NUM_HARMONICS; hm++) {
              if (Harmonic_bitmap & (1 << hm)) {
                for (fun = NUM_CH_FUNCTIONS; fun < NUM_FUNCTIONS; fun++) {
                  if (Functions[fun][enc].changed) {
                    // FIX: Send value to synth
                    Harmonic_memory[ch][hm][fun - NUM_CH_FUNCTIONS][enc] = Functions[fun][enc].value;
                  }
                } // end for (fun)
              } // end if (harmonic on)
            } // end for (hm)
          } // end if (Lowest_harmonic set)
        } // end for (enc)
      } // end if (channel on)
    } // end for (ch)

    // Reset Functions.changed.  Doesn't matter if haramonic Functions.changed reset if no harmonic
    // selected, because those Functions will be reloaded anyway when a harmonic switch is set.
    for (fun = 0; fun < NUM_FUNCTIONS; fun++) {
      for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
        Functions[fun][enc].changed = 0;
      }
    }
  } // end if (Lowest_channel set)
} // end send_functions_to_synth

void harmonic_on(byte sw) {
  byte hm = sw - FIRST_HARMONIC_SWITCH;
  Harmonic_bitmap |= 1 << hm;
  if (hm < Lowest_harmonic) {
    if (Lowest_harmonic == 0xFF) {
      Encoders[FUNCTION_ENCODER].var->var_type->max = NUM_FUNCTIONS - 1;
    }
    Lowest_harmonic = hm;
    load_functions(1);
  }
}

void truncate_function() {
  // Reduces FUNCTION_ENCODER max value.
  // Checks FUNCTION to see if it's a harmonic function.
  // If so, changes it to last non-harmonic function.
  // If FUNCTION changed, clears previous encoder displays.
  // Updates new encoder displays.
  variable_t *var = Encoders[FUNCTION_ENCODER].var;
  var->var_type->max = NUM_CH_FUNCTIONS - 1;
  if (var->value > var->var_type->max) { // harmonic function
    var->value = var->var_type->max;
    run_event(var->var_type->display_value, FUNCTION_ENCODER); // update FUNCTION display
    load_encoders();
    update_displays();
  }
}

void harmonic_off(byte sw) {
  byte hm = sw - FIRST_HARMONIC_SWITCH;
  Harmonic_bitmap &= ~(1 << hm);
  if (hm == Lowest_harmonic) {
    Lowest_harmonic = 0xFF;
    byte i;
    for (i = hm + 1; i < NUM_HARMONICS; i++) {
      if (Switches[FIRST_HARMONIC_SWITCH + i].current) {
        Lowest_harmonic = i;
        break;
      }
    } // end for (i)
    if (Lowest_harmonic == 0xFF) {
      // No more harmonics selected...
      truncate_function();
    } else {
      load_functions(1);
    }
  } // end if (Lowest_harmonic)
}

void channel_on(byte sw) {
  byte ch = sw - FIRST_CHANNEL_SWITCH;
  Channel_bitmap |= 1 << ch;
  if (ch < Lowest_channel) {
    byte old_ch = Lowest_channel;
    Lowest_channel = ch;
    if (old_ch == 0xFF) {
      // turn function encoder back on
      variable_t *var = Encoders[FUNCTION_ENCODER].var;
      var->var_type->flags &= ~ENCODER_FLAGS_DISABLED;  // enable FUNCTION_ENCODER
      if (Lowest_harmonic == 0xFF) {
        var->var_type->max = NUM_CH_FUNCTIONS - 1;
        if (var->value > var->var_type->max) var->value = var->var_type->max;
      }
      run_event(var->var_type->display_value, FUNCTION_ENCODER);
      load_encoders();
    } // end if (old_ch == 0xFF)
    load_functions();
  } // end if (ch < Lowest_channel)
}

void channel_off(byte sw) {
  byte ch = sw - FIRST_CHANNEL_SWITCH;
  Channel_bitmap &= ~(1 << ch);
  if (ch == Lowest_channel) {
    Lowest_channel = 0xFF;
    byte i;
    for (i = ch + 1; i < NUM_CHANNELS; i++) {
      if (Switches[FIRST_CHANNEL_SWITCH + i].current) {
        Lowest_channel = i;
        break;
      }
    } // end for (i)
    if (Lowest_channel == 0xFF) {
      Encoders[FUNCTION_ENCODER].var->var_type->flags |= ENCODER_FLAGS_DISABLED;
      turn_off_choices_leds(FUNCTION_ENCODER);
      clear_displays();
    } else {
      load_functions();
    }
  } // end if (Lowest_channel)
}

void reset_function_encoders(void) {
  // Called when FUNCTION changes.
  // Won't get called if FUNCTION_ENCODER is disabled.
  load_encoders();
  update_displays();
}

byte setup_functions(byte EEPROM_offset) {
  // Returns num EEPROM bytes needed.
  EEPROM_functions_offset = EEPROM_offset;

  byte ch, hm, fun, enc;

  // Copy initial Functions values to all channels and harmonics in Channel_memory and Harmonic_memory.
  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
      for (fun = 0; fun < NUM_CH_FUNCTIONS; fun++) {
        Channel_memory[ch][fun][enc] = Functions[fun][enc].value;
      }
      for (fun = NUM_CH_FUNCTIONS; fun < NUM_HM_FUNCTIONS; fun++) {
        for (hm = 0; hm < NUM_HARMONICS; hm++) {
          Harmonic_memory[ch][hm][fun][enc] = Functions[fun][enc].value;
        }
      }
    } // end for (enc)
  } // end for (ch)

  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    Switch_closed_event[FIRST_CHANNEL_SWITCH + ch] = CHANNEL_ON;
    Switch_opened_event[FIRST_CHANNEL_SWITCH + ch] = CHANNEL_OFF;
    //if (Switches[FIRST_CHANNEL_SWITCH + ch].current) {
    //  channel_on(FIRST_CHANNEL_SWITCH + ch);
    //}
  } // end for (ch)

  for (hm = 0; hm < NUM_HARMONICS; hm++) {
    Switch_closed_event[FIRST_HARMONIC_SWITCH + hm] = HARMONIC_ON;
    Switch_opened_event[FIRST_HARMONIC_SWITCH + hm] = HARMONIC_OFF;
    //if (Switches[FIRST_HARMONIC_SWITCH + hm].current) {
    //  harmonic_on(FIRST_HARMONIC_SWITCH + hm);
    //}
  } // end for (hm)

  return 0; // for now...
}

// vim: sw=2
