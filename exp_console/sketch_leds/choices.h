// choices.h

#define MAX_CHOICES          4
#define MAX_CHOICES_LENGTH  16

extern byte EEPROM_Num_choices(void);
extern byte EEPROM_Choices_start(byte choices);
extern byte EEPROM_Choices_length(byte choices);

extern byte Num_choices;
extern byte Choices_start[MAX_CHOICES];
extern byte Choices_length[MAX_CHOICES];

extern byte setup_choices(byte my_EEPROM_offset);
extern void select_choice(byte choices, byte choice);
extern void clear_choices(byte choices);

// vim: sw=2
