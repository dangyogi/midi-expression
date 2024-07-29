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
