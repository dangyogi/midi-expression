# alpha.py

from collections import defaultdict


Segments = frozenset("A B C D E F G1 G2 J I H K L M DP".split())

Segment_map = {
    'A': frozenset("F H G1 E K".split()),
    'B': frozenset("A F J G1 E K D".split()),
    'C': frozenset("A F E D".split()),
    'D': frozenset("F H E M".split()),
    'E': frozenset("A F G1 E D".split()),
    'F': frozenset("A F G1 E".split()),
    'G': frozenset("A F G2 E C D".split()),
    'H': frozenset("F B G1 G2 E C".split()),
    'I': frozenset("A I L D".split()),
    'J': frozenset("A B E C D".split()),
    'K': frozenset("F J G1 E K".split()),
    'L': frozenset("F E D".split()),
    'M': frozenset("F H J B E C".split()),
    'N': frozenset("F H B E K C".split()),
    'O': frozenset("A F B E C D".split()),
    'P': frozenset("A F B G1 G2 E".split()),
    'Q': frozenset("A F B E K C D".split()),
    'R': frozenset("A F B G1 G2 E K".split()),
    'S': frozenset("A H G2 C D".split()),
    'T': frozenset("A I L".split()),
    'U': frozenset("F B E C D".split()),
    'V': frozenset("H B K C".split()),
    'W': frozenset("F B E M K C".split()),
    'X': frozenset("H J M K".split()),
    'Y': frozenset("H J L".split()),
    'Z': frozenset("A J M D".split()),
    '0': frozenset("A F J B E M C D".split()),
    '1': frozenset("I L".split()),
    '2': frozenset("A B G2 M D".split()),
    '3': frozenset("A B G2 C D".split()),
    '4': frozenset("F B G1 G2 C".split()),
    '5': frozenset("A F G1 G2 C D".split()),
    '6': frozenset("A F G1 G2 E C D".split()),
    '7': frozenset("A J M".split()),
    '8': frozenset("A F B G1 G2 E C D".split()),
    '9': frozenset("A F B G1 G2 C D".split()),
    '-': frozenset("G1 G2".split()),
    '_': frozenset("D".split()),
    '.': frozenset("DP".split()),
}

Sets_by_segment = defaultdict(list)

for segments in Segment_map.values():
    for segment in segments:
        Sets_by_segment[segment].append(segments)

print("Inclusive segments:")
for key, sequences in Sets_by_segment.items():
    segments = Segments.intersection(*sequences)
    assert len(segments) > 0, f"{key=}, {sequences=}"
    if len(segments) > 1:
        print(key, '->', sorted(segments - {key,}))

print()
print("Exclusive segments:")
for key, sequences in Sets_by_segment.items():
    segments = Segments.difference(*sequences)
    assert len(segments) > 0, f"{key=}, {sequences=}"
    if len(segments) > 1:
        print(key, '->', sorted(segments))



if __name__ == "__main__":
    print(len(Segments))
    print(len(Segment_map))
    print(len(Sets_by_segment))
