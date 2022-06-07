# parse_wiring.py

from collections import defaultdict

from .utils import read_yaml
from .parse_devices import load_devices, get_device, convert_name


Parts = {}
Connections = defaultdict(list)

class Part:
    pins = ()
    fans_out_to = ()

    def __init__(self, name, device, attrs={}):
        self.name = name
        self.device = device
        if name in Parts:
            raise AssertionError(f"Duplicate name, {name!r}")
        Parts[name] = self
        for name, value in attrs.items():
            setattr(self, convert_name(name), value)
        self.init2()

    def init2(self):
        self.device.add_instance(self)

    def __repr__(self):
        return f"<{self.__class__.__name__} {self.name}>"

    def make_connections(self):
        for p in self.device.get_ordered_pins():
            for clause in p.connect:
                Connection(self, p, clause)
        for my_pin in self.pins:
            device_pin = self.device.get_unique_pin(my_pin['name'])
            for clause in my_pin.get('connect', ()):
                Connection(self, device_pin, clause)
        if self.fans_out_to:
            fanout_pins = []
            for name in self.device.fanout_pins:
                device_pin = self.device.get_unique_pin(name)
                fanout_pins.append(device_pin)
            connects = self.get_fan_out_to_connects()
            assert len(fanout_pins) >= len(connects), \
                   f"{self}: too many fans_out_to parts"
            #print(f"{self}.make_connections, fans_out_to {connects}")
            for my_pin, connect_to in zip(fanout_pins, connects):
                Connection(self, my_pin, connect_to)

    def get_fan_out_to_connects(self):
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


class Batch(Part):
    def init2(self):
        self.devices = tuple(Part(v['name'], self.device, v) for v in self.devices)

    def make_connections(self):
        if hasattr(self, 'batch_common_pins'):
            first_part = self.devices[0]
            for dest_part in self.devices[1:]:
                for pin_name in self.batch_common_pins:
                    Connection(first_part, first_part.device.get_unique_pin(pin_name),
                               (dest_part, pin_name))
        to_parts = tuple(Parts[name] for name in self.fans_out_to)
        for my_part in self.devices:
            fanout_pins = my_part.device.fanout_pins
            assert len(to_parts) <= len(fanout_pins), \
                   f"{self}.make_connections: more fans-out-to than " \
                   f"{my_part.name} fanout-pins"
            to_pin_name = my_part.links_pin
            for to_part, fanout_pin in zip(to_parts, fanout_pins):
                Connection(my_part, my_part.device.get_unique_pin(fanout_pin),
                           (to_part, to_pin_name))


class Connection:
    def __init__(self, from_part, from_pin, connect_spec):
        self.from_part = from_part
        self.from_pin = from_pin
        self.connect_spec = connect_spec
        Connections[self.from_part, self.from_pin].append(self)


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
    wiring = get_wiring("exp_console.yaml")
    print(len(Parts), "parts")
    print(len(Connections), "connections")
    print(len(wiring['controls']), "controls")
    print(len(wiring['devices']), "devices")
