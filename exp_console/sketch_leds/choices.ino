// choices.ino

byte EEPROM_choices_offset;

byte EEPROM_Num_choices(void) {
  return EEPROM_choices_offset;
}

byte EEPROM_Choices_start(byte choices) {
  return 1 + choices + EEPROM_choices_offset;
}

byte EEPROM_Choices_length(byte choices) {
  return 1 + MAX_CHOICES + choices + EEPROM_choices_offset;
}

byte Num_choices;
byte Choices_start[MAX_CHOICES];
byte Choices_length[MAX_CHOICES];
byte Current_choice[MAX_CHOICES];   // 0xff if no current choice

byte setup_choices(byte my_EEPROM_offset) {
  EEPROM_choices_offset = my_EEPROM_offset;

  byte b = EEPROM[EEPROM_Num_choices()];
  if (b == 0xFF) {
    Serial.println("Num_choices not set in EEPROM");
  } else if (b > MAX_CHOICES) {
    Errno = 110;
    Err_data = b;
  } else Num_choices = b;

  byte i;
  for (i = 0; i < Num_choices; i++) {
    b = EEPROM[EEPROM_Choices_start(i)];
    if (b == 0xFF) {
      Serial.print("Choices_start not set in EEPROM for ");
      Serial.println(i);
    } else if (b > Num_rows * NUM_COLS) {
      Errno = 111;
      Err_data = b;
    } else Choices_start[i] = b;

    b = EEPROM[EEPROM_Choices_length(i)];
    if (b == 0xFF) {
      Serial.print("Choices_length not set in EEPROM for ");
      Serial.println(i);
    } else if (b > MAX_CHOICES_LENGTH) {
      Errno = 112;
      Err_data = b;
    } else Choices_length[i] = b;
  } // end for (i)

  return 1 + 2*MAX_CHOICES;  // EEPROM needed
}

void select_choice(byte choices, byte choice) {
  if (choices >= Num_choices) {
    Errno = 113;
    Err_data = choices;
    return;
  }
  if (choice >= Choices_length[choices]) {
    Errno = 114;
    Err_data = choice;
    return;
  }
  if (Current_choice[choices] < Choices_length[choices]) {
    led_off(Choices_start[choices] + Current_choice[choices]);
  }
  led_on(Choices_start[choices] + choice);
  Current_choice[choices] = choice;
}

void clear_choices(byte choices) {
  if (choices >= Num_choices) {
    Errno = 115;
    Err_data = choices;
    return;
  }
  if (Current_choice[choices] < Choices_length[choices]) {
    led_off(Choices_start[choices] + Current_choice[choices]);
    Current_choice[choices] = 0xff;
  }
}

// vim: sw=2
