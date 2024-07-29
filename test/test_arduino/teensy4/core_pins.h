/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2018 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
//#include "imxrt.h"
//#include "pins_arduino.h"

#define HIGH			1
#define LOW			0
#define INPUT			0
#define OUTPUT			1
#define INPUT_PULLUP		2
#define INPUT_PULLDOWN		3
#define OUTPUT_OPENDRAIN	4
#define INPUT_DISABLE		5
#define LSBFIRST		0
#define MSBFIRST		1
#define _BV(n)			(1<<(n))
#define CHANGE			4
#define FALLING			2
#define RISING			3



#ifdef __cplusplus
extern "C" {
#endif

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
uint8_t digitalRead(uint8_t pin);
void digitalToggle(uint8_t pin);

#ifdef COMMENT_OUT

void init_pins(void);
// Causes a PWM capable pin to output a pulsing waveform with specific
// duty cycle.  PWM is useful for varying the average power delivered to
// LEDs, motors and other devices.  Unless analogWriteResolution() was
// used, the range is 0 (pin stays LOW) to 256 (pin stays HIGH).  The
// PWM frequency may be configured with analogWriteFrequency().
void analogWrite(uint8_t pin, int value);
uint32_t analogWriteRes(uint32_t bits);
// Configure PWM resolution for the analogWrite() function.  For example, 12
// bits gives a range of 0 to 4096.  This function returns the prior
// resolution, allowing you to temporarily change resolution, call analogWrite()
// and then restore the resolution, so other code or libraries using analogWrite()
// are not impacted.
static inline uint32_t analogWriteResolution(uint32_t bits) { return analogWriteRes(bits); }
// Configure the PWM carrier frequency used by a specific PWM pin.  The frequency
// is a floating point number, so you are not limited to integer frequency.  You can
// have 261.63 Hz (musical note C4), if you like.  analogWriteFrequency() should
// be called before analogWrite().  If the pin is already in PWM mode, the result is
// unpredictagle.  Because groups of PWM pins are controlled by the same timer, changing
// a pin may affect others in the same group.
// See https://www.pjrc.com/teensy/td_pulse.html for details.
void analogWriteFrequency(uint8_t pin, float frequency);
// Run a function when a pin changes or has a specific input.  The function runs
// as an interrupt, so care should be taken to minimize time spent.  The mode
// may be RISING, FALLING, CHANGE, HIGH or LOW.  For best compatibility with
// Arduino boards, the first parameter should be specified as
// digitalPinToInterrupt(pin), even though not required by Teensy.
void attachInterrupt(uint8_t pin, void (*function)(void), int mode);
// Remove a previously configured attachInterrupt() function from a pin.
void detachInterrupt(uint8_t pin);
void _init_Teensyduino_internal_(void);
// Read the voltage at an analog pin.  The pin may be specified as the actual
// pin number, or names A0 to A17.  Unless analogReadResolution() was used, the
// return value is a number from 0 to 1023, representing 0 to 3.3 volts.
int analogRead(uint8_t pin);
// On Teensy 4, analogRead() always uses the 3.3V power as its reference.  This
// function has no effect, but is provided to allow programs developed for
// Arduino boards to compile.
void analogReference(uint8_t type);
void analogReadRes(unsigned int bits);
// Configure the number of bits resolution returned by analogRead().
static inline void analogReadResolution(unsigned int bits) { analogReadRes(bits); }
// Configure the number readings used (and averaged) internally when reading analog
// voltage with analogRead().  Possible configurations are 1, 4, 8, 16, 32.  More
// readings averaged gives better results, but takes longer.
void analogReadAveraging(unsigned int num);
void analog_init(void);
// Teensy 4 boards to not have capacitive touch sensing hardware.  This function
// is not implemented for Teensy 4.
int touchRead(uint8_t pin);
// Read a group of 32 fuse bits.  The input must be a fuse bits register name.
uint32_t IMXRTfuseRead(volatile uint32_t *fuses);
// Write to fuse bits.  This is a PERMANENT IRREVERSIBLE operation.  Writing can
// only change fuse bits which are 0 to 1.  User programs should only write to
// fuse registers HW_OCOTP_GP1, HW_OCOTP_GP2, HW_OCOTP_GP30, HW_OCOTP_GP31,
// HW_OCOTP_GP32 and HW_OCOTP_GP33.  On Lockable Teensy, writing to the wrong
// fuses can PERMANENTLY BRICK your hardware.  After writing 1 or more fuse bit
// registers, call IMXRTfuseReload() for changes to become visible.
void IMXRTfuseWrite(volatile uint32_t *fuses, uint32_t value);
// Reloads the fuse bits buffer memory from actual fuse hardware.
void IMXRTfuseReload();

// Transmit 8 bits to a shift register connected to 2 digital pins.  bitOrder
// may be MSBFIRST or LSBFIRST.
static inline void shiftOut(uint8_t, uint8_t, uint8_t, uint8_t) __attribute__((always_inline, unused));
extern void _shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t value) __attribute__((noinline));
extern void shiftOut_lsbFirst(uint8_t dataPin, uint8_t clockPin, uint8_t value) __attribute__((noinline));
extern void shiftOut_msbFirst(uint8_t dataPin, uint8_t clockPin, uint8_t value) __attribute__((noinline));

static inline void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t value)
{
        if (__builtin_constant_p(bitOrder)) {
                if (bitOrder == LSBFIRST) {
                        shiftOut_lsbFirst(dataPin, clockPin, value);
                } else {
                        shiftOut_msbFirst(dataPin, clockPin, value);
                }
        } else {
                _shiftOut(dataPin, clockPin, bitOrder, value);
        }
}

// Receive 8 bits from a shift register connected to 2 digital pins.  bitOrder
// may be MSBFIRST or LSBFIRST.
static inline uint8_t shiftIn(uint8_t, uint8_t, uint8_t) __attribute__((always_inline, unused));
extern uint8_t _shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) __attribute__((noinline));
extern uint8_t shiftIn_lsbFirst(uint8_t dataPin, uint8_t clockPin) __attribute__((noinline));
extern uint8_t shiftIn_msbFirst(uint8_t dataPin, uint8_t clockPin) __attribute__((noinline));

static inline uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder)
{
        if (__builtin_constant_p(bitOrder)) {
                if (bitOrder == LSBFIRST) {
                        return shiftIn_lsbFirst(dataPin, clockPin);
                } else {
                        return shiftIn_msbFirst(dataPin, clockPin);
                }
        } else {
                return _shiftIn(dataPin, clockPin, bitOrder);
        }
}

void _reboot_Teensyduino_(void) __attribute__((noreturn));
void _restart_Teensyduino_(void) __attribute__((noreturn));

// Define a set of flags to know which things yield should check when called. 
extern uint8_t yield_active_check_flags;

#define YIELD_CHECK_USB_SERIAL      0x01  // check the USB for Serial.available()
#define YIELD_CHECK_HARDWARE_SERIAL 0x02  // check Hardware Serial ports available
#define YIELD_CHECK_EVENT_RESPONDER 0x04  // User has created eventResponders that use yield
#define YIELD_CHECK_USB_SERIALUSB1  0x08  // Check for SerialUSB1
#define YIELD_CHECK_USB_SERIALUSB2  0x10  // Check for SerialUSB2

// Allow other functions to run.  Typically these will be serial event handlers
// and functions call by certain libraries when lengthy operations complete.
void yield(void);
#endif // COMMENT_OUT

void delay(uint32_t msec);

#ifdef COMMENT_OUT
extern volatile uint32_t F_CPU_ACTUAL;
extern volatile uint32_t F_BUS_ACTUAL;
extern volatile uint32_t scale_cpu_cycles_to_microseconds;
extern volatile uint32_t systick_millis_count;
#endif // COMMENT_OUT

uint32_t millis(void);

uint32_t micros(void);

void delayMicroseconds(uint32_t usec);

#ifdef COMMENT_OUT

static inline void delayNanoseconds(uint32_t) __attribute__((always_inline, unused));
// Wait for a number of nanoseconds.  During this time, interrupts remain
// active, but the rest of your program becomes effectively stalled.
static inline void delayNanoseconds(uint32_t nsec)
{
	uint32_t begin = ARM_DWT_CYCCNT;
	uint32_t cycles =   ((F_CPU_ACTUAL>>16) * nsec) / (1000000000UL>>16);
	while (ARM_DWT_CYCCNT - begin < cycles) ; // wait
}


unsigned long rtc_get(void);
void rtc_set(unsigned long t);
void rtc_compensate(int adjust);

void tempmon_init(void);
float tempmonGetTemp(void);
void tempmon_Start();
void tempmon_Stop();
void tempmon_PwrDwn();

#endif // COMMENT_OUT

#ifdef __cplusplus
}

#ifdef COMMENT_OUT

// DateTimeFields represents calendar date & time with 7 fields, hour (0-23), min
// (0-59), sec (0-59), wday (0-6, 0=Sunday), mday (1-31), mon (0-11), year
// (70-206, 70=1970, 206=2106).  These 7 fields follow C standard "struct tm"
// convention, but are stored with only 8 bits to conserve memory.
typedef struct  {
	uint8_t sec;   // 0-59
	uint8_t min;   // 0-59
	uint8_t hour;  // 0-23
	uint8_t wday;  // 0-6, 0=sunday
	uint8_t mday;  // 1-31
	uint8_t mon;   // 0-11
	uint8_t year;  // 70-206, 70=1970, 206=2106
} DateTimeFields;
// Convert a "unixtime" number into 7-field DateTimeFields
void breakTime(uint32_t time, DateTimeFields &tm);  // break 32 bit time into DateTimeFields
// Convert 7-field DateTimeFields to a "unixtime" number.  The wday field is not used.
uint32_t makeTime(const DateTimeFields &tm); // convert DateTimeFields to 32 bit time

class teensy3_clock_class
{
public:
        static unsigned long get(void) __attribute__((always_inline)) { return rtc_get(); }
        static void set(unsigned long t) __attribute__((always_inline)) { rtc_set(t); }
        static void compensate(int adj) __attribute__((always_inline)) { rtc_compensate(adj); }
};
extern teensy3_clock_class Teensy3Clock;

#endif // COMMENT_OUT
#endif // __cplusplus
