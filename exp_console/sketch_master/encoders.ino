// encoders.ino

byte EEPROM_encoder_offset;

encoder_t Encoders[NUM_ENCODERS];

encoder_var_t Filename_var = {0, 1, 0b11, 1, 10};
encoder_var_t Function_var[2] = {  // Synth, Program
  {0, 14, 0b11, 1, 5},
  {0, 14, 0b11, 1, 5}
};

byte setup_encoders(byte EEPROM_offset) {
  EEPROM_encoder_offset = EEPROM_offset;

  // Function
  Encoders[FUNCTION_ENCODER].A_sw = SWITCH_NUM(2, 0);
  Encoders[FUNCTION_ENCODER].var = &Function_var[SAVE_OR_SYNTH];

  // Function Params
  Encoders[1].A_sw = SWITCH_NUM(2, 3);
  Encoders[1].display_num = 0;
  Encoders[1].var = NULL;
  Encoders[2].A_sw = SWITCH_NUM(2, 6);
  Encoders[1].display_num = 1;
  Encoders[2].var = NULL;
  Encoders[3].A_sw = SWITCH_NUM(3, 0);
  Encoders[1].display_num = 2;
  Encoders[3].var = NULL;
  Encoders[4].A_sw = SWITCH_NUM(3, 3);
  Encoders[1].display_num = 3;
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
