// sketch_master.ino

// Arduino IDE Tools settings:
//   Board: Teensy 4.1
//   USB Type: Serial + MIDI*4

#include <EEPROM.h>
#include <Wire.h>
#include "flash_errno.h"
#include "switches.h"
#include "events.h"
#include "encoders.h"
#include "notes.h"
#include "functions.h"
#include "triggers.h"

#define PROGRAM_ID    "Master V57"

// These are set to INPUT_PULLDOWN to prevent flickering on unused ports
#define FIRST_PORT    0
#define LAST_PORT    54

#define ERR_LED      13   // built-in LED
#define ERR_LED_2    32   // front panel Master Error LED

#define MIN_ERROR_DISPLAY_INTERVAL      1000ul  /* mSec */

// Indexed by I2C_addr - I2C_BASE
#define NUM_REMOTES   2
byte Remote_Errno[NUM_REMOTES];
byte Remote_Err_data[NUM_REMOTES];
byte Remote_last_errno[NUM_REMOTES];
unsigned long Remote_last_display_time[NUM_REMOTES];
char Remote_char[NUM_REMOTES] = {'P', 'L'};
TwoWire *Remote_wire[NUM_REMOTES] = {&Wire1, &Wire};

#define I2C_BASE              0x31
#define I2C_POT_CONTROLLER    0x31
#define I2C_LED_CONTROLLER    0x32

#define I2C_MASTER            0x30    /* FIX: not needed */

byte EEPROM_used;

void set_EEPROM(byte EEPROM_addr, byte value) {
  EEPROM[EEPROM_addr] = value;
}

byte get_EEPROM(byte EEPROM_addr) {
  return EEPROM[EEPROM_addr];
}

#define NUM_PERIODICS           6

// Periodic functions:
#define PULSE_NOTES_ON          0
#define PULSE_NOTES_OFF         1
#define UPDATE_LEDS             2   /* not used */
#define GET_POTS                3
#define SEND_MIDI               4   /* not used */
#define SWITCH_REPORT           5

unsigned short Periodic_period[NUM_PERIODICS];  // mSec, 0 to disable
unsigned short Periodic_offset[NUM_PERIODICS];  // mSec offset from even Periodic_period boundary
unsigned long  Periodic_next[NUM_PERIODICS];    // next time to run Periodic function

// mSec:
#define PULSE_NOTES_PERIOD        500
#define PULSE_NOTES_ON_OFFSET       6
#define PULSE_NOTES_OFF_OFFSET   (PULSE_NOTES_ON_OFFSET + 405)
#define GET_POTS_PERIOD            20
#define GET_POTS_OFFSET             1
#define SWITCH_REPORT_PERIOD     1000
#define SWITCH_REPORT_OFFSET       16

// Subtracts two times (millis or micros) giving a signed result, so you can tell which came first
#define TIME_A_MINUS_B(a, b)        (long(a) - long(b))

// True if time A >= time B
#define TIME_A_GEQ_B(a, b)          (TIME_A_MINUS_B(a, b) >= 0)

void turn_off_periodic_fun(byte fun) {
  Periodic_period[fun] = 0;
}

void turn_on_periodic_fun(byte fun, unsigned short period) {
  if (Periodic_period[fun] == 0) {
    unsigned long now = millis();
    Periodic_period[fun] = period;
    Periodic_next[fun] = now - now % period + Periodic_offset[fun];
    Serial.print("turn_on_periodic_fun "); Serial.print(fun);
    Serial.print(", now "); Serial.print(now);
    Serial.print(", period "); Serial.print(period);
    Serial.print(", first attempt at next "); Serial.println(Periodic_next[fun]);
    if (TIME_A_GEQ_B(now, Periodic_next[fun])) {
      Serial.print("next < now, adding period "); Serial.println(period);
      Periodic_next[fun] += period;
    }
    if (TIME_A_MINUS_B(Periodic_next[fun], now) <= 20) {
      Serial.print("Small start interval, adding period "); Serial.println(period);
      Periodic_next[fun] += period;  // a little extra won't hurt here...
    }
    Serial.print("Final Periodic_next: "); Serial.println(Periodic_next[fun]);
  }
}

void toggle_periodic_fun(byte fun, unsigned short period) {
  if (Periodic_period[fun] == 0) {
    turn_on_periodic_fun(fun, period);
  } else {
    turn_off_periodic_fun(fun);
  }
}

byte Driver_up;

void test2(const char *a_str, unsigned long a, const char *b_str, unsigned long b, long expect) {
  if (TIME_A_MINUS_B(a, b) != expect) {
    Serial.print("test2, "); Serial.print(a_str); Serial.print(" = 0x"); Serial.print(a, HEX);
    Serial.print(" - "); Serial.print(b_str); Serial.print(" = 0x"); Serial.print(b, HEX);
    Serial.print(" is "); Serial.println(TIME_A_MINUS_B(a, b));
  }
}

void test_a(unsigned long a) {
  test2("a", a, "a - 16", a - 16, 16);
  test2("a", a, "a", a, 0);
  test2("a", a, "a + 16", a + 16, -16);
}

void test_b(unsigned long b) {
  test2("b - 16", b - 16, "b", b, -16);
  test2("b", b, "b", b, 0);
  test2("b + 16", b + 16, "b", b, 16);
}

void test(unsigned long x) {
  test_a(x);
  test_b(x);
}

void setup() {
  // put your setup code here, to run once:
  byte i;

  Serial.begin(230400);
  
  for (i = FIRST_PORT; i <= LAST_PORT; i++) {
    pinMode(i, INPUT_PULLDOWN);
  }
  delay(2);
  
  Serial.println(PROGRAM_ID);
  Serial.println();

  err_led(ERR_LED, ERR_LED_2);
  delay(2);

  //Wire.begin(I2C_MASTER);       // to LED controller, FIX: I2C_MASTER not needed
  Wire.begin();                 // to LED controller
  Wire.setClock(400000);
  // Wire.onReceive(receiveLEDErrno);  // FIX: not needed
  //Wire1.begin(I2C_MASTER);      // to Pot controller, FIX: I2C_MASTER not needed
  Wire1.begin();                // to Pot controller
  Wire1.setClock(400000);

  byte EEPROM_used = setup_switches(0);
  EEPROM_used += setup_events(EEPROM_used);
  EEPROM_used += setup_encoders(EEPROM_used);
  EEPROM_used += setup_functions(EEPROM_used);
  EEPROM_used += setup_notes(EEPROM_used);
  EEPROM_used += setup_triggers(EEPROM_used);

  Serial.print("EEPROM_used: "); Serial.println(EEPROM_used);

  Periodic_offset[PULSE_NOTES_ON] = PULSE_NOTES_ON_OFFSET;
  Periodic_offset[PULSE_NOTES_OFF] = PULSE_NOTES_OFF_OFFSET;

  //Periodic_period[GET_POTS] = GET_POTS_PERIOD;  // mSec
  Periodic_offset[GET_POTS] = GET_POTS_OFFSET;
  //Periodic_period[SWITCH_REPORT] = SWITCH_REPORT_PERIOD;
  Periodic_offset[SWITCH_REPORT] = SWITCH_REPORT_OFFSET;

  /**
  // test error LEDs
  delay(2);
  for (i = 0; i < 20; i++) {
    digitalWrite(ERR_LED, HIGH);
    digitalWrite(ERR_LED_2, HIGH);
    delay(500);
    digitalWrite(ERR_LED, LOW);
    digitalWrite(ERR_LED_2, LOW);
    delay(500);
  }
  **/
  
  delay(2);

  /**
  Serial.print("sizeof(short) "); Serial.print(sizeof(short));
  Serial.print(", sizeof(int) "); Serial.print(sizeof(int));
  Serial.print(", sizeof(long) "); Serial.println(sizeof(long));
  **/

  test(0xfffffff8ul);
  test(0x0ul);
  test(0x7ffffff8ul);

  // FIX: scan_switches();   // Runs events which could set Errno

} // end setup()

byte Got_LED_response = 0;
byte Display_errors = 1;
byte Master_last_errno;
unsigned long Last_display_time;

// FIX: not needed
void receiveLEDErrno(int how_many) {
  if (how_many != 2) {
    Serial.print("receiveLEDResponse got "); Serial.print(how_many);
    Serial.println(" bytes, expected 2");
  } else {
    byte LED_errno = Wire.read();
    byte LED_err_data = Wire.read();
    if (LED_errno) {
      Remote_Errno[1] = LED_errno;
      Remote_Err_data[1] = LED_err_data;
    }
  }
  Got_LED_response = 1;
}

byte Debug = 0;

void running_help(void) {
  Serial.println();
  Serial.println(PROGRAM_ID);
  Serial.println();
  Serial.println(F("running:"));
  Serial.println(F("? - help"));
  Serial.println(F("I - initialize driver"));
  Serial.println(F("D - go into Debug mode"));
  Serial.println(F("W - show switch stats"));
  Serial.println(F("X<errno> - set Errno"));
  Serial.println(F("E - show Errno, Err_data"));
  Serial.println(F("S - show settings"));
  Serial.println(F("P<sw_debounce_period>,<enc_debounce_period> - set debounce_periods in EEPROM"));
  Serial.println(F("N - toggle Display_errors"));
  Serial.println(F("T - toggle Trace_events"));
  Serial.println(F("O - toggle periodic switch_report"));
  Serial.println(F("L - toggle periodic get_pots"));
  Serial.println(F("U - toggle Trace_encoders"));
  Serial.println(F("Z - toggle scan_switches_on"));
  Serial.println(F("Q - toggle scan_switches trace"));
  Serial.println(F("R<row> - dump switches on row"));
  Serial.println(F("A - dump triggers"));
  Serial.println(F("C - dump encoders"));
  Serial.println(F("F - dump pots"));
  Serial.println(F("B - dump Debounce_delay_counts"));
  Serial.println(F("M<controller:(P|L)>,<len_expected>,<comma_sep_bytes> - send I2C message to <controller>"));
  Serial.println(F("K<len_expected> - receive I2C report from pot_controller"));
  Serial.println(F("G<first>,<last> - Serial.write raw bytes in the range first-last (inclusive)"));
  Serial.println(F("V<first>,<last>\\n<raw_bytes> - verify that raw_bytes are in the range first-last"));
  Serial.println();
  Serial.print(F("sizeof(short) is ")); Serial.println(sizeof(short));
  Serial.print(F("sizeof(int) is ")); Serial.println(sizeof(int));
  Serial.print(F("sizeof(long) is ")); Serial.println(sizeof(long));
  Serial.println();
  // Unused letters: H J Y
}

void debug_help(void) {
  Serial.println();
  Serial.println(F("debug:"));
  Serial.println(F("? - help"));
  Serial.println(F("D - leave Debug mode"));
  Serial.println(F("X<errno> - set Errno"));
  Serial.println(F("E - show Errno, Err_data"));
  Serial.println(F("S - scan for shorts"));
  Serial.println(F("N - toggle Display_errors"));
  Serial.println(F("On - turn on output n"));
  Serial.println(F("In - turn on input n"));
  Serial.println(F("F - turn off test pin"));
  Serial.println(F("P<data> - send I2C command to pot_controller"));
  Serial.println(F("Q<len_expected> - receive I2C report from pot_controller"));
  Serial.println(F("L<data> - send I2C command to led_controller"));
  Serial.println(F("M<len_expected> - receive I2C report from led_controller"));
  Serial.println();
}

byte Debug_pin_high = 0xFF;    // 0xFF means no pins high

void turn_off_test_pin(void) {
  if (Debug_pin_high != 0xFF) {
    pinMode(Debug_pin_high, INPUT_PULLDOWN);
    Debug_pin_high = 0xFF;
  }
}

unsigned long I2C_send_time;    // uSec

void sendRequest(byte i2c_addr, byte *data, byte data_len) {
  byte b0, status;
  byte remote_index = i2c_addr - I2C_BASE;
  TwoWire *I2C_port = Remote_wire[remote_index];
  unsigned long start_time = micros();
  I2C_port->beginTransmission(i2c_addr);
  b0 = I2C_port->write(data, data_len);
  if (b0 != data_len) {
    Errno = 20;
    Err_data = 100 * remote_index + b0;
  }
  status = I2C_port->endTransmission();
  if (status) {
    Errno = 20 + status;        // 21 to 25
    Err_data = remote_index;
  }
  /***** FIX: delete
  if (i2c_addr == I2C_LED_CONTROLLER) {
    for (b0 = 0; b0 < 100; b0++) {
      if (Got_LED_response) break;
      delayMicroseconds(10);
    }
    if (Got_LED_response) Got_LED_response = 0;
    else {
      Errno = 41;
      Err_data = data[0];
    }
  }
  **********/
  unsigned long elapsed_time = micros() - start_time;
  if (elapsed_time > I2C_send_time) I2C_send_time = elapsed_time;
}

unsigned long I2C_request_from_time;    // uSec
unsigned long I2C_read_time;            // uSec

byte Response_data[32];

byte getResponse(byte i2c_addr, byte data_len, byte check_errno) {
  // Returns bytes received.  (0 if error)
  // Data in Response_data.
  byte i;
  byte remote_index = i2c_addr - I2C_BASE;
  TwoWire *I2C_port = Remote_wire[remote_index];
  unsigned long start_time = micros();
  if (data_len > 32) {
    Errno = 10;
    Err_data = 100 * remote_index + data_len;
    return 0;
  }
  byte bytes_received = I2C_port->requestFrom(i2c_addr, data_len);
  unsigned long mid_time = micros();
  unsigned long elapsed_time = mid_time - start_time;
  if (elapsed_time > I2C_request_from_time) I2C_request_from_time = elapsed_time;
  if (bytes_received > data_len) {
    //if (Serial) {
    //  Serial.print("Remote "); Serial.print(Remote_char[remote_index]);
    //  Serial.print(": Bytes_received too long, got "); Serial.print(bytes_received);
    //  Serial.print("expected "); Serial.println(data_len);
    //}
    Errno = 11;
    Err_data = 100 * remote_index + bytes_received;
    return 0;
  }
  if (bytes_received != I2C_port->available()) {
    //if (Serial) {
    //  Serial.print("Remote "); Serial.print(Remote_char[remote_index]);
    //  Serial.print(F(": I2C.requestFrom: bytes_received, ")); Serial.print(bytes_received);
    //  Serial.print(F(", != available(), ")); Serial.println(I2C_port->available());
    //}
    Errno = 12;
    Err_data = 100 * remote_index + I2C_port->available();
    return 0;
  }
  for (i = 0; i < bytes_received; i++) {
    Response_data[i] = I2C_port->read();
  }
  unsigned long read_time = micros() - mid_time;
  if (read_time > I2C_read_time) I2C_read_time = read_time;
  if (bytes_received == 0) {
    Errno = 13;
    Err_data = remote_index;
    return bytes_received;
  }
  if (bytes_received >= 2) {
    if ((check_errno || (bytes_received < data_len && bytes_received == 2))
        && Response_data[0] != 0
    ) {
      Remote_Errno[remote_index] = Response_data[0];
      Remote_Err_data[remote_index] = Response_data[1];
    } // end error check
  } // end if received at least 2 bytes
  return bytes_received;
}

void skip_ws(void) {
  // skips whitespace on Serial.
  while (isspace(Serial.peek())) Serial.read();
}

void report_error() {
  Serial.print("$E");
  Serial.write('M');
  Serial.write(Errno);
  Serial.write(Err_data);
  Errno = Err_data = 0;
}

void report_remote_error(byte remote_index) {
  Serial.print("$E");
  Serial.write(Remote_char[remote_index]);
  Serial.write(Remote_Errno[remote_index]);
  Serial.write(Remote_Err_data[remote_index]);
  Remote_Errno[remote_index] = 0;
  Remote_Err_data[remote_index] = 0;
}

byte Scan_switches_on = 1;
byte Scan_switches_trace = 0;

#define NUM_POTS                20

byte Current_pot_value[NUM_POTS];
byte Synced_pot_value[NUM_POTS];

unsigned long Last_get_pots_time;

void loop() {
  // put your main code here, to run repeatedly:
  byte b0, b1, b2, b3, b4, i;
  unsigned short us0, us1;
  byte buffer[32];

  if (!Debug) {
    if (Scan_switches_on) {
      scan_switches(Scan_switches_trace);   // takes optional trace param...
    }
    unsigned long now = millis();
    long interval_time;
    for (i = 0; i < NUM_PERIODICS; i++) {
      if (Periodic_period[i] && TIME_A_GEQ_B(now, Periodic_next[i])) {
        do {
          Periodic_next[i] += Periodic_period[i];
        } while (TIME_A_MINUS_B(Periodic_next[i], now) < Periodic_period[i]);
        switch (i) {
        case PULSE_NOTES_ON:
          notes_on();
          break;
        case PULSE_NOTES_OFF:
          notes_off();
          break;
        case UPDATE_LEDS:
          break;
        case GET_POTS:
          if (Last_get_pots_time) {
            interval_time = TIME_A_MINUS_B(now, Last_get_pots_time);
            if (interval_time < Periodic_period[GET_POTS] || interval_time > Periodic_period[GET_POTS] + 2) {
              Serial.print("GET_POTS missed interval, interval_time "); Serial.print(interval_time);
              Serial.print(", now "); Serial.print(now);
              Serial.print(", next "); Serial.print(Periodic_next[i]);
              Serial.print(", Last_get_pots_time "); Serial.println(Last_get_pots_time);
              Errno = 17;
              Err_data = interval_time;
            }
          }
          Last_get_pots_time = now;
          delayMicroseconds(20);
          b0 = getResponse(I2C_POT_CONTROLLER, 2 + NUM_POTS, 1);
          if (b0 != 2 + NUM_POTS) {
            if (Errno == 0) {
              Errno = 15;
              Err_data = b0;
            }
          } else {
            for (b1 = 0; b1 < NUM_POTS; b1++) {
              Current_pot_value[b1] = Response_data[2 + b1];
            }
          }
          break;
        case SEND_MIDI:
          break;
        case SWITCH_REPORT:
          if (Serial) {
            //Serial.println(F("still running..."));
            if (Trace_events) {
              for (i = 0; i < NUM_SWITCHES; i++) {
                if (Close_counts[i]) {
                  Serial.print(F("Switch "));
                  Serial.print(i);
                  Serial.print(F(" closed "));
                  Serial.print(Close_counts[i]);
                  Serial.println(F(" times"));
                  Close_counts[i] = 0;
                } // end if (Close_counts)
              } // end for (i)
            } // end if (Trace_events)
            if (Trace_encoders) {
              for (i = 0; i < NUM_ENCODERS; i++) {
                if (Encoders[i].var == NULL) {
                  Serial.print(F("Encoder "));
                  Serial.print(i);
                  Serial.println(F(": var is NULL"));
                } else if (Encoders[i].var->changed) {
                  Serial.print(F("Encoder "));
                  Serial.print(i);
                  Serial.print(F(" changed to "));
                  Serial.println(Encoders[i].var->value);
                  Encoders[i].var->changed = 0;
                } // end if (changed)
              } // end for (i)
            } // end if (Trace_events)
          } // end if (Serial)
          break;
        default:
          Errno = 16;
          Err_data = i;
          break;
        } // end switch (i)
      } // end if (period)
    } // for (i)
    if (Serial.available()) {
      b0 = toupper(Serial.read());
      switch (b0) {
      case '?': running_help(); break;
      case 'I':
        // initialize driver
        Serial.print("$I");   // init
        Serial.write(NUM_CHANNELS);
        Serial.write(NUM_CH_FUNCTIONS);
        Serial.write(NUM_FUNCTION_ENCODERS);
        Serial.write(NUM_HARMONICS);
        Serial.write(NUM_HM_FUNCTIONS);
        Driver_up = 1;
        Display_errors = 0;
        if (Errno) {
          report_error();
        }
        for (i = 0; i < NUM_REMOTES; i++) {
          if (Remote_Errno[i]) {
            report_remote_error(i);
          }
        }
        break;
      case 'D':
        for (b1 = 0; b1 < NUM_ROWS; b1++) {
          pinMode(Rows[b1], INPUT_PULLDOWN);
        } // end for (b1)
        Serial.println(F("Entering Debug mode"));
        Debug = 1;
        if (!Driver_up) {
          Serial.println("Turning Display_errors on");
          Display_errors = 1;
        }
        Serial.println();
        break;
      case 'W': // show switch stats
        Serial.println("Show switch stats (all times are uSec):");
        Serial.print(F("  Longest_open_bounce (switch): ")); Serial.println(Longest_open_bounce[0]);
        Longest_open_bounce[0] = 0;
        Serial.print(F("  Longest_open_bounce (encoder): ")); Serial.println(Longest_open_bounce[1]);
        Longest_open_bounce[1] = 0;
        Serial.print(F("  Shortest_close_bounce (switch): ")); Serial.println(Shortest_close_bounce[0]);
        Shortest_close_bounce[0] = 1000000ul;
        Serial.print(F("  Shortest_close_bounce (encoder): ")); Serial.println(Shortest_close_bounce[1]);
        Shortest_close_bounce[1] = 1000000ul;
        Serial.print(F("  Longest_wire_clear: ")); Serial.println(Longest_wire_clear);
        Longest_wire_clear = 0;
        Serial.print(F("  Longest_scan: ")); Serial.println(Longest_scan);
        Longest_scan = 0;
        Serial.println();
        break;
      case 'X': // set Errno
        skip_ws();
        Errno = Serial.parseInt();
        Serial.print(F("Errno set to ")); Serial.println(Errno);
        Serial.println();
        break;
      case 'E': // show Errno, Err_data
        Serial.print(F("Errno: "));
        Serial.print(Errno);
        Serial.print(F(", Err_data: "));
        Serial.println(Err_data);
        Errno = 0;
        Err_data = 0;
        for (i = 0; i < NUM_REMOTES; i++) {
          Serial.print("Remote "); Serial.print(Remote_char[i]);
          Serial.print(": Errno "); Serial.print(Remote_Errno[i]);
          Serial.print(", Err_data "); Serial.println(Remote_Err_data[i]);
          Remote_Errno[i] = 0;
          Remote_Err_data[i] = 0;
        }
        Serial.println();
        break;
      case 'S':  // show settings
        Serial.print(F("Switch debounce_period is ")); Serial.print(Debounce_period[0]);
        Serial.println(F(" uSec"));
        Serial.print(F("Encoder debounce_period is ")); Serial.print(Debounce_period[1]);
        Serial.println(F(" uSec"));
        Serial.println();
        break;
      case 'P':  // <debounce_period> - set debounce_period in EEPROM
        skip_ws();
        us0 = Serial.parseInt();
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("P: expected ',' after debounce_period[0], got ");
          Serial.println(b3);
        } else {
          us1 = Serial.parseInt();
          set_debounce_period(0, us0);
          Serial.print(F("Switch debounce_period set to ")); Serial.print(us0);
          Serial.println(F(" uSec in EEPROM"));
          set_debounce_period(1, us1);
          Serial.print(F("Encoder debounce_period set to ")); Serial.print(us1);
          Serial.println(F(" uSec in EEPROM"));
        }
        Serial.println();
        break;
      case 'N': // toggle Display_errors
        Display_errors = 1 - Display_errors;
        Serial.print(F("Display_errors set to "));
        Serial.println(Display_errors);
        Serial.println();
        break;
      case 'T': // toggle Trace_events
        Trace_events = 1 - Trace_events;
        Serial.print(F("Trace_events set to "));
        Serial.println(Trace_events);
        Serial.println();
        break;
      case 'O': // toggle periodic switch_report
        toggle_periodic_fun(SWITCH_REPORT, SWITCH_REPORT_PERIOD);
        Serial.print(F("Periodic_period[SWITCH_REPORT] set to "));
        Serial.print(Periodic_period[SWITCH_REPORT]);
        if (Periodic_period[SWITCH_REPORT]) {
          Serial.print(", in "); Serial.print(TIME_A_MINUS_B(Periodic_next[SWITCH_REPORT], now)); Serial.print(" mSec");
        }
        Serial.println();
        Serial.println();
        break;
      case 'L': // toggle periodic get_pots
        toggle_periodic_fun(GET_POTS, GET_POTS_PERIOD);
        Serial.print(F("Periodic_period[GET_POTS] set to "));
        Serial.print(Periodic_period[GET_POTS]);
        if (Periodic_period[GET_POTS]) {
          Serial.print(", in "); Serial.print(TIME_A_MINUS_B(Periodic_next[GET_POTS], now)); Serial.print(" mSec");
          Last_get_pots_time = 0;
        }
        Serial.println();
        Serial.println();
        break;
      case 'U': // toggle Trace_encoders
        Trace_encoders = 1 - Trace_encoders;
        Serial.print(F("Trace_encoders set to "));
        Serial.println(Trace_encoders);
        Serial.println();
        break;
      case 'Z': // toggle Scan_switches_on
        Scan_switches_on = 1 - Scan_switches_on;
        Serial.print(F("Scan_switches_on set to "));
        Serial.println(Scan_switches_on);
        Serial.println();
        break;
      case 'Q': // toggle Scan_switches_trace
        Scan_switches_trace = 1 - Scan_switches_trace;
        Serial.print(F("Scan_switches_trace set to "));
        Serial.println(Scan_switches_trace);
        Serial.println();
        break;
      case 'R':  // dump switches on row
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 >= NUM_ROWS) {
          Serial.print(F("Invalid row, must be < ")); Serial.println(NUM_ROWS);
        } else {
          for (b2 = 0; b2 < NUM_COLS; b2++) {
            b3 = SWITCH_NUM(b1, b2);
            Serial.print(F("Switch ")); Serial.print(b3);
            Serial.print(F(", row ")); Serial.print(b1);
            Serial.print(F(", col ")); Serial.print(b2);
            Serial.print(F(": current ")); Serial.print(Switches[b3].current);
            Serial.print(F(": debounce_index ")); Serial.print(Switches[b3].debounce_index);
            Serial.print(F(": tag ")); Serial.print(Switches[b3].tag);
            Serial.print(F(", closed_event ")); Serial.print(Switch_closed_event[b3]);
            Serial.print(F(", opened_event ")); Serial.println(Switch_opened_event[b3]);
          }
        }
        Serial.println();
        break;
      case 'A':  // dump triggers
        for (b1 = 0; b1 < NUM_TRIGGERS; b1++) {
          Serial.print(F("Trigger ")); Serial.print(b1);
          Serial.print(F(": switch_ ")); Serial.print(Triggers[b1].switch_);
          Serial.print(F(": button ")); Serial.print(Triggers[b1].button);
          Serial.print(F(": led ")); Serial.print(Triggers[b1].led);
          Serial.print(F(": check_event ")); Serial.print(Triggers[b1].check_event);
          Serial.print(F(": continuous ")); Serial.print(Triggers[b1].continuous);
          if (Triggers[b1].check_event == CHECK_POTS) {
            Serial.print(F(", pots: "));
            for (b2 = 0; b2 < Num_pots[b1]; b2++) {
              if (b2) Serial.print(F(", "));
              Serial.print(Pots[b1][b2]);
            }
          } // end if (CHECK_POTS)
          Serial.println();
        } // end for (b1)
        Serial.println();
        break;
      case 'C':  // dump encoders
        for (b1 = 0; b1 < NUM_ENCODERS; b1++) {
          b2 = Encoders[b1].A_sw;
          Serial.print(F("Encoder ")); Serial.print(b1);
          Serial.print(F(": A_sw ")); Serial.print(b2);
          Serial.print(F(", A closed_event ")); Serial.print(Switch_closed_event[b2]);
          Serial.print(F(", A opened_event ")); Serial.print(Switch_opened_event[b2]);
          Serial.print(F(", B closed_event ")); Serial.print(Switch_closed_event[b2 + 1]);
          Serial.print(F(", B opened_event ")); Serial.print(Switch_opened_event[b2 + 1]);
          if (Encoders[b1].var == 0) {
            Serial.println(F(", var is NULL"));
          } else {
            if (!(Encoders[b1].var->var_type->flags & ENCODER_FLAGS_DISABLED)) {
              Serial.println(F(", enabled"));
              Serial.print(F("         flags 0b"));
              Serial.print(Encoders[b1].var->var_type->flags, BIN);
              Serial.print(F(", display_value "));
              Serial.print(Encoders[b1].var->var_type->display_value);
              Serial.print(F(", encoder_event "));
              Serial.print(Encoders[b1].encoder_event);
              Serial.print(F(", value ")); Serial.print(Encoders[b1].var->value);
              if (Encoders[b1].var->changed) Serial.println(F(", changed"));
              else Serial.println(F(", not changed"));
            } else Serial.println(F(", not enabled"));
          }
        } // end for (b1)
        Serial.println();
        break;
      case 'F':  // dump pots
        for (b1 = 0; b1 < NUM_POTS; b1++) {
          Serial.print(F("Pot ")); Serial.print(b1);
          Serial.print(F(": current ")); Serial.print(Current_pot_value[b1]);
          Serial.print(F(", synched ")); Serial.println(Synced_pot_value[b1]);
        } // end for (b1)
        Serial.println();
        break;
      case 'B':  // dump debounce_delay_counts
        for (b1 = 0; b1 < 2; b1 += 1) {
          if (b1) {
            Serial.println("Dumping Encoder debounce_delay_counts:");
          } else {
            Serial.println("Dumping Switch debounce_delay_counts:");
          }
          for (b2 = 0; b2 <= MAX_DEBOUNCE_COUNT; b2 += 1) {
            byte count = Debounce_delay_counts[b1][b2];
            if (count) {
              Serial.print("  ");
              if (b2 == MAX_DEBOUNCE_COUNT) {
                Serial.print("overflow: ");
              } else {
                Serial.print(b2); Serial.print(" mSec: ");
              }
              Serial.println(count);
              Debounce_delay_counts[b1][b2] = 0;
            }
          }
          Serial.println();
        }
        Serial.println();
        break;
      case 'M':  // send I2C message to controller
        // M<controller>,<len_expected>,<comma_seperated_bytes> - send I2C message to <controller>
        skip_ws();
        b1 = Serial.read();     // controller
        switch (b1) {
        case 'p':
        case 'P':
          b1 = I2C_POT_CONTROLLER;
          break;
        case 'l':
        case 'L':
          b1 = I2C_LED_CONTROLLER;
          break;
        default:
          Serial.print("M: unrecognized controller ");
          Serial.println(b1);
          goto error;
        }
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("M: expected ',' after controller, got ");
          Serial.println(b3);
          goto error;
        }
        skip_ws();
        b2 = Serial.parseInt(); // len_expected
        if (b2 > 32) {
          Serial.print("M: len_expected > 32, got ");
          Serial.println(b2);
          goto error;
        }
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("M: expected ',' after len_expected, got ");
          Serial.println(b3);
          goto error;
        }
        skip_ws();
        buffer[0] = Serial.parseInt();
        for (i = 1; i < 32; i++) {
          b3 = Serial.read();  // delimiter char
          if (b3 == '\n') {
            sendRequest(b1, buffer, i);
            I2C_send_time = 0;
            break;
          } else if (b3 != ',') {
            Serial.print("M: expected ',' after byte ");
            Serial.print(i);
            Serial.print(", got ");
            Serial.println(b3);
            goto error;
          }
          skip_ws();
          buffer[i] = Serial.parseInt();
        } // end for (i)
        if (b2 == 0) {
          Serial.println("Message sent");
        } else {
          skip_ws();
          delayMicroseconds(200);   // give remote time to respond.
          b3 = getResponse(b1, b2, 0);
          I2C_request_from_time = 0;
          I2C_read_time = 0;
          if (b3 == 0) {
            Serial.println("no response");
          } else {
            Serial.print(Response_data[0]);
            for (i = 1; i < b3; i++) {
              Serial.print(", "); Serial.print(Response_data[i]);
            }
            Serial.println();
          }
        }
       error:
        Serial.println();
        break;
      case 'K': // receive I2C report from pot_controller
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 > 32) {
          Serial.println(F("ERROR: len_expected > 32"));
        } else {
          b2 = getResponse(I2C_POT_CONTROLLER, b1, 0);
          Serial.print(F("I2C_request_from_time ")); Serial.print(I2C_request_from_time);
          Serial.println(F(" uSec"));
          I2C_request_from_time = 0;
          Serial.print(F("I2C_read_time ")); Serial.print(I2C_read_time);
          Serial.println(F(" uSec"));
          I2C_read_time = 0;
          if (b2 == 0) {
            Serial.print("got Errno "); Serial.print(Errno);
            Serial.print(", Err_data "); Serial.println(Err_data);
          } else {
            Serial.print(Response_data[0]);
            for (i = 1; i < b2; i++) {
              Serial.print(", "); Serial.print(Response_data[i]);
            }
            Serial.println();
          }
        }
        Serial.println();
        break;
      case 'G': // G<first>,<last> - generate raw bytes in the range first-last (inclusive)
        b1 = Serial.parseInt(); // first
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("G: expected ',' after first, got ");
          Serial.println(b3);
          goto error;
        }
        skip_ws();
        b2 = Serial.parseInt(); // last
        b3 = Serial.read();     // \n
        if (b3 != '\n') {
          Serial.print("G: expected '\\n' after last, got ");
          Serial.println(b3);
          goto error;
        }
        for (i = b1; i <= b2; i++) {
          Serial.write(i);
          if (i == 255) break;  // so i doesn't get incremented back to 0
        }
        Serial.println();
        break;
      case 'V': // V<first>,<last>\n<raw_bytes> - verify that raw_bytes are in the range first-last
        b1 = Serial.parseInt(); // first
        b3 = Serial.read();     // comma delimiter
        if (b3 != ',') {
          Serial.print("V: expected ',' after first, got ");
          Serial.println(b3);
          goto error;
        }
        skip_ws();
        b2 = Serial.parseInt(); // last
        b3 = Serial.read();     // comma delimiter
        if (b3 != '\n') {
          Serial.print("V: expected '\\n' after last, got ");
          Serial.println(b3);
          goto error;
        }
        b4 = 0;  // num errors
        for (i = b1; i <= b2; i++) {
          b3 = Serial.read();
          if (b3 != i) {
            if (b4 == 0) {
              Serial.print("V ERRORS: ");
            } else {
              Serial.print(", ");
            }
            Serial.print("expected ");
            Serial.print(i);
            Serial.print(" got ");
            Serial.print(b3);
            b4 += 1;
          }
          if (i == 255) break;  // so i doesn't get incremented back to 0
        }
        if (b4 == 0) {
          Serial.println("V: no errors");
        } else {
          Serial.println();
        }
        Serial.println();
        break;
      case ' ': case '\t': case '\n': case '\r': break;
      default: running_help(); break;
      } // end switch

      while (Serial.available() && Serial.read() != '\n') ;
    } // end if (Serial.available())
  } else { // Debug
    if (Serial.available()) {
      b0 = toupper(Serial.read());
      switch (b0) {
      case '?': debug_help(); break;
      case 'D':
        Debug = 0;
        turn_off_test_pin();     
        for (b1 = 0; b1 < NUM_ROWS; b1++) {
          pinMode(Rows[b1], OUTPUT);
          digitalWrite(Rows[b1], LOW);
        } // end for (b1)
        Serial.println(F("Leaving Debug mode"));
        Serial.println();
        break;
      case 'X': // set Errno
        skip_ws();
        Errno = Serial.parseInt();
        Serial.print(F("Errno set to ")); Serial.println(Errno);
        Serial.println();
        break;
      case 'E': // show Errno, Err_data
        Serial.print(F("Errno: "));
        Serial.print(Errno);
        Serial.print(F(", Err_data: "));
        Serial.println(Err_data);
        Errno = 0;
        Err_data = 0;
        for (i = 0; i < NUM_REMOTES; i++) {
          Serial.print("Remote "); Serial.print(Remote_char[i]);
          Serial.print(": Errno "); Serial.print(Remote_Errno[i]);
          Serial.print(", Err_data "); Serial.println(Remote_Err_data[i]);
          Remote_Errno[i] = 0;
          Remote_Err_data[i] = 0;
        }
        Serial.println();
        break;
      case 'S': // scan for shorts
        turn_off_test_pin();     

        // Test output pins high
        for (b1 = 0; b1 < NUM_ROWS; b1++) {
          pinMode(Rows[b1], OUTPUT);
          digitalWrite(Rows[b1], HIGH);

          // Test other Output pins
          for (b2 = 0; b2 < NUM_ROWS; b2++) {
            if (b2 != b1 && digitalRead(Rows[b2])) {
              Serial.print(F("Possible short between O"));
              Serial.print(b1);
              Serial.print(F(" and O"));
              Serial.println(b2);
            }
          } // end for (b2)

          // Test Input pins
          for (b2 = 0; b2 < NUM_COLS; b2++) {
            if (digitalRead(Cols[b2])) {
              Serial.print(F("Possible short between O"));
              Serial.print(b1);
              Serial.print(F(" and I"));
              Serial.println(b2);
            }
          } // end for (b2)
          pinMode(Rows[b1], INPUT_PULLDOWN);
        } // end for (b1)

        // Test input pins high
        for (b1 = 0; b1 < NUM_COLS; b1++) {
          pinMode(Cols[b1], OUTPUT);
          digitalWrite(Cols[b1], HIGH);

          // Test other Input pins
          for (b2 = 0; b2 < NUM_COLS; b2++) {
            if (b2 != b1 && digitalRead(Cols[b2])) {
              Serial.print(F("Possible short between I"));
              Serial.print(b1);
              Serial.print(F(" and I"));
              Serial.println(b2);
            }
          } // end for (b2)

          // Test Output pins
          for (b2 = 0; b2 < NUM_ROWS; b2++) {
            if (digitalRead(Rows[b2])) {
              Serial.print(F("Possible short between I"));
              Serial.print(b1);
              Serial.print(F(" and O"));
              Serial.println(b2);
            }
          } // end for (b2)
          pinMode(Cols[b1], INPUT_PULLDOWN);
        } // end for (b1)
        Serial.println(F("Scan complete"));
        Serial.println();
        break;
      case 'N': // toggle Display_errors
        Display_errors = 1 - Display_errors;
        Serial.print(F("Display_errors set to "));
        Serial.println(Display_errors);
        Serial.println();
        break;
      case 'O': // turn on output n
        turn_off_test_pin();     
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 >= NUM_ROWS) {
          Serial.print(F("Invalid output pin, must be < "));
          Serial.println(NUM_ROWS);
        } else {
          pinMode(Rows[b1], OUTPUT);
          digitalWrite(Rows[b1], HIGH);
          Debug_pin_high = Rows[b1];
        }
        Serial.print(F("O"));
        Serial.print(b1);
        Serial.println(F(" on"));
        Serial.println();
        break;
      case 'I': // turn on input n
        turn_off_test_pin();     
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 >= NUM_COLS) {
          Serial.print(F("Invalid input pin, must be < "));
          Serial.println(NUM_COLS);
        } else {
          pinMode(Cols[b1], OUTPUT);
          digitalWrite(Cols[b1], HIGH);
          Debug_pin_high = Cols[b1];
        }
        Serial.print(F("I"));
        Serial.print(b1);
        Serial.println(F(" on"));
        Serial.println();
        break;
      case 'F': // turn off test pin
        turn_off_test_pin();
        Serial.println(F("test pin off"));
        Serial.println();
        break;
      case 'P': // send I2C command to pot_controller
        skip_ws();
        buffer[0] = Serial.parseInt();
        for (b1 = 1; b1 < 32; b1++) {
          b2 = Serial.read();
          if (b2 == '\n') {
            sendRequest(I2C_POT_CONTROLLER, buffer, b1);
            Serial.print(b1);
            Serial.println(" bytes sent to Pot Controller");
            for (b2 = 0; b2 < b1; b2++) {
              Serial.print(buffer[b2]);
              if (b2 + 1 < b1) {
                Serial.print(", ");
              }
            }
            Serial.println();
            Serial.println();
            Serial.print(F("I2C_send_time ")); Serial.print(I2C_send_time);
            Serial.println(F(" uSec"));
            I2C_send_time = 0;
            break;
          } else if (b2 != ',') {
            Serial.println(F("comma expected between bytes"));
            break;
          }
          skip_ws();
          buffer[b1] = Serial.parseInt();
        } // end for (b1)
        Serial.println();
        break;
      case 'Q': // receive I2C report from pot_controller
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 > 32) {
          Serial.println(F("ERROR: len_expected > 32"));
        } else {
          b2 = getResponse(I2C_POT_CONTROLLER, b1, 0);
          Serial.print(F("I2C_request_from_time ")); Serial.print(I2C_request_from_time);
          Serial.println(F(" uSec"));
          I2C_request_from_time = 0;
          Serial.print(F("I2C_read_time ")); Serial.print(I2C_read_time);
          Serial.println(F(" uSec"));
          I2C_read_time = 0;
          if (b2 == 0) {
            Serial.print("got Errno "); Serial.print(Errno);
            Serial.print(", Err_data "); Serial.println(Err_data);
          } else {
            Serial.print(Response_data[0]);
            for (i = 1; i < b2; i++) {
              Serial.print(", "); Serial.print(Response_data[i]);
            }
            Serial.println();
          }
        }
        Serial.println();
        break;
      case 'L': // send I2C command to led_controller
        skip_ws();
        buffer[0] = Serial.parseInt();
        for (b1 = 1; b1 < 32; b1++) {
          b2 = Serial.read();
          if (b2 == '\n') {
            sendRequest(I2C_LED_CONTROLLER, buffer, b1);
            Serial.print(b1);
            Serial.println(" bytes sent to LED Controller");
            for (b2 = 0; b2 < b1; b2++) {
              Serial.print(buffer[b2]);
              if (b2 + 1 < b1) {
                Serial.print(", ");
              }
            }
            Serial.println();
            Serial.println();
            Serial.print(F("I2C_send_time ")); Serial.print(I2C_send_time);
            Serial.println(F(" uSec"));
            I2C_send_time = 0;
            break;
          } else if (b2 != ',') {
            Serial.println(F("comma expected between bytes"));
            break;
          }
          skip_ws();
          buffer[b1] = Serial.parseInt();
        } // end for (b1)
        Serial.println();
        break;
      case 'M': // receive I2C report from led_controller
        skip_ws();
        b1 = Serial.parseInt();
        if (b1 > 32) {
          Serial.println(F("ERROR: len_expected > 32"));
        } else {
          b2 = getResponse(I2C_LED_CONTROLLER, b1, 0);
          Serial.print(F("I2C_request_from_time ")); Serial.print(I2C_request_from_time);
          Serial.println(F(" uSec"));
          I2C_request_from_time = 0;
          Serial.print(F("I2C_read_time ")); Serial.print(I2C_read_time);
          Serial.println(F(" uSec"));
          I2C_read_time = 0;
          if (b2 == 0) {
            Serial.print("got Errno "); Serial.print(Errno);
            Serial.print(", Err_data "); Serial.println(Err_data);
          } else {
            Serial.print(Response_data[0]);
            for (i = 1; i < b2; i++) {
              Serial.print(", "); Serial.print(Response_data[i]);
            }
            Serial.println();
          }
        }
        Serial.println();
        break;
      case ' ': case '\t': case '\n': case '\r': break;
      default: debug_help(); break;
      } // end switch

      while (Serial.available() && Serial.read() != '\n') ;
    } // end if (Serial.available())
  } // end else if (!Debug)

  // MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) ;  // read & ignore incoming messages

  if (Errno) {
    if (Display_errors &&
        (Errno != Master_last_errno || millis() - Last_display_time > MIN_ERROR_DISPLAY_INTERVAL)
    ) {
      Serial.print("Errno "); Serial.print(Errno); Serial.print(", Err_data "); Serial.println(Err_data);
      Master_last_errno = Errno;
      Last_display_time = millis();
      if (!Driver_up) {
        Errno = Err_data = 0;
      }
    }
    if (Driver_up) {
      report_error();
    }
  }

  for (i = 0; i < NUM_REMOTES; i++) {
    if (Remote_Errno[i]) {
      if (Driver_up) {
        report_remote_error(i);
      } else if (Serial && Display_errors &&
                 (Remote_Errno[i] != Remote_last_errno[i] ||
                  millis() - Remote_last_display_time[i] > MIN_ERROR_DISPLAY_INTERVAL)
      ) {
        Serial.print("Remote "); Serial.print(Remote_char[i]);
        Serial.print(": Errno "); Serial.print(Remote_Errno[i]);
        Serial.print(", Err_data "); Serial.println(Remote_Err_data[i]);
        Remote_last_errno[i] = Remote_Errno[i];
        Remote_last_display_time[i] = millis();
        Remote_Errno[i] = 0;
        Remote_Err_data[i] = 0;
      }
    } // end if (Remote_Errno)
  } // end for (i)

  errno();
}

// vim: sw=2
