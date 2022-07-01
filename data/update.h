// update.h

{#  Template expects:
    
    num_triggers
    max_commands_per_param
    num_parameters

    num_lin_ptypes
    num_geom_ptypes
    num_constant_ptypes
    num_choices_ptypes
    max_choice_leds

    num_commands
#}

#define MAX_COMMANDS_PER_PARAM          ( {{ max_commands_per_param }} )
#define NUM_PARAMETERS                  ( {{ num_parameters }} )
#define NUM_COMMANDS                    ( {{ num_commands }} )

extern byte First_changed_param;       // NULL if none changed.

#define PTYPE_LIN               0
#define PTYPE_GEOM              1
#define PTYPE_CONSTANT          2
#define PTYPE_CHOICES           3

#define NUM_LIN_PTYPES          ( {{ num_lin_ptypes }} )
#define NUM_GEOM_PTYPES         ( {{ num_geom_ptypes }} )
#define NUM_CONSTANT_PTYPES     ( {{ num_constant_ptypes }} )
#define NUM_CHOICES_PTYPES      ( {{ num_choices_ptypes }} )

typedef struct {
  byte type;                    // see PTYPE_<X> #defines
  byte ptype_num;
  byte bits;
  byte value_bit_offset;        // how many bits to shift left
  byte next_changed_param;      // 0xff == not changed
  byte first_command_function;
  byte num_commands;
  byte commands[MAX_COMMANDS_PER_PARAM];        // Arranged by function, starting at
                                                // first_command_function, for num_commands.
  byte new_raw_value;
  byte current_raw_value;
} param_t;

extern param_t Params[NUM_PARAMETERS];

extern void param_changed(byte param_num, byte new_raw_value);

typedef struct {
  // value = step_size*param + start
  unsigned short start;
  unsigned short step_size;
  unsigned short limit;
  byte bits;
  byte decimal_pt;
} lin_ptype_t;

extern lin_ptype_t Lin_ptypes[NUM_LIN_PTYPES];

typedef struct {
  // value = e**(m*param + b) + c
  unsigned short m;
  unsigned short b;
  unsigned short c;
  unsigned short start;
  unsigned short limit;
  byte bits;
  byte decimal_pt;
} geom_ptype_t;

extern geom_ptype_t Geom_ptypes[NUM_GEOM_PTYPES];

typedef struct {
  unsigned short value;
  byte bits;
} constant_ptype_t;

extern constant_ptype_t Constant_ptypes[NUM_CONSTANT_PTYPES];

#define CHOICE_TYPE_LEDS        0
#define CHOICE_TYPE_NOTES       1
#define MAX_CHOICE_LEDS         ( {{ max_choice_leds }} )

typedef struct {
  byte type;
  byte bits;
  byte null_value;                     // 0xff is none
  byte limit;
  byte num_leds;
  byte led[MAX_CHOICE_LEDS];
} choices_ptype_t;

extern choices_ptype_t Choices_ptypes[NUM_CHOICES_PTYPES];

extern byte First_changed_command;     // NULL if none changed.

typedef struct {
  unsigned short MIDI_value;
  byte next_changed_command;           // 0xff == not changed
} command_t;

extern command_t Commands[NUM_COMMANDS];

extern void command_changed(byte command_num, unsigned short and_value,
                            unsigned short or_value);

typedef struct {
  byte sw;
  byte led;     // on iff not SAVE_OR_SYNTH and sw on
  byte bt;
  byte state;   // 0 - Manual-idle, 1 - Manual-triggered, 2 - stream,
                // 3 - save (only bt works for save, led off)
} trigger_t;

#define TRIGGER_STATE_MANUAL_IDLE       0
#define TRIGGER_STATE_MANUAL_TRIGGERED  1
#define TRIGGER_STATE_STREAM            2
#define TRIGGER_STATE_SAVE              3

#define NUM_TRIGGERS            ( {{ num_triggers }} )

#define NEXT_MULTIPLE(n, m)     (((n) + (m) - 1) / (m) * (m))
#define FIRST_TRIGGER_SW_EVENT  NEXT_MULTIPLE(NEXT_AVAIL_EVENT, NUM_TRIGGERS)
#define TRIGGER_SW_CLOSED(t)    (FIRST_TRIGGER_SW_EVENT + (t))
#define TRIGGER_SW_OPENED(t)    (FIRST_TRIGGER_SW_EVENT + NUM_TRIGGERS + (t))
#define TRIGGER_BT_CLOSED(t)    (FIRST_TRIGGER_SW_EVENT + 2*NUM_TRIGGERS + (t))

#define TRIGGER_CASES                               \
    {% for i in range(num_triggers) %}
    case TRIGGER_SW_CLOSED({{ i }}):                   \
    {% endfor %}
      enc = event_num % ( {{ num_triggers }} );           \
      if (!SAVE_OR_SYNTH) {                         \
        Triggers[enc].state = TRIGGER_STATE_STREAM; \
        FIX: Turn on LED
      }                                             \
      break;                                        \
    {% for i in range(num_triggers) %}
    case TRIGGER_SW_OPENED({{ i }}):                        \
      enc = event_num % ( {{ num_triggers }} );          \
      if (Triggers[enc].state == TRIGGER_STATE_STREAM) { \
        Triggers[enc].state = TRIGGER_STATE_MANUAL_IDLE; \
        FIX: Turn off LED
      }                                                  \
      break;                                             \
    {% endfor %}
    {% for i in range(num_triggers) %}
    case TRIGGER_BT_CLOSED({{ i }}): \
    {% endfor %}
      enc = event_num % ( {{ num_triggers }} );    \
      if (SAVE_OR_SYNTH) {                              \
        FIX save to program
      } else if (Triggers[enc].state == TRIGGER_STATE_MANUAL_IDLE) { \
        Triggers[enc].state = TRIGGER_STATE_MANUAL_TRIGGERED;        \
      }                                                              \
      break;


extern trigger_t Triggers[NUM_TRIGGERS];

extern byte setup_update(byte EEPROM_offset);

extern void update(void);


// vim: sw=2
