# parse_wiring.py

from itertools import zip_longest, chain, repeat, groupby, cycle, dropwhile
from operator import itemgetter
from collections import defaultdict, Counter

from .utils import read_yaml
from .parse_devices import Attrs, load_devices, get_device, all_devices, convert_name


Parts = {}
Little_stuff = Counter()


Last_part_list = None  # [device, [Part]]
Last_part_order = 0

class Part(Attrs):
    dont_inherit = Attrs.dont_inherit.union(('pins_by_name', 'pins_by_number',
                     'connected', 'next_col', 'position', 'part_order'))
    dont_attrs = Attrs.dont_attrs.union(('pins',))
    pins = ()
    fans_out_to = ()
    connected = False
    position = None
    next_col = None

    def __init__(self, name, device, attrs={}):
        Attrs.__init__(self, name, attrs, uses=device)
        self.device = device
        if name in Parts:
            raise AssertionError(f"Duplicate name, {name!r}")
        Parts[name] = self
        self.init2()

    def init2(self):
        global Last_part_list, Last_part_order
        self.device.add_instance(self)
        self.pins_by_name = {}
        my_pins = {}
        for my_pin in self.get_my_attr('pins'):
            device_pin = self.device.get_unique_pin(my_pin['name'])
            new_pin = Pin(my_pin['name'], my_pin, parent=self, uses=device_pin)
            my_pins[device_pin.pin_number] = new_pin
            self.pins_by_name[my_pin['name']] = new_pin
        self.pins_by_number = []
        for device_pin in self.device.get_ordered_pins():
            if device_pin.pin_number in my_pins:
                self.pins_by_number.append(my_pins[device_pin.pin_number])
            else:
                my_pin = Pin(device_pin.name, {}, parent=self, uses=device_pin)
                self.pins_by_number.append(my_pin)
                self.pins_by_name[device_pin.name] = my_pin

        # Link to last Part, if same device:
        if Last_part_list is not None and Last_part_list[0] == self.device:
            # link myself to the previous Part
            Last_part_list[1][-1].next_col = self
            Last_part_list[1].append(self)
        else:
            # device changed, start a new list.  Maybe the next Part will link up...
            Last_part_list = [self.device, [self]]

        self.part_order = Last_part_order
        Last_part_order += 1

    def __repr__(self):
        return f"<{self.__class__.__name__} {self.name}>"

    def get_unique_pin(self, name):
        #if name not in self.pins_by_name:
        #    self.pins_by_name[name] = Pin(name, attrs, parent=self,
        #                                  uses=self.device.get_unique_pin(name))
        return self.pins_by_name[name]

    def make_connections(self, quiet=False):
        for p in self.pins_by_number:
            for clause in p.connect:
                p.connect_to(clause, quiet=quiet)
        if self.fans_out_to:
            fanout_pins = [self.pins_by_name[name] for name in self.fanout_pins]
            connects = self.get_fan_out_to_connects()
            assert len(fanout_pins) >= len(connects), \
                   f"{self}: too many fans_out_to parts"
            #print(f"{self}.make_connections, fans_out_to {connects}")
            for my_pin, clause in zip(fanout_pins, connects):
                my_pin.connect_to(clause, quiet=quiet)

    def get_fan_out_to_connects(self):
        r'''Returns [(to_part, to_pin_name)]
        '''
        ans = []
        for to_device_name in self.fans_out_to:
            if isinstance(to_device_name, str):
                to_part = Parts[to_device_name]
                to_pin = to_part.device.pin_connect
                ans.append((to_part, to_pin))
            else:
                assert len(to_device_name) == 2
                to_name, fanout_att_name = to_device_name
                to_part = Parts[to_name]
                fanout_pins = getattr(to_part.device, convert_name(fanout_att_name))
                for p in fanout_pins:
                    ans.append((to_part, p))
        return ans

    def dump(self):
        for line in self.gen_dump_lines():
            print(line)

    def gen_dump_lines(self):
        r'''Returns a list of lines for dump.

        These do NOT have newlines at the end...
        '''
        lines = []
        max_line_len = 134
        title = f"{self.name}: {self.device.name}"
        if not hasattr(self.device, 'num_rows') or self.device.num_rows == 1:
            lines.append(title)
            lines.append('')
            pin_descs = equalize((pin.desc() for pin in self.pins_by_number), max_line_len)
            for pin_number, pin_name, groups in pin_descs:
                if not groups:
                    lines.append(f"{pin_number} {pin_name}")
                else:
                    lines.append(f"{pin_number} {pin_name} -> {', '.join(groups[0])}")
                    for group in groups[1:]:
                        lines.append(' ' * (len(pin_number) + len(pin_name) + 5)
                                     + ', '.join(group))
        else:
            lines.append(' ' * ((max_line_len - len(title) + 1)//2 + 1) + title)
            lines.append('')
            max_line_len_per_side = (max_line_len - 8) // 2
            left_side = equalize((pin.desc(reverse=True)
                                  for pin in self.pins_by_number[:self.num_pins//2]),
                                 max_line_len_per_side,
                                 '>')
            right_side = equalize((pin.desc()
                                   for pin
                                    in reversed(self.pins_by_number[self.num_pins//2:])),
                                  max_line_len_per_side)
            #print(''.join(f"{i:10}" for i in range(1, 10)))
            #print("1234567890" * 10, max_line_len_per_side)
            lines.append(f"{'+----+':>{max_line_len_per_side+7}}")
            for left, right in zip(left_side, right_side):
                left_pin_number, left_pin_name, left_connections = left
                right_pin_number, right_pin_name, right_connections = right
                if left_connections:
                    left_line = ' '.join((', '.join(left_connections[0]), '<-',
                                                    left_pin_name, left_pin_number))
                else:
                    left_line = ' '.join((left_pin_name, left_pin_number))
                if right_connections:
                    right_line = ' '.join((right_pin_number, right_pin_name, '->',
                                          ', '.join(right_connections[0])))
                else:
                    right_line = ' '.join((right_pin_number, right_pin_name))
                lines.append(f"{left_line:>{max_line_len_per_side}}-|    |-{right_line}")
                left_fudge = ' ' * (len(left_pin_number) + len(left_pin_name) + 5)
                left_width = max_line_len_per_side - len(left_fudge)
                right_fudge = ' ' * (len(right_pin_number) + len(right_pin_name) + 5)
                for left_group, right_group \
                 in zip_longest(left_connections[1:], right_connections[1:]):
                    line = []
                    if left_group is None:
                        line.append(' ' * max_line_len_per_side + " |    | ")
                    else:
                        line.append(
                          f"{', '.join(left_group):>{left_width}}{left_fudge} |    | ")
                    if right_group is not None:
                        line.append(right_fudge + ', '.join(right_group))
                    lines.append(''.join(line))
            lines.append(f"{'+----+':>{max_line_len_per_side+7}}")
        return lines

def equalize(list_of_chunks, max_connections_len, align='<'):
    r'''Returns [(adj_pin_number, adj_pin_name, groups)]

    Equalizes the widths of pin_number and pin_name and groups connections.
    '''
    list_of_chunks = tuple(list_of_chunks)
    num_width, name_width = [max(len(x[i]) for x in list_of_chunks) for i in range(2)]
    max_connections_len -= num_width + name_width + 2 + 4  # final 4 for "->" arrow
    return [(f"{num:>{num_width}}", f"{name:{align}{name_width}}",
             tuple(group(connections, max_connections_len)))
            for num, name, connections in list_of_chunks]

def group(connections, max_connections_len):
    group = []
    line_left = max_connections_len
    for conn in connections:
        if len(conn) + 2 > line_left:
            yield group
            group = []
            line_left = max_connections_len
        group.append(conn)
        line_left -= len(conn) + 2   # leave room for ', '
    if group:
        yield group

class Pin(Attrs):
    dont_inherit = Attrs.dont_inherit.union(('connections',))

    def __init__(self, name, attrs, parent=None, uses=None):
        Attrs.__init__(self, name, attrs, parent, uses)
        self.connections = []

    def connect_to(self, clause, one_way=False, quiet=False):
        if isinstance(clause, str):
            clause = (clause,)
        assert len(clause) <= 2
        if len(clause) == 1:
            part, pin = clause[0], None
        else:
            part, pin = clause
        if isinstance(part, str):
            if not part[0].isdigit() and part[0] not in '+-.' and part != 'GND':
                try:
                    part = Parts[part]
                except KeyError:
                    if not quiet:
                        print(f"{self.parent}.{self.name}: connect_to got invalid {part=}")

        if isinstance(part, str):
            if pin is None:
                connect = (part,)
            else:
                connect = (part, pin)
            if connect not in self.connections:
                self.connections.append(connect)
                Little_stuff[part] += 1
        else:
            assert pin is not None, \
                   f"{self.parent}.{self.name}: connect_to part, {part.name}, must have pin"
            if (part, pin) not in self.connections:
                self.connections.append((part, pin))
                self.parent.connected = True
                part.connected = True

            #if not one_way:
            #    part.get_unique_pin(pin).connect_to((self.parent, self.name), True, quiet=quiet)

    def desc(self, reverse=False):
        r'''Returns pin_number, pin_name, [connection]

        Reverse reverses the order of the connections.
        '''
        conn_fragments = []
        for conn in self.connections:
            if isinstance(conn, str):
                conn_fragments.append(str)
            elif not isinstance(conn[0], Part):
                conn_fragments.append(' '.join(conn))
            else:
                conn_fragments.append(f"{conn[0].name}.{conn[1]}")
        if reverse:
            conn_fragments.reverse()
        return f"{self.pin_number}", self.name, conn_fragments

class Batch(Part):
    connected = False
    dont_attrs = Part.dont_attrs.union(('devices',))

    def init2(self):
        self.devices = tuple(Part(v['name'], self.device, v) for v in self.devices)

    def make_connections(self, quiet=False):
        if hasattr(self, 'batch_common_pins'):
            first_part = self.devices[0]
            for dest_part in self.devices[1:]:
                for pin_name in self.batch_common_pins:
                    first_part.get_unique_pin(pin_name).connect_to((dest_part, pin_name),
                                                                   quiet=quiet)
        to_parts = tuple(Parts[name] for name in self.fans_out_to)
        for my_part in self.devices:
            fanout_pins = my_part.device.fanout_pins
            assert len(to_parts) <= len(fanout_pins), \
                   f"{self}.make_connections: more fans-out-to than " \
                   f"{my_part.name} fanout-pins"
            to_pin_name = my_part.links_pin
            for to_part, fanout_pin in zip(to_parts, fanout_pins):
                my_part.get_unique_pin(fanout_pin).connect_to((to_part, to_pin_name),
                                                              quiet=quiet)

class List:
    r'''One or two dimensional lists of devices.

    The lists are stored col-wise, so the first num_rows devices form the first column.
    Thus, the columns can be activated, one at a time, and then the row elements within
    that column processed as a unit.

    The col_pins are the pin names for the different columns within a row.
    The row_pins are the pin names for the different rows within a column.

    The num_elements are the number of elements added to the list for that device.

    Attrs:
    
        num_rows
        num_cols

        devices, or:
            device_name_prefix
            device_type

        device_map
        connect
        <my_device_name>
    '''
    connected = False

    def __init__(self, name, device_map, num_rows=1, num_cols=1, devices=None,
                 device_name_prefix=None, device_type=None, device_number_attr_name=None,
                 connect_by_col=(), connect_by_row=(), **dev_inits):
        self.name = name
        self.device_map = device_map
        self.num_rows = num_rows
        self.num_cols = num_cols
        self.connect_by_col = connect_by_col
        self.connect_by_row = connect_by_row
        self.dev_inits = dev_inits

        assert self.num_rows > 1 or self.num_cols > 1, \
               f"{self.name}: must specify num_rows and/or num_cols"

        Parts[name] = self

        if devices is not None:
            assert device_name_prefix is None and device_type is None, \
                   f"{self.name}: must not specify device_name_prefix or device_type with devices"
            self.devices = devices
        else:
            assert device_name_prefix is not None and device_type is not None, \
                   f"{self.name}: must specify devices or device_name_prefix and device_type"
            device = get_device(device_type)
            num_elements = self.device_map[device_type]['num-elements']
            self.devices = []
            num_devices = (self.num_rows * self.num_cols + num_elements - 1) // num_elements
            for i in range(1, num_devices + 1):
                name = f"{device_name_prefix}{i}"
                Part(name, device)
                self.devices.append(name)

        assert len(self.devices) <= self.num_cols * self.num_rows, \
               f"{self.name}: there are more devices, {len(self.devices)} than rows and cols"

        device_number = 0
        for dev_name in self.devices:
            if dev_name is None:
                device_number += 1
            else:
                dev = Parts[dev_name]
                dev_map = device_map[dev.device.name]
                if 'col-pins' in dev_map:
                    count = count_pins(dev_map['col-pins'])
                    assert dev_map['num-elements'] == count, \
                           f"{self.name}: number of col-pins for {dev.device.name}, {count}, " \
                           f"!= num_elements, {dev_map['num-elements']}"
                if 'row-pins' in dev_map:
                    count = count_pins(dev_map['row-pins'])
                    assert dev_map['num-elements'] == count, \
                           f"{self.name}: number of row-pins for {dev.device.name}, {count}, " \
                           f"!= num_elements, {dev_map['num-elements']}"
                if device_number_attr_name is not None:
                    assert 'row-pins' in dev_map, \
                           f"{self.name}: no 'row-pins' for {dev.device.name} " \
                           f"with device_number_attr_name defined"
                    for pin_name in dev_map['row-pins']:
                        assert not isinstance(pin_name, (tuple, list)), \
                               f"{self.name}: device {dev.device.name} has a list row-pin, {pin_name}"
                        setattr(dev.get_unique_pin(pin_name), device_number_attr_name, device_number)
                        device_number += 1

    def get_rows_by_col(self, pin_names_name='col-pins', pin_name=None):
        r'''Yields a row tuple of (dev, pin), or (None, None) for each column.

        Each row is a tuple of (dev, pin), or (None, None).

        Indexed by [col][row]

        Maps against col-pins.

        If pin_name is passed, it is used directly, rather than looking up pins by pin_names_name.
        '''
        return grouper(self.get_all_connects(pin_names_name, pin_name), self.num_rows, fillvalue=(None, None))

    def get_cols_by_row(self, pin_name=None):
        r'''Yields a tuple of columns for each row.

        Each column is a tuple of (dev, pin), or (None, None).

        Indexed by [row][col]

        Maps against row-pins.

        If pin_name is passed, it is used directly, rather than looking up pins by pin_names_name.
        '''
        return zip_longest(*tuple(self.get_rows_by_col('row-pins', pin_name)),
                           fillvalue=(None, None))

    def get_all_connects(self, pin_names_name, pin_name=None):
        for dev_name in self.devices:
            if dev_name is None:
                yield None, None
            else:
                dev = Parts[dev_name]
                if pin_name is not None:
                    yield dev, pin_name
                else:
                    for p_name in self.device_map[dev.device.name][pin_names_name]:
                        if isinstance(p_name, (tuple, list)):
                            name, num = p_name
                            for i in range(num):
                                yield dev, name
                        else:
                            yield dev, p_name

    def make_connections(self, quiet=False):
        # connect each device to dev_inits.pins.connect and device_map.pins.connect
        for device_name in self.devices:
            if device_name is not None:
                device = Parts[device_name]
                if device_name in self.dev_inits:
                    self.connect_device(device, self.dev_inits[device_name]['pins'], quiet)
                dm = self.device_map[device.device.name]
                if 'pins' in dm:
                    self.connect_device(device, dm['pins'], quiet)

        if False:
            if all('row-pins' in value for value in self.device_map.values()):
                pin_name = None
            else:
                pin_name = 'dummy'
            print()
            print(f"{self.name}.make_connections: get_cols_by_row:")
            for row in self.get_cols_by_row(pin_name=pin_name):
                for (part, pin) in row:
                    if part is None:
                        print(f"{part}.{pin}", end=' ')
                    else:
                        print(f"{part.name}.{pin}", end=' ')
                print()
            if all('col-pins' in value for value in self.device_map.values()):
                pin_name = None
            else:
                pin_name = 'dummy'
            print()
            print(f"{self.name}.make_connections: get_rows_by_col:")
            for col in self.get_rows_by_col(pin_name=pin_name):
                for (part, pin) in col:
                    if part is None:
                        print(f"{part}.{pin}", end=' ')
                    else:
                        print(f"{part.name}.{pin}", end=' ')
                print()
            print()

        # cross-connect devices row-wise:
        def get_dest_connections(dest_device, pin_names_name):
            dev = Parts[dest_device]
            if isinstance(dev, List):
                if pin_names_name == 'row-pins':
                    #print(f"get_dest_connections: {dest_device=}, {pin_names_name=} doing by_row")
                    return dev.get_cols_by_row()
                #print(f"get_dest_connections: {dest_device=}, {pin_names_name=} doing by_col")
                return dev.get_rows_by_col()
            #print(f"get_dest_connections: {dest_device=}, {pin_names_name=} getting pins from device")
            ans = []
            for pin in getattr(dev, convert_name(pin_names_name)):
                #print(f"get_dest_connections: {dest_device=}, {pin_names_name=} yielding {dev=}, {pin=}")
                ans.append(((dev, pin),))
            return ans

        def process_connections(connections_attr):
            for dest_connect in getattr(self, connections_attr):
                if isinstance(dest_connect, (tuple, list)):
                    my_pin, dest_device = dest_connect
                else:
                    my_pin, dest_device = None, dest_connect
                if connections_attr == 'connect_by_row':
                    my_connects = tuple(self.get_cols_by_row(pin_name=my_pin))
                    dest_connects = tuple(get_dest_connections(dest_device, 'row-pins'))
                else:
                    my_connects = tuple(self.get_rows_by_col(pin_name=my_pin))
                    dest_connects = tuple(get_dest_connections(dest_device, 'col-pins'))
                #print(f"{len(my_connects)=}, {len(dest_connects)=}")
                #print("my_connects", my_connects)
                #print("dest_connects", dest_connects)
                for my_cols, dest_cols in zip(my_connects, dest_connects):
                    my_cols = tuple(my_cols)
                    dest_cols = tuple(dest_cols)
                    #print(f"{len(my_cols)=}, {len(dest_cols)=}")
                    #print("my_cols", my_cols)
                    #print("dest_cols", dest_cols)
                    if len(my_cols) == 1 and len(dest_cols) > 1:
                        my_cols = cycle(my_cols)
                    if len(dest_cols) == 1 and len(my_cols) > 1:
                        dest_cols = cycle(dest_cols)
                    for my_connect, dest_connect in zip(my_cols, dest_cols):
                        if my_connect[0] is not None and dest_connect[0] is not None:
                            #print(f"{self.name}: {my_connect=}, {dest_connect=}")
                            my_dev, my_pin = my_connect
                            my_dev.get_unique_pin(my_pin).connect_to(dest_connect, quiet=quiet)

        process_connections('connect_by_row')
        process_connections('connect_by_col')

    def connect_device(self, device, pins, quiet):
        for info in pins:  # info should have 'name' and, maybe, 'connect'
            pin_name = info['name']
            if 'connect' in info:
                for connect_clause in info['connect']:
                    #print(f"{self.name}.connect_device {device=}, {pin_name=}, {connect_clause=}")
                    device.get_unique_pin(pin_name).connect_to(connect_clause, quiet=quiet)

def count_pins(pins):
    ans = 0
    for pin in pins:
        if isinstance(pin, (tuple, list)):
            ans += pin[1]
        else:
            ans += 1
    return ans

def unique_justseen(iterable, key=None):
    "List unique elements, preserving order. Remember only the element just seen."
    # unique_justseen('AAAABBBCCDAABBB') --> A B C D A B
    # unique_justseen('ABBCcAD', str.lower) --> A B C A D
    return map(next, map(itemgetter(1), groupby(iterable, key)))

def ncycles(iterable, n):
    r'''Return the sequence elements n times.
    '''
    return chain.from_iterable(repeat(tuple(iterable), n))

def grouper(iterable, n, fillvalue=None):
    r'''Collect data into fixed-length chunks or blocks.

    grouper('ABCDEFG', 3, 'x') -> ABC DEF Gxx
    '''
    args = [iter(iterable)] * n
    return zip_longest(*args, fillvalue=fillvalue)

def get_wiring(filename, quiet=False):
    load_devices()
    yaml = read_yaml(filename)
    return parse_wiring(yaml, quiet)

def parse_wiring(yaml, quiet):
    prefixes = parse_prefixes(yaml['prefixes'])
    ans = {}
    ans['controls'] = parse_controls(prefixes, yaml['controls'])
    ans['devices'] = parse_devices(yaml['devices'])
    parse_internal_numbers(yaml['internal-numbers'])
    for p in Parts.values():
        p.make_connections(quiet)
    return ans

def parse_prefixes(prefixes):
    return {prefix: get_device(device_name)
            for prefix, device_name in prefixes.items()}

def parse_controls(prefixes, controls, into=None, position=None):
    if into is None:
        into = {}
    if isinstance(controls, dict):
        for key, value in controls.items():
            parse_controls(prefixes, value, into)
    elif isinstance(controls, (tuple, list)):
        for i, l2 in enumerate(controls):
            parse_controls(prefixes, l2, into, i)
    else:
        assert isinstance(controls, str)
        under = controls.find('_')
        hyphen = controls.find('-')
        if under < 0:
            assert hyphen > 0
            prefix = controls[:hyphen]
        elif hyphen < 0:
            prefix = controls[:under]
        else:
            prefix = controls[:min(under, hyphen)]
        device = prefixes[prefix]
        into[controls] = Part(controls, device, dict(position=position))
    return into

def parse_internal_numbers(internal_numbers):
    parse_pot_numbers(internal_numbers['pots'])
    parse_switch_numbers(internal_numbers['switches'])
    parse_led_numbers(internal_numbers['leds'])
    parse_numeric_display_numbers(internal_numbers['numeric-displays'])
    parse_alpha_display_numbers(internal_numbers['alpha-displays'])

def parse_pot_numbers(pots):
    i = 0
    for name in pots:
        p = Parts[name]
        while p is not None:
            print(f"{p.name}.pot_number is {i}")
            p.pot_number = i
            i += 1
            p = p.next_col

def parse_switch_numbers(switches):
    encoder_num = 0
    for row_num, row in enumerate(switches):
        col_num = 0
        for name in row:
            p = Parts[name]
            while p is not None and col_num < 9:
                if p.device.name == 'Rotary Encoder':
                    print(f"{p.name}.encoder_number is {encoder_num}, "
                          f"switch_number is {9 * row_num + col_num}")
                    p.encoder_number = encoder_num
                    encoder_num += 1
                    p.switch_number = 9 * row_num + col_num
                    col_num += 3
                else:
                    print(f"{p.name}.switch_number is {9 * row_num + col_num}")
                    p.switch_number = 9 * row_num + col_num
                    col_num += 1
                p = p.next_col
            if col_num >= 9:
                break

def parse_led_numbers(leds):
    row_num = 0
    for row in leds:
        col_num = 0
        for name in led_row(row):
            p = Parts[name]
            print(f"{p.name}.led_number is {16 * row_num + col_num}")
            p.led_number = 16 * row_num + col_num
            if p.device.name == 'LED-numeric-display':
                if col_num == 0:  # first of a pair making up 3 rows
                    col_num += 8
                else:             # second of a pair making up 3 rows
                    row_num += 2  # plus 1 more after the for loop...
            elif p.device.name == 'LED-alpha-display':
                col_num += 15
            else:
                col_num += 1
            if col_num >= 16:
                break
        row_num += 1

def led_row(row):
    for name in row:
        if isinstance(name, (tuple, list)):
            if isinstance(name[0], int):
                pos_matches = tuple(
                  filter(lambda p: isinstance(p, Part) and p.position == name[0],
                         Parts.values()))
                #print(f"led_row got position {name[0]}, {pos_matches=}")
                sorted_matches = sorted(pos_matches, key=lambda p: p.part_order)
                #print(f"{sorted_matches=}")
                for p in dropwhile(lambda p: p.name != name[1], sorted_matches):
                    yield p.name
                    if p.name == name[2]:
                        break
            elif name[0] == 'chain':
                for n in name[1:]:
                    p = Parts[n]
                    while p is not None:
                        yield p.name
                        p = p.next_col
            elif name[0] == 'range':
                prefix = name[1]
                while prefix[-1].isdigit(): prefix = prefix[:-1]
                start = int(name[1][len(prefix):])
                end = int(name[2][len(prefix):])
                for i in range(start, end + 1):
                    yield f"{prefix}{i}"
            else:
                raise AssertionError(f"parse_led_numbers: unknown order {name[0]!r}")
        else:
            yield name

def parse_numeric_display_numbers(numeric_displays):
    for i, name in enumerate(numeric_displays):
        p = Parts[name]
        print(f"{p.name}.numeric_display_number is {i}")
        p.numeric_display_number = i

def parse_alpha_display_numbers(alpha_displays):
    for i, units in enumerate(alpha_displays):
        for name in units:
            p = Parts[name]
            print(f"{p.name}.alpha_display_number is {i}")
            p.alpha_display_number = i

def parse_devices(devices):
    ans = {}
    for name, desc in devices.items():
        ans[name] = parse_device(name, desc, ans) 
    return ans

def parse_device(name, desc, into):
    attrs = desc.copy()
    type = attrs.pop('type')
    if isinstance(type, str):
        if type == 'list':
            device_map = attrs.pop('device-map')
            into[name] = List(name, device_map,
                              **{convert_name(key): value
                                 for key, value in attrs.items()}
                             )
        else:
            into[name] = Part(name, get_device(type), attrs)
    else:
        assert type[0] == 'batch'
        assert len(type) == 2
        batch = Batch(name, get_device(type[1]), attrs)
        into[name] = batch
        for p in batch.devices:
            into[p.name] = p

def convert_keys(value):
    if isinstance(value, dict):
        return {convert_name(key): convert_keys(v)
                for key, v in value.items()}
    elif isinstance(value, (tuple, list)):
        return [convert_keys(v) for v in value]
    return value



if __name__ == "__main__":
    import argparse

    argparser = argparse.ArgumentParser()
    argparser.add_argument('-p', "--part")
    argparser.add_argument('-l', "--little", action='store_true')
    argparser.add_argument('-d', "--devices", action='store_true')
    argparser.add_argument('-i', "--isolated", action='store_true')
    argparser.add_argument('-s', "--summary", action='store_true')
    argparser.add_argument('-q', "--quiet", action='store_true')

    args = argparser.parse_args()

    wiring = get_wiring("exp_console.yaml", args.quiet)

    if args.summary:
        print(len(Parts), "parts")
        print(len(wiring['controls']), "controls")
        print(len(wiring['devices']), "devices")
    elif args.devices:
        for name, device in all_devices():
            if hasattr(device, 'num_onhand'):
                if device.num_onhand < len(device.instances):
                    print(f"{name + ':':21}need {len(device.instances):3}, {device.num_onhand:3} onhand, "
                          f"ORDER {len(device.instances) - device.num_onhand:3}")
                else:
                    print(f"{name + ':':21}need {len(device.instances):3}, {device.num_onhand:3} onhand")
            else:
                print(f"{name + ':':21}need {len(device.instances):3}, unknown onhand")
    elif args.little:
        little_stuff_onhand = get_device('Little-stuff')
        for name, count in sorted(Little_stuff.items()):
            if hasattr(little_stuff_onhand, convert_name(name)):
                num_onhand = getattr(little_stuff_onhand, convert_name(name)).num_onhand
                if num_onhand < count:
                    print(f"{name+':':20} need {count:3}, {num_onhand:3} onhand, ORDER {count - num_onhand:3}")
                else:
                    print(f"{name+':':20} need {count:3}, {num_onhand:3} onhand")
            else:
                print(f"{name+':':20} need {count:3}, unknown onhand")
    elif args.isolated:
        for name, part in Parts.items():
            if not part.connected and not isinstance(part, List):
                print(f"part {name} not connected")
    elif args.part:
        if args.part != 'all':
            Parts[args.part].dump()
        else:
            lines_per_page = 44
            lines_left = lines_per_page
            need_blank_line = False
            for name, part in sorted(Parts.items()):
                if not isinstance(part, List):
                    lines = part.gen_dump_lines()
                    if len(lines) + need_blank_line > lines_left:
                        for i in range(lines_left):
                            print()
                        lines_left = lines_per_page
                        need_blank_line = False
                    if need_blank_line:
                        print()
                        lines_left -= 1
                    for line in lines:
                        print(line)
                    lines_left -= len(lines)
                    need_blank_line = True
    else:
        argparser.print_help()
