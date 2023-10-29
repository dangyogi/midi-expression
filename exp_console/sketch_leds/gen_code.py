# gen_code.py

from collections import defaultdict


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

#Port_nums = dict(B=1, C=2, D=3, E=4)


def gen_on():
    for i, (port, num) in enumerate(Assignments):
        n = int(num)
        bit = '0' * (7 - n) + '1' + '0' * n
        print(f"  case {i:2}: Col_ports[row].port_{port.lower()} |= 0b{bit}; break;")

def gen_off():
    for i, (port, num) in enumerate(Assignments):
        n = int(num)
        bit = '0' * (7 - n) + '1' + '0' * n
        print(f"  case {i:2}: Col_ports[row].port_{port.lower()} &= ~0b{bit}; break;")

def gen_decode(file, start=0):
    high_low = 'low' if start else 'high'
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
            if bits & (1 << bit_num):
                ports[port] |= 1 << int(num)
        for key, bits in ports.items():
            ports_used[key] |= bits
        ans.append(f"{{0b{ports['D']:08b}, 0b{ports['B']:08b}, "
                   f"0b{ports['C']:08b}, 0b{ports['E']:08b}}}")
    return ans, ports_used



if __name__ == "__main__":
    #print(len(Assignments))
    print()
    gen_on()
    print()
    gen_off()
    with open("decode_masks.h", "w") as f:
        print("//", "decode_masks.h", file=f)
        gen_decode(f)
        gen_decode(f, 8)
