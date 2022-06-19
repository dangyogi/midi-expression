// sketch_leds.ino

#include <EEPROM.h>
#include <Wire.h>
#include "flash_errno.h"

#define COUNTER_MR_NOT  6     /* PF4 */
#define COUNTER_CP      3     /* PF5 */

// These go from the MSB of bits passed from on high, to the LSB:
#define COL_0           12    /* PE1 */
#define COL_1            4    /* PC6 */
#define COL_2            0    /* PC5 */
#define COL_3            1    /* PC4 */
#define COL_4            8    /* PE3 */
#define COL_5            5    /* PB2 */
#define COL_6           10    /* PB1 */
#define COL_7            9    /* PB0 */

#define COL_8           AREF  /* PD7 */
#define COL_9           11    /* PE0 */
#define COL_10          A7    /* PD5 */
#define COL_11          A6    /* PD4 */
#define COL_12          A0    /* PD3 */
#define COL_13          A1    /* PD2 */
#define COL_14          A2    /* PD1 */
#define COL_15          A3    /* PD0 */

#define NUM_COLS            16
#define NUM_ROWS            16
#define EEPROM_NEEDED       1
#define NUM_EEPROM_USED     EEPROM_NEEDED
#define EEPROM_SIZE         (EEPROM.length())
#define EEPROM_AVAIL        (EEPROM_SIZE - EEPROM_NEEDED - 1)

byte Col_pins[] = {COL_0, COL_1, COL_2, COL_3, COL_4, COL_5, COL_6, COL_7, COL_8,
                   COL_9, COL_10, COL_11, COL_12, COL_13, COL_14, COL_15};
                   
byte Num_rows;

#define EEPROM_num_rows    0

#define EEPROM_storage_addr(addr) (EEPROM_NEEDED + 1 + (addr))

struct col_ports_s {   // size 4 bytes, representing 16 bits (cols)
  byte port_d;  // 7, 5-0 -> COL_8, COL_10 to COL_15
  byte port_b;  // 2-0    -> COL_5 to COL_7
  byte port_c;  // 6-4    -> COL_1 to COL_3
  byte port_e;  // 3,1,0  -> COL_4, COL_0, COL_9
};

struct col_ports_s Col_ports[NUM_ROWS];   // 64 bytes
const struct col_ports_s All_leds_off = {
  //76543210
  0B10111111,  // PORTD
  0B00000111,  // PORTB
  0B01110000,  // PORTC
  0B00001011,  // PORTE
};

byte Errno = 0;
byte Err_data = 0;

#define ERR_LED    13   // built-in LED
#define ERR_LED2    7   // LED on panel

void setup() {
  // put your setup code here, to run once:
  err_led(ERR_LED, ERR_LED2);
  
  digitalWrite(ERR_LED, HIGH);
  digitalWrite(ERR_LED2, HIGH);
  
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

  Wire.begin(0x32);
  Wire.setClock(400000);
  Serial.begin(9600);
  
  byte num_rows = EEPROM[EEPROM_num_pows];
  if (num_rows == 0xFF) {
    Num_pots[a_pin] = 1;
    if (!num_pots_msg_seen) {
      Serial.print("Num_pots for ");
      Serial.print(a_pin);
      Serial.println(" not set in EEPROM");
      num_pots_msg_seen = 1;
    }
  } else if (num_rows > NUM_ROWS) {
    Num_rows[a_pin] = NUM_ROWS;
    Errno = 100;
    Err_data = a_pin;
  } else Num_rows[a_pin] = num_rows;
 
  if (EEPROM[NUM_EEPROM_USED] == 0xFF) {
    EEPROM[NUM_EEPROM_USED] = 0;
  }

  Wire.onReceive(receiveRequest);  // callback for requests from on high
  Wire.onRequest(sendReport);      // callback for reports to on high
  
  Serial.print("sizeof(struct pot_info_s) is ");
  Serial.println(sizeof(struct pot_info_s));
  
  digitalWrite(ERR_LED, LOW);
  digitalWrite(ERR_LED2, LOW);
} // end setup()

// 0 = ??
// 1 = num_rows, num_a_pins, num_EEPROMS_avail, num_EEPROMS_used
// 2 = errno, err_data
// 3 = num_pots for each a_pin
// 4 = cycle_time
// 5 = ??
// 6 = EEPROM <addr> (one byte at a time, auto increments)
// 0xFF = report nothing (no data returned)
byte Report = 0xFF;
byte Report_addr = 0;
unsigned long Cycle_time;  // uSec

byte check_eq(byte b, byte n, byte errno) {
  if (b != n) {
    Errno = errno;
    Err_data = b;
    return 1;
  }
  return 0;
}

byte check_ls(byte b, byte limit, byte errno) {
  if (b >= limit) {
    Errno = errno;
    Err_data = b;
    return 1;
  }
  return 0;
}

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

int How_many;
byte ReceiveReport_running;

void receiveRequest(int how_many) {
  // callback for requests from on high
  if (ReceiveReport_running) {
    Errno = 70;
  } else {
    ReceiveReport_running = 1;
    How_many = how_many;
    schedule_step_fun(STEP_RECEIVE_REPORT);
  }
}

void step_receiveReport(void) {  
  byte b0, b1, b2;
  b0 = Wire.read();
  if (b0 < 5) {
    if (!check_eq(How_many, 1, 1)) Report = b0;
  } else if (b0 < 7) {
    if (!check_eq(How_many, 2, 2)) {
      b1 = Wire.read();
      if (b0 == 5) {
        if (!check_ls(b1, NUM_A_PINS, 3)) {
          Report = b0;
          Report_addr = b1;
        }
      } else {
        // b0 == 6
        if (!check_ls(b1, EEPROM[NUM_EEPROM_USED], 4)) {
          Report = b0;
          Report_addr = b1;
        }
    }
  } else {
    switch (b0) {
    case 8:  // set num pots, a_pin, number
      if (check_eq(How_many, 3, 8)) break;
      b1 = Wire.read();
      if (check_ls(b1, NUM_A_PINS, 9)) break;
      b2 = Wire.read();
      if (check_ls(b2, 8, 10)) break;
      Num_pots[b1] = b2;
      EEPROM[EEPROM_num_pots_addr(b1)] = b2;
      break;
    case 9:  // store EEPROM addr, value
      if (check_eq(How_many, 3, 20)) break;
      b1 = Wire.read();
      if (check_ls(b1, EEPROM_AVAIL, 21)) break;
      b2 = Wire.read();
      if (b1 > EEPROM[NUM_EEPROM_USED]) EEPROM[NUM_EEPROM_USED] = b1;
      EEPROM[EEPROM_storage_addr(b1)] = b2;
      break;
    default:
      Errno = 110;
      Err_data = b0;
      break;
    } // end switch (b0)
  }
  ReceiveReport_running = 0;
}

byte SendReport_running;

void sendReport(void) {
  // callback for reports from on high
  if (SendReport_running) {
    Errno = 80;
  } else {
    SendReport_running = 1;
    schedule_step_fun(STEP_SEND_REPORT);
  }
}

void step_sendReport(void) {
  byte a_pin, pot_addr;
  switch (Report) {
  case 0:  // Errno, Err_data
    Wire.write(Errno);
    Wire.write(Err_data);
    Errno = 0;
    Err_data = 0;
    break;
  case 1:  // Num_rows, NUM_COLS, Num_numeric_displays, Num_alpha_displays, EEPROM_AVAIL, EEPROM USED (4 bytes total)
    Wire.write(Num_rows);
    Wire.write(byte(NUM_COLS));
    Wire.write(byte(EEPROM_AVAIL));
    Wire.write(EEPROM[NUM_EEPROM_USED]);
    break;
  case 4:  // Cycle_time (uSec, 4 bytes total)
    Wire.write(Cycle_time);
    break;
  case 6:  // next stored EEPROM byte (1 byte, EEPROM_addr auto-incremented)
    if (Report_addr < EEPROM[NUM_EEPROM_USED]) {
      Wire.write(EEPROM[EEPROM_storage_addr(Report_addr)]);
      Report_addr++;
    } else {
      Errno = 4;
      Err_data = Report_addr;
    }
    break;
  } // end switch (Report)
}

void help(void) {
  Serial.println();
  Serial.println("? - help");
  Serial.println("P - show Num_rows");
  Serial.println("Sn - set Num_rows to n");
  Serial.println("T - show Cycle_time");
  Serial.println("X<errno> - set Errno");
  Serial.println("E - show Errno, Err_data");
  Serial.println();
}

byte Timeout_fun_scheduled[NUM_TIMEOUT_FUNS];

void timeout(void) {
  // This is the slow loop.  It is run roughly every 1.6 mSec.
  
  byte i;
  for (i = 0; i < NUM_TIMEOUT_FUNS; i++) {
    if (Timeout_fun_scheduled[i]) {
      switch (i) {
      case 0:
      } // end switch (i)
      return;
    } // end if (Timeout_fun_scheduled[i])
  } // end for (i)
  
  if (Serial.available()) {
    char c = toupper(Serial.read());
    byte n;
    switch (c) {
    case '?': help(); break;
    case 'P':
      for (a_pin = 0; a_pin < NUM_A_PINS - 1; a_pin++) {
        Serial.print(Num_pots[a_pin]);
        Serial.print(", ");
      }
      Serial.println(Num_pots[NUM_A_PINS - 1]);
      break;
    case 'S':
      a_pin = Serial.parseInt(SKIP_WHITESPACE);
      if (a_pin >= NUM_A_PINS) {
        Serial.print("Invalid a_pin ");  
        Serial.print(a_pin);
        Serial.print(" must be < ");
        Serial.println(NUM_A_PINS);    
      } else if (Serial.read() != ',') {
        Serial.println("Missing ',' in 'S' command -- aborted");
      } else {
        n = Serial.parseInt(SKIP_WHITESPACE);
        if (n < 8) {
          Num_pots[a_pin] = n;
          EEPROM[EEPROM_num_pots_addr(a_pin)] = n;
          Serial.print("Num_pots for a_pin ");
          Serial.print(a_pin);
          Serial.print(" set to ");
          Serial.println(n);
        } else {
          Serial.print("Invalid num_pots ");  
          Serial.print(n);
          Serial.println(" must be < 8");
        }
      }
      break;
    case 'T':
      Serial.print("Cycle_time is ");
      Serial.print(Cycle_time);
      Serial.println(" uSec");
      break;
    case 'X':
      Errno = Serial.parseInt(SKIP_WHITESPACE);
      Err_data = 0;
      Serial.print("Errno set to ");
      Serial.println(Errno);
      break;
    case 'E':
      Serial.print("Errno is ");
      Serial.print(Errno);
      Serial.print(", Err_data is ");
      Serial.println(Err_data);
      Errno = 0;
      Err_data = 0;
      break;
    case ' ': case '\t': case '\n': case '\r': break;
    default: help(); break;
    } // end switch (c)
  } // end if (Serial.available())
  
  errno(Errno);
}

unsigned short On_start, On_end;

byte Current_row = NUM_ROWS - 1;

void step(byte who_dunnit) {
  // caller has 100 uSec (1600 processor clock cycles) to call this after prior call.
  // caller identifies himself by a who_dunnit Errno.  Err_data will be time over in 0.01 mSecs.
  
  unsigned short now = (unsigned short)micros();
  if (On_end > On_start) {
    if (now >= On_end || now < On_start) {
      // naughty!!!
      Errno = who_dunnit;
    } else {
      // wait for On_end time...
      now = (unsigned short)micros();
      while (!(now >= On_end || now < On_start)) ;
    }
  } else {
    if (now >= On_end && now < On_start) {
      // naughty!!!
      Errno = who_dunnit;
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
  if (Current_row >= NUM_ROWS - 1) {
    // 1 processor clock cycle is 62.5 nSec
    digitalWrite(COUNTER_MR_NOT, LOW);  // 30nSec pulse width
    Current_row = 0;
    digitalWrite(COUNTER_MR_NOT, HIGH);
  } else {
    digitalWrite(COUNTER_CP, HIGH); // 25nSec pulse width
    Current_row++;
    digitalWrite(COUNTER_CP, LOW);
  }

  if (Errno == who_dunnit) {
    Err_data = (now - On_end) / 100;
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
  On_end = On_start + 100us;
}

byte = Next_step_fun = 0xFF;
byte Step_fun_scheduled[NUM_STEP_FUNS];

void schedule_step_fun(byte step_fun) {
  if (step_fun >= NUM_STEP_FUNS) {
    Errno = 60;
    Err_data = step_fun;
  } else {
    Step_fun_scheduled[step_fun] = 1;
    if (step_fun < Next_step_fun) Next_step_fun = step_fun;
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  // Run next step_fun:
  if (Next_step_fun != 0xFF) {
    byte next_step_fun = Next_step_fun;
    Step_fun_scheduled[next_step_fun] = 0;
    Next_step_fun = 0xFF;
    switch (next_step_fun) {
    case 0:
    } // end switch (next_step_fun)
    step(10*(next_step_fun+1));
    if (Next_step_fun > next_step_fun) {
      byte end = Next_step_fun == 0xFF ? NUM_STEP_FUNS : Next_step_fun;
      for (byte i = next_step_fun; i < end; i++) {
        if (Step_fun_scheduled[i]) {
          Next_step_fun = i;
          break;
        }
      } // end for (i)
      step(50);
    } // end if (Next_step_fun > next_step_fun)
  } else {
    step(51);
  } // end if (Next_step_fun != 0xFF)
}
