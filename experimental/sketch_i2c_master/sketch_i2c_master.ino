
// sketch_i2c_master.ino

#include <Wire.h>

void
setup(void) {
  Serial.begin(230400);

  Wire.begin();
  Wire.setClock(100000);

  delay(1500);   // give RAM controller time to get started
  Serial.println("Ready");
}

void sendRequest(byte i2c_addr, byte *data, byte data_len) {
  unsigned long start_time = micros();
  byte b0, status;
  Wire.beginTransmission(i2c_addr);
  b0 = Wire.write(data, data_len);
  if (b0 != data_len) {
    Serial.print("Wire.write returned ");
    Serial.print(b0);
    Serial.print(" data_len sent to it was ");
    Serial.println(data_len);
  }
  status = Wire.endTransmission();
  unsigned long elapsed_time = micros() - start_time;
  if (status) {
    Serial.print("Wire.endTransmission failed: ");
    switch (status) {
    case 1:
      Serial.println("data too long to fit in transmit buffer");
      break;
    case 2:
      Serial.println("received NACK on transmit of address");
      break;
    case 3:
      Serial.println("received NACK on transmit of data");
      break;
    case 4:
      Serial.println("other error");
      break;
    case 5:
      Serial.println("timeout");
      break;
    default:
      Serial.println("undocumented status number");
      break;
    }
  }
  Serial.print(elapsed_time);
  Serial.print(" uSec to send ");
  Serial.print(data_len);
  Serial.println(" bytes");
}

byte ResponseData[32];

byte getResponse(byte i2c_addr, size_t data_len) {
  // Returns bytes received.  (0 if error)
  // Data in ResponseData.
  byte i;
  unsigned long start_time = micros();
  if (data_len > 32) {
    Serial.println("ERROR: data_len > 32");
    return 0;
  }
  byte bytes_received = Wire.requestFrom(i2c_addr, data_len);
  unsigned long mid_time = micros();
  unsigned long mid_elapsed_time = mid_time - start_time;
  if (bytes_received > data_len) {
    Serial.print("Bytes_received too long, got "); Serial.print(bytes_received);
    Serial.print("expected "); Serial.println(data_len);
    return 0;
  }
  if (bytes_received != Wire.available()) {
    Serial.print(F("I2C.requestFrom: bytes_received, ")); Serial.print(bytes_received);
    Serial.print(F(", != available(), ")); Serial.println(Wire.available());
  }
  for (i = 0; i < bytes_received; i++) {
    ResponseData[i] = Wire.read();
  }
  unsigned long read_time = micros() - mid_time;
  Serial.print(bytes_received);
  Serial.println(" bytes received");
  Serial.print(mid_elapsed_time);
  Serial.println(" uSec requestFrom time");
  Serial.print(read_time);
  Serial.println(" uSec read time");
  if (bytes_received == 0) {
    return bytes_received;
  }
  for (i = 0; i < bytes_received; i++) {
    Serial.print(ResponseData[i]);
    if (i + 1 < bytes_received) {
      Serial.print(", ");
    }
  }
  Serial.println();
  return bytes_received;
}

void
help(void) {
  Serial.println("? - help");
  Serial.println("S<bytes> - Send, bytes are comma seperated");
  Serial.println("R<num_bytes> - Receive");
}

void
loop(void) {
  byte b0, b1;
  byte buffer[12];
  byte buf_len;
  if (Serial.available()) {
    b0 = toupper(Serial.read());
    switch (b0) {
    case '?': help(); break;
    case 'S':
      buf_len = 0;
      buffer[buf_len++] = Serial.parseInt();
      while (Serial.read() == ',') {
        buffer[buf_len++] = Serial.parseInt();
      }
      sendRequest(0x31, buffer, buf_len);
      break;
    case 'R':
      b1 = Serial.parseInt();
      getResponse(0x31, b1);
      break;
    case ' ': case '\t': case '\n': case '\r': break;
    default: help(); break;
    } // end switch
  } // end if (Serial.available())
}

// vim: sw=2
