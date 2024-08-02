# test_driver.py

import sys
import socket

def send_sock(data):
    bdata = data.encode('ascii')
    while bdata:
        len_sent = Sock.send(bdata)
        bdata = bdata[len_sent:]

Sock_buffer = ''

class Sock_closed(EOFError):
    pass

def sock_readline(recv_flags=0):
    global Sock_buffer
    newline = Sock_buffer.find('\n')
    while newline == -1:
        data_read = Sock.recv(4096, recv_flags).decode('ascii')
        if not data_read:
            print()
            print("Null recv, quiting connection")
            raise Sock_closed
        Sock_buffer += data_read
        newline = Sock_buffer.find('\n')
    ans = Sock_buffer[:newline]
    Sock_buffer = Sock_buffer[newline + 1:]
    #print("sock_readline returning:", ans)
    return ans

Defines = {}   # name: value
Classes = {}   # name: (subclasses)
Structs = {}   # struct_name: Struct()
Globals = {}   # name: Global()
Arrays = {}    # name: Array()
Functions = {} # name: Function()

def init():
    global Sock_buffer, Defines, Classes, Structs, Globals, Arrays
    Sock_buffer = ''
    Defines = {}  # name: value
    Classes = {}  # name: (subclasses)
    Structs = {}  # struct_name: Struct()
    Globals = {}  # name: Global()
    Arrays = {}   # name: Array()
    Functions = {} # name: Function()

def add_define(name, value):
    assert name not in Defines, f"{name} already #defined"
    Defines[name] = value

def add_subclasses(name, subclasses):
    assert name not in Classes, f"{name} already subclassed"
    Classes[name] = tuple(subclasses)

def unsigned(type):
    return type[0] == 'u' or type == 'byte'

class Struct:
    def __init__(self, name):
        self.name = name
        self.fields = {}

    def add_field(self, field):
        assert field.name not in self.fields, f"{field.name} already in {self.name}"
        self.fields[field.name] = field

class Field:
    def __init__(self, struct, name, offset, len, type):
        self.struct = struct
        self.name = name
        self.offset = offset
        self.len = len
        self.type = type
        self.unsigned = unsigned(type)

def add_field(struct, name, offset, len, type):
    if struct not in Structs:
        Structs[struct] = Struct(struct)
    Structs[struct].add_field(Field(struct, name, offset, len, type))

def from_hex(data, unsigned, len):
    # converts hex data received from client "get_global" command to integer.
    # data has a 0X prefix.
    assert len(data) == len + 2
    num = int(data, 16)
    if unsigned:
        return num
    if num & (1 << (len * 8 - 1)):
        # num is negative
        num -= 1 << len * 8
    return num

def to_hex(value, unsigned, len):
    # converts value to 0X hex data value for client "set_global" command.
    if unsigned:
        assert value >= 0
        assert value < (1 << len * 8)
        data = f"{value:#0{len*2+2}X}"
    elif value >= 0:
        assert value < (1 << (len * 8 - 1))
        data = f"{value:#0{len*2+2}X}"
    else: # value < 0
        assert value >= -(1 << (len * 8 - 1))
        value += 1 << len * 8
        data = f"{value:#0{len*2+2}X}"

class Global:
    def __init__(self, name, addr, len, type):
        self.name = name
        self.addr = addr
        self.len = len
        self.type = type
        self.unsigned = unsigned(type)

    def get(self):
        assert not Sock_buffer
        send_sock(f"get_global {self.addr} {self.len}\n")
        command, final_addr, data = sock_readline().split()
        assert command == 'get_global'
        assert int(final_addr) == self.addr
        return from_hex(data, self.unsigned, self.len)

    def set(self, value):
        # returns prev value
        assert not Sock_buffer
        send_sock(f"set_global {self.addr} {self.len} {to_hex(value, self.unsigned, self.len)}\n")
        command, final_addr, prev_data = sock_readline().split()
        assert command == 'set_global'
        assert int(final_addr) == self.addr
        return from_hex(prev_data, self.unsigned, self.len)

def add_global(name, addr, len, type):
    assert name not in Globals
    Globals[name] = Global(name, addr, len, type)

class Array:
    def __init__(self, name, addr, array_size, element_size, type, dims):
        self.name = name
        self.addr = addr
        self.array_size = array_size
        self.element_size = element_size
        self.type = type
        self.unsigned = unsigned(type)
        self.dims = dims

def add_array(name, addr, array_size, element_size, type, dims):
    assert name not in Arrays
    Arrays[name] = Array(name, addr, array_size, element_size, type, dims)

class Function:
    def __init__(self, name, expects_return, params):
        self.name = name
        self.ret = expects_return
        self.params = params

def add_function(name, expects_return, params):
    assert name not in Functions
    Functions[name] = Function(name, expects_return, params)

def load():
    while True:
        line = sock_readline()
        words = line.split()
        if words[0] == '#define':
            add_define(words[1], int(words[2]))
        elif words[0] == 'sub_classes':
            add_subclasses(words[1], words[2:])
        elif words[0] == 'field':
            add_field(words[1], words[2], int(words[3]), int(words[4]), ' '.join(words[5:]))
        elif words[0] == 'global':
            add_global(words[1], int(words[2]), int(words[3]), ' '.join(words[4:]))
        elif words[0] == 'array':
            head, dims_str = line.split(':')
            words = head.split()
            dims = [int(dim) for dim in dims_str.strip().split()]
            add_array(words[1], int(words[2]), int(words[3]), int(words[4]), 
                      ' '.join(words[4:]), dims)
        elif words[0] == 'function':
            add_function(words[1], int(words[2]), words[3:])
        elif words[0] == 'ready':
            break
        else:
            print("ERROR: unrecognized client command:", line, file=sys.stderr)

def run(port):
    global Sock

    listen_socket = socket.socket()
    listen_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    addr = 'localhost', port
    listen_socket.bind(('localhost', port))
    print("listening on", addr)
    listen_socket.listen()

    while True:
        init()
        print("Waiting for connection")
        Sock, addr = listen_socket.accept()
        print("got connection from", addr)
        print()
        Sock.settimeout(0.3)
        try:
            load()
            interactive()
        except Sock_closed:
            pass
        Sock.close()
        Sock = None

def interactive():
    print("client ready!")
    while True:
        print("> ", end='')
        sys.stdout.flush()
        request = sys.stdin.readline()
        send_sock(request)
        try:
            response = sock_readline()
            print("got", repr(response))
        except socket.timeout:
            pass



if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--port', '-p', type=int, default=2020)
    args = parser.parse_args()

    run(args.port)
