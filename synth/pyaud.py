# pyaud.py

r'''Uses pyaudio to output the sound to the soundcard through ALSA.

Run the program with pasuspender to shut off pulse audio!
'''

import time
import numpy as np
import pyaudio


# Allowed (though not necessarily support on your platform) sample rates with
# simpleaudio: 8, 11.025, 16, 22.05, 32, 44.1, 48, 88.2, 96 and 192 kHz.
#
#    8000 - 2**6, 5**3  (40/sec w/5, 32 or 64/sec, w/o 5)
#   16000 - 2**7, 5**3  (40/sec w/5, 32 or 64/sec, w/o 5)
#   32000 - 2**8, 5**3  (40/sec w/5, 32 or 64/sec, w/o 5)
#   11025 - 3**2, 5**2, 7**2  (45/sec w/5, 49 or 63/sec w/7)
#   22050 - 2, 3**2, 5**2, 7**2  (50/sec w/5, 63/sec w/7)
#   44100 - 2**2, 3**2, 5**2, 7**2  (50 or 60/sec w/5, 63/sec w/7)
#   88200 - 2**3, 3**2, 5**2, 7**2  (50 or 60/sec w/5, 63/sec w/7) 
#   48000 - 2**7, 3, 5**3  (50 or 60/sec w/5, 64/sec w/o 5)
#   96000 - 2**8, 3, 5**3  (50 or 60/sec w/5, 64/sec w/o 5)
#  192000 - 2**9, 3, 5**3  (50 or 60/sec w/5, 64/sec w/o 5)


class Soundcard:
    callback_flags = {
        pyaudio.paInputUnderflow: "Input Underflow",
        pyaudio.paInputOverflow: "Input Overflow",
        pyaudio.paOutputUnderflow: "Output Underflow",
        pyaudio.paOutputOverflow: "Output Overflow",
        pyaudio.paPrimingOutput: "Priming Output",
    }

    process_overhead = 0.001   # sec, seems to be min required to prevent underflows

    def __init__(self, fill_sound_block_fn, channels=1, sample_rate=44100, bits=16,
                 block_duration=0.050, idle_fn=None):
        r'''
        fill_sound_block_fn takes an np.array for the sound block and the target time,
        according to time.perf_counter().  It returns True to continue, False to quit.

        The sound block is a float8 array of samples.  Sample values are percent of max
        possible volume.
        '''
        self.fill_sound_block_fn = fill_sound_block_fn
        self.channels = channels
        self.sample_rate = sample_rate
        self.block_duration = block_duration
        self.bits = bits

        self.max_sound_level = 2**(bits - 1) - 1
        self.delta_t = 1/sample_rate
        self.block_size = round(block_duration * sample_rate)   # samples/block as int
        self.delta_times = np.full(self.block_size, self.delta_t)
        print(f"{self.channels=}, {self.sample_rate=}, {self.delta_t=}, {self.bits=}")
        print(f"{self.max_sound_level=}, {self.block_duration=}, {self.block_size=}")

        self.audio = pyaudio.PyAudio()
        self.stream = self.audio.open(
                        format=self.audio.get_format_from_width(bits//8),
                        #format=pyaudio.paFloat32,  # doesn't work...
                        channels=self.channels,
                        rate=self.sample_rate,
                        frames_per_buffer=self.block_size,  # error with 2*block_size
                            # shows up as frame_count in callback
                            # seems to default to 1024.
                        output=True,
                        output_device_index=0,         # required with pasuspender
                        stream_callback=self.callback,
                        start=False,   # could be omitted for blocking writes
                       )
        self.block = self.new_block()
        self.idle_fn = idle_fn
        self.process_time_overruns = 0
        self.total_process_time_over = 0
        self.callback_calls = 0
        self.in_callback = False
        #print(dir(self.stream))

    def run(self):
        r'''Never returns...
        '''
        print("Soundcard.run called")
        self.callback_calls = 0
        self.start_time = time.perf_counter()
        self.underflows = 0
        try:
            self.stream.start_stream()
            print("output_latency", self.stream.get_output_latency())
            while self.stream.is_active():
                if self.idle_fn is not None:
                    self.idle_fn()
                time.sleep(0.1)
        finally:
            print("soundcard.run done")

    def report(self):
        print(f"soundcard: {self.underflows} underflows")
        print(f"soundcard: {self.process_time_overruns} soundcard callback "
              "process time overruns", end='')
        if self.process_time_overruns:
            avg_time_over = self.total_process_time_over \
                              / self.process_time_overruns
            print(f", avg {avg_time_over * 1e3} msec")
        else:
            print()
        print("soundcard:", self.callback_calls, "callback calls")
        print("soundcard: stream active", self.stream.is_active())

    def close(self):
        try:
            # These don't complete, don't catch any exceptions here either... ???
            # They do complete if program terminated by call to sys.exit, rather than
            # SIGINT (hitting ^C).
            print("calling steam.stop_stream")
            self.stream.stop_stream()      # never returns (or raises exception)...
            print("calling steam.close")
            self.stream.close()
            print("calling audio.terminate")
            self.audio.terminate()

            print("Soundcard.close done!")
        except BaseException as e:
            print("Soundcard.close caught exception")
            print("Soundcard.close caught:", e.__class__.__name__, e)
        finally:
            print("Soundcard.close.finally")

    def callback(self, in_data, frame_count, time_info, status):
        assert not self.in_callback, "Soundcard: callback called while still running!"
        self.in_callback = True
        assert frame_count == self.block_size, \
                 f"expected {self.block_size} frame_count, got {frame_count}"
        assert in_data is None, f"expected None in_data, got {in_data}"
        if status == pyaudio.paOutputUnderflow:
            print("soundcard.callback: got Output Underflow")
            self.underflows += 1
        else:
            assert status == 0, f"expected 0 status, got {self.callback_flags[status]}"
        # print("perf_counter", time.perf_counter())  # matches time_info.current_time
        # print("time_info", time_info)      # all zeros with pulse,
                                           # has values with pasuspender!
        self.callback_calls += 1
        target_time = time.perf_counter() + (time_info['output_buffer_dac_time']
                                              - time_info['current_time']) \
                      - self.process_overhead
        self.block.fill(0.0)
        cont = self.fill_sound_block_fn(self.block, target_time)
        now = time.perf_counter()
        if now > target_time:
            self.process_time_overruns += 1
            self.total_process_time_over += now - target_time
        #print(f"{(target_time - now) * 1e3:.2f} mSec time_target margin")
        self.block *= self.max_sound_level
        self.in_callback = False
        return self.block.astype(np.int16), \
               (pyaudio.paContinue if cont else pyaudio.paComplete)

    def new_block(self):
        return np.zeros(self.block_size, dtype='f8')



if __name__ == "__main__":

    def millis(n):
        start_time = time.perf_counter()
        n /= 1e3
        while time.perf_counter() - start_time < n:
            pass
        #print("micros", n, "took", (time.perf_counter() - start_time) * 1e6, "uSec")

    safety_factor = 1  # mSec
      # 1 mSec gives 0 underflows
      # 100 uSec gives 35% underflows
      # 300 uSec gives 10% underflows
      # 400 uSec gives 0 underflows, but sounds like crap...
      # needs to be at least 1 mSec

    def fill_sound_block(block, deadline):
        global count
        start_callback = time.perf_counter()
        delay = (deadline - start_callback) * 1e3 - safety_factor
        print(f"======= count {count} at {(start_callback - start_time) * 1e3:.2f} mSec, "
              f"waiting {delay:.2f} mSec")
        millis(delay)
        #if count % 2:
        #    # ALSA discards late arrivals, which should keep things in time sync overall.
        #    micros(60 * 1000)                  # tolerates 30 mSec, but goes silent at 40
        np.copyto(block, npsin2)
        count += 1
        return count < 20

    sc = Soundcard(fill_sound_block)
    
    freq = 440
    # The dtype='f4' speeds things up.  It gets copied to all remaining arrays.
    times = np.linspace(0, sc.block_duration, sc.block_size, endpoint=False, dtype='f4')
    print("times.dtype", times.dtype)

    # tobytes here results in little-endian data, which is what PyAudio needs!
    start_time = time.perf_counter()
    now = time.perf_counter()
    print("nothing took", (now - start_time) * 1e6, "uSec")
        # f4: 1, 1, 1, 1
        # f8: 1, 1, 1, 1

    start_time = time.perf_counter()
    sin_in = freq * 2 * np.pi * times
    now = time.perf_counter()
    print("* times took", (now - start_time) * 1e6, "uSec")
        # f4: 23, 13, 15, 17 (avg 17)
        # f8: 12, 11, 13, 17 (avg 13.25)
    print("sin_in.dtype", sin_in.dtype)

    sin_in2 = sin_in.copy()
    start_time = time.perf_counter()
    npsin = np.sin(sin_in, out=sin_in2)  # 4 uSec slower w/out=sin_in!,
                                         # but 2 uSec faster w/out=sin_in2!
    now = time.perf_counter()
    print("np.sin took", (now - start_time) * 1e6, "uSec")
        # f4: 11, 16, 12, 23 (avg 15.5); w/out=sin_in: 19, 18, 19, 21 (avg 19.25)
        #     w/out=sin_in2: 12, 18, 15, 10 (avg 13.75)
        # f8: 46, 33, 42, 37 (avg 39.5)
    print("npsin.dtype", npsin.dtype)
    print("npsin is sin_in2", npsin is sin_in2)  # True w/out=sin_in and out=sin_in2
    npsin2 = npsin.copy()
    print("npsin2.dtype", npsin2.dtype)
    print("npsin is npsin2", npsin is npsin2)    # False

    start_time = time.perf_counter()
    npsin2 *= 0.3  # 3 uSec faster than npsin * 10000.0
    now = time.perf_counter()
    print("*= 0.3 took", (now - start_time) * 1e6, "uSec")
        # f4: *=(10, 9, 10, 14 -- avg 10.75), *(12, 15, 15, 14 -- avg 14)
        # f8: *=(13, 8, 8, 8 -- avg 9.25)
    print("npsin2.dtype", npsin2.dtype)

    start_time = time.perf_counter()
    npsamples = npsin2.astype(np.int16)  # "i2" or np.int16, copy=False doesn't work here
    now = time.perf_counter()
    print("np.array to convert to int16 took", (now - start_time) * 1e6, "uSec")
        # f4: 7, 5, 7, 7 (avg 6.5)
        # f8: 11, 10, 9, 9 (avg 9.75)
    print("copy=False worked", npsamples is npsin2)
    print("npsamples.dtype", npsamples.dtype)

    # This doesn't work! Get silence, even without npsin2 copy...
    #start_time = time.perf_counter()
    #npsamples = np.multiply(npsin, 10000.0, dtype=np.int16, casting="unsafe")
    #now = time.perf_counter()
    #print("np.multiply", (now - start_time) * 1e6, "uSec")

    start_time = time.perf_counter()
    samples = npsamples.tobytes()   # not necessary with simpleaudio
    now = time.perf_counter()
    print("tobytes took", (now - start_time) * 1e6, "uSec")
        # f4: 8, 7, 8, 10 (avg 8.25)
        # f8: 9, 8, 7, 8  (avg 8)
    #print(samples)

    count = 0
    start_time = time.perf_counter()
    sc.run()
