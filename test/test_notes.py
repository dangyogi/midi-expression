# test_notes.py

import pytest

from synth.notes import *


@pytest.mark.parametrize("base_note, octave, answer", (
    ("C", -1, 0),
    ("C", 0, 12),
    ("C", 4, 60),
    ("C#", 4, 61),
    ("Db", 4, 61),
    ("D", 4, 62),
    ("D#", 4, 63),
    ("Eb", 4, 63),
    ("E", 4, 64),
    ("E#", 4, 65),
    ("Fb", 4, 64),
    ("F", 4, 65),
    ("F#", 4, 66),
    ("Gb", 4, 66),
    ("G", 4, 67),
    ("G#", 4, 68),
    ("Ab", 4, 68),
    ("A", 4, 69),
    ("A#", 4, 70),
    ("Bb", 4, 70),
    ("B", 4, 71),
    ("B#", 4, 60),
    ("C", 5, 72),
))
def test_nat(base_note, octave, answer):
    assert nat(Notes[base_note], octave) == answer
    assert sharp(Notes[base_note], octave) == answer + 1
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


@pytest.mark.parametrize("midi_note, name", (
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
@pytest.mark.parametrize("octave", (
    0, 4,
))
def test_midi_note(midi_note, octave, name):
    assert Key_signature().note_name(nat(Notes[midi_note], octave)) == f"{name}{octave}"


@pytest.mark.parametrize("num_sharps, expect", (
    # Sharps: F#, C#, G#, D#, A#, E#, B#
    # Black keys are       C#,   D#,   F#,   G# and A#.
    # 0 expect_translate(("C#", "Eb", "F#", "Ab", "Bb"))
    (1, expect_translate(("C#", "Eb", "F#", "G#", "Bb"))),                  # Ab -> G#
    (2, expect_translate(("C#", "D#", "F#", "G#", "Bb"))),                  # Eb -> D#
    (3, expect_translate(("C#", "D#", "F#", "G#", "A#"))),                  # Bb -> A#
    (4, expect_translate(("C#", "D#", "F#", "G#", "A#"))),
    (5, expect_translate(("C#", "D#", "F#", "G#", "A#"))),
    (6, expect_translate(("C#", "D#", "F#", "G#", "A#"), F="E#")),          # F  -> E#
    (7, expect_translate(("C#", "D#", "F#", "G#", "A#"), F="E#", C="B#")),  # C  -> B#
))
def test_sharp_signature(num_sharps, expect):
    assert Key_signature(num_sharps).translation() == expect

@pytest.mark.parametrize("num_flats, expect", (
    # Flats: Bb, Eb, Ab, Db, Gb, Cb, Fb
    # Black keys are       Db,   Eb,   Gb,   Ab and Bb.
    # 0 expect_translate(("C#", "Eb", "F#", "Ab", "Bb"))
    (1, expect_translate(("Db", "Eb", "F#", "Ab", "Bb"))),                  # C# -> Db
    (2, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"))),                  # F# -> Gb
    (3, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"))),
    (4, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"))),
    (5, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"))),
    (6, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"), B="Cb")),          # B  -> Cb
    (7, expect_translate(("Db", "Eb", "Gb", "Ab", "Bb"), B="Cb", E="Fb")),  # E  -> Fb
))
def test_flat_signature(num_flats, expect):
    assert Key_signature(-num_flats).translation() == expect

