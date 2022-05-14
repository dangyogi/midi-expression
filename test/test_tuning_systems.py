# test_tuning_systems.py

import pytest
from itertools import tee
from functools import partial

from synth.notes import nat
from synth.tuning_systems import *


# Start by testing helper functions:

@pytest.mark.parametrize("note, octave, freq", (
    ("A", 4, 440),
    ("C", 4, 261.6256),
    ("C", 0, 16.3516),
    ("C", -1, 8.1758),
))
def test_freq_to_Hz(note, octave, freq):
    tuning = Equal_temperament()
    assert freq_to_Hz(tuning.note_to_freq(nat(Notes[note], octave))) == \
             pytest.approx(freq, abs=1e-4)


@pytest.mark.parametrize("freq, hz", (
    (0, 8.1758),
    (1200, 16.3516),
    (6000, 261.6256),
    (6900, 440),
))
def test_freq_to_Hz(freq, hz):
    assert freq_to_Hz(freq) == pytest.approx(hz, abs=1e-4)


@pytest.mark.parametrize("inc, num_elements, ans", (
    (1, 3, (0, 1, 2)),
    (1000, 3, (0, 1000, 800)),
))
def test_frange(inc, num_elements, ans):
    assert tuple(frange(inc, num_elements)) == ans


@pytest.mark.parametrize("numbers, sums", (
    ((1, 3), (0, 1, 4)),
))
def test_cumsum(numbers, sums):
    assert tuple(cumsum(*numbers)) == sums


# Then test tuning_systems:
def test_equal_temperament():
    assert Equal_temperament().notes_to_freq == \
      [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100]


def pairwise(iterable):
    "s -> (s0,s1), (s1,s2), (s2, s3), ..."
    a, b = tee(iterable)
    next(b, None)
    return zip(a, b)

@pytest.mark.parametrize("cls, arg2_name, arg2_values, arg3_name, arg3_values", (
    (Meantone, "pct_comma_fudge", (0.25, 0.20), "tonic_offset", (4, -1, 7)),
    (Pythagorean_tuning, None, (), None, ()),
    (Just_intonation, "tuning", tuple(range(14)), None, ()),
    (Well_temperament, None, (), None, ()),
))
def test_tuning_systems(cls, arg2_name, arg2_values, arg3_name, arg3_values):
    if arg2_name is None:
        create_fns = cls,
    elif arg3_name is None:
        create_fns = [partial(cls, **{arg2_name: value2}) for value2 in arg2_values]
    else:
        create_fns = [partial(cls, **{arg2_name: value2, arg3_name: value3})
                      for value2 in arg2_values
                      for value3 in arg3_values]
    for create_fn in create_fns:
        Notes = ("C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B",)
        def test(t1, t2):
            ntf1 = create_fn(tonic=t1).notes_to_freq
            assert len(ntf1) == 12
            assert ntf1[0] == pytest.approx(0, abs=1e-4)
            ntf2 = create_fn(tonic=t2).notes_to_freq
            assert len(ntf2) == 12
            assert ntf2[0] == pytest.approx(0, abs=1e-4)
            for i in range(10):
                print(f"{t1=}, {t2=}, {i=}")
                assert ntf2[i+1] - ntf2[i] == pytest.approx(ntf1[i+2] - ntf1[i+1], abs=1e-4)
            assert ntf2[11] - ntf2[10] == pytest.approx(ntf1[0] + 1200 - ntf1[11], abs=1e-4)
        for t1, t2 in pairwise(Notes):
            test(t1, t2)
        test("B", "C")

