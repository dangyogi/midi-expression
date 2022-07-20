// encoders.h

#define ENCODER_FLAGS_ENABLED       1
#define ENCODER_FLAGS_CYCLE         2
#define ENCODER_FLAGS_CHOICE_LEDS   4

typedef struct {
  byte min, max;
  byte flags;           // See ENCODER_FLAGS_<X> #defines
  byte bt_mul[2];       // up, down
  byte value;
  byte changed;         // set to 1 each time value changes.
  byte param_num;       // not directly used here
} encoder_var_t;

typedef struct {
  byte state;           // bit 0: last A, bit 1: last B
  byte A_sw;
  byte display_num;     // first led_num if ENCODER_FLAGS_CHOICE_LEDS, else numeric disp#
  encoder_var_t *var;   // disabled if NULL
} encoder_t;

#define NUM_ENCODERS    6

extern encoder_t Encoders[NUM_ENCODERS];

#define FUNCTION_ENCODER   0
#define FILENAME_ENCODER   5

extern encoder_var_t Filename_var;
extern encoder_var_t Function_var[2];

#define FUNCTION        (Function_var[SAVE_OR_SYNTH].value)

extern byte setup_encoders(byte EEPROM_offset);

// vim: sw=2
