// Wire.cpp

#include "Wire.h"


// FIX: Implement these!

void TwoWire_begin_void(byte channel) {
}

void TwoWire_begin(byte channel, uint8_t address) {
}

void TwoWire_end(byte channel) {
}

void TwoWire_setClock(byte channel, uint32_t frequency) {
}

void TwoWire_beginTransmission(byte channel, uint8_t address) {
}

uint8_t TwoWire_endTransmission(byte channel, uint8_t sendStop) {
  return 0;
}

size_t TwoWire_write(byte channel, const uint8_t *buf_addr, size_t len) {
  return 0;
}

int TwoWire_available(byte channel) {
  return 0;
}

int TwoWire_read(byte channel) {
  return 0;
}

uint8_t TwoWire_requestFrom(byte channel, uint8_t address, uint8_t quantity, uint8_t sendStop) {
  return 0;
}


TwoWire Wire(0);
TwoWire Wire1(1);
