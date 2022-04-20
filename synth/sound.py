# sound.py

r'''
'''

import math
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
    pass
