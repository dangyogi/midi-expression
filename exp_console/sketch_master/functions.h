// functions.h

#define NUM_CHANNELS           16  /* incl synth */
#define NUM_HARMONICS          10

#define NUM_CH_FUNCTIONS        8
#define NUM_HM_FUNCTIONS        7
#define NUM_FUNCTIONS           (NUM_CH_FUNCTIONS + NUM_HM_FUNCTIONS)
#define NUM_FUNCTION_ENCODERS   4

extern variable_t Functions[NUM_FUNCTIONS][NUM_FUNCTION_ENCODERS];

// 512 bytes
extern byte Channel_memory[NUM_CHANNELS][NUM_CH_FUNCTIONS][NUM_FUNCTION_ENCODERS];

// 4480 bytes
extern byte Harmonic_memory[NUM_CHANNELS][NUM_HARMONICS][NUM_HM_FUNCTIONS][NUM_FUNCTION_ENCODERS];

extern byte Function_changed; // Some function somewhere has changed.

extern byte Lowest_harmonic;  // 0-9, 0xFF when all switches off
extern byte Lowest_channel;   // 0-15, 0xFF when all switches off

extern void update_channel_memory(byte ch);
extern void update_harmonic_memory(byte ch, byte hm);
extern void load_functions(byte skip_ch_functions=0);
extern void truncate_function(void);
extern void load_encoders(void);
extern void clear_displays(void);
extern void update_displays(void);
extern void harmonic_on(byte sw);
extern void harmonic_off(byte sw);
extern void channel_on(byte sw);
extern void channel_off(byte sw);
extern void reset_function_encoders(void);

extern byte setup_functions(byte EEPROM_offset);

// vim: sw=2
