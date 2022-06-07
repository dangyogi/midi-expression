# parse_wiring.py

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
                if isinstance(clause, str):
                    p.connect_to(clause)
                elif len(clause) == 1:
                    clause = clause[0]
                    assert isinstance(clause, str)
                    p.connect_to(clause)
                else:
                    part_name, pin = clause
                    if part_name[0].isdigit() or part_name[0] in '+-.' or \
                       part_name == 'GND':
                        p.connect_to(part_name, pin)
                    else:
                        p.connect_to(Parts[part_name], pin)
        if self.fans_out_to:
            fanout_pins = [self.pins_by_name[name] for name in self.fanout_pins]
            connects = self.get_fan_out_to_connects()
            assert len(fanout_pins) >= len(connects), \
                   f"{self}: too many fans_out_to parts"
            #print(f"{self}.make_connections, fans_out_to {connects}")
            for my_pin, connect_to in zip(fanout_pins, connects):
                if isinstance(connect_to, str):
                    my_pin.connect_to(connect_to)
                else:
                    my_pin.connect_to(*connect_to)

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

    def connect_to(self, part, pin=None, one_way=False):
        if pin is None:
            assert not isinstance(part, Part)
            Little_stuff[part] += 1
            self.connections.append((part,))
        elif isinstance(part, Part):
            self.connections.append((part, pin))
        else:
            assert isinstance(part, str)
            assert part[0].isdigit() or part[0] in '+-.' or part == 'GND', \
                   f"connect_to got invalid {part=}"
            Little_stuff[part] += 1
            self.connections.append((part, pin))
        #if not one_way and isinstance(part, Part) and pin is not None:
        #    part.get_unique_pin(pin).connect_to(self.parent, self.name, True)

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
    dont_attrs = Part.dont_attrs.union(('devices',))

    def init2(self):
        self.devices = tuple(Part(v['name'], self.device, v) for v in self.devices)

    def make_connections(self):
        if hasattr(self, 'batch_common_pins'):
            first_part = self.devices[0]
            for dest_part in self.devices[1:]:
                for pin_name in self.batch_common_pins:
                    first_part.get_unique_pin(pin_name).connect_to(dest_part, pin_name)
        to_parts = tuple(Parts[name] for name in self.fans_out_to)
        for my_part in self.devices:
            fanout_pins = my_part.device.fanout_pins
            assert len(to_parts) <= len(fanout_pins), \
                   f"{self}.make_connections: more fans-out-to than " \
                   f"{my_part.name} fanout-pins"
            to_pin_name = my_part.links_pin
            for to_part, fanout_pin in zip(to_parts, fanout_pins):
                my_part.get_unique_pin(fanout_pin).connect_to(to_part, to_pin_name)


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
        into[name] = Part(name, get_device(type), attrs)
    else:
        assert type[0] == 'batch'
        assert len(type) == 2
        batch = Batch(name, get_device(type[1]), attrs)
        into[name] = batch
        for p in batch.devices:
            into[p.name] = p




if __name__ == "__main__":
    import argparse

    argparser = argparse.ArgumentParser()
    argparser.add_argument('-p', "--part")
    argparser.add_argument('-l', "--little", action='store_true')
    argparser.add_argument('-d', "--devices", action='store_true')

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
    else:
        Parts[args.part].dump()
