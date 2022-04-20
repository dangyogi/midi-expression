# utils.py


r'''
'''

import math

from .channel import Var, Actor



class Largest_value_calculator:
    r'''Computes largest expected value using mean and stddev of samples.

    From https://en.wikipedia.org/wiki/Standard_deviation#Rapid_calculation_methods.
    '''
    def __init__(self, name):
        self.name = name
        self.num_values = 0     # number of data points added
        self.mean = 0.0   # mean0 = 0
                          # meann = sum(x, n-1) / n-1 - sum(x, n-1) / n-1 / n + xn / n
                          #         sum(x, n-1) * (1/n-1 - 1/(n-1)*n) + xn/n
                          #         sum(x, n-1) * (n-1)/n*(n-1) + xn/n
                          #         sum(x, n-1)/n + xn/n
        self.Q = 0.0
        self.max_value = -1e20

    def add(self, value):
        self.num_values += 1
        delta = value - self.mean
        self.mean += delta / self.num_values
        self.Q += delta * (value - self.mean)
        if value > self.max_value:
            self.max_value = value

    def stddev(self):
        return math.sqrt(self.Q / self.num_values)

    def get_largest_value(self, num_stddev=2):
        if self.num_values == 0:
            return 0.010
        return self.mean + num_stddev*self.stddev()

    def report(self, scale=1, units=''):
        print(self.name, "final largest value", self.get_largest_value() * scale, units)
        print(self.name, "max value", self.max_value * scale, units)


class Target_time_actor(Actor):
    def __init__(self):
        super().__init__(Num_harmonics)

    def set_target_time(self, target_time, harmonic_gen_time):
        self.dac_target_time = target_time
        self.harmonic_gen_time = harmonic_gen_time
        self.target_time = \
                 self.dac_target_time - Num_harmonics.value * self.harmonic_gen_time

    def recalc(self):
        self.set('target_time',
                 self.dac_target_time - Num_harmonics.value * self.harmonic_gen_time)


Num_harmonics = Var('Num_harmonics', value=0)
print(f"{id(Num_harmonics)=}, {Num_harmonics.value=}")
Target_time = Target_time_actor()

