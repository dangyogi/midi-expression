// sketch_pots

#include <EEPROM.h>
#include <Wire.h>
#include "flash_errno.h"

extern byte scale_slide_pot(int reading, int calibrated_threshold, int calibrated_center);

#define MUX_A      9
#define MUX_B     10
#define MUX_C      5

// MUX X pins are connected to A0 - A4; X0 - X7 are connected to pots.

#define NUM_A_PINS          4
#define EEPROM_NEEDED       (NUM_A_PINS * (1 + 8*2*sizeof(int)))
#define NUM_EEPROM_USED     EEPROM_NEEDED
#define EEPROM_SIZE         (EEPROM.length())
#define EEPROM_AVAIL        (EEPROM_SIZE - EEPROM_NEEDED - 1)

byte A_pins[] = {A0, A1, A2, A3};
byte Num_pots[NUM_A_PINS];    // number of pots on each A_pin

struct pot_info_s {   // size 16 bytes
  int a_low;
  int a_high;
  int last_reported;
  int cal_low;
  int cal_center;
  int cal_high;
  int cal_threshold;
  byte calibrated;    // bits: 0=low, 1=center, 2=high
  byte value;
};

// indexed by [A_pin#][pot#]
struct pot_info_s Pot_info[NUM_A_PINS][8];   

byte Errno = 0;
byte Err_data = 0;

#define ERR_LED    13   // built-in LED

void setup() {
  // put your setup code here, to run once:
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);
  pinMode(MUX_C, OUTPUT);

  err_led(ERR_LED);

  Wire.begin(0x31);
  Wire.setClock(400000);
  Serial.begin(9600);
  
  byte a_pin, pot_addr;
  byte EEPROM_cal_addr = 4;
  byte num_pots_msg_seen = 0;
  byte cal_msg_seen = 0;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    byte num_pots = EEPROM[a_pin];
    if (num_pots == 0xFF) {
      Num_pots[a_pin] = 1;
      if (!num_pots_msg_seen) {
        Serial.print("Num_pots for ");
        Serial.print(a_pin);
        Serial.println(" not set in EEPROM");
        num_pots_msg_seen = 1;
      }
    } else if (num_pots > 8) {
      Num_pots[a_pin] = 8;
      Errno = 100;
      Err_data = a_pin;
    } else Num_pots[a_pin] = num_pots;
    for (pot_addr = 0; pot_addr < 8; pot_addr++) {
      // Go ahead and initialize all 8, in case the number of pots changes...
      Pot_info[a_pin][pot_addr].a_low = 100000;
      Pot_info[a_pin][pot_addr].a_high = -100000;
      Pot_info[a_pin][pot_addr].last_reported = -1;
      int b;
      EEPROM.get(EEPROM_cal_addr, b);
      // Serial.print("int b from EEPROM is ");
      // Serial.println(b);
      EEPROM_cal_addr += sizeof(int);
      if (b == -1) {
        if (!cal_msg_seen) {
          Serial.print("cal_threshold for a_pin ");
          Serial.print(a_pin);
          Serial.print(", pot_addr ");
          Serial.print(pot_addr);
          Serial.println(" not set in EEPROM");
          cal_msg_seen = 1;
        }
        b = 0;
      } else if (b > 20) {
        Errno = 101;
        Err_data = (a_pin << 4) | pot_addr;
        b = 0;
      }
      Pot_info[a_pin][pot_addr].cal_threshold = b;
      EEPROM.get(EEPROM_cal_addr, b);
      EEPROM_cal_addr += sizeof(int);
      if (b == -1) {
        if (!cal_msg_seen) {
          Serial.print("cal_center for a_pin ");
          Serial.print(a_pin);
          Serial.print(", pot_addr ");
          Serial.print(pot_addr);
          Serial.println(" not set in EEPROM");
          cal_msg_seen = 1;
        }
        b = 511;
      } else if (abs(b - 511) > 50) {
        Errno = 102;
        Err_data = (a_pin << 4) | pot_addr;
        b = 511;
      }
      Pot_info[a_pin][pot_addr].cal_center = b;
      Pot_info[a_pin][pot_addr].calibrated = 0;
      Pot_info[a_pin][pot_addr].value = 0;
    } // end for (pot_addr)
  } // end for (a_pin)

  if (EEPROM[NUM_EEPROM_USED] == 0xFF) {
    EEPROM[NUM_EEPROM_USED] = 0;
  }

  Wire.onReceive(receiveRequest);  // callback for requests from on high
  Wire.onRequest(sendReport);      // callback for reports to on high
  
  Serial.print("sizeof(struct pot_info_s) is ");
  Serial.println(sizeof(struct pot_info_s));
} // end setup()

// 0 = errno, err_data, all pot values
// 1 = num_pots, num_a_pins, num_EEPROMS_avail, num_EEPROMS_used
// 2 = errno, err_data
// 3 = num_pots for each a_pin
// 4 = cycle_time
// 5 = calibration values <a_pin> (8 2-bytes values, auto increments)
// 6 = EEPROM <addr> (one byte at a time, auto increments)
// 0xFF = report nothing (no data returned)
byte Report = 0xFF;
byte Report_addr = 0;
unsigned long Cycle_time;

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

byte Cal_iterations = 100;

void calibrate_low(void) {  // errnos 31-39
  int values[NUM_A_PINS][8];
  byte i, a_pin, pot_addr;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      values[a_pin][pot_addr] = 0;
    }
  }
  for (i = 0; i < Cal_iterations; i++) {
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        digitalWrite(MUX_A, pot_addr & 1);
        digitalWrite(MUX_B, pot_addr & 2);
        digitalWrite(MUX_C, pot_addr & 4);
        int a = analogRead(A_pins[a_pin]);
        if (a > values[a_pin][pot_addr]) values[a_pin][pot_addr] = a;
      }
    }
  } // end for (i)
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      if (values[a_pin][pot_addr] < 40) {
        Pot_info[a_pin][pot_addr].cal_low = values[a_pin][pot_addr];
        Pot_info[a_pin][pot_addr].calibrated |= 1;
      } else {
        Errno = 31;
        Err_data = values[a_pin][pot_addr];
      }
    }
  } // end for (a_pin)
}

void calibrate_center(void) {  // errnos 41-49
  int min_values[NUM_A_PINS][8];
  int max_values[NUM_A_PINS][8];
  byte i, a_pin, pot_addr;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      min_values[a_pin][pot_addr] = 10000;
      max_values[a_pin][pot_addr] = 0;
    }
  }
  for (i = 0; i < Cal_iterations; i++) {
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        digitalWrite(MUX_A, pot_addr & 1);
        digitalWrite(MUX_B, pot_addr & 2);
        digitalWrite(MUX_C, pot_addr & 4);
        int a = analogRead(A_pins[a_pin]);
        if (a < min_values[a_pin][pot_addr]) min_values[a_pin][pot_addr] = a;
        if (a > max_values[a_pin][pot_addr]) max_values[a_pin][pot_addr] = a;
      }
    }
  } // end for (i)
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      int median = (min_values[a_pin][pot_addr] + max_values[a_pin][pot_addr]) / 2;
      if (abs(median - 510) < 40) {
        Pot_info[a_pin][pot_addr].cal_center = median;
        Pot_info[a_pin][pot_addr].calibrated |= 2;
      } else {
        Errno = 41;
        Err_data = median;
      }
    }
  } // end for (a_pin)
}

void calibrate_high(void) {  // errnos 51-59
  int values[NUM_A_PINS][8];
  byte i, a_pin, pot_addr;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      values[a_pin][pot_addr] = 10000;
    }
  }
  for (i = 0; i < Cal_iterations; i++) {
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        digitalWrite(MUX_A, pot_addr & 1);
        digitalWrite(MUX_B, pot_addr & 2);
        digitalWrite(MUX_C, pot_addr & 4);
        int a = analogRead(A_pins[a_pin]);
        if (a < values[a_pin][pot_addr]) values[a_pin][pot_addr] = a;
      }
    }
  } // end for (i)
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      if (values[a_pin][pot_addr] > (1023 - 40)) {
        Pot_info[a_pin][pot_addr].cal_high = values[a_pin][pot_addr];
        Pot_info[a_pin][pot_addr].calibrated |= 3;
      } else {
        Errno = 51;
        Err_data = values[a_pin][pot_addr];
      }
    }
  } // end for (a_pin)
}

void write_calibrations(void) {  // errnos 61-69
  // FIX
}

void receiveRequest(int how_many) {
  byte b0, b1, b2;
  b0 = Wire.read();
  if (b0 < 5) {
    Report = b0;
    check_eq(how_many, 1, 1);
    return;
  }
  if (b0 < 7) {
    Report = b0;
    if (check_eq(how_many, 2, 2)) {
      Report = 0xFF;
      return;
    }
    b1 = Wire.read();
    byte max;
    if (b0 == 5) max = 4;
    else max = EEPROM[NUM_EEPROM_USED];
    if (check_ls(b1, max, 3)) {
      Report = 0xFF;
      return;
    }
    Report_addr = b1;
    return;
  }
  switch (b0) {
  case 8:  // set num pots, a_pin, number
    if (check_eq(how_many, 3, 8)) break;
    b1 = Wire.read();
    if (check_ls(b1, NUM_A_PINS, 9)) break;
    b2 = Wire.read();
    if (check_ls(b2, 8, 10)) break;
    Num_pots[b1] = b2;
    EEPROM[b1] = b2;
    break;
  case 9:  // store EEPROM addr, value
    if (check_eq(how_many, 3, 20)) break;
    b1 = Wire.read();
    if (check_ls(b1, EEPROM_AVAIL, 21)) break;
    b2 = Wire.read();
    if (b1 > EEPROM[NUM_EEPROM_USED]) EEPROM[NUM_EEPROM_USED] = b1;
    EEPROM[EEPROM_NEEDED + 1 + b1] = b2;
    break;
  case 10:  // calibrate low
    if (check_eq(how_many, 1, 30)) break;
    calibrate_low();
    break;
  case 11:  // calibrate center
    if (check_eq(how_many, 1, 40)) break;
    calibrate_center();
    break;
  case 12:  // calibrate high
    if (check_eq(how_many, 1, 50)) break;
    calibrate_high();
    break;
  case 13:  // write calibrations
    if (check_eq(how_many, 1, 60)) break;
    write_calibrations();
    break;
  default:
    Errno = 110;
    Err_data = b0;
    break;
  } // end switch (b0)
}

void sendReport(void) {
  byte a_pin, pot_addr;
  switch (Report) {
  case 0:  // 2 + num_pots
    Wire.write(Errno);
    Wire.write(Err_data);
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        Wire.write(Pot_info[a_pin][pot_addr].value);
      }
    }
    Errno = 0;
    Err_data = 0;
    break;
  case 1:  // 4 bytes
    byte num_pots = 0;
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      num_pots += Num_pots[a_pin];
    }
    Wire.write(num_pots);
    Wire.write(byte(NUM_A_PINS));
    Wire.write(byte(EEPROM_AVAIL));
    Wire.write(EEPROM[NUM_EEPROM_USED]);
    break;
  case 2:  // 2 bytes
    Wire.write(Errno);
    Wire.write(Err_data);
    Errno = 0;
    Err_data = 0;
    break;
  case 3:  // 4 bytes
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      Wire.write(Num_pots[a_pin]);
    }
    break;
  case 4:  // 4 bytes
    Wire.write(Cycle_time);
    break;
  case 5:  // 4 * Num_pots[a_pin] bytes
    if (Report_addr < NUM_A_PINS) {
      a_pin = Report_addr++;
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        Wire.write(Pot_info[a_pin][pot_addr].cal_threshold);
        Wire.write(Pot_info[a_pin][pot_addr].cal_center);
      }
    }
    break;
  case 6:  // 1 byte
    if (Report_addr < EEPROM[NUM_EEPROM_USED]) {
      Wire.write(EEPROM[Report_addr + EEPROM_NEEDED + 1]);
      Report_addr++;
    }
    break;
  } // end switch (Report)
}

unsigned int Num_reads = 0;
unsigned int Read_threshold = 10;

void read(byte a_pin, byte pot_addr) {
  int a = analogRead(A_pins[a_pin]);

  struct pot_info_s *pi = &Pot_info[a_pin][pot_addr];
  
  if (a < pi->a_low) pi->a_low = a;
  if (a > pi->a_high) pi->a_high = a;
  
  if (Num_reads >= Read_threshold) {
    if (pi->last_reported < pi->a_low || pi->last_reported > pi->a_high) {
      int new_value = (pi->a_high + pi->a_low) / 2;
      if (abs(pi->last_reported - new_value) > 8) {
        // report new value
        pi->last_reported = new_value;
        pi->value = scale_slide_pot(new_value, pi->cal_threshold, pi->cal_center);
      }
    }
    pi->a_low = a;
    pi->a_high = a;
  } // end if (Num_reads...)
}

void help(void) {
  Serial.println("? - help");
  Serial.println("P - show Num_pots");
  Serial.println("Sa_pin,n - set Num_pots on a_pin to n");
  Serial.println("T - show Cycle_time");
  Serial.println("X<errno> - set Errno");
  Serial.println("E - show Errno, Err_data");
  Serial.println("L - calibrate_low");
  Serial.println("C - calibrate_center");
  Serial.println("H - calibrate_high");
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long start_time = micros();
  int a_pin, pot_addr;
  
  Num_reads += 1;

  // cycle time is 1.8 mSec
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      digitalWrite(MUX_A, pot_addr & 0x1);
      digitalWrite(MUX_B, pot_addr & 0x2);
      digitalWrite(MUX_C, pot_addr & 0x4);
      read(a_pin, pot_addr);
    } // end for (pot_addr)
  } // end for (a_pin)
  
  if (Num_reads >= Read_threshold) Num_reads = 1;

  if (Serial.available()) {
    char c = toupper(Serial.read());
    byte a_pin, n;
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
        Serial.println("Missing ',' in 'S' command -- ignored");
      } else {
        n = Serial.parseInt(SKIP_WHITESPACE);
        if (n < 8) {
          Num_pots[a_pin] = n;
          EEPROM[a_pin] = n;
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
      break;
    case 'E':
      Serial.print("Errno is ");
      Serial.print(Errno);
      Serial.print(", Err_data is ");
      Serial.println(Err_data);
      Errno = 0;
      Err_data = 0;
      break;
    case 'L':
      calibrate_low();
      if (Pot_info[0][0].calibrated & 1) {
        Serial.print("calibrated low is ");
        Serial.println(Pot_info[0][0].cal_low);
      } else {
        Serial.println("failed to calibrate");
      }
      break;
    case 'C':
      calibrate_center();
      if (Pot_info[0][0].calibrated & 2) {
        Serial.print("calibrated center is ");
        Serial.println(Pot_info[0][0].cal_center);
      } else {
        Serial.println("failed to calibrate");
      }
      break;
    case 'H':
      calibrate_high();
      if (Pot_info[0][0].calibrated & 4) {
        Serial.print("calibrated high is ");
        Serial.println(Pot_info[0][0].cal_high);
      } else {
        Serial.println("failed to calibrate");
      }
      break;
    case ' ':
    case '\t':
    case '\n':
    case '\r': break;
    default: help(); break;
    }
  } // end if (Serial.available())
  
  errno(Errno);   // Flash pin 13
  
  Cycle_time = micros() - start_time;
}
