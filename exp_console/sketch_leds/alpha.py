# alpha.py

from collections import defaultdict

from letters import Segment_map
from col_ports import Assignments, Port_order, Segment_to_bit_number


Segments = frozenset("A B C D E F G1 G2 H I J K L M DP".split())

#        A       -----
#      FHIJB     |\|/|
#      G1 G2     -- --
#      EMLKC     |/|\|
#        D  DP   ----- .

Unknown_letter = Segment_map['unknown']

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
    print("const col_ports_t Alpha_decoder[] = {")
    for i in range(128):
        c = chr(i)
        c_upper = c.upper()
        ports = [Assignments[Segment_to_bit_number[segment]]
                 for segment in Segment_map.get(c_upper, Unknown_letter)]
        port_nums = {}
        for port, bits in groupby(sorted(ports), itemgetter(0)):
            #bits = tuple(bits)
            #print(f"Got {port=}, {bits=}")
            n = 0
            for bit in bits:
                n |= 1 << int(bit[1])
            port_nums[port] = f"0b{n:08b}"
        print("  {", end='')
        print(', '.join((port_nums[port] if port in port_nums else "0b00000000")
                        for port in Port_order), end='')
        if i < 127:
            print("},    //", hex(i), end='')
        else:
            print("}     //", hex(i), end='')
        if c_upper not in Segment_map:
            print(f" {chr(i)!r} <unknown>");
        else:
            print(f" {chr(i)!r}");
    print("};")

