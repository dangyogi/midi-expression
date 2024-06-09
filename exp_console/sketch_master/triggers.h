// triggers.h

#define NUM_POT_TRIGGERS     6
#define NUM_TRIGGERS         7
#define MAX_TRIGGER_POTS     3

typedef struct trigger_s {
  byte switch_;
  byte button;
  byte led;
  byte check_event;
  byte continuous;
} trigger_t;

extern trigger_t Triggers[NUM_TRIGGERS];

extern byte Num_pots[NUM_POT_TRIGGERS];
extern byte Pots[NUM_POT_TRIGGERS][MAX_TRIGGER_POTS];

extern void disable_triggers(void);
extern void changed(trigger_t *trig);
extern void check_triggers(void);        // checks triggers in continous mode
extern void check_trigger(byte trigger); // called when triggered (button pressed) or in continuos mode

extern byte setup_triggers(byte EEPROM_offset);


// vim: sw=2
