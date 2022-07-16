// update.ino

{# Template expects:
   
   enc_displays: [(enc, led)]
   internal_params: [(t, off, bits)]
   param_commands: [cmd]
   pot_params: [(t, off, bits, fc, nc)]
   pot_commands: [cmd_num]
   display_params: [(t, off, bits, fc, nc, pt)]
   fun_commands: [cmd_num]
   lin_ptypes: [(start, step, limit, dp)]
   geom_ptypes: [(base, b2, c, start, limit, dp)]
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

void update_cmd_value(byte value, internal_param_s *param, single_command_s *cmd) {
  // and_value is applied first.  It can be all 1 to clear all bits before or_value is
  // applied.

  // clear bits that are set to 1 here
  unsigned long and_value = ((1ul << param->bits) - 1) << param->value_bit_offset;

  // set bits that are set to 1 here
  unsigned long or_value =
      ((unsigned long)(value & ((1 << param->bits) - 1))) << param->value_bit_offset;
  cmd->MIDI_value &= ~and_value;
  cmd->MIDI_value |= or_value;
}

internal_param_s Internal_params[NUM_INTERNAL_PARAMS] = {
  {% for t, off, bits in internal_params %}
  { {{ t }}, {{ off }}, {{ bits }} },
  {% endfor %}
};

pot_param_s Pot_params[NUM_POT_PARAMS] = {
  {% for t, off, bits, fc, nc in pot_params %}
  { {{ t }}, {{ off }}, {{ bits }}, 0, 0, {{ fc }}, {{ nc }} },
  {% endfor %}
};

byte Pot_commands[NUM_POT_PARAMS] = {
  {% for cmd_num in pot_commands %}
  { {{ cmd_num }} },
  {% endfor %}
};

display_param_s Display_params[NUM_DISPLAY_PARAMS] = {
  {% for t, off, bits, fc, nc, pt in display_params %}
  { {{ t }}, {{ off }}, {{ bits }}, 0, 0, {{ fc }}, {{ nc }}, {{ pt }} },
  {% endfor %}
};

byte Function_commands[NUM_FUNCTIONS] = {
  {% for cmd_num in fun_commands %}
  { {{ cmd_num }} },
  {% endfor %}
};

internal_param_s *get_internal_param(byte param_num) {
  switch (PARAM_TYPE(param_num)) {
  case INTERNAL_PARAM_TYPE: return &Internal_params[PARAM_NUM(param_num)];
  case POT_PARAM_TYPE: return &Pot_params[PARAM_NUM(param_num)];
  case DISPLAY_PARAM_TYPE: return &Display_params[PARAM_NUM(param_num)];
  default: Errno = 51; Err_data = param_num; return &Internal_params[0];
  }
}

pot_param_s *get_pot_param(byte param_num) {
  switch (PARAM_TYPE(param_num)) {
  case POT_PARAM_TYPE: return &Pot_params[PARAM_NUM(param_num)];
  //case DISPLAY_PARAM_TYPE: return &Display_params[PARAM_NUM(param_num)];
  default: Errno = 52; Err_data = param_num; return &Pot_params[0];
  }
}

display_param_s *get_display_param(byte param_num) {
  switch (PARAM_TYPE(param_num)) {
  case DISPLAY_PARAM_TYPE: return &Display_params[PARAM_NUM(param_num)];
  default: Errno = 53; Err_data = param_num; return &Display_params[0];
  }
}

void param_changed(byte param_num, byte new_value, byte display_num) {
  switch (PARAM_TYPE(param_num)) {
  case INTERNAL_PARAM_TYPE:
    break;
  case POT_PARAM_TYPE:
    pot_param_s *pot_param = get_pot_param(param_num);
    pot_param->new_value = new_value;
    if (new_value != pot_param->current_setting_value) {
      byte i;
      for (i = 0; i < param->num_commands; i++) {
        command_changed(Pot_commands[pot_param->first_command + i], new_value, pot_param);
      }
    } // end if value changed
    break;
  case DISPLAY_PARAM_TYPE:
    display_param_s *disp_param = get_display_param(param_num);
    disp_param->new_value = new_value;
    if (new_value != disp_param->current_setting_value) {
      command_changed(Function_commands[FUNCTION], new_value, disp_param);

      // update display
      if (display_num != 0xff) {
        byte value = disp_param->new_value;
        if (value != disp_param->current_display_value) {
          unsigned short disp_value, disp_current_value;
          lin_ptype_t *lin_type;
          geom_ptype_t *geom_type;
          choice_notes_s *choice_notes;
          choice_leds_s *choice_leds;
          switch (param->type) {
          case PTYPE_LIN:
            lin_type = &Lin_ptypes[disp_param->ptype_num];
            disp_value = lin_type->step_size * value + lin_type->start;
            disp_value = min(lin_type->limit, disp_value);
            display_number(display_num, disp_value, lin_type->decimal_pt);
            break;
          case PTYPE_GEOM:
            geom_type = &Geom_ptypes[disp_param->ptype_num];
            float disp_value_float;
            disp_value_float = pow(geom_type->base, value + geom_type->b2) + geom_type->c;
            disp_value_float = max(geom_type->start, disp_value_float);
            disp_value_float = min(geom_type->limit, disp_value_float);
            for (byte i = 3; i > geom_type->decimal_pt; i--) disp_value_float *= 10.0;
            display_number(display_num, (unsigned short)disp_value_float,
                           geom_type->decimal_pt);
            break;
          case PTYPE_CHOICE_NOTES:
            choice_notes = &Choice_notes_ptypes[disp_param->ptype_num];
            disp_value = min(choice_notes->limit, value);
            if (disp_value == choice_notes->null_value) {
              // blank display
              blank_note(display_num);
            } else {
              // display note
              display_note(display_num, disp_value);
            }
            break;
          case PTYPE_CHOICE_LEDS:
            choice_leds = &Choice_leds_ptypes[disp_param->ptype_num];
            disp_value = min(choice_leds->limit, value);
            disp_current_value = min(choice_leds->limit, disp_param->current_display_value);
            if (disp_current_value != choice_leds->null_value) {
              turn_off_led(choice_leds->led[disp_current_value]);
            }
            if (disp_value != choice_leds->null_value) {
              turn_on_led(choice_leds->led[disp_value]);
            }
            break;
          } // end switch
          disp_param->current_display_value = value;
        } // end if display value changed
      } // end if display_num != 0xff
    } // end if value changed
  } // end switch
}

lin_ptype_t Lin_ptypes[NUM_LIN_PTYPES] = {
  {% for start, step, limit, dp in lin_ptypes %}
  { {{ start }}, {{ step }}, {{ limit }}, {{ dp }} },
  {% endfor %}
};

geom_ptype_t Geom_ptypes[NUM_GEOM_PTYPES] = {
  {% for base, b2, c, start, limit, dp in geom_ptypes %}
  { {{ base }}, {{ b2 }}, {{ c }}, {{ start }}, {{ limit }}, {{ dp }} },
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
  {0, {{ code }}, {{ fl }}, 0xff},
  {% endfor %}
};

channel_command_s Channel_commands[NUM_CHANNEL_COMMANDS] = {
  {% for code, fl, tr, chp in channel_commands %}
  {0, {{ code }}, {{ fl }}, 0xff, {{ tr }}, {{ chp }} },
  {% endfor %}
};

harmonic_command_s Harmonic_commands[NUM_HARMONIC_COMMANDS] = {
  {% for code, fl, tr, chp, hmp in harmonic_commands %}
  {0, {{ code }}, {{ fl }}, 0xff, {{ tr }}, {{ chp }}, {{ hmp }} },
  {% endfor %}
};

single_command_s *get_single_command(byte command_num) {
  switch (COMMAND_TYPE(command_num)) {
  case SINGLE_COMMAND_TYPE: return &Single_commands[COMMAND_NUM(command_num)];
  case CHANNEL_COMMAND_TYPE: return &Channel_commands[COMMAND_NUM(command_num)];
  case HARMONIC_COMMAND_TYPE: return &Harmonic_commands[COMMAND_NUM(command_num)];
  default: Errno = 54; Err_data = command_num; return &Single_commands[0];
  }
}

channel_command_s *get_channel_command(byte command_num) {
  switch (COMMAND_TYPE(command_num)) {
  case CHANNEL_COMMAND_TYPE: return &Channel_commands[COMMAND_NUM(command_num)];
  case HARMONIC_COMMAND_TYPE: return &Harmonic_commands[COMMAND_NUM(command_num)];
  default: Errno = 55; Err_data = command_num; return &Channel_commands[0];
  }
}

harmonic_command_s *get_harmonic_command(byte command_num) {
  switch (COMMAND_TYPE(command_num)) {
  case HARMONIC_COMMAND_TYPE: return &Harmonic_commands[COMMAND_NUM(command_num)];
  default: Errno = 56; Err_data = command_num; return &Harmonic_commands[0];
  }
}

void command_changed(byte command_num, byte value, internal_param_s *param) {
  single_command_s *command = get_single_command(command_num);
  update_cmd_value(value, param, command);
  if (command->next_changed_command == 0xff) {
    command->next_changed_command = First_changed_command;
    First_changed_command = command_num;
  }
}

trigger_t Triggers[NUM_TRIGGERS] = {
  {% for sw, led, bt in triggers %}
  { {{ sw }}, {{ led }}, {{ bt }}, 0},
  {% endfor %}
};

void update(void) {
  // pots have been read just prior to this call.
  byte *prior;
  byte *next;

  for (prior = &First_changed_command; *prior != NULL;) {
    single_command_s *cmd = get_single_command(*prior);

    // local vars for switch code:
    channel_command_s *ch_cmd;
    harmonic_command_s *hm_cmd;
    byte trigger_num;
    byte channel_param;
    byte harmonic_param;
    byte ch, hm;

    switch (COMMAND_TYPE(*prior)) {
    case SINGLE_COMMAND_TYPE:
      // single commands are always triggered.
      send_MIDI_command(cmd);
      *prior = cmd->next;  // delink
      break;
    case CHANNEL_COMMAND_TYPE:
      ch_cmd = (channel_command_s *)cmd;
      if (triggered(ch_cmd->trigger_num)) {
        for (ch = 0; ch < NUM_CHANNELS; ch++) {
          if (Switches[FIRST_CHANNEL_SWITCH + ch].current) {
            if (ch != 0) {
              update_cmd_value(ch, get_internal_param(ch_cmd->channel_param), ch_cmd);
            }
            send_MIDI_command(ch_cmd, i == 0);
            // FIX: update values
          }
        } // end for (ch)
        *prior = ch_cmd->next;  // delink
      } // end if triggered
      break;
    case HARMONIC_COMMAND_TYPE:
      hm_cmd = (harmonic_command_s *)cmd;
      if (triggered(hm_cmd)->trigger_num)) {
        for (ch = 1; ch < NUM_CHANNELS; ch++) {
          if (Switches[FIRST_CHANNEL_SWITCH + ch].current) {
            update_cmd_value(ch, get_internal_param(hm_cmd->channel_param), hm_cmd);
            for (hm = 0; hm < NUM_HARMONICS; hm++) {
              if (Switches[FIRST_HARMONIC_SWITCH + hm].current) {
                update_cmd_value(hm, get_internal_param(hm_cmd->harmonic_param), hm_cmd);
                send_MIDI_command(hm_cmd);
                // FIX: update values
              } // end if hm switch on
            } // end for (hm)
          } // end if ch switch on
        } // end for (ch)
        *prior = hm_cmd->next;  // delink
      } // end if triggered
      break;
    } // end switch
  } // end for (prior)
}

byte triggered(trigger_num) {
  if (!(Triggers[trigger_num].flags & TRIGGER_FLAGS_TRIGGERED)) return 0;
  if (!(Triggers[trigger_num].flags & TRIGGER_FLAGS_STREAM_MANUAL)) {
    // triggered by button, turn off triggered flag
    Triggers[trigger_num].flags &= ~TRIGGER_FLAGS_TRIGGERED;
  }
  return 1;
}

void send_MIDI_command(single_command_s *cmd, byte synth=0) {
  // FIX
}


// vim: sw=2
