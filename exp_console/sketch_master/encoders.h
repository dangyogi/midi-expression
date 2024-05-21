// encoders.h

#define ENCODER_FLAGS_DISABLED      1
#define ENCODER_FLAGS_CYCLE         2
#define ENCODER_FLAGS_CHOICE_LEDS   4

// Every parameter type has one of these:
typedef struct var_type_s {
  var_type_s(byte _max, byte _display_value = 0xFF, byte _flags = 0, byte _bt_mul_down = 1, byte _min = 0, byte _bt_mul_up = 1) {
    min = _min;
    max = _max;
    display_value = _display_value;
    flags = _flags;
    bt_mul[0] = _bt_mul_up;
    bt_mul[1] = _bt_mul_down;
  }
  byte min, max;
  byte display_value;   // sent to run_event with enc, 0xFF is None
  byte flags;           // See ENCODER_FLAGS_<X> #defines
  byte bt_mul[2];       // button: up, down
} var_type_t;
 
// Every parameter has one of these:
typedef struct {
  var_type_t *var_type;
  byte value;
  byte changed;         // set to 1 each time value changes.  Used by triggers, which resets it.
} variable_t;

// Every encoder has one of these:
typedef struct {
  byte A_sw;            // A_sw, B_sw, Button are consecutive column numbers
  byte display_num;     // numeric disp# (ignored when ENCODER_FLAGS_CHOICE_LEDS is set)
  byte encoder_event;   // process encoder changes (display_value event updates display) 0xFF is None
  signed char count;
  variable_t *var;      // disabled if NULL
} encoder_t;

typedef struct choices_s: var_type_s {
  choices_s(byte _choices_num, byte _choices_length, byte _bt_mul_down = 1, byte _additional_flags = 0)
    : var_type_s(_choices_length - 1, UPDATE_CHOICES, _additional_flags | ENCODER_FLAGS_CHOICE_LEDS, _bt_mul_down)
  {
    choices_num = _choices_num;
  }
  byte choices_num;
} choices_t;

typedef struct linear_number_s: var_type_s {
  linear_number_s(byte _min, byte _max, long _offset = 0, byte _bt_mul_down = 1, byte _dp = 0, long _scale = 1, byte _trim = 0, 
                  byte _flags = 0)
    : var_type_s(_max, UPDATE_LINEAR_NUM, _flags, _bt_mul_down, _min)
  {
    scale = _scale;
    offset = _offset;
    dp = _dp;
    trim = _trim;
  }
  long scale;  // encoder value multiplied by 10^dp, then divided by scale (rounded)
  long offset;
  byte dp;     // decimal point position
  byte trim;   // true/false, if true leading digits are trimmed.
} linear_number_t;

typedef struct geometric_number_s: var_type_s {
  long scale;  // encoder value multiplied by 10^dp, then divided by scale (rounded)
  long offset;
  byte dp;     // decimal point position
  byte trim;   // true/false, if true leading digits are trimmed.
} geometric_number_t;

typedef struct note_s: var_type_s {
  // C, "C#", D, Eb, E, F, "F#", G, Ab, A, Bb, B
  // Eb, Ab
  byte include_null;
} note_t;

#define NUM_ENCODERS    6

extern encoder_t Encoders[NUM_ENCODERS];

#define FUNCTION_ENCODER   4
#define FILENAME_ENCODER   5

#define FUNCTION   (Encoders[FUNCTION_ENCODER].var->value)

extern var_type_t Disabled;
extern void select_led(byte enc);
extern void turn_off_choices_leds(byte enc);
extern void display_number(byte display_num, unsigned short num, byte dp);
extern void clear_numeric_display(byte display_num);
extern void display_linear_number(byte enc);
extern void display_geometric_number(byte enc);
extern void display_note(byte enc);

extern byte setup_encoders(byte EEPROM_offset);

// vim: sw=2
