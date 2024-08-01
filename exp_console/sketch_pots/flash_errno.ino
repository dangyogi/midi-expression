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

void report_errno(void) {
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
