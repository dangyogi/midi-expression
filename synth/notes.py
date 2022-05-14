# notes.py

r'''
Notes are numbered by MIDI as going from 0 to 127 with 12 notes/octave.  Note 0 is C-1
(8.18 Hz).  Note 12 is C0 (16.32 Hz), 60 is C4, 127 is G9.

    MIDI: 0-127, 12 notes/octave, 0 is C-1 (8.18 Hz, lowest MIDI note),
    12 is C0 (16.32 Hz), 21 is A0 (27.5 Hz, lowest piano key),
    60 is C4 (261.63 Hz, 69 is A4 (440 Hz, the "A" for "A-440"),
    108 is C8 (4186 Hz, highest piano key), 127 is G9 (12,544 Hz, highest MIDI note).

Notes are translated into my freqs using a Tuning_system.
'''


# MIDI note numbers for one octave.
Notes = {  # Note numbers offset from C (12 notes/octave).
    "C":   0,
    "C#":  1,
    "Db":  1,
    "D":   2,
    "D#":  3,
    "Eb":  3,
    "E":   4,
    "E#":  5,
    "Fb":  4,
    "F":   5,
    "F#":  6,
    "Gb":  6,
    "G":   7,
    "G#":  8,
    "Ab":  8,
    "A":   9,
    "A#": 10,
    "Bb": 10,
    "B":  11,
    "B#":  0,
    "Cb": 11,
}

# Sorted by note number
Note_names = tuple(k for k, v in sorted(Notes.items(), key=lambda i: i[1]))

#print(Note_names)

def nat(base_note, octave):
    r'''Lookup base_note in Notes.  C4 is middle C.
    '''
    return 12 * (octave + 1) + base_note

def sharp(base_note, octave):
    return 12 * (octave + 1) + (base_note + 1)

def flat(base_note, octave):
    return 12 * (octave + 1) + (base_note - 1)


Fifths = "FCGDAEB"
Flats = tuple(n + 'b' for n in Fifths)
Sharps = tuple(n + '#' for n in Fifths)
Circle_of_fifths = Flats + tuple(Fifths) + Sharps

class Key_signature:
    # Keyboard reference:
    #    #   b    #   b? b
    #   C  D  E  F  G  A  B  C
    # translate = (0, 1, 3, 5, 6, 9, 10, 12, 14, 15, 17, 18)

    def __init__(self, sf=0, minor=False):
        self.init(sf, minor)

    def set_midi_value(self, value):
        r'''value is combination of minor (bit 0x10) and sf (bits 0x0f as signed int).
        '''
        minor = bool(value & 0x10)
        if value & 0x08:
            sf = -(value & 0x07)
        else:
            sf = value & 0x07
        self.init(sf, minor)

    def init(self, sf=0, minor=False):
        r'''Positive sf for sharps, negative for flats.

        Thus, sf can go from -7 (7 flats) to 7 (7 sharps).

        self.translate is indexed by the MIDI base note number (0-11, see MIDI_notes).
        '''
        self.sf = sf
        self.minor = bool(minor)
        self.tonic = Circle_of_fifths[sf + 8]  # FIX: Does minor flag figure into this??
        print(f"{sf=}, {self.tonic=}")
        translate = [None] * 12
        if -2 <= sf <= 3:
            # Doesn't include Cb, Fb, E# or B#, no adjustment necessary
            notes = Circle_of_fifths[sf + 4: sf + 16]
        elif -6 < sf < -2:
            # exclude Cb, Fb, as they are not in the signature
            notes = Circle_of_fifths[2: 14]
        elif sf <= -6:
            # include Cb (and Fb), as they are in the signature
            notes = Circle_of_fifths[sf + 7: sf + 19]
        elif 3 < sf <= 5:
            # exclude E#, B#, as they are not in the signature
            notes = Circle_of_fifths[-14:-2]
        else:  # sf > 5
            # include E# (and B#), as they are in the signature
            notes = Circle_of_fifths[sf + 2: sf + 14]
        print(f"{sf=}, {notes=}")
        for note in notes:
            #print(f"doing {note=}, {Notes[note]=}")
            assert translate[Notes[note]] is None
            translate[Notes[note]] = note

        #print("translate", translate)
        for i, note in enumerate(translate):
            assert note is not None, f"translate[{i}] not set"

        self.translate = tuple(translate)

    def translation(self):
        return self.translate

    def note_name(self, note_number):
        octave, note = divmod(note_number, 12)
        return f"{self.translate[note]}{octave - 1}"



if __name__ == "__main__":
    print(f"{Circle_of_fifths=}")
    #print(f"{Circle_of_fifths[4: 16]=}")
    for sf in range(-7, 8):
        ks = Key_signature(sf, False)
        print(sf, ks.translation())
        print(ks.note_name(68))
