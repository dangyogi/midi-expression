// switches.ino

// High (rows) -- outputs, LOW is on, HIGH is off
#define ROW_0           11  /* PA16 */
#define ROW_1            8  /* PA18 */
#define ROW_2           12  /* PA19 */
#define ROW_3            9  /* PA20 */
#define ROW_4           10  /* PA21 */
#define ROW_5           A1  /* PB02 */
#define ROW_6           A7  /* PB03 */
#define ROW_7            2  /* PB10 */
#define ROW_8            3  /* PB11 */

// Low (cols) -- inputs w/ pull-downs, LOW is open, HIGH is closed
#define COL_0           A0  /* PA02 */
#define COL_1            0  /* PB23 RX */
#define COL_2            6  /* PA04 */
#define COL_3            5  /* PA05 */
#define COL_4            7  /* PA06 */
#define COL_5            4  /* PA07 */
#define COL_6           A6  /* PA09 */
#define COL_7           A3  /* PA10 */
#define COL_8           A2  /* PA11 */


byte Rows[] = {ROW_0, ROW_1, ROW_2, ROW_3, ROW_4, ROW_5, ROW_6, ROW_7, ROW_8};
byte Cols[] = {COL_0, COL_1, COL_2, COL_3, COL_4, COL_5, COL_6, COL_7, COL_8};

switch_t Switches[NUM_SWITCHES];

byte EEPROM_switches_offset;

unsigned short Debounce_period = 5000;   /* uSec */

byte setup_switches(byte EEPROM_offset) {
  EEPROM_switches_offset = EEPROM_offset;

  byte i;
  for (i = 0; i < 9; i++) {
    pinMode(Rows[i], OUTPUT);
    digitalWrite(Rows[i], LOW);   // off
    pinMode(Cols[i], INPUT_PULLDOWN);
  }

  unsigned short us = get_EEPROM(EEPROM_switches_offset, 2);
  if (us == 0xFFFF) {
    if (Serial) {
      Serial.println("debounce period not set in EEPROM");
    }
  }

  return 2;
}

unsigned long Longest_scan;  // uSec

byte Close_counts[NUM_SWITCHES];

void scan_switches(byte trace) {
  // Takes 210 uSec to run with no activity.
  byte row, col;
  unsigned short cols;
  unsigned long start_scan_time = micros();
  for (row = 0; row < 9; row++) {
    digitalWrite(Rows[row], HIGH);     // turn on row
    for (col = 0; col < 9; col++) {
      byte sw_num = SWITCH_NUM(row, col);
      switch_t *sw = &Switches[sw_num];
      if (digitalRead(Cols[col])) {
        // switch closed
        if (!sw->current) {
          // currently open, close is immediate!
          sw->current = 1;
          sw->opening = 0;
          switch_closed(sw_num);
          Close_counts[sw_num]++;
          if (trace) {
            Serial.print("Switch at row ");
            Serial.print(row);
            Serial.print(", col ");
            Serial.print(col);
            Serial.println(" has closed");
          } // end if (trace)
        } // end if (!sw->current)
      } else {
        // switch open
        if (sw->current) {
          // currently closed
          if (!sw->opening) {
            sw->opening = 1;
            sw->open_time = start_scan_time;
          } else if (sw->open_time < sw->open_time + Debounce_period) {
            if (start_scan_time > sw->open_time + Debounce_period ||
                start_scan_time < sw->open_time
            ) {
              sw->current = 0;
              switch_opened(sw_num);
              if (trace) {
                Serial.print("Switch at row ");
                Serial.print(row);
                Serial.print(", col ");
                Serial.print(col);
                Serial.println(" is now open");
              } // end if (trace)
            } // end if (timer expired)
          } else if (start_scan_time > sw->open_time + Debounce_period &&
                  start_scan_time < sw->open_time
          ) {
            sw->current = 0;
            switch_opened(sw_num);
            if (trace) {
              Serial.print("Switch at row ");
              Serial.print(row);
              Serial.print(", col ");
              Serial.print(col);
              Serial.println(" is now open");
            } // end if (trace)
          }
        } // end if (sw->current)
      } // end else if (digitalRead)
    } // end for (col)
    digitalWrite(Rows[row], LOW);    // turn off row
  } // end for (row)
  unsigned long end_scan_time = micros();
  if (end_scan_time > start_scan_time) {
    end_scan_time -= start_scan_time;
    if (end_scan_time > Longest_scan) Longest_scan = end_scan_time;
  }
}

// vim: sw=2
