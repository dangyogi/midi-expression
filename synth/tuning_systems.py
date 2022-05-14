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


class Base_tuning_system:
    r'''Converts notes to freqs.  C is 0.

    Subclass sets self.note_to_freq to a list of freqs, indexed by note.
    '''
    # Keyboard reference:
    #    # #   # # #
    #   C D E F G A B C
    def to_tonic(self, freqs, tonic_start):
        self.notes_to_freq = [f - freqs[tonic_start] for f in freqs[tonic_start:]] \
                             + [f + 1200 - freqs[tonic_start] for f in freqs[:tonic_start]]

    def realign(self):
        # Realign self.notes_to_freq so that C is 0:
        adj = self.notes_to_freq[Notes["C"]]
        for i in range(len(self.notes_to_freq)):
            self.notes_to_freq[i] -= adj

    def note_to_freq(self, note_number):
        r'''Use freq_to_Hz to translate this freq to Hz.
        '''
        octave, note = divmod(note_number, 12)
        return self.notes_to_freq[note] + 1200 * octave


def frange(inc, num_elements=12):
    r'''Freq range, starting at 0.

    Freqs wrap around at 1200.
    '''
    for i in range(num_elements):
        yield (i * inc) % 1200


H3 = math.log(3/2) / Ln_cent                # 701.955 cents

class H3_tuning(Base_tuning_system):
    r'''These are the family of tuning_systems that depend on a cycle of H3 [- delta]
    freqs.
    '''
    def __init__(self, tonic_start, delta=0):
        r'''`start` is the offset to "C".
        '''
        freqs = sorted(frange(H3 - delta))
        self.to_tonic(freqs, tonic_start)


class Meantone(H3_tuning):
    r'''Meantone tuning (commonly 1/4 comma, but 1/5 comma is also supported here).

    Start at tonic - 4 semitones (e.g., if tonic is "C", start at "Ab").
    Alternative is to start at tonic + 1 semitone (e.g., if tonic is "C", start at "C#").
    Starting at tonic - 7 semitones, e.g., if tonic is "C", start at "F", gives nearly
    perfect 7th at Bb.

    This encompasses all 3-limit tunings with fudges applied to eliminate (or reduce)
    the syntonic comma.  A pct_comma_fudge of 0 produces the Pythagorean tuning (but with a
    different interpretation of tonic).  A common meantone tuning is 1/4 comma
    (pct_comma_fudge) which completely eliminates the Syntonic comma by narrowing (fudging)
    each of the 5ths by 1/4 of a syntonic comma.

    The syntonic comma is the difference between the Pythagorian tuning for E (based on
    the 3rd harmonic applied 4 times, or 3**4 == 81) and the pure 5th harmonic
    (5 * 16 == 80 -- times 16 to get it into the same octave).
    '''
    Syntonic_comma = math.log(81/80) / Ln_cent  # 21.5 cents
    tonic_offset = 7                            # start with tonic - 7 (Ab for tonic "C")

    def __init__(self, tonic="C", pct_comma_fudge=0.25, tonic_offset=7):
        super().__init__((Notes[tonic] + tonic_offset) % 12,
                         self.Syntonic_comma * pct_comma_fudge)
        #print(f"Meantone.__init__, {self.notes_to_freq=}")


class Pythagorean_tuning(H3_tuning):
    r'''Pythagorean tuning.  Did anybody ever actually use this???

    Start at tonic - 3 semitones (e.g., if tonic is 'C', start at 'A').
    '''
    def __init__(self, tonic="C"):
        super().__init__((Notes[tonic] + 3) % 12)


class Just_intonation(Base_tuning_system):
    r'''This has 7 different representative examples of just intonations.

    This includes 4 different 5-limit tunings, and one 7-limit and
    two 17-limit tunings.

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
        sum += n
        yield sum % 1200

class Well_temperament(Base_tuning_system):
    r'''Well temperament as defined by Herbert Anton Kellner:

        https://www.jstor.org/stable/41640471

    Herbert starts at tonic - 4 semitones (e.g., if tonic is "C", start at "Ab").
    Alternatively, start at tonic + 1 semitone (e.g., if tonic is "C", start at "C#").

    This is for keyboard instruments, so only has 12 notes (e.g., no difference
    between A# and Bb).

    Uses two different adjustments to pythagorean tuning on different 5ths (rather
    than using the same adjustment on all 5ths as on meantone).

    Circle of Fifths:
      P = Pythagorian Comma, 3**12/2**19 (531441/524288) or 1.013643265, or 23.46 cents.
      P/5 = 4.692 cents (between 1/4 comma (5.375 cents) and 1/5 comma (4.3 cents) meantone
      adjustment)

      Adjustments subtracted from Pythagorian tuning for each interval:

    '''

    # Pythagorian comma
    P = math.log(3**12/2**19) / Ln_cent  # (531441/524288) or 1.013643265, or 23.46 cents.
    P_5 = P/5

    freqs = tuple(sorted(cumsum(
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
                           #   H3,
                           # C:     0
                        )))

    def __init__(self, tonic="C"):
        start = Notes[tonic]
        self.to_tonic(self.freqs, start)


class Equal_temperament(Base_tuning_system):
    r'''Equal_temperament has no tonic...
    '''
    def __init__(self):
        self.notes_to_freq = [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100]


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
    argparser.add_argument("-t", "--tuning", type=int, choices=tuple(range(14)))

    args = argparser.parse_args()

    #print("args", args)

    kws = {}
    if args.pct_comma is not None:
        kws['pct_comma_fudge'] = args.pct_comma
    if args.tuning is not None:
        kws['tuning'] = args.tuning

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
        H3 = math.log(3/2) / Ln_cent                # 701.955 cents
        H5 = math.log(5/4) / Ln_cent                # 386.314 cents
        H7 = math.log(7/4) / Ln_cent                # 968.826 cents
        def show_major(key, freqs):
            return f"{key + ':':3} " \
                      f"H3 {abs(freqs[7] - freqs[0] - H3):5.2f}, " \
                      f"H5 {abs(freqs[4] - freqs[0] - H5):5.2f}, " \
                      f"H7 {abs(freqs[10] - freqs[0] - H7):5.2f}"
        def show_minor(key, freqs):
            return f"{key + ':':3} " \
                      f"H3 {abs(freqs[-1] - freqs[-8] - H3):5.2f}, " \
                      f"H5 {abs(freqs[-1] - freqs[-5] - H5):5.2f}, " \
                      f"H7 {abs(freqs[-1] - freqs[-11] - H7):5.2f}"
        ntf = cls_map[args.cls](**kws).notes_to_freq
        #print("ntf", ntf)
        ntf_doubled = ntf + [freq + 1200 for freq in ntf]
        #print("ntf_doubled", ntf_doubled)
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
            freqs = ntf_doubled[Notes[key]:Notes[key]+11]
            assert len(freqs) == 11
            if i % 2:
                print('         ', show_major(key, freqs), end='')
            else:
                print('   ', show_major(key, freqs))
        print()

        minor_keys = ("A", "D", "E", "G", "B", "C", "F#", "F", "C#", "Bb", "Ab", "Eb")
        #print(minor_keys, sorted(minor_keys))
        print()
        print("minors:                 ", end='')
        for i, key in enumerate(minor_keys):
            last_note = Notes[key] + 8
            if last_note <= 12:
                last_note += 12
            #print("minor", key, last_note, notes_doubled[last_note - 11: last_note])
            freqs = ntf_doubled[last_note - 11: last_note]
            assert len(freqs) == 11
            if i % 2:
                print('         ', show_minor(key, freqs), end='')
            else:
                print('   ', show_minor(key, freqs))
        print()

