
static const uint8_t RESET = 13;  // Built-in LED
static const uint8_t RS = 5;
static const uint8_t RW = 11;
static const uint8_t ENABLE = 12;
static const uint8_t DB0 = A3;
static const uint8_t RB0 = 6;
static const uint8_t DB1 = A2;
static const uint8_t RB1 = 7;
static const uint8_t DB2 = A1;
static const uint8_t RB2 = 8;
static const uint8_t DB3 = A0;
static const uint8_t RB3 = 9;
static const uint8_t DB4 = A6;
static const uint8_t RB4 = 3;
static const uint8_t DB5 = A7;
static const uint8_t RB5 = 2;
static const uint8_t DB6 = 4;
static const uint8_t RB6 = 0;   // RX
static const uint8_t DB7 = 1;   // TX, old 23 (AREF)
static const uint8_t RB7 = 10;

static const uint8_t DB_REGS[] = {DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7};
static const uint8_t R_REGS[] = {RB0, RB1, RB2, RB3, RB4, RB5, RB6, RB7};
static const uint8_t DD_RAM_ADDR[] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,   //  0 - 11
                                      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};  // 12 - 24

uint8_t DB_state = 0;  // 0 means DB_REGs are in unknown state, 1 means INPUT, 2 means OUTPUT

void db_in(void) {
  // Sets all DBx pins to INPUT (with no pull-ups)
  uint8_t i;
  if (DB_state != 1) {
    for (i = 0; i < 8; i++) {
      digitalWrite(DB_REGS[i], 0);
      pinMode(DB_REGS[i], INPUT);
    }
    DB_state = 1;
  } // end if
}

void db_out(void) {
  // Sets all DBx pins to OUTPUT
  uint8_t i;
  if (DB_state != 2) {
    for (i = 0; i < 8; i++) {
      pinMode(DB_REGS[i], OUTPUT);
    }
    DB_state = 2;
  }
}

uint8_t R_state = 0;   // 0 means R_REGS are in unknown state, 1 means OFF, 2 means HIGH, 3 means LOW

void r_off(void) {
  // Sets all RBx pins to INPUT (with no pull-ups)
  uint8_t i;
  if (R_state != 1) {
    for (i = 0; i < 8; i++) {
      //digitalWrite(R_REGS[i], 0);
      pinMode(R_REGS[i], INPUT);
    }
    R_state = 1;
  }
}

void r_high_low(uint8_t high_low) {
  // Sets all RBx pins HIGH
  uint8_t i;
  if (high_low) {
    if (R_state != 2) {
      for (i = 0; i < 8; i++) {
        pinMode(R_REGS[i], OUTPUT);
        digitalWrite(R_REGS[i], HIGH);
      }
      R_state = 2;
    }
  } else {  // !high_low
    if (R_state != 3) {
      for (i = 0; i < 8; i++) {
        pinMode(R_REGS[i], OUTPUT);
        digitalWrite(R_REGS[i], LOW);
      }
      R_state = 3;
    }
  } // end if (high_low)
}

extern void display_on(void);

void reset(void) {
  // RESET the LCD
  Serial.println("reset called");
  r_off();
  digitalWrite(RESET, LOW);
  //delay(15);
  delay(100);
  digitalWrite(RESET, HIGH);
  delay(100);
  display_on();
}

uint8_t read(uint8_t rs) {
  r_off();
  db_in();
  digitalWrite(RS, (rs ? 1 : 0));
  digitalWrite(RW, 1);     // read
  digitalWrite(ENABLE, 1);
  //unsigned long start_time = micros();
  delayMicroseconds(2);
  uint8_t ans = 0;
  ans |= digitalRead(DB7) << 7;
  ans |= digitalRead(DB6) << 6;
  ans |= digitalRead(DB5) << 5;
  ans |= digitalRead(DB4) << 4;
  ans |= digitalRead(DB3) << 3;
  ans |= digitalRead(DB2) << 2;
  ans |= digitalRead(DB1) << 1;
  ans |= digitalRead(DB0);
  //unsigned long micros_taken = micros() - start_time;
  digitalWrite(ENABLE, 0);
  delayMicroseconds(2);
  //Serial.print("read took ");
  //Serial.print(micros_taken);
  //Serial.println(" micros");
  return ans;
}

uint8_t busy(void) {
  // Is the LCD busy? (i.e., not ready to receive another command)
  return read(0) & 0x80;
}

void wait(char *why) {
  // Wait until the LCD is not busy (i.e., ready to receive another command).
  //unsigned long start_time = micros();
  int i = 0;
  while (busy()) i++;
  //unsigned long micros_taken = micros() - start_time;
  if (i) {
    Serial.print("wait ");
    Serial.print(why);
    Serial.print(" got ");
    Serial.print(i);
    Serial.println(" busys");
  }
}

void write(uint8_t rs, uint8_t data) {
  // IMPORTANT: caller does wait!
  r_off();
  digitalWrite(RS, (rs ? 1 : 0));
  digitalWrite(RW, 0);     // write
  digitalWrite(ENABLE, 1);
  unsigned long start_time = micros();
  //delayMicroseconds(2);
  db_out();
  uint8_t i;
  for (i = 0; i < 8; i++) {
    digitalWrite(DB_REGS[i], ((data & (1 << i)) ? HIGH : LOW));
  }
  delayMicroseconds(2);
  digitalWrite(ENABLE, 0);
  delayMicroseconds(2);
  db_in();
}

void display_control(void) {
  Serial.println("display_control called");
  wait("display_control");
  write(0, 0x28);
}

void cursor_control(void) {
  Serial.println("cursor_control called");
  wait("cursor_control");
  write(0, 0x08);
}

void display_on(void) {
  Serial.println("display_on called");
  wait("display_on");
  write(0, 0x14);
}

void start_oscillator(void) {
  //Serial.println("start_oscillator called");
  wait("start_oscillator");
  write(0, 0x03);
  delay(15);
}

void power_on(void) {
  //Serial.println("power_on called");
  wait("power_on");
  write(0, 0x1C);
}

void entry_mode(void) {
  //Serial.println("entry_mode called");
  wait("entry_mode");
  write(0, 0x06);
}

void contrast(void) {
  uint8_t level = Serial.parseInt(SKIP_WHITESPACE) & 0x0F;
  Serial.print("contrast called, level ");
  Serial.println(level);
  wait("contrast");
  write(0, 0x80 | level);
}

void clear(void) {
  wait("clear");
  write(0, 0x01);
}

void setup() {
  // put your setup code here, to run once:
  r_off();
  db_in();
  
  pinMode(ENABLE, OUTPUT);
  digitalWrite(ENABLE, LOW);
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);
  pinMode(RS, OUTPUT);
  digitalWrite(RS, LOW);
  pinMode(RW, OUTPUT);
  digitalWrite(RW, HIGH);


  Serial.begin(9600);
  Serial.setTimeout(5000);    // mSec
  Serial.print("sizeof(int) is ");
  Serial.println(sizeof(int));
  Serial.print("int(3.99) is ");
  Serial.println(int(3.99));
  //Serial.print("LOW is ");
  //Serial.println(LOW);
  //Serial.print("HIGH is ");
  //Serial.println(HIGH);
  
  Serial.print("A0: "); Serial.println(A0);
  //Serial.print("A1: "); Serial.println(A1);
  //Serial.print("A2: "); Serial.println(A2);
  //Serial.print("A3: "); Serial.println(A3);
  //Serial.print("A4: "); Serial.println(A4);
  //Serial.print("A5: "); Serial.println(A5);
  //Serial.print("A6: "); Serial.println(A6);
  Serial.print("A7: "); Serial.println(A7);

  delay(100);   // mSec, let power stabilize before RESET
  reset();
  //delay(100);
  Serial.println("setup done");
}

void help() {
  Serial.println();
  Serial.println("H -- Help");
  Serial.println("X -- RESET");
  Serial.println("O -- Start Oscillator");
  Serial.println("P -- Power On");
  Serial.println("N -- Display Control (Num Lines)");
  Serial.println("F -- Cursor OFf");
  Serial.println("D -- Display On");
  Serial.println("E -- Entry Mode");
  Serial.println("Clevel -- Set Contrast to level");
  Serial.println("B -- Clear (blank)");
  Serial.println("Saddr -- Set Addr");
  Serial.println("A -- read Addr");
  Serial.println("Wstr -- Write str");
  Serial.println("Rlen -- Read len chars from DDRAM");
  Serial.println("T -- Test DB pin float");
  Serial.println("1 -- Set all RB pins HIGH");
  Serial.println("0 -- Set all RB pins LOW");
  Serial.println("~ -- analog read");
  Serial.println();
}

void set_char_addr(uint8_t char_addr) {
  // char_addr starts at 0.
  wait("set_char_addr 1");
  write(0, 0xC0 | (DD_RAM_ADDR[char_addr] >> 5));
  wait("set_char_addr 2");
  write(0, 0xE0 | (DD_RAM_ADDR[char_addr] & 0x1F));
}

void write_char(uint8_t c) {
  wait("write_char");
  Serial.print("write_char: writing '");
  Serial.print((char)c);
  Serial.println("'");
  write(1, c);
}

void write_str(char *s) {
  uint8_t i;
  for (i = 0; s[i]; i++) {
    write_char(s[i]);
  }
}

void set_addr(void) {
  uint8_t char_addr = Serial.parseInt(SKIP_WHITESPACE);
  Serial.print("Setting char addr to ");
  Serial.println(char_addr);
  set_char_addr(char_addr);
}

void read_status(void) {
  uint8_t status = read(0);
  Serial.print("read status => 0x");
  Serial.println(status, HEX);
}

void display(void) {
  String str = Serial.readStringUntil('\n');
  Serial.print("Writing \"");
  Serial.print(str.c_str());
  Serial.println("\"");
  write_str(str.c_str());
}

void read_DDRAM(void) {
  uint8_t len = Serial.parseInt(SKIP_WHITESPACE);
  uint8_t i;
  for (i = 0; i < len; i++) {
    wait("read_DDRAM");
    char c = read(1);
    Serial.print(c);
  }
  Serial.println();
}

void check_db_pins(uint8_t rs, uint8_t rw, uint8_t high_low) {
  r_high_low(high_low);
  unsigned long stop = millis() + 100;
  int iterations = 0;
  int err_count_by_DB[] = {0,0,0,0,0,0,0,0};
  uint8_t i;
  
  while (millis() < stop) {
    for (i = 0; i < 8; i++) {
      if (digitalRead(DB_REGS[i]) != high_low) {
        err_count_by_DB[i]++;
      }
    } // end for
    iterations++;
  } // end while
  Serial.print("check_db_prints: RS=");
  Serial.print(rs);
  Serial.print(", RW=");
  Serial.print(rw);
  Serial.print(", expected=");
  Serial.print(high_low);
  
  Serial.print(", did ");
  Serial.print(iterations);
  Serial.println(" iterations in 100 mSec");
  for (i = 0; i < 8; i++) {
    if (err_count_by_DB[i]) {
      Serial.print("DB pin ");
      Serial.print(i);
      Serial.print(" had ");
      Serial.print(err_count_by_DB[i]);
      Serial.println(" errors");
    }
  }
}

void test() {
  uint8_t rs;
  uint8_t rw;
  uint8_t high_low;
  digitalWrite(ENABLE, LOW);
  digitalWrite(RESET, HIGH);
  db_in();
  for (rs = 0; rs < 2; rs++) {
    digitalWrite(RS, rs);
    for (rw = 0; rw < 2; rw++) {
      digitalWrite(RW, rw);
      for (high_low = 0; high_low < 2; high_low++) {
        check_db_pins(rs, rw, high_low);
      }
    }
  }
}

int a_low = 10000;
int a_high = -10000;
unsigned int num_reads = 0;
unsigned int read_time;
unsigned int probe_time;
int threshold = 10;
int last_reported = 0;

void analog_read(void) {
  int a = analogRead(A4);
  Serial.print("analog_read got ");
  Serial.println(a);
  Serial.print("a_low ");
  Serial.print(a_low);
  Serial.print(", a_high ");
  Serial.println(a_high);
  Serial.print("probe_time ");
  Serial.print(probe_time);
  Serial.print(", read_time ");
  Serial.print(read_time);
  Serial.print(", num_reads ");
  Serial.println(num_reads);
  a_low = a;
  a_high = a;
  num_reads = 0;
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long probe_start = micros();
  unsigned long start_time = micros();
  int a = analogRead(A4);
  read_time = micros() - start_time;
  probe_time = start_time - probe_start;
  
  if (a < a_low) a_low = a;
  if (a > a_high) a_high = a;
  num_reads += 1;
  if (num_reads >= threshold) {
    if (last_reported < a_low || last_reported > a_high) {
      int new_value = (a_high + a_low) / 2;
      if (abs(last_reported - new_value) > 8) {
        // report new value
        last_reported = new_value;
        Serial.print("Pot ");
        Serial.println(last_reported);
      }
    }
    a_low = a;
    a_high = a;
    num_reads = 1;
  }

  // Commands: H (help), R (reset), C (clear), Daddr, str (Display), T (test)
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
    case 'H':
    case 'h': help();
              break;
    case 'X': ;
    case 'x': reset();
              Serial.println("RESET done.");
              break;
    case 'O': ;
    case 'o': start_oscillator();
              Serial.println("Start Oscillator done.");
              break;
    case 'P': ;
    case 'p': power_on();
              Serial.println("Power On done.");
              break;
    case 'N': ;
    case 'n': display_control();
              Serial.println("Display Control done.");
              break;
    case 'F': ;
    case 'f': cursor_control();
              Serial.println("Cursor Control done.");
              break;
    case 'D': ;
    case 'd': display_on();
              Serial.println("Display On done.");
              break;
    case 'C': ;
    case 'c': contrast();
              Serial.println("Set Contrast done.");
              break;
    case 'E': ;
    case 'e': entry_mode();
              Serial.println("Entry Mode done.");
              break;
    case 'B': ;
    case 'b': clear();
              Serial.println("Clear done.");
              break;
    case 'S': ;
    case 's': set_addr();
              Serial.println("Set addr done.");
              break;
    case 'A': ;
    case 'a': read_status();
              Serial.println("Read status done.");
              break;
    case 'W': ;
    case 'w': display();
              Serial.println("Write done.");
              break;
    case 'R': ;
    case 'r': read_DDRAM();
              Serial.println("Read DDRAM done.");
              break;
    case 'T': ;
    case 't': test();
              Serial.println("Test done.");
              break;
    case '1': db_in();
              pinMode(13, INPUT);
              r_high_low(HIGH);
              Serial.println("RBs set HIGH.");
              break;
    case '0': db_in();
              pinMode(13, INPUT);
              r_high_low(LOW);
              Serial.println("RBs set LOW.");
              break;
    case '~': analog_read();
              break;
    case ' ':
    case '\t':
    case '\r':
    case '\n': break;  // ignore whitespace...
    default: help();
             break;
    } // end switch
  } // end if
}
