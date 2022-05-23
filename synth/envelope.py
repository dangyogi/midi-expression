# envelope.py

r'''Envelopes change a value over time.  That value could be a freq, or an amplitude.

The values are generated in soundcard numpy blocks.

The duration of the envelope (how many blocks it will generate) may be scaled by a base freq.
'''

import math

from .notify import Var, Actor
from .tuning_systems import C4_freq


class Envelope(Var):
    r'''Derived classes must define "generator" class variable.
    '''
    def __init__(self, name, scale_3):
        super().__init__(name=name)
        if scale_3 is None:
            self.scale_base = 0
        else:
            self.scale_base = math.log(scale_3) / (3 * 1200)  # scale by cents
        self.number_of_generators = 0

    def start(self, base_freq, start=None):
        r'''Returns a generator yielding soundcard block-sized numpy arrays.

        The base_freq is an internal absolute freq (in cents) and is used to scale the
        period of the envelope.  This is done by giving each Envelope a scale_fn that
        takes a base_freq and returns a scale (1 is no change, 0.5 cuts the period in
        half, 2 doubles it, etc).

        The start indicates a starting value for the iteration.  Not used by all
        Envelopes...
        '''
        self.number_of_generators += 1
        return self.generator(self, self.scale(base_freq), start)

    def scale(self, base_freq):
        return math.exp(self.scale_base * (C4_freq - base_freq))


class Block_generator(Actor):
    r'''Derived class must provide recalc and __iter__ methods.

    __iter__ must set self.next to the value following the next yield.  This should be done
    immediately before the yield statement.  It should also call self.delete in a finally
    block.

    recalc must recalc self.scale, along with anything else needed.
    '''
    def __init__(self, envelope, base_freq, start):
        super().__init__(envelope, name=f"{envelope.name}_{envelope.number_of_generators}")
        self.envelope = envelope
        self.base_freq = base_freq
        self.next = start
        self.recalc()

    def recalc(self):
        self.scale = self.envelope.scale(self.base_freq)

    def next_value(self):
        return self.next


class Constant_generator(Block_generator):
    def recalc(self):
        super().recalc()
        if self.envelope.duration is None:
            self.duration = None   # forever...
        else:
            self.duration = self.envelope.duration * self.envelope.scale(self.base_freq)
        self.value = self.envelope.value
        self.next = self.value

    def __iter__(self):
        try:
            yield from Flat_generator(self)
        finally:
            self.delete()


class Constant(Envelope):
    r'''A constant value for a duration.

    The duration is scaled.
    '''
    generator = Constant_generator

    def __init__(self, name, value, scale_3=None, duration=None):
        super().__init__(name, scale_3)
        self.value = value
        self.duration = duration



class Flat_generator(Block_generator):
    def __init__(self, user):
        r'''user must define: length (may be None) and value.
        '''
        super().__init__(user)
        self.user = user  # has value and length (may be None)

    def __iter__(self):
        amount_sent = 0
        try:
            while self.user.length is None or \
                  self.user.length - amount_sent >= Output.block_size:
                yield self.user.value, Output.block_size, self.user.value
                amount_sent += Output.block_size
            if self.user.length - amount_sent > 0:
                yield self.user.value, self.user.length - amount_sent, self.user.value
        finally:
            self.delete()


class Line(Envelope):
    r'''One-shot line generation.

    The line is specified by start, stop and duration.  The duration is scaled.  The
    duration is also affected by the start value given on generation.  This may increase or
    decrease the duration (before scaling) depending on whether it lies before the
    specified start, or after it.
    '''
    def __init__(self, start, stop, duration, scale_fn):
        self.start = start
        self.stop = stop
        self.duration = duration
        self.scale_fn = scale_fn

    def for_period(self, base_freq, start=None):
        if start is None:
            start = self.start
        length_remaining = self.duration * (self.stop - start) / (self.stop - self.start) \
                             * self.scale_fn(base_freq)
        step = (self.stop - start) / length_remaining
        while length_remaining >= Output.block_size:
            stop = start + step * Output.block_size
            yield (np.linspace(start, stop, Output.block_size, endpoint=False),
                   Output.block_size,
                   stop)
            start = stop
            length_remaining -= Output.block_size
        if length_remaining > 0:
            yield (np.linspace(start, self.stop, length_remaining, endpoint=False),
                   length_remaining,
                   self.stop)


class Line_generator(Block_generator):
    def __init__(self, line, base_freq, start):
        super().__init__(line)
        self.line = line  # has start, stop, duration and scale_fn
        self.base_freq = base_freq
        if start is None:
            self.start = self.line.start
        else:
            self.start = start
        self.recalc()

    def recalc(self):
        percent_remaining =   (self.line.stop - self.start) \
                            / (self.line.stop - self.line.start)
        self.length =   self.line.duration \
                      * percent_remaining \
                      * self.line.scale_fn(self.base_freq)
        self.step = (self.line.stop - self.start) / self.length
        self.amount_sent = 0

    def __iter__(self):
        while self.length - self.amount_sent >= Output.block_size:
            stop = self.start + self.step * Output.block_size
            yield (np.linspace(start, stop, Output.block_size, endpoint=False),
                   Output.block_size,
                   stop)
            self.start = stop
            self.amount_sent += Output.block_size
        if self.length - self.amount_sent > 0:
            yield (np.linspace(self.start, self.line.stop,
                               self.length - self.amount_sent,
                               endpoint=False),
                   self.length - self.amount_sent,
                   self.line.stop)
        self.delete()


class Sequence(Envelope):
    r'''Concatenates several envelope segments.
    '''


class Ramp(Envelope):
    r'''Has a base rate, implied by geom and lin, which is scaled by the base_freq.

    If start < limit, it's ramping up.  geom must be >= 1 and lin must be >= 0.  In this
    case ramping is initially down by lin, until the geom step size >= lin.  Then the
    ramping is done by geom.

    If start > limit, it's ramping down.  geom must be <= 1 and lin must be <= 0.
    In this case ramping is initially down by geom, until the geom step size <= lin.
    Then the ramping is done by lin.

    The geom is scaled so that geom**X == geom'**(X * scale).
    Thus, geom' == geom**(X / (X * scale))
                == geom**(1/scale)

    The lin is scaled so that lin*X == lin'*(X * scale).
    Thus, lin' == lin*X/(X * scale)
               == lin/scale
    '''
    def __init__(self, scale_fn, geom=1.0, lin=0.0, start=None, limit=None):
        assert geom != 1 or lin != 0
        self.geom = geom
        self.lin = lin
        self.scale_fn = scale_fn
        self.start = start
        self.limit = limit
        if geom > 1 or lin > 0:
            # ramping up
            if start is None:
                self.start = 2**-15
            if limit is None:
                self.limit = 1
        else:
            # ramping down
            if start is None:
                self.start = 1
            if limit is None:
                self.limit = 2**-15
        assert self.start != self.limit
        if self.start > self.limit:
            # ramping up
            assert geom >= 1
            assert lin >= 0
        else:
            # ramping down
            assert geom <= 1
            assert lin <= 0

    def for_period(self, base_freq, start=None):
        if start is None:
            start = self.start
        return Ramp_generator(self, base_freq, start)

        # FIX: Rewrite with new self.num_samples(...)
        if geom == 1:
            if lin == 0:
                while True:
                    yield start, Output.block_size
            else:
                samples_left = self.num_samples(start, lin=lin)
                while samples_left >= Output.block_size:
                    stop = start + lin * Output.block_size
                    yield (np.linspace(start, stop, Output.block_size, endpoint=False),
                           Output.block_size)
                    start = stop
                    samples_left -= Output.block_size
                if samples_left > 0:
                    stop = start + lin * samples_left
                    yield (np.linspace(start, stop, samples_left, endpoint=False),
                           samples_left)
        elif lin == 0:
            samples_left = self.num_samples(start, geom=geom)
            while samples_left >= Output.block_size:
                stop = start * geom ** Output.block_size
                yield (np.geomspace(start, stop, Output.block_size, endpoint=False),
                       Output.block_size)
                start = stop
                samples_left -= Output.block_size
            if samples_left > 0:
                stop = start * geom ** samples_left
                yield (np.geomspace(start, stop, samples_left, endpoint=False),
                       samples_left)
        else:
            samples_left = self.num_samples(start, geom=geom, lin=lin)
            lin_stop = lin * Output.block_size
            lin_adj = np.linspace(0, lin_stop, Output.block_size, endpoint=False)
            while samples_left >= Output.block_size:
                geom_stop = start * geom ** Output.block_size
                yield (np.geomspace(start, geom_stop, Output.block_size, endpoint=False)
                         + lin_adj, \
                       Output.block_size)
                start = geom_stop + lin_stop
                samples_left -= Output.block_size
            if samples_left > 0:
                stop = start * geom ** samples_left
                yield (np.geomspace(start, stop, samples_left, endpoint=False)
                         + np.linspace(0, lin * samples_left, samples_left, endpoint=False),
                      samples_left)

    def num_samples(self, start, geom, lin):
        r'''Returns lin_samples, geom_samples, lin_samples.

        Calculates the number of samples until the delta produced by geom <= the delta
        produced by lin.  If this is past limit, then the number of samples until limit is
        reached is returned.
        '''
        if lin == 0:
            # start * geom**X < limit
            # geom**X < limit / start
            # math.log(limit / start, geom)
            return 0, round(math.log(self.limit / start, geom)), 0

        if geom == 0:
            # start + lin*X < limit
            # lin*X < limit - start
            # X < (limit - start) / lin
            return round((self.limit - start) / lin), 0, 0

        if lin > 0:
            # ramping up, start lin, end geom, geom > 1, lin > 0
            #
            # (start + lin * X) * (geom - 1) >= lin
            # start + lin * X >= lin / (geom - 1)
            # lin * X >= lin / (geom - 1) - start
            # X >= 1 / (geom - 1) - start / lin
            lin_samples = math.ceil(1 / (geom - 1) - start / lin - 1e-4)
            transition = start + lin * lin_samples
            if transition > self.limit:
                return round((self.limit - start) / lin)
            geom_samples = round(math.log(self.limit / transition, geom))
            return lin_samples, geom_samples, 0

        # else ramping down, start geom, end lin.  geom < 1, lin < 0
        #
        # start * geom**X + delta_X == start * geom**(X + 1)
        # delta_X = start * geom**(X + 1) - start * geom**X
        # delta_X = start * (geom**(X + 1) - geom**X)
        # delta_X = start * geom**X * (geom - 1)
        #
        # delta_X >= lin
        # start * geom**X * (geom - 1) >= lin
        # geom**X >= lin / start / (geom - 1)
        geom_samples = math.floor(math.log(lin / start / (geom - 1), geom))
        transition = start * geom**geom_samples
        if transition < self.limit:
            return round(math.log(self.limit / start, geom))
        return 0, geom_samples, round((self.limit - start) / lin)


class Ramp_generator(Block_generator):
    def __init__(self, ramp, base_freq, start):
        super().__init__(ramp)
        self.ramp = ramp   # has start, stop, geom, lin and scale_fn
        self.base_freq = base_freq
        if start is None:
            self.start = self.ramp.start
        else:
            self.start = start
        self.recalc()

    def recalc(self):
        scale = self.ramp.scale_fn(self.base_freq)
        self.geom = self.ramp.geom**(1/scale)
        self.lin = self.ramp.lin / scale

        percent_remaining =   (self.ramp.stop - self.start) \
                            / (self.ramp.stop - self.ramp.start)
        self.length =   self.ramp.duration \
                      * percent_remaining \
                      * self.ramp.scale_fn(self.base_freq)
        self.step = (self.ramp.stop - self.start) / self.length
        self.amount_sent = 0

    def __iter__(self):
        while self.length - self.amount_sent >= Output.block_size:
            stop = self.start + self.step * Output.block_size
            yield (np.linspace(start, stop, Output.block_size, endpoint=False),
                   Output.block_size,
                   stop)
            self.start = stop
            self.amount_sent += Output.block_size
        if self.length - self.amount_sent > 0:
            yield (np.linspace(self.start, self.ramp.stop,
                               self.length - self.amount_sent,
                               endpoint=False),
                   self.length - self.amount_sent,
                   self.ramp.stop)
        self.delete()


class Sin(Envelope):
    def __init__(self, freq_Hz=1.0, scale_fn=lambda x: x):
        self.freq_Hz = freq_Hz
        self.scale_fn = scale_fn

    def for_period(self, base_freq, start=None):
        r'''start is not used.
        '''
        freq = self.freq_Hz * self.scale_fn(base_freq)

