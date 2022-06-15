# parse_wiring.py

from itertools import zip_longest, chain, repeat
from collections import defaultdict, Counter

from .utils import read_yaml
from .parse_devices import Attrs, load_devices, get_device, all_devices, convert_name


Parts = {}
Little_stuff = Counter()

class Part(Attrs):
    dont_inherit = Attrs.dont_inherit.union(('pins_by_name', 'pins_by_number'))
    dont_attrs = Attrs.dont_attrs.union(('pins',))
    pins = ()
    fans_out_to = ()
    connected = False

    def __init__(self, name, device, attrs={}):
        Attrs.__init__(self, name, attrs, uses=device)
        self.device = device
        if name in Parts:
            raise AssertionError(f"Duplicate name, {name!r}")
        Parts[name] = self
        self.init2()

    def init2(self):
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

    def __repr__(self):
        return f"<{self.__class__.__name__} {self.name}>"

    def get_unique_pin(self, name):
        #if name not in self.pins_by_name:
        #    self.pins_by_name[name] = Pin(name, attrs, parent=self,
        #                                  uses=self.device.get_unique_pin(name))
        return self.pins_by_name[name]

    def make_connections(self):
        for p in self.pins_by_number:
            for clause in p.connect:
                p.connect_to(clause)
        if self.fans_out_to:
            fanout_pins = [self.pins_by_name[name] for name in self.fanout_pins]
            connects = self.get_fan_out_to_connects()
            assert len(fanout_pins) >= len(connects), \
                   f"{self}: too many fans_out_to parts"
            #print(f"{self}.make_connections, fans_out_to {connects}")
            for my_pin, clause in zip(fanout_pins, connects):
                my_pin.connect_to(clause)

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
        print(f"{self.name}: {self.device.name}")
        print()
        if self.device.num_rows == 1:
            for pin in self.pins_by_number:
                print(pin.desc(reverse=True))
        else:
            left_side = equalize((pin.desc()
                                  for pin in self.pins_by_number[:self.num_pins//2]),
                                 '>')
            right_side = equalize((pin.desc(reverse=True)
                                   for pin
                                    in reversed(self.pins_by_number[self.num_pins//2:])),
                                 '<')
            print(' ' * len(left_side[0]), '+----+')
            for left, right in zip(left_side, right_side):
                print(left, '-|    |-', right.rstrip(), sep='')
            print(' ' * len(left_side[0]), '+----+')

def box(height, width=6):
    yield '+' + '-' * (width - 2) + '+'
    for i in range(height - 2):
        yield '|' + ' ' * (width - 2) + '|'
    yield '+' + '-' * (width - 2) + '+'

def equalize(list_of_chunks, align):
    list_of_chunks = tuple(list_of_chunks)
    widths = [max(len(x[i]) for x in list_of_chunks)
              for i in range(len(list_of_chunks[0]))]
    return [' '.join(f"{chunk:{align}{width}}"
                     for chunk, width in zip(chunks, widths))
            for chunks in list_of_chunks]

class Pin(Attrs):
    dont_inherit = Attrs.dont_inherit.union(('connections',))

    def __init__(self, name, attrs, parent=None, uses=None):
        Attrs.__init__(self, name, attrs, parent, uses)
        self.connections = []

    def connect_to(self, clause, one_way=False):
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
                    print(f"{self.parent}.{self.name}: connect_to got invalid {part=}")

        if isinstance(part, str):
            Little_stuff[part] += 1
            if pin is None:
                self.connections.append((part,))
            else:
                self.connections.append((part, pin))
        else:
            assert pin is not None, \
                   f"{self.parent}.{self.name}: connect_to part, {part.name}, must have pin"
            self.connections.append((part, pin))
            self.parent.connected = True
            part.connected = True

            #if not one_way:
            #    part.get_unique_pin(pin).connect_to((self.parent, self.name), True)

    def desc(self, reverse=False):
        if reverse:
            align = "<"
            arrow = "->"
        else:
            align = ">"
            arrow = "<-"
        conn_fragments = []
        for conn in self.connections:
            if isinstance(conn, str):
                conn_fragments.append(str)
            elif not isinstance(conn[0], Part):
                conn_fragments.append(' '.join(conn))
            else:
                conn_fragments.append(f"{conn[0].name}.{conn[1]}")
        chunks = [', '.join(conn_fragments),
                  (arrow if conn_fragments else '  '),
                  self.name,
                  f"{self.pin_number}"
                 ]
        if reverse:
            chunks.reverse()
        return chunks

class Batch(Part):
    connected = False
    dont_attrs = Part.dont_attrs.union(('devices',))

    def init2(self):
        self.devices = tuple(Part(v['name'], self.device, v) for v in self.devices)

    def make_connections(self):
        if hasattr(self, 'batch_common_pins'):
            first_part = self.devices[0]
            for dest_part in self.devices[1:]:
                for pin_name in self.batch_common_pins:
                    first_part.get_unique_pin(pin_name).connect_to((dest_part, pin_name))
        to_parts = tuple(Parts[name] for name in self.fans_out_to)
        for my_part in self.devices:
            fanout_pins = my_part.device.fanout_pins
            assert len(to_parts) <= len(fanout_pins), \
                   f"{self}.make_connections: more fans-out-to than " \
                   f"{my_part.name} fanout-pins"
            to_pin_name = my_part.links_pin
            for to_part, fanout_pin in zip(to_parts, fanout_pins):
                my_part.get_unique_pin(fanout_pin).connect_to((to_part, to_pin_name))

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
                 connect=(), **dev_inits):
        self.name = name
        self.device_map = device_map
        self.num_rows = num_rows
        self.num_cols = num_cols
        self.connect = connect
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

    def get_connect_by_row(self, pin_names_name='row-pins'):
        r'''Yields a tuple of (dev, pin), or (None, None) for each row.

        Indexed by [row][col]
        '''
        return grouper(self.get_all_connects(pin_names_name), self.num_rows, fillvalue=(None, None))

    def get_connect_by_col(self):
        r'''Yields a tuple of (dev, pin), or (None, None) for each column.

        Indexed by [col][row]
        '''
        return zip_longest(*tuple(self.get_connect_by_row('col-pins')),
                           fillvalue=(None, None))

    def get_all_connects(self, pin_names_name):
        for dev_name in self.devices:
            if dev_name is None:
                yield None, None
            else:
                dev = Parts[dev_name]
                for pin_name in self.device_map[dev.device.name][pin_names_name]:
                    if isinstance(pin_name, (tuple, list)):
                        name, num = pin_name
                        for i in range(num):
                            yield dev, name
                    else:
                        yield dev, pin_name

    def make_connections(self):
        # connect each device to dev_inits.pins.connect and device_map.pins.connect
        for device_name in self.devices:
            if device_name is not None:
                device = Parts[device_name]
                if device_name in self.dev_inits:
                    self.connect_device(device, self.dev_inits[device_name]['pins'])
                dm = self.device_map[device.device.name]
                if 'pins' in dm:
                    self.connect_device(device, dm['pins'])
        # connect each device to dev_inits.pins.connect and device_map.pins.connect
        if all('row-pins' in value for value in self.device_map.values()):
            print()
            print(f"{self.name}.make_connections: get_connect_by_row:")
            for row in self.get_connect_by_row():
                for (part, pin) in row:
                    if part is None:
                        print(f"{part}.{pin}", end=' ')
                    else:
                        print(f"{part.name}.{pin}", end=' ')
                print()
        if all('col-pins' in value for value in self.device_map.values()):
            print()
            print(f"{self.name}.make_connections: get_connect_by_col:")
            for col in self.get_connect_by_col():
                for (part, pin) in col:
                    if part is None:
                        print(f"{part}.{pin}", end=' ')
                    else:
                        print(f"{part.name}.{pin}", end=' ')
                print()
        print()

    def connect_device(self, device, pins):
        for info in pins:  # info should have 'name' and, maybe, 'connect'
            pin_name = info['name']
            if 'connect' in info:
                for connect_clause in info['connect']:
                    #print(f"{self.name}.connect_device {device=}, {pin_name=}, {connect_clause=}")
                    device.get_unique_pin(pin_name).connect_to(connect_clause)

def count_pins(pins):
    ans = 0
    for pin in pins:
        if isinstance(pin, (tuple, list)):
            ans += pin[1]
        else:
            ans += 1
    return ans

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

def get_wiring(filename):
    load_devices()
    yaml = read_yaml(filename)
    return parse_wiring(yaml)

def parse_wiring(yaml):
    prefixes = parse_prefixes(yaml['prefixes'])
    ans = {}
    ans['controls'] = parse_controls(prefixes, yaml['controls'])
    ans['devices'] = parse_devices(yaml['devices'])
    for p in Parts.values():
        p.make_connections()
    return ans

def parse_prefixes(prefixes):
    return {prefix: get_device(device_name)
            for prefix, device_name in prefixes.items()}

def parse_controls(prefixes, controls, into=None):
    if into is None:
        into = {}
    if isinstance(controls, dict):
        for key, value in controls.items():
            parse_controls(prefixes, value, into)
    elif isinstance(controls, (tuple, list)):
        for l2 in controls:
            parse_controls(prefixes, l2, into)
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
        into[controls] = Part(controls, device)
    return into

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

    args = argparser.parse_args()

    wiring = get_wiring("exp_console.yaml")
    print(len(Parts), "parts")
    print(len(wiring['controls']), "controls")
    print(len(wiring['devices']), "devices")
    print()

    if args.devices:
        for name, device in all_devices():
            print(f"{name:20} {len(device.instances):3}")
    elif args.little:
        for name, count in sorted(Little_stuff.items()):
            print(f"{name}: {count}")
    elif args.isolated:
        for name, part in Parts.items():
            if not part.connected:
                print(f"part {name} not connected")
    else:
        Parts[args.part].dump()
