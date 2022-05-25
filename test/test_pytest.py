# test_pytest.py

import pytest


@pytest.fixture
def one():
    return 1

@pytest.fixture
def two():
    return 2

@pytest.fixture
def three():
    return 3

#@pytest.fixture(params=(one,two,three))   # gets functions, not fixture values...
@pytest.fixture(params=(1, 2, 3))
def foo(request):
    #return request.param()                # can't call fixture functions directly
    return request.param


def test_foo(request, foo):
    print("foo", foo)
    #print("request", request)              # can't see fixture params here
    assert foo in (1,2,3)


@pytest.fixture
def duration():
    return 7

@pytest.fixture
def bar(duration):
    return duration


def test_bar1(bar, duration):
    assert bar == duration


@pytest.mark.parametrize("duration", (4,5))
def test_bar2(bar, duration):
    assert bar == duration              # works!
    assert duration in (4,5)


@pytest.fixture
def bar2(duration2=27):
    return duration2


def test_bar21(bar2):
    assert bar2 == 27                    # works!


@pytest.mark.parametrize("duration2", (24,25))
def test_bar22(bar2, duration2):
    #assert bar2 == duration2              # doesn't work, bar2 is 27!
    assert duration2 in (24,25)


@pytest.fixture
def baz(duration):
    return duration

@pytest.mark.parametrize("duration", (24,25))
def test_baz(baz, duration):
    assert baz == duration              # works!
    assert duration in (24,25)

@pytest.mark.parametrize("duration", (24,25))
def test_baz2(baz):
    assert baz in (24,25)

def test_baz3(baz, duration=4):
    assert duration == 4                # gets 4, not 7 from fixture
