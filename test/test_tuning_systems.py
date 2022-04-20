# test_tuning_systems.py

import pytest

from synth.notes import nat
from synth.tuning_systems import *


@pytest.mark.parametrize("note, octave, freq", (
    ("A", 4, 440),
    ("C", 4, 261.6256),
    ("C", 0, 16.3516),
    ("C", -1, 8.1758),
    ("Cb", -1, 7.7169),
))
def test_freq_to_Hz(note, octave, freq):
    tuning = Equal_temperament()
    assert freq_to_Hz(tuning.note_to_freq(nat(Notes[note], octave))) == \
             pytest.approx(freq, abs=1e-4)

@pytest.mark.parametrize("seq, n, ans", (
    ("ABCDEFG", 3, ("ABC", "DEF", "G")),
    ("ABCDEFG", -3, ("EFG", "BCD", "A")),
))
def test_grouper(seq, n, ans):
    assert tuple(grouper(seq, n)) == ans
