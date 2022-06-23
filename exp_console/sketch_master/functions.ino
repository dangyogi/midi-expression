// functions.ino

#define NUM_CH_FUNCTIONS        8
#define NUM_HM_FUNCTIONS        7
#define NUM_FUNCTIONS           (NUM_CH_FUNCTIONS + NUM_HM_FUNCTIONS)
#define NUM_FUNCTION_ENCODERS   4

byte EEPROM_functions_offset;

byte Function_encoders[] = {1, 2, 3, 4};

encoder_var_t Functions[NUM_FUNCTIONS][NUM_FUNCTION_ENCODERS] = {
  {  // function 0: key_signature
    {-7, 7, 0b01, 1, 1}, {0, 0, 0b00}, {0, 0, 0b00}, {0, 1, 0b11, 1, 1},
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

void reset_function_encoders(void) {
  byte i;
  for (i = 0; i < NUM_FUNCTION_ENCODERS; i++) {
    Encoders[Function_encoders[i]].var = &Functions[Encoders[FUNCTION_ENCODER].var->value][i];
  } // end for (i)
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

  // assign var_nums to HM FUNS
  var_num = 0;
  for (fun = NUM_CH_FUNCTIONS; fun < NUM_FUNCTIONS; fun++) {
    for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
      if (Functions[fun][enc].flags & 1) { // enabled
        Functions[fun][enc].var_num = var_num++;
      } // end if (enabled)
    } // end for (enc)
  } // end for (fun)

  return 0; // for now...
}

// vim: sw=2
