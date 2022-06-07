// flash_errno.ino

#define MAX_DELAYS   50

int Delays[MAX_DELAYS];
byte Num_delays;

byte Current_delay = 0;
byte Level;
unsigned long Start_time;

byte Last_errno = 0;

byte LED_pin = 13;

void err_led(byte led_pin) {
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  LED_pin = led_pin;
}

void push(int delay) {
  if (Num_delays < MAX_DELAYS) {
    Delays[Num_delays++] = delay;
    Serial.println(delay);
  }
}

void errno(byte current_errno) {
  if (current_errno == 0) {
    Last_errno = 0;
    Num_delays = 0;
    digitalWrite(LED_pin, LOW);
  } else {
    if (Last_errno == 0) {
      // Display this errno!
      Last_errno = current_errno;
      byte i;
      byte divisor = 100;
      byte seen_first = 0;
      Num_delays = 0;  // just to be sure...
      for (divisor = 100; divisor; divisor /= 10) {
        // one time through per digit, left to right
        byte remainder = current_errno % divisor;
        byte digit = (current_errno - remainder) / divisor;
        current_errno = remainder;
        if (digit == 0) {
          if (seen_first || divisor == 1) {
            push(750);  // LONG on
            push(400);  // off between digits
          }
        } else {
          for (i = 0; i < digit - 1; i++) {
            push(150);  // on
            push(150);   // off
          }
          push(150);    // last on
          push(600);      // off between digits
          seen_first = 1;
        }
      } // end for (divisor)

      // Change final delay to seperate repetitions
      Delays[Num_delays - 1] = 1200;

      Start_time = millis();
      Current_delay = 0;
      digitalWrite(LED_pin, HIGH);
      Level = 0;
    } // end if (Last_errno == 0)
  } // end else if (current_errno == 0)
  if (Current_delay < Num_delays) {
    unsigned long current_time = millis();
    if (current_time - Start_time >= Delays[Current_delay]) {
      digitalWrite(LED_pin, Level);
      Level = 1 - Level;
      Start_time = current_time;
      if (++Current_delay >= Num_delays) Current_delay = 0;  // wrap around
    }
  } // end if (Current_delay < Num_delays)
}

