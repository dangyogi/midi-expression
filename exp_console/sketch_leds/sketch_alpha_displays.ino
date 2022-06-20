// sketch_alpha_displays.ino

#include "flash_errno.h"

#include "step.h"
#include "sketch_alpha_displays.h"

// Defines "const col_ports_t Alpha_decoder[]" indexed by char (< 128)
#include "Alpha_decoder.h"

static byte EEPROM_offset;

byte EEPROM_Num_alpha_strings(void) {
  return EEPROM_offset;
}

byte EEPROM_Alpha_num_chars(byte string_num) {
  return 1 + string_num;
}

byte EEPROM_Alpha_index(byte string_num) {
  return 1 + MAX_NUM_STRINGS + string_num;
}

byte Num_alpha_strings;
byte Alpha_num_chars[MAX_NUM_STRINGS]; // num chars display units in alpha string
byte Alpha_index[MAX_NUM_STRINGS];     // word index for left-most char display

byte setup_alpha_displays(byte my_EEPROM_offset) {
  // put your setup code here, to run once:
  EEPROM_offset = my_EEPROM_offset;

  byte b = EEPROM[EEPROM_Num_alpha_strings()];
  if (b == 0xFF) {
    Serial.println("Num_alpha_strings not set in EEPROM");
  } else if (b > MAX_NUM_STRINGS) {
    Errno = 110;
    Err_data = b;
  } else Num_alpha_strings = b;

  byte i;
  for (i = 0; i < Num_alpha_strings; i++) {
    b = EEPROM[EEPROM_Alpha_num_chars(i)];
    if (b == 0xFF) {
      Serial.print("Alpha_num_chars not set in EEPROM for ");
      Serial.println(i);
    } else if (b > MAX_STRING_LEN) {
      Errno = 111;
      Err_data = b;
    } else Alpha_num_chars[i] = b;

    b = EEPROM[EEPROM_Alpha_index(i)];
    if (b == 0xFF) {
      Serial.print("Alpha_index not set in EEPROM for ");
      Serial.println(i);
    } else if (b > Num_rows * NUM_COLS) {
      Errno = 112;
      Err_data = b;
    } else Alpha_index[i] = b;
  } // end for (i)

  return 1 + 2*MAX_NUM_STRINGS;
} // end setup_alpha_displays()

col_ports_t Alpha_string[MAX_STRING_LEN + END_GAP][MAX_NUM_STRINGS];
byte String_len[MAX_NUM_STRINGS];
byte Scrolling_index[MAX_NUM_STRINGS];   // current starting scrolling index
unsigned long Time_to_advance_strings;   // millis()

byte advance_strings(void) {
  // Called by timeout().
  // Returns 1 if scrolling done, else 0.
  unsigned long now = millis();
  if (now >= Time_to_advance_strings) {
    byte i;
    for (i = 0; i < MAX_NUM_STRINGS; i++) {
      if (String_len[i] > Alpha_num_chars[i]) {
        // We're scrolling this string!  Time to advance 1 char...
        Scrolling_index[i] += 1;
        Scrolling_index[i] %= String_len[i];
        byte j;
        for (j = 0; j < Alpha_num_chars[i]; j++) {
          Col_ports[Alpha_index[i] + j] =
            Alpha_string[(Scrolling_index[i] + j) % String_len[i]][i];
        } // end for (j)
      } // end if (scrolling this string?)
    } // end for (i)
    Time_to_advance_strings = now + SCROLL_DELAY;
    return 1;
  } // end if (time to advance strings)
  return 0;
}

void load_string(byte string_num, char *s) {
  // Pass NULL or "" to delete the string.
  byte as_index = 0;  // where next char goes in Alpha_string
  if (string_num >= MAX_NUM_STRINGS) {
    Errno = 70;
    Err_data = string_num;
  } else if (s == NULL || *s == 0) {
    // Turn off this string and set it to blanks.
    String_len[string_num] = 0;
    for (as_index = 0; as_index < Alpha_num_chars[string_num]; as_index++) {
      Col_ports[Alpha_index[string_num] + as_index++] = Alpha_decoder[' '];
    }
  } else if (strlen(s) > MAX_STRING_LEN) {
    Errno = 71;
    Err_data = strlen(s);
  } else {
    while (*s) {
      if (*s == '.') {
        if (as_index == 0) {
          Alpha_string[as_index++][string_num] = Alpha_decoder[' '];
        }
        Alpha_string[as_index - 1][string_num].port_d |= 0b10;  // DP
        s++;
      } else {
        Alpha_string[as_index++][string_num] = Alpha_decoder[*s++];
      } // end if ('.')
    } // end while (*s)
    for (byte i = 0; i < END_GAP; i++) {
      Alpha_string[as_index++][string_num] = Alpha_decoder[' '];
    }
    String_len[string_num] = as_index;
    Scrolling_index[string_num] = 0;
  }
}
