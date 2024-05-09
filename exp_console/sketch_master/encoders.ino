// encoders.ino

byte EEPROM_encoder_offset;

encoder_t Encoders[NUM_ENCODERS];


// min, max, flags, bt_mul[2] (up, down)
encoder_var_t Filename_var = {0, 1, ENCODER_FLAGS_ENABLED | ENCODER_FLAGS_CYCLE, 1, 7};
encoder_var_t Function_var[2] = {  // Synth, Program
  {0, 14, ENCODER_FLAGS_ENABLED | ENCODER_FLAGS_CYCLE | ENCODER_FLAGS_CHOICE_LEDS, 1, 4},
  {0, 14, ENCODER_FLAGS_ENABLED | ENCODER_FLAGS_CYCLE | ENCODER_FLAGS_CHOICE_LEDS, 1, 4}
};

byte setup_encoders(byte EEPROM_offset) {
  EEPROM_encoder_offset = EEPROM_offset;

  // Function
  Encoders[FUNCTION_ENCODER].A_sw = SWITCH_NUM(2, 0);
  Encoders[FUNCTION_ENCODER].display_num = 176;  // first LED #
  Encoders[FUNCTION_ENCODER].var = &Function_var[SAVE_OR_SYNTH];

  // Function Params
  Encoders[1].A_sw = SWITCH_NUM(2, 3);
  Encoders[1].display_num = 0;
  Encoders[1].var = NULL;
  Encoders[2].A_sw = SWITCH_NUM(2, 6);
  Encoders[2].display_num = 1;
  Encoders[2].var = NULL;
  Encoders[3].A_sw = SWITCH_NUM(3, 0);
  Encoders[3].display_num = 2;
  Encoders[3].var = NULL;
  Encoders[4].A_sw = SWITCH_NUM(3, 3);
  Encoders[4].display_num = 3;
  Encoders[4].var = NULL;

  // Filename
  Encoders[FILENAME_ENCODER].A_sw = SWITCH_NUM(0, 5);
  Encoders[FILENAME_ENCODER].var = &Filename_var;

  byte i;
  for (i = 0; i < NUM_ENCODERS; i++) {
    Switch_closed_event[Encoders[i].A_sw]     = ENC_A_CLOSED(i);   // switch closed ch A
    Switch_closed_event[Encoders[i].A_sw + 1] = ENC_B_CLOSED(i);   // switch closed ch B
    Switch_opened_event[Encoders[i].A_sw]     = ENC_A_OPENED(i);   // switch opened ch A
    Switch_opened_event[Encoders[i].A_sw + 1] = ENC_B_OPENED(i);   // switch opened ch B
  } // for (i)

  return 0;  // for now...
}

// vim: sw=2
