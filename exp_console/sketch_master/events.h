// events.h

// Events for Switch_closed_events, Switch_opened_events and Encoder_events.
#define ENC_A_CLOSED(n)                    (n)
#define ENC_B_CLOSED(n)                    (NUM_ENCODERS + (n))
#define ENC_A_OPENED(n)                    (2 * NUM_ENCODERS + (n))
#define ENC_B_OPENED(n)                    (3 * NUM_ENCODERS + (n))
#define EVENT_ENCODER_NUM(ev)              ((ev) % NUM_ENCODERS)
#define SYNTH_PROGRAM_OR_FUNCTION_CHANGED  24
#define NOTE_BT_ON                         25
#define NOTE_BT_OFF                        26
#define NOTE_SW_ON                         27
#define NOTE_SW_OFF                        28
#define CONTINUOUS                         29
#define PULSE                              30
#define CHANNEL_ON                         31
#define CHANNEL_OFF                        32
#define HARMONIC_ON                        33
#define HARMONIC_OFF                       34
#define NEXT_AVAIL_EVENT                   35

extern byte Trace_events;

extern byte setup_events(byte EEPROM_offset);

extern void run_event(byte event_num, byte param);

extern byte Switch_closed_event[NUM_SWITCHES];
extern byte Switch_opened_event[NUM_SWITCHES];

extern void switch_closed(byte sw);
extern void switch_opened(byte sw);

extern byte Encoder_event[NUM_ENCODERS];

// vim: sw=2
