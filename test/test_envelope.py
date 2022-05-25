# test_envelope.py

import pytest
from unittest.mock import Mock

from synth.envelope import *


@pytest.fixture
def synth():
    return Mock(block_duration="block_dur", block_size="block_sz", dtype="f4")

@pytest.fixture
def instrument(synth):
    return Mock(synth=synth)

@pytest.fixture
def harmonic(instrument):
    return Mock(instrument=instrument)



def test_constant(harmonic):
    c = Constant("some_constant", harmonic, 3)
    assert c.block_duration == 'block_dur'
    assert c.block_size == 'block_sz'
    assert c.dtype == 'f4'

