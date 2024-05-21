// encoders.ino

byte EEPROM_encoder_offset;

encoder_t Encoders[NUM_ENCODERS];

// byte _max, byte _display_value = 0xFF, byte _flags = 0, byte _bt_mul_down = 1, byte _min = 0,
// byte _bt_mul_up = 1
var_type_t Disabled(0, 0xFF, ENCODER_FLAGS_DISABLED);


void select_led(byte enc) {
  variable_t *var = Encoders[enc].var;
  if (Trace_encoders) {
    Serial.print("select_led "); Serial.println(var->value);
  }
  byte msg[3] = {30, ((choices_t *)var->var_type)->choices_num, var->value};
  sendRequest(I2C_LED_CONTROLLER, msg, 3);
  // getResponse(I2C_LED_CONTROLLER, 2, 1);
}

void turn_off_choices_leds(byte enc) {
  variable_t *var = Encoders[enc].var;
  if (Trace_encoders) {
    Serial.print("turn_off_choices_leds "); Serial.println(((choices_t *)var->var_type)->choices_num);
  }
  byte msg[2] = {29, ((choices_t *)var->var_type)->choices_num};
  sendRequest(I2C_LED_CONTROLLER, msg, 2);
  // getResponse(I2C_LED_CONTROLLER, 2, 1);
} 

void display_number(byte display_num, unsigned short num, byte dp) {
  if (Trace_encoders) {
    Serial.print("display_number "); Serial.print(num); Serial.print(", dp "); Serial.println(dp);
  }
  // 19<disp><value_s16><dec_place>
  byte msg[5] = {19, display_num, byte(num << 8), byte(num & 0xFF), dp};
  sendRequest(I2C_LED_CONTROLLER, msg, 5);
  // getResponse(I2C_LED_CONTROLLER, 2, 1);

}

void clear_numeric_display(byte display_num) {
  if (Trace_encoders) {
    Serial.print("clear_numeric_display "); Serial.println(display_num);
  }
  byte msg[2] = {31, display_num};
  sendRequest(I2C_LED_CONTROLLER, msg, 2);
  // getResponse(I2C_LED_CONTROLLER, 2, 1);
}

const short Powers_of_ten[] = {0, 10, 100, 1000, 10000};

void display_linear_number(byte enc) {
  variable_t *var = Encoders[enc].var;
  linear_number_t *var_type = (linear_number_t *)var->var_type;
  long n = var->value;
  n *= Powers_of_ten[var_type->dp];
  n = (n + var_type->scale / 2) / var_type->scale;  // divide rounded
  n += var_type->offset;
  if (var_type->trim) {
    if (n < -99) n = -(-n % 100);
    else if (n > 999) n = n % 1000;
  }
  display_number(enc, (unsigned short)abs(n), var_type->dp);
}

void display_geometric_number(byte enc) {
}

void display_note(byte enc) {
}

 
// byte _choices_num, byte _choices_length, byte _bt_mul_down = 1, byte _additional_flags = 0
choices_t Function_var_type(0, 15, 4, ENCODER_FLAGS_CYCLE);
  // {0, 14, ENCODER_FLAGS_CYCLE | ENCODER_FLAGS_CHOICE_LEDS, 1, 4, UPDATE_CHOICES, 0};

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
