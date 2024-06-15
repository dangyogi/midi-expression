// midi_control.ino

// {control, player, synth}
pot_control_t Pot_controls[] = {
  // These go to PLAYER_CABLE, except the synth control(s) (ie., Synth Vol).

  // Chord 1
  {0x18}, // vol
  {0x19}, // Note-on delay
  {0x1A}, // Note-off delay

  // Chord 2
  {0x1B}, // vol
  {0x1C}, // Note-on delay
  {0x1D}, // Note-off delay

  // Chord 3
  {0x1E}, // vol
  {0x1F}, // Note-on delay
  {0x34}, // Note-off delay

  // Chord 4
  {0x35}, // vol
  {0x36}, // Note-on delay
  {0x37}, // Note-off delay

  // Chord 5
  {0x38}, // vol
  {0x39}, // Note-on delay
  {0x3A}, // Note-off delay

  // Always triggered, no channels
  {0x14, 1},    // Tempo
  {0x07, 0, 1}, // Synth Vol

  {0x15}, // Channel Vol
  {0x16}, // Note-on delay
  {0x17}, // Note-off delay
};

#define NUM_FUN_CONTROLS    (NUM_FUNCTIONS + 1)

// {control, nrpn, encoder_slices[NUM_FUNCTION_ENCODERS], add_harmonic, synth, next_control}
fun_control_t Function_controls[NUM_FUN_CONTROLS] = {
  {0x14,   0, {{4, 0}, {}, {}, {1, 4}}, 0, 1}, // Key Signature
  {0x0007, 1, {{}, {4, 10}, {3, 7}, {7, 0}}, 0, 1}, // Tune Absolute
  {0x000A, 1, {{}, {4, 4}, {4, 0}, {}}}, // Match Tuning
  {0x0002, 1, {{}, {}, {}, {5, 0}}}, // Equal Temperament
  {0x0003, 1, {{4, 4}, {4, 0}, {}, {}}}, // Well Tempered
  {0x0005, 1, {{4, 10}, {2, 8}, {7, 0}, {1, 7}}}, // Meantone
  {0x0004, 1, {{4, 4}, {4, 0}, {}, {4, 8}}}, // Just Intonation
  {0x0006, 1, {{4, 7}, {}, {7, 0}, {}}}, // Pythagorean

  {0x0070, 1, {{}, {7, 7}, {7, 0}, {}}, 1, 0, 15}, // Harmonic Basics 1: freq
  {0x0010, 1, {{3, 11}, {4, 7}, {4, 3}, {3, 0}}, 1}, // Freq Env: Ramp
  {0x0020, 1, {{3, 11}, {4, 7}, {7, 0}, {}}, 1}, // Freq Env: Sine
  {0x0030, 1, {{3, 11}, {4, 7}, {4, 3}, {3, 0}}, 1}, // Attack
  {0x0040, 1, {{3, 8}, {}, {5, 3}, {3, 0}}, 1}, // Decay
  {0x0050, 1, {{3, 11}, {4, 3}, {3, 0}, {4, 7}}, 1}, // Sustain
  {0x0060, 1, {{3, 7}, {}, {4, 3}, {3, 0}}, 1}, // Release

  {0x36, 0, {{7, 0}, {}, {}, {}}, 1}, // Harmonic Basics 2: ampl
};

void update_ch_hm_memory(byte update_memory, byte ch, byte hm) {
  switch (update_memory) {
  case 1:
    update_channel_memory(ch);
  case 0:
    break;
  case 2:
    update_harmonic_memory(ch, hm);
    break;
  } // end switch
}

void control_change_channels(byte control, byte value, byte cable, byte update_memory=0, byte hm=0) {
  // update_memory: 0 - no update, 1 - update Channel_memory, 2 - update Harmonic_memory (also requires
  // harmonic param).
  byte ch;
  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    if (Switches[CHANNEL_TO_SWITCH(ch)].current) {
      control_change(ch, control, value, cable);
      update_ch_hm_memory(update_memory, ch, hm);
    } // end if
  } // end for
}

void control_change_harmonics(byte base_control, byte value, byte cable) {
  // Pass 2 for update_memory (to get Harmonic_memory updated).
  byte hm;
  for (hm = 0; hm < NUM_HARMONICS; hm++) {
    if (Switches[HARMONIC_TO_SWITCH(hm)].current) {
      control_change_channels(base_control + hm, value, cable, 2, hm);
    }
  }
}

void nrpn_change_channels(unsigned short param_num, unsigned short value, byte cable,
                          byte update_memory=0, byte hm=0
) {
  // update_memory: 0 - no update, 1 - update Channel_memory, 2 - update Harmonic_memory (also requires
  // harmonic param).
  byte ch;
  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    if (Switches[CHANNEL_TO_SWITCH(ch)].current) {
      nrpn_change(ch, param_num, value, cable);
      update_ch_hm_memory(update_memory, ch, hm);
    }
  }
}

void nrpn_change_harmonics(unsigned short base_param_num, unsigned short value, byte cable) {
  byte hm;
  for (hm = 0; hm < NUM_HARMONICS; hm++) {
    if (Switches[HARMONIC_TO_SWITCH(hm)].current) {
      nrpn_change_channels(base_param_num + hm, value, cable, 2, hm);
    }
  }
}

void send_pot(byte pot) {
  // Caller responsible for only calling when control_change should be sent out.
  // This includes checking to make sure that at least one channel switch is on (except for player or
  // synth pots, ie. Tempo and Synth Vol).
  pot_control_t *pot_control = &Pot_controls[pot];
  if (pot_control->synth) {
    control_change(SYNTH_CHANNEL, pot_control->control, Current_pot_value[pot], SYNTH_CABLE);
  } else if (pot_control->player) {
    control_change(PLAYER_CHANNEL, pot_control->control, Current_pot_value[pot], PLAYER_CABLE);
  } else {
    control_change_channels(pot_control->control, Current_pot_value[pot], PLAYER_CABLE);
  }
  Synced_pot_value[pot] = Current_pot_value[pot];
}

void send_function(void) {
  // Caller responsible for only calling when control_change should be sent out.
  // This includes checking to make sure that at least one channel switch is on (except for synth
  // functions, ie., Key Signature and Tune Absolute).
  // And at least one harmonic switch is on for harmonic functions.
  byte fun = FUNCTION;
  do {
    fun_control_t *fun_control = &Function_controls[fun];

    // Gather up value from function encoders
    unsigned short value = 0;
    byte enc;
    for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
      value |=
        ((unsigned short)(Functions[FUNCTION][enc].value & fun_control->encoder_slices[enc].bit_mask))
        << fun_control->encoder_slices[enc].lshift;
    }

    if (fun_control->synth) {
      // No add_harmonics with synth
      if (fun_control->nrpn) {
        nrpn_change(SYNTH_CHANNEL, fun_control->control, value, SYNTH_CABLE);
        update_channel_memory(SYNTH_CHANNEL);
      } else {
        control_change(SYNTH_CHANNEL, fun_control->control, value, SYNTH_CABLE);
        update_channel_memory(SYNTH_CHANNEL);
      }
    } else if (fun_control->add_harmonic) {
      // No synth with harmonics
      if (fun_control->nrpn) {
        nrpn_change_harmonics(fun_control->control, value, SYNTH_CABLE);
      } else {
        control_change_harmonics(fun_control->control, value, SYNTH_CABLE);
      }
    } else {
      if (fun_control->nrpn) {
        nrpn_change_channels(fun_control->control, value, SYNTH_CABLE, 1);
      } else {
        control_change_channels(fun_control->control, value, SYNTH_CABLE, 1);
      }
    }
    fun = fun_control->next_control;
  } while (fun);
  Function_changed = 0;
}

byte setup_midi_control(byte EEPROM_offset) {
  byte fun, enc;
  for (fun = 0; fun < NUM_FUN_CONTROLS; fun++) {
    param_slice_t *params = Function_controls[fun].encoder_slices;
    for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
      params[enc].bit_mask = (1 << params[enc].bits) - 1;
    }
  }
  return 0;
}


// vim: sw=2
