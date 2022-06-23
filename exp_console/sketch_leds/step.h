// step.h

#define NUM_COLS            16
#define NUM_ROWS            16

#define EEPROM_Num_rows         0

typedef struct {   // size 4 bytes, representing 16 bits (cols)
  byte port_d;  // 7, 5-0 -> COL_8, COL_10 to COL_15
  byte port_b;  // 2-0    -> COL_5 to COL_7
  byte port_c;  // 6-4    -> COL_1 to COL_3
  byte port_e;  // 3,1,0  -> COL_4, COL_0, COL_9
} col_ports_t;

extern col_ports_t Col_ports[NUM_ROWS];   // 64 bytes

extern byte Num_rows;

extern byte setup_step(void);   // return EEPROM needed
extern void step(byte who_dunnit_errno);

extern void led_on(byte bit_num);
extern void led_off(byte bit_num);

extern byte load_8(byte bits, byte byte_num);
extern byte load_16(unsigned short bits, byte word_num);

// vim: sw=2
