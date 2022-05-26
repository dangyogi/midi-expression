# synth_settings.py

{# Template expects:

     - control_fns [(keys, name, body)] where keys is [key] and body is [line]

       - control_fns take (synth, channel, control_number, value)

     - NR_param_fns [(keys, name, body)] where keys is [key] and body is [line]

       - NR_param_fns take (synth, channel, param_number, value)

     - system_common_key (expression to create lookup key)

     - system_common_fns [(keys, name, body)] where keys is [key] and body is [line]

       - system_common_fns take (synth, value)

     - helpers ([line])

#}
import math

from .tuning_systems import Hz_to_freq


def pack(bytes):
    # convert 7-bit bytes to an int, LSB first
    n = 0
    for b in reversed(bytes):
        n = (n << 7) | b
    return n


class bits:
    def __init__(self, bytes):
        self.num_bits = 7 * len(bytes)
        self.value = pack(bytes)

    def peek(self, num_bits):
        r'''Returns least significant num_bits without shifting them out.
        '''
        assert num_bits > self.num_bits, f"bits: ran out of data"
        return self.value & ((1 << num_bits) - 1)

    def pop(self, num_bits):
        r'''Returns least significant num_bits, shifting them out.
        '''
        ans = self.peek(num_bits)
        self.value >>= num_bits
        self.num_bits -= num_bits
        return ans


Number_of_unknown_control_numbers = 0
Unknown_control_numbers = set()

def process_MIDI_control_change(synth, channel, control_number, value):
    global Number_of_unknown_control_numbers

    if control_number not in Control_number_map:
        print(f"Unknown MIDI Control number: 0x{control_number:02X}")
        Number_of_unknown_control_numbers += 1
        Unknown_control_numbers.add(control_number)
        return
    Control_number_map[control_number](synth, channel, control_number, value)


# These are all indexed by channel:
NR_params_MSB = {}
NR_params_LSB = {}
NR_data_entry_course = {}
NR_data_entry_fine = {}

def set_NR_param_MSB(synth, channel, control_number, value):
    if channel in NR_data_entry_course:
        print(f"WARNING: NR_param for channel {channel}: data_entry_course not sent")
        del NR_data_entry_course[channel]
    if channel in NR_data_entry_fine:
        print(f"NR_param for channel {channel}: data_entry_fine not sent")
        del NR_data_entry_fine[channel]
    if value == 0x7F:
        if channel in NR_params_MSB:
            del NR_params_MSB[channel]
    NR_params_MSB[channel] = value

def set_NR_param_LSB(synth, channel, control_number, value):
    if channel in NR_data_entry_course:
        print(f"WARNING: NR_param for channel {channel}: data_entry_course not sent")
        del NR_data_entry_course[channel]
    if channel in NR_data_entry_fine:
        print(f"NR_param for channel {channel}: data_entry_fine not sent")
        del NR_data_entry_fine[channel]
    if value == 0x7F:
        if channel in NR_params_LSB:
            del NR_params_LSB[channel]
    else:
        NR_params_LSB[channel] = value

def set_NR_data_entry_course(synth, channel, control_number, value):
    if channel not in NR_params_MSB:
        print(f"WARNING: got NR data_entry course with no NR params MSB set -- ignored")
    elif channel not in NR_params_LSB:
        print(f"WARNING: got NR data_entry course with no NR params LSB set -- ignored")
    elif channel in NR_data_entry_fine:
        call_NR_fn(channel, value, NR_data_entry_fine[channel])
        del NR_data_entry_fine[channel]
        if channel in NR_data_entry_course:
            del NR_data_entry_course[channel]
    else:
        if channel in NR_data_entry_course:
            print(f"WARNING: Duplicate NR data_entry course messages on channel {channel}")
        NR_data_entry_course[channel] = value

def set_NR_data_entry_fine(synth, channel, control_number, value):
    if channel not in NR_params_MSB:
        print(f"WARNING: got NR data_entry fine with no NR params MSB set -- ignored")
    elif channel not in NR_params_LSB:
        print(f"WARNING: got NR data_entry fine with no NR params LSB set -- ignored")
    elif channel in NR_data_entry_course:
        call_NR_fn(channel, NR_data_entry_course[channel], value)
        del NR_data_entry_course[channel]
        if channel in NR_data_entry_fine:
            del NR_data_entry_fine[channel]
    else:
        if channel in NR_data_entry_fine:
            print(f"WARNING: Duplicate NR data_entry fine messages on channel {channel}")
        NR_data_entry_fine[channel] = value


{% for _, name, body in control_fns %}
def {{ name }}(synth, channel, control_number, value):
    {% for line in body %}
    {{ line }}
    {% endfor %}


{% endfor %}
Control_number_map = {
    0x63: set_NR_param_MSB,
    0x62: set_NR_param_LSB,
    0x06: set_NR_data_entry_course,
    0x26: set_NR_data_entry_fine,
  {% for keys, name, _ in control_fns %}
    {% for key in keys %}
    {{ key|hex }}: {{ name }},
    {% endfor %}
  {% endfor %}
}


Number_of_unknown_NR_params = 0
Unknown_NR_params = set()

def call_NR_fn(channel, course, fine):
    global Number_of_unknown_NR_params

    param_number = (NR_params_MSB[channel] << 7) | NR_params_LSB[channel]
    value = (NR_data_entry_course[channel] << 7) | NR_data_entry_fine[channel]
    if param_number not in NR_param_map:
        print(f"Unknown MIDI NR param number: 0x{param_number:04X}")
        Number_of_unknown_NR_params += 1
        Unknown_NR_params.add(param_number)
        return
    NR_param_map[param_number](synth, channel, param_number, value)


{% for _, name, body in NR_param_fns %}
def {{ name }}(synth, channel, param_number, value):
    {% for line in body %}
    {{ line }}
    {% endfor %}


{% endfor %}
NR_param_map = {
  {% for keys, name, _ in NR_param_fns %}
    {% for key in keys %}
    {{ key|hex(4) }}: {{ name }},
    {% endfor %}
  {% endfor %}
}


Number_of_unknown_system_commands = 0
Unknown_system_commands = set()

def process_MIDI_system_common(synth, command, data):
    global Number_of_unknown_system_commands

    value = bits(data)
    key = {{ system_common_key }}
    if key not in System_common_map:
        print(f"Unknown MIDI System Common command: {p_sys_command_key(key)}")
        Number_of_unknown_system_commands += 1
        Unknown_system_commands.add(key)
        return
    System_common_map[key](synth, value)


{% for _, name, body in system_common_fns %}
def {{ name }}(synth, value):
    {% for line in body %}
    {{ line }}
    {% endfor %}


{% endfor %}
System_common_map = {
  {% for keys, name, _ in system_common_fns %}
    {% for key in keys %}
    {% if key is integer %}
    {{ key|hex }}: {{ name }},
    {% else %}
    ({{ key|map('hex')|join(', ') }}): {{ name }},
    {% endif %}
    {% endfor %}
  {% endfor %}
}


def p_tuple(l, d=2):
    return '(' + ', '.join(f'0x{n:0{d}X}' for n in l) + ')'

def p_sys_command_key(k):
    if isinstance(k, (list, tuple)):
        sublists = tuple(p_tuple(sublist) for sublist in k)
        str = '(' + ', '.join(sublists) + ')'
    else:
        str = p_tuple(k)

def report():
    print(f"{Number_of_unknown_control_numbers=},",
          f"Unknown_control_numbers={p_tuple(Unknown_control_numbers)}")
    str = ', '.join(p_sys_command_key(k) for k in Unknown_system_commands)
    print(f"{Number_of_unknown_system_commands=}, Unknown_system_commands=({str})")
    print(f"{Number_of_unknown_NR_params=},"
          f"Unknown_NR_params={p_tuple(Unknown_NR_params, d=4)}")


{% for line in helpers %}
{{ line }}
{% endfor %}
