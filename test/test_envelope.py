# test_envelope.py

from itertools import tee

import numpy as np
import pytest
from unittest.mock import Mock

from synth.envelope import *


@pytest.fixture
def synth(sample_rate=50):
    block_duration = 0.1
    block_size = int(sample_rate * block_duration)
    #sample_rate = block_size / block_duration
    delta_t = 1 / sample_rate
    dtype = "f4"
    return Mock(block_duration=block_duration, block_size=block_size, dtype=dtype,
                delta_times=np.full(block_size, delta_t, dtype=dtype))

@pytest.fixture
def instrument(synth):
    return Mock(synth=synth)

@pytest.fixture
def harmonic(instrument):
    return Mock(instrument=instrument)


@pytest.fixture
def scale_3():
    return None

# Fixture madness for Envelopes:

def constant(harmonic, duration, scale_3):
    return Constant("test_constant", harmonic, 0.123, duration, scale_3)

@pytest.fixture(name='constant')
def constant_fixture(harmonic, duration, scale_3):
    return constant(harmonic, duration, scale_3)

@pytest.fixture
def bend():
    return 0

def ramp_up(harmonic, duration, scale_3, bend):
    return Ramp("test_ramp_up", harmonic, 1, 16, duration, bend=bend, scale_3=scale_3)

@pytest.fixture(name="ramp_up")
def ramp_up_fixture(harmonic, duration, scale_3, bend):
    return ramp_up(harmonic, duration, scale_3, bend)

def ramp_down(harmonic, duration, scale_3, bend):
    return Ramp("test_ramp_down", harmonic, 16, 1, duration, bend=bend, scale_3=scale_3)

@pytest.fixture(name="ramp_down")
def ramp_down_fixture(harmonic, duration, scale_3, bend):
    return ramp_down(harmonic, duration, scale_3, bend)

@pytest.fixture(params=(ramp_up, ramp_down))
def ramp(request, harmonic, duration, scale_3, bend):
    return request.param(harmonic, duration, scale_3, bend)

def sin_cycle1(harmonic, duration, scale_3):
    return Sin("test_sin_cycle1", harmonic, 1, 100, 50, duration, scale_3)

@pytest.fixture(name="sin_cycle1")
def sin_cycle1_fixture(harmonic, duration, scale_3):
    return sin_cycle(harmonic, duration, scale_3)

def sin_cycle2(harmonic, duration, scale_3):
    return Sin("test_sin_cycle1", harmonic, 1, 'base_freq', 50, duration, scale_3)

@pytest.fixture(name="sin_cycle2")
def sin_cycle2_fixture(harmonic, duration, scale_3):
    return sin_cycle2(harmonic, duration, scale_3)

def sin(harmonic, duration, scale_3):
    return Sin("test_sin_cycle1", harmonic, duration=duration, scale_3=scale_3)

@pytest.fixture(name="sin")
def sin_fixture(harmonic, duration, scale_3):
    return sin(harmonic, duration, scale_3)

@pytest.fixture(params=(sin_cycle1, sin_cycle2, sin))
def sins(request, harmonic, duration, scale_3):
    return request.param(harmonic, duration, scale_3)

@pytest.fixture(params=(constant, ramp_up, ramp_down, sin_cycle1, sin_cycle2, sin))
def env(request, harmonic, duration, scale_3, bend):
    if request.param in (ramp_up, ramp_down):
        if duration is None:
            return None
        return request.param(harmonic, duration, scale_3, bend)
    return request.param(harmonic, duration, scale_3)

@pytest.fixture
def sequence(harmonic):
    return Sequence([None, 
                     Constant("c1", harmonic, 1, 0.5),
                     None,
                     Constant("c2", harmonic, 2, 0.5),
                     None])


def count_blocks(blk_gen):
    r'''Counts the number of sound blocks returned by blk_gen.

    Limits the count to 1000.

    Returns first_sample, last_i, last_sample
    '''
    first_sample = None
    last_i = 0
    last_sample = None
    for i, samples in zip(range(1, 1001), blk_gen):
        #print("count_blocks", i, samples)
        if first_sample is None:
            if isinstance(samples, (int, float)):
                first_sample = samples
            else:
                first_sample = samples[0]
        last_i = i
        if isinstance(samples, (int, float)):
            last_sample = samples
        else:
            last_sample = samples[-1]
    return first_sample, last_i, last_sample


def check_np(np_array, values):
    assert tuple(np_array) == tuple(pytest.approx(n, abs=1e-5) for n in values)
    #assert len(np_array) == len(values)
    #for i, (np_x, exp_x) in enumerate(zip(np_array, values)):
    #    assert np_x == pytest.approx(exp_x, abs=1e-4), \


def pairwise(iterable):
    "s -> (s0,s1), (s1,s2), (s2, s3), ..."
    a, b = tee(iterable)
    next(b, None)
    return zip(a, b)


def test_synth(synth):
    assert synth.block_duration == 0.1
    assert synth.block_size == 5
    check_np(synth.delta_times, (0.02,)*5)


#@pytest.mark.skip
@pytest.mark.parametrize("duration, exp_count", (
    (None, 1000),
    (1, 10),
))
@pytest.mark.parametrize("base_freq", (100, 1000))
@pytest.mark.parametrize("start", (None, 1))
def test_duration(harmonic, env, duration, exp_count, base_freq, start):
    if env is not None:
        #print("env", env, "duration", duration, "base_freq", base_freq, "start", start)
        assert count_blocks(env.start(base_freq, start))[1] == exp_count

@pytest.mark.parametrize("duration", (None, 0.5))
@pytest.mark.parametrize("start_value", (None, 1))
def test_first_constant(constant, duration, start_value):
    gen = constant.start(1234, start_value)
    assert gen.next_value() == (start_value or 0.123)
    assert gen.target_value() == 0.123
    it = iter(gen)
    seen = 0
    if start_value is not None:
        check_np(next(it), (1, 0.8246, 0.6492, 0.4738, 0.2984))
        assert gen.target_value() == 0.123
        assert gen.next_value() == 0.123
        seen = 1
    for i, n in enumerate(it, seen + 1):
        assert n == 0.123
        assert gen.target_value() == 0.123
        assert gen.next_value() == 0.123
        if i >= 1000:
            break
    if duration is None:
        assert i == 1000
    else:
        assert i == gen.num_blocks
        assert i == duration / constant.block_duration


@pytest.mark.parametrize("duration", (5,))   # 50 blocks
@pytest.mark.parametrize("scale_3", (5,))
@pytest.mark.parametrize("base_freq, num_blocks", (
    (2400, 250),
    (3600, 146),
    (4800, 85),
    (6000, 50),  # middle C
    (7200, 29),
    (8400, 17),
    (9600, 10),
))
def test_constant_scaling(constant, duration, base_freq, scale_3, num_blocks):
    assert count_blocks(constant.start(base_freq))[1] == num_blocks


def test_sequence(sequence):
    gen = sequence.start(6000)
    #assert gen.next_value() == 4
    assert gen.target_value() == 2
    it = iter(gen)
    for i, samples in zip(range(5), it):
        assert samples == 1
        assert gen.next_value() == 1
        assert gen.target_value() == 2
    assert i == 4
    check_np(next(it), (1, 1.2, 1.4, 1.6, 1.8))
    for i, samples in zip(range(4), it):
        assert samples == 2
        assert gen.next_value() == 2
        assert gen.target_value() == 2
    assert i == 3
    with pytest.raises(StopIteration):
        next(it)
    assert gen.next_value() == 2
    assert gen.target_value() == 2


@pytest.mark.parametrize("duration", (0.3,))   # 3 blocks
@pytest.mark.parametrize("bend, expect", (
    (0, ((1, 2, 3, 4, 5), (6, 7, 8, 9, 10), (11, 12, 13, 14, 15))),
))
def test_ramp_up1(ramp_up, bend, expect):
    gen = ramp_up.start(6000)
    assert gen.next_value() == 1
    assert gen.target_value() == 16
    it = iter(gen)
    for i, samples, samples_expected in zip(count(1), it, expect):
        check_np(samples, samples_expected)
        assert gen.next_value() == 1 + 5*i
        assert gen.target_value() == 16
    assert i == 3
    with pytest.raises(StopIteration):
        next(it)
    assert gen.next_value() == 16
    assert gen.target_value() == 16

@pytest.mark.parametrize("duration", (0.3,))   # 3 blocks
@pytest.mark.parametrize("bend, first_step, last_step", (
    (-1, 0, 2),
    (0, 1, 1),
    (1, 2, 0),
))
def test_ramp_up2(ramp_up, bend, first_step, last_step):
    gen = ramp_up.start(6000)
    it = iter(gen)

    samples = next(it)
    assert samples[1] - samples[0] == pytest.approx(first_step, abs=1e-2)
    for a, b in pairwise(samples):
        assert b - a <= max(first_step, last_step)

    samples = next(it)
    for a, b in pairwise(samples):
        assert b - a <= max(first_step, last_step)

    samples = next(it)
    for a, b in pairwise(samples):
        assert b - a <= max(first_step, last_step)
    assert samples[-1] - samples[-2] == pytest.approx(last_step, abs=1e-2)

    with pytest.raises(StopIteration):
        next(it)


@pytest.mark.parametrize("duration", (0.3,))   # 3 blocks
@pytest.mark.parametrize("bend, expect", (
    (0, ((16, 15, 14, 13, 12), (11, 10, 9, 8, 7), (6, 5, 4, 3, 2))),
))
def test_ramp_down1(ramp_down, bend, expect):
    gen = ramp_down.start(6000)
    assert gen.next_value() == 16
    assert gen.target_value() == 1
    it = iter(gen)
    for i, samples, samples_expected in zip(count(1), it, expect):
        check_np(samples, samples_expected)
        assert gen.next_value() == 16 - 5*i
        assert gen.target_value() == 1
    assert i == 3
    with pytest.raises(StopIteration):
        next(it)
    assert gen.next_value() == 1
    assert gen.target_value() == 1

@pytest.mark.parametrize("duration", (0.3,))   # 3 blocks
@pytest.mark.parametrize("bend, first_step, last_step", (
    (-1, -2, 0),
    (0, -1, -1),
    (1, 0, -2),
))
def test_ramp_down2(ramp_down, bend, first_step, last_step):
    gen = ramp_down.start(6000)
    it = iter(gen)

    samples = next(it)
    assert samples[1] - samples[0] == pytest.approx(first_step, abs=1e-2)
    for a, b in pairwise(samples):
        assert b - a <= max(first_step, last_step)

    samples = next(it)
    for a, b in pairwise(samples):
        assert b - a <= max(first_step, last_step)

    samples = next(it)
    for a, b in pairwise(samples):
        assert b - a <= max(first_step, last_step)
    assert samples[-1] - samples[-2] == pytest.approx(last_step, abs=1e-2)

    with pytest.raises(StopIteration):
        next(it)

