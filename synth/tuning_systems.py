# tuning_systems.py

r'''
My notes: 0-230, 21 notes/octave counting the natural, sharp and flat of
          each diatonic note.  1 is C-1 (8.18Hz), 230 is B#10 (31,609Hz).
          (E10 is 21,096Hz).

My notes are translated into my freqs using a Tuning_system.note_to_freq(my_note).

My freqs are integers representing the number of (equal_temperment) cents in an interval.
Thus, an octave is 1200, a major 5th is 700, a major 3rd 400 (in equal_temperment).

My freqs are also used to represent absolute freqs by assigning freq 0 as Cb-1 in equal
temperament.  In this case, since the note values cover 11 octaves, this is 13,200 freqs.
These absolute freqs can be converted to Hz with freq_to_Hz.
'''

import math
import numpy as np

from .notes import Notes, nat
from .channel import Channel, Channel_setting



# Internal intervals are based on cents in log base cent
Cent = math.pow(2, 1/1200)  # 1.0005777895065548
Ln_cent = math.log(Cent)


# Notes and intervals:
def to_freq(expanded_interval, delta=0):
    return round((math.log(expanded_interval) / Ln_cent + delta) % 1200)


C4 = 1200 * 4
A4 = C4 + 900
A4_freq = 440
C4_freq = A4_freq / Cent**900   # 261.6256
C0_freq = A4_freq / Cent**A4    # 16.3516
print(f"{C4_freq=}")
print(f"{C0_freq=}")
Freq_offset = math.log(C0_freq) / Ln_cent - 100 - 1200  # Offset to Cb-1
print(f"{Freq_offset=}")

def freq_to_Hz(freq):
    r'''Translate internal absolute freq value to Hz.
    '''
    return np.exp((freq + Freq_offset) * Ln_cent)


class Base_tuning_system:
    r'''Converts notes to freqs.  Cb is 0.
    '''
    # Keyboard reference:
    #    # #   # # #
    #   C D E F G A B C
    def realign(self):
        # Realign self.notes_to_freq so that Cb is 0:
        adj = self.notes_to_freq[Notes["Cb"]]
        for note in self.notes_to_freq.keys():
            self.notes_to_freq[note] -= adj

    def note_to_freq(self, note):
        r'''Use freq_to_Hz to translate this freq to Hz.
        '''
        return self.notes_to_freq[note % 21] + 1200 * (note // 21)

class Meantone(Base_tuning_system):
    r'''This encompasses all 3-limit tunings with fudges applied to eliminate (or reduce)
    the syntonic comma.  A comma_fudge of 0 produces the Pythagorean tuning.  A common
    meantone tuning is 1/4 comma (comma_fudge) which completely eliminates the Syntonic
    comma by narrowing (fudging) each of the 5ths by 1/4 of a syntonic comma.

    The syntonic comma is the difference between the Pythagorian tuning for E (based on
    the 3rd harmonic applied 4 times, or 3**4 == 81) and the pure 5th harmonic
    (5 * 16 == 80, times 16 to get it into the same octave).
    '''
    Syntonic_comma = math.log(81/80) / Ln_cent

    def __init__(self, comma_fudge=0):
        self.notes_to_freq = {
            Notes[note]: to_freq(3**n, n*(self.Syntonic_comma * comma_fudge))
            for n, note in enumerate(("Fb", "Cb", "Gb", "Db", "Ab", "Eb", "Bb",
                                      "F", "C", "G", "D", "A", "E", "B",
                                      "F#", "C#", "G#", "D#", "A#", "E#", "B#"),
                                     -1)
        }
        # Get the B notes in the right octave, even if they go past 1200!
        for b_check in ("Bb", "B", "B#"):
            if self.notes_to_freq[Notes[b_check]] < 1000:
                self.notes_to_freq[Notes[b_check]] += 1200

class Pythagorean_tuning(Base_tuning_system):
    r'''OBSOLETE: Use Meantone(0) instead.

    Sharps move up, flats move down.

    Num Sharps: (max 7 = C# Major)
             Major/Minor
       None:   C  / A
          1:   G  / E    # F#
          2:   D  / B    # F#, C#
          3:   A  / F#   # F#, C#, G#
          4:   E  / C#   # F#, C#, G#, D#
          5:   B  / G#   # F#, C#, G#, D#, A#
          6:   F# / D#   # F#, C#, G#, D#, A#, E#
          7:   C# / A#   # F#, C#, G#, D#, A#, E#, B#

    Num Flats: (max 7 = Cb Major)
             Major/Minor
       None:   C  / A
          1:   F  / D    # Bb
          2:   Bb / G    # Bb, Eb
          3:   Eb / C    # Bb, Eb, Ab
          4:   Ab / F    # Bb, Eb, Ab, Db
          5:   Db / Bb   # Bb, Eb, Ab, Db, Gb
          6:   Gb / Eb   # Bb, Eb, Ab, Db, Gb, Cb
          7:   Cb / Ab   # Bb, Eb, Ab, Db, Gb, Cb, Fb
    '''
    notes_to_freq = {
        Notes["Fb"]: to_freq(3**-1),    # E
        Notes["Cb"]: to_freq(3**0),     # B
        Notes["Gb"]: to_freq(3**1),     # F#
        Notes["Db"]: to_freq(3**2),     # C#
        Notes["Ab"]: to_freq(3**3),     # G#
        Notes["Eb"]: to_freq(3**4),     # D#
        Notes["Bb"]: to_freq(3**5),     # A#
        Notes["F"]: to_freq(3**6),
        Notes["C"]: to_freq(3**7),
        Notes["G"]: to_freq(3**8),
        Notes["D"]: to_freq(3**9),
        Notes["A"]: to_freq(3**10),
        Notes["E"]: to_freq(3**11),
        Notes["B"]: to_freq(3**12),
        Notes["F#"]: to_freq(3**13),    # Gb
        Notes["C#"]: to_freq(3**14),    # Db
        Notes["G#"]: to_freq(3**15),    # Ab
        Notes["D#"]: to_freq(3**16),    # Eb
        Notes["A#"]: to_freq(3**17),    # Bb
        Notes["E#"]: to_freq(3**18),    # F
        Notes["B#"]: to_freq(3**19),    # C
    }

def grouper(seq, n):
    r'''Yield n-len sequences from t.

    grouper('ABCDEFG', 3) --> ABC DEF G

    If n is < 0, yields from the end of seq, working forward.

    grouper('ABCDEFG', -3) --> EFG BCD A
    '''
    if n > 0:
        start = 0
    else:
        start = len(seq) + n
    while 0 <= start < len(seq):
        yield seq[start: start + abs(n)]
        start += n
    if start < 0:
        yield seq[: start + abs(n)]

class Just_intonation(Base_tuning_system):
    r'''5-limit just intonation centered on tonic.
    '''
    def __init__(self, tonic="C"):
        # Load self.notes_to_freq:
        start = Circle_of_fifths.index(tonic)
        self.notes_to_freq = {}
        def go_up():
            for n5, group in enumerate(grouper(Circle_of_fifths[start + 3:], 4), 1):
                for n3, note in enumerate(group, -1):
                    self.notes_to_freq[Notes[note]] = to_freq(3**n3 * 5**n5)
        def go_down():
            for n5, group in enumerate(grouper(Circle_of_fifths[:start - 2], 4), 1):
                for n3, note in enumerate(group, -2):
                    self.notes_to_freq[Notes[note]] = to_freq(3**n3 * 5**-n5)
        if start <= 2:
            for n3, note in enumerate(Circle_of_fifths[:5-start], -2 + start):
                self.notes_to_freq[Notes[note]] = to_freq(3**n3)
            go_up()
        elif start >= len(Circle_of_fifths) - 3:
            for n, note in enumerate(Circle_of_fifths[start - 2:], -2):
                self.notes_to_freq[Notes[note]] = to_freq(3**n)
            go_down()
        else:
            for n, note in enumerate(Circle_of_fifths[start - 2: start + 3], -2):
                self.notes_to_freq[Notes[note]] = to_freq(3**n)
            go_up()
            go_down()
        self.realign()

class Well_temperament(Base_tuning_system):
    r'''Well temperament as defined by Herbert Anton Kellner:

        https://www.jstor.org/stable/41640471

    This is for keyboarded instruments, so only has 12 notes (e.g., no difference
    between A# and Bb).

    Circle of Fifths:
      P = Pythagorian Comma, 3**12/2**19 (531441/524288) or 1.013643265, or 23.46 cents.

      Adjustments subtracted from Pythagorian tuning for each interval.

        Ab:   -4
           0
        Eb:   -3
           0
        Bb:   -2
           0
        F:    -1
           0
        C:     0
           P/5
        G:     1
           P/5
        D:     2
           P/5
        A:     3
           P/5
        E:     4
           0
        B:     5
           P/5
        F#:    6
           0
        C#:    7
           0
        Ab:    8



    ================== FIX: cut from here down ================
    FAKE!!  This is really just a 1/5-comma Meantone tuning...

    Well tempered counting the sharps and flats differently (A# != Bb).

    Sharps and flats:

        This ends up with all of the flats and sharps being offset by 84 cents
        from their base note.  (This also applies to Fb and B#, but see below).

    Gaps between "same" notes:

        Also, two notes that would be the same on a keyboard (e.g., A#/Bb, or E#/F) are all
        offset by a 28 cent (1/3 of 84) gap.  This also applies to E/Fb and B#/C (as well as
        E#/F and B/Cb).

    But this gets confusing with Fb.  While Fb to F is 84 cents, and E to E# is 84 cents.
    But E# to F is 28 cents, as is E to Fb.  So it's E - E# - F (omitting Fb), for an 84
    jump and a 28 jump, or E - Fb - F, for a 28 jump and an 84 jump (omitting E#).

    Same thing happens with B#.  B to B# is 84 cents, as is Cb to C.  And B# to C is 28
    cents, as is B to Cb.  So it's B - B# - C (omitting Cb), for an 84 jump and a 28 jump,
    or B - Cb - C (omitting B#) for a 28 jump and an 84 jump.

    Thus, there are two notes that really only have one accidental.  Since Fb and B# aren't
    used in the 15 key signatures (C major + 1-7 sharps + 1-7 flats).  It seems to make
    sense to not count those two.

    So the 7 notes (A-G) each have a sharp and a flat, minus Fb and B#, so that's 12 * 84,
    or 1008 cents.  Plus 7 * 28 for the gaps, or 196 cents.  Is 1204 cents, or just over an
    octave.  That's because the 28 cents is really 27.907 cents, and the 84 cents is really
    83.721 cents.
    
    A perfect fifth is 701.955 cents.

    A perfect third is 386.314 cents.

    These are each offset by 4.3013 cents in this temperament.  So the fifths become 697.654
    while the thirds become 390.615.

    You could also do this by using the 43rd root of 2 as a "slice", then using 3 slices
    where it says 84 cents above, and 1 slice where it says 28 cents.  Then whole steps
    (e.g., C to D) are 7 slices, while half-steps (e.g., B to C) are 4 slices.  Like so:

         Cb    C#      Eb    E#   Gb    G#      Bb    B#
         |     |       |     |    |     |       |     |
        B...C......D......E...F......G......A......B...C... ...
           |    |     |    |     |       |     |    |     |
           B#   Db    D#   Fb    F#      Ab    A#   Cb    C#

    Note that C to Db and C# to D are half-steps, while C# to D# and Cb to Db are whole
    steps.

    notes_to_freq = {
        Notes["Cb"]: -84 + 84,
        Notes["C"]: 0 + 84,
        Notes["C#"]: 84 + 84,
        Notes["Db"]: 112 + 84,
        Notes["D"]: 195 + 84,
        Notes["D#"]: 279 + 84,
        Notes["Eb"]: 307 + 84,
        Notes["E"]: 391 + 84,
        Notes["E#"]: 474 + 84,
        Notes["Fb"]: 419 + 84,   # never used??
        Notes["F"]: 502 + 84,
        Notes["F#"]: 586 + 84,
        Notes["Gb"]: 614 + 84,
        Notes["G"]: 698 + 84,
        Notes["G#"]: 781 + 84,
        Notes["Ab"]: 809 + 84,
        Notes["A"]: 893 + 84,
        Notes["A#"]: 977 + 84,
        Notes["Bb"]: 1005 + 84,
        Notes["B"]: 1088 + 84,
        Notes["B#"]: 1172 + 84,  # never used??
    }
    '''

    # Pythagorian comma
    P = math.log(3**12/2**19) / Ln_cent  # (531441/524288) or 1.013643265, or 23.46 cents.

    fudge = P/5
    fifth = math.log(3/2) / Ln_cent

    def __init__(self, tonic="C"):
        start = Circle_of_fifths.index(tonic)
        if start > len(Circle_of_fifths) - 8:
            start = len(Circle_of_fifths) - 8
        if start < 4:  
            start = 4
        self.notes_to_freq = {}
        for n, note in enumerate(Circle_of_fifths[start - 4: start + 8], -4):
            if n in (0, 1, 2, 3, 5):
                self.notes_to_freq[Notes[note]] = to_freq(3**n, -self.fudge)
            else:
                self.notes_to_freq[Notes[note]] = to_freq(3**n)
        self.realign()


class Equal_temperament(Base_tuning_system):
    notes_to_freq = {
        Notes["Cb"]: 0,
        Notes["C"]: 100,
        Notes["C#"]: 200,
        Notes["Db"]: 200,
        Notes["D"]: 300,
        Notes["D#"]: 400,
        Notes["Eb"]: 400,
        Notes["E"]: 500,
        Notes["E#"]: 600,
        Notes["Fb"]: 500,   # never used??
        Notes["F"]: 600,
        Notes["F#"]: 700,
        Notes["Gb"]: 700,
        Notes["G"]: 800,
        Notes["G#"]: 900,
        Notes["Ab"]: 900,
        Notes["A"]: 1000,
        Notes["A#"]: 1100,
        Notes["Bb"]: 1100,
        Notes["B"]: 1200,   # keep the "B" notes in the same octave...
        Notes["B#"]: 1300,  # never used??
    }


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
