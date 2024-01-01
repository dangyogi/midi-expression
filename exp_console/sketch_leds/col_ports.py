# col_ports.py

Assignments = tuple(reversed((
    "D7",       # C15
    "D3",       # C14
    "D2",       # C13
    "D1",       # C12
    "D0",       # C11
    "D4",       # C10
    "D5",       # C9
    "E1",       # C8

    "E0",       # C7
    "B1",       # C6
    "B0",       # C5
    "E3",       # C4
    "B2",       # C3
    "C6",       # C2
    "C5",       # C1
    "C4",       # C0
)))

Port_order = "DBCE"


# Maps alpha display 14-segment names to bit numbers
Segment_to_bit_number = {segment: i for i, segment in enumerate("A B C D E F G1 G2 H I J K L M DP".split())}


if __name__ == "__main__":
    print(f"{Assignments=}")
    print(f"{Port_order=}")
    print(f"{Segment_to_bit_number=}")
