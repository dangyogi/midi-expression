# just.py

r'''Tries to figure out a 5-limit just intonation.

Neither Fb nor B# appear in any major or minor chord.  Thus, there are 19 notes of concern
here...

Only 15 notes have a higher M3.  All only have one M3 higher, and all of the higher notes
only have one lower:

    'A',       'Ab',
    'B',       'Bb',
    'C', 'C#', 'Cb',
    'D',       'Db',
    'E',       'Eb',
    'F', 'F#',
    'G',       'Gb'

Only 18 notes have a higher M5.  All only have one M5 higher, and all of the higher notes
only have one lower:

    'A', 'A#', 'Ab',
    'B',       'Bb',
    'C', 'C#', 'Cb',
    'D', 'D#', 'Db',
    'E',       'Eb',
    'F', 'F#',
    'G', 'G#', 'Gb'

'''

from collections import defaultdict
from itertools import chain


Note_order = "FCGDAEB"

Major1s = tuple(chain((n + 'b' for n in Note_order[1:]),
                      Note_order,
                      (n + '#' for n in Note_order[:2])))
Major2s = tuple(chain((n + 'b' for n in Note_order[-2:]),
                      Note_order,
                      (n + '#' for n in Note_order[:6])))
Major3s = tuple(chain((n + 'b' for n in Note_order[2:]),
                      Note_order,
                      (n + '#' for n in Note_order[:3])))
Majors = tuple(zip(Major1s, Major2s, Major3s))
#print(Major1s)
#print(Major2s)
#print(Major3s)
#print(Majors)

Minor1s = tuple(chain((n + 'b' for n in Note_order[-3:]),
                      Note_order,
                      (n + '#' for n in Note_order[:5])))
Minor2s = Major1s
Minor3s = Major2s
Minors = tuple(zip(Minor1s, Minor2s, Minor3s))
#print(Minor1s)
#print(Minor2s)
#print(Minor3s)
#print(Minors)


M3a = sorted((c, e) for c, e, g in Majors)
M3b = sorted((c, e) for a, c, e in Minors)

assert M3a == M3b

#print(len(M3a), M3a)

M3 = defaultdict(set)
for low, high in M3a:
    M3[low].add(high)
for low, high in M3b:
    M3[low].add(high)

#M3['A'].add('bob')
assert all(len(s) == 1 for s in M3.values())

M5a = sorted((c, g) for c, e, g in Majors)
M5b = sorted((a, e) for a, c, e in Minors)

M5 = defaultdict(set)
for low, high in M5a:
    M5[low].add(high)
for low, high in M5b:
    M5[low].add(high)

assert all(len(s) == 1 for s in M5.values())

#print("M3", M3)
#print()
#print("M5", M5)

M3_keys = sorted(M3.keys())
M3_uppers = sorted(tuple(v)[0] for v in M3.values())
#print(len(M3_keys), "M3 keys", M3_keys)
#print(len(M3_uppers), "M3 uppers", M3_uppers)

M5_keys = sorted(M5.keys())
M5_uppers = sorted(tuple(v)[0] for v in M5.values())
#print(len(M5_keys), "M5 keys", M5_keys)
#print(len(M5_uppers), "M5 uppers", M5_uppers)

# Strip sets as values:
M3 = {k: tuple(v)[0] for k, v in M3.items()}
M5 = {k: tuple(v)[0] for k, v in M5.items()}

if False:
    print("M3 links:")
    seen = set()
    for k, v in sorted(M3.items()):
        print(k, v)

    print()
    print("M5 links:")
    seen = set()
    for k, v in sorted(M5.items()):
        print(k, v)

def note_order(d):
    keys = frozenset(d.keys())
    values = frozenset(d.values())
    for first in sorted(keys.difference(values)):
        print(tuple(yield_chain(first, d)))

def yield_chain(first, d):
    #print("yield_chain got", first, d)
    yield first
    if first in d:
        yield from yield_chain(d[first], d)


print("M3 order:")
note_order(M3)

print()
print("M5 order:")
note_order(M5)
