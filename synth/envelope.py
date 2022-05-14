# envelope.py

r'''Envelopes change a value over time.  That value could be a freq, or an amplitude.

The values are generated in blocks.

The length of envelope may be scaled by giving it a base freq.
'''

from .channel import Block_generator


class Envelope:
    def for_period(self, base_freq, start=None):
        r'''Returns a generator yielding value(s), duration (in samples), end.

        The generator yields up to Output.block_size samples per iteration.

        The base_freq is used to scale the period of the envelope.  This is done by giving
        each Envelope a scale_fn that takes a base_freq and returns a scale (1 is no
        change, 0.5 cuts the period in half, 2 doubles it, etc).

        The start indicates a starting value.  Not used by all Envelopes...
        '''


class Constant(Envelope):
    r'''A constant value for a duration.

    The duration is scaled.
    '''
    def __init__(self, value, duration=None, scale_fn=None):
        self.value = value
        self.duration = duration
        self.scale_fn = scale_fn

    def for_period(self, base_freq, start=None):
        r'''start is not used.
        '''
        return Constant_generator(self, base_freq)


class Constant_generator(Block_generator):
    def __init__(self, constant, base_freq):
        super().__init__(constant)
        self.constant = constant  # has value, duration (may be None) and scale_fn
        self.base_freq = base_freq
        self.recalc()

    def recalc(self):
        if self.constant.duration is None:
            self.set('length', None)
        else:
            self.set('length',
                     self.constant.duration * self.constant.scale_fn(self.base_freq))
        self.set('value', self.constant.value)

    def __iter__(self):
        try:
            yield from Flat_generator(self)
        finally:
            self.delete()


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

