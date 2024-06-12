// midi_control.ino

// {trigger, control, player, synth}
pot_control_t Pot_controls[] = {  // these are all player controls
  // Chord 1
  {0, 0x18}, // vol
  {0, 0x19}, // Note-on delay
  {0, 0x1A}, // Note-off delay

  // Chord 2
  {1, 0x1B}, // vol
  {1, 0x1C}, // Note-on delay
  {1, 0x1D}, // Note-off delay

  // Chord 3
  {2, 0x1E}, // vol
  {2, 0x1F}, // Note-on delay
  {2, 0x34}, // Note-off delay

  // Chord 4
  {3, 0x35}, // vol
  {3, 0x36}, // Note-on delay
  {3, 0x37}, // Note-off delay

  // Chord 5
  {4, 0x38}, // vol
  {4, 0x39}, // Note-on delay
  {4, 0x3A}, // Note-off delay

  // Always triggered, no channels
  {0xFF, 0x14, 1},    // Tempo
  {0xFF, 0x07, 0, 1}, // Synth Vol

  {5, 0x15}, // Channel Vol
  {5, 0x16}, // Note-on delay
  {5, 0x17}, // Note-off delay
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

void control_change_channels(byte control, byte value, byte cable) {
  byte ch;
  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    if (Switches[CHANNEL_TO_SWITCH(ch)].current) {
      control_change(ch, control, value, cable);
    }
  }
}

void control_change_harmonics(byte base_control, byte value, byte cable) {
  byte hm;
  for (hm = 0; hm < NUM_HARMONICS; hm++) {
    if (Switches[HARMONIC_TO_SWITCH(hm)].current) {
      control_change_channels(base_control + hm, value, cable);
    }
  }
}

void nrpn_change_channels(unsigned short control, unsigned short value, byte cable) {
  byte ch;
  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    if (Switches[CHANNEL_TO_SWITCH(ch)].current) {
      nrpn_change(ch, control, value, cable);
    }
  }
}

void nrpn_change_harmonics(unsigned short base_control, unsigned short value, byte cable) {
  byte hm;
  for (hm = 0; hm < NUM_HARMONICS; hm++) {
    if (Switches[HARMONIC_TO_SWITCH(hm)].current) {
      nrpn_change_channels(base_control + hm, value, cable);
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
        nrpn_change_channels(fun_control->control, value, SYNTH_CABLE);
      } else {
        control_change_channels(fun_control->control, value, SYNTH_CABLE);
      }
    } else if (fun_control->add_harmonic) {
      // No synth with harmonics
      if (fun_control->nrpn) {
        nrpn_change_harmonics(fun_control->control, value, SYNTH_CABLE);
      } else {
        control_change_harmonics(fun_control->control, value, SYNTH_CABLE);
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
