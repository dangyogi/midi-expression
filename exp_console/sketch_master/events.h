// events.h

// Events for Switch_closed_events, Switch_opened_events and Encoder display_value and encoder_events.
#define ENC_A_CLOSED(n)                    (n)
#define ENC_B_CLOSED(n)                    (NUM_ENCODERS + (n))
#define ENC_A_OPENED(n)                    (2 * NUM_ENCODERS + (n))
#define ENC_B_OPENED(n)                    (3 * NUM_ENCODERS + (n))
#define EVENT_ENCODER_NUM(ev)              ((ev) % NUM_ENCODERS)
#define FUNCTION_CHANGED                   24
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
#define UPDATE_CHOICES                     35
#define UPDATE_LINEAR_NUM                  36
#define UPDATE_GEOMETRIC_NUM               37
#define UPDATE_NOTE                        38
#define UPDATE_SHARPS_FLATS                39
#define TRIGGER_SW_ON                      40
#define TRIGGER_SW_OFF                     41
#define TRIGGER_BT_PRESSED                 42
#define TRIGGER_BT_RELEASED                43
#define CHECK_POTS                         44
#define CHECK_FUNCTIONS                    45
#define FUN_PARAM_CHANGED                  46
#define NEXT_AVAIL_EVENT                   47

extern byte Trace_events;
extern byte Trace_encoders;

extern byte setup_events(byte EEPROM_offset);

extern void run_event(byte event_num, byte param);
extern void encoder_changed(byte enc);

// These are set in the various setup routines and never change after that.
extern byte Switch_closed_event[NUM_SWITCHES];
extern byte Switch_opened_event[NUM_SWITCHES];

extern void switch_closed(byte sw);
extern void switch_opened(byte sw);


// vim: sw=2
