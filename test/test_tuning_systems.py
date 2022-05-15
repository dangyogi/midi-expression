# test_tuning_systems.py

import pytest
from itertools import tee, product
from functools import partial

from synth.notes import nat, Notes
from synth.tuning_systems import *


# Start by testing helper functions:

@pytest.mark.parametrize("note, octave, freq", (
    ("A", 4, 440),
    ("C", 4, 261.6256),
    ("C", 0, 16.3516),
    ("C", -1, 8.1758),
))
def test_freq_to_Hz_1(note, octave, freq):
    tuning = Equal_temperament()
    assert freq_to_Hz(tuning.note_to_freq(nat(Notes[note], octave))) == \
             pytest.approx(freq, abs=1e-4)


@pytest.mark.parametrize("freq, hz", (
    (0, 8.1758),
    (1200, 16.3516),
    (6000, 261.6256),
    (6900, 440),
))
def test_freq_to_Hz_2(freq, hz):
    assert freq_to_Hz(freq) == pytest.approx(hz, abs=1e-4)


@pytest.mark.parametrize("inc, cents_per_octave, num_elements, ans", (
    (1, 1200, 3, (0, 1, 2)),
    (1000, 1200, 3, (0, 1000, 800)),
    (1000, 1100, 3, (0, 1000, 900)),
))
def test_frange1(inc, cents_per_octave, num_elements, ans):
    assert tuple(frange(inc, cents_per_octave, num_elements)) == ans


@pytest.mark.parametrize("numbers, sums", (
    ((1, 3), (0, 1, 4)),
))
def test_cumsum(numbers, sums):
    assert tuple(cumsum(*numbers)) == sums


# Then test tuning_systems:
@pytest.mark.parametrize("cents_per_semitone", (
    100, 90, 110
))
def test_equal_temperament(cents_per_semitone):
    ts = Equal_temperament(cents_per_semitone)
    for start in range(60, 80):
        assert ts.note_to_freq(start + 1) - ts.note_to_freq(start) == \
                 pytest.approx(cents_per_semitone, abs=1e-3)
    assert ts.cents_per_octave == pytest.approx(cents_per_semitone * 12, abs=1e-4)


@pytest.mark.parametrize("tonic_start, notes_to_freq", (
    (0, [0,    100, 201, 303, 406, 510, 615, 721, 828, 936, 1045, 1155]),
    (1, [0,    45, 145, 246, 348, 451, 555, 660, 766, 873, 981, 1090]),
    (5, [0, 107, 215, 324, 434,    479, 579, 680, 782, 885, 989, 1094]),
    (11, [0, 101, 203, 306, 410, 515, 621, 728, 836, 945, 1055,   1100]),
))
def test_to_tonic(tonic_start, notes_to_freq):
    ts = Base_tuning_system()
    freqs = [0, 100, 201, 303, 406, 510, 615, 721, 828, 936, 1045, 1155]
    assert len(freqs) == 12
    ts.to_tonic(freqs, tonic_start)
    assert ts.notes_to_freq == notes_to_freq


@pytest.mark.parametrize("tonic", (
    "C", "C#", "B",
))
def test_well_temperament(tonic):
    ts = Well_temperament(tonic=tonic, tonic_offset=0)
    tonic_note_number = Notes[tonic]
    p5s = tuple((x + tonic_note_number) % 12 for x in (0, 7, 2, 9, 11))
    for i in p5s:
        assert ts.note_to_freq(nat(i+7, 4)) - ts.note_to_freq(nat(i, 4)) == \
               pytest.approx(H3 - P_comma/5, abs=1e-3)
    for i in range(12):
        if i not in p5s:
            assert ts.note_to_freq(nat(i+7, 4)) - ts.note_to_freq(nat(i, 4)) == \
                   pytest.approx(H3, abs=1e-3)


@pytest.mark.parametrize("tonic", (
    "C", "C#", "B",
))
@pytest.mark.parametrize("max_octave_fudge", (
    0, 10, 100,
))
def test_pythagorean_tuning(tonic, max_octave_fudge):
    ts = Pythagorean_tuning(tonic=tonic, max_octave_fudge=max_octave_fudge)
    if max_octave_fudge < 23:
        assert ts.cents_per_octave == pytest.approx(1200 + max_octave_fudge, abs=1e-4)
    else:
        assert ts.cents_per_octave == pytest.approx(1200 + P_comma, abs=1e-4)
    for H3_start in H3_tuning.H3_order:
        start = nat(Notes[H3_start], 4) + Notes[tonic] - 3
        assert ts.note_to_freq(start + 7) - ts.note_to_freq(start) == \
                 pytest.approx(H3, abs=1e-3)


@pytest.mark.parametrize("tonic", (
    "C", "C#", "B",
))
@pytest.mark.parametrize("pct_comma_fudge", (
    0.20, 0.25,
))
@pytest.mark.parametrize("max_octave_fudge", (
    0, 10, 100,
))
def test_meantone(tonic, pct_comma_fudge, max_octave_fudge):
    ts = Meantone(tonic=tonic, pct_comma_fudge=pct_comma_fudge, tonic_offset=0,
                  max_octave_fudge=max_octave_fudge)
    inc = H3 - Syntonic_comma * pct_comma_fudge
    octave_comma = 1200 - 12 * inc % 1200
    if max_octave_fudge < octave_comma:
        assert ts.cents_per_octave == pytest.approx(1200 - max_octave_fudge, abs=1e-4)
    else:
        assert ts.cents_per_octave == pytest.approx(1200 - octave_comma, abs=1e-4)
    if max_octave_fudge < octave_comma or pct_comma_fudge != 0.25:
        for H3_start in H3_tuning.H3_order:
            start = nat(Notes[H3_start], 4) + Notes[tonic]
            assert ts.note_to_freq(start + 7) - ts.note_to_freq(start) == \
                     pytest.approx(inc, abs=1e-3)


def pairwise(iterable):
    "s -> (s0,s1), (s1,s2), (s2, s3), ..."
    a, b = tee(iterable)
    next(b, None)
    return zip(a, b)


def prepare_create_fns(cls, args):
    if not args:
        return [(cls.__name__, cls)]
    product_input = [[(key, value) for value in values]
                        for key, values in args.items()]
    #print("product_input", product_input)
    arg_combinations = tuple(product(*product_input))
    #print("arg_combinations", arg_combinations)
    kws_list = [{key: value for key, value in combination}
                for combination in arg_combinations]
    print("kws_list", kws_list)
    return [((cls.__name__, kws), partial(cls, **kws)) for kws in kws_list]

@pytest.mark.parametrize("msg, create_fn",
    prepare_create_fns(Well_temperament, {}) # +
    #prepare_create_fns(Meantone, {"pct_comma_fudge": (0.25, 0.20),
    #                              "tonic_offset": (4, -1, 7),
    #                              "max_octave_fudge": (0, 10, 100)}) +
    #prepare_create_fns(Pythagorean_tuning, {}) +
    #prepare_create_fns(Just_intonation, {"tuning": tuple(range(14))})
)
def test_tonics(msg, create_fn):
    Note_order = ("C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B",)
    for t1, t2 in pairwise(Note_order + ("C",)):
        ts1 = create_fn(tonic=t1)
        assert len(ts1.notes_to_freq) == 12
        assert ts1.notes_to_freq[0] == pytest.approx(0, abs=1e-4)
        ts2 = create_fn(tonic=t2)
        assert len(ts2.notes_to_freq) == 12
        assert ts2.notes_to_freq[0] == pytest.approx(0, abs=1e-4)
        print("ts1.notes_to_freq", ts1.notes_to_freq)
        print("ts2.notes_to_freq", ts2.notes_to_freq)
        for n in range(Notes[t1], Notes[t1] + 12):
            print(f"{msg=}, {t1=}, {t2=}, {n=}")
            assert ts1.note_to_freq(nat(n+1,4)) - ts1.note_to_freq(nat(n,4)) == \
              pytest.approx(ts2.note_to_freq(nat(n+2,4)) - ts2.note_to_freq(nat(n+1,4)),
                            abs=1e-4)
        #assert ntf2[11] - ntf2[10] == pytest.approx(ntf1[0] + 1200 - ntf1[11], abs=1e-4)


@pytest.mark.parametrize("pct_comma_fudge, max_octave_fudge, octave_cents", (
    (0.25, 0, 1200),
    (0.25, 10, 1190),
    (0.25, 50, 1158.941),
    (0.20, 0, 1200),
    (0.20, 10, 1190),
    (0.20, 50, 1171.845),
))
def test_meantone_max_octave_fudge(pct_comma_fudge, max_octave_fudge, octave_cents):
    ts = Meantone(pct_comma_fudge=pct_comma_fudge, max_octave_fudge=max_octave_fudge)
    assert ts.cents_per_octave == pytest.approx(octave_cents, abs=1e-3)
