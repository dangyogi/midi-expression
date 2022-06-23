// events.h

extern byte setup_events(byte EEPROM_offset);

extern void run_event(byte event_num, byte param);

extern byte Switch_closed_event[NUM_SWITCHES];
extern byte Switch_opened_event[NUM_SWITCHES];

extern void switch_closed(byte sw);
extern void switch_opened(byte sw);

// vim: sw=2
