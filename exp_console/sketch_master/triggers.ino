// triggers.ino

byte EEPROM_triggers_offset;

// {switch_, button, led, check_event}
trigger_t Triggers[NUM_TRIGGERS] = {  // pot triggers share same array index as Num_pots/Pots arrays.
  { 0,  9, 167, CHECK_POTS},      // 1st Note
  { 1, 10, 166, CHECK_POTS},      // 2nd Note
  { 2, 11, 165, CHECK_POTS},      // 3rd Note
  { 3, 12, 164, CHECK_POTS},      // 4th Note
  { 4, 13, 163, CHECK_POTS},      // 5th Note
  {34, 47, 171, CHECK_POTS},      // vol note_on/off
  {33, 46, 170, CHECK_FUNCTIONS}, // functions
};

byte Num_pots[NUM_POT_TRIGGERS] = {3, 3, 3, 3, 3, 3};
byte Pots[NUM_POT_TRIGGERS][MAX_TRIGGER_POTS] = {
  { 1,  2,  3},  // 1st Note
  { 4,  5,  6},  // 2nd Note
  { 7,  8,  9},  // 3rd Note
  {10, 11, 12},  // 4th Note
  {13, 14, 15},  // 5th Note
  {18, 19, 20},  // vol note_on/off
};

void disable_triggers(void) {
  byte i;
  for (i = 0; i < NUM_TRIGGERS; i++) {
    if (Triggers[i].continuous) {
      Triggers[i].continuous = 0;
      led_off(Triggers[i].led);
    }
  }
}

void changed(byte trigger) {
}

void check_triggers(void) {
  byte i;
  for (i = 0; i < NUM_TRIGGERS; i++) {
    if (Triggers[i].continuous) check_trigger(i);
  }
}

void check_trigger(byte trigger) {
  run_event(Triggers[trigger].check_event, trigger);
}

byte setup_triggers(byte EEPROM_offset) {
  EEPROM_triggers_offset = EEPROM_offset;
  byte i;
  for (i = 0; i < NUM_TRIGGERS; i++) {
    Switches[Triggers[i].switch_].tag = i;
    Switch_closed_event[Triggers[i].switch_] = TRIGGER_SW_ON;
    Switch_opened_event[Triggers[i].switch_] = TRIGGER_SW_OFF;

    Switches[Triggers[i].button].tag = i;
    Switch_closed_event[Triggers[i].button] = TRIGGER_BT_PRESSED;
    Switch_opened_event[Triggers[i].button] = TRIGGER_BT_RELEASED;
  }
  return 0;
}


// vim: sw=2
