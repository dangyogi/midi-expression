// sketch_i2c_slave.ino

#include <Wire.h>

#define LED_PIN   13

void
setup(void) {
  Serial.begin(230400);
  pinMode(LED_PIN, OUTPUT);

  Wire.begin(0x31);
  Wire.setClock(100000);
  Wire.onReceive(receiveRequest);  // callback for requests from on high
  Wire.onRequest(sendReport);      // callback for reports to on high

  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
}

byte Buffer[31];
byte Buf_len;

void receiveRequest(int how_many) {
  // callback for requests from on high

  // Only request is: #byte,bytes which returns the bytes in reverse order + 0x10
  Buf_len = Wire.read();
  byte i;
  for (i = 0; i < Buf_len; i++) {
    Buffer[i] = Wire.read();
  }
}

void sendReport(void) {
  // callback for reports requested from on high
  byte i, b2;
  for (i = Buf_len; i > 0; i--) {
    b2 = Wire.write(Buffer[i - 1] + 0x10);
    if (b2 != 1) {
      digitalWrite(LED_PIN, HIGH);
      Serial.print("Wire.write returned ");
      Serial.print(b2);
      Serial.println(", expected 1");
    }
  }
}

void
loop(void) {
}

// vim: sw=2
