// step.h

#define NUM_COLS            16
#define NUM_ROWS            16

#define EEPROM_Num_rows         0

typedef struct {   // size 4 bytes, representing 16 bits (cols)
  byte port_d;  // 7, 5-0 -> COL_15, COL_9, COL_10, COL_14 to COL_11
  byte port_b;  // 2-0    -> COL_3, COL_6, COL_5
  byte port_c;  // 6-4    -> COL_2 to COL_0
  byte port_e;  // 3,1,0  -> COL_4, COL_8, COL_7
} col_ports_t;

extern col_ports_t Col_ports[NUM_ROWS];   // 64 bytes

extern byte Num_rows;
extern byte Current_row;

extern byte setup_step(void);   // return EEPROM needed
extern void step(byte who_dunnit_errno);

extern void led_on(byte bit_num);
extern void led_off(byte bit_num);

extern unsigned short test_led_order(void);

extern byte load_8(byte bits, byte byte_num);
extern byte load_16(unsigned short bits, byte row_num);

extern void turn_off_all_columns(void);
extern void turn_on_column(byte col);

extern void turn_on_first_row(void);
extern void turn_on_last_row(void);
extern void turn_on_next_row(void);

// vim: sw=2
