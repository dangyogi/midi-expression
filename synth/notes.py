# notes.py

r'''
Notes are numbered in two ways:

    MIDI: 0-127, 12 notes/octave, 0 is C-1, 12 is C0, 60 is C4, 127 is G9.

    My notes: 0-230, 21 notes/octave counting the natural, sharp and flat of
              each diatonic note.  1 is C-1 (8.18Hz), 230 is B#10 (31,609Hz).
              (E10 is 21,096Hz).

MIDI notes are converted to my notes by a Key_signature.

My notes are translated into my freqs using a Tuning_system.
'''


# My Notes.  These differentiate between sharps and flats (e.g., C# and Db).  There
# are 7 main notes (C D E F G A B) that each have a sharp and a flat.  That gives 21 notes
# total.  For these notes, each octave starts at Cb so that Cb4 is right next to C4 and the
# octave breaks going from B# to Cb.  This also means any main note - 1 is it's flat, and
# + 1 is it's sharp.
Notes = {  # Note numbers offset from Cb (21 notes/octave).
    "Cb":  0,
    "C":   1,
    "C#":  2,
    "Db":  3,
    "D":   4,
    "D#":  5,
    "Eb":  6,
    "E":   7,
    "E#":  8,
    "Fb":  9,
    "F":  10,
    "F#": 11,
    "Gb": 12,
    "G":  13,
    "G#": 14,
    "Ab": 15,
    "A":  16,
    "A#": 17,
    "Bb": 18,
    "B":  19,
    "B#": 20,
}

Note_names = tuple(k for k, v in sorted(Notes.items(), key=lambda i: i[1]))

print(Note_names)

def nat(base_note, octave):
    r'''Lookup base_note in Notes.  C4 is middle C.
    '''
    return 21 * (octave + 1) + base_note

def sharp(base_note, octave):
    r'''This will sharp any note, including notes already sharped.
    '''
    if base_note % 3 == 2:    # a sharp already
        if base_note in (8, 20):  # E# (almost F) or B# (almost C)
            # E# -> F#, B# -> C#
            return 21 * (octave + 1) + (base_note + 3)
        # ... up to next natural
        return 21 * (octave + 1) + (base_note + 2)
    # else it's a flat or a natural
    return 21 * (octave + 1) + (base_note + 1)

def flat(base_note, octave):
    r'''This will flat any note, including notes already flatted.
    '''
    if base_note % 3 == 0:      # a flat already
        if base_note in (0, 9):  # Cb (almost B) or  Fb (almost E)
            # Cb -> Bb, Fb -> Eb
            return 21 * (octave + 1) + (base_note - 3)
        # ... down to next natural
        return 21 * (octave + 1) + (base_note - 2)
    # else it's a sharp or a natural
    return 21 * (octave + 1) + (base_note - 1)


# MIDI Notes go from 0 = C-1 to 127 = G9
MIDI_notes = {  # for one octave
    "C": 0,
    "C#": 1,
    "Db": 1,
    "D": 2,
    "D#": 3,
    "Eb": 3,
    "E": 4,
    "F": 5,
    "F#": 6,
    "Gb": 6,
    "G": 7,
    "G#": 8,
    "Ab": 8,
    "A": 9,
    "A#": 10,
    "Bb": 10,
    "B": 11,
}

Fifths = "FCGDAEB"
Circle_of_fifths = tuple(n + 'b' for n in Fifths) \
                 + tuple(Fifths) \
                 + tuple(n + '#' for n in Fifths)

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
        self.minor = bool(minor)
        translate = [None] * 12

        # Set default values.  Black keys are a crap shoot here...
        for note, midi_index in MIDI_notes.items():
            translate[midi_index] = Notes[note]

        # sharps:
        if sf > -2:
            for note in Fifths[:2+min(sf, 5)]:
                print("sharping", note,
                      "setting", (MIDI_notes[note] + 1) % 12, "to", Notes[note] + 1)
                translate[(MIDI_notes[note] + 1) % 12] = Notes[note] + 1
        # flats:
        if sf < 3:
            for note in Fifths[4+max(sf, -4):]:
                print("flatting", note,
                      "setting", (MIDI_notes[note] - 1) % 12, "to", Notes[note] - 1)
                translate[(MIDI_notes[note] - 1) % 12] = Notes[note] - 1
        self.translate = translate

    def translation(self):
        return tuple(Note_names[t] for t in self.translate)

    def MIDI_to_note(self, midi_note):
        octave, note = divmod(midi_note, 12)
        return 21 * octave + self.translate[note]

