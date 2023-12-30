// alpha_displays.ino

// includes from sketch_led.ino show up here with Arduino IDE!

// Defines "const col_ports_t Alpha_decoder[]" indexed by char (< 128)
#include "Alpha_decoder.h"

byte EEPROM_alpha_offset;

byte EEPROM_Num_alpha_strings(void) {
  return EEPROM_alpha_offset;
}

byte EEPROM_Alpha_num_chars(byte string_num) {
  return 1 + string_num + EEPROM_alpha_offset;
}

byte EEPROM_Alpha_index(byte string_num) {
  return 1 + MAX_NUM_STRINGS + string_num + EEPROM_alpha_offset;
}

byte Num_alpha_strings;
byte Alpha_num_chars[MAX_NUM_STRINGS]; // num chars display units in alpha string
byte Alpha_index[MAX_NUM_STRINGS];     // word index for left-most char display

byte setup_alpha_displays(byte my_EEPROM_offset) {
  // put your setup code here, to run once:
  EEPROM_alpha_offset = my_EEPROM_offset;

  byte b = EEPROM[EEPROM_Num_alpha_strings()];
  if (b == 0xFF) {
    Serial.println("Num_alpha_strings not set in EEPROM");
  } else if (b > MAX_NUM_STRINGS) {
    Errno = 81;
    Err_data = b;
  } else Num_alpha_strings = b;

  byte i;
  for (i = 0; i < Num_alpha_strings; i++) {
    b = EEPROM[EEPROM_Alpha_num_chars(i)];
    if (b == 0xFF) {
      Serial.print("Alpha_num_chars not set in EEPROM for ");
      Serial.println(i);
    } else if (b > MAX_STRING_LEN) {
      Errno = 82;
      Err_data = b;
    } else Alpha_num_chars[i] = b;

    b = EEPROM[EEPROM_Alpha_index(i)];
    if (b == 0xFF) {
      Serial.print("Alpha_index not set in EEPROM for ");
      Serial.println(i);
    } else if (b > Num_rows * NUM_COLS) {
      Errno = 83;
      Err_data = b;
    } else Alpha_index[i] = b;
  } // end for (i)

  Timeout_fun_runtime[ADVANCE_STRINGS] = 1;  // activate advance_strings

  return 1 + 2*MAX_NUM_STRINGS;
} // end setup_alpha_displays()

col_ports_t Alpha_string[MAX_NUM_STRINGS][MAX_STRING_LEN + END_GAP];
byte String_len[MAX_NUM_STRINGS];
byte Scrolling_index[MAX_NUM_STRINGS];   // current starting scrolling index

void display_string(byte string_num) {
  byte j;
  for (j = 0; j < Alpha_num_chars[string_num]; j++) {
    Col_ports[Alpha_index[string_num] + j] =
      Alpha_string[string_num][(Scrolling_index[string_num] + j) % String_len[string_num]];
  } // end for (j)
}

unsigned short advance_strings(void) {
  // Called by timeout().
  byte i;
  for (i = 0; i < MAX_NUM_STRINGS; i++) {
    if (String_len[i] > Alpha_num_chars[i]) {
      // We're scrolling this string!  Time to advance 1 char...
      Scrolling_index[i] += 1;
      Scrolling_index[i] %= String_len[i];
      if (Trace) {
        Serial.print("advance_string "); Serial.print(i);
        Serial.print(": String_len "); Serial.print(String_len[i]);
        Serial.print(", Alpha_num_chars "); Serial.print(Alpha_num_chars[i]);
        Serial.print(", New Scrolling_index "); Serial.println(Scrolling_index[i]);
      }
      display_string(i);
    } // end if (scrolling this string?)
  } // end for (i)
  return SCROLL_DELAY;
}

void load_string(byte string_num, char *s) {
  // Pass NULL or "" to delete the string.
  byte as_index = 0;  // where next char goes in Alpha_string
  if (string_num >= MAX_NUM_STRINGS) {
    Errno = 84;
    Err_data = string_num;
  } else if (s == NULL || *s == 0) {
    // Turn off this string and set it to blanks.
    for (as_index = 0; as_index < Alpha_num_chars[string_num]; as_index++) {
      Alpha_string[string_num][as_index] = Alpha_decoder[' '];
    }
    String_len[string_num] = as_index;
    Scrolling_index[string_num] = 0;
    display_string(string_num);
  } else if (strlen(s) > MAX_STRING_LEN) {
    Errno = 85;
    Err_data = strlen(s);
  } else {
    while (*s) {
      if (*s == '.') {
        if (Trace) {
          Serial.println("load_string: got '.', tack onto last char");
        }
        if (as_index == 0) {
          Alpha_string[string_num][as_index++] = Alpha_decoder[' '];
          if (Trace) {
            Serial.println("load_string: adding ' ' for '.'");
          }
        }
        col_on(&Alpha_string[string_num][as_index - 1], 14);  // DP is col 14
        s++;
      } else {
        if (Trace) {
          Serial.print("load_string: adding "); Serial.println(*s);
        }
        Alpha_string[string_num][as_index++] = Alpha_decoder[*s++];
      } // end if ('.')
    } // end while (*s)
    byte end_gap_len;
    if (as_index > Alpha_num_chars[string_num]) {
      // gap between two copies of string during scrolling
      end_gap_len = END_GAP;
    } else {
      // fill out string Alpha_num_chars[string_num] to blank out prior string
      end_gap_len = Alpha_num_chars[string_num] - as_index;
    }
    for (byte i = 0; i < end_gap_len; i++) {
      if (Trace) {
        Serial.println("load_string: adding ' ' for END_GAP");
      }
      Alpha_string[string_num][as_index++] = Alpha_decoder[' '];
    }
    if (Trace) {
      Serial.print("load_string: final String_len "); Serial.println(as_index);
    }
    String_len[string_num] = as_index;
    Scrolling_index[string_num] = 0;
    display_string(string_num);
  }
}

#define TEST_ALPHA_DECODER_STRING_DELAY   1000
#define TEST_STRING1                      "~`@$^*()_-+={}\\\"'<>/ \t?"
#define TEST_STRING2                      "A'B`CD\"E\"FGHIJKLMNOPQRSTUVWXYZ"   

byte Test_i;

// Activate by setting Timeout_fun_runtime[TEST_ALPHA_DECODER] = 1;

unsigned short test_alpha_decoder(void) {
  // Returns delay (in msecs) until next step, 0 means done.
  if (Test_i == 0) {
    load_string(0, TEST_STRING1);
    Test_i++;
    return (strlen(TEST_STRING1) + 3) * SCROLL_DELAY + TEST_ALPHA_DECODER_STRING_DELAY;
  }
  if (Test_i == 1) {
    load_string(0, TEST_STRING2);
    Test_i++;
    return (strlen(TEST_STRING2) + 3) * SCROLL_DELAY + TEST_ALPHA_DECODER_STRING_DELAY;
  }
  load_string(0, "");
  Test_i = 0;
  return 0; // done!
}

// vim: sw=2
