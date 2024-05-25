// encoders.ino

byte EEPROM_encoder_offset;

encoder_t Encoders[NUM_ENCODERS];

// byte _max, byte _display_value = 0xFF, byte _flags = 0, byte _bt_mul_down = 1, byte _min = 0,
// byte _bt_mul_up = 1
var_type_t Disabled(0, 0xFF, ENCODER_FLAGS_DISABLED);


void send_LED_request(byte *msg, byte length) {
  sendRequest(I2C_LED_CONTROLLER, msg, length);
  delayMicroseconds(200);
  getResponse(I2C_LED_CONTROLLER, 2, 1);
}

void select_led(byte enc) {
  variable_t *var = Encoders[enc].var;
  if (Trace_encoders) {
    Serial.print("select_led "); Serial.println(var->value);
  }
  byte msg[3] = {30, ((choices_t *)var->var_type)->choices_num, var->value};
  send_LED_request(msg, 3);
}

void turn_off_choices_leds(byte enc) {
  variable_t *var = Encoders[enc].var;
  if (Trace_encoders) {
    Serial.print("turn_off_choices_leds "); Serial.println(((choices_t *)var->var_type)->choices_num);
  }
  byte msg[2] = {29, ((choices_t *)var->var_type)->choices_num};
  send_LED_request(msg, 2);
} 

void display_digit(byte display_num, byte digit_num, byte num, byte dp) {
  if (Trace_encoders) {
    Serial.print("display_digit "); Serial.print(digit_num);
    Serial.print(", num "); Serial.print(num); Serial.print(", dp "); Serial.println(dp);
  }
  // 18<disp><digit#><value><dp>
  byte msg[5] = {18, display_num, digit_num, num, dp};
  send_LED_request(msg, 5);
}

void display_sharp_flat(byte display_num, byte sharp_flat) {
  // sharp_flat: 0 = nat (blank), 1 = SH, 2 = FL
  if (Trace_encoders) {
    Serial.print("display_sharp_flat "); Serial.println(sharp_flat);
  }
  // 21<disp><sharp_flat>: load sharp_flat
  byte msg[3] = {21, display_num, sharp_flat};
  send_LED_request(msg, 3);
}

void display_sharps_flats(byte enc) {
  variable_t *var = Encoders[enc].var;
  signed char n = var->value - 7;
  display_digit(enc, 0, abs(n), 0);
  if (n == 0) display_sharp_flat(enc, 0);     // natural
  else if (n < 0) display_sharp_flat(enc, 2); // flat
  else display_sharp_flat(enc, 1);            // sharp
}

void display_number(byte display_num, short num, byte dp) {
  if (Trace_encoders) {
    Serial.print("display_number "); Serial.print(num); Serial.print(", dp "); Serial.println(dp);
  }
  // 19<disp><value_s16><dec_place>
  byte msg[5] = {19, display_num, byte(num >> 8), byte(num & 0xFF), dp};
  send_LED_request(msg, 5);
}

void clear_numeric_display(byte display_num) {
  if (Trace_encoders) {
    Serial.print("clear_numeric_display "); Serial.println(display_num);
  }
  byte msg[2] = {31, display_num};
  send_LED_request(msg, 2);
}

const short Powers_of_ten[] = {1, 10, 100, 1000, 10000};

void display_linear_number(byte enc) {
  // number displayed is value*10^dp / scale + offset
  variable_t *var = Encoders[enc].var;
  linear_number_t *var_type = (linear_number_t *)var->var_type;
  long n = var->value;
  n *= Powers_of_ten[var_type->dp + var_type->extra_10s];
  n = (n + var_type->scale / 2) / var_type->scale;  // divide rounded
  n += var_type->offset;
  signed char dp = var_type->dp;
  if (var_type->trim) {
    if (n < -99) n = -(-n % 100);
    else if (n > 999) n = n % 1000;
  } else while (n < -99 || n > 999) {
    n /= 10;
    dp -= 1;
  }
  display_number(enc, short(n), max(0, dp));
}

void display_geometric_number(byte enc) {
  variable_t *var = Encoders[enc].var;
  geometric_number_t *var_type = (geometric_number_t *)var->var_type;
  float n = exp(var_type->m * var->value + var_type->b) + var_type->c;
  if (abs(n) < 0.005) display_number(enc, 0, 0);   // display as 0
  short s;
  if (n < -9.95) {
    s = (short)(n - 0.5);
    display_number(enc, s, 0);
  } else if (n < -0.995) {
    s = (short)(n * 10.0 - 0.5);
    display_number(enc, s, 1);
  } else if (n < -0.005) {
    s = (short)(n * 100.0 - 0.5);
    display_number(enc, s, 2);
  } else if (n > 99.95) {
    s = (short)(n + 0.5);
    display_number(enc, s, 0);
  } else if (n > 9.995) {
    s = (short)(n * 10.0 + 0.05);
    display_number(enc, s, 1);
  } else {
    s = (short)(n * 100.0 + 0.005);
    display_number(enc, s, 2);
  }
}

void display_a_note(byte display_num, byte note, byte sharp_flat) {
  // note: 0-6 = A-G, sharp_flat: 0 = nat (blank), 1 = SH, 2 = FL
  if (Trace_encoders) {
    Serial.print("display_note "); Serial.print(note);
    Serial.print(", sharp_flat "); Serial.println(sharp_flat);
  }
  // 20<disp><note><sharp_flat>: load note
  byte msg[4] = {20, display_num, note, sharp_flat};
  send_LED_request(msg, 4);
}

void display_note(byte enc) {
  variable_t *var = Encoders[enc].var;
  note_t *var_type = (note_t *)var->var_type;
  byte value = var->value;
  if (var_type->include_null && value == var_type->num_notes) { // NULL
    clear_numeric_display(enc);
  } else {
    const char *note = var_type->notes[value];
    if (note[1] == 0) {  // no sharp/flat
      display_a_note(enc, note[0] - 'A', 0);
    } else if (note[1] == '#') {
      display_a_note(enc, note[0] - 'A', 1);
    } else if (note[1] == 'b') {
      display_a_note(enc, note[0] - 'A', 2);
    } else {
      Errno = 31;
      Err_data = note[1];
    }
  }
}

 
// starts disabled until a ch sw turned on
// byte _choices_num, byte _choices_length, byte _bt_mul_down = 1, byte _additional_flags = 0
choices_t Function_var_type(0, NUM_FUNCTIONS, 4, ENCODER_FLAGS_DISABLED);

// static variables
variable_t Filename_var = {&Disabled};
variable_t Function_var = {&Function_var_type};

void set_debounce(byte A_sw) {
  Switches[A_sw].debounce_index = Switches[A_sw + 1].debounce_index = 1;
}

byte setup_encoders(byte EEPROM_offset) {
  EEPROM_encoder_offset = EEPROM_offset;

  // Function Params
  Encoders[0].A_sw = SWITCH_NUM(2, 3);
  Encoders[0].display_num = 0;
  Encoders[0].var = 0;
  set_debounce(Encoders[0].A_sw);
  Encoders[1].A_sw = SWITCH_NUM(2, 6);
  Encoders[1].display_num = 1;
  Encoders[1].var = 0;
  set_debounce(Encoders[1].A_sw);
  Encoders[2].A_sw = SWITCH_NUM(3, 0);
  Encoders[2].display_num = 2;
  Encoders[2].var = 0;
  set_debounce(Encoders[2].A_sw);
  Encoders[3].A_sw = SWITCH_NUM(3, 3);
  Encoders[3].display_num = 3;
  Encoders[3].var = 0;
  set_debounce(Encoders[3].A_sw);

  // Function
  Encoders[FUNCTION_ENCODER].A_sw = SWITCH_NUM(2, 0);
  Encoders[FUNCTION_ENCODER].display_num = 176;  // first LED #
  Encoders[FUNCTION_ENCODER].var = &Function_var;
  Encoders[FUNCTION_ENCODER].encoder_event = FUNCTION_CHANGED;
  set_debounce(Encoders[FUNCTION_ENCODER].A_sw);

  // Filename
  Encoders[FILENAME_ENCODER].A_sw = SWITCH_NUM(0, 5);
  Encoders[FILENAME_ENCODER].var = &Filename_var;
  Encoders[FILENAME_ENCODER].encoder_event = 0xFF;  // FIX: figure this out later
  set_debounce(Encoders[FILENAME_ENCODER].A_sw);

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
