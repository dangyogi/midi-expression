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
    # Caller must supply trailing '\n'
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
Report_lines = deque()       # lines output from internal commands, with no trailing '\n'

def sock_readline(sock_only=False, recv_flags=0):
    # Returns next line with the trailing '\n' stripped.
    global Sock_buffer
    if Trace:
        print("sock_readline called, recv_flags", recv_flags)
    #if Report_lines and not sock_only:
    #    return Report_lines.popleft()
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

Defines = {}   # name: value (as int)
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
Call_depth = 0
Pass_through_depth = None   # pass-through until Call_depth < Pass_through_depth
Pass_through_start = None   # 'call' or 'fun_called'

def init():
    global Sock_buffer, Report_lines, Defines, Lookups, Classes, Structs, Globals, Arrays
    global Functions, Seq_numbers, Sketch_dir, Script, Current_script, Call_depth, Pass_through_depth
    global Pass_through_start
    Sock_buffer = ''
    Report_lines = deque()
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
    Call_depth = 0
    Pass_through_depth = None
    Pass_through_start = None   # 'call' or 'fun_called'

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
        self.params = []
        self.lookups = {}
        for i, param in enumerate(params, 2):
            if '|' in param:
                pname, lookup = param.split('|')
                self.params.append(pname)
                self.lookups[pname] = lookup
            else:
                self.params.append(param)

    def format_params(self, params):
        # params should not be empty
        assert len(params) == len(self.params)
        plist = []
        for pname, value in zip(self.params, params):
            if pname in self.lookups:
                vnames = Lookups[self.lookups[pname]]
                #print(f"Function.format {pname=} in lookups: {vnames=}, {value=!r}")
                if value in vnames:
                    plist.append(vnames[value])
                else:
                    plist.append(value)
            else:
                plist.append(value)
        return ' '.join(plist)

I2C_LED_commands = {
    14: "led_on led",
    15: "led_off led",
    18: "load_digit disp digit# value dp",
    21: "load_sharp_flat disp sharp_flat",
    19: "load_numeric disp value_s16 dec_place",
    20: "load_note disp note sharp_flat",
    31: "clear_display disp",
    29: "clear_choices choices_num",
    30: "select_choice choices_num choice",
}

LED_history = defaultdict(lambda: defaultdict(list))

def sendRequest_notes(params):
    assert int(params[0]) == Defines['I2C_LED_CONTROLLER']
    digits = [int(params[1][2*i:2*i+2], 16) for i in range(len(params[1])//2)]
    if digits[0] in I2C_LED_commands:
        cmd, unit, *params = I2C_LED_commands[digits[0]].split()
        param_decode = [f"{name}={value}" for name, value in zip(params, digits[2:])]
        if param_decode:
            history_line = f"{cmd}: {' '.join(param_decode)}"
        else:
            history_line = cmd
        LED_history[unit][digits[1]].append(history_line)

Midi_types = {
    0x80: "NoteOff",
    0x90: "NoteOn",
    0xA0: "AfterTouchPoly",
    0xB0: "ControlChange",
    0xC0: "ProgramChange",
    0xD0: "AfterTouchChannel",
    0xE0: "PitchBend",
    # omitting 0xF? types
}

Control_codes = {
    0x01: "modulation",
    0x06: "msb parameter value",
    0x26: "lsb parameter value",
    0x07: "channel volume",
    0x0A: "pan",
    0x0B: "expression controller",
    0x40: "sustain pedal on/off, <=63 off; >=64 on",
    0x62: "NRPN lsb",
    0x63: "NRPN msb",
    0x64: "RPN lsb",
    0x65: "RPN msb",
    0x79: "Reset all controllers",
    0x7B: "All notes off",
}

Midi_send_history = []

Cables = ("Synth", "Player")

Notes = "C C# D Eb E F F# G Ab A Bb B".split()

def midi_send_notes(params):
    # channel numbers here are midi channels (1-16)
    type, data1, data2, channel, cable = [int(p) for p in params]
    if type in (0x80, 0x90):  # NoteOff, NoteOn
        octave, note = divmod(data1 - 12, 12)
        Midi_send_history.append(f"{Cables[cable]} {channel}: {Midi_types.get(type, hex(type))} "
                                 f"{Notes[note]}{octave} {data2}")
    elif type == 0xB0:
        Midi_send_history.append(f"{Cables[cable]} {channel}: {Midi_types.get(type, hex(type))} "
                                 f"{Control_codes.get(data1, hex(data1))} {data2}")
    else:
        Midi_send_history.append(f"{Cables[cable]} {channel}: {Midi_types.get(type, hex(type))} "
                                 f"{data1} {data2}")

Fun_notes = {
    'sendRequest': sendRequest_notes,
    'usb_midi_send': midi_send_notes,
}

def format_command(command):
    if not command.startswith('call ') and not command.startswith('fun_called '):
        return command
    cmd, fname, *params = command.split()
    fun = Functions[fname]
    head = f"{cmd} {fname}"
    if not params:
        return head
    return f"{head} {fun.format_params(params)}"

def add_function(name, expects_return, params):
    assert name not in Functions, f"{name=!r} already in Functions"
    Functions[name] = Function(name, expects_return, params)
    if False and Functions[name].lookups:
        call = f"call {name} {' '.join(str(i) for i in range(35, 35 + len(params)))}"
        fcall = format_command(call)
        print(f"format_command({call=!r}) gives {fcall!r}")

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
            add_lookup(words[1], words[2], words[3])
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
                       'report ', 'exit ']
            choices.extend(f"{fn} return" for fn in Functions.keys())
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
        elif prior_words[0] in Functions:
            if len(prior_words) == 1:
                choices = ['return']
            elif prior_words[1] == 'return':
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
    # called from run_script and do_icommand
    # request does not require trailing '\n' (but is OK with it)
    # returns a command string terminated with '\n' ('' if report run, so no C++ command generated)
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
        Reports[words_in[1]]()  # loads Report_lines with output lines
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
        print(f"ERROR: Unrecognized Global/Array {words_in[0]!r}", file=sys.stderr)
        sys.exit(2)
    return type, offsets


def compare(response, script_line):
    rwords = response.split()
    swords = script_line.split()
    if len(rwords) != len(swords):
        print(f"ERROR: different number of words in response={response!r} and script={script_line!r}",
              file=sys.stderr)
        sys.exit(1)
    for i, (rword, sword) in enumerate(zip(rwords, swords), 1):
        if sword != '.' and rword != sword and rword != translate_word(sword):
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
    Report_lines.append("ENC F encoder_event display_value value min max")
    Report_lines.append(dump_encoder('FUNCTION_ENCODER'))
    for enc in range(4):
        Report_lines.append(dump_encoder(str(enc)))

def dump_encoder(enc):
    if enc.isdigit():
        enc_name = f"P{int(enc) + 1}"
    else:
        enc_name = "FN"
    encoder_event = decode_event(get('Encoders', enc, 'encoder_event'))
    var = get('Encoders', enc, 'var')
    if var == '0':
        # NULL var
        return f"{enc_name}: N {encoder_event} - - - -"
    display_value = decode_event(get('Encoders', enc, 'var', 'var_type', 'display_value'))
    flags = int(get('Encoders', enc, 'var', 'var_type', 'flags'))
    if flags & Defines['ENCODER_FLAGS_DISABLED']:
        # Disabled
        return f"{enc_name}: D {encoder_event} {display_value} - - -"
    value = get('Encoders', enc, 'var', 'value')
    min = get('Encoders', enc, 'var', 'var_type', 'min')
    max = get('Encoders', enc, 'var', 'var_type', 'max')
    # Enabled
    return f"{enc_name}: E {encoder_event} {display_value} {value} {min} {max}"

def decode_event(n):
    # n must be an int
    # return may str or int
    if n in Lookups['events']:
        return Lookups['events'][n]
    if n == '255':
        return '0xFF'
    return n

def dump_events():
    for value, name in sorted(Lookups['events'].items(), key=lambda item: int(item[0])):
        Report_lines.append(f"{value}: {name}")

def clear_led_history():
    global LED_history
    LED_history = defaultdict(lambda: defaultdict(list))
    Report_lines.append("LED_history cleared")

def dump_led_history():
    for type, history in sorted(LED_history.items()):
        for unit, lines in sorted(history.items()):
            for line in lines:
                Report_lines.append(f"{type} {unit} {line}")
    if not Report_lines:
        Report_lines.append("no LED history")

def clear_midi_send_history():
    global Midi_send_history
    Midi_send_history = []
    Report_lines.append("midi_history cleared")

def dump_midi_send_history():
    if not Midi_send_history:
        Report_lines.append("no midi history")
    else:
        Report_lines.extend(Midi_send_history)

Reports = {
    'encoders': dump_encoders,
    'events': dump_events,
    'clear_led_history': clear_led_history,
    'led_history': dump_led_history,
    'clear_midi_history': clear_midi_send_history,
    'midi_history': dump_midi_send_history,
}

def indent(added_call_depth=0):
    # returns indent string
    assert Call_depth >= 0
    return ' ' * ((Call_depth + added_call_depth) * 2)

def run_script(script_name, verbose):
    global Current_script, Call_depth
    if Trace:
        print(f"run_script called, {Call_depth=}")
    try:
        Current_script = Script[script_name]
        Seq_numbers = Counter()
        for line_no, line in enumerate(Current_script['script'].split('\n'), 1):
            #print(f"run_script({script_name=}): {line_no=}, {line=!r}")
            line = strip_comment(line)
            if not line:
                continue
            if line[0] == '<':
                line_rest = line[1:].lstrip()
                print(indent(), '< ', format_command(line_rest), sep='')
                to_cpp(line_rest)
            elif line[0] == '>':
                if Trace:
                    print(f"run_script got {line=!r}, calling from_cpp")
                response = from_cpp(verbose)
                if Trace:
                    print("run_script: from_cpp returned", repr(response))
                expect = line[1:].lstrip()
                compare(response, expect)
            else:
                print(f"ERROR: {script_name}[{line_no}]: Unknown line prefix {line=!r}", file=sys.stderr)
                sys.exit(2)
    finally:
        if Report_lines:
            print("run_script: extra report_lines not matched at end of script", file=sys.stderr)
            for line in Report_lines:
                print(" >", line, file=sys.stderr)
            exit(2)
        Current_script = None
        if Trace:
            print(f"run_script done, {Call_depth=}")

def test_driver_adj_depth(test_driver_command):
    # Adjusts Call_depth after test_driver_command issued to C++ program
    global Call_depth
    if test_driver_command.startswith('call') \
       or test_driver_command.startswith('get_global') \
       or test_driver_command.startswith('set_global'):
        Call_depth += 1
    elif 'return' in test_driver_command:
        Call_depth -= 1

def cpp_adj_depth(cpp_command):
    # Adjusts Call_depth after cpp_command issued to test driver
    global Call_depth
    if cpp_command.startswith('fun_called'):
        Call_depth += 1
    elif 'returned' in cpp_command \
       or cpp_command.startswith('get_global') \
       or cpp_command.startswith('set_global'):
        Call_depth -= 1
    else:
        print("Unexpected cpp_command:", repr(cpp_command), file=sys.stderr)
        exit(2)

def to_cpp(command):
    # caller must print command to stdout before calling
    # does not return anything
    global Call_depth, Pass_through_depth, Pass_through_start
    assert not Report_lines, f"to_cpp called with {len(Report_lines)} Report_lines remaining"
    tcmd = translate(command)  # runs report and returns '', if "report" command given
    if command.startswith('get ') or command.startswith('set '):
        # don't print the translated "get" or "set" (i.e., tcmd)
        Call_depth += 1
        send_sock(tcmd)
    elif tcmd == '':
        # report run
        Call_depth += 1
    elif tcmd.startswith('call '):
        Call_depth += 1
        send_sock(tcmd)
        action = get_action(tcmd)
        if Trace:
            print(f"to_cpp({tcmd=!r}): {action=}")
        if action == 'pass-through' and Pass_through_depth is None:
            if Trace:
                print("to_cpp: setting Pass_through_depth to", Call_depth)
            Pass_through_depth = Call_depth
            Pass_through_start = 'call'
    else:
        send_sock(tcmd)
        if 'return' in tcmd:
            Call_depth -= 1

def from_cpp(verbose):
    # This skips over pass-through and default return fun_calls so that the caller doesn't see them,
    # though they are printed if verbose is True.
    # Prints recvd_cmd before returning it (no trailing '\n').
    # Caller does validation (if any) on recvd_cmd.
    global Call_depth, Pass_through_depth, Pass_through_start
    while True:
        if Trace:
            print(f"top of from_cpp loop, {Call_depth=}, {Pass_through_depth=}")
        dec_call_depth = False
        default_return = None
        ret_pass_through_cmd = None
        do_adjust_indent = True
        if Report_lines:
            recvd_cmd = Report_lines.popleft()
            do_adjust_indent = False
            if not Report_lines:
                dec_call_depth = True
        else:
            if Trace:
                print("from_cpp loop calling sock_readline")
            recvd_cmd = sock_readline()  # no trailing '\n'
            if Trace:
                print("from_cpp loop received:", repr(recvd_cmd))
            if recvd_cmd.startswith('fun_called '):
                cmd, fname, *params = recvd_cmd.split()
                if fname in Fun_notes:
                    Fun_notes[fname](params)
                action = get_action(recvd_cmd)
                if action is not None and 'return' in action:
                    default_return = action
                    if Trace:
                        print("from_cpp loop, fun_called with default return:", repr(default_return))
                    send_sock(default_return + '\n')
                    dec_call_depth = True
                else:
                    if action is not None and action == 'pass-through':
                        if Pass_through_depth is None:
                            Pass_through_depth = Call_depth + 1
                            Pass_through_start = 'fun_called'
                            if Trace:
                                print(f"from_cpp loop, fun_called with pass-through {Call_depth=} "
                                      f"{Pass_through_depth=}")
                    if Pass_through_depth is not None:
                        call_cmd = 'call' + recvd_cmd[10:]
                        if Trace:
                            print(f"from_cpp loop, sending {call_cmd=!r}")
                        send_sock(call_cmd + '\n')
            else:
                if Pass_through_depth is not None:
                    if Call_depth > Pass_through_depth or \
                       Call_depth == Pass_through_depth and Pass_through_start == 'fun_called':
                        assert 'returned' in recvd_cmd, \
                               f"ERROR: from_cpp expected 'returned', got {recvd_cmd}"
                        ret_pass_through_cmd = recvd_cmd.replace('returned', 'return', 1)
                        send_sock(ret_pass_through_cmd + '\n')
        if Pass_through_depth is None and default_return is None or verbose:
            if ret_pass_through_cmd:
                print(indent(), '< ', ret_pass_through_cmd, sep='')
            else:
                print(indent(), '> ', format_command(recvd_cmd), sep='')
                if default_return is not None:
                    print(indent(1), '< ', default_return, sep='')
        if do_adjust_indent:
            cpp_adj_depth(recvd_cmd)
        if dec_call_depth:
            Call_depth -= 1
        if Pass_through_depth is not None and Call_depth < Pass_through_depth:
            if Trace:
                print(f"from_cpp loop, setting {Pass_through_depth=} to None, {Call_depth=}")
            Pass_through_depth = None
            Pass_through_start = None
            if not verbose and 'returned' in recvd_cmd:
                print(indent(1), '> ', format_command(recvd_cmd), sep='')
        if Pass_through_depth is None and default_return is None:
            if Trace:
                print(f"from_cpp no pass-through or default_return, returning {recvd_cmd=!r}")
            return recvd_cmd


def do_icommand(request, verbose):
    # request should not have trailing whitespace (including '\n')
    global Trace, Call_depth
    #print(f"do_icommand called, {Call_depth=}")
    if request.startswith('?'):
        print(indent(), translate(request[1:].strip()), sep='', end='')
    elif request.startswith('trace '):
        words = request.split()
        if words[1] == 'on':
            Trace = 1
        else:
            Trace = 0
    elif request.startswith('readline'):
        try:
            print(f"{indent()}sock_readline returned:", repr(sock_readline()))
        except socket.timeout:
            print(f"{indent()}sock_readline: socket.timeout")
    elif request.startswith('run '):
        words = request.split()
        assert len(words) == 2, f'invalid "run" request: expected 2 words, got {len(words)}'
        Call_depth += 1
        run_script(words[1], verbose)
        Call_depth -= 1
    else:
        to_cpp(format_command(request))
        starting_depth = Call_depth
        try:
            while Call_depth >= starting_depth:
                from_cpp(verbose)
        except socket.timeout:
            print("do_icommand: socket.timeout")
    #print(f"do_icommand done, {Call_depth=}")

def get_action(command):
    # returns None, 'pass-through', or 'fun_name return X' (no trailing '\n')
    if not command.startswith('call') and not command.startswith('fun_called'):
        assert 'returned' in command, f"get_action: expected 'returned', got {command!r}"
        assert Pass_through_depth is not None  # I think this will always be the case... ??
        return command.replace('returned', 'return', 1)   # change 'returned' to 'return'
    cmd, fname, *params = command.split()
    if Current_script and fname in Current_script.get('defaults', {}):
        action = Current_script['defaults'][fname]
    elif fname in Script.get('defaults', {}):
        action = Script['defaults'][fname]
    else:
        #print(f"get_action {fname=} -> None")
        return None
    while True:
        if action == 'pass-through':
            #print(f"get_action {fname=} -> 'pass-through'")
            if Pass_through_depth is not None:
                return None
            return 'pass-through'
        if isinstance(action, dict):
            action = action[params[0]]
            #print(f"get_action got dict for", fname, "new action is", action)
        elif isinstance(action, (list, tuple)):
            action = action[Seq_numbers[fname]]
            #print("get_action got sequence for", fname, "new action is", action, "seq_num is",
            #      Seq_numbers[fname])
            Seq_numbers[fname] += 1
        else:
            break
    # Got an default action that is not 'pass=through', hence a return value
    if cmd == 'call':
        return None
    if action is None:
        return f"{fname} return"
    else:
        return f"{fname} return {action}"

def interactive(verbose):
    global Call_depth
    print("client ready!")
    Call_depth = 0
    while True:
        #print(f"interactive loop, {Call_depth=}")
        request = input(f"{indent()}< ")
        do_icommand(request.strip(), verbose)



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
