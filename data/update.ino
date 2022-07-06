// update.ino

{# Template expects:
   
   enc_displays: [(enc, led)]
   internal_params: [(t, off, bits, fc, nc)]
   param_commands: [cmd]
   pot_params: [(t, off, bits, fc, nc)]
   display_params: [(t, off, bits, fc, nc, pt)]
   lin_ptypes: [(start, step, limit, dp)]
   geom_ptypes: [(m, b, c, start, limit, dp)]
   choice_notes_ptypes: [(null, limit)]
   choice_led_ptypes: [(null, limit, [led])]
   single_commands: [(code, fl)]
   channel_commands: [(code, fl, tr, chp)]
   harmonic_commands: [(code, fl, tr, chp, hmp)]
   triggers: [(sw, led, bt)]

#}
byte EEPROM_update_offset;

byte setup_update(byte EEPROM_offset) {
  byte i;
  EEPROM_update_offset = EEPROM_offset;

  {% for enc, led in enc_displays %}
  Encoders[{{ enc }}].display_num = {{ led }};
  {% endfor %}

  for (i = 0; i < NUM_TRIGGERS; i++) {
    Switch_closed_events[Triggers[i].sw] = TRIGGER_SW_CLOSED(i);
    Switch_opened_events[Triggers[i].sw] = TRIGGER_SW_OPENED(i);
    Switch_closed_events[Triggers[i].bt] = TRIGGER_BT_CLOSED(i);
  } // end for (i)

  return 0;
}

internal_param_s Internal_params[NUM_INTERNAL_PARAMS] = {
  {% for t, off, bits, fc, nc in internal_params %}
  { {{ t }}, {{ off }}, {{ bits }}, {{ fc }}, {{ nc }} },
  {% endfor %}
};

byte Param_commands[NUM_PARAM_COMMANDS] = {
  {% for c in param_commands %}
  {{ c }},
  {% endfor %}
};

pot_param_s Pot_params[NUM_POT_PARAMS] = {
  {% for t, off, bits, fc, nc in pot_params %}
  { {{ t }}, {{ off }}, {{ bits }}, {{ fc }}, {{ nc }} },
  {% endfor %}
};

display_param_s Display_params[NUM_DISPLAY_PARAMS] = {
  {% for t, off, bits, fc, nc, pt in display_params %}
  { {{ t }}, {{ off }}, {{ bits }}, {{ fc }}, {{ nc }}, 0, 0, {{ pt }} },
  {% endfor %}
};

internal_param_s *get_internal_param(byte param_num) {
  switch ((param_num & PARAM_TYPE_MASK) >> PARAM_TYPE_SHIFT) {
  case 0: return &Internal_params[param_num & ~PARAM_TYPE_MASK];
  case 1: return &Pot_params[param_num & ~PARAM_TYPE_MASK];
  case 2: return &Display_params[param_num & ~PARAM_TYPE_MASK];
  default: Errno = 51; Err_data = param_num; return &Internal_params[0];
  }
}

pot_param_s *get_pot_param(byte param_num) {
  switch ((param_num & PARAM_TYPE_MASK) >> PARAM_TYPE_SHIFT) {
  case 1: return &Pot_params[param_num & ~PARAM_TYPE_MASK];
  case 2: return &Display_params[param_num & ~PARAM_TYPE_MASK];
  default: Errno = 52; Err_data = param_num; return &Pot_params[0];
  }
}

display_param_s *get_display_param(byte param_num) {
  switch ((param_num & PARAM_TYPE_MASK) >> PARAM_TYPE_SHIFT) {
  case 2: return &Display_params[param_num & ~PARAM_TYPE_MASK];
  default: Errno = 53; Err_data = param_num; return &Display_params[0];
  }
}

void param_changed(byte param_num, byte new_value, byte display_num) {
  pot_params_s *param = pot_param(param_num);
  param->new_value = new_value;
  if (new_value != param->current_setting_value) {
    byte i;
    unsigned short and_value, or_value;

    // clear bits that are set to 1 here
    and_value = ((1 << param->bits) - 1) << param->value_bit_offset;

    // set bits that are set to 1 here
    or_value = (new_value & ((1 << param->bits) - 1)) << param->value_bit_offset;

    for (i = 0; i < param->num_commands; i++) {
      command_changed(Param_commands[param->first_command + i], and_value, or_value);
    }
    if (display_num != 0xff) {
      // FIX
    }
  }
}

lin_ptype_t Lin_ptypes[NUM_LIN_PTYPES] = {
  {% for start, step, limit, dp in lin_ptypes %}
  { {{ start }}, {{ step }}, {{ limit }}, {{ dp }} },
  {% endfor %}
};

geom_ptype_t Geom_ptypes[NUM_GEOM_PTYPES] = {
  {% for m, b, c, start, limit, dp in geom_ptypes %}
  { {{ m }}, {{ b }}, {{ c }}, {{ start }}, {{ limit }}, {{ dp }} },
  {% endfor %}
};

choice_notes_s Choice_notes_ptypes[NUM_CHOICE_NOTES_PTYPES] = {
  {% for null, limit in choice_notes_ptypes %}
  { {{ null }}, {{ limit }} },
  {% endfor %}
};

choice_leds_s Choice_leds_ptypes[NUM_CHOICE_LEDS_PTYPES] = {
  {% for null, limit, leds in choice_leds_ptypes %}
  { {{ null }}, {{ limit }}, {{ len(leds) }}, {
  {% for led in leds %}
  {{ led }},
  {% endfor %}
  } },
  {% endfor %}
};

byte First_changed_command;

single_command_s Single_commands[NUM_SINGLE_COMMANDS] = {
  {% for code, fl in single_commands %}
  { {{ code }}, 0, {{ fl }}, 0xff},
  {% endfor %}
};

channel_command_s Channel_commands[NUM_CHANNEL_COMMANDS] = {
  {% for code, fl, tr, chp in channel_commands %}
  { {{ code }}, 0, {{ fl }}, 0xff, {{ tr }}, {{ chp }} },
  {% endfor %}
};

harmonic_command_s Harmonic_commands[NUM_HARMONIC_COMMANDS] = {
  {% for code, fl, tr, chp, hmp in harmonic_commands %}
  { {{ code }}, 0, {{ fl }}, 0xff, {{ tr }}, {{ chp }}, {{ hmp }} },
  {% endfor %}
};

single_command_s *get_single_command(byte command_num) {
  switch ((command_num & COMMAND_TYPE_MASK) >> COMMAND_TYPE_SHIFT) {
  case 0: return &Single_commands[command_num & ~COMMAND_TYPE_MASK];
  case 1: return &Channel_commands[command_num & ~COMMAND_TYPE_MASK];
  case 2: return &Harmonic_commands[command_num & ~COMMAND_TYPE_MASK];
  default: Errno = 54; Err_data = command_num; return &Single_commands[0];
  }
}

channel_command_s *get_channel_command(byte command_num) {
  switch ((command_num & COMMAND_TYPE_MASK) >> COMMAND_TYPE_SHIFT) {
  case 1: return &Channel_commands[command_num & ~COMMAND_TYPE_MASK];
  case 2: return &Harmonic_commands[command_num & ~COMMAND_TYPE_MASK];
  default: Errno = 55; Err_data = command_num; return &Channel_commands[0];
  }
}

harmonic_command_s *get_harmonic_command(byte command_num) {
  switch ((command_num & COMMAND_TYPE_MASK) >> COMMAND_TYPE_SHIFT) {
  case 2: return &Harmonic_commands[command_num & ~COMMAND_TYPE_MASK];
  default: Errno = 56; Err_data = command_num; return &Harmonic_commands[0];
  }
}

void command_changed(byte command_num, unsigned short and_value, unsigned short or_value) {
  // and_value is applied first.  It can be all 1 to clear all bits before or_value is
  // applied.
  single_command_s *command = get_single_command(command_num);
  if (command->next_changed_command == 0xff) {
    command->next_changed_command = First_changed_command;
    First_changed_command = command_num;
  }
  command->MIDI_value &= ~and_value;
  command->MIDI_value |= or_value;
}

trigger_t Triggers[NUM_TRIGGERS] = {
  {% for sw, led, bt in triggers %}
  { {{ sw }}, {{ led }}, {{ bt }}, 0},
  {% endfor %}
};

void update(void) {
  // pots have been read just prior to this call.
  byte i;

  // FIX: review, this is out of date...

  for (i = 0; i < NUM_TRIGGERS; i++) {
    byte send = 0;
    if (Triggers[i].state == TRIGGER_STATE_MANUAL_TRIGGERED) {
      send = 1;
      Triggers[i].state = TRIGGER_STATE_MANUAL_IDLE;
    } else if (Triggers[i].state == TRIGGER_STATE_STREAM) {
      send = 1;
    }
    send_MIDI_commands(i, send);
  } // end for (i)
}

void send_MIDI_commands(byte trigger_num, byte send) {
  // FIX
}

void update_displays(void) {
  // FIX
}


// vim: sw=2
