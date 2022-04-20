# alsaaud.py

r'''
'''

import math
import numpy as np
import alsaaudio


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


def init(channels=1, sample_rate=44100, block_duration=0.050, bits=16, callback=None):
    global Channels, Sample_rate, Block_duration, Bits
    global Max_sound_level, Delta_t, Block_size, Block_times, Stream

    Channels = channels
    Sample_rate = sample_rate
    Block_duration = block_duration
    Bits = bits

    Max_sound_level = 2**(bits - 1) - 1
    Delta_t = 1/Sample_rate
    Block_size = round(Block_duration * Sample_rate)   # samples/block as int
    Block_times = np.full(Block_size, Delta_t)
    print(f"{Channels=}, {Sample_rate=}, {Delta_t=}, {Bits=}, {Max_sound_level=}")
    print(f"{Block_duration=}, {Block_size=}")

    print("pcms()")
    for device in alsaaudio.pcms():
        if "CARD=PCH" in device:
            print("   ", device)

    #print("cards()")
    ##for card in alsaaudio.cards():
    #    print("   ", card)

    Stream = alsaaudio.PCM(rate=sample_rate, channels=channels,
                           format=alsaaudio.PCM_FORMAT_S16_LE,
                           periodsize=Block_size * channels,  # in frames
                           # default buffer size is 4 * periodsize, based on 'periods' in
                           # alsapcm_setup being hard-coded at 4 (line 413), passed to
                           # snd_pcm_hw_params_set_periods_near (line 417).
                           # device='front:CARD=PCH,DEV=0',  # this works with pasuspender
                           # gets ALSAAudioError: Data size must be a multiple of framesize
                          # device='dsnoop:CARD=PCH,DEV=0',
                           # ALSAAudioError: Invalid argument [dsnoop:CARD=PCH,DEV=0]
                          # device='hw:CARD=PCH,DEV=0',
                           # gets ALSAAudioError: Data size must be a multiple of framesize
                           # with or without pasuspender. period size and buffer size
                           # output look OK...
                          # device='sysdefault:CARD=PCH',  # this works with pasuspender
                                                          # but gets wonky periodsize and
                                                          # buffer size, but seems to work!
                                                          # seems to ignore periodsize...
                           device='plughw:CARD=PCH,DEV=0',  # this works with pasuspender
                           # period size is what I sent in.
                         #  cardindex=0,
                          )
    print(f"{Stream.pcmtype()=}, {alsaaudio.PCM_CAPTURE=}, {alsaaudio.PCM_PLAYBACK=}")
    print(f"{Stream.pcmmode()=}, {alsaaudio.PCM_NONBLOCK=}, {alsaaudio.PCM_NORMAL=}")
    print(f"{Stream.cardname()=}")
    #print(f"{Stream.rate=}, {Stream.channels=}, {Stream.periodsize=}")
    #print(dir(Stream))
    # ['cardname', 'close', 'drop', 'dumpinfo', 'getchannels', 'getformats',
    #  'getratebounds', 'getrates', 'pause', 'pcmmode', 'pcmtype', 'polldescriptors',
    #  'read', 'setchannels', 'setformat', 'setperiodsize', 'setrate', 'write']
    Stream.dumpinfo()
    #print(f"{Stream.getchannels()=}, {Stream.getformats()=}, {Stream.getrates()=}")
     # The above all return all possible values.
    #print(f"{Stream.getratebounds()=}, {Stream.polldescriptors()=}")


class Sound:
    r'''Sound (volume) samples over time.

    This probably won't get used...
    '''
    def together_with(self, *others):
        r'''Returns a new Sound by adding itself and others.
        '''

    def play(self, volume_scale=1):
        pass

    def scale_by(self, ampl_scale):
        r'''Multiplies itself by ampl_scale.
        '''


class Amplitude_scale:
    r'''Amplitude scale percentages (-1 to 1) over time.

    These can either be constant (single value), or a mapping of time -> ampl_scale.
    '''
    def to_sound(self, volume):
        pass
    def scaled_by(self, *other_ampl_scales):
        r'''Returns a new Amplitude_scale.
        '''


#Harmonics = (None,) + tuple(round(math.log(h)/Ln_cent) for h in range(1, 21))

def harmonic(h):
    r'''Converts harmonic number to internal interval (in cents as an int).
    '''
    return round(math.log(h)/Ln_cent)


# FIX: delete
# C = 0
# m2 = 100
# D = M2 = 200
# m3 = 300
# E = M3 = 400
# F = m4 = 500
# M4 = m5 = 600
# G = M5 = 700
# m6 = 800
# A = M6 = 900
# m7 = 1000
# B = M7 = 1100


class Interval:
    r'''Intervals are in cents.

    May be either an absolute freq, or a freq_scale factor.

    When used as an absolute freq, the interval starts at Cb-1 (16.4Hz).  E10 (21096Hz) is
    limit of human hearing.  C0 is 0, C10 is 12000, E10 is 12400.

    May be a single interval, or a mapping of time -> interval.
    '''

    def add(self, *other_intervals):
        r'''Returns a new Interval.
        '''

    def to_ampl_scales(self, duration, *other_ampl_scales):
        r'''Applies sin function to produce ampl_scales over time.
        '''


class Sound_template:
    r'''A set of mappings from interval -> ampl_scale.

    Each interval may be a single interval, or a mapping of time -> interval.

    Each ampl_scale may be a single ampl_scale, or a mapping of time -> ampl_scale.

    A single mapping could represent a harmonic.
    '''
    def add(self, *other_sound_templates):
        pass
    def to_sound(self, volume, base_freq, duration=None):
        pass


class Filter:
    r'''Mapping of freq -> ampl_scale.
    '''
    def filtered_by(self, *other_filters):
        r'''Ampl_scales are multiplied.
        '''

    def filter(self, sound_template):
        pass


class Cycle:
    r'''Maps percentage_of_cycle -> ampl_scale.
    '''


class Note:
    pass



if __name__ == "__main__":
    import time

    def micros(n):
        start_time = time.perf_counter()
        for _ in range(116 * n):
            pass
        now = time.perf_counter()
        #print("micros", n, "took", (now - start_time) * 1e6, "uSec")

    use_callback = False

    delay = 30 * 1000

    count = 0
    def callback(in_data, frame_count, time_info, status):
        global count
        start_callback = time.perf_counter_ns()
        now = time.perf_counter_ns()
        print("============ count", count, "at", (now - start_time) / 1e6, "msec")
        #print("in_data", in_data)          # None for output
        print("frame_count", frame_count)
        print("perf_counter", time.perf_counter())
        print("time_info", time_info)      # all zeros with pulse,
                                           # has values with pasuspender!
        print("status", status)            # 0
        print("waiting", delay, "uSec")
        if count % 2:
            # ALSA discards late arrivals, which should keep things in time sync overall.
            micros(60 * 1000)                      # tolerates 30 mSec, but goes silent at 40
        count += 1
        now = time.perf_counter_ns()
        print((now - start_callback) / 1e6, "msec total callback delay")
        if count >= 20:
            return samples, pyaudio.paComplete
        return samples, pyaudio.paContinue

    init()
    
    freq = 440
    # The dtype='f4' speeds things up.  It gets copied to all remaining arrays.
    times = np.linspace(0, Block_duration, Block_size, endpoint=False, dtype='f4')
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
    npsin2 *= 10000.0  # 3 uSec faster than npsin * 10000.0
    now = time.perf_counter()
    print("*= 10000 took", (now - start_time) * 1e6, "uSec")
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

    if use_callback:
        start_time = time.perf_counter_ns()
        Stream.start_stream()
        while Stream.is_active():
            print("output_latency", Stream.get_output_latency())
            time.sleep(0.1)
    else:
        #Stream.start_stream()
        for t in range(0, 2000, 50):
            #start_time = now
            #micros(60 * 1000)      # this gets drop-outs even with no delay...  :-(
            print(f"{len(npsamples)=}")
            start_time = time.perf_counter_ns()
            data_size = Stream.write(npsamples)   # either nparray or bytes works,
                                                  # both return 2205 frames
            #data_size = Stream.write(samples)
            now = time.perf_counter_ns()
            print("write delay", (now - start_time) / 1e6, "msec", t)
            print("write returned", data_size)

    #Stream.stop_stream()
    Stream.close()
