// step.ino

#define COUNTER_MR_NOT     6    /* PF4 */
#define COUNTER_CP         3    /* PF5 */
#define ROWS_ENABLE_NOT    2    /* PA0 */

// These go from the MSB of bits passed from on high, to the LSB:
//#define COL_0            1    /* PC4 */
//#define COL_1            0    /* PC5 */
//#define COL_2            4    /* PC6 */
//#define COL_3            5    /* PB2 */
//#define COL_4            8    /* PE3 */
//#define COL_5            9    /* PB0 */
//#define COL_6           10    /* PB1 */
//#define COL_7           11    /* PE0 */

//#define COL_8           12    /* PE1 */
//#define COL_9           A7    /* PD5 */
//#define COL_10          A6    /* PD4 */
//#define COL_11          A3    /* PD0 */
//#define COL_12          A2    /* PD1 */
//#define COL_13          A1    /* PD2 */
//#define COL_14          A0    /* PD3 */
//#define COL_15          AREF  /* PD7 */

byte Num_rows;

/***
typedef struct {   // size 4 bytes, representing 16 bits (cols)
  byte port_d;  // 7, 5-0 -> COL_15, COL_9, COL_10, COL_14 to COL_11
  byte port_b;  // 2-0    -> COL_3, COL_6, COL_5
  byte port_c;  // 6-4    -> COL_2 to COL_0
  byte port_e;  // 3,1,0  -> COL_4, COL_8, COL_7
} col_ports_t;
***/

col_ports_t Col_ports[NUM_ROWS];   // 64 bytes

const col_ports_t All_leds_off = {
  //76543210
  0B10111111,  // PORTD
  0B00000111,  // PORTB
  0B01110000,  // PORTC
  0B00001011,  // PORTE
};

byte setup_step(void) {
  // put your setup code here, to run once:
  
  pinMode(ROWS_ENABLE_NOT, OUTPUT);
  digitalWrite(ROWS_ENABLE_NOT, HIGH);	// disable all rows
  pinMode(COUNTER_MR_NOT, OUTPUT);
  digitalWrite(COUNTER_MR_NOT, HIGH);
  pinMode(COUNTER_CP, OUTPUT);
  digitalWrite(COUNTER_CP, LOW);


  // Set Row pins to OUTPUT mode, and set all outputs to LOW
  PORTB.DIRSET = All_leds_off.port_b;
  PORTB.OUTCLR = All_leds_off.port_b;
  PORTC.DIRSET = All_leds_off.port_c;
  PORTC.OUTCLR = All_leds_off.port_c;
  PORTD.DIRSET = All_leds_off.port_d;
  PORTD.OUTCLR = All_leds_off.port_d;
  PORTE.DIRSET = All_leds_off.port_e;
  PORTE.OUTCLR = All_leds_off.port_e;

  byte b = EEPROM[EEPROM_Num_rows];
  if (b == 0xFF) {
    Serial.println("Num_rows not set in EEPROM");
    Num_rows = NUM_ROWS;
  } else if (b > NUM_ROWS) {
    Errno = 71;
    Err_data = b; 
  } else Num_rows = b;

  return 1;
} // end setup_step()

void led_on(byte bit_num) {
  if (bit_num >= Num_rows * NUM_COLS) {
    Errno = 72;
    Err_data = bit_num;
    return;
  }
  byte row = bit_num >> 4;
  byte col = bit_num & 0x0F;
  switch (col) {                   // 76543210
  case  0: Col_ports[row].port_c |= 0b00010000; break;
  case  1: Col_ports[row].port_c |= 0b00100000; break;
  case  2: Col_ports[row].port_c |= 0b01000000; break;
  case  3: Col_ports[row].port_b |= 0b00000100; break;
  case  4: Col_ports[row].port_e |= 0b00001000; break;
  case  5: Col_ports[row].port_b |= 0b00000001; break;
  case  6: Col_ports[row].port_b |= 0b00000010; break;
  case  7: Col_ports[row].port_e |= 0b00000001; break;
  case  8: Col_ports[row].port_e |= 0b00000010; break;
  case  9: Col_ports[row].port_d |= 0b00100000; break;
  case 10: Col_ports[row].port_d |= 0b00010000; break;
  case 11: Col_ports[row].port_d |= 0b00000001; break;
  case 12: Col_ports[row].port_d |= 0b00000010; break;
  case 13: Col_ports[row].port_d |= 0b00000100; break;
  case 14: Col_ports[row].port_d |= 0b00001000; break;
  case 15: Col_ports[row].port_d |= 0b10000000; break;
  } // end switch (col)
}

void led_off(byte bit_num) {
  if (bit_num >= Num_rows * NUM_COLS) {
    Errno = 73;
    Err_data = bit_num;
    return;
  }
  byte row = bit_num >> 4;
  byte col = bit_num & 0x0F;
  switch (col) {                    // 76543210
  case  0: Col_ports[row].port_c &= ~0b00010000; break;
  case  1: Col_ports[row].port_c &= ~0b00100000; break;
  case  2: Col_ports[row].port_c &= ~0b01000000; break;
  case  3: Col_ports[row].port_b &= ~0b00000100; break;
  case  4: Col_ports[row].port_e &= ~0b00001000; break;
  case  5: Col_ports[row].port_b &= ~0b00000001; break;
  case  6: Col_ports[row].port_b &= ~0b00000010; break;
  case  7: Col_ports[row].port_e &= ~0b00000001; break;
  case  8: Col_ports[row].port_e &= ~0b00000010; break;
  case  9: Col_ports[row].port_d &= ~0b00100000; break;
  case 10: Col_ports[row].port_d &= ~0b00010000; break;
  case 11: Col_ports[row].port_d &= ~0b00000001; break;
  case 12: Col_ports[row].port_d &= ~0b00000010; break;
  case 13: Col_ports[row].port_d &= ~0b00000100; break;
  case 14: Col_ports[row].port_d &= ~0b00001000; break;
  case 15: Col_ports[row].port_d &= ~0b10000000; break;
  } // end switch (col)
}

#define LED_ORDER_SCROLL_RATE     250   /* mSec */

byte Current_led;
byte Leds_lit;

// Just set Timeout_fun_runtime[TEST_LED_ORDER] = 1 to start test.
unsigned short test_led_order(void) {
  if (Leds_lit) {
    led_off(Current_led++);
  }
  if (Leds_lit >= Num_rows * NUM_COLS) {
    Current_led = 0;    // get ready for next time!
    Leds_lit = 0;
    return 0;           // Done!
  }
  led_on(Current_led);
  Leds_lit++;
  return LED_ORDER_SCROLL_RATE;
}

byte load_8(byte bits, byte byte_num) {
  // The MSB of bits goes into the first column, and the LSB into the last column.
  // Returns 1 if successful, 0 on invalid byte_num.
  if (byte_num & 1) {
    return load_high_8(bits, byte_num >> 1);
  }
  return load_low_8(bits, byte_num >> 1);
}

// This file is generated by gen_code.py
#include "decode_masks.h"

// const col_ports_t Decode_high[] PROGMEM = {    // The unused bits in each port are always 0 here.
//  {0b76543210, 0b76543210, 0b76543210, 0b76543210},
//  {0b76543210, 0b76543210, 0b76543210, 0b76543210}, 
//  ...
//};
//
//const col_ports_t Masks_high = {   // 1 bits are not used by any of the high columns
//  0b76543210, 0b76543210, 0b76543210, 0b76543210, 
//};

byte Decode_byte;

#define load_port(port, high_low)                                       \
  Decode_byte = pgm_read_byte_near(&Decode_##high_low[bits].port);      \
  Col_ports[row_num].port |= Decode_byte;                               \
  Col_ports[row_num].port &= (Decode_byte | Masks_##high_low.port);

byte load_high_8(byte bits, byte row_num) {
  if (row_num >= Num_rows) return 0;

  load_port(port_d, high)
  load_port(port_b, high)
  load_port(port_c, high)
  load_port(port_e, high)

  return 1;
}

byte load_low_8(byte bits, byte row_num) {
  if (row_num >= Num_rows) return 0;

  load_port(port_d, low)
  load_port(port_b, low)
  load_port(port_c, low)
  load_port(port_e, low)

  return 1;
}

byte load_16(unsigned short bits, byte row_num) {
  // The MSB of bits goes into the first column, and the LSB into the last column.
  // Returns 1 if successful, 0 on invalid row_num.
  return load_high_8(byte(bits), row_num) && load_low_8(byte(bits >> 8), row_num);
}

unsigned short On_start, On_end = 10000;

byte Current_row = NUM_ROWS;

#define TURN_OFF_ALL_COLUMNS() \
  PORTB.OUTCLR = All_leds_off.port_b; \
  PORTC.OUTCLR = All_leds_off.port_c; \
  PORTD.OUTCLR = All_leds_off.port_d; \
  PORTE.OUTCLR = All_leds_off.port_e

void turn_off_all_columns(void) {
  TURN_OFF_ALL_COLUMNS();
}

void turn_on_column(byte col) {
  switch (col) {         // 76543210
  case  0: PORTC.OUTSET = 0b00010000; break;
  case  1: PORTC.OUTSET = 0b00100000; break;
  case  2: PORTC.OUTSET = 0b01000000; break;
  case  3: PORTB.OUTSET = 0b00000100; break;
  case  4: PORTE.OUTSET = 0b00001000; break;
  case  5: PORTB.OUTSET = 0b00000001; break;
  case  6: PORTB.OUTSET = 0b00000010; break;
  case  7: PORTE.OUTSET = 0b00000001; break;
  case  8: PORTE.OUTSET = 0b00000010; break;
  case  9: PORTD.OUTSET = 0b00100000; break;
  case 10: PORTD.OUTSET = 0b00010000; break;
  case 11: PORTD.OUTSET = 0b00000001; break;
  case 12: PORTD.OUTSET = 0b00000010; break;
  case 13: PORTD.OUTSET = 0b00000100; break;
  case 14: PORTD.OUTSET = 0b00001000; break;
  case 15: PORTD.OUTSET = 0b10000000; break;
  } // end switch (col)
}

#define TURN_ON_LED_COLUMNS(row)        \
  PORTB.OUTSET = Col_ports[row].port_b; \
  PORTC.OUTSET = Col_ports[row].port_c; \
  PORTD.OUTSET = Col_ports[row].port_d; \
  PORTE.OUTSET = Col_ports[row].port_e

void turn_on_led_columns(byte row) {
  TURN_ON_LED_COLUMNS(row);
}

void clear_row(byte row) {
  Col_ports[row].port_b = 0;
  Col_ports[row].port_c = 0;
  Col_ports[row].port_d = 0;
  Col_ports[row].port_e = 0;
}

#define DISABLE_ALL_ROWS()  \
  digitalWrite(ROWS_ENABLE_NOT, HIGH) 	// disable all rows

#define ENABLE_ALL_ROWS()  \
  digitalWrite(ROWS_ENABLE_NOT, LOW) 	// enable all rows

void disable_all_rows(void) {
  DISABLE_ALL_ROWS();
}

void enable_all_rows(void) {
  ENABLE_ALL_ROWS();
}

#define TURN_ON_FIRST_ROW()                                       \
    /* 1 processor clock cycle is 62.5 nSec */                    \
    digitalWrite(COUNTER_MR_NOT, LOW);  /* 30nSec pulse width */  \
    Current_row = 0;                                              \
    digitalWrite(COUNTER_MR_NOT, HIGH)

void turn_on_first_row(void) {
  TURN_ON_FIRST_ROW();
}

#define TURN_ON_NEXT_ROW(num_rows)                              \
  if (Current_row + 1 >= (num_rows)) {                          \
    TURN_ON_FIRST_ROW();                                        \
  } else {                                                      \
    digitalWrite(COUNTER_CP, HIGH); /* 25nSec pulse width */    \
    Current_row++;                                              \
    digitalWrite(COUNTER_CP, LOW);                              \
  }

void turn_on_next_row(byte num_rows) {
  TURN_ON_NEXT_ROW(num_rows);
}

void step(byte who_dunnit_errno) {
  // caller has 100 uSec (1600 processor clock cycles) to call this after prior call.
  // caller identifies himself by a who_dunnit_errno.  Err_data will be time in excess
  // in 0.01 mSecs.

  // step overhead is 30 uSec/row.
  
  unsigned short now = (unsigned short)micros();
  if (On_end > On_start) {
    if (now >= On_end || now < On_start) {
      // naughty!!!
      Errno = who_dunnit_errno;
    } else {
      // wait for On_end time...
      now = (unsigned short)micros();
      while (!(now >= On_end || now < On_start)) now = (unsigned short)micros();
    }
  } else {
    if (now >= On_end && now < On_start) {
      // naughty!!!
      Errno = who_dunnit_errno;
    } else {
      // wait for On_end time...
      now = (unsigned short)micros();
      while (!(now >= On_end && now < On_start)) now = (unsigned short)micros();
    }
  }

  // Turn off all Rows and Columns:
  DISABLE_ALL_ROWS();
  TURN_OFF_ALL_COLUMNS();

  // Turn on next Row:
  TURN_ON_NEXT_ROW(Num_rows);

  if (Errno == who_dunnit_errno) {
    Err_data = (now - On_end) / 10;
  }

  // Does anything that can't be broken into steps < 100 uSec while the LEDs are off.
  // This will get called about every 1.6 mSec.
  if (Current_row == 0) {
    timeout();
  }

  // Turn on LED Columns for this Row:
  TURN_ON_LED_COLUMNS(Current_row);

  On_start = (unsigned short)micros();
  ENABLE_ALL_ROWS();
  On_end = On_start + 100;
}

// vim: sw=2
