// update.ino

{# Template expects:
   
   num_triggers
   num_commands
   enc_displays: [(enc, led)]
   triggers: [(sw, led, bt)]
   params: [(t, tn, bits, off, fc, nc, [cmd])]
   lin_ptypes: [(start, step, limit, bits, dp)]
   geom_ptypes: [(m, b, c, start, limit, bits, dp)]
   constant_ptypes: [(value, bits)]
   choices_ptypes: [(t, bits, null, limit, num_leds, [led])]

#}

byte EEPROM_update_offset;

byte setup_update(byte EEPROM_offset) {
  byte i;
  EEPROM_update_offset = EEPROM_offset;

  {% for enc, led in enc_displays %}
  Encoders[{{ enc }}].tag = {{ led }};
  {% endfor %}

  for (i = 0; i < NUM_TRIGGERS; i++) {
    Switch_closed_events[Triggers[i].sw] = TRIGGER_SW_CLOSED(i);
    Switch_opened_events[Triggers[i].sw] = TRIGGER_SW_OPENED(i);
    Switch_closed_events[Triggers[i].bt] = TRIGGER_BT_CLOSED(i);
  } // end for (i)

  return 0;
}

byte First_changed_param;

param_t Params[NUM_PARAMETERS] = {
  {% for t, tn, bits, off, fc, nc, cmds in params %}
  { {{ t }}, {{ tn }}, {{ bits }}, {{ off }}, 0xff, {{ fc }}, {{ nc }}, {
    {% for c in cmds %}{{ c }}, {% endfor %} } },
  {% endfor %}
};

void param_changed(byte param_num, byte new_raw_value) {
  Params[param_num].new_raw_value = new_raw_value;
  if (Params[param_num].next_changed_param == 0xff) {
    if (new_raw_value != Params[param_num].current_raw_value) {
      Params[param_num].next_changed_param = First_changed_param;
      First_changed_param = param_num;
    }
  }
}

lin_ptype_t Lin_ptypes[NUM_LIN_PTYPES] = {
  {% for start, step, limit, bits, dp in lin_ptypes %}
  { {{ start }}, {{ step }}, {{ limit }}, {{ bits }}, {{ dp }} },
  {% endfor %}
};

geom_ptype_t Geom_ptypes[NUM_GEOM_PTYPES] = {
  {% for m, b, c, start, limit, bits, dp in geom_ptypes %}
  { {{ m }}, {{ b }}, {{ c }}, {{ start }}, {{ limit }}, {{ bits }}, {{ dp }} },
  {% endfor %}
};

constant_ptype_t Constant_ptypes[NUM_CONSTANT_PTYPES] = {
  {% for value, bits in constant_ptypes %}
  { {{ value }}, {{ bits }} },
  {% endfor %}
};

choices_ptype_t Choices_ptypes[NUM_CHOICES_PTYPES] = {
  {% for t, bits, null, limit, num_leds, leds in choices_ptypes %}
  { {{ t }}, {{ bits }}, {{ null }}, {{ limit }}, {{ num_leds }}, {
    {% for led in leds %}{{ led }}, {% endfor %} } },
  {% endfor %}
};

byte First_changed_command;

command_t Commands[NUM_COMMANDS] = {
  {% for _ in range(num_commands) %}
  {0, 0xff},
  {% endfor %}
};

void command_changed(byte command_num, unsigned short or_value, unsigned short and_value) {
  // or_value is applied first.  It can be all 1 to clear all bits before and_value is
  // applied.
  if (Commands[command_num].next_changed_command == 0xff) {
    Commands[command_num].next_changed_command = First_changed_command;
    First_changed_command = command_num;
  }
  Commands[command_num].MIDI_value &= ~or_value;
  Commands[command_num].MIDI_value |= and_value;
}

trigger_t Triggers[NUM_TRIGGERS] = {
  {% for sw, led, bt in triggers %}
  { {{ sw }}, {{ led }}, {{ bt }}, TRIGGER_STATE_MANUAL_IDLE },
  {% endfor %}
};

void update(void) {
  byte i;

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
}

void update_displays(void) {
}


// vim: sw=2
