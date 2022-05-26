# test_utils.py

import math

import pytest

from synth.utils import *
from synth.notify import recalc_actors



def test_lv_calc():
    lvc = Largest_value_calculator('lvc')
    assert lvc.get_largest_value() == 0.010
    data = (10, 30, 40)
    lvc.add(data[0])
    assert lvc.get_largest_value() == 10
    lvc.add(data[1])
    lvc.add(data[2])
    mean = sum(data) / 3
    var = sum(abs(n-mean)**2 for n in data) / 3
    std = math.sqrt(var)
    assert lvc.get_largest_value() == mean + std*2


def test_target_time():
    Target_time.set_target_time(4, 1)
    assert Target_time.target_time == 4
    Num_harmonics.value += 2
    assert Target_time.target_time == 4
    recalc_actors()
    assert Target_time.target_time == 2


def test_two_byte_value():
    assert two_byte_value(0x12, 0x34) == 0x0934


class foo:
    b = 0

    @Cross_setter
    def a(self, value):
        self.b = value * 2


def test_Cross_setter():
    f = foo()
    assert f.b == 0
    f.a = 7
    assert f.a == 7
    assert f.b == 14

