# segments.py

import re


letter = re.compile(r'([^ ]|unknown) .*?([A-Z][A-Z12 ]*)$')


if __name__ == "__main__":
    with open("ALPHA_MAP", "rt") as in_file, \
         open("letters.py", 'wt') as out_file:
        print("# letters.py", file=out_file)
        print(file=out_file)
        print("Segment_map = {", file=out_file)
        for line in in_file:
            m = letter.match(line.rstrip())
            if m:
                print(f"got {m.group(1)=}, {m.group(2)=}")
                print(f"    {m.group(1)!r}: frozenset({m.group(2)!r}.split()),", file=out_file)
            elif line and line[0].isalpha():
                print(f"{line.rstrip()!r} does not match")
        print("    ' ': frozenset(),", file=out_file)
        print(r"    '\t': frozenset()", file=out_file)
        print("}", file=out_file)


