# alpha.py

from collections import defaultdict


Segments = frozenset("A B C D E F G1 G2 H I J K L M DP".split())

#        A       -----
#      FHIJB     |\|/|
#      G1 G2     -- --
#      EMLKC     |/|\|
#        D  DP   ----- .

Unknown_letter = frozenset("A B C D E F H J M K".split())

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
    ' ': frozenset("".split()),
    '/': frozenset("M J".split()),
    '\\': frozenset("H K".split()),
    '|': frozenset("I L".split()),
    '$': frozenset("A F G1 G2 C D I L".split()),
    '&': frozenset("C K H A J M D".split()),
    '*': frozenset("H I J M L K G1 G2".split()),
    '+': frozenset("I L G1 G2".split()),
    '=': frozenset("G1 G2 D".split()),
    '<': frozenset("J K".split()),
    '>': frozenset("H M".split()),
    '"': frozenset("H J".split()),
    "'": frozenset("J".split()),
    "`": frozenset("H".split()),
}

Segment_to_bit_number = {letter: i
                         for i, letter in enumerate(sorted(Segments.difference(["DP"])))}
Segment_to_bit_number["DP"] = 14
#print(f"{Segment_to_bit_number=}")

Col_ports = [
    ("e", 1),  # 0
    ("c", 6),  # 1
    ("c", 5),  # 2
    ("c", 4),  # 3
    ("e", 3),  # 4
    ("b", 2),  # 5
    ("b", 1),  # 6
    ("b", 0),  # 7
    ("d", 7),  # 8
    ("e", 0),  # 9
    ("d", 5),  # 10
    ("d", 4),  # 11
    ("d", 3),  # 12
    ("d", 2),  # 13
    ("d", 1),  # 14
    ("d", 0),  # 15
]

Port_order = "decb"

#print(f"{sorted(Col_ports)=}")

if False:
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
    from itertools import groupby
    from operator import itemgetter

    if False:
        print(len(Segments))
        print(len(Segment_map))
        print(len(Sets_by_segment))

    print("// Alpha_decoder.h")
    print()
    print("const struct col_ports_s Alpha_decoder[] = {")
    for i in range(128):
        c = chr(i).upper()
        #if c not in Segment_map:
        #    print("  {0b11111111, 0b11111111, 0b11111111, 0b11111111", end='')
        #else:
        ports = {port: 0 for port in Col_ports}
        for segment in Segment_map.get(c, Unknown_letter):
            ports[Col_ports[Segment_to_bit_number[segment]]] = 1 
        port_nums = {}
        for port, bits in groupby(sorted(ports.keys()), itemgetter(0)):
            n = 0
            for bit in bits:
                n |= ports[port, bit[1]] << bit[1]
            port_nums[port] = f"0b{n:08b}"
        print("  {", end='')
        print(', '.join((port_nums[port] if port in port_nums else "0b0")
                        for port in Port_order), end='')
        if i < 127:
            print("},")
        else:
            print("}")
    print("};")

