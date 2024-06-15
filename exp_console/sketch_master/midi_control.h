// midi_control.h

/************

    Control changes:
      control_change(channel, control, value, cable)

    Non-registered Parameters (NRPNs) (14 bit param number and 14 bit value):
      control: 0x62, value is param# LSB
      control: 0x63, value is param# MSB
      control: 0x06, value MSB
      control: 0x26, value LSB (optional)

    Registered Parameter 0x7F, 0x7F Null the saved parameter number for the channel.
      control: 0x64, 0x7F
      control: 0x65, 0x7F

    Status bytes are remembered so that repeats do not need to be sent.

*************/

typedef struct pot_control_s { // default PLAYER_CABLE
  byte control;
  byte player;        // send to PLAYER_CHANNEL
  byte synth;         // send to SYNTH_CHANNEL, SYNTH_CABLE
} pot_control_t;

typedef struct param_slice_s {
  byte bits;
  byte lshift;
  byte bit_mask;
} param_slice_t;

typedef struct fun_control_s {
  unsigned short control;  // param_num when nrpn is true
  byte nrpn;               // true/false
  param_slice_t encoder_slices[NUM_FUNCTION_ENCODERS];
  byte add_harmonic;       // harmonic added to control value, and sent to all harmonic switches on
  byte synth;              // send to SYNTH_CHANNEL
  byte next_control;       // Next control (pseudo function) if one function covers multiple controls.
                           // Forms a linked list of controls.  0 = End.
} fun_control_t;

extern pot_control_t  Pot_controls[];
extern fun_control_t  Function_controls[];

extern void send_pot(byte pot);
extern void send_function(void);
extern byte setup_midi_control(byte EEPROM_offset);


// vim: sw=2
