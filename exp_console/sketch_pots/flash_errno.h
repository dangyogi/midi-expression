// flash_errno.h

extern byte Errno;
extern byte Err_data;

extern void err_led(byte led_pin1, byte led_pin2=0xFF);
extern void report_errno(void);

// vim: sw=2
