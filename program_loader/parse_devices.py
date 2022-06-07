# parse_devices.py

from collections import defaultdict

from .utils import read_yaml


class AttrNameError(AttributeError):
    pass


class Attrs:
    dont_attrs = frozenset()
    dont_override = frozenset()
    dont_inherit = frozenset(('name', 'parent', 'uses', '__class__', 'get_my_attr'))

    def __init__(self, name, attrs, parent=None, uses=None):
        self.name = name
        self.parent = parent
        self.uses = uses
        for key, value in attrs.items():
            if key == 'uses':
                pass
                #print(f"Attrs.__init__ got 'uses' in attrs: {self.uses=}, {value=}")
            elif key != 'name':
                if key in self.get_my_attr('dont_attrs'):
                    setattr(self, convert_name(key), value)
                else:
                    if self.uses is None:
                        value_uses = None
                    else:
                        value_uses = getattr(self.uses, key, None)
                    setattr(self, convert_name(key),
                            Attrs.to_attrs(key, value, self, uses=value_uses))

    @classmethod
    def to_attrs(cls, name, value, parent, uses=None):
        if isinstance(value, dict):
            if 'name' in value:
                return cls(value['name'], value, parent, uses)
            try:
                return cls(name, value, parent, uses)
            except AttrNameError:
                return {key: Attrs.to_attrs(key, desc, parent, uses)
                        for key, desc in value.items()}
        if isinstance(value, (tuple, list)):
            if name == 'pins':
                return tuple(Pin.to_attrs('unnamed', v, parent) for v in value)
            else:
                return tuple(Attrs.to_attrs('unnamed', v, parent, uses) for v in value)
        if value is None or isinstance(value, (str, int, float)):
            return value
        raise AttributeError(
                f"{parent}.{name} unknown value type {type(value)} for value {value}")

    def __repr__(self):
        if self.parent is not None:
            return f"<{self.__class__.__name__}: {self.parent}.{self.name}>"
        return f"<{self.__class__.__name__}: {self.name}>"

    def get_my_attr(self, name):
        return object.__getattribute__(self, name)

    def __getattribute__(self, name):
        dont_inherit = object.__getattribute__(self, 'dont_inherit')
        if name in dont_inherit:
            return object.__getattribute__(self, name)
        try:
            uses = self.get_my_attr('uses')
        except AttributeError:
            print(f"{self}.__getattribute__: caught AttributeError on 'uses'")
            raise
        if uses is None:
            return self.get_my_attr(name)
        assert uses is not self
        if name in self.get_my_attr('dont_override'):
            #print("__getattribute__ dont_override", name, "uses", uses)
            return getattr(uses, name)
        inherited_value = getattr(uses, name, None)
        try:
            my_value = self.get_my_attr(name)
        except AttributeError:
            if inherited_value is None:
                raise
            return inherited_value

        # This is basically for 'connect'
        if isinstance(inherited_value, (tuple, list)) and \
           isinstance(my_value, (tuple, list)):
            # concatenate values
            return tuple(inherited_value) + tuple(my_value)

        return my_value


Devices = {}

class Device(Attrs):
    dont_override = {'pins'}
    dont_inherit = Attrs.dont_inherit.union(('pins_by_name', 'instances'))

    def __init__(self, name, desc):
        global Devices
        if 'uses' in desc:
            Attrs.__init__(self, name, desc, uses=Devices[desc['uses'].lower()])
        else:
            Attrs.__init__(self, name, desc)
        if 'pins' in desc:
            self.pins_by_name = defaultdict(list)
            for i, p in enumerate(self.get_my_attr('pins'), 1):
                if self.uses is None:
                    p.pin_number = i
                else:
                    inherited_pin = self.uses.get_unique_pin(p.name)
                    p.pin_number = inherited_pin.pin_number
                    assert p.uses is None
                    p.uses = inherited_pin
                self.add_pin(p.name, p)
                if hasattr(p, 'aka'):
                    if isinstance(p.aka, (tuple, list)):
                        for name in p.aka:
                            self.add_pin(name, p)
                    else:
                        self.add_pin(p.aka, p)
            if self.uses is None:
                self.num_pins = i
        self.instances = []
        Devices[self.name.lower()] = self

    def add_pin(self, name, pin):
        self.pins_by_name[name].append(pin)

    def get_pins(self, name):
        r'''Returns a list of pins.

        Raises KeyError if none found.
        '''
        try:
            pins_by_name = self.pins_by_name
        except AttributeError:
            if self.uses is None:
                raise KeyError(repr(name))
            return self.uses.get_pins(name)
        if name not in pins_by_name:
            if self.uses is not None:
                return self.uses.get_pins(name)
            raise KeyError(name)
        return pins_by_name[name]

    def get_unique_pin(self, name):
        pins = self.get_pins(name)
        assert len(pins) == 1, f"{self}.get_unique_pin: {name} not unique"
        return pins[0]

    def get_pin_number(self, pin_number):
        name = self.pins[pin_number - 1].name
        for p in self.get_pins(name):
            if p.pin_number == pin_number:
                return p
        raise AssertionError(
                f"{self}.get_pin_number: something's wrong, "
                f"couldn't find pin {name}, pin_number {pin_number}")

    def get_ordered_pins(self):
        return tuple(self.get_pin_number(pin_number)
                     for pin_number in range(1, self.num_pins + 1))

    def add_instance(self, part):
        self.instances.append(part)

    def __repr__(self):
        return f"<Device: {self.name}>"


class Pin(Attrs):
    inverted = False
    comment = None
    connect = ()

    def dump(self):
        print(f"{self.name=}")
        print(f"{self.pin_number=}")
        print(f"{self.uses=}")
        print(f"{self.connect=}")
        print(f"{self.comment=}")
        print(f"{self.inverted=}")
        print(f"{self.parent=}")


def convert_name(name):
    try:
        return name.replace('-', '_')
    except AttributeError:
        raise AttrNameError

def load_devices(filename="devices.yaml"):
    yaml = read_yaml(filename)
    parse_devices(yaml)
    return Devices

def parse_devices(yaml):
    for name, desc in yaml.items():
        Device(name, desc)

def get_device(name):
    return Devices[name.lower()]

def all_devices():
    return sorted(Devices.items())


if __name__ == "__main__":
    devices = load_devices()
    print(tuple(devices.keys()))
    print(f"{len(devices)} devices")
    io_output = devices['i/o expander output']
    print(f"{io_output.I2C.init=}")
    print(f"{io_output.num_pins=}")
    print()
    print(f"{io_output.pins=}")
    print()
    print(f"{io_output.get_ordered_pins()=}")
    print()
    print(f"{len(io_output.get_ordered_pins())=}")
    print()
    print(f"{io_output.I2C.commands=}")
    print()
    print("io_output.get_pins('GP1.0'):")
    for p in io_output.get_pins('GP1.0'):
        p.dump()
        print()
    print("io_output.get_pins('CLK'):")
    for p in io_output.get_pins('CLK'):
        p.dump()
        print()

