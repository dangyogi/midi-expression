// sketch_ram.ino

// Arduino IDE Tools settings:
//   Board -> SAMD -> Nano 33 IoT

#include <Wire.h>
#include "flash_errno.h"

#define ERR_LED   13

void setup() {
  // put your setup code here, to run once:
  err_led(ERR_LED);
  
  Wire.begin(0x33);
  Wire.setClock(400000);

  Wire.onReceive(receiveRequest);
  Wire.onRequest(sendReport);

  Errno = 55;  // Uninitialized
}

// Must leave at least 4264 bytes available in RAM

// Total 27200 bytes, leaving 5568
#define FUN_BYTES    (2*16*10*5)       /* 1600 */
#define HM_BYTES     (2*16*16*10*5)    /* 25600 */

byte Num_channels;
byte Num_ch_functions;
byte Num_encoders;

byte Num_harmonics;
byte Num_hm_functions;

byte Fun_bytes[FUN_BYTES];  // [2][Num_channels][Num_ch_functions][Num_encoders]
byte Hm_bytes[HM_BYTES];    // [2][Num_channels][Num_harmonics][Num_hm_functions][Num_encoders]

unsigned short Fun_synth_prog_step_size, Fun_ch_step_size, Fun_fun_step_size;
unsigned short Hm_synth_prog_step_size, Hm_ch_step_size, Hm_harmonic_step_size, Hm_fun_step_size;

#define Fun_value(synth_prog, ch, fun, enc)     Fun_bytes[Fun_synth_prog_step_size * (synth_prog) + Fun_ch_step_size * (ch) + \
                                                          Fun_fun_step_size * (fun) + (enc)]
#define Hm_value(synth_prog, ch, hm, fun, enc)  Hm_bytes[Hm_synth_prog_step_size * (synth_prog) + Hm_ch_step_size * (ch) + \
                                                         Hm_harmonic_step_size * (hm) + Hm_fun_step_size * (fun) + (enc)]

byte Report;
byte Synth_prog, Fun, Channel, Harmonic;

void receiveRequest(int how_many) {
  byte req, synth_prog, fun, ch, hm, enc, value;
  unsigned short ch_map, hm_map;

  req = Wire.read();
  switch (req) {
  case 0:   // init: Num_channels, Num_ch_funs, Num_encoders, Num_harmonics, Num_hm_funs
    Report = 0;
    if (how_many != 6) {
      Errno = 1;
      Err_data = how_many;
      break;
    }
    Num_channels = Wire.read();
    Num_ch_functions = Wire.read();
    Num_encoders = Wire.read();
    Num_harmonics = Wire.read();
    Num_hm_functions = Wire.read();
    Fun_fun_step_size = Num_encoders;
    Fun_ch_step_size = Fun_fun_step_size * Num_ch_functions;
    Fun_synth_prog_step_size = Fun_ch_step_size * Num_channels;
    if (Fun_synth_prog_step_size * 2 > FUN_BYTES) {
      Errno = 2;
      Err_data = (byte)((Fun_synth_prog_step_size * 2) / 10);
      Num_channels = 0;
      break;
    }
    Hm_fun_step_size = Num_encoders;
    Hm_harmonic_step_size = Hm_fun_step_size * Num_hm_functions;
    Hm_ch_step_size = Hm_harmonic_step_size * Num_harmonics;
    Hm_synth_prog_step_size = Hm_ch_step_size * Num_channels;
    if (Hm_synth_prog_step_size * 2 > HM_BYTES) {
      Errno = 3;
      Err_data = (byte)((Hm_synth_prog_step_size * 2) / 200);
      Num_channels = 0;
      break;
    }
    if (Errno == 55) Errno = 0;
    break;
  case 1:   // get_ch_values: synth_prog, channel, function
    Report = 0;
    if (Num_channels == 0) {
      Errno = 10;
      Err_data = 0;
      break;
    }
    if (how_many != 4) {
      Errno = 11;
      Err_data = how_many;
      break;
    }
    Synth_prog = Wire.read();
    if (Synth_prog >= 2) {
      Errno = 12;
      Err_data = Synth_prog;
      break;
    }
    Channel = Wire.read();
    if (Channel >= Num_channels) {
      Errno = 13;
      Err_data = Channel;
      break;
    }
    Fun = Wire.read();
    if (Fun >= Num_ch_functions) {
      Errno = 14;
      Err_data = Fun;
      break;
    }
    Report = 1;
    break;
  case 2:   // get_hm_values: synth_prog, channel, harmonic, function
    Report = 0;
    if (Num_channels == 0) {
      Errno = 20;
      Err_data = 0;
      break;
    }
    if (how_many != 5) {
      Errno = 21;
      Err_data = how_many;
      break;
    }
    Synth_prog = Wire.read();
    if (Synth_prog >= 2) {
      Errno = 22;
      Err_data = Synth_prog;
      break;
    }
    Channel = Wire.read();
    if (Channel >= Num_channels) {
      Errno = 23;
      Err_data = Channel;
      break;
    }
    Harmonic = Wire.read();
    if (Harmonic >= Num_harmonics) {
      Errno = 24;
      Err_data = Harmonic;
      break;
    }
    Fun = Wire.read();
    if (Fun >= Num_hm_functions) {
      Errno = 25;
      Err_data = Fun;
      break;
    }
    Report = 2;
    break;
  case 3:   // set_ch_values: synth_prog, channel_bitmap (2 bytes), function,
            //                values (Num_encoder bytes)
    Report = 0;
    if (Num_channels == 0) {
      Errno = 30;
      Err_data = 0;
      break;
    }
    if (how_many != 5 + Num_encoders) {
      Errno = 31;
      Err_data = how_many;
      break;
    }
    synth_prog = Wire.read();
    if (synth_prog >= 2) {
      Errno = 32;
      Err_data = synth_prog;
      break;
    }
    ch_map = ((unsigned short)(Wire.read()) << 8) | Wire.read();
    fun = Wire.read();
    if (fun >= Num_ch_functions) {
      Errno = 33;
      Err_data = fun;
      break;
    }
    for (enc = 0; enc < Num_encoders; enc++) {
      value = Wire.read();
      for (ch = 0; ch < Num_channels; ch++) {
        if (!ch_map) break;
        if (ch_map & 1) Fun_value(synth_prog, ch, fun, enc) = value;
        ch_map >>= 1;
      } // end for (ch)
    } // end for (enc)
    break;
  case 4:   // set_hm_values: synth_prog, channel_bitmap (2 bytes),
            //                harmonic_bitmap (2 bytes), function,
            //                values (Num_encoder bytes)
    Report = 0;
    if (Num_channels == 0) {
      Errno = 40;
      Err_data = 0;
      break;
    }
    if (how_many != 7 + Num_encoders) {
      Errno = 41;
      Err_data = how_many;
      break;
    }
    synth_prog = Wire.read();
    if (synth_prog >= 2) {
      Errno = 42;
      Err_data = synth_prog;
      break;
    }
    ch_map = ((unsigned short)(Wire.read()) << 8) | Wire.read();
    hm_map = ((unsigned short)(Wire.read()) << 8) | Wire.read();
    fun = Wire.read();
    if (fun >= Num_hm_functions) {
      Errno = 43;
      Err_data = fun;
      break;
    }
    for (enc = 0; enc < Num_encoders; enc++) {
      value = Wire.read();
      for (ch = 0; ch < Num_channels; ch++) {
        if (!ch_map) break;
        if (ch_map & 1) {
          for (hm = 0; hm < Num_harmonics; hm++) {
            if (!hm_map) break;
            if (hm_map & 1) Hm_value(synth_prog, ch, hm, fun, enc) = value;
            hm_map >>= 1;
          } // end for (hm)
        } // end if ch_map bit set
        ch_map >>= 1;
      } // end for (ch)
    } // end for (enc)
    break;
  case 5:   // Copy synth values to prog
    Report = 0;
    if (Num_channels == 0) {
      Errno = 45;
      Err_data = 0;
      break;
    }
    if (how_many != 1) {
      Errno = 46;
      Err_data = how_many;
      break;
    }
    for (enc = 0; enc < Num_encoders; enc++) {
      for (ch = 0; ch < Num_channels; ch++) {
        for (fun = 0; fun < Num_ch_functions; fun++) {
          Fun_value(1, ch, fun, enc) = Fun_value(0, ch, fun, enc);
        }
      }
    } // end for (enc)
    for (enc = 0; enc < Num_encoders; enc++) {
      for (ch = 0; ch < Num_channels; ch++) {
        for (hm = 0; hm < Num_harmonics; hm++) {
          for (fun = 0; fun < Num_hm_functions; fun++) {
            Hm_value(1, ch, hm, fun, enc) = Hm_value(0, ch, hm, fun, enc);
          }
        }
      }
    } // end for (enc)
    break;
  case 6:       // Set Errno, Err_data
    Report = 0;
    if (how_many != 3) {
      Errno = 47;
      Err_data = how_many;
      break;
    }
    Errno = Wire.read();
    Err_data = Wire.read();
    break;
  case 7:       // Report Errno, Err_data
    Report = 0;
    if (how_many != 1) {
      Errno = 48;
      Err_data = how_many;
      break;
    }
    break;
  default:
    Errno = 49;
    Err_data = req;
    Report = 0;
    break;
  } // end switch
}

void sendReport(void) {
  byte len_written, enc;
  switch (Report) {
  case 0:    // Errno, Err_data
    len_written = Wire.write(Errno);
    if (len_written != 1) {
      Errno = 50;
      Err_data = len_written;
    }
    len_written = Wire.write(Err_data);
    if (len_written != 1) {
      Errno = 51;
      Err_data = len_written;
    }
    Errno = 0;
    Err_data = 0;
    break;
  case 1:    // fun enc_value * Num_encoders
    for (enc = 0; enc < Num_encoders; enc++) {
      len_written = Wire.write(Fun_value(Synth_prog, Channel, Fun, enc));
      if (len_written != 1) {
        Errno = 52;
        Err_data = len_written;
      }
    }
    Report = 0;
    break;
  case 2:    // hm enc_value * Num_encoders
    for (enc = 0; enc < Num_encoders; enc++) {
      len_written = Wire.write(Hm_value(Synth_prog, Channel, Harmonic, Fun, enc));
      if (len_written != 1) {
        Errno = 53;
        Err_data = len_written;
      }
    }
    Report = 0;
    break;
  default:
    Errno = 54;
    Err_data = Report;
    Report = 0;
    break;
  } // end switch
}

void loop() {
  // put your main code here, to run repeatedly:
  errno();
}


// vim: sw=2
