# sinewaves.py

from functools import partial
import argparse

from utils import *


arg_parser = argparse.ArgumentParser()
arg_parser.add_argument('--duration', '-d', type=float, default=1)

class parse_attack(argparse.Action):
    def __init__(self, option_strings, dest, **kwargs):
        self.attack_fn = kwargs.pop('attack_fn')
        super().__init__(option_strings, dest, nargs=2, default=None,
                         metavar=("INTERVAL", "DURATION"),
                         #help=f"{self.attack_fn.__name__}(interval, duration)",
                         **kwargs)
    def __call__(self, parser, namespace, values, option_string):
        #print("attack got", values)
        setattr(namespace, self.dest, self.attack_fn(Notes[values[0]], float(values[1])))
arg_parser.add_argument('--up', '-U', action=parse_attack, attack_fn=up)
arg_parser.add_argument('--down', '-D', action=parse_attack, attack_fn=down)

class create_adsr(argparse.Action):
    def __call__(self, parser, namespace, values, option_string):
        #print("create_adsr", repr(values))
        setattr(namespace, self.dest, partial(adsr, *values))
arg_parser.add_argument('--adsr', '-a', nargs=4,
                        metavar=("ATTACK", "DECAY", "SUSTAIN", "RELEASE"),
                        action=create_adsr,
                        type=float, default=lambda dur: None)

class parse_harmonic(argparse.Action):
    max_mul = 13
    def __init__(self, option_strings, dest, **kwargs):
        super().__init__(option_strings, dest, default=[],
                         metavar="MULTIPLE[/AMP_FN][/AMP_ADJ]",
                         help="AMP_FN is ampl_over_x_pow:pow or ampl_exp:base",
                         **kwargs)
    def __call__(self, parser, namespace, values, option_string):
        print("harmonic got", values)
        args = values.split('/')
        assert 1 <= len(args) <= 3
        mul = args[0]
        print("harmonic", mul, args)
        amp_fn = None
        amp_adj = 1
        if len(args) == 3:
            amp_fn = self.parse_amp_fn(args[1])
            amp_adj = float(args[2])
        elif len(args) == 2:
            if args[1][0].isdigit():
                amp_adj = float(args[1])
            else:
                amp_fn = self.parse_amp_fn(args[1])

        harmonics = getattr(namespace, self.dest)
        if mul[0].isdigit():
            harmonics.append(harmonic(float(mul), amp_fn, amp_adj))
        else:
            args = mul.split(':', maxsplit=1)
            name = args[0]
            if len(args) > 1:
                max_mul = int(args[1])
            else:
                max_mul = self.max_mul
            if name == 'even':
                for m in range(2, max_mul, 2):
                    harmonics.append(harmonic(m, amp_fn, amp_adj))
            elif name == 'odd':
                for m in range(3, max_mul, 2):
                    harmonics.append(harmonic(m, amp_fn, amp_adj))
            elif name == 'all':
                for m in range(2, max_mul):
                    harmonics.append(harmonic(m, amp_fn, amp_adj))
            else:
                parser.error(f"invalid harmonic multiple: {name}")

    def parse_amp_fn(self, args):
        name, *fn_args = args.split(':', maxsplit=1)
        if not fn_args:
            return ampl_fns[name]
        return ampl_fns[name + ':'](*[float(arg) for arg in fn_args])
arg_parser.add_argument('--harmonic', '-H', action=parse_harmonic)

args = arg_parser.parse_args()


duration = args.duration

# attack
#attack = up(M4, 0.05)
#attack = down(M4, 0.05)
if args.up is not None:
    attack = args.up
else:
    attack = args.down  # might be None


adsr_curve = args.adsr(duration)


# harmonics

# Unusual to encounter harmonics higher than the 5th on stringed instruments
# (except the double bass).

# even 0.5, odd 0 sounds a bit like a pipe organ
# even 0, odd 0.5 sounds a bit like a woodwind
harmonics = args.harmonic

print("duration", duration)
print("attack", attack)
print("adsr_curve", adsr_curve)
print("harmonics", harmonics)


if adsr_curve is None:
    play(note(freq_envelope(duration, attack) + nat(C), *harmonics))
    play(note(freq_envelope(duration, attack) + nat(E), *harmonics))
    play(note(freq_envelope(duration, attack) + nat(G), *harmonics))
else:
    play(adsr_curve * note(freq_envelope(duration, attack) + nat(C), *harmonics))
    play(adsr_curve * note(freq_envelope(duration, attack) + nat(E), *harmonics))
    play(adsr_curve * note(freq_envelope(duration, attack) + nat(G), *harmonics))

# chord notes
#play(note(freq_envelope(duration, attack) * nat(C), *harmonics)
#   + note(freq_envelope(duration, attack) * nat(E), *harmonics)
#   + note(freq_envelope(duration, attack) * nat(G), *harmonics))
