// variable.h

// 8288 total variables (bytes)

#define NUM_CHANNEL_VARS            19
#define NUM_CHANNELS                16  /* incl synth */

// 608 bytes
extern byte Channel_values[2][NUM_CHANNELS][NUM_CHANNEL_VARS];

#define NUM_HARMONIC_VARS           24
#define NUM_HARMONICS               10

// 7680 bytes
extern byte Harmonic_values[2][NUM_CHANNELS][NUM_HARMONICS][NUM_HARMONIC_VARS];

// vim: sw=2
