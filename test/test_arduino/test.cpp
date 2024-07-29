#line 1 "first.cpp"
// first.cpp

#include <Arduino.h>
#include <EEPROM.h>

byte EEPROM[EEPROM_SIZE];

#line 1 "core_pins.cpp"
// core_pins.cpp

#include "core_pins.h"


// FIX: Implement these!

void pinMode(uint8_t pin, uint8_t mode) {
}

void digitalWrite(uint8_t pin, uint8_t val) {
}

uint8_t digitalRead(uint8_t pin) {
    return 0;
}

void digitalToggle(uint8_t pin) {
}


void delay(uint32_t msec) {
}

uint32_t millis(void) {
    return 0;
}

uint32_t micros(void) {
    return 0;
}

void delayMicroseconds(uint32_t usec) {
}

#line 1 "usb_serial.cpp"
// usb_serial.cpp

#include "usb_serial.h"


serial_class Serial;


// FIX: Implement these!

void serial_begin(long baud) {
}

bool serial_bool(void) {
  return 0;
}

int serial_available(void) {
  return 0;
}

int serial_peekchar(void) {
  return 0;
}

int serial_read(void) {
  return 0;
}

long serial_parseInt(LookaheadMode lookahead, char ignore) {
  return 0;
}

size_t serial_println_void(void) {
  return 0;
}

size_t serial_print_str(const char s[]) {
  return 0;
}

size_t serial_println_str(const char s[]) {
  return 0;
}

size_t serial_print_ulong(unsigned long l, int base) {
  return 0;
}

size_t serial_println_ulong(unsigned long l, int base) {
  return 0;
}

size_t serial_print_long(long l, int base) {
  return 0;
}

size_t serial_println_long(long l, int base) {
  return 0;
}

size_t serial_putchar(uint8_t c) {
  return 0;
}

size_t serial_print_double(double d) {
  return 0;
}

size_t serial_println_double(double d) {
  return 0;
}

#line 1 "Wire.cpp"
// Wire.cpp

#include "Wire.h"


// FIX: Implement these!

void TwoWire_begin_void(byte channel) {
}

void TwoWire_begin(byte channel, uint8_t address) {
}

void TwoWire_end(byte channel) {
}

void TwoWire_setClock(byte channel, uint32_t frequency) {
}

void TwoWire_beginTransmission(byte channel, uint8_t address) {
}

uint8_t TwoWire_endTransmission(byte channel, uint8_t sendStop) {
  return 0;
}

size_t TwoWire_write(byte channel, const uint8_t *buf_addr, size_t len) {
  return 0;
}

int TwoWire_available(byte channel) {
  return 0;
}

int TwoWire_read(byte channel) {
  return 0;
}

uint8_t TwoWire_requestFrom(byte channel, uint8_t address, uint8_t quantity, uint8_t sendStop) {
  return 0;
}


TwoWire Wire(0);
TwoWire Wire1(1);
#line 1 "usb_midi.cpp"
// usb_midi.cpp

#include "usb_midi.h"


void usb_midi_flush_output(void) {
}

int usb_midi_read(uint32_t channel) {
    return 0;
}

void usb_midi_send(uint8_t type, uint8_t data1, uint8_t data2, uint8_t channel, uint8_t cable) {
}


usb_midi_class usbMIDI;

#line 1 "sketch_master.ino"
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
#include "midi_control.h"

#define PROGRAM_ID    "Master V59"

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

  Wire.begin();                 // to LED controller
  Wire.setClock(400000);
  Wire1.begin();                // to Pot controller
  Wire1.setClock(400000);

  byte EEPROM_used = setup_switches(0);
  EEPROM_used += setup_events(EEPROM_used);
  EEPROM_used += setup_encoders(EEPROM_used);
  EEPROM_used += setup_functions(EEPROM_used);
  EEPROM_used += setup_notes(EEPROM_used);
  EEPROM_used += setup_triggers(EEPROM_used);
  EEPROM_used += setup_midi_control(EEPROM_used);

  Serial.print("EEPROM_used: "); Serial.println(EEPROM_used);

  Periodic_offset[PULSE_NOTES_ON] = PULSE_NOTES_ON_OFFSET;
  Periodic_offset[PULSE_NOTES_OFF] = PULSE_NOTES_OFF_OFFSET;

  Periodic_period[GET_POTS] = GET_POTS_PERIOD;  // mSec
  Periodic_offset[GET_POTS] = GET_POTS_OFFSET;
  Periodic_next[GET_POTS] = millis() + 500;     // start in ~500 mSec
  Periodic_next[GET_POTS] -= Periodic_next[GET_POTS] % Periodic_period[GET_POTS];
  Periodic_next[GET_POTS] += Periodic_offset[GET_POTS];
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

byte Display_errors = 1;
byte Master_last_errno;
unsigned long Last_display_time;

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
            if (interval_time < Periodic_period[GET_POTS] ||
                interval_time > Periodic_period[GET_POTS] + 2
            ) {
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
              if (Current_pot_value[b1] != Synced_pot_value[b1]) {
                pot_changed(b1);
              }
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
                if (Encoders[i].var == 0) {
                  Serial.print(F("Encoder "));
                  Serial.print(i);
                  Serial.println(F(": var is 0"));
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
            Serial.println(F(", var is 0"));
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
#line 1 "encoders.ino"
// encoders.ino

byte EEPROM_encoder_offset;

encoder_t Encoders[NUM_ENCODERS];

// byte _max, byte _display_value = 0xFF, byte _flags = 0, byte _bt_mul_down = 1, byte _min = 0,
// byte _bt_mul_up = 1
var_type_t Disabled(0, 0xFF, ENCODER_FLAGS_DISABLED);


void send_LED_request(byte *msg, byte length) {
  sendRequest(I2C_LED_CONTROLLER, msg, length);
  delayMicroseconds(200);
  getResponse(I2C_LED_CONTROLLER, 2, 1);
}

void led_on(byte led) {
  byte msg[2] = {14, led};
  send_LED_request(msg, 2);
}

void led_off(byte led) {
  byte msg[2] = {15, led};
  send_LED_request(msg, 2);
}

void select_led(byte enc) {
  variable_t *var = Encoders[enc].var;
  if (Trace_encoders) {
    Serial.print("select_led "); Serial.println(var->value);
  }
  byte msg[3] = {30, ((choices_t *)var->var_type)->choices_num, var->value};
  send_LED_request(msg, 3);
}

void turn_off_choices_leds(byte enc) {
  variable_t *var = Encoders[enc].var;
  if (Trace_encoders) {
    Serial.print("turn_off_choices_leds "); Serial.println(((choices_t *)var->var_type)->choices_num);
  }
  byte msg[2] = {29, ((choices_t *)var->var_type)->choices_num};
  send_LED_request(msg, 2);
} 

void display_digit(byte display_num, byte digit_num, byte num, byte dp) {
  if (Trace_encoders) {
    Serial.print("display_digit "); Serial.print(digit_num);
    Serial.print(", num "); Serial.print(num); Serial.print(", dp "); Serial.println(dp);
  }
  // 18<disp><digit#><value><dp>
  byte msg[5] = {18, display_num, digit_num, num, dp};
  send_LED_request(msg, 5);
}

void display_sharp_flat(byte display_num, byte sharp_flat) {
  // sharp_flat: 0 = nat (blank), 1 = SH, 2 = FL
  if (Trace_encoders) {
    Serial.print("display_sharp_flat "); Serial.println(sharp_flat);
  }
  // 21<disp><sharp_flat>: load sharp_flat
  byte msg[3] = {21, display_num, sharp_flat};
  send_LED_request(msg, 3);
}

void display_sharps_flats(byte enc) {
  variable_t *var = Encoders[enc].var;
  signed char n = var->value - 7;
  display_digit(enc, 0, abs(n), 0);
  if (n == 0) display_sharp_flat(enc, 0);     // natural
  else if (n < 0) display_sharp_flat(enc, 2); // flat
  else display_sharp_flat(enc, 1);            // sharp
}

void display_number(byte display_num, short num, byte dp) {
  if (Trace_encoders) {
    Serial.print("display_number "); Serial.print(num); Serial.print(", dp "); Serial.println(dp);
  }
  // 19<disp><value_s16><dec_place>
  byte msg[5] = {19, display_num, byte(num >> 8), byte(num & 0xFF), dp};
  send_LED_request(msg, 5);
}

void clear_numeric_display(byte display_num) {
  if (Trace_encoders) {
    Serial.print("clear_numeric_display "); Serial.println(display_num);
  }
  byte msg[2] = {31, display_num};
  send_LED_request(msg, 2);
}

const short Powers_of_ten[] = {1, 10, 100, 1000, 10000};

void display_linear_number(byte enc) {
  // number displayed is value*10^dp / scale + offset
  variable_t *var = Encoders[enc].var;
  linear_number_t *var_type = (linear_number_t *)var->var_type;
  long n = var->value;
  n *= Powers_of_ten[var_type->dp + var_type->extra_10s];
  n = (n + var_type->scale / 2) / var_type->scale;  // divide rounded
  n += var_type->offset;
  signed char dp = var_type->dp;
  if (var_type->trim) {
    if (n < -99) n = -(-n % 100);
    else if (n > 999) n = n % 1000;
  } else while (n < -99 || n > 999) {
    n /= 10;
    dp -= 1;
  }
  display_number(enc, short(n), max(0, dp));
}

void display_geometric_number(byte enc) {
  variable_t *var = Encoders[enc].var;
  geometric_number_t *var_type = (geometric_number_t *)var->var_type;
  float n = exp(var_type->m * var->value + var_type->b) + var_type->c;
  if (abs(n) < 0.005) display_number(enc, 0, 0);   // display as 0
  short s;
  if (n < -9.95) {
    s = (short)(n - 0.5);
    display_number(enc, s, 0);
  } else if (n < -0.995) {
    s = (short)(n * 10.0 - 0.5);
    display_number(enc, s, 1);
  } else if (n < -0.005) {
    s = (short)(n * 100.0 - 0.5);
    display_number(enc, s, 2);
  } else if (n > 99.95) {
    s = (short)(n + 0.5);
    display_number(enc, s, 0);
  } else if (n > 9.995) {
    s = (short)(n * 10.0 + 0.05);
    display_number(enc, s, 1);
  } else {
    s = (short)(n * 100.0 + 0.005);
    display_number(enc, s, 2);
  }
}

void display_a_note(byte display_num, byte note, byte sharp_flat) {
  // note: 0-6 = A-G, sharp_flat: 0 = nat (blank), 1 = SH, 2 = FL
  if (Trace_encoders) {
    Serial.print("display_note "); Serial.print(note);
    Serial.print(", sharp_flat "); Serial.println(sharp_flat);
  }
  // 20<disp><note><sharp_flat>: load note
  byte msg[4] = {20, display_num, note, sharp_flat};
  send_LED_request(msg, 4);
}

void display_note(byte enc) {
  variable_t *var = Encoders[enc].var;
  note_t *var_type = (note_t *)var->var_type;
  byte value = var->value;
  if (var_type->include_null && value == var_type->num_notes) { // NULL setting
    clear_numeric_display(enc);
  } else {
    const char *note = var_type->notes[value];
    if (note[1] == 0) {  // no sharp/flat
      display_a_note(enc, note[0] - 'A', 0);
    } else if (note[1] == '#') {
      display_a_note(enc, note[0] - 'A', 1);
    } else if (note[1] == 'b') {
      display_a_note(enc, note[0] - 'A', 2);
    } else {
      Errno = 31;
      Err_data = note[1];
    }
  }
}

 
// starts disabled until a ch sw turned on
// byte _choices_num, byte _choices_length, byte _bt_mul_down = 1, byte _additional_flags = 0
choices_t Function_var_type(0, NUM_FUNCTIONS, 4, ENCODER_FLAGS_DISABLED);

// static variables
variable_t Filename_var = {&Disabled};
variable_t Function_var = {&Function_var_type};

void set_debounce(byte A_sw) {
  Switches[A_sw].debounce_index = Switches[A_sw + 1].debounce_index = 1;
}

byte setup_encoders(byte EEPROM_offset) {
  EEPROM_encoder_offset = EEPROM_offset;

  // Function Params
  Encoders[0].A_sw = SWITCH_NUM(2, 3);
  Encoders[0].display_num = 0;
  Encoders[0].encoder_event = FUN_PARAM_CHANGED;
  Encoders[0].var = 0;
  set_debounce(Encoders[0].A_sw);
  Encoders[1].A_sw = SWITCH_NUM(2, 6);
  Encoders[1].display_num = 1;
  Encoders[1].encoder_event = FUN_PARAM_CHANGED;
  Encoders[1].var = 0;
  set_debounce(Encoders[1].A_sw);
  Encoders[2].A_sw = SWITCH_NUM(3, 0);
  Encoders[2].display_num = 2;
  Encoders[2].encoder_event = FUN_PARAM_CHANGED;
  Encoders[2].var = 0;
  set_debounce(Encoders[2].A_sw);
  Encoders[3].A_sw = SWITCH_NUM(3, 3);
  Encoders[3].display_num = 3;
  Encoders[3].encoder_event = FUN_PARAM_CHANGED;
  Encoders[3].var = 0;
  set_debounce(Encoders[3].A_sw);

  // Function
  Encoders[FUNCTION_ENCODER].A_sw = SWITCH_NUM(2, 0);
  Encoders[FUNCTION_ENCODER].display_num = 176;  // first LED #
  Encoders[FUNCTION_ENCODER].var = &Function_var;
  Encoders[FUNCTION_ENCODER].encoder_event = FUNCTION_CHANGED;
  set_debounce(Encoders[FUNCTION_ENCODER].A_sw);

  // Filename
  Encoders[FILENAME_ENCODER].A_sw = SWITCH_NUM(0, 5);
  Encoders[FILENAME_ENCODER].var = &Filename_var;
  Encoders[FILENAME_ENCODER].encoder_event = 0xFF;  // FIX: figure this out later
  set_debounce(Encoders[FILENAME_ENCODER].A_sw);

  byte i;
  for (i = 0; i < NUM_ENCODERS; i++) {
    Switch_closed_event[Encoders[i].A_sw]     = ENC_A_CLOSED(i);   // switch closed ch A
    Switch_closed_event[Encoders[i].A_sw + 1] = ENC_B_CLOSED(i);   // switch closed ch B
    Switch_opened_event[Encoders[i].A_sw]     = ENC_A_OPENED(i);   // switch opened ch A
    Switch_opened_event[Encoders[i].A_sw + 1] = ENC_B_OPENED(i);   // switch opened ch B
  } // for (i)

  return 0;  // for now...
}

// vim: sw=2
#line 1 "events.ino"
// events.ino

byte EEPROM_events_offset;

byte Trace_events = 0;
byte Trace_encoders = 0;

void inc_encoder(byte enc) {
  // Called each time both encoders switches become open (detent position reached).
  // Only called if encoder is enabled.
  byte new_value;
  if (Trace_encoders) {
    Serial.print("inc_encoder "); Serial.print(enc);
    Serial.print(", count "); Serial.print(Encoders[enc].count);
    Serial.print(", value "); Serial.print(Encoders[enc].var->value);
    Serial.print(", max "); Serial.print(Encoders[enc].var->var_type->max);
    Serial.print(", min "); Serial.print(Encoders[enc].var->var_type->min);
    Serial.print(", flags 0b"); Serial.println(Encoders[enc].var->var_type->flags, BIN);
  }
  if (Encoders[enc].count >= 2) {
    // inc encoder value
    variable_t *var = Encoders[enc].var;
    new_value = var->value + var->var_type->bt_mul[Switches[Encoders[enc].A_sw + 2].current];
    if (new_value > var->var_type->max) {
      if (Trace_encoders) {
        Serial.print("  new_value ("); Serial.print(new_value); Serial.println(") > max");
      }
      if (var->var_type->flags & ENCODER_FLAGS_CYCLE) {
        // cycle
        if (Trace_encoders) {
          Serial.println("  CYCLE on");
        }
        var->value = new_value - (var->var_type->max + 1 - var->var_type->min);
        var->changed = 1;
        Function_changed = 1;
        encoder_changed(enc);
      } else if (var->value != var->var_type->max) {
        if (Trace_encoders) {
          Serial.println("  current value != max");
        }
        var->value = var->var_type->max;
        var->changed = 1;
        Function_changed = 1;
        encoder_changed(enc);
      }
    } else {
      var->value = new_value;
      var->changed = 1;
      Function_changed = 1;
      encoder_changed(enc);
    }
  } else if (Encoders[enc].count <= -2) {
    // dec encoder value
    variable_t *var = Encoders[enc].var;
    byte adj = var->var_type->bt_mul[Switches[Encoders[enc].A_sw + 2].current];
    if (var->value < var->var_type->min + adj) {
      if (Trace_encoders) {
        Serial.print("  value < min + adj ("); Serial.print(adj); Serial.println(")");
      }
      if (var->var_type->flags & ENCODER_FLAGS_CYCLE) {
        // cycle
        if (Trace_encoders) {
          Serial.println("  CYCLE on");
        }
        var->value = (var->value + var->var_type->max + 1) - var->var_type->min - adj;
        var->changed = 1;
        Function_changed = 1;
        encoder_changed(enc);
      } else if (var->value != var->var_type->min) {
        if (Trace_encoders) {
          Serial.println("  current value != min");
        }
        var->value = var->var_type->min;
        var->changed = 1;
        Function_changed = 1;
        encoder_changed(enc);
      }
    } else {
      var->value -= adj;
      var->changed = 1;
      Function_changed = 1;
      encoder_changed(enc);
    }
  }
  Encoders[enc].count = 0;
}

void run_event(byte event_num, byte param) {
  // This runs Switch_closed_events, Switch_opened_events and Encoder display_value and encoder_events.
  if (event_num != 0xFF) {
    byte enc, pots_index, pot;
    trigger_t *trig;
    variable_t *var;
    switch (event_num) {
    case ENC_A_CLOSED(0): case ENC_A_CLOSED(1): case ENC_A_CLOSED(2):
    case ENC_A_CLOSED(3): case ENC_A_CLOSED(4): case ENC_A_CLOSED(5):
      // switch A closed for encoders
      enc = EVENT_ENCODER_NUM(event_num);
      var = Encoders[enc].var;
      if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {  // enabled
        byte B_sw = Encoders[enc].A_sw + 1;
        if (!Switches[B_sw].current) {  // B open
          // CW
          Encoders[enc].count += 1;
        } else {
          // CCW
          Encoders[enc].count -= 1;
        }
      } // end if (enabled)
      break;
    case ENC_B_CLOSED(0): case ENC_B_CLOSED(1): case ENC_B_CLOSED(2):
    case ENC_B_CLOSED(3): case ENC_B_CLOSED(4): case ENC_B_CLOSED(5):
      // switch closed for encoders B
      enc = EVENT_ENCODER_NUM(event_num);
      var = Encoders[enc].var;
      if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {  // enabled
        byte A_sw = Encoders[enc].A_sw;
        if (Switches[A_sw].current) {   // A closed
          // CW
          Encoders[enc].count += 1;
        } else {
          // CCW
          Encoders[enc].count -= 1;
        }
      } // end if (enabled)
      break;
    case ENC_A_OPENED(0): case ENC_A_OPENED(1): case ENC_A_OPENED(2):
    case ENC_A_OPENED(3): case ENC_A_OPENED(4): case ENC_A_OPENED(5):
      // switch A opened for encoders
      enc = EVENT_ENCODER_NUM(event_num);
      var = Encoders[enc].var;
      if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {  // enabled
        byte B_sw = Encoders[enc].A_sw + 1;
        if (Switches[B_sw].current) {   // B closed
          // CW
          Encoders[enc].count += 1;
        } else {
          // CCW, back at detent
          Encoders[enc].count -= 1;
          inc_encoder(enc);
        }
      } // end if (enabled)
      break;
    case ENC_B_OPENED(0): case ENC_B_OPENED(1): case ENC_B_OPENED(2):
    case ENC_B_OPENED(3): case ENC_B_OPENED(4): case ENC_B_OPENED(5):
      // switch B opened for encoders
      enc = EVENT_ENCODER_NUM(event_num);
      var = Encoders[enc].var;
      if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {  // enabled
        byte A_sw = Encoders[enc].A_sw;
        if (!Switches[A_sw].current) {   // A open
          // CW, back at detent
          Encoders[enc].count += 1;
          inc_encoder(enc);
        } else {
          // CCW
          Encoders[enc].count -= 1;
        }
      } // end if (enabled)
      break;
    case FUNCTION_CHANGED: // Function changed
      reset_function_encoders();
      break;
    case NOTE_BT_ON:   // tagged to note buttons
      note_on_by_bt(param);
      break;
    case NOTE_BT_OFF:  // tagged to note buttons
      note_off_by_bt(param);
      break;
    case NOTE_SW_ON:   // tagged to note switches
      note_on_by_sw(param);
      break;
    case NOTE_SW_OFF:  // tagged to note switches
      note_off_by_sw(param);
      break;
    case CONTINUOUS:
      // changing from pulse to continuous, notes may be off right now!
      turn_off_periodic_fun(PULSE_NOTES_ON);
      turn_off_periodic_fun(PULSE_NOTES_OFF);
      notes_on();
      break;
    case PULSE:
      // changing from continuous to pulse, but no notes may be switched on now...
      check_pulse_on();
      break;
    case CHANNEL_ON:
      channel_on(param);
      disable_triggers();
      break;
    case CHANNEL_OFF:
      channel_off(param);
      disable_triggers();
      break;
    case HARMONIC_ON:
      harmonic_on(param);
      disable_triggers();
      break;
    case HARMONIC_OFF:
      harmonic_off(param);
      disable_triggers();
      break;
    case UPDATE_CHOICES:  // enc
      select_led(param);
      break;
    case UPDATE_LINEAR_NUM:  // enc
      display_linear_number(param);
      break;
    case UPDATE_GEOMETRIC_NUM:  // enc
      display_geometric_number(param);
      break;
    case UPDATE_NOTE:  // enc
      display_note(param);
      break;
    case UPDATE_SHARPS_FLATS:  // enc
      display_sharps_flats(param);
      break;
    case TRIGGER_SW_ON:   // trigger switch
      trig = &Triggers[Switches[param].tag];
      if (Lowest_channel != 0xFF) {
        trig->continuous = 1;
        led_on(trig->led);
      }
      break;
    case TRIGGER_SW_OFF:  // trigger switch
      trig = &Triggers[Switches[param].tag];
      trig->continuous = 0;
      led_off(trig->led);
      break;
    case TRIGGER_BT_PRESSED:  // trigger button
      trig = &Triggers[Switches[param].tag];
      if (!trig->continuous && Lowest_channel != 0xFF) {
        led_on(trig->led);
        check_trigger(Switches[param].tag);
      }
      break;
    case TRIGGER_BT_RELEASED: // trigger button
      trig = &Triggers[Switches[param].tag];
      if (!trig->continuous) {
        led_off(trig->led);
      }
      break;
    case CHECK_POTS: // trigger_num, called when trigger button is pressed
      for (pots_index = 0; pots_index < Num_pots[param]; pots_index++) {
        pot = Pots[param][pots_index];
        if (Synced_pot_value[pot] != Current_pot_value[pot]) {
          send_pot(pot);
        }
      }
      break;
    case CHECK_FUNCTIONS: // trigger_num, called when trigger button is pressed
      if (Function_changed) {
        send_function();
      }
      break;
    case FUN_PARAM_CHANGED: // enc, called when fun parameter encoder changes
      if (Triggers[FUNCTIONS_TRIGGER].continuous) {
        send_function();
      }
      break;
    default:
      Errno = 30;
      Err_data = event_num;
      break;
    } // end switch (event_num)
  } // end if (0xFF)
} // end run_event()

byte Switch_closed_event[NUM_SWITCHES]; // 0xFF is None
byte Switch_opened_event[NUM_SWITCHES]; // 0xFF is None

void switch_closed(byte sw) {
  if (Trace_events) {
    Serial.print(F("switch "));
    Serial.print(sw);
    Serial.print(F(" closed, event "));
    Serial.println(Switch_closed_event[sw]);
  }
  run_event(Switch_closed_event[sw], sw);
}

void switch_opened(byte sw) {
  if (Trace_events) {
    Serial.print(F("switch "));
    Serial.print(sw);
    Serial.print(F(" opened, event "));
    Serial.println(Switch_opened_event[sw]);
  }
  run_event(Switch_opened_event[sw], sw);
}

void encoder_changed(byte enc) {
  if (Trace_encoders) {
    Serial.print(F("encoder "));
    Serial.print(enc);
    Serial.print(F(" changed, display_value "));
    Serial.print(Encoders[enc].var->var_type->display_value);
    Serial.print(F(", new value "));
    Serial.print(Encoders[enc].var->value);
    Serial.print(F(", encoder_event "));
    Serial.println(Encoders[enc].encoder_event);
  }
  run_event(Encoders[enc].var->var_type->display_value, enc);
  run_event(Encoders[enc].encoder_event, enc);
}

byte setup_events(byte EEPROM_offset) {
  EEPROM_events_offset = EEPROM_offset;
  byte i;
  for (i = 0; i < NUM_SWITCHES; i++) {
    Switch_closed_event[i] = 0xFF;
    Switch_opened_event[i] = 0xFF;
  }

  return 0;  // for now...
}

// vim: sw=2
#line 1 "flash_errno.ino"
// flash_errno.ino

#define MAX_DELAYS   42

// All of these are in mSec:
#define SHORT_ON     150
#define SHORT_OFF    150
#define LONG_ON      750
#define OFF_BETWEEN_DIGITS     600
#define OFF_BETWEEN_REPEATS   1800

unsigned short Delays[MAX_DELAYS];
byte Num_delays;

byte Current_delay = 0;
byte Level;
unsigned long Start_time;

byte Errno;
byte Err_data;

byte Last_errno = 0;

byte LED_pin1 = 13, LED_pin2 = 0xFF;

void err_led(byte led_pin1, byte led_pin2) {
//void err_led(byte led_pin1, byte led_pin2=0xFF) {
  pinMode(led_pin1, OUTPUT);
  digitalWrite(led_pin1, LOW);
  LED_pin1 = led_pin1;
  if (led_pin2 != 0xFF) {
    pinMode(led_pin2, OUTPUT);
    digitalWrite(led_pin2, LOW);
    LED_pin2 = led_pin2;
  }
}

void push(int delay) {
  if (Num_delays < MAX_DELAYS) {
    Delays[Num_delays++] = delay;
    //Serial.println(delay);
  }
}

void errno(void) {
  if (Errno == 0) {
    if (Last_errno) {
      Last_errno = 0;
      Num_delays = 0;
      digitalWrite(LED_pin1, LOW);
      if (LED_pin2 != 0xFF) {
        digitalWrite(LED_pin2, LOW);
      }
    } // end if (Last_errno)
  } else {
    if (Last_errno == 0) {
      // Display this errno!
      byte current_errno = Errno;
      Last_errno = Errno;
      byte i;
      byte divisor;
      byte seen_first = 0;
      Num_delays = 0;  // just to be sure...
      for (divisor = 100; divisor; divisor /= 10) {
        // one time through per digit, left to right
        byte remainder = current_errno % divisor;
        byte digit = (current_errno - remainder) / divisor;
        current_errno = remainder;
        if (digit == 0) {
          if (seen_first || divisor == 1) {
            push(LONG_ON);  // LONG on
            push(OFF_BETWEEN_DIGITS);  // off between digits
          }
        } else {
          for (i = 0; i < digit - 1; i++) {
            push(SHORT_ON);  // on
            push(SHORT_OFF);   // off
          }
          push(SHORT_ON);    // last on
          push(OFF_BETWEEN_DIGITS);      // off between digits
          seen_first = 1;
        }
      } // end for (divisor)

      // Change final delay to seperate repetitions
      Delays[Num_delays - 1] = OFF_BETWEEN_REPEATS;

      Start_time = millis();
      Current_delay = 0;
      digitalWrite(LED_pin1, HIGH);
      if (LED_pin2 != 0xFF) {
        digitalWrite(LED_pin2, HIGH);
      }
      Level = 0;
    } // end if (Last_errno == 0)
  } // end else if (Errno == 0)
  if (Current_delay < Num_delays) {
    unsigned long current_time = millis();
    if (current_time - Start_time >= Delays[Current_delay]) {
      digitalWrite(LED_pin1, Level);
      if (LED_pin2 != 0xFF) {
        digitalWrite(LED_pin2, Level);
      }
      Level = 1 - Level;
      Start_time = current_time;
      if (++Current_delay >= Num_delays) Current_delay = 0;  // wrap around
    }
  } // end if (Current_delay < Num_delays)
}

// vim: sw=2
#line 1 "functions.ino"
// functions.ino

/**
#define NUM_CH_FUNCTIONS        8
#define NUM_HM_FUNCTIONS        7
#define NUM_FUNCTIONS           (NUM_CH_FUNCTIONS + NUM_HM_FUNCTIONS)
#define NUM_FUNCTION_ENCODERS   4
**/

byte EEPROM_functions_offset;

byte Function_changed;

// 512 bytes
byte Channel_memory[NUM_CHANNELS][NUM_CH_FUNCTIONS][NUM_FUNCTION_ENCODERS];

// 4480 bytes
byte Harmonic_memory[NUM_CHANNELS][NUM_HARMONICS][NUM_HM_FUNCTIONS][NUM_FUNCTION_ENCODERS];

// Each variable_t.var_type has {min, max, flags, bt_mul[button_up], bt_mul[button_down], param_num}
// 
// flags are:
//
//    ENCODER_FLAGS_DISABLED      0b001
//    ENCODER_FLAGS_CYCLE         0b010
//    ENCODER_FLAGS_CHOICE_LEDS   0b100

// byte _choices_num, byte _choices_length, byte _bt_mul_down = 1, byte _additional_flags = 0
choices_t Major_minor(1, 2, 1, ENCODER_FLAGS_CYCLE);
choices_t Meantone(2, 2, 1, ENCODER_FLAGS_CYCLE);
choices_t Just_intonation(3, 14, 4);

sharps_flats_t Flats_and_sharps;
 
// number displayed is value*10^(dp+extra_10s) / scale + offset
// byte _min, byte _max, long _offset = 0, byte _bt_mul_down = 1, byte _dp = 0, long _scale = 1,
// byte extra_10s = 0, byte _trim = 0, byte _flags = 0
linear_number_t Cents_per_semitone(0, 20, 99858, 10, 2, 12, 0, 1);
linear_number_t Max_octave_fudge(0, 127, 0, 10, 1, 3);
linear_number_t Channel_num(1, 16, 0, 4);
linear_number_t Octave(0, 7);
linear_number_t Frequency_offset(0, 127, -318, 10, 1, 2);
linear_number_t Scale_3(0, 5);
linear_number_t Freq_ramp_start(0, 15, -400, 4, 0, 2, 2);
linear_number_t Attack_start(0, 15, -40, 4, 0, 3, 1);
linear_number_t Bend(0, 7, -100, 1, 0, 35, 3);
linear_number_t Cycle_time(0, 15, 5, 1, 2, 20);
linear_number_t Sine_ampl_swing(0, 127, 0, 10, 0, 3333, 4);
linear_number_t Sustain_ampl_swing(0, 7, 0, 1, 1, 7, 1);
linear_number_t Center_ampl(0, 15, 5, 4);
linear_number_t Ampl_offset(0, 127, 0, 10, 1, 1411, 3);
linear_number_t Freq_course(0, 127, 0, 10, 1, 10);
linear_number_t Freq_fine(0, 99, 0, 10);

// byte _max, float _limit, float _b, float _start = 0.0, byte _bt_mul_down = 1
// dur = exp(0.049957498*value + 1.5) - 4.48168907
// value = (log(dur + 4.48168907) - 1.5)/0.049957498
geometric_number_s Duration_5(15, 5.0, 1.5);
// dur = exp(0.068489354*value + 1) - 2.718281828
// value = (log(dur + 2.718281828) - 1)/0.068489354
geometric_number_s Duration_20(31, 20.0, 1.0);

// byte _num_notes, char **_notes, byte _include_null = 0, byte _flags = 0, byte _min = 0
const char *Note_list_12[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
note_t Notes_12(12, Note_list_12);
note_t Notes_12_null(12, Note_list_12, 1); // 12 is null
const char *Note_list_2[] = {"Eb", "Ab"};
note_t Notes_Eb_Ab_null(2, Note_list_2, 1, ENCODER_FLAGS_CYCLE);  // 2 is null


// {&var_type}
variable_t Functions[NUM_FUNCTIONS][NUM_FUNCTION_ENCODERS] = {
  {  // function 0: key_signature
    {&Flats_and_sharps, 7}, {&Disabled}, {&Disabled}, {&Major_minor},
  },
  {  // function 1: tune absolute
    {&Disabled}, {&Notes_12, 9}, {&Octave, 4}, {&Frequency_offset, 64},
  },
  {  // function 2: match tuning
    {&Disabled}, {&Notes_12, 9}, {&Channel_num, 1}, {&Disabled},
  },
  {  // function 3: equal_temperament
    {&Disabled}, {&Disabled}, {&Cents_per_semitone, 17}, {&Disabled},
  },
  {  // function 4: well_tempered
    {&Notes_12}, {&Notes_12_null, 12}, {&Disabled}, {&Disabled},
  },
  {  // function 5: meantone
    {&Notes_12}, {&Notes_Eb_Ab_null, 2}, {&Max_octave_fudge}, {&Meantone},
  },
  {  // function 6: just_intonation
    {&Notes_12}, {&Notes_12_null, 12}, {&Disabled}, {&Just_intonation},
  },
  {  // function 7: pythagorean
    {&Notes_12}, {&Disabled}, {&Max_octave_fudge}, {&Disabled},
  },
  {  // function 8: harmonic basics
    {&Ampl_offset, 4}, {&Freq_course}, {&Freq_fine}, {&Disabled},
  },
  {  // function 9: freq env: ramp
    {&Scale_3}, {&Freq_ramp_start}, {&Duration_5, 1}, {&Bend, 4},
  },
  {  // function 10: freq env: sine
    {&Scale_3}, {&Cycle_time, 3}, {&Sine_ampl_swing, 15}, {&Disabled},
  },
  {  // function 11: attack
    {&Scale_3}, {&Attack_start}, {&Duration_5, 1}, {&Bend, 4},
  },
  {  // function 12: decay
    {&Scale_3}, {&Disabled}, {&Duration_20, 1}, {&Bend, 4},
  },
  {  // function 13: sustain
    {&Scale_3}, {&Cycle_time, 7}, {&Sustain_ampl_swing, 4}, {&Center_ampl, 7},
  },
  {  // function 14: release
    {&Scale_3}, {&Disabled}, {&Duration_5, 3}, {&Bend, 4},
  },
};

byte Lowest_harmonic = 0xFF;  // 0-9, 0xFF when all switches off
unsigned short Harmonic_bitmap;
byte Lowest_channel = 0xFF;   // 0-15, 0xFF when all switches off
unsigned short Channel_bitmap;
byte Buffer[NUM_FUNCTION_ENCODERS];

void update_channel_memory(byte ch) { 
  for (byte enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
    if (Encoders[enc].var) {
      Channel_memory[ch][FUNCTION][enc] = Encoders[enc].var->value;
    }
  }
}

void update_harmonic_memory(byte ch, byte hm) { 
  for (byte enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
    if (Encoders[enc].var) {
      Harmonic_memory[ch][hm][FUNCTION - NUM_CH_FUNCTIONS][enc] = Encoders[enc].var->value;
    }
  }
}

void load_functions(byte skip_ch_functions) {
  // Loads Functions.values from Function_memory and Harmonic_memory.
  // Called when the Lowest_channel or Lowest_harmonic changes.
  // Resets Functions.changed.
  // If current FUNCTION changed, updates displays.

  if (Lowest_channel != 0xFF) {
    byte fun, enc;
    byte fun_changed = 0;
    if (!skip_ch_functions) {
      for (fun = 0; fun < NUM_CH_FUNCTIONS; fun++) {
        for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
          if (Functions[fun][enc].value != Channel_memory[Lowest_channel][fun][enc]) {
            Functions[fun][enc].value = Channel_memory[Lowest_channel][fun][enc];
            Functions[fun][enc].changed = 1;
            if (fun == FUNCTION) fun_changed = 1;
          }
        }
      } // end for (fun)
    } else {
      // Harmonic functions going away (no longer in changed state).
      // If no ch functions have changed, we need to reset Function_changed...
      for (fun = 0; fun < NUM_CH_FUNCTIONS; fun++) {
        for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
          if (Functions[fun][enc].changed) {
            fun_changed = 1;
            break;
          }
        }
      } // end for (fun)
    } // end if (!skip_ch_functions)
    if (Lowest_harmonic != 0xFF) {
      for (fun = NUM_CH_FUNCTIONS; fun < NUM_FUNCTIONS; fun++) {
        for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
          if (Functions[fun][enc].value !=
              Harmonic_memory[Lowest_channel][Lowest_harmonic][fun - NUM_CH_FUNCTIONS][enc]
          ) {
            Functions[fun][enc].value = 
              Harmonic_memory[Lowest_channel][Lowest_harmonic][fun - NUM_CH_FUNCTIONS][enc];
            Functions[fun][enc].changed = 1;
            if (fun == FUNCTION) fun_changed = 1;
          }
        }
      } // end for (fun)
    } // end if (Lowest_harmonic set)
    if (!skip_ch_functions || FUNCTION >= NUM_CH_FUNCTIONS) {
      update_displays();
    }
    if (fun_changed) Function_changed = 1;
  } // end if (Lowest_channel set)
}

void load_encoders(void) {
  // Loads Encoders from Functions.
  // Called when the FUNCTION changes.
  // Does not reset Functions.changed.
  // Clears displays prior to update.
  // Does not update displays afterwards.

  clear_displays();

  for (byte enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
    Encoders[enc].var = &Functions[FUNCTION][enc];
  } // end for (enc)
}

void clear_displays(void) {
  // Clears all Encoder displays.

  for (byte enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
    variable_t *var = Encoders[enc].var;
    if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {
      if (var->var_type->flags & ENCODER_FLAGS_CHOICE_LEDS) {
        turn_off_choices_leds(enc);
      } else {
        clear_numeric_display(enc);  // display_num == encoder number
      }
    }
  } // end for (enc)
}

void update_displays(void) {
  // Updates displays to match Encoder values.

  for (byte enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
    variable_t *var = Encoders[enc].var;
    if (var && !(var->var_type->flags & ENCODER_FLAGS_DISABLED)) {
      run_event(var->var_type->display_value, enc); // display value
    }
  } // end for (enc)
}

void harmonic_on(byte sw) {
  byte hm = SWITCH_TO_HARMONIC(sw);
  Harmonic_bitmap |= 1 << hm;
  if (hm < Lowest_harmonic) {
    if (Lowest_harmonic == 0xFF) {
      Encoders[FUNCTION_ENCODER].var->var_type->max = NUM_FUNCTIONS - 1;
    }
    Lowest_harmonic = hm;
    load_functions(1);
  }
}

void truncate_function() {
  // Reduces FUNCTION_ENCODER max value.
  // Checks FUNCTION to see if it's a harmonic function.
  // If so, changes it to last non-harmonic function.
  // If FUNCTION changed, clears previous encoder displays.
  // Updates new encoder displays.
  variable_t *var = Encoders[FUNCTION_ENCODER].var;
  var->var_type->max = NUM_CH_FUNCTIONS - 1;
  if (var->value > var->var_type->max) { // harmonic function
    var->value = var->var_type->max;
    run_event(var->var_type->display_value, FUNCTION_ENCODER); // update FUNCTION display
    load_encoders();
    update_displays();
  }
}

void harmonic_off(byte sw) {
  byte hm = SWITCH_TO_HARMONIC(sw);
  Harmonic_bitmap &= ~(1 << hm);
  if (hm == Lowest_harmonic) {
    Lowest_harmonic = 0xFF;
    byte i;
    for (i = hm + 1; i < NUM_HARMONICS; i++) {
      if (Switches[HARMONIC_TO_SWITCH(i)].current) {
        Lowest_harmonic = i;
        break;
      }
    } // end for (i)
    if (Lowest_harmonic == 0xFF) {
      // No more harmonics selected...
      truncate_function();
    } else {
      load_functions(1);
    }
  } // end if (Lowest_harmonic)
}

void channel_on(byte sw) {
  byte ch = SWITCH_TO_CHANNEL(sw);
  Channel_bitmap |= 1 << ch;
  if (ch < Lowest_channel) {
    byte old_ch = Lowest_channel;
    Lowest_channel = ch;
    if (old_ch == 0xFF) {
      // turn function encoder back on
      Encoders[FUNCTION_ENCODER].var = &Function_var;
      variable_t *var = Encoders[FUNCTION_ENCODER].var;
      if (Lowest_harmonic == 0xFF) {
        var->var_type->max = NUM_CH_FUNCTIONS - 1;
        if (var->value > var->var_type->max) var->value = var->var_type->max;
      }
      run_event(var->var_type->display_value, FUNCTION_ENCODER);
      load_encoders();
    } // end if (old_ch == 0xFF)
    load_functions();
  } // end if (ch < Lowest_channel)
}

void channel_off(byte sw) {
  byte ch = SWITCH_TO_CHANNEL(sw);
  Channel_bitmap &= ~(1 << ch);
  if (ch == Lowest_channel) {
    Lowest_channel = 0xFF;
    byte i;
    for (i = ch + 1; i < NUM_CHANNELS; i++) {
      if (Switches[CHANNEL_TO_SWITCH(i)].current) {
        Lowest_channel = i;
        break;
      }
    } // end for (i)
    if (Lowest_channel == 0xFF) {
      Encoders[FUNCTION_ENCODER].var = 0;   // disable FUNCTION_ENCODER
      turn_off_choices_leds(FUNCTION_ENCODER);
      clear_displays();
      for (byte enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
        Encoders[enc].var = 0; // disable function parameter encoders
      }
    } else {
      load_functions();
    }
  } // end if (Lowest_channel)
}

void reset_function_encoders(void) {
  // Called when FUNCTION changes.
  // Won't get called if FUNCTION_ENCODER is disabled.
  load_encoders();
  update_displays();
}

byte setup_functions(byte EEPROM_offset) {
  // Returns num EEPROM bytes needed.
  EEPROM_functions_offset = EEPROM_offset;

  byte ch, hm, fun, enc;

  // Copy initial Functions values to all channels and harmonics in Channel_memory and Harmonic_memory.
  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
      for (fun = 0; fun < NUM_CH_FUNCTIONS; fun++) {
        Channel_memory[ch][fun][enc] = Functions[fun][enc].value;
      }
      for (fun = NUM_CH_FUNCTIONS; fun < NUM_HM_FUNCTIONS; fun++) {
        for (hm = 0; hm < NUM_HARMONICS; hm++) {
          Harmonic_memory[ch][hm][fun][enc] = Functions[fun][enc].value;
        }
      }
    } // end for (enc)
  } // end for (ch)

  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    Switch_closed_event[CHANNEL_TO_SWITCH(ch)] = CHANNEL_ON;
    Switch_opened_event[CHANNEL_TO_SWITCH(ch)] = CHANNEL_OFF;
    //if (Switches[CHANNEL_TO_SWITCH(ch)].current) {
    //  channel_on(CHANNEL_TO_SWITCH(ch));
    //}
  } // end for (ch)

  for (hm = 0; hm < NUM_HARMONICS; hm++) {
    Switch_closed_event[HARMONIC_TO_SWITCH(hm)] = HARMONIC_ON;
    Switch_opened_event[HARMONIC_TO_SWITCH(hm)] = HARMONIC_OFF;
    //if (Switches[HARMONIC_TO_SWITCH(hm)].current) {
    //  harmonic_on(HARMONIC_TO_SWITCH(hm));
    //}
  } // end for (hm)

  return 0; // for now...
}

// vim: sw=2
#line 1 "midi_control.ino"
// midi_control.ino

// {control, player, synth}
pot_control_t Pot_controls[] = {
  // These go to PLAYER_CABLE, except the synth control(s) (ie., Synth Vol).

  // Chord 1
  {0x18}, // vol
  {0x19}, // Note-on delay
  {0x1A}, // Note-off delay

  // Chord 2
  {0x1B}, // vol
  {0x1C}, // Note-on delay
  {0x1D}, // Note-off delay

  // Chord 3
  {0x1E}, // vol
  {0x1F}, // Note-on delay
  {0x34}, // Note-off delay

  // Chord 4
  {0x35}, // vol
  {0x36}, // Note-on delay
  {0x37}, // Note-off delay

  // Chord 5
  {0x38}, // vol
  {0x39}, // Note-on delay
  {0x3A}, // Note-off delay

  // Always triggered, no channels
  {0x14, 1},    // Tempo
  {0x07, 0, 1}, // Synth Vol

  {0x15}, // Channel Vol
  {0x16}, // Note-on delay
  {0x17}, // Note-off delay
};

#define NUM_FUN_CONTROLS    (NUM_FUNCTIONS + 1)

// {control, nrpn, encoder_slices[NUM_FUNCTION_ENCODERS], add_harmonic, synth, next_control}
fun_control_t Function_controls[NUM_FUN_CONTROLS] = {
  {0x14,   0, {{4, 0}, {}, {}, {1, 4}}, 0, 1}, // Key Signature
  {0x0007, 1, {{}, {4, 10}, {3, 7}, {7, 0}}, 0, 1}, // Tune Absolute
  {0x000A, 1, {{}, {4, 4}, {4, 0}, {}}}, // Match Tuning
  {0x0002, 1, {{}, {}, {}, {5, 0}}}, // Equal Temperament
  {0x0003, 1, {{4, 4}, {4, 0}, {}, {}}}, // Well Tempered
  {0x0005, 1, {{4, 10}, {2, 8}, {7, 0}, {1, 7}}}, // Meantone
  {0x0004, 1, {{4, 4}, {4, 0}, {}, {4, 8}}}, // Just Intonation
  {0x0006, 1, {{4, 7}, {}, {7, 0}, {}}}, // Pythagorean

  {0x0070, 1, {{}, {7, 7}, {7, 0}, {}}, 1, 0, 15}, // Harmonic Basics 1: freq
  {0x0010, 1, {{3, 11}, {4, 7}, {4, 3}, {3, 0}}, 1}, // Freq Env: Ramp
  {0x0020, 1, {{3, 11}, {4, 7}, {7, 0}, {}}, 1}, // Freq Env: Sine
  {0x0030, 1, {{3, 11}, {4, 7}, {4, 3}, {3, 0}}, 1}, // Attack
  {0x0040, 1, {{3, 8}, {}, {5, 3}, {3, 0}}, 1}, // Decay
  {0x0050, 1, {{3, 11}, {4, 3}, {3, 0}, {4, 7}}, 1}, // Sustain
  {0x0060, 1, {{3, 7}, {}, {4, 3}, {3, 0}}, 1}, // Release

  {0x36, 0, {{7, 0}, {}, {}, {}}, 1}, // Harmonic Basics 2: ampl
};

void update_ch_hm_memory(byte update_memory, byte ch, byte hm) {
  switch (update_memory) {
  case 1:
    update_channel_memory(ch);
  case 0:
    break;
  case 2:
    update_harmonic_memory(ch, hm);
    break;
  } // end switch
}

void control_change_channels(byte control, byte value, byte cable, byte update_memory=0, byte hm=0) {
  // update_memory: 0 - no update, 1 - update Channel_memory, 2 - update Harmonic_memory (also requires
  // harmonic param).
  byte ch;
  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    if (Switches[CHANNEL_TO_SWITCH(ch)].current) {
      control_change(ch, control, value, cable);
      update_ch_hm_memory(update_memory, ch, hm);
    } // end if
  } // end for
}

void control_change_harmonics(byte base_control, byte value, byte cable) {
  // Pass 2 for update_memory (to get Harmonic_memory updated).
  byte hm;
  for (hm = 0; hm < NUM_HARMONICS; hm++) {
    if (Switches[HARMONIC_TO_SWITCH(hm)].current) {
      control_change_channels(base_control + hm, value, cable, 2, hm);
    }
  }
}

void nrpn_change_channels(unsigned short param_num, unsigned short value, byte cable,
                          byte update_memory=0, byte hm=0
) {
  // update_memory: 0 - no update, 1 - update Channel_memory, 2 - update Harmonic_memory (also requires
  // harmonic param).
  byte ch;
  for (ch = 0; ch < NUM_CHANNELS; ch++) {
    if (Switches[CHANNEL_TO_SWITCH(ch)].current) {
      nrpn_change(ch, param_num, value, cable);
      update_ch_hm_memory(update_memory, ch, hm);
    }
  }
}

void nrpn_change_harmonics(unsigned short base_param_num, unsigned short value, byte cable) {
  byte hm;
  for (hm = 0; hm < NUM_HARMONICS; hm++) {
    if (Switches[HARMONIC_TO_SWITCH(hm)].current) {
      nrpn_change_channels(base_param_num + hm, value, cable, 2, hm);
    }
  }
}

void send_pot(byte pot) {
  // Caller responsible for only calling when control_change should be sent out.
  // This includes checking to make sure that at least one channel switch is on (except for player or
  // synth pots, ie. Tempo and Synth Vol).
  pot_control_t *pot_control = &Pot_controls[pot];
  if (pot_control->synth) {
    control_change(SYNTH_CHANNEL, pot_control->control, Current_pot_value[pot], SYNTH_CABLE);
  } else if (pot_control->player) {
    control_change(PLAYER_CHANNEL, pot_control->control, Current_pot_value[pot], PLAYER_CABLE);
  } else {
    control_change_channels(pot_control->control, Current_pot_value[pot], PLAYER_CABLE);
  }
  Synced_pot_value[pot] = Current_pot_value[pot];
}

void send_function(void) {
  // Caller responsible for only calling when control_change should be sent out.
  // This includes checking to make sure that at least one channel switch is on (except for synth
  // functions, ie., Key Signature and Tune Absolute).
  // And at least one harmonic switch is on for harmonic functions.
  byte fun = FUNCTION;
  do {
    fun_control_t *fun_control = &Function_controls[fun];

    // Gather up value from function encoders
    unsigned short value = 0;
    byte enc;
    for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
      value |=
        ((unsigned short)(Functions[FUNCTION][enc].value & fun_control->encoder_slices[enc].bit_mask))
        << fun_control->encoder_slices[enc].lshift;
    }

    if (fun_control->synth) {
      // No add_harmonics with synth
      if (fun_control->nrpn) {
        nrpn_change(SYNTH_CHANNEL, fun_control->control, value, SYNTH_CABLE);
        update_channel_memory(SYNTH_CHANNEL);
      } else {
        control_change(SYNTH_CHANNEL, fun_control->control, value, SYNTH_CABLE);
        update_channel_memory(SYNTH_CHANNEL);
      }
    } else if (fun_control->add_harmonic) {
      // No synth with harmonics
      if (fun_control->nrpn) {
        nrpn_change_harmonics(fun_control->control, value, SYNTH_CABLE);
      } else {
        control_change_harmonics(fun_control->control, value, SYNTH_CABLE);
      }
    } else {
      if (fun_control->nrpn) {
        nrpn_change_channels(fun_control->control, value, SYNTH_CABLE, 1);
      } else {
        control_change_channels(fun_control->control, value, SYNTH_CABLE, 1);
      }
    }
    fun = fun_control->next_control;
  } while (fun);
  Function_changed = 0;
}

byte setup_midi_control(byte EEPROM_offset) {
  byte fun, enc;
  for (fun = 0; fun < NUM_FUN_CONTROLS; fun++) {
    param_slice_t *params = Function_controls[fun].encoder_slices;
    for (enc = 0; enc < NUM_FUNCTION_ENCODERS; enc++) {
      params[enc].bit_mask = (1 << params[enc].bits) - 1;
    }
  }
  return 0;
}


// vim: sw=2
#line 1 "notes.ino"
// notes.ino

#define SW_TO_NOTE_NUM(sw)   (sw - (sw < FIRST_BUTTON ? FIRST_SWITCH : FIRST_BUTTON))

byte EEPROM_notes_offset;

// indexed by SW_TO_NOTE_NUM
byte MIDI_note[] = {57, 60, 64, 67, 70, 72};  // A3, C4, E4, G4, Bb4, C5

byte Notes_currently_on;  // Set/Reset during pulsing

void note_on_by_bt(byte sw) {
  byte note = SW_TO_NOTE_NUM(sw);
  // Switch takes precedence over button...
  if (!Switches[NOTE_SWITCH(note)].current) note_on(note);
}

void note_off_by_bt(byte sw) {
  byte note = SW_TO_NOTE_NUM(sw);
  // Switch takes precedence over button...
  if (!Switches[NOTE_SWITCH(note)].current) note_off(note);
}

void note_on_by_sw(byte sw) {
  byte note = SW_TO_NOTE_NUM(sw);
  if (CONTINUOUS_ON) {
    if (!Switches[NOTE_BUTTON(note)].current) note_on(note);
  } else {
    // Pulse mode!  Make sure pulsing is on...
    turn_on_periodic_fun(PULSE_NOTES_ON, PULSE_NOTES_PERIOD);
    turn_on_periodic_fun(PULSE_NOTES_OFF, PULSE_NOTES_PERIOD);
    if (Notes_currently_on) note_on(note);
  }
}

void note_off_by_sw(byte sw) {
  byte note = SW_TO_NOTE_NUM(sw);
  if (!Switches[NOTE_BUTTON(note)].current && (CONTINUOUS_ON || Notes_currently_on)) {
    note_off(note);
  }
  if (!CONTINUOUS_ON) check_pulse_off();
}

void check_pulse_on(void) {
  // If any note switches are on, turn on pulsing!
  //
  // Called when Continuous changed to Pulse, so any note switches that are on are currently playing.
  byte i;
  byte pulse_enabled = 0;
  for (i = 0; i < NUM_NOTES; i++) {
    if (Switches[NOTE_SWITCH(i)].current) {
      // Some note switch is on, make sure pulsing is on
      if (!pulse_enabled) {
        turn_on_periodic_fun(PULSE_NOTES_ON, PULSE_NOTES_PERIOD);
        turn_on_periodic_fun(PULSE_NOTES_OFF, PULSE_NOTES_PERIOD);
        if (Periodic_next[PULSE_NOTES_ON] - Periodic_next[PULSE_NOTES_OFF] < 35000) {
          // Next periodic is PULSE_NOTES_OFF.  Leave notes on!
          break;
        }
        pulse_enabled = 1;
      }
      note_off(i);
    } // end if (note sw on)
  } // end for (i)
}

void check_pulse_off(void) {
  // If all note switches are off now, turn off pulsing.
  byte i;
  for (i = 0; i < NUM_NOTES; i++) {
    if (Switches[NOTE_SWITCH(i)].current) {
      // Some note switch is on, so don't turn off pulsing yet
      return;
    }
  } // end for (i)
  // No note switch is on, turn off pulsing
  turn_off_periodic_fun(PULSE_NOTES_ON);
  turn_off_periodic_fun(PULSE_NOTES_OFF);
}

void note_on(byte note) {
  // Always channel 1
  usbMIDI.sendNoteOn(MIDI_note[note], 50, 1, SYNTH_CABLE);        // note, velocity, channel, cable
}

void note_off(byte note) {
  // Always channel 1
  usbMIDI.sendNoteOff(MIDI_note[note], 0, 1, SYNTH_CABLE);      // note, velocity, channel, cable
  //usbMIDI.sendNoteOn(MIDI_note[note], 0, 1, SYNTH_CABLE);
}

void notes_on(void) {
  byte i;
  if (!Notes_currently_on) {
    for (i = 0; i < NUM_NOTES; i++) {
      if (Switches[NOTE_SWITCH(i)].current) {
        note_on(i);
      }
    }
    Notes_currently_on = 1;
  }
}

void notes_off(void) {
  byte i;
  if (Notes_currently_on) {
    for (i = 0; i < NUM_NOTES; i++) {
      if (Switches[NOTE_SWITCH(i)].current) {
        note_off(i);
      }
    }
    Notes_currently_on = 0;
  }
}

void control_change(byte channel, byte control, byte value, byte cable) {
  usbMIDI.sendControlChange(control, value, channel + 1, cable);
}

unsigned short Last_nrpn[NUM_CHANNELS][2];

void nrpn_change(byte channel, unsigned short param_num, unsigned short value, byte cable) {
  if (param_num != Last_nrpn[channel][cable]) {
    usbMIDI.beginNrpn(param_num, channel + 1, cable);
    Last_nrpn[channel][cable] = param_num;
  }
  usbMIDI.sendNrpnValue(value, channel + 1, cable);
  // usbMIDI.endNrpn(channel + 1, cable);
}

void flush_midi(void) {
  usbMIDI.send_now();
}

byte setup_notes(byte EEPROM_offset) {
  EEPROM_notes_offset = EEPROM_offset;
  byte i;
  for (i = 0; i < NUM_NOTES; i++) {
    Switch_closed_event[NOTE_BUTTON(i)] = NOTE_BT_ON;
    Switch_opened_event[NOTE_BUTTON(i)] = NOTE_BT_OFF;
    Switch_closed_event[NOTE_SWITCH(i)] = NOTE_SW_ON;
    Switch_opened_event[NOTE_SWITCH(i)] = NOTE_SW_OFF;
  }
  Switch_closed_event[CONTINUOUS_PULSE_SW] = CONTINUOUS;
  Switch_opened_event[CONTINUOUS_PULSE_SW] = PULSE;
  return 0;  // for now...
}

// vim: sw=2
#line 1 "switches.ino"
// switches.ino

// High (rows) -- outputs, LOW is off, HIGH is on
#define ROW_0           34
#define ROW_1           35
#define ROW_2           36
#define ROW_3           37
#define ROW_4           38
#define ROW_5           39
#define ROW_6           21
#define ROW_7           22
#define ROW_8           23

// Low (cols) -- inputs w/ pull-downs, LOW is open, HIGH is closed
#define COL_0           31
#define COL_1           30
#define COL_2           29
#define COL_3           28
#define COL_4           27
#define COL_5           26
#define COL_6           12
#define COL_7           11
#define COL_8           10


byte Rows[] = {ROW_0, ROW_1, ROW_2, ROW_3, ROW_4, ROW_5, ROW_6, ROW_7, ROW_8};
byte Cols[] = {COL_0, COL_1, COL_2, COL_3, COL_4, COL_5, COL_6, COL_7, COL_8};

switch_t Switches[NUM_SWITCHES];

byte EEPROM_switches_offset;

unsigned short Debounce_period[2];   /* 0 = switches, 1 = encoders: uSec */

unsigned short get_debounce_period(byte debounce_index) {
  return ((unsigned short)get_EEPROM(EEPROM_switches_offset + 2 * debounce_index) << 8)
       | get_EEPROM(EEPROM_switches_offset + 2 * debounce_index + 1);
}

void set_debounce_period(byte debounce_index, unsigned short dp) {
  set_EEPROM(EEPROM_switches_offset + 2 * debounce_index, dp >> 8);
  set_EEPROM(EEPROM_switches_offset + 2 * debounce_index + 1, dp & 0xFF);
  Debounce_period[debounce_index] = dp;
}

byte setup_switches(byte EEPROM_offset) {
  EEPROM_switches_offset = EEPROM_offset;

  byte row, col;
  for (row = 0; row < NUM_ROWS; row++) {
    pinMode(Rows[row], OUTPUT);
    digitalWrite(Rows[row], LOW);   // off
  }
  for (col = 0; col < NUM_COLS; col++) {
    pinMode(Cols[col], INPUT_PULLDOWN);
  }

  unsigned short us;
  us = get_debounce_period(0);    // Switch debounce
  if (us == 0xFFFF) {
    if (Serial) {
      Serial.println(F("switch debounce period not set in EEPROM"));
    }
    Debounce_period[0] = 20000;
  } else {
    Debounce_period[0] = us;
  }

  us = get_debounce_period(1);    // encoder debounce
  if (us == 0xFFFF) {
    if (Serial) {
      Serial.println(F("encoder debounce period not set in EEPROM"));
    }
    Debounce_period[1] = 1000;
  } else {
    Debounce_period[1] = us;
  }

  return 2;
}

unsigned long Longest_scan = 0;                                   // uSec, measured at 13
unsigned long Longest_open_bounce[2] = {0, 0};                    // uSec, measured at 35700, 25800 (?? maybe wrong)
unsigned long Shortest_close_bounce[2] = {1000000ul, 1000000ul};  // uSec, measured at 9, 9
unsigned long Longest_wire_clear = 0;                             // uSec, measured at 5

#define MAX_DEBOUNCE_COUNT    30
byte Close_counts[NUM_SWITCHES];
byte Debounce_delay_counts[2][MAX_DEBOUNCE_COUNT + 1];   // debounce_index, mSec

void scan_switches(byte trace, byte trace_col) {
  // Takes 320 uSec to run with no activity on Teensy LC (ARM Cortex-M0+ at 48 MHz).
  // Takes 7 uSec to run with no activity on Teensy 4.1 (ARM Cortex-M7 at 600 MHz).
  if (trace) {
    Serial.println("Starting scan_switches");
  }
  byte row, col;
  unsigned long start_scan_time = micros();
  for (row = 0; row < 9; row++) {
    digitalWrite(Rows[row], HIGH);     // turn on row
    byte cols_closed[NUM_COLS];
    byte cols_closed_index = 0;
    for (col = 0; col < 9; col++) {
      byte sw_num = SWITCH_NUM(row, col);
      switch_t *sw = &Switches[sw_num];
      if (digitalRead(Cols[col])) {
        // switch closed
        if (trace && col == trace_col) {
          Serial.print("Switch "); Serial.print(row); Serial.print(", "); Serial.print(col); Serial.print(" closed, last_seen ");
          Serial.print(sw->last_seen); Serial.print(", current "); Serial.println(sw->current);
        }
        cols_closed[cols_closed_index++] = col;
        if (sw->last_seen == 0) {  // last seen open!
          sw->last_seen = 1;
          sw->close_time = start_scan_time;
          sw->close_time_set = 1;

          if (sw->open_time_set) {
            // Update Debounce_delay_counts
            unsigned long time_open = start_scan_time - sw->open_time;
            byte mSec = time_open / 1000ul;
            if (mSec < 100) {
              if (mSec > MAX_DEBOUNCE_COUNT) mSec = MAX_DEBOUNCE_COUNT;
              if (Debounce_delay_counts[sw->debounce_index][mSec] < 255) {
                Debounce_delay_counts[sw->debounce_index][mSec] += 1;
              }
            }
            if (time_open < 100000ul && time_open > Longest_open_bounce[sw->debounce_index]) {
              Longest_open_bounce[sw->debounce_index] = time_open;
            }
          } // end if (sw->open_time_set)
        } // end if (sw->last_seen == 0)

        if (!sw->current) {
          // currently open, close is immediate!
          sw->current = 1;      // set to currently closed
          switch_closed(sw_num);
          Close_counts[sw_num]++;
          if (trace && col == trace_col) {
            Serial.print(float(micros()) / 1000.0);
            Serial.print(F(": Switch at row "));
            Serial.print(row);
            Serial.print(F(", col "));
            Serial.print(col);
            Serial.println(F(" has closed"));
          } // end if (trace)
        } // end if (!sw->current)
      } else {
        // switch open
        if (trace && col == trace_col) {
          Serial.print("Switch "); Serial.print(row); Serial.print(", "); Serial.print(col); Serial.print(" open, last_seen ");
          Serial.print(sw->last_seen); Serial.print(", current "); Serial.println(sw->current);
        }
        if (sw->last_seen == 1) {  // last seen closed!  Start timer...
          sw->last_seen = 0;
          sw->open_time = start_scan_time;
          sw->open_time_set = 1;

          if (sw->close_time_set) {
            unsigned long time_closed = start_scan_time - sw->close_time;
            if (time_closed < Shortest_close_bounce[sw->debounce_index]) {
              Shortest_close_bounce[sw->debounce_index] = time_closed;
            }
          }
        } // end if (sw->last_seen == 1)

        if (sw->current) {
          // currently closed
          if (start_scan_time - sw->open_time > Debounce_period[sw->debounce_index]) {
            sw->current = 0;    // set to currently open
            switch_opened(sw_num);
            if (trace && col == trace_col) {
              Serial.print(float(micros()) / 1000.0);
              Serial.print(F(": Switch at row "));
              Serial.print(row);
              Serial.print(F(", col "));
              Serial.print(col);
              Serial.println(F(" is now open"));
            } // end if (trace)
          } // end if (timer expired)
        } // end if (sw->current)
      } // end else if (digitalRead)
    } // end for (col)
    digitalWrite(Rows[row], LOW);    // turn off row

    delayMicroseconds(10);           // give the col wires a little time to drop LOW again

    cols_closed[0] += 1;    // Stop the compiler from complaining because the following code is commented out...
    /********************************
    // See how long it takes for all HIGH cols to go LOW after Row is turned off.
    unsigned long row_off_time = micros();
    if (cols_closed_index) {
      unsigned long i;
      for (i = 0; i < 1000000ul; i++) {
        byte done = 1;
        byte col = cols_closed_index;
        while (col--) {
          if (digitalRead(Cols[cols_closed[col]])) {
            // col still HIGH...
            done = 0;
            break;
          }
        } // end while (col)
        if (done) break;
      } // end for (i)
      unsigned long wait_time = micros() - row_off_time;
      if (wait_time > Longest_wire_clear) Longest_wire_clear = wait_time;
    } // end if (cols_closed_index)
    ********************************/

  } // end for (row)
  if (trace) {
    Serial.println("scan_switches done");
    Serial.println();
    delay(500);
  }
  unsigned long end_scan_time = micros();
  end_scan_time -= start_scan_time;
  if (end_scan_time > Longest_scan) Longest_scan = end_scan_time;
}

// vim: sw=2
#line 1 "triggers.ino"
// triggers.ino

byte EEPROM_triggers_offset;

// {switch_, button, led, check_event}
trigger_t Triggers[NUM_TRIGGERS] = {  // pot triggers share same array index as Num_pots/Pots arrays.
  { 0,  9, 167, CHECK_POTS},      // 1st Note
  { 1, 10, 166, CHECK_POTS},      // 2nd Note
  { 2, 11, 165, CHECK_POTS},      // 3rd Note
  { 3, 12, 164, CHECK_POTS},      // 4th Note
  { 4, 13, 163, CHECK_POTS},      // 5th Note
  {34, 47, 171, CHECK_POTS},      // vol note_on/off
  {33, 46, 170, CHECK_FUNCTIONS}, // functions
};

byte Num_pots[NUM_POT_TRIGGERS] = {3, 3, 3, 3, 3, 3};
byte Pots[NUM_POT_TRIGGERS][MAX_TRIGGER_POTS] = {
  { 1,  2,  3},  // 1st Note
  { 4,  5,  6},  // 2nd Note
  { 7,  8,  9},  // 3rd Note
  {10, 11, 12},  // 4th Note
  {13, 14, 15},  // 5th Note
  {18, 19, 20},  // vol note_on/off
};

// 0xFF means no trigger == always continuous!
byte Pot_trigger[NUM_POTS];   // initialized from Triggers/Pots in setup_triggers

// Called when Channel or Harmonic switches change.
void disable_triggers(void) {
  byte i;
  for (i = 0; i < NUM_TRIGGERS; i++) {
    if (Triggers[i].continuous) {
      Triggers[i].continuous = 0;
      led_off(Triggers[i].led);
    }
  }
}

// Called when one of the monitored pot values for the trigger changes.
void pot_changed(byte pot) {
  if (Pot_trigger[pot] == 0xFF || Triggers[Pot_trigger[pot]].continuous) {
    send_pot(pot);
  }
}

// Called when trigger button is pressed.  Value may, or may not have, changed.
void check_trigger(byte trigger) {
  run_event(Triggers[trigger].check_event, trigger);
}

byte setup_triggers(byte EEPROM_offset) {
  EEPROM_triggers_offset = EEPROM_offset;
  byte tr, pot;
  for (tr = 0; tr < NUM_TRIGGERS; tr++) {
    Switches[Triggers[tr].switch_].tag = tr;
    Switch_closed_event[Triggers[tr].switch_] = TRIGGER_SW_ON;
    Switch_opened_event[Triggers[tr].switch_] = TRIGGER_SW_OFF;

    Switches[Triggers[tr].button].tag = tr;
    Switch_closed_event[Triggers[tr].button] = TRIGGER_BT_PRESSED;
    Switch_opened_event[Triggers[tr].button] = TRIGGER_BT_RELEASED;

    for (pot = 0; pot < NUM_POTS; pot++) {
      Pot_trigger[pot] = 0xFF;
    }
    if (Triggers[tr].check_event == CHECK_POTS) {
      for (pot = 0; pot < Num_pots[tr]; pot++) {
        Pot_trigger[pot] = tr;
      }
    }
  }
  return 0;
}


// vim: sw=2
#line 1 "main.cpp"
// main.cpp

#include <stdio.h>

int main() {
    printf("Hello World!\n");
    return 0;
}
