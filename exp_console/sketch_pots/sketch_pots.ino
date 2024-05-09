// sketch_pots.ino

// Arduino IDE Tools settings:
//   Board: Nano Every
//   Registers Emulation: None

#include <EEPROM.h>
#include <Wire.h>
#include "flash_errno.h"

#define PROGRAM_ID          "Pots V11"

#define MUX_A      9
#define MUX_B     10
#define MUX_C      5

#define SET_MUX_ADDR(n)   \
        digitalWrite(MUX_A, !!((n) & 1));  \
        digitalWrite(MUX_B, !!((n) & 2));  \
        digitalWrite(MUX_C, !!((n) & 4))

// MUX X pins are connected to A0 - A3; X0 - X7 are connected to pots.

#define NUM_A_PINS          4
#define EEPROM_NEEDED       (NUM_A_PINS * (1 + 8*3*sizeof(int)))
#define NUM_EEPROM_USED     EEPROM_NEEDED
#define EEPROM_SIZE         (EEPROM.length())
#define EEPROM_AVAIL        (EEPROM_SIZE - EEPROM_NEEDED - 1)

byte A_pins[] = {A0, A1, A2, A3};
byte Num_pots[NUM_A_PINS];    // number of pots on each A_pin

#define EEPROM_num_pots_addr(a_pin)    (a_pin)

#define EEPROM_cal_low(a_pin, pot_addr) (NUM_A_PINS + (a_pin) * 8 * 3 * sizeof(int) + (pot_addr) * 3 * sizeof(int))
#define EEPROM_cal_center(a_pin, pot_addr) (EEPROM_cal_low(a_pin, pot_addr) + sizeof(int))
#define EEPROM_cal_high(a_pin, pot_addr) (EEPROM_cal_center(a_pin, pot_addr) + sizeof(int))

#define EEPROM_storage_addr(addr) (EEPROM_NEEDED + 1 + (addr))

#define CALIBRATED_LOW          0x1
#define CALIBRATED_CENTER       0x2
#define CALIBRATED_HIGH         0x4

#define MAX_CALIBRATION_ERROR_CENTER   60
#define MAX_CALIBRATION_ERROR_ENDS     30

#define NUM_SAMPLES             35

// 5 just fits in an "int" (2 bytes, signed), so no larger than that!
//#define NUM_SLIDING_BITS        4   /* mult by 15/16 */
#define NUM_SLIDING_BITS        5   /* mult by 31/32 */

struct pot_info_s {   // size 22 + 2 * NUM_SAMPLES(?) bytes (ints are 2 bytes, longs are 4)
  unsigned long a_sum;         // sum of a_samples for averaging
  unsigned int a_samples[NUM_SAMPLES];  // wrap around buffer

  // FIX: a_low/a_high don't work that well... Can be deleted!
  unsigned int a_low;          // raw analog value (0 - 1023), a_high - a_low is typically ~30
  unsigned int a_high;         // raw analog value

  unsigned int last_reported;  // raw analog value
  unsigned int a_running_avg;  // multiplied by 31/32 each sample (0.96875), divide by 32 to get avg
                               // or multiplied by 15/16 each sample (0.9375), divide by 16 to get avg
  int cal_low;
  int cal_center;
  int cal_high;

  // For a_samples:
  byte first_sample;
  byte num_samples;

  byte calibrated;    // bits: CALIBRATED_LOW, CALIBRATED_CENTER, CALIBRATED_HIGH
  byte value;         // linearized and scaled (calibrated) value (0 - 127)
};

// indexed by [A_pin#][pot#]
struct pot_info_s Pot_info[NUM_A_PINS][8];   

#define ERR_LED    13   // built-in LED
#define ERR_LED2   12   // LED on panel

void setup() {
  // put your setup code here, to run once:
  err_led(ERR_LED, ERR_LED2);
  
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);
  pinMode(MUX_C, OUTPUT);

  Wire.begin(0x31);
  Wire.setClock(400000);
  Serial.begin(230400);

  Serial.println(PROGRAM_ID);
  //Serial.print("sizeof(int) is "); Serial.println(sizeof(int)); // 2 bytes
  //Serial.print("sizeof(long) is "); Serial.println(sizeof(long)); // 4 bytes
  Serial.println();

  byte a_pin, pot_addr;
  byte num_pots_msg_seen = 0;
  byte cal_msg_seen = 0;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    byte num_pots = EEPROM[EEPROM_num_pots_addr(a_pin)];
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
      Pot_info[a_pin][pot_addr].a_low = 10000;
      Pot_info[a_pin][pot_addr].a_high = -10000;
      Pot_info[a_pin][pot_addr].last_reported = -1;
      Pot_info[a_pin][pot_addr].first_sample = 0;
      Pot_info[a_pin][pot_addr].num_samples = 0;
      Pot_info[a_pin][pot_addr].a_sum = 0;
      Pot_info[a_pin][pot_addr].a_running_avg = 0;

      if (pot_addr < Num_pots[a_pin]) {
        int b;

        EEPROM.get(EEPROM_cal_low(a_pin, pot_addr), b);
        // Serial.print("int b from EEPROM is ");
        // Serial.println(b);
        if (b == -1) {
          if (!cal_msg_seen) {
            Serial.print("cal_low for a_pin ");
            Serial.print(a_pin);
            Serial.print(", pot_addr ");
            Serial.print(pot_addr);
            Serial.println(" not set in EEPROM");
            cal_msg_seen = 1;
          }
          b = 0;
        } else if (b > MAX_CALIBRATION_ERROR_ENDS) {
          Errno = 101;
          Err_data = 10 * a_pin + pot_addr;
          b = 0;
        }
        Pot_info[a_pin][pot_addr].cal_low = b;

        EEPROM.get(EEPROM_cal_center(a_pin, pot_addr), b);
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
        } else if (abs(b - 511) > MAX_CALIBRATION_ERROR_CENTER) {
          Errno = 102;
          Err_data = 10 * a_pin + pot_addr;
          b = 511;
        }
        Pot_info[a_pin][pot_addr].cal_center = b;

        EEPROM.get(EEPROM_cal_high(a_pin, pot_addr), b);
        // Serial.print("int b from EEPROM is ");
        // Serial.println(b);
        if (b == -1) {
          if (!cal_msg_seen) {
            Serial.print("cal_high for a_pin ");
            Serial.print(a_pin);
            Serial.print(", pot_addr ");
            Serial.print(pot_addr);
            Serial.println(" not set in EEPROM");
            cal_msg_seen = 1;
          }
          b = 1023;
        } else if (b < (1023 - MAX_CALIBRATION_ERROR_ENDS)) {
          Errno = 103;
          Err_data = 10 * a_pin + pot_addr;
          b = 1023;
        }
        Pot_info[a_pin][pot_addr].cal_high = b;
      } // end if (pot_addr in range)
      Pot_info[a_pin][pot_addr].calibrated = 0;
      Pot_info[a_pin][pot_addr].value = 0;
    } // end for (pot_addr)
  } // end for (a_pin)

  if (EEPROM[NUM_EEPROM_USED] == 0xFF) {
    EEPROM[NUM_EEPROM_USED] = 0;
  }

  Wire.onReceive(receiveRequest);  // callback for requests from on high
  Wire.onRequest(sendReport);      // callback for reports to on high
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

byte Cal_iterations = 100;

void calibrate_low(void) {  // errnos 31-39
  int max_values[NUM_A_PINS][8];
  byte i, a_pin, pot_addr;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      max_values[a_pin][pot_addr] = 0;
    }
  }
  for (i = 0; i < Cal_iterations; i++) {
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        SET_MUX_ADDR(pot_addr);
        int a = analogRead(A_pins[a_pin]);
        if (a > max_values[a_pin][pot_addr]) max_values[a_pin][pot_addr] = a;
      }
    }
  } // end for (i)
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      if (max_values[a_pin][pot_addr] < MAX_CALIBRATION_ERROR_ENDS) {
        Pot_info[a_pin][pot_addr].cal_low = max_values[a_pin][pot_addr];
        Pot_info[a_pin][pot_addr].calibrated |= CALIBRATED_LOW;
      } else {
        Errno = 31;
        Err_data = max_values[a_pin][pot_addr];
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
        SET_MUX_ADDR(pot_addr);
        int a = analogRead(A_pins[a_pin]);
        if (a < min_values[a_pin][pot_addr]) min_values[a_pin][pot_addr] = a;
        if (a > max_values[a_pin][pot_addr]) max_values[a_pin][pot_addr] = a;
      }
    }
  } // end for (i)
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      int median = (min_values[a_pin][pot_addr] + max_values[a_pin][pot_addr]) / 2;
      if (abs(median - 511) < MAX_CALIBRATION_ERROR_CENTER) {
        Pot_info[a_pin][pot_addr].cal_center = median;
        Pot_info[a_pin][pot_addr].calibrated |= CALIBRATED_CENTER;
      } else {
        Errno = 41;
        Err_data = abs(median - 511);
      }
    }
  } // end for (a_pin)
}

void calibrate_high(void) {  // errnos 51-59
  int min_values[NUM_A_PINS][8];
  byte i, a_pin, pot_addr;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      min_values[a_pin][pot_addr] = 10000;
    }
  }
  for (i = 0; i < Cal_iterations; i++) {
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        SET_MUX_ADDR(pot_addr);
        int a = analogRead(A_pins[a_pin]);
        if (a < min_values[a_pin][pot_addr]) min_values[a_pin][pot_addr] = a;
      }
    }
  } // end for (i)
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      if (min_values[a_pin][pot_addr] > (1023 - MAX_CALIBRATION_ERROR_ENDS)) {
        Pot_info[a_pin][pot_addr].cal_high = min_values[a_pin][pot_addr];
        Pot_info[a_pin][pot_addr].calibrated |= CALIBRATED_HIGH;
      } else {
        Errno = 51;
        Err_data = 1023 - min_values[a_pin][pot_addr];
      }
    }
  } // end for (a_pin)
}

void write_calibrations(void) {  // errnos 61-69
  byte i, a_pin, pot_addr;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      struct pot_info_s *pi = &Pot_info[a_pin][pot_addr];
      if (pi->calibrated & CALIBRATED_LOW) {
        EEPROM.put(EEPROM_cal_low(a_pin, pot_addr), pi->cal_low);
      } else {
        Errno = 35;
        Err_data = a_pin * 8 + pot_addr;
      }
      if (pi->calibrated & CALIBRATED_CENTER) {
        EEPROM.put(EEPROM_cal_center(a_pin, pot_addr), pi->cal_center);
      } else {
        Errno = 45;
        Err_data = a_pin * 8 + pot_addr;
      }
      if (pi->calibrated & CALIBRATED_HIGH) {
        EEPROM.put(EEPROM_cal_high(a_pin, pot_addr), pi->cal_high);
      } else {
        Errno = 55;
        Err_data = a_pin * 8 + pot_addr;
      }
      pi->calibrated = 0;
    } // end for (pot_addr)
  } // end for (a_pin)
}

void receiveRequest(int how_many) {
  // callback for requests from on high
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
    if (b0 == 5) {
      if (check_ls(b1, NUM_A_PINS, 3)) {
        Report = 0xFF;
        return;
      }
    } else {
      // b0 == 6
      if (check_ls(b1, EEPROM[NUM_EEPROM_USED], 4)) {
        Report = 0xFF;
        return;
      }
    }
    Report_addr = b1;
    return;
  }
  Report = 0;
  switch (b0) {
  case 8:  // set num pots, a_pin, number
    if (check_eq(how_many, 3, 8)) break;
    b1 = Wire.read();
    if (check_ls(b1, NUM_A_PINS, 9)) break;
    b2 = Wire.read();
    if (check_ls(b2, 9, 10)) break;
    Num_pots[b1] = b2;
    EEPROM[EEPROM_num_pots_addr(b1)] = b2;
    break;
  case 9:  // store EEPROM addr, value
    if (check_eq(how_many, 3, 20)) break;
    b1 = Wire.read();
    if (check_ls(b1, EEPROM_AVAIL, 21)) break;
    b2 = Wire.read();
    if (b1 > EEPROM[NUM_EEPROM_USED]) EEPROM[NUM_EEPROM_USED] = b1;
    EEPROM[EEPROM_storage_addr(b1)] = b2;
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
  case 14:  // set Errno, Err_data
    if (check_eq(how_many, 3, 65)) break;
    Errno = Wire.read();
    Err_data = Wire.read();
    break;
  default:
    Errno = 110;
    Err_data = b0;
    break;
  } // end switch (b0)
}

void sendReport(void) {
  // callback for reports from on high
  byte len_written, a_pin, pot_addr;
  switch (Report) {
  case 0:  // Errno, Err_data + pot.value * num_pots (2 + num_pots total)
    len_written = Wire.write(Errno);
    if (len_written != 1) {
      Errno = 70;
      Err_data = len_written;
      break;
    }
    len_written = Wire.write(Err_data);
    if (len_written != 1) {
      Errno = 71;
      Err_data = len_written;
      break;
    }
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        len_written = Wire.write(Pot_info[a_pin][pot_addr].value);
        if (len_written != 1) {
          Errno = 72;
          Err_data = len_written;
          break;
        }
      }
    }
    Errno = 0;
    Err_data = 0;
    break;
  case 1:  // num_pots, NUM_A_PINS, EEPROM_AVAIL, EEPROM USED (4 bytes total)
    byte num_pots = 0;
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      num_pots += Num_pots[a_pin];
    }
    len_written = Wire.write(num_pots);
    if (len_written != 1) {
      Errno = 73;
      Err_data = len_written;
      break;
    }
    len_written = Wire.write(byte(NUM_A_PINS));
    if (len_written != 1) {
      Errno = 74;
      Err_data = len_written;
      break;
    }
    len_written = Wire.write(byte(EEPROM_AVAIL));
    if (len_written != 1) {
      Errno = 75;
      Err_data = len_written;
      break;
    }
    len_written = Wire.write(EEPROM[NUM_EEPROM_USED]);
    if (len_written != 1) {
      Errno = 76;
      Err_data = len_written;
      break;
    }
    break;
  case 2:  // Errno, Err_data (2 bytes total)
    len_written = Wire.write(Errno);
    if (len_written != 1) {
      Errno = 77;
      Err_data = len_written;
      break;
    }
    len_written = Wire.write(Err_data);
    if (len_written != 1) {
      Errno = 78;
      Err_data = len_written;
      break;
    }
    Errno = 0;
    Err_data = 0;
    break;
  case 3:  // Num_pots for each a_pin (NUM_A_PINS bytes total)
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      len_written = Wire.write(Num_pots[a_pin]);
      if (len_written != 1) {
        Errno = 79;
        Err_data = len_written;
        break;
      }
    }
    break;
  case 4:  // Cycle_time (uSec, 4 bytes total)
    len_written = Wire.write(Cycle_time);  // unsigned long
    if (len_written != 4) {
      Errno = 80;
      Err_data = len_written;
      break;
    }
    break;
  case 5:  // cal_low, cal_center, cal_high for each pot in a_pins (6 * num_pots for a_pin)
    if (Report_addr < NUM_A_PINS) {
      a_pin = Report_addr;
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        len_written = Wire.write(Pot_info[a_pin][pot_addr].cal_low);
        if (len_written != 2) {
          Errno = 81;
          Err_data = len_written;
          break;
        }
        len_written = Wire.write(Pot_info[a_pin][pot_addr].cal_center);
        if (len_written != 2) {
          Errno = 82;
          Err_data = len_written;
          break;
        }
        len_written = Wire.write(Pot_info[a_pin][pot_addr].cal_high);
        if (len_written != 2) {
          Errno = 83;
          Err_data = len_written;
          break;
        }
      }
      Report_addr++;
    } else {
      Errno = 3;
      Err_data = Report_addr;
    }
    break;
  case 6:  // next stored EEPROM byte (1 byte, EEPROM_addr auto-incremented)
    if (Report_addr < EEPROM[NUM_EEPROM_USED]) {
      len_written = Wire.write(EEPROM[EEPROM_storage_addr(Report_addr)]);
      if (len_written != 1) {
        Errno = 84;
        Err_data = len_written;
        break;
      }
      Report_addr++;
    } else {
      Errno = 4;
      Err_data = Report_addr;
    }
    break;
  default:
    Errno = 89;
    Err_data = Report;
    break;
  } // end switch (Report)
}

unsigned int Num_reads = 0;
unsigned int Read_threshold = 10;

byte Trace_on = 0;

void read(byte a_pin, byte pot_addr, byte pot_num) {
  int a = analogRead(A_pins[a_pin]);

  struct pot_info_s *pi = &Pot_info[a_pin][pot_addr];
  
  if (a < pi->a_low) pi->a_low = a;
  if (a > pi->a_high) pi->a_high = a;

  if (pi->num_samples < NUM_SAMPLES) {
    pi->a_samples[(pi->first_sample + pi->num_samples) % NUM_SAMPLES] = a;
    pi->a_sum += a;
    pi->num_samples += 1;
  } else {
    pi->a_sum -= pi->a_samples[pi->first_sample];
    pi->a_samples[pi->first_sample] = a;
    pi->a_sum += a;
    pi->first_sample = (pi->first_sample + 1) % NUM_SAMPLES;
  }

  // This needs to be shifted right NUM_SLIDING_BITS to get the running avg
  pi->a_running_avg =
      (((unsigned long)pi->a_running_avg * ((1 << NUM_SLIDING_BITS) - 1)   // * 31(15)
                                         + (1 << (NUM_SLIDING_BITS - 1)))  // rounded up
                                         >> NUM_SLIDING_BITS)              // / 32(16)
                                         + a;
  
  if (Num_reads >= Read_threshold) {
    int avg = pi->a_sum / pi->num_samples;
    unsigned int running_avg = (pi->a_running_avg + (1 << (NUM_SLIDING_BITS - 1)))  // rounded up
                                                  >> NUM_SLIDING_BITS;              // / 32(16)
    //int new_raw_value = (pi->a_high + pi->a_low) / 2;
    //int new_raw_value = running_avg;
    int new_raw_value = avg;

    if (abs(avg - pi->last_reported) > 2 && abs(running_avg - pi->last_reported) > 2) {
      // save new raw value
      byte scaled_value = scale_slide_pot(new_raw_value, pi->cal_low, pi->cal_center, pi->cal_high);
      if (pi->value != scaled_value) {
        // report new scaled value and reset a_low, a_high
        if (Trace_on) {
          Serial.print("Trace: pot_num ");
          Serial.print(pot_num);
          Serial.print(", a_low ");
          Serial.print(pi->a_low);
          Serial.print(", a_high ");
          Serial.print(pi->a_high);
          Serial.print(", num_samples ");
          Serial.print(pi->num_samples);
          Serial.print(", avg ");
          Serial.print(avg);
          Serial.print(", a_running_avg ");
          Serial.print(pi->a_running_avg);
          Serial.print(", running avg ");
          Serial.print(running_avg);
          Serial.print(", last_reported ");
          Serial.print(pi->last_reported);
          Serial.print(", new_raw_value ");
          Serial.print(new_raw_value);
          Serial.print(" -> value changed from ");
          Serial.print(pi->value);
          Serial.print(" to ");
          Serial.println(scaled_value);
        } // end if (Trace_on)
        pi->value = scaled_value;
        pi->a_low = a;
        pi->a_high = a;
        /************ Don't reset these... 
        pi->num_samples = 1;
        pi->a_sum = a;
        pi->a_running_avg = a << NUM_SLIDING_BITS;
        ************/
      } // end if (scaled_value changed)
      pi->last_reported = new_raw_value;
    } // end if (new_raw_value changed)
  } // end if (Num_reads...)
}

void help(void) {
  Serial.println();
  Serial.println(PROGRAM_ID);
  Serial.println();
  Serial.println("? - help");
  Serial.println("P - show Num_pots");
  Serial.println("R - view current pot values");
  Serial.println("Sa_pin,n - set Num_pots on a_pin to n");
  Serial.println("T - show Cycle_time");
  Serial.println("X<errno> - set Errno");
  Serial.println("E - show Errno, Err_data");
  Serial.println("L - calibrate_low");
  Serial.println("C - calibrate_center");
  Serial.println("H - calibrate_high");
  Serial.println("V - view_calibrations");
  Serial.println("W - write_calibrations");
  Serial.println("N - trace oN");
  Serial.println("F - trace ofF");
  Serial.println("An - set MUX addr to n, and disable activity, trace ofF re-enables");
  Serial.println();
}

void view_pot_values(void) {
  byte a_pin, pot_addr, pot_num;
  pot_num = 1;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      struct pot_info_s *pi = &Pot_info[a_pin][pot_addr];
      Serial.print("pot: ");
      Serial.print(pot_num);
      Serial.print(", a_low: ");
      Serial.print(pi->a_low);
      Serial.print(", a_high: ");
      Serial.print(pi->a_high);
      Serial.print(", num_samples: ");
      Serial.print(pi->num_samples);
      Serial.print(", avg: ");
      Serial.print(pi->a_sum / pi->num_samples);
      Serial.print(", a_running_avg: ");
      Serial.print(pi->a_running_avg);
      Serial.print(", running_avg: ");
      Serial.print((pi->a_running_avg + (1 << (NUM_SLIDING_BITS - 1))) >> NUM_SLIDING_BITS);
      Serial.print(", last_reported: ");
      Serial.print(pi->last_reported);
      Serial.print(", value: ");
      Serial.println(pi->value);
      Serial.print("  Samples: ");
      for (byte i = 0; i < pi->num_samples; i++) {
        if (i) Serial.print(", ");
        Serial.print(pi->a_samples[(pi->first_sample + i) % NUM_SAMPLES]);
      }
      Serial.println();
      pot_num++;
    } // end for (pot_addr)
  } // end for (a_pin)
}

void view_calibrations(void) {
  byte a_pin, pot_addr, pot_num;
  pot_num = 1;
  for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
    for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
      struct pot_info_s *pi = &Pot_info[a_pin][pot_addr];
      // pot: PP, cal_low: F, low: 12, cal_center: T, center: 498, cal_high: F, high: 1020
      Serial.print("pot: ");
      Serial.print(pot_num);
      if (pi->calibrated & CALIBRATED_LOW) {
        Serial.print(", cal_low: T, low: ");
      } else {
        Serial.print(", cal_low: F, low: ");
      }
      Serial.print(pi->cal_low);

      if (pi->calibrated & CALIBRATED_CENTER) {
        Serial.print(", cal_center: T, center: ");
      } else {
        Serial.print(", cal_center: F, center: ");
      }
      Serial.print(pi->cal_center);

      if (pi->calibrated & CALIBRATED_HIGH) {
        Serial.print(", cal_high: T, high: ");
      } else {
        Serial.print(", cal_high: F, high: ");
      }
      Serial.println(pi->cal_high);

      pot_num++;
    } // end for (pot_addr)
  } // end for (a_pin)
}

byte Disable = 0;

void loop() {
  // put your main code here, to run repeatedly:

  // Cycle_time for 20 pins is reported as 2.8 mSec

  unsigned long start_time = micros();
  byte a_pin, pot_addr, pot_num;

  if (!Disable) {
    Num_reads += 1;
  
    // cycle time is 1.8 mSec
    pot_num = 0;
    for (a_pin = 0; a_pin < NUM_A_PINS; a_pin++) {
      for (pot_addr = 0; pot_addr < Num_pots[a_pin]; pot_addr++) {
        SET_MUX_ADDR(pot_addr);
        read(a_pin, pot_addr, ++pot_num);
      } // end for (pot_addr)
    } // end for (a_pin)

    if (Num_reads >= Read_threshold) Num_reads = 0;
  } // end if (!Disable)
  
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
    case 'R': view_pot_values(); break;
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
        if (n < 9) {
          Num_pots[a_pin] = n;
          EEPROM[EEPROM_num_pots_addr(a_pin)] = n;
          Serial.print("Num_pots for a_pin ");
          Serial.print(a_pin);
          Serial.print(" set to ");
          Serial.println(n);
        } else {
          Serial.print("Invalid num_pots ");  
          Serial.print(n);
          Serial.println(" must be < 9");
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
    case 'L':
      calibrate_low();
      if (Pot_info[0][0].calibrated & CALIBRATED_LOW) {
        Serial.print("calibrated low is ");
        Serial.println(Pot_info[0][0].cal_low);
      } else {
        Serial.println("failed to calibrate");
      }
      break;
    case 'C':
      calibrate_center();
      if (Pot_info[0][0].calibrated & CALIBRATED_CENTER) {
        Serial.print("calibrated center is ");
        Serial.println(Pot_info[0][0].cal_center);
      } else {
        Serial.println("failed to calibrate");
      }
      break;
    case 'H':
      calibrate_high();
      if (Pot_info[0][0].calibrated & CALIBRATED_HIGH) {
        Serial.print("calibrated high is ");
        Serial.println(Pot_info[0][0].cal_high);
      } else {
        Serial.println("failed to calibrate");
      }
      break;
    case 'V': view_calibrations(); break;
    case 'W':
      write_calibrations();
      Serial.println("write_calibrations done");
      break;
    case 'N':
      Trace_on = 1;
      Serial.println("Trace oN");
      break;
    case 'F':
      Trace_on = 0;
      Disable = 0;
      Serial.println("Trace ofF");
      break;
    case 'A':
      n = Serial.parseInt(SKIP_WHITESPACE);
      if (n >= 8) {
        Serial.print("Invalid MUX addr ");  
        Serial.print(n);
        Serial.println(" must be < 8");
      } else {
        SET_MUX_ADDR(n);
        Disable = 1;
        Serial.print("Set MUX addr to ");
        Serial.println(n);
      }
      break;
    case ' ': case '\t': case '\n': case '\r': break;
    default: help(); break;
    } // end switch (c)

    while (Serial.available() && Serial.read() != '\n') ;
  } // end if (Serial.available())
  
  errno();   // Flash pin D13 and D12
  
  Cycle_time = micros() - start_time;
}

// vim: sw=2
