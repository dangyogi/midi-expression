// step.ino

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

/***
typedef struct {   // size 4 bytes, representing 16 bits (cols)
  byte port_d;  // 7, 5-0 -> COL_8, COL_10 to COL_15
  byte port_b;  // 2-0    -> COL_5 to COL_7
  byte port_c;  // 6-4    -> COL_1 to COL_3
  byte port_e;  // 3,1,0  -> COL_4, COL_0, COL_9
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

void led_on(byte bit_num) {
  if (bit_num >= Num_rows * NUM_COLS) {
    Errno = 50;
    Err_data = bit_num;
    return;
  }
  byte row = bit_num >> 4;
  byte col = bit_num & 0x0F;
  switch (col) {                  // 76543210
  case 0:  Col_port[row].port_e |= 0b00000010; break;
  case 1:  Col_port[row].port_c |= 0b01000000; break;
  case 2:  Col_port[row].port_c |= 0b00100000; break;
  case 3:  Col_port[row].port_c |= 0b00010000; break;
  case 4:  Col_port[row].port_e |= 0b00001000; break;
  case 5:  Col_port[row].port_b |= 0b00000100; break;
  case 6:  Col_port[row].port_b |= 0b00000010; break;
  case 7:  Col_port[row].port_b |= 0b00000001; break;
  case 8:  Col_port[row].port_d |= 0b10000000; break;
  case 9:  Col_port[row].port_e |= 0b00000001; break;
  case 10: Col_port[row].port_d |= 0b00100000; break;
  case 11: Col_port[row].port_d |= 0b00010000; break;
  case 12: Col_port[row].port_d |= 0b00001000; break;
  case 13: Col_port[row].port_d |= 0b00000100; break;
  case 14: Col_port[row].port_d |= 0b00000010; break;
  case 15: Col_port[row].port_d |= 0b00000001; break;
  } // end switch (col)
}

void led_off(byte bit_num) {
  if (bit_num >= Num_rows * NUM_COLS) {
    Errno = 60;
    Err_data = bit_num;
    return;
  }
  byte row = bit_num >> 4;
  byte col = bit_num & 0x0F;
  switch (col) {                   // 76543210
  case 0:  Col_port[row].port_e &= ~0b00000010; break;
  case 1:  Col_port[row].port_c &= ~0b01000000; break;
  case 2:  Col_port[row].port_c &= ~0b00100000; break;
  case 3:  Col_port[row].port_c &= ~0b00010000; break;
  case 4:  Col_port[row].port_e &= ~0b00001000; break;
  case 5:  Col_port[row].port_b &= ~0b00000100; break;
  case 6:  Col_port[row].port_b &= ~0b00000010; break;
  case 7:  Col_port[row].port_b &= ~0b00000001; break;
  case 8:  Col_port[row].port_d &= ~0b10000000; break;
  case 9:  Col_port[row].port_e &= ~0b00000001; break;
  case 10: Col_port[row].port_d &= ~0b00100000; break;
  case 11: Col_port[row].port_d &= ~0b00010000; break;
  case 12: Col_port[row].port_d &= ~0b00001000; break;
  case 13: Col_port[row].port_d &= ~0b00000100; break;
  case 14: Col_port[row].port_d &= ~0b00000010; break;
  case 15: Col_port[row].port_d &= ~0b00000001; break;
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
  if (byte_num >= Num_rows * NUM_COLS / 8) return 0;
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

  return load_8(byte(bits), word_num) && load_8(byte(bits >> 8), word_num + 1);
}

unsigned short On_start, On_end;

byte Current_row = NUM_ROWS - 1;

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
  // This will get called about every 1.6 mSec.
  if (Current_row == 0) {
    timeout();
  }

  // Turn on LED Columns for this Row:
  On_start = (unsigned short)micros();
  PORTB.OUTSET = Col_ports[Current_row].port_b;
  PORTC.OUTSET = Col_ports[Current_row].port_c;
  PORTD.OUTSET = Col_ports[Current_row].port_d;
  PORTE.OUTSET = Col_ports[Current_row].port_e;
  On_end = On_start + 100;
}

