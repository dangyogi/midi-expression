# tuning_systems.py

r'''
Notes: 0-127, 12 notes/octave.  0 is C-1 (8.18Hz).  (see doc string in notes.py).

Notes are translated into my freqs using a Tuning_system.note_to_freq(note).

My freqs are integers representing the number of (equal_temperment) cents in an interval.
Thus, an octave is 1200, a major 5th is 700, a major 3rd 400 (in equal_temperment).

My freqs are also used to represent absolute freqs by assigning freq 0 as C-1 in equal
temperament.  In this case, since the note values cover 10+ octaves, this is 12,700 freqs.
These absolute freqs can be converted to Hz with freq_to_Hz.
'''

import math
import numpy as np

from .notes import Notes, nat
from .channel import Channel, Channel_setting



# Internal intervals are based on cents in log base cent
Cent = math.pow(2, 1/1200)  # 1.0005777895065548
Ln_cent = math.log(Cent)


C4 = 1200 * 4   # offset from C0
A4 = C4 + 900   # offset from C0
A4_freq = 440
C4_freq = A4_freq / Cent**900   # 261.6256
C0_freq = A4_freq / Cent**A4    # 16.3516
#print(f"{C4_freq=}")
#print(f"{C0_freq=}")
Freq_offset = math.log(C0_freq) / Ln_cent - 1200  # Offset to C-1

def freq_to_Hz(freq):
    r'''Translate internal absolute freq value to Hz.
    '''
    return np.exp((freq + Freq_offset) * Ln_cent)

#print(f"{Freq_offset=}, {A4=}, {freq_to_Hz(A4 + 1200)=}")

def format_list(l, format="{:.2f}"):
    str_elements = ', '.join(format.format(n) for n in l)
    return f"[{str_elements}]"


class Base_tuning_system:
    r'''Converts notes to freqs.  C is 0.

    Subclass sets self.note_to_freq to a list of freqs, indexed by note.
    '''
    # Keyboard reference:
    #    # #   # # #
    #   C D E F G A B C

    cents_per_octave = 1200

    def to_tonic(self, freqs, tonic_start):
        assert len(freqs) == 12
        self.notes_to_freq = [f - freqs[-tonic_start]
                              for f in freqs[-tonic_start:]] + \
                             [f + (self.cents_per_octave - freqs[-tonic_start])
                              for f in freqs[:-tonic_start]]
        assert len(self.notes_to_freq) == 12

    def realign(self):
        # Realign self.notes_to_freq so that C is 0:
        adj = self.notes_to_freq[Notes["C"]]
        for i in range(len(self.notes_to_freq)):
            self.notes_to_freq[i] -= adj

    def note_to_freq(self, note_number):
        r'''Use freq_to_Hz to translate this freq to Hz.
        '''
        octave, note = divmod(note_number, 12)
        return self.notes_to_freq[note] + self.cents_per_octave * octave


def frange(inc, cents_per_octave=1200, num_elements=12):
    r'''Freq range, starting at 0.

    Freqs wrap around at cents_per_octave.
    '''
    for i in range(num_elements):
        yield (i * inc) % cents_per_octave


# Harmonics in cents:
H3 = math.log(3/2) / Ln_cent  # 701.955 cents, ideally 100.279 cents/semitone, off by 0.279
H5 = math.log(5/4) / Ln_cent  # 386.314 cents, ideally 96.579 cents/semitone, off by 3.421
H7 = math.log(7/4) / Ln_cent  # 968.826 cents, ideally 96.883 cents/semitone, off by 3.117
H11 = math.log(11/8) / Ln_cent # 551.318 cents

# Overall errors at 100 cents/semitone:
#
#    H2 = 0
#    H3 = 1.955
#    H5 = 13.686
#    H7 = 31.174

# Avg of lowest to highest is 98.429 cents/semi, off by 1.571
#
# But if we look at the overall error, H5 is 4*cent_err while H3 is 7*cent_err.  So
# splitting the overall error down the middle gives (100-e)*4 - H5 == H3 - (100-e)*7
#   400 - 4e - H5 == H3 - 700 + 7e
#   1100 - H5 == H3 + 11e
#   1100 - (H5 + H3) == 11e
#   1100 - 1088.269 == 11e
#   e = 1.066, so semitone would be 98.9335 cents giving an overall error of 9.42 for H3
#                 and H5, but this gives an overall error of 12.798 for the octave!
#
# Trying again...
#   (100-e)*4 - H5 == 1200 - (100-e)*12
#   400 - 4e - H5 == 1200 - 1200 + 12e
#   400 - H5 == 16e
#   e = 0.855375, so semitone would be 99.145 giving overall errors:
#
#      H2: 10.26
#      H3: 7.94
#      H5: 10.27
#      H7: 22.624
#
# Targeting H7, rather than H5:
#   (100-e)*10 - H7 == 1200 - (100-e)*12
#   1000 - 10e - H7 == 1200 - 1200 + 12e
#   1000 - H7 = 22e
#   e = 1.417, so semitone would 98.583 giving overall errors:
#
#      H2: 17.004
#      H3: 8.018
#      H5: 11.874
#      H7: 17.004
#
# So the smallest sensible semitone size is 98.583 which splits the overall error between
# H2 and H7.  The largest semitone size is 100.279, which is Pythagorean tuning.  Note that
# 1/4 comma Meantone has a 94.625 cent semitone, and 1/5 comma is 95.7 cents.  The 98.583
# would be 0.0659 pct_comma_fudge Meantone.


# Pythagorian comma, how far 12 H3's miss an octave:
P_comma = math.log((3/2)**12/2**7) / Ln_cent  # 531441/524288 == 1.013643265, or 23.46 cents

# How far 4 H3's miss H5.
Syntonic_comma = math.log((3/2)**4/5) / Ln_cent # 81/80 == 1.0125, or 21.5 cents

# How far 10 H3's miss H7.
H7_comma = math.log((3/2)**10/7/2**3) / Ln_cent # 59049/57344 == 1.02973284, or 50.724 cents
#print("H7_comma", H7_comma)


class H3_tuning(Base_tuning_system):
    r'''These are the family of tuning_systems that depend on a cycle of H3 [- delta]
    freqs.  These start at the tonic and go in H3_order.
    '''
    H3_order = ("C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#")

    def init(self, tonic_start, delta=0, cents_per_octave=1200):
        r'''`start` is the offset to "C".
        '''
        freqs = sorted(frange(H3 - delta, cents_per_octave))
        self.cents_per_octave = cents_per_octave
        self.to_tonic(freqs, tonic_start)


class Meantone(H3_tuning):
    r'''Meantone tuning (commonly 1/4 comma, but 1/5 comma is also supported here).

    Tonic_offsets of 3 or 8 work best for both 1/4 and 1/5 commma.

    This narrows the 5th (H3) to eliminate (or reduce) the syntonic comma.
    A pct_comma_fudge of 0 produces the Pythagorean tuning (but with a different
    interpretation of tonic).  A common meantone tuning is 1/4 comma
    (pct_comma_fudge) which completely eliminates the Syntonic comma by narrowing (fudging)
    each of the 5ths by 1/4 of a syntonic comma.

    The syntonic comma is the difference between the Pythagorian tuning for E (based on
    the 3rd harmonic applied 4 times, or 3**4 == 81) and the pure 5th harmonic
    (5 * 16 == 80 -- times 16 to get it into the same octave).

    WARNING: using pct_comma_fudge of 0.25 with max_octave_fudge > 10 may give very wonky
    results!  Seems OK with pct_comma_fudge of 0.20.
    '''

    def __init__(self, tonic="C", pct_comma_fudge=0.25, tonic_offset=3, max_octave_fudge=0):
        r'''
        Octave at pct_comma_fudge of 0.20 is 1171.845 cents, off by 28.155 cents.
        Octave at pct_comma_fudge of 0.25 is 1158.941 cents, off by 41.059 cents.
        '''
        delta = Syntonic_comma * pct_comma_fudge
        times_12 = (H3 - delta) * 12 % 1200
        octave_comma = min(abs(times_12), abs(times_12 - 1200))
        self.init((Notes[tonic] + tonic_offset) % 12,
                  delta,
                  1200 - math.copysign(min(octave_comma, max_octave_fudge), times_12 - 600))
        #print(f"Meantone.__init__, {self.notes_to_freq=}")


class Pythagorean_tuning(H3_tuning):
    r'''Pythagorean tuning.  Did anybody ever actually use this???

    Start at tonic - 3 semitones (e.g., if tonic is 'C', start at 'A').

    This jumps by H3, but is wrapped to the "octave", which may be greater than 1200 cents
    (if max_octave_fudge > 0).
    '''
    def __init__(self, tonic="C", max_octave_fudge=0):
        self.init((Notes[tonic] - 3) % 12,
                  cents_per_octave=1200 + min(P_comma, max_octave_fudge))
        #print(f"Pythagorean_tuning, {tonic=}, {max_octave_fudge=}, "
        #      f"freqs={format_list(self.notes_to_freq)}")


class Just_intonation(Base_tuning_system):
    r'''This has 14 different representative examples of just intonations.

    This includes 8 different 5-limit tunings, 2 7-limit and 4 17-limit tunings.

    These all start on the tonic.

    From: https://en.wikipedia.org/wiki/Five-limit_tuning#The_just_ratios
    '''
    tunings = (
        # Symmetric 5-limit:
        ("Symmetric 5-limit 1-A",
          1, 16/15,  9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3, 16/9, 15/8, 2),
        ("Symmetric 5-limit 1-B",
          1, 16/15,  9/8, 6/5, 5/4, 4/3, 64/45, 3/2, 8/5, 5/3, 16/9, 15/8, 2),
        ("Symmetric 5-limit 2-A",
          1, 16/15, 10/9, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
        ("Symmetric 5-limit 2-B",
          1, 16/15, 10/9, 6/5, 5/4, 4/3, 64/45, 3/2, 8/5, 5/3,  9/5, 15/8, 2),

        # Asymmetric 5-limit (standard and extended):
        ("Asymmetric 5-limit standard A",
          1, 16/15,  9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
        ("Asymmetric 5-limit standard B",
          1, 16/15,  9/8, 6/5, 5/4, 4/3, 64/45, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
        ("Asymmetric 5-limit extended A",
          1, 16/15,  9/8, 6/5, 5/4, 4/3, 25/18, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
        ("Asymmetric 5-limit extended B",
          1, 16/15,  9/8, 6/5, 5/4, 4/3, 36/25, 3/2, 8/5, 5/3,  9/5, 15/8, 2),

        # 7-limit:
        ("7-limit A",
          1, 15/14,  8/7, 6/5, 5/4, 4/3,   7/5, 3/2, 8/5, 5/3, 7/4, 15/8, 2),
        ("7-limit B",
          1, 15/14,  8/7, 6/5, 5/4, 4/3,  10/7, 3/2, 8/5, 5/3, 7/4, 15/8, 2),

        # 17-limit:
        ("17-limit A",
          1, 14/13,  8/7, 6/5, 5/4, 4/3,   7/5, 3/2, 8/5, 5/3, 7/4, 13/7, 2),
        ("17-limit B",
          1, 14/13,  8/7, 6/5, 5/4, 4/3,  10/7, 3/2, 8/5, 5/3, 7/4, 13/7, 2),
        ("17-limit C",
          1, 14/13,  8/7, 6/5, 5/4, 4/3, 17/12, 3/2, 8/5, 5/3, 7/4, 13/7, 2),
        ("17-limit D",
          1, 14/13,  8/7, 6/5, 5/4, 4/3, 24/17, 3/2, 8/5, 5/3, 7/4, 13/7, 2),
    )
    def __init__(self, tuning=0, tonic="C"):
        ratios = tuple(math.log(ratio) / Ln_cent for ratio in self.tunings[tuning][1:-1])
        tonic_start = Notes[tonic]
        self.to_tonic(ratios, tonic_start)


def cumsum(*numbers):
    r'''Does a % 1200 on all results.
    '''
    sum = 0
    yield 0
    for n in numbers:
        sum = (sum + n) % 1200
        yield sum

class Well_temperament(Base_tuning_system):
    r'''Well temperament as defined by Herbert Anton Kellner:

        https://www.jstor.org/stable/41640471

    Herbert starts at tonic - 4 semitones (e.g., if tonic is "C", start at "Ab").
    Alternatively, start at tonic + 1 semitone (e.g., if tonic is "C", start at "C#").

    Uses two different adjustments to pythagorean tuning on different 5ths (rather
    than using the same adjustment on all 5ths as on meantone).

    P_comma is 23.46 cents.
    P_comma/5 = 4.692 cents (between 1/4 comma (5.375 cents) and
                             1/5 comma (4.3 cents) meantone adjustment)

    H3 - P_comma/5 is applied to 5 5ths, and H3 is applied to the remaining 7 5ths.  This
    brings the octave into a perfect 8th.
    '''

    P_5 = P_comma/5    # 4.692 cents

    raw_freqs = tuple(cumsum(
                       # C:     0
                           H3 - P_5,
                       # G:     1
                           H3 - P_5,
                       # D:     2
                           H3 - P_5,
                       # A:     3
                           H3 - P_5,
                       # E:     4
                           H3,
                       # B:     5
                           H3 - P_5,
                       # F#:    6
                           H3,
                       # C#:    7
                           H3,
                       # Ab:    8
                           H3,
                       # Eb:    9
                           H3,
                       # Bb:   10
                           H3,
                       # F:    10
                           H3,
                       # C:     0
                    ))

    freqs = tuple(sorted(raw_freqs[:-1]))

    def __init__(self, tonic="C", tonic_offset=0):
        start = (Notes[tonic] + tonic_offset) % 12
        assert len(self.raw_freqs) == 13
        assert abs(self.raw_freqs[-1]) < 1e-3, \
               f"Well_temperament expected last raw_freq 1200, got {self.raw_freqs[-1]}"
        self.to_tonic(self.freqs, start)


class Equal_temperament(Base_tuning_system):
    r'''Equal_temperament has no tonic...
    '''
    def __init__(self, cents_per_semitone=100):
        self.notes_to_freq = [n * cents_per_semitone for n in range(12)]
        self.cents_per_octave = 12 * cents_per_semitone


class Tuning_system(Channel_setting):
    name = 'tuning_system'
    midi_config_number = 0x02  # FIX

    def set_midi_value(self, value):
        if value == 22:
            tuning_system = Equal_temperament()
        elif value & 0x20 == 0x00:
            tuning_system = Well_temperament(Notes[value & 0x1F])  # value up to 21
        elif value & 0x20 == 0x10:
            tuning_system = Just_temperament(Notes[value & 0x1F])  # value up to 21
        elif value & 0x40 == 0x40:
            tuning_system = Meantone((value & 0x30) / (value & 0x0F))
        self.set('tuning_system', tuning_system)
        # FIX: Add tuning to match another channel's tuning_system based on key_signature.


Channel.Settings.append(Tuning_system)



if __name__ == "__main__":
    import argparse

    cls_map = {
        "Meantone": Meantone,
        "Pythagorean_tuning": Pythagorean_tuning,
        "Just_intonation": Just_intonation,
        "Well_temperament": Well_temperament,
        "Equal_temperament": Equal_temperament,
    }

    argparser = argparse.ArgumentParser()
    argparser.add_argument("cls", choices=tuple(cls_map.keys()))
    argparser.add_argument("-c", "--pct-comma", type=float, choices=(0.25, 0.20))
    argparser.add_argument("-o", "--tonic_offset", type=int)
    argparser.add_argument("-t", "--tuning", type=int, choices=tuple(range(14)))

    args = argparser.parse_args()

    #print("args", args)

    kws = {}
    if args.pct_comma is not None:
        kws['pct_comma_fudge'] = args.pct_comma
    if args.tuning is not None:
        kws['tuning'] = args.tuning
    if args.tonic_offset is not None:
        kws['tonic_offset'] = args.tonic_offset

    print(args.cls, end='')
    if kws:
        print(',', kws)
    else:
        print()
    if args.cls != 'Equal_temperament':
        kws['tonic'] = "C"

    if False:
        for tonic in ("C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B",):
            ntf = cls_map[args.cls](tonic=tonic, **kws).notes_to_freq
            print(tonic, end=': ')
            for i in range(11):
                print(f"{ntf[i + 1] - ntf[i]:.2f}, ", end='')
            print(f"{ntf[0] + 1200 - ntf[11]:.2f}, ")
    else:
        def show_major(key, ts):
            start = nat(Notes[key], 4)
            return f"{key + ':':3} " \
              f"H3 {abs(ts.note_to_freq(start + 7) - ts.note_to_freq(start) - H3):5.2f}, " \
              f"H5 {abs(ts.note_to_freq(start + 4) - ts.note_to_freq(start) - H5):5.2f}, " \
              f"H7 {abs(ts.note_to_freq(start + 10) - ts.note_to_freq(start) - H7):5.2f}"
        def show_minor(key, ts):
            end = nat(Notes[key], 4) + 7
            return f"{key + ':':3} " \
              f"H3 {abs(ts.note_to_freq(end) - ts.note_to_freq(end-7) - H3):5.2f}, " \
              f"H5 {abs(ts.note_to_freq(end) - ts.note_to_freq(end-4) - H5):5.2f}, " \
              f"H7 {abs(ts.note_to_freq(end) - ts.note_to_freq(end-10) - H7):5.2f}"
        ts = cls_map[args.cls](**kws)
        notes = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"]
        notes_doubled = notes + [n + '2' for n in notes]
        #print("notes", notes)
        #print("notes_doubled", notes_doubled)

        major_keys = ("C", "F", "G", "Bb", "D", "Eb", "A", "Ab", "E", "C#", "B", "F#")
        #print(major_keys, sorted(major_keys))
        print()
        print("majors:                 ", end='')
        for i, key in enumerate(major_keys):
            #print("major", key, notes_doubled[Notes[key]:Notes[key]+11])
            if i % 2:
                print('         ', show_major(key, ts), end='')
            else:
                print('   ', show_major(key, ts))
        print()

        minor_keys = ("A", "D", "E", "G", "B", "C", "F#", "F", "C#", "Bb", "Ab", "Eb")
        #print(minor_keys, sorted(minor_keys))
        print()
        print("minors:                 ", end='')
        for i, key in enumerate(minor_keys):
            #print("minor", key, last_note, notes_doubled[last_note - 11: last_note])
            if i % 2:
                print('         ', show_minor(key, ts), end='')
            else:
                print('   ', show_minor(key, ts))
        print()

