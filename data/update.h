// update.h

{#  Template expects:
    
    num_triggers
    num_internal_params
    num_pot_params
    num_pot_commands
    num_display_params

    num_lin_ptypes
    num_geom_ptypes
    num_choice_notes_ptypes
    num_choice_leds_ptypes
    max_choice_leds

    num_single_commands
    num_channel_commands
    num_harmonic_commands
#}

// param is dependent on function, but independent of channel and harmonic

#define NUM_INTERNAL_PARAMS         ( {{ num_internal_params }} )
#define NUM_POT_PARAMS              ( {{ num_pot_params }} )
#define NUM_POT_COMMANDS            ( {{ num_pot_commands }} )
#define NUM_DISPLAY_PARAMS          ( {{ num_display_params }} )

#define NUM_LIN_PTYPES              ( {{ num_lin_ptypes }} )
#define NUM_GEOM_PTYPES             ( {{ num_geom_ptypes }} )
#define NUM_CHOICE_NOTES_PTYPES     ( {{ num_choice_notes_ptypes }} )
#define NUM_CHOICE_LEDS_PTYPES      ( {{ num_choice_leds_ptypes }} )
#define MAX_CHOICE_LEDS             ( {{ max_choice_leds }} )

#define PTYPE_CONSTANT          0
#define PTYPE_CHANNEL           1
#define PTYPE_HARMONIC          2
#define PTYPE_LIN               3
#define PTYPE_GEOM              4
#define PTYPE_CHOICE_NOTES      5
#define PTYPE_CHOICE_LEDS       6

// internal params: PTYPE_CONSTANT, PTYPE_CHANNEL, PTYPE_HARMONIC
struct internal_param_s {
  byte type;              // see PTYPE_<X> #defines
  byte value_bit_offset;  // has value for PTYPE_CONSTANT (offset always 0 in that case)
                          // value for PTYPE_CHANNEL and PTYPE_HARMONIC not stored on param.
  byte bits;
};

extern void update_cmd_value(byte value, internal_param_s *param, single_command_s *cmd);

extern internal_param_s Internal_params[NUM_INTERNAL_PARAMS];

struct param_s: internal_param_s {
  byte new_value;
  byte current_setting_value;
};

struct pot_param_s: param_s {  // value not displayed or saved
  byte first_command;     // index into Pot_commands
  byte num_commands;      // in Pot_commands
};

extern pot_param_s Pot_params[NUM_POT_PARAMS];
extern byte Pot_commands[NUM_POT_COMMANDS];

struct display_param_s: param_s {
  byte ptype_num;
  byte current_display_value;
};

extern display_param_s Display_params[NUM_DISPLAY_PARAMS];
extern byte Function_commands[NUM_FUNCTIONS];

#define PARAM_TYPE_MASK         0xC0
#define PARAM_TYPE_SHIFT        6
#define INTERNAL_PARAM_TYPE     0
#define POT_PARAM_TYPE          1
#define DISPLAY_PARAM_TYPE      2

#define PARAM_TYPE(param_num)   ((param_num & PARAM_TYPE_MASK) >> PARAM_TYPE_SHIFT)
#define PARAM_NUM(param_num)    (param_num & ~PARAM_TYPE_MASK)

extern internal_param_s *get_internal_param(byte param_num);  // works for all PARAM_TYPEs
extern pot_param_s *get_pot_param(byte param_num); // does not work for DISPLAY_PARAM_TYPE
extern display_param_s *get_display_param(byte param_num);

extern void param_changed(byte param_num, byte new_value, byte display_num=0xff);


// ptypes are independent of function, channel and harmonic

typedef struct {
  // value = step_size*param + start
  unsigned short start;
  unsigned short step_size;
  unsigned short limit;
  byte decimal_pt;
} lin_ptype_t;

extern lin_ptype_t Lin_ptypes[NUM_LIN_PTYPES];

typedef struct {
  // value = e**(m*param + b) + c
  // value = pow(base, param + b2) + c, where base = e**m, and base**b2 == e**b
  //                                 so b2 == log(base, e**b) == log(e**m, e**b) == b/m
  float base;
  float b2;
  float c;
  float start;
  float limit;
  byte decimal_pt;
} geom_ptype_t;

extern geom_ptype_t Geom_ptypes[NUM_GEOM_PTYPES];

struct choice_notes_s {
  byte null_value;                     // 0xff is none
  byte limit;
};

extern choice_notes_s Choice_notes_ptypes[NUM_CHOICE_NOTES_PTYPES];

struct choice_leds_s: choice_notes_s {
  byte num_leds;
  byte led[MAX_CHOICE_LEDS];
};

extern choice_leds_s Choice_leds_ptypes[NUM_CHOICE_LEDS_PTYPES];


// command is dependent on function, but independent of channel and harmonic

#define NUM_SINGLE_COMMANDS     ( {{ num_single_commands }} )
#define NUM_CHANNEL_COMMANDS    ( {{ num_channel_commands }} )
#define NUM_HARMONIC_COMMANDS   ( {{ num_harmonic_commands }} )

extern byte First_changed_command;     // NULL if none changed.

#define COMMAND_BIT_NR_PARAM           0x01
#define COMMAND_BIT_KEY_ON             0x02
#define COMMAND_BIT_ADD                0x04
#define COMMAND_BIT_SYNTH_F4           0x08
#define COMMAND_BIT_SYNTH_F4_F5        0x10
#define COMMAND_BIT_PLAYER_CHANNEL     0x20

struct single_command_s {
  unsigned long MIDI_value;
  unsigned short MIDI_code;
  byte flags;                          // bits: see COMMAND_BIT_<X> #defines
  byte next_changed_command;           // 0xff == not changed
};

extern single_command_s Single_commands[NUM_SINGLE_COMMANDS];

struct channel_command_s: single_command_s {
  byte trigger_num;
  byte channel_param;
};

extern channel_command_s Channel_commands[NUM_CHANNEL_COMMANDS];

// none of these are synth
struct harmonic_command_s: channel_command_s {
  byte harmonic_param;
};

extern harmonic_command_s Harmonic_commands[NUM_HARMONIC_COMMANDS];

#define COMMAND_TYPE_MASK         0xC0
#define COMMAND_TYPE_SHIFT        6
#define SINGLE_COMMAND_TYPE       0
#define CHANNEL_COMMAND_TYPE      1
#define HARMONIC_COMMAND_TYPE     2

#define COMMAND_TYPE(command_num)   ((command_num & COMMAND_TYPE_MASK) >> COMMAND_TYPE_SHIFT)
#define COMMAND_NUM(command_num)    (command_num & ~COMMAND_TYPE_MASK)

extern single_command_s *get_single_command(byte command_num);
extern channel_command_s *get_channel_command(byte command_num);
extern harmonic_command_s *get_harmonic_command(byte command_num);

extern void command_changed(byte command_num, byte value, internal_param_s *param);

// trigger is independent of function, channel and harmonic

typedef struct {
  byte sw;
  byte led;     // on iff not SAVE_OR_SYNTH and sw on
  byte bt;
  byte flags;   // bit map: 1 - triggered, 2 - stream/Manual, 4 - save/synth, 8 - led on
} trigger_t;

#define TRIGGER_FLAGS_TRIGGERED         1
#define TRIGGER_FLAGS_STREAM_MANUAL     2
#define TRIGGER_FLAGS_SAVE_SYNTH        4
#define TRIGGER_FLAGS_LED_ON            8

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
