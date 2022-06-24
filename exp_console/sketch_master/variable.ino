// variable.ino

// These are for the 4 function encoders that set the parameter values for the various
// functions.
//
// There are two types of functions: channel functions, and harmonic functions.

// 8288 total variables (bytes)

// 608 bytes
byte Channel_values[2][NUM_CHANNELS][NUM_CHANNEL_VARS];

// 7680 bytes
byte Harmonic_values[2][NUM_CHANNELS][NUM_HARMONICS][NUM_HARMONIC_VARS];

// vim: sw=2
