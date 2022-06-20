// step.ino

#include "step.h"

#define COUNTER_MR_NOT  6     /* PF4 */
#define COUNTER_CP      3     /* PF5 */

// These go from the MSB of bits passed from on high, to the LSB:
//#define COL_0           12    /* PE1 */
//#define COL_1            4    /* PC6 */
//#define COL_2            0    /* PC5 */
//#define COL_3            1    /* PC4 */
//#define COL_4            8    /* PE3 */
//#define COL_5            5    /* PB2 */
//#define COL_6           10    /* PB1 */
//#define COL_7            9    /* PB0 */

//#define COL_8           AREF  /* PD7 */
//#define COL_9           11    /* PE0 */
//#define COL_10          A7    /* PD5 */
//#define COL_11          A6    /* PD4 */
//#define COL_12          A0    /* PD3 */
//#define COL_13          A1    /* PD2 */
//#define COL_14          A2    /* PD1 */
//#define COL_15          A3    /* PD0 */

byte Num_rows;

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
  
  pinMode(COUNTER_MR_NOT, OUTPUT);
  digitalWrite(COUNTER_MR_NOT, LOW);
  pinMode(COUNTER_CP, OUTPUT);
  digitalWrite(COUNTER_CP, HIGH);

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
  } else if (b > NUM_ROWS) {
    Errno = 100;
    Err_data = b; 
  } else Num_rows = b;

  return 1;
} // end setup_step()

byte load_8(byte bits, byte byte_num) {
  // The MSB of bits goes into the first column, and the LSB into the last column.
  // Returns 1 if successful, 0 on invalid byte_num.
  if (byte_num >= 2*Num_rows) return 0;
  if (byte_num & 1) {
    // high byte, COL_8 to COL_15
    byte_num >> 1;   // translate to Row number

    // byte port_d;  // 7, 5-0 -> COL_8, COL_10 to COL_15
    // byte port_b;  // 2-0    -> COL_5 to COL_7
    // byte port_c;  // 6-4    -> COL_1 to COL_3
    // byte port_e;  // 3,1,0  -> COL_4, COL_0, COL_9

    // load bits 7, 5-0 -> COL_8, COL_10 to COL_15
    Col_ports[byte_num].port_d = bits & 0xBF;

    // load bit 6 -> COL_9
    if (bits & 0x40) {
      Col_ports[byte_num].port_e |= 0x40;     
    } else {
      Col_ports[byte_num].port_e &= ~0x40;
    }  
  } else {
    // low byte, COL_0 to COL_7
    byte_num >> 1;   // translate to Row number

    // load bits 2-0, COL_5 to COL_7
    Col_ports[byte_num].port_b &= ~0x07;
    Col_ports[byte_num].port_b |= bits & 0x07;
    
    // load bit 3, COL_4
    if (bits & 0x08) {
      Col_ports[byte_num].port_e |= 0x08;     
    } else {
      Col_ports[byte_num].port_e &= ~0x08;
    }  
    
    // load bit 6-4, COL_1 to COL_3
    Col_ports[byte_num].port_c &= ~0x70;
    Col_ports[byte_num].port_c |= bits | 0x70;
    
    // load bit 7, COL_0
    if (bits & 0x80) {
      Col_ports[byte_num].port_e |= 0x02;     
    } else {
      Col_ports[byte_num].port_e &= ~0x02;
    }  
  }
  return 1;
}

byte load_16(unsigned short bits, byte word_num) {
  // The MSB of bits goes into the first column, and the LSB into the last column.
  // Returns 1 if successful, 0 on invalid word_num.
  word_num <<= 1;   // translate to byte_num

  return load_8(byte(bits >> 8), word_num) && load_8(byte(bits), word_num + 1);
}

unsigned short On_start, On_end;

byte Current_row = NUM_ROWS - 1;

extern void timeout(void);      // defined elsewhere...

void step(byte who_dunnit_errno) {
  // caller has 100 uSec (1600 processor clock cycles) to call this after prior call.
  // caller identifies himself by a who_dunnit_errno.  Err_data will be time in excess
  // in 0.01 mSecs.
  
  unsigned short now = (unsigned short)micros();
  if (On_end > On_start) {
    if (now >= On_end || now < On_start) {
      // naughty!!!
      Errno = who_dunnit_errno;
    } else {
      // wait for On_end time...
      now = (unsigned short)micros();
      while (!(now >= On_end || now < On_start)) ;
    }
  } else {
    if (now >= On_end && now < On_start) {
      // naughty!!!
      Errno = who_dunnit_errno;
    } else {
      // wait for On_end time...
      now = (unsigned short)micros();
      while (!(now >= On_end && now < On_start)) ;
    }
  }

  // Turn off all Columns:
  PORTB.OUTCLR = All_leds_off.port_b;
  PORTC.OUTCLR = All_leds_off.port_c;
  PORTD.OUTCLR = All_leds_off.port_d;
  PORTE.OUTCLR = All_leds_off.port_e;

  // Turn on next Row:
  if (Current_row >= Num_rows) {
    // 1 processor clock cycle is 62.5 nSec
    digitalWrite(COUNTER_MR_NOT, LOW);  // 30nSec pulse width
    Current_row = 0;
    digitalWrite(COUNTER_MR_NOT, HIGH);
  } else {
    digitalWrite(COUNTER_CP, HIGH); // 25nSec pulse width
    Current_row++;
    digitalWrite(COUNTER_CP, LOW);
  }

  if (Errno == who_dunnit_errno) {
    Err_data = (now - On_end) / 10;
  }

  // Does anything that can't be broken into steps < 100 uSec while the LEDs are off.
  // This should get called about every 1.6 mSec.
  timeout();

  // Turn on LED Columns for this Row:
  On_start = (unsigned short)micros();
  PORTB.OUTSET = Col_ports[Current_row].port_b;
  PORTC.OUTSET = Col_ports[Current_row].port_c;
  PORTD.OUTSET = Col_ports[Current_row].port_d;
  PORTE.OUTSET = Col_ports[Current_row].port_e;
  On_end = On_start + 100;
}

