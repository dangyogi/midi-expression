# test_driver.py

import sys
import os.path
from collections import Counter
import socket

from utils import *


Trace = False

class Sock_closed(EOFError):
    pass

def send_sock(data):
    bdata = data.encode('ascii')
    if Trace:
        print("send_sock sending:", repr(bdata))
    while bdata:
        len_sent = Sock.send(bdata)
        if len_sent == 0:
            print()
            print("0 send, quiting connection")
            raise Sock_closed
        bdata = bdata[len_sent:]
    if Trace:
        print("send_sock done")

Sock_buffer = ''

def sock_readline(recv_flags=0):
    # Returns next line with the trailing '\n' stripped.
    global Sock_buffer
    if Trace:
        print("sock_readline called, recv_flags", recv_flags)
    newline = Sock_buffer.find('\n')
    while newline == -1:
        if Trace:
            print("sock_readline calling recv")
        data_read = Sock.recv(4096, recv_flags).decode('ascii')
        if not data_read:
            print()
            print("Null recv, quiting connection")
            raise Sock_closed
        Sock_buffer += data_read
        newline = Sock_buffer.find('\n')
    ans = Sock_buffer[:newline]
    Sock_buffer = Sock_buffer[newline + 1:]
    if Trace:
        print("sock_readline returning:", repr(ans))
    return ans

Defines = {}   # name: value
Classes = {}   # name: (subclasses)
Structs = {}   # struct_name: Struct()
Globals = {}   # name: Global()
Arrays = {}    # name: Array()
Functions = {} # name: Function()

Defaults = {}  # fname: default_return_value

Seq_numbers = Counter() # fname: seq_number

Sketch_dir = None

def init():
    global Sock_buffer, Defines, Classes, Structs, Globals, Arrays, Functions, Defaults, Seq_numbers
    global Sketch_dir
    Sock_buffer = ''
    Defines = {}   # name: value
    Classes = {}   # name: (subclasses)
    Structs = {}   # struct_name: Struct()
    Globals = {}   # name: Global()
    Arrays = {}    # name: Array()
    Functions = {} # name: Function()
    Defaults = {}  # name: default_return_value
    Seq_numbers = Counter()  # fname: seq_number
    Sketch_dir = None

def add_define(name, value):
    assert name not in Defines, f"{name} already #defined"
    Defines[name] = value

def add_subclasses(name, subclasses):
    assert name not in Classes, f"{name} already subclassed"
    Classes[name] = tuple(subclasses)

def unsigned(type):
    return type[0] == 'u' or type == 'byte'

def get_global_args(fields, type, ggtype, next_poffset, poffsets=None):
    # fields is a list of strings
    # returns type, poffsets
    if poffsets is None:
        poffsets = []

    while fields:
        if type.endswith('*'):
            poffsets.append(str(next_poffset))
            next_poffset = 0
            type = type[:-1].strip()
        if type in Classes:
            subclass = fields.pop(0)
            assert subclass in Classes[type]
            assert subclass in Structs
        if type in Structs:
            struct = Structs[type]
            field = struct.get_field(fields.pop(0))
            next_poffset += field.offset
            type = field.type
            ggtype = field.ggtype
        else:
            print(f"ERROR: expected Struct type, ended up with {type=}", file=sys.stderr)
            sys.exit(1)

    assert ggtype
    poffsets.append(str(next_poffset))
    return ggtype, poffsets

class Struct:
    def __init__(self, name):
        self.name = name
        self.fields = {}

    def add_field(self, field):
        assert field.name not in self.fields, f"{field.name} already in {self.name}"
        self.fields[field.name] = field

    def get_field(self, field_name):
        return self.fields[field_name]

class Field:
    def __init__(self, struct, name, offset, len, type):
        self.struct = struct
        self.name = name
        self.offset = offset
        self.len = len
        self.type = type
        self.ggtype = to_ggtype(type, len)

def to_ggtype(type, len):
    # returns type for "get_global" command: s<len>, u<len>, f<len>, str, <data_len>
    if integer(type):
        if unsigned(type):
            return f'u{len}'
        return f's{len}'
    if double_float(type):
        if 'float' in type:
            return 'f4'
        return 'f8'
    # FIX: doesn't currenly handle "str" or "<len>", but not needed anyhow.
    return None

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
        self.ggtype = to_ggtype(type, len)

    def global_args(self, fields):
        # fields is a list of strings
        # returns type, offsets
        return get_global_args(fields, self.type, self.ggtype, self.addr)

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
        self.ggtype = to_ggtype(type, element_size)
        self.dims = dims

    def global_args(self, words_in):
        # words_in is a list of strings
        # returns type, offsets
        addr = self.addr
        if Trace:
            print(f"{self.name}.global_args, {self.addr=}, {self.element_size=}, {self.type=}")
            print(f"  {self.ggtype=}, {self.dims=}")
        for i in range(len(self.dims)):
            arg = translate_word(words_in.pop(0))
            dim_in = int(arg)
            addr += dim_in * product(self.dims[i + 1:]) * self.element_size
            if Trace:
                print(f"{self.name}.global_args, got arg={dim_in}, {addr=}")
        return get_global_args(words_in, self.type, self.ggtype, addr)

def product(iterable):
    ans = 1
    for i in iterable:
        ans *= i
    return ans

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
    # loads all of the initial "#define", "sub_classes", "field", "global", "array" and "function"
    # info from the C++ program.
    global Sketch_dir
    while True:
        line = sock_readline()
        words = line.split()
        if words[0] == 'sketch_dir':
            assert Sketch_dir is None
            Sketch_dir = words[1]
        elif words[0] == '#define':
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
                      ' '.join(words[5:]), dims)
        elif words[0] == 'function':
            add_function(words[1], int(words[2]), words[3:])
        elif words[0] == 'ready':
            break
        else:
            print("ERROR: unrecognized client command:", line, file=sys.stderr)

def load_defaults(filename):
    with open(os.path.join(Sketch_dir, filename)) as defaults_file:
        for line in defaults_file:
            line = strip_comment(line)
            if not line:
                continue
            if ' ' in line:
                name, ret = line.split()
                assert name not in Defaults
                Defaults[name] = str(ret)   # FIX: change when test_scripts.yaml is used...
            else:
                assert line not in Defaults
                Defaults[line] = None

def run(port, verbose):
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
        #Sock.settimeout(0.3)
        try:
            load()
            load_defaults("defaults")
            interactive(verbose)
        except Sock_closed:
            pass
        Sock.close()
        Sock = None

def translate(request):
    words_in = request.split()
    words_out = []
    if words_in[0] == 'get':
        type, offsets = make_get_global(words_in[1:])
        words_out.append('get_global')
        words_out.append(type)
        words_out.extend(offsets)
    else:
        words_out.append(words_in[0])
        for word in words_in[1:]:
            words_out.append(translate_word(word))
    return ' '.join(words_out) + '\n'

def translate_word(word):
    if word[0] in "0123456789+-":
        return word
    if word in Defines:
        return Defines[word]
    return word

def make_get_global(words_in):
    # words_in: global_name/array_name dims fields
    # returns type, poffsets
    # returned poffsets are strings, not ints
    if words_in[0] in Globals:
        type, offsets = Globals[words_in[0]].global_args(words_in[1:])
    elif words_in[0] in Arrays:
        type, offsets = Arrays[words_in[0]].global_args(words_in[1:])
    else:
        printf(f"ERROR: Unrecognized Global/Array {words_in[0]!r}", file=sys.stderr)
        sys.exit(2)
    return type, offsets


def compare(response, script_line):
    rwords = response.split()
    swords = script_line.split()
    if len(rwords) != len(swords):
        print(f"ERROR: different number of words in response=%{response!r} and script={script_line!r}",
              file=sys.stderr)
        sys.exit(1)
    for i, (rword, sword) in enumerate(zip(rwords, swords), 1):
        if sword != '.' and rword != translate_word(sword):
            print(f"ERROR: in response=%{response!r}, "
                  f"word {i}={rword!r} does not match script={sword!r}",
                  file=sys.stderr)
            sys.exit(1)

def strip_comment(line):
    # strips trailing '\n'
    #
    # does not strip trailing spaces if no comment.
    line = line[: -1]
    comment_start = line.find('#')
    if comment_start >= 0:
        line = line[: comment_start]
        return line.strip()
    return line

def run_script(filename, verbose):
    with open(os.path.join(Sketch_dir, filename)) as script_file:
        Seq_numbers = Counter()
        for line_no, line in enumerate(script_file, 1):
            line = strip_comment(line)
            if not line:
                continue
            if line[0] == '<':
                translated_request = translate(request[1:].lstrip())
                print('<', translated_request, end='')
                sock_send(translated_request)
            elif line[0] == '>':
                while True:
                    response = sock_readline()
                    if not do_default(response):
                        expect = line[1:].lstrip()
                        if expect != response:
                            print("ERROR: expected '{expect!r}', got '{response!r}'", file=sys.stderr)
                            sys.exit(1)
                        print(">", repr(response))
                        break

def do_icommand(request, verbose):
    # request includes trailing '\n'
    global Trace
    if request.startswith('?'):
        print(translate(request[1:].strip()), end='')
    elif request.startswith('trace '):
        words = request.split()
        if words[1] == 'on':
            Trace = 1
        else:
            Trace = 0
    elif request.startswith('readline'):
        try:
            print("sock_readline returned:", repr(sock_readline()))
        except socket.timeout:
            print("sock_readline: socket.timeout")
    elif request.startswith('run '):
        words = request.split()
        assert len(words) == 2, f'invalid "run" request: expected 2 words, got {len(words)}'
        run_script(words[1], verbose)
    else:
        translated_request = translate(request.strip())
        if Trace:
            print(f"{translated_request=!r}")
        send_sock(translated_request)
        try:
            while True:
                response = sock_readline()
                if not do_default(response, verbose):
                    # don't know this one, show it to user and let them respond...
                    print(">", repr(response))
                    break
        except socket.timeout:
            print("do_icommand: socket.timeout")

def do_default(cpp_command, verbose):
    if not cpp_command.startswith('fun_called '):
        return False
    words = cpp_command.split()
    if words[1] not in Defaults:
        return False
    result = Defaults[words[1]]
    while True:
        if isinstance(result, dict):
            cpp_cmd, fname, param0, *params = cpp_command.split()
            result = result[param0]
        elif isinstance(result, (list, tuple)):
            cpp_cmd, fname, *params = cpp_command.split()
            result = result[Seq_numbers[fname]]
            Seq_numbers[fname] += 1
        elif result is None:
            if verbose:
                print(">", cpp_command)
                print("< return")
            send_sock("return\n")
            return True
        else:
            if verbose:
                print(">", cpp_command)
                print("< return", result)
            send_sock("return {result}\n")
            return True

def interactive(verbose):
    print("client ready!")
    while True:
        print("< ", end='')
        sys.stdout.flush()
        request = sys.stdin.readline()
        do_icommand(request, verbose)



if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--port', '-p', type=int, default=2020)
    parser.add_argument('--verbose', '-v', action='store_true')
    parser.add_argument('--trace', '-t', action='store_true')
    args = parser.parse_args()

    if args.trace:
        Trace = True

    run(args.port, args.verbose)
