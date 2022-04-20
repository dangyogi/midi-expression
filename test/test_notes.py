# test_notes.py

import pytest

from synth.notes import *


@pytest.mark.parametrize("base_note, octave, answer", (
    ("Cb", -1, 0),
    ("C", -1, 1),
    ("C", 0, 22),
    ("C", 4, 106),
    ("C#", 4, 107),
    ("Db", 4, 108),
    ("D", 4, 109),
    ("D#", 4, 110),
    ("Eb", 4, 111),
    ("E", 4, 112),
    ("E#", 4, 113),
    ("Fb", 4, 114),
    ("F", 4, 115),
    ("F#", 4, 116),
    ("Gb", 4, 117),
    ("G", 4, 118),
    ("G#", 4, 119),
    ("Ab", 4, 120),
    ("A", 4, 121),
    ("A#", 4, 122),
    ("Bb", 4, 123),
    ("B", 4, 124),
    ("B#", 4, 125),
))
def test_nat(base_note, octave, answer):
    assert nat(Notes[base_note], octave) == answer
    if base_note.endswith('#'):
        if base_note in ("B#", "E#"):
            assert sharp(Notes[base_note], octave) == answer + 3
        else:
            assert sharp(Notes[base_note], octave) == answer + 2
    else:
        assert sharp(Notes[base_note], octave) == answer + 1
    if base_note.endswith('b'):
        if base_note in ("Cb", "Fb"):
            assert flat(Notes[base_note], octave) == answer - 3
        else:
            assert flat(Notes[base_note], octave) == answer - 2
    else:
        assert flat(Notes[base_note], octave) == answer - 1


def expect_translate(black_keys, **others):
    r'''Black keys are C#, D#, F#, G# and A#.
    '''
    blacks = {note: key for note, key in zip("CDFGA", black_keys)}
    ans = []
    for note in "CDEFGAB":
        ans.append(others[note] if note in others else note)
        if note in blacks:
            ans.append(blacks[note])
    return tuple(ans)

def test_expect():
    assert expect_translate("cdfga", C="B") == ("B", "c", "D", "d", "E",
                                                "F", "f", "G", "g", "A", "a", "B")

def test_key_signature():
    assert Key_signature().translation() == expect_translate(("C#", "Eb", "F#", "Ab", "Bb"))


@pytest.mark.parametrize("midi_note, note", (
    ("C", "C"),
    ("C#", "C#"),
    ("Db", "C#"),
    ("D", "D"),
    ("D#", "Eb"),
    ("Eb", "Eb"),
    ("E", "E"),
    ("F", "F"),
    ("F#", "F#"),
    ("Gb", "F#"),
    ("G", "G"),
    ("G#", "Ab"),
    ("Ab", "Ab"),
    ("A", "A"),
    ("A#", "Bb"),
    ("Bb", "Bb"),
    ("B", "B"),
))
def test_midi_note(midi_note, note):
    assert Note_names[Key_signature().MIDI_to_note(MIDI_notes[midi_note])] == note


@pytest.mark.parametrize("num_sharps, expect", (
    # 0 expect_translate(("C#", "Eb", "F#", "Ab", "Bb"))
    (1, expect_translate(("C#", "Eb", "F#", "G#", "Bb"))),                  # F# -> G#
    (2, expect_translate(("C#", "D#", "F#", "G#", "Bb"))),                  # C# -> D#
    (3, expect_translate(("C#", "D#", "F#", "G#", "A#"))),                  # G# -> A#
    (4, expect_translate(("C#", "D#", "F#", "G#", "A#"), F="E#")),          # D# -> E#
    (5, expect_translate(("C#", "D#", "F#", "G#", "A#"), F="E#", C="B#")),  # A# -> B#
    (6, expect_translate(("C#", "D#", "F#", "G#", "A#"), F="E#", C="B#")),  # E# -> F##??
    (7, expect_translate(("C#", "D#", "F#", "G#", "A#"), F="E#", C="B#")),  # B# -> C##??
))
def test_sharp_signature(num_sharps, expect):
    assert Key_signature(num_sharps).translation() == expect

@pytest.mark.parametrize("num_flats, expect", (
    # 0 expect_translate(("C#", "Eb", "F#", "Ab", "Bb"))
    (1, expect_translate(("Db", "Eb", "F#", "Ab", "Bb"))),                  # Bb -> Db
    (2, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"))),                  # Eb -> Gb
    (3, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"), B="Cb")),          # Ab -> Cb
    (4, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"), B="Cb", E="Fb")),  # Db -> Fb
    (5, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"), B="Cb", E="Fb")),  # Gb -> Bbb??
    (6, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"), B="Cb", E="Fb")),  # Cb -> Ebb??
    (7, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"), B="Cb", E="Fb")),  # Fb -> Abb??
))
def test_flat_signature(num_flats, expect):
    assert Key_signature(-num_flats).translation() == expect

