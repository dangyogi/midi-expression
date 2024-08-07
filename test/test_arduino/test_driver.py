# test_driver.py

import sys
import os.path
from collections import Counter, deque, defaultdict
import readline
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
Report_output = deque()       # lines output from internal commands, with no trailing '\n'

def sock_readline(sock_only=False, recv_flags=0):
    # Returns next line with the trailing '\n' stripped.
    global Sock_buffer
    if Trace:
        print("sock_readline called, recv_flags", recv_flags)
    #if Report_output and not sock_only:
    #    return Report_output.popleft()
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
Lookups = defaultdict(dict)   # type: {value: name}
Classes = {}   # name: (subclasses)
Structs = {}   # struct_name: Struct()
Globals = {}   # name: Global()
Arrays = {}    # name: Array()
Functions = {} # name: Function()

Seq_numbers = Counter() # fname: seq_number

Sketch_dir = None
Script = None
Current_script = None

def init():
    global Sock_buffer, Report_output, Defines, Lookups, Classes, Structs, Globals, Arrays
    global Functions, Seq_numbers, Sketch_dir, Script, Current_script
    Sock_buffer = ''
    Report_output = deque()
    Defines = {}   # name: int(value)
    Lookups = defaultdict(dict)   # type: {value: name}
    Classes = {}   # name: (subclasses)
    Structs = {}   # struct_name: Struct()
    Globals = {}   # name: Global()
    Arrays = {}    # name: Array()
    Functions = {} # name: Function()
    Seq_numbers = Counter()  # fname: seq_number
    Sketch_dir = None
    Script = None
    Current_script = None

def add_define(name, value):
    assert name not in Defines, f"{name} already #defined"
    Defines[name] = value

def add_lookup(type, value, name):
    assert value not in Lookups[type], f"{value} already stored in Lookups as {Lookups[type][value]}"
    Lookups[type][value] = name

def add_subclasses(name, subclasses):
    assert name not in Classes, f"{name} already subclassed"
    Classes[name] = tuple(subclasses)

def unsigned(type):
    return type[0] == 'u' or type == 'byte'

def get_global_args(fields, type, ggtype, next_poffset, poffsets=None):
    # fields is a list of strings
    # returns type, poffsets
    if Trace:
        print(f"get_global_args({fields=}, {type=}, {ggtype=}, {next_poffset=}, {poffsets=}")
    if poffsets is None:
        poffsets = []

    fields = fields.copy()

    while fields:
        if Trace:
            print(f"get_global_args examining {type=} with {fields=} and {next_poffset=}")
        if type.endswith('*'):
            poffsets.append(str(next_poffset))
            next_poffset = 0
            type = type[:-1].strip()
        if type in Classes and fields[0] in Classes[type]:
            subclass = fields.pop(0)
            #assert subclass in Classes[type], f"{subclass=} not in Classes[{type}]"
            #assert subclass in Structs, f"{subclass=} not in Structs"
            type = subclass
        if type in Structs:
            struct = Structs[type]
            field = struct.get_field(fields.pop(0))
            next_poffset += field.offset
            type = field.type
            ggtype = field.ggtype
        else:
            print(f"ERROR: expected Struct type, ended up with {type=}", file=sys.stderr)
            sys.exit(1)

    poffsets.append(str(next_poffset))
    if type.endswith('*'):
        if Trace:
            print(f"get_global_args for pointer -> u4, {poffsets}")
        return 'u4', poffsets
    assert ggtype, f"no ggtype, not enough fields to get to base type"
    if Trace:
        print(f"get_global_args for base type -> {ggtype}, {poffsets}")
    return ggtype, poffsets

def get_completion_choices(type, words_in, verbose):
    if verbose:
        print(f"get_completion_choices({type=}, {words_in=})")
    words_in = words_in.copy()
    while True:
        if verbose:
            print(f"get_completion_choices examining {type=}")
        if type.endswith('*'):
            type = type[:-1].strip()
        if type in Classes:
            if verbose:
                print(f"get_completion_choices type in Classes with {words_in=}")
            if words_in:
                if words_in[0] in Classes[type]:
                    subclass = words_in.pop(0)
                    assert subclass in Structs, f"{subclass=} not in Structs"
                    type = subclass
            else:
                ans = list(Classes[type]).copy()
                ans.extend(Structs[type].fields.keys())
                return ans
        if type in Structs:
            if verbose:
                print(f"get_completion_choices type in Structs with {words_in=}")
            if words_in:
                struct = Structs[type]
                field = struct.get_field(words_in.pop(0))
                type = field.type
            else:
                return list(Structs[type].fields.keys())
        else:
            if verbose:
                print(f"get_completion_choices unknown {type=}")
            return []

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

# FIX: Not used
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

# FIX: Not used
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

    def get_completion_choices(self, words_in, verbose):
        return get_completion_choices(self.type, words_in, verbose)

    # FIX: Not used
    def get(self, fields):
        # returns data as str
        # not intended to be called from command line
        assert Sock_buffer.find('\n') == -1, f"Global.get: Sock_buffer not empty"
        type, offsets = self.global_args(fields)
        send_sock(f"get_global {type} {' '.join(offsets)}\n")
        command, final_addr, data = sock_readline().split()
        assert command == 'get_global'
        return data  # as str

    # FIX: Not used
    def set(self, value):
        # returns prev value
        assert not Sock_buffer
        send_sock(f"set_global {self.addr} {self.len} {to_hex(value, self.unsigned, self.len)}\n")
        command, final_addr, prev_data = sock_readline().split()
        assert command == 'set_global'
        assert int(final_addr) == self.addr
        return from_hex(prev_data, self.unsigned, self.len)

def add_global(name, addr, len, type):
    assert name not in Globals, f"{name=!r} already in Globals"
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
            if dim_in >= self.dims[i]:
                print(f"ERROR Array({self.name}), dim {i}: "
                      f"requested dim={dim_in} >= array dim={self.dims[i]}",
                      file=sys.stderr) 
                sys.exit(2)
            addr += dim_in * product(self.dims[i + 1:]) * self.element_size
            if Trace:
                print(f"{self.name}.global_args, got arg={dim_in}, {addr=}")
        return get_global_args(words_in, self.type, self.ggtype, addr)

    def get_completion_choices(self, words_in, verbose):
        num_dims = len(self.dims)
        if len(words_in) < num_dims:
            if verbose:
                print(f"Array({self.name}).get_completion_choices needs more dims")
            return list(Defines.keys())
        if verbose:
            print(f"Array({self.name}).get_completion_choices got all dims")
        return get_completion_choices(self.type, words_in[num_dims:], verbose)

def product(iterable):
    ans = 1
    for i in iterable:
        ans *= i
    return ans

def add_array(name, addr, array_size, element_size, type, dims):
    assert name not in Arrays, f"{name=!r} already in Arrays"
    Arrays[name] = Array(name, addr, array_size, element_size, type, dims)

class Function:
    def __init__(self, name, expects_return, params):
        self.name = name
        self.ret = expects_return
        self.params = params

def add_function(name, expects_return, params):
    assert name not in Functions, f"{name=!r} already in Functions"
    Functions[name] = Function(name, expects_return, params)

def load():
    # loads all of the initial "#define", "sub_classes", "field", "global", "array" and "function"
    # info from the C++ program.
    global Sketch_dir
    while True:
        line = sock_readline()
        words = line.split()
        if words[0] == 'sketch_dir':
            assert Sketch_dir is None, "Duplicate C++ 'Sketch_dir' command"
            Sketch_dir = words[1]
        elif words[0] == '#define':
            add_define(words[1], int(words[2]))
        elif words[0] == 'lookup':
            add_lookup(words[1], int(words[2]), words[3])
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

Completer_trace = 0

def completer(text, state):
    # This is called by readline library which gobbles up (hides) exceptions!
    try:
        line = readline.get_line_buffer()
        prior_words = line.split()
        if text:
            prior_words = prior_words[:-1]
        if Completer_trace > 1:
            print(f"completer({text=!r}, {state=}), {line=!r}, {prior_words=}")
        choices = []
        got_Q = False
        if prior_words and prior_words[0] == '?':
            prior_words = prior_words[1:]
            got_Q = True
        if not prior_words:
            choices = ['? ', 'trace ', 'readline', 'run ', 'display ', 'get ', 'call ',
                       'report ', 'return ', 'exit ']
            if got_Q:
                choices.remove('? ')
        elif prior_words[0] == 'trace':
            if len(prior_words) == 1:
                choices = ['on', 'off']
        elif prior_words[0] == 'run':
            if len(prior_words) == 1:
                choices = list(Script.keys())
                choices.remove('defaults')
        elif prior_words[0] == 'report':
            if len(prior_words) == 1:
                choices = list(Reports.keys())
        elif prior_words[0] == 'get':
            if len(prior_words) == 1:
                choices = list(Globals.keys())
                choices.extend(Arrays.keys())
            elif prior_words[1] in Globals:
                if Completer_trace and state == 0:
                    print(f"completer {prior_words[1]=} in Globals")
                choices = Globals[prior_words[1]].get_completion_choices(prior_words[2:], state == 0)
            elif prior_words[1] in Arrays:
                if Completer_trace and state == 0:
                    print(f"completer {prior_words[1]=} in Arrays")
                choices = Arrays[prior_words[1]].get_completion_choices(prior_words[2:], state == 0)
            else:
                choices = list(Defines.keys())
        elif prior_words[0] == 'call':
            if len(prior_words) == 1:
                choices = list(Functions.keys())
            else:
                choices = list(Defines.keys())
        elif prior_words[0] == 'return':
            if len(prior_words) == 1:
                choices = list(Defines.keys())
        if Completer_trace and state == 0:
            print("completer choices", choices)
        legal_choices = [choice for choice in choices if choice.startswith(text)]
        if Completer_trace and state == 0:
            print(f"completer choices starting with {text!r}:", legal_choices)
        if state < len(legal_choices):
            if Completer_trace > 2:
                print("completer ->", legal_choices[state])
            return legal_choices[state]
        if Completer_trace > 2:
            print("completer -> None")
        return None
    except e:
        print("completer got exception", e, file=sys.stderr)

def run(port, script_file, verbose):
    global Sock, Script

    histfile = '.history'
    try:
        readline.read_history_file(histfile)
    except FileNotFoundError:
        pass
    readline.set_history_length(1000)
    readline.set_completer(completer)
    readline.set_completer_delims(" ")
    print("readline completer_delims", repr(readline.get_completer_delims()))

    listen_socket = socket.socket()
    listen_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    addr = 'localhost', port
    listen_socket.bind(('localhost', port))
    print("listening on", addr)
    listen_socket.listen()

    try:
        while True:
            init()
            Script = read_yaml(script_file)
            print("Waiting for connection")
            Sock, addr = listen_socket.accept()
            print("got connection from", addr)
            print()
            #Sock.settimeout(0.3)
            try:
                load()
                interactive(verbose)
            except Sock_closed:
                pass
            Sock.close()
            Sock = None
    finally:
        readline.write_history_file(histfile)

def translate(request):
    # request does not require trailing '\n' (but is OK with it)
    # returns a command string terminated with '\n' ('' if no C++ command generated)
    words_in = request.split()
    words_out = []
    if words_in[0] == 'get':
        type, offsets = make_get_global(words_in[1:])
        words_out.append('get_global')
        words_out.append(type)
        words_out.extend(offsets)
    elif words_in[0] == 'set':
        value = words_in.pop(-1)
        assert words_in[-1] == '=', f"Missing '=' in 'set' command {request!r}"
        words_in.pop(-1)
        type, offsets = make_get_global(words_in[1:])
        words_out.append('set_global')
        words_out.append(type)
        words_out.extend(offsets)
        words_out.append('=')
        words_out.append(value)
    elif words_in[0] == 'report':
        assert len(words_in) == 2, f'invalid "report" request: expected 2 words, got {len(words_in)}'
        Reports[words_in[1]]()  # loads Report_output with output lines
        return ''
    else:
        words_out.append(words_in[0])
        for word in words_in[1:]:
            words_out.append(translate_word(word))
    return ' '.join(words_out) + '\n'

def translate_word(word):
    # returns a str
    if word[0] in "0123456789+-":
        return word
    if word in Defines:
        return str(Defines[word])
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
            print(f"ERROR: in response={response!r}, "
                  f"word {i}={rword!r} does not match script={sword!r}",
                  file=sys.stderr)
            sys.exit(1)

def strip_comment(line):
    # strips trailing '\n'
    #
    # does not strip trailing spaces if no comment.
    if line and line[-1] == '\n':
        line = line[: -1]
    comment_start = line.find('#')
    if comment_start >= 0:
        line = line[: comment_start]
    return line.strip()

def get(*words_in):
    # returns data as str
    # not intended to be called from command line
    assert Sock_buffer.find('\n') == -1, f"get: Sock_buffer not empty"
    type, offsets = make_get_global(list(words_in))
    send_sock(f"get_global {type} {' '.join(offsets)}\n")
    command, final_addr, data = sock_readline(sock_only=True).split()
    assert command == 'get_global'
    return data  # as str

def dump_encoders():
    Report_output.append(dump_encoder('FUNCTION_ENCODER'))
    for enc in range(4):
        Report_output.append(dump_encoder(str(enc)))

def dump_encoder(enc):
    if enc.isdigit():
        enc_name = f"P{int(enc) + 1}"
    else:
        enc_name = "FN"
    encoder_event = decode_event(int(get('Encoders', enc, 'encoder_event')))
    var = get('Encoders', enc, 'var')
    if var == '0':
        # NULL var
        return f"{enc_name}: N {encoder_event} - -"
    display_value = decode_event(int(get('Encoders', enc, 'var', 'var_type', 'display_value')))
    flags = int(get('Encoders', enc, 'var', 'var_type', 'flags'))
    if flags & Defines['ENCODER_FLAGS_DISABLED']:
        # Disabled
        return f"{enc_name}: D {encoder_event} {display_value} -"
    value = get('Encoders', enc, 'var', 'value')
    # Enabled
    return f"{enc_name}: E {encoder_event} {display_value} {value}"

def decode_event(n):
    # n must be an int
    # return may str or int
    if n in Lookups['events']:
        return Lookups['events'][n]
    return n

Reports = {
    'encoders': dump_encoders,
}

def run_script(script_name, indent, verbose):
    global Current_script
    try:
        Current_script = Script[script_name]
        Seq_numbers = Counter()
        in_report = False
        for line_no, line in enumerate(Current_script['script'].split('\n'), 1):
            #print(f"run_script: {line_no=}, {line=!r}")
            line = strip_comment(line)
            if not line:
                continue
            elif line[0] == '<':
                line_rest = line[1:].lstrip()
                translated_request = translate(line_rest)
                if not translated_request:
                    print(' ' * indent, '< ', line_rest, sep='', end='')
                    indent += 2
                    in_report = True
                else:
                    print(' ' * indent, '< ', translated_request, sep='', end='')
                    send_sock(translated_request)
                    indent += test_driver_indent(translated_request)
            elif line[0] == '>':
                if in_report:
                    if Report_output:
                        expect = line[1:].lstrip()
                        response = Report_output.popleft()
                        print(' ' * indent, "> ", repr(response), sep='')
                        compare(response, expect)
                        continue
                    else:
                        indent -= 2
                        in_report = False
                while True:
                    assert not Report_output
                    response = sock_readline()
                    if not do_default(response, indent, verbose):
                        expect = line[1:].lstrip()
                        print(' ' * indent, "> ", repr(response), sep='')
                        compare(response, expect)
                        indent += cpp_indent(response)
                        break
            else:
                print(f"ERROR: {script_name}[{line_no}]: Unknown line prefix {line=!r}", file=sys.stderr)
                sys.exit(2)
    finally:
        Current_script = None

def test_driver_indent(test_driver_command):
    # Returns change to indent level after test_driver_request issued to C++ program
    if test_driver_command.startswith('call') \
       or test_driver_command.startswith('get_global') \
       or test_driver_command.startswith('set_global'):
        return 2
    if test_driver_command.startswith('return'):
        return -2
    return 0

def cpp_indent(cpp_command):
    if cpp_command.startswith('fun_called'):
        return 2
    if cpp_command.startswith('returned') \
       or cpp_command.startswith('get_global') \
       or cpp_command.startswith('set_global'):
        return -2
    print("Unexpected cpp_command:", cpp_command, file=sys.stderr)
    exit(2)

def do_icommand(request, indent, verbose):
    # request should not have trailing whitespace (including '\n')
    # returns new indent level
    global Trace
    if request.startswith('?'):
        print(' ' * indent, translate(request[1:].strip()), sep='', end='')
    elif request.startswith('trace '):
        words = request.split()
        if words[1] == 'on':
            Trace = 1
        else:
            Trace = 0
    elif request.startswith('readline'):
        try:
            print(f"{' ' * indent}sock_readline returned:", repr(sock_readline()))
        except socket.timeout:
            print(f"{' ' * indent}sock_readline: socket.timeout")
    elif request.startswith('run '):
        words = request.split()
        assert len(words) == 2, f'invalid "run" request: expected 2 words, got {len(words)}'
        run_script(words[1], indent, verbose)
        indent -= 2
    else:
        translated_request = translate(request)
        if not translated_request:
            if Trace:
                print(f"{' ' * indent}{request=!r}")
            while Report_output:
                print(' ' * indent, "> ", repr(Report_output.popleft()), sep='')
            indent -= 2
        else:
            if Trace:
                print(f"{' ' * indent}{translated_request=!r}")
            send_sock(translated_request)
            indent += test_driver_indent(translated_request)
            try:
                while True:
                    assert not Report_output
                    response = sock_readline()
                    if not do_default(response, verbose):
                        # don't know this one, show it to user and let them respond...
                        print(' ' * indent, "> ", repr(response), sep='')
                        indent += cpp_indent(response)
                        break
            except socket.timeout:
                print("do_icommand: socket.timeout")
    return indent

def do_default(cpp_command, indent, verbose):
    # Returns True if cpp_command handled by a default, False otherwise.
    if not cpp_command.startswith('fun_called '):
        return False
    words = cpp_command.split()
    if Current_script and words[1] in Current_script.get('defaults', {}):
        result = Current_script['defaults'][words[1]]
    elif words[1] in Script.get('defaults', {}):
        result = Script['defaults'][words[1]]
    else:
        return False
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
                print(' ' * indent, "> ", cpp_command, sep='')
                print(' ' * (indent + 2), "< return", sep='')
            send_sock("return\n")
            return True
        else:
            if verbose:
                print(' ' * indent, "> ", cpp_command, sep='')
                print(' ' * (indent + 2), "< return ", result, sep='')
            send_sock(f"return {result}\n")
            return True

def interactive(verbose):
    print("client ready!")
    indent = 0
    while True:
        request = input(f"{' ' * indent}< ")
        indent = do_icommand(request.strip(), indent + 2, verbose)



if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--port', '-p', type=int, default=2020)
    parser.add_argument('--verbose', '-v', action='store_true')
    parser.add_argument('--trace', '-t', action='store_true')
    parser.add_argument('script_file')
    args = parser.parse_args()

    if args.trace:
        Trace = True

    run(args.port, args.script_file, args.verbose)
