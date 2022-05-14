# tuning_system_study.py

import math
from itertools import chain


Cent = 2**(1/1200)
Ln_cent = math.log(Cent)

Pyth_comma = 3**12 / 2**19
Pyth_comma_cents = math.log(Pyth_comma) / Ln_cent
#print(f"{Pyth_comma=}, {Pyth_comma_cents=}")
P_5 = Pyth_comma_cents / 5

Syntonic_comma = 81/80
Syntonic_comma_cents = math.log(Syntonic_comma) / Ln_cent
#print(f"{Syntonic_comma=}, {Syntonic_comma_cents=}")

H3_cents = math.log(3/2) / Ln_cent
H5_cents = math.log(5/4) / Ln_cent


# 0=Ab, 1=Eb, 2=Bb, 3=F, 4=C, 5=G, 6=D, 7=A, 8=E, 9=B, 10=F#, 11=C#
Order = (0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5, 12)

def reorder(seq):
    seq = tuple(seq)
    return tuple(seq[i] for i in Order)

def trunc(cents):
    return cents % 1200

def even(name, delta):
    #print("even", name, delta)
    return (name,) + reorder(trunc(delta * i) for i in range(13))

def product(name, *deltas):
    i = 0
    ratios = [i]
    for d in deltas:
        i = trunc(i + d)
        ratios.append(i)
    return (name,) + reorder(ratios)

def to_cents(*seq):
    return seq[:1] + tuple(trunc(math.log(n) / Ln_cent) for n in seq[1:])

Tuning_systems = ((
    even("Pyth", H3_cents),
    even("1/4, MT", H3_cents - Syntonic_comma_cents/4),
    even("1/5, MT", H3_cents - Syntonic_comma_cents/5),
    even("eq temp", H3_cents - Pyth_comma_cents/12),
    product("well temp",
              H3_cents, H3_cents, H3_cents, H3_cents,
              H3_cents - P_5, H3_cents - P_5, H3_cents - P_5, H3_cents - P_5,
              H3_cents, H3_cents - P_5, H3_cents, H3_cents),
    ),(

    to_cents("Sym 5L 1A", 
               1, 16/15,  9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3, 16/9, 15/8, 2),
    to_cents("Sym 5L 1B", 
               1, 16/15,  9/8, 6/5, 5/4, 4/3, 64/45, 3/2, 8/5, 5/3, 16/9, 15/8, 2),
    to_cents("Sym 5L 2A", 
               1, 16/15, 10/9, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
    to_cents("Sym 5L 2B", 
               1, 16/15, 10/9, 6/5, 5/4, 4/3, 64/45, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
    ),(
    to_cents("Asym 5L std A", 
               1, 16/15,  9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
    to_cents("Asym 5L std B", 
               1, 16/15,  9/8, 6/5, 5/4, 4/3, 64/45, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
    to_cents("Asym 5L ext A", 
               1, 16/15,  9/8, 6/5, 5/4, 4/3, 25/18, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
    to_cents("Asym 5L ext B", 
               1, 16/15,  9/8, 6/5, 5/4, 4/3, 36/25, 3/2, 8/5, 5/3,  9/5, 15/8, 2),
    ),(
    to_cents("7-limit A", 
               1, 15/14,  8/7, 6/5, 5/4, 4/3,   7/5, 3/2, 8/5, 5/3, 7/4, 15/8, 2),
    to_cents("7-limit B", 
               1, 15/14,  8/7, 6/5, 5/4, 4/3,  10/7, 3/2, 8/5, 5/3, 7/4, 15/8, 2),
    to_cents("17-limit A", 
               1, 14/13,  8/7, 6/5, 5/4, 4/3,   7/5, 3/2, 8/5, 5/3, 7/4, 13/7, 2),
    to_cents("17-limit B", 
               1, 14/13,  8/7, 6/5, 5/4, 4/3,  10/7, 3/2, 8/5, 5/3, 7/4, 13/7, 2),
    to_cents("17-limit C", 
               1, 14/13,  8/7, 6/5, 5/4, 4/3, 17/12, 3/2, 8/5, 5/3, 7/4, 13/7, 2),
    to_cents("17-limit D", 
               1, 14/13,  8/7, 6/5, 5/4, 4/3, 24/17, 3/2, 8/5, 5/3, 7/4, 13/7, 2),
    ),
)


abs_tol = 3  # cents

def examine(tuning_system):
    extended_ts = tuning_system[:-1] + tuple(n + 1200 for n in tuning_system[1:8])
    #print(f"{extended_ts=}")
    for i in range(13):
        if i == 0:
            yield f'"{extended_ts[i]}"', '5th', '3rd', ''
        else:
            yield (f'"{extended_ts[i]}"',
              abs(extended_ts[i+7] - extended_ts[i] - H3_cents),
              abs(extended_ts[i+4] - extended_ts[i] - H5_cents),
              #abs((extended_ts[i+7]-extended_ts[i]) % 1200 - H3_cents),
              #abs((extended_ts[i+4]-extended_ts[i]) % 1200 - H5_cents),
              '',
            )


def examine_set(tuning_systems):
    for row in zip(*(tuple(examine(t)) for t in tuning_systems)):
        if False:
            print("row", row)
            print("row chained", tuple(chain.from_iterable(row)))
            break
        print(','.join(str(x) for x in chain.from_iterable(row)))
    print()


#print(tuple(examine(Tuning_systems[0])))

if True:
    for s in Tuning_systems:
        examine_set(s)
