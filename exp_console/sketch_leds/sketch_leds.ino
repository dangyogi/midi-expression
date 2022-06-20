// sketch_leds.ino

#include <EEPROM.h>
#include <Wire.h>
#include "flash_errno.h"

#include "step.h"
#include "sketch_numeric_displays.h"
#include "sketch_alpha_displays.h"

#define NUM_EEPROM_USED     EEPROM_needed
#define EEPROM_SIZE         (EEPROM.length())
#define EEPROM_AVAIL        (EEPROM_SIZE - EEPROM_needed - 1)

#define EEPROM_storage_addr(addr) (EEPROM_needed + 1 + (addr))

#define ERR_LED    13   // built-in LED
#define ERR_LED2    7   // LED on panel

byte EEPROM_needed;

void setup() {
  // put your setup code here, to run once:
  err_led(ERR_LED, ERR_LED2);

  digitalWrite(ERR_LED, HIGH);
  digitalWrite(ERR_LED2, HIGH);

  Serial.begin(9600);
  
  EEPROM_needed = setup_step();
  EEPROM_needed += setup_numeric_displays(EEPROM_needed);
  EEPROM_needed += setup_alpha_displays(EEPROM_needed);

  Wire.begin(0x32);
  Wire.setClock(400000);
  
  if (EEPROM[NUM_EEPROM_USED] == 0xFF) {
    EEPROM[NUM_EEPROM_USED] = 0;
  }

  Wire.onReceive(receiveRequest);  // callback for requests from on high
  Wire.onRequest(sendReport);      // callback for reports to on high
  
  digitalWrite(ERR_LED, LOW);
  digitalWrite(ERR_LED2, LOW);
} // end setup()

#define STEP_RECEIVE_REPORT     0
#define STEP_SEND_REPORT        1
#define NUM_STEP_FUNS           2

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

int How_many;
byte ReceiveRequest_running;

void receiveRequest(int how_many) {
  // callback for requests from on high
  if (ReceiveRequest_running) {
    Errno = 70;
  } else {
    ReceiveRequest_running = 1;
    How_many = how_many;
    schedule_step_fun(STEP_RECEIVE_REPORT);
  }
}

void step_receiveRequest(void) {
  // request from on high
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
  ReceiveRequest_running = 0;
}

byte SendReport_running;

void sendReport(void) {
  // callback for reports requested from on high
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
  case 1:  // Num_rows, NUM_COLS, Num_numeric_displays, Num_alpha_displays, EEPROM_AVAIL,
           // EEPROM USED (4 bytes total)
    Wire.write(Num_rows);
    Wire.write(byte(NUM_COLS));

    Wire.write(byte(EEPROM_AVAIL));
    Wire.write(EEPROM[NUM_EEPROM_USED]);
    break;
  case 2:  // num_digits (Num_numeric_displays bytes)
  case 3:  // alpha_displays FIX
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

#define NUM_TIMEOUT_FUNS        0

byte Timeout_fun_scheduled[NUM_TIMEOUT_FUNS];

void timeout(void) {
  // This is the slow loop.  It is run roughly every 1.6 mSec.

  byte i;
  for (i = 0; i < NUM_TIMEOUT_FUNS; i++) {
    if (Timeout_fun_scheduled[i]) {
      switch (i) {
      case 0: break;  // FIX
      } // end switch (i)
      return;
    } // end if (Timeout_fun_scheduled[i])
  } // end for (i)

  if (advance_strings()) return;

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
  
  errno();
}

byte Next_step_fun = 0xFF;
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
    case 0: break;  // FIX
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
