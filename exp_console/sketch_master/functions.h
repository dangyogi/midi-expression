// functions.h

#define NUM_CHANNELS           16  /* incl synth */
#define NUM_HARMONICS          10

#define NUM_CH_FUNCTIONS        8
#define NUM_HM_FUNCTIONS        7
#define NUM_FUNCTIONS           (NUM_CH_FUNCTIONS + NUM_HM_FUNCTIONS)
#define NUM_FUNCTION_ENCODERS   4

extern encoder_var_t Functions[NUM_FUNCTIONS][NUM_FUNCTION_ENCODERS];

extern byte Lowest_harmonic;  // 0-9, 0xFF when all switches off
extern byte Lowest_channel;   // 0-15, 0xFF when all switches off

extern void load_function_encoder_values(void);
extern void save_function_encoder_values(void);
extern void harmonic_on(byte sw);
extern void harmonic_off(byte sw);
extern void channel_on(byte sw);
extern void channel_off(byte sw);
extern void reset_function_encoders(void);

extern byte setup_functions(byte EEPROM_offset);

// vim: sw=2
