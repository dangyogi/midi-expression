# envelope.py

r'''Envelopes change a value over time.  That value could be a freq, or an amplitude.

The values are generated in soundcard numpy blocks.

The duration of the envelope (how many blocks it will generate) may be scaled by a base freq.
'''

import math
from itertools import count
import numpy as np

from .notify import Notify_actors, Var, Actor
from .tuning_systems import C4_freq, freq_to_Hz
from .utils import Cross_setter

__all__ = ("Constant", "Sequence", "Ramp", "Sin")


class Envelope(Var):
    r'''Base class for the various types of Envelopes.

    These objects last long-term and are used many times over and over again.  The
    generator objects created by the `start` method are only used once.

    Derived classes must define the "generator" class variable as the class to instantiate
    by the `start` method.
    '''
    duration = Notify_actors()
    scale_base = Notify_actors()

    def __init__(self, name, harmonic, duration, scale_3):
        super().__init__(name=name)
        self.harmonic = harmonic
        self.duration = duration
        self.block_duration = harmonic.instrument.synth.block_duration
        self.block_size = harmonic.instrument.synth.block_size
        self.dtype = harmonic.instrument.synth.dtype
        self.scale_3 = scale_3
        self.number_of_generators = 0

    @Cross_setter
    def scale_3(self, value):
        if value is None:
            self.scale_base = 0
        else:
            self.scale_base = math.log(value) / (3 * 1200)  # scale by cents

    def start(self, base_freq, start_value=None):
        r'''Returns a Block_generator yielding soundcard block-sized numpy arrays.

        The base_freq is an internal absolute freq (in cents) and is used to scale the
        duration of the envelope.  This is done by giving each Envelope a scale_3 that
        takes a base_freq and returns a scale that gets multiplied by the duration.

        The start_value indicates a starting value for the iteration.  Not used by all
        Envelopes...  None means to use the start_value cooked into the envelope.
        '''
        self.number_of_generators += 1   # becomes part of their name...
        return self.generator(self, base_freq, start_value)

    def scale(self, base_freq):
        ans = math.exp(self.scale_base * (C4_freq - base_freq))
        #print(f"{self.name}.scale, scale_base is {self.scale_base:.4f}, ans is {ans:.4f}")
        return ans


class Base_block_generator:
    def __init__(self, envelope, base_freq, start_value):
        self.envelope = envelope
        self.base_freq = base_freq
        if start_value is None and hasattr(envelope, 'start_value'):
            self.next = envelope.start_value
        else:
            self.next = start_value
        self.blocks_sent = 0

    def next_value(self):
        return self.next

    def target_value(self):
        return self.next_value()

class Block_generator(Base_block_generator, Actor):
    r'''Derived class must provide recalc and __iter__ methods.

    __iter__ must set self.next to the value following the next yield.  This should be done
    immediately before the yield statement.  It should also call self.delete in a finally
    block.

    recalc must recalc self.scale, along with anything else needed.
    '''
    def __init__(self, envelope, base_freq, start_value):
        Base_block_generator.__init__(self, envelope, base_freq, start_value)
        Actor.__init__(self, envelope, envelope.harmonic,
                       name=f"{envelope.name}_{envelope.number_of_generators}")
        self.recalc()

    def recalc(self):
        self.scale = self.envelope.scale(self.base_freq)
        if self.envelope.duration is None:
            self.duration = None
        else:
            self.duration = self.envelope.duration * self.scale


class Constant_generator(Block_generator):
    def recalc(self):
        super().recalc()  # recalcs self.scale
        if self.envelope.duration is None:
            self.duration = None   # forever...
            self.num_blocks = None
        else:
            self.duration = self.envelope.duration * self.scale
            self.num_blocks = round(self.duration / self.envelope.block_duration)

    def __iter__(self):
        try:
            if self.next == self.envelope.start_value or self.num_blocks == 0:
                self.blocks_sent = 0
            else:
                # quick one block linear ramp to adjust start_value given to my value.
                start = self.next
                self.next = self.envelope.start_value
                yield np.linspace(start, self.next, self.envelope.block_size, 
                                  endpoint=False, dtype=self.envelope.dtype)
                self.blocks_sent = 1
            for self.blocks_sent in count(self.blocks_sent):
                #print("blocks_sent", self.blocks_sent)
                if self.num_blocks is not None and self.blocks_sent >= self.num_blocks:
                    break
                yield self.next
        finally:
            self.delete()

    def target_value(self):
        return self.envelope.start_value


class Constant(Envelope):
    r'''A constant value for a duration.

    The duration is scaled.
    '''
    generator = Constant_generator

    value = Notify_actors()

    def __init__(self, name, harmonic, value, duration=None, scale_3=None):
        super().__init__(name, harmonic, duration, scale_3)
        self.start_value = value


class Sequence_generator(Base_block_generator):
    def __init__(self, envelope, base_freq, start_value):
        super().__init__(envelope, base_freq, start_value)
        self.current_env = None
        self.generator = None

    def __iter__(self):
        #try:
            for i in count():
                if i >= len(self.envelope.envelope_list):
                    break
                self.current_env = self.envelope.envelope_list[i]
                if self.current_env is not None:
                    self.generator = self.current_env.start(self.base_freq, self.next)
                    yield from self.generator
                    self.next = self.generator.next_value()
        #finally:
        #    self.delete()

    def next_value(self):
        if self.generator is None:
            return self.next
        return self.generator.next_value()

    def target_value(self):
        for env in reversed(self.envelope.envelope_list):
            if env is not None:
                if env == self.current_env:
                    return self.generator.target_value()
                else:
                    return env.start(self.base_freq).target_value()
        return None


class Sequence:
    r'''Like itertools.chain, but for Envelopes.

    Takes a list of Envelopes and waits until it needs the next Envelope before getting
    it from the list.  So if the later list contents change while earlier contents are
    being generated, the changes will be seen.  This includes the length of
    the list changing in flight.

    Ignores None values in the list of Envelopes.

    Passes the next value from one generator to the next Envelope's start method.
    '''
    def __init__(self, envelope_list):
        self.envelope_list = envelope_list
        #self.number_of_generators = 0

    def start(self, base_freq, start_value=None):
        #self.number_of_generators += 1   # becomes part of their name...
        return Sequence_generator(self, base_freq, start_value)


class Ramp_generator(Block_generator):
    def recalc(self):
        super().recalc()

        start = self.next
        self.stop = self.envelope.stop_value
        if self.envelope.add_base_freq:
            start += self.base_freq
            self.stop += self.base_freq

        # slope from start_value to stop_value
        self.S = (self.stop - start) \
               / (self.duration - self.blocks_sent * self.envelope.block_duration)

        # change in slope based on bend
        self.X = abs(self.S) * self.envelope.bend

        # the starting slope (at t0).
        self.S0 = self.S + self.X

        self.num_blocks = round(self.duration / self.envelope.block_duration)

        # change in slope per block iteration
        self.delta = 2*(self.S - self.S0) / ((self.num_blocks - self.blocks_sent) - 1)

        self.start_at = self.blocks_sent

    def __iter__(self):
        try:
            for self.blocks_sent in count():
                if self.blocks_sent >= self.num_blocks:
                    break
                start = self.next
                self.next = \
                  start + (self.S0 + self.delta * (self.blocks_sent - self.start_at)) \
                        * self.envelope.block_duration
                yield np.linspace(start, self.next, self.envelope.block_size,
                                  endpoint=False, dtype=self.envelope.dtype)
        finally:
            self.delete()

    def target_value(self):
        return self.stop



class Ramp(Envelope):
    r'''Ramp up or down.

    Specified by start_value, stop_value, duration and bend (a +/- % of max).

    Fitting a quadratic (A*t**2 + B*t + C) to those parameters:

        - C = start

    To simplify a bit, we'll introduce a few variables:

        - D = duration
        - S = (stop-start)/D    # the slope of the line from start to stop
        - X = S * bend          # the offset of the starting slope from S (max S).
        - S0 = S + X            # the starting slope (at t0).

    Now we can tackle B:

        - derivative is 2*A*t + B, which at t=0 is simply B, so:

        - B = S0 = S + X

    Then we pick A to bring us to stop at D:

        - A*D**2 + B*D + start == stop 

        - A*D + B = (stop-start)/D = S

        - A*D + S + X = S

        - A = -X/D

    This will always result in a final slope, SD, that is the same offset from flat_slope.

        - slope at t=D is 2*A*D + B = SD
        - SD = -2X + S + X
        - SD = S - X

    We'll approximate the quadratic by using line segments for each generated block.  The
    slope of these lines starts at S0, and change by a constant delta (derivative of
    quadratic is linear).  Thus,

        - Db = block_duration
        - D/Db = N
        - stop = start + sum((S0+delta*i)*Db, i=0 to N-1)
        - stop - start = Db*sum(S0+delta*i, i=0 to N-1)
        - (stop - start) / Db = N*S0 + delta*sum(i, i=0 to N-1)
        - (stop - start) / Db - N*S0 = delta*N*(N-1)/2
        - delta = ((stop - start) / Db - N*S0) / (N*(N-1)/2)
        - delta = ((stop - start) / Db - D/Db*S0) / (D/Db*(D/Db-1)/2)
        - delta = (((stop - start) - D*S0)/Db) / (D/Db*(D/Db-1)/2)
        - delta = ((stop - start) - D*S0) / Db*(D/Db*(D/Db-1)/2)
        - delta = ((stop - start) - D*S0) / (D*(D/Db-1)/2)
        - delta = ((stop - start) - D*S0) / (D^2/Db-D)/2)
        - delta = 2*((stop - start) - D*S0) / (D^2/Db-D)
        - delta = 2*((stop - start)/D - S0) / (D/Db-1))
        - delta = 2*(S - S0) / (N-1)
    '''
    generator = Ramp_generator

    start_value = Notify_actors()
    stop_value = Notify_actors()
    bend = Notify_actors()

    def __init__(self, name, harmonic, start_value, stop_value, duration, bend=0,
                 scale_3=None, add_base_freq=False):
        assert duration is not None, f"{name}: infinate durations not allowed"
        super().__init__(name, harmonic, duration, scale_3)
        self.start_value = start_value
        self.stop_value = stop_value
        self.bend = bend
        self.add_base_freq = add_base_freq


class Sin_generator(Block_generator):
    r'''The Block_generator for the Sin envelope.

    The next_value(), at end of the generator, is the radian_offset for the next Sin
    envelope.  This should work with any freq.
    '''
    def __init__(self, envelope, base_freq, start_value):
        self.waveform = None
        self.cycle_time = None
        self.delta_times = envelope.harmonic.instrument.synth.delta_times
        self.first_recalc = True
        super().__init__(envelope, base_freq, start_value)
        if self.next is None:
            self.next = 0

    def recalc(self):
        super().recalc()
        if self.envelope.duration is None:
            self.duration = None   # forever...
            self.num_blocks = None
        else:
            self.duration = self.envelope.duration * self.scale
            self.num_blocks = round(self.duration / self.envelope.block_duration)
            print(f"{self.name}: duration {self.duration}, num_blocks {self.num_blocks}")
        if self.first_recalc or self.cycle_time is not None:
            if self.envelope.cycle_time is not None:
                self.cycle_time = self.envelope.cycle_time * self.scale
                radians_per_sec = 1/self.cycle_time * 2.0 * np.pi
                print(f"{self.name}: cycle_time {self.cycle_time},",
                      f"radians_per_sec {radians_per_sec}")
                self.radians = np.cumsum(radians_per_sec * self.delta_times,
                                         dtype=self.envelope.dtype)
                self.radians_inc = \
                  (radians_per_sec * self.envelope.block_duration) % (2 * np.pi)
                if self.envelope.ampl_swing is not None:
                    self.ampl_swing = self.envelope.ampl_swing
                else:
                    self.ampl_swing = 1
                if self.envelope.center_ampl is not None:
                    self.target = self.envelope.center_ampl
                else:
                    self.target = 0
                if self.envelope.add_base_freq:
                    self.target += self.base_freq
                self.have_iter = False
                print(f"{self.name}: radians[0] {self.radians[0]},",
                      f"radians[-1] {self.radians[-1]},",
                      f"radians_inc {self.radians_inc},",
                      f"ampl_swing {self.ampl_swing},",
                      f"target {self.target},",
                      f"have_iter {self.have_iter},")
        if self.cycle_time is None:
            self.ampl_swing = 1
            self.target = 0
            if isinstance(self.base_freq, Base_block_generator):
                self.freq_iter = iter(self.base_freq)
                self.have_iter = True
            else:
                radians_per_sec = \
                  freq_to_Hz(self.base_freq) \
                  * (self.envelope.harmonic.freq_offset * 2.0 * np.pi)
                self.radians = np.cumsum(radians_per_sec * self.delta_times,
                                         dtype=self.envelope.dtype)
                self.radians_inc = \
                  (radians_per_sec * self.envelope.block_duration) % (2 * np.pi)
                self.have_iter = False
                print(f"{self.name}: radians_per_sec {radians_per_sec},",
                      f"radians[0] {self.radians[0]},",
                      f"radians[-1] {self.radians[-1]},",
                      f"radians_inc {self.radians_inc},",
                      f"ampl_swing {self.ampl_swing},",
                      f"target {self.target},",
                      f"have_iter {self.have_iter},")
        self.first_recalc = False

    def __iter__(self):
        try:
            for self.blocks_sent in count():
                if self.num_blocks is not None and self.blocks_sent >= self.num_blocks:
                    break
                if self.have_iter:
                    print(f"{self.name}: have_iter")
                    try:
                        freq_block = next(self.freq_iter)
                    except StopIteration:
                        # continue with self.base_freq.target_value
                        radians_per_sec = \
                          freq_to_Hz(self.base_freq.target_value()) \
                          * (self.envelope.harmonic.freq_offset * 2.0 * np.pi)
                        self.radians = np.cumsum(radians_per_sec * self.delta_times,
                                                 dtype=self.envelope.dtype)
                        self.radians_inc = \
                          (radians_per_sec * self.envelope.block_duration) % (2 * np.pi)
                        self.have_iter = False
                    else:
                        radians_per_sec = freq_to_Hz(freq_block) \
                          * (self.envelope.harmonic.freq_offset * 2.0 * np.pi)
                        self.radians = np.cumsum(radians_per_sec * self.delta_times,
                                                 dtype=self.envelope.dtype)
                        # FIX: I think that this is off by 1, but am hoping that it is
                        # undetectable.  I guess it could stretch the waveform out a bit
                        # and slightly lower the freq?
                        self.radians_inc = self.radians[-1] % (2 * np.pi)

                radians_per_sample = self.radians + self.next
                #print(f"{radians_per_sample[0]=}, {radians_per_sample[1]=}, " \
                #      f"{radians_per_sample[-1]=}")

                self.next = (self.next + self.radians_inc) % (2 * np.pi)

                # takes ~15 uSec with dtype='f4'
                ans = np.sin(radians_per_sample, dtype=self.envelope.dtype)
                if self.ampl_swing != 1:
                    ans *= self.ampl_swing
                if self.target:
                    ans += self.target
                yield ans
        finally:
            self.delete()

    def target_value(self):
        return self.target


class Sin(Envelope):
    r'''Can be used in one of two ways:

    1. passing cycle_time to envelope ctor.  This will always use the same cycle_time
       (but scales it based on the base_freq passed to start).  If add_base_freq is
       True, base_freq is added to the generated output.  Example:

           - harmonic.freq_envelope = Sin('vibrato', harmonic, 1/3, ampl_swing=50,
                                          add_base_freq=true)
           - harmonic.freq_gen = Sin('gen', harmonic)

       When note_on is received:

           - env_gen = harmonic.freq_envelope.start(base_freq)
             - will add base_freq to output generated (because add_base_freq is True)
           - sample_gen = harmonic.freq_gen.start(env_gen)
             - env_gen.target_value() used when env_gen runs out

    2. not passing cycle_time to envelope ctor.  In this case, it uses base_freq directly.
       Base_freq may be a scalar or a generator (in which case, one kind of generator could
       be from another Sin envelope with cycle_time set).  If base_freq is a generator and
       it stops generating, the Sin_generator using it will continue on with a fixed freq
       (the final target_value reported).
    '''
    generator = Sin_generator
    cycle_time = Notify_actors()

    def __init__(self, name, harmonic, cycle_time=None, center_ampl=None, ampl_swing=None,
                 duration=None, scale_3=None, add_base_freq=False):
        super().__init__(name, harmonic, duration, scale_3)
        assert cycle_time is not None or not add_base_freq, \
          f"{name}: add_base_freq only allowed with cycle_time"
        self.cycle_time = cycle_time
        self.ampl_swing = ampl_swing
        self.center_ampl = center_ampl
        self.add_base_freq = add_base_freq

