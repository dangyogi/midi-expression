# utils.py

import math
import os.path

from yaml import safe_load
#try:
#    from yaml import CSafeLoader as SafeLoader
#except ImportError:
#    from yaml import SafeLoader


Data_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "data")

def read_yaml(filename="synth_settings.yaml"):
    with open(os.path.join(Data_dir, filename), "r") as yaml_file:
        return safe_load(yaml_file)


def calc_geom(bits, limit, b=0, start=0):
    # ans = e**(mx + b) + c
    max_param_value = 2**bits - 1
    m = math.log((limit - start) / math.exp(b) + 1) \
	     / max_param_value
    c = start - math.exp(b)
    assert math.isclose(math.exp(b) + c, start)
    assert math.isclose(math.exp(m * max_param_value + b) + c, limit)
    return m, c




if __name__ == "__main__":
    import argparse

    argparser = argparse.ArgumentParser()
    argparser.add_argument("-s", "--start", type=float, default=0)
    argparser.add_argument("limit", type=float)
    argparser.add_argument("bits", type=float)

    args = argparser.parse_args()

    max_param_value = 2**args.bits - 1

    for i in range(-4, 5):
        b = i / 2
        m, c = calc_geom(args.bits, args.limit, b, args.start)
        last_step = math.exp(m*max_param_value+b)+c - \
                    (math.exp(m*(max_param_value - 1) + b)+c)
        print(f"{b:4.1f}: first step {math.exp(m+b)+c - args.start:.3f}, "
              f"last step {last_step:.3f}")

