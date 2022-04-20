# wave_play.py

import wave
import numpy as np


test_data = (0,0,0,1,2,0,0xff,0xfe,0xfe,00)

def wave_write(channels):
    with wave.open("play.wav", "wb") as w:
        w.setnchannels(channels)
        w.setsampwidth(2)
        w.setframerate(44100)
        w.setnframes(5)
        w.writeframes(bytes(test_data))
        print("wrote", test_data)


def wave_read():
    with wave.open("play.wav", "rb") as w:
        print("nchannels", w.getnchannels())
        print("sampwidth", w.getsampwidth())
        print("framerate", w.getframerate())
        print("nframes", w.getnframes())
        print("data", tuple(w.readframes(5)))


if __name__ == "__main__":
    # wave files are little-endian.
    wave_write(1)
    wave_read()
    a = np.full(5, 1, dtype='i2')
    print("np array", a)
    print("np tobytes", a.tobytes())
