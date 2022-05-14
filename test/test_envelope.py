# test_envelope.py

from synth.envelope import *


def scale_unity(x):
    return x

def test_constant():
    c = Constant(4, 3, scale_unity)

