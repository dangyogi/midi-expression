# gen_col_ports_maps.py

from collections import defaultdict

from col_ports import *

#Port_nums = dict(B=1, C=2, D=3, E=4)


def gen_col_on(f):
    print("void col_on(col_ports_t *col, byte col_num) {", file=f)
    print("  switch (col_num) {     // 76543210", file=f)
    for i, (port, num) in enumerate(Assignments):
        n = int(num)
        bit = '0' * (7 - n) + '1' + '0' * n
        print(f"  case {i:2}: col->port_{port.lower()} |= 0b{bit}; break;", file=f)
    print("  }", file=f)
    print("}", file=f)

def gen_col_off(f):
    print("void col_off(col_ports_t *col, byte col_num) {", file=f)
    print("  switch (col_num) {      // 76543210", file=f)
    for i, (port, num) in enumerate(Assignments):
        n = int(num)
        bit = '0' * (7 - n) + '1' + '0' * n
        print(f"  case {i:2}: col->port_{port.lower()} &= ~0b{bit}; break;", file=f)
    print("  }", file=f)
    print("}", file=f)

def gen_decode(file, start=0):
    high_low = 'high' if start else 'low'
    col_ports, ports_used = decode(Assignments[start: start + 8])
    print(file=file)
    print("// The unused bits in each port are always 0 here.", file=file)
    print(f"const col_ports_t Decode_{high_low}[] PROGMEM = {{", file=file)
    for i, ports in enumerate(col_ports):
        print(f"  {ports},   // {i}", file=file)
    print("};", file=file)
    print(file=file)
    print(f"const col_ports_t Masks_{high_low} = {{   "
          f"// 1 bits are not used by any of the {high_low} columns", file=file)
    print("  ", end='', file=file)
    for port in "DBCE":
        unused_bits = ports_used[port] ^ 0xFF
        print(f"0b{unused_bits:08b}, ", end='', file=file)
    print(file=file)
    print("};", file=file)
    print(file=file)

def decode(assignments):
    ans = []
    ports_used = defaultdict(int)
    for bits in range(256):
        ports = defaultdict(int)
        for bit_num, (port, num) in zip(range(8), assignments):
            if bits & (0b10000000 >> bit_num):
                ports[port] |= 1 << int(num)
        for key, bits in ports.items():
            ports_used[key] |= bits
        ans.append(f"{{0b{ports['D']:08b}, 0b{ports['B']:08b}, "
                   f"0b{ports['C']:08b}, 0b{ports['E']:08b}}}")
    return ans, ports_used



if __name__ == "__main__":
    #print(len(Assignments))
    with open("col_ports_maps.h", "w") as f:
        print("//", "col_ports_maps.h", file=f)
        print(file=f)
        gen_col_on(f)
        print(file=f)
        gen_col_off(f)
        gen_decode(f)           # low:  C0-7
        gen_decode(f, 8)        # high: C8-15
