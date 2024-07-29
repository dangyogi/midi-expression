// Wire.h

#ifndef TwoWire_h
#define TwoWire_h

extern void TwoWire_begin_void(byte channel);
extern void TwoWire_begin(byte channel, uint8_t address);
extern void TwoWire_end(byte channel);
extern void TwoWire_setClock(byte channel, uint32_t frequency);
extern void TwoWire_beginTransmission(byte channel, uint8_t address);
extern uint8_t TwoWire_endTransmission(byte channel, uint8_t sendStop);
extern size_t TwoWire_write(byte channel, const uint8_t *buf_addr, size_t len);
extern int TwoWire_available(byte channel);
extern int TwoWire_read(byte channel);
extern uint8_t TwoWire_requestFrom(byte channel, uint8_t address, uint8_t quantity, uint8_t sendStop);


class TwoWire
{
 private:
    byte channel;
 public:
    TwoWire(byte _channel) {channel = _channel;}
    void begin() {TwoWire_begin_void(channel);}
    void begin(uint8_t address) {TwoWire_begin(channel, address);}
    void begin(int address) {begin((uint8_t)address);}
    void end() {TwoWire_end(channel);}
    void setClock(uint32_t frequency) {TwoWire_setClock(channel, frequency);}
    void beginTransmission(uint8_t address)  {TwoWire_beginTransmission(channel, address);}
    void beginTransmission(int address) {beginTransmission((uint8_t)address);}
    uint8_t endTransmission(uint8_t sendStop) {return TwoWire_endTransmission(channel, sendStop);}
    uint8_t endTransmission(void) {return endTransmission(1);}
    size_t write(const uint8_t *buf_addr, size_t len) {return TwoWire_write(channel, buf_addr, len);}
    int available(void) {return TwoWire_available(channel);}
    int read(void) {return TwoWire_read(channel);}
    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) {return TwoWire_requestFrom(channel, address, quantity, sendStop);}
    uint8_t requestFrom(uint8_t address, uint8_t quantity, bool sendStop) {
      return requestFrom(address, quantity, (uint8_t)(sendStop ? 1 : 0));
    }
    uint8_t requestFrom(uint8_t address, uint8_t quantity) {
      return requestFrom(address, quantity, (uint8_t)1);
    }
    uint8_t requestFrom(int address, int quantity, int sendStop) {
      return requestFrom((uint8_t)address, (uint8_t)quantity,
        (uint8_t)(sendStop ? 1 : 0));
    }
    uint8_t requestFrom(int address, int quantity) {
      return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)1);
    }

/*******
    virtual size_t write(uint8_t);
    virtual int peek(void);
        virtual void flush(void);
    void onReceive( void (*)(int) );
    void onRequest( void (*)(void) );
**********/
};

extern TwoWire Wire;
extern TwoWire Wire1;

#endif
