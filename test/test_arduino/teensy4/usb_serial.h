// usb_serial.h

#pragma once

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

enum LookaheadMode {SKIP_ALL, SKIP_NONE, SKIP_WHITESPACE};

extern void serial_begin(long baud);
extern bool serial_bool(void);
extern int serial_available(void);
extern int serial_peekchar(void);
extern int serial_read(void);
extern long serial_parseInt(LookaheadMode lookahead, char ignore);
extern size_t serial_println_void(void);
extern size_t serial_print_str(const char s[]);
extern size_t serial_println_str(const char s[]);
extern size_t serial_print_ulong(unsigned long l, int base);
extern size_t serial_println_ulong(unsigned long l, int base);
extern size_t serial_print_long(long l, int base);
extern size_t serial_println_long(long l, int base);
extern size_t serial_putchar(uint8_t c);
extern size_t serial_print_double(double d);
extern size_t serial_println_double(double d);


class serial_class
{
public:
    void begin(long baud) { return serial_begin(baud); }

    operator bool() { return serial_bool(); }
    int available() { return serial_available(); }
    int peek() { return serial_peekchar(); }
    int read() { return serial_read(); }
    long parseInt(LookaheadMode lookahead = SKIP_ALL, char ignore = '\x01') { return serial_parseInt(lookahead, ignore); }

    size_t println(void) { return serial_println_void(); }
    size_t print(const char s[]) { return serial_print_str(s); }
    size_t println(const char s[]) { return serial_println_str(s); }

    size_t print(unsigned long l, int base) { return serial_print_ulong(l, base); }
    size_t println(unsigned long l, int base) { return serial_println_ulong(l, base); }
    size_t print(unsigned int l, int base) { return serial_print_ulong(l, base); }
    size_t println(unsigned int l, int base) { return serial_println_ulong(l, base); }
    size_t print(unsigned char l, int base) { return serial_print_ulong(l, base); }
    size_t println(unsigned char l, int base) { return serial_println_ulong(l, base); }
    size_t print(unsigned long l) { return print(l, DEC); }
    size_t println(unsigned long l) { return println(l, DEC); }
    size_t print(unsigned int l) { return print((unsigned long)l, DEC); }
    size_t println(unsigned int l) { return println((unsigned long)l, DEC); }
    size_t print(unsigned char l) { return print((unsigned long)l, DEC); }
    size_t println(unsigned char l) { return println((unsigned long)l, DEC); }

    size_t print(long l, int base) { return serial_print_long(l, base); }
    size_t println(long l, int base) { return serial_println_long(l, base); }
    size_t print(int l, int base) { return serial_print_long(l, base); }
    size_t println(int l, int base) { return serial_println_long(l, base); }
    size_t print(char l, int base) { return serial_print_long(l, base); }
    size_t println(char l, int base) { return serial_println_long(l, base); }
    size_t print(long l) { return print((long)l, DEC); }
    size_t println(long l) { return println((long)l, DEC); }
    size_t print(int l) { return print((long)l, DEC); }
    size_t println(int l) { return println((long)l, DEC); }
    size_t print(char l) { return print((long)l, DEC); }
    size_t println(char l) { return println((long)l, DEC); }

    size_t print(double d) { return serial_print_double(d); }
    size_t println(double d) { return serial_println_double(d); }

    size_t write(uint8_t c) { return serial_putchar(c); }
    size_t write(char c) { return serial_putchar(c); }
    size_t write(unsigned int i) { return serial_putchar(i); }
    size_t write(int i) { return serial_putchar(i); }
    size_t write(unsigned long l) { return serial_putchar(l); }
    size_t write(long l) { return serial_putchar(l); }
};

extern serial_class Serial;

