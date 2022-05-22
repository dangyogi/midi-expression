# gen_synth_settings.py

import math
from collections.abc import Mapping


from .utils import read_yaml, calc_geom


Reserved_control_numbers = frozenset((
    0x01,  # modulation
    0x06,  # data entry (course)
    0x07,  # channel volume
    0x0A,  # pan
    0x0B,  # expression controller
    0x26,  # data entry (fine)
    0x40,  # sustain
    0x62,  # Non-Reg param number LSB
    0x63,  # Non-Reg param number MSB
    0x64,  # Reg param number LSB
    0x65,  # Reg param number MSB
    0x79,  # Reset all controllers
    0x7B,  # All notes off
))

Reserved_system_common = frozenset((
    0xF0,  # System Exclusive
    0xF1,  # MIDI Time Code Quarter Frame
    0xF2,  # Song Position Pointer
    0xF3,  # Song Select
    0xF6,  # Tune Request
    0xF7,  # End of System Exclusive Message
))


class Thing:
    kw_args = dict(identified_by=None,
                   ignore_control_codes=[],
                   subordinates={},
                   settings={},
              )

    def __init__(self, name, location, **args):
        self.name = name
        if location:
            self.full_name = f"{location}.{name}"
        else:
            self.full_name = name
        seen = set()
        for key, value in args.items():
            lkey = key.lower()
            if lkey not in self.kw_args:
                raise TypeError(f"{self.full_name}, unexpected argument {key}")
            setattr(self, lkey, value)
            seen.add(lkey)
        for key, default in self.kw_args.items():
            if key not in seen:
                setattr(self, key, default)
        assert isinstance(self.ignore_control_codes, list)
        self.ignore_control_codes = frozenset(self.ignore_control_codes)
        self.subordinates = parse_things(self.subordinates, self.full_name)
        self.settings = parse_settings(self.settings, self.full_name, self)

    def __str__(self):
        return f"<Thing: {self.full_name}>"

    def dump(self, indent=''):
        print(indent, f"Thing: {self.name}", sep='')
        indent += '  '
        if self.settings:
            print(indent, "settings:", sep='')
            for setting in self.settings.values():
                setting.dump(indent + '  ')
        if self.subordinates:
            print(indent, "subordinates:", sep='')
            for subordinate in self.subordinates.values():
                subordinate.dump(indent + '  ')


Settings = []


class Setting:
    kw_args = dict(immediate=False,
                   parameters=(),
                   non_registered_parameter=None,
                   control_change=None,
                   system_common=None,
                   kill_value=None,
              )

    def __init__(self, name, location, thing, group, **args):
        self.name = name
        self.full_name = f"{location}={name}"
        self.thing = thing
        self.group = group
        seen = set()
        for key, value in args.items():
            lkey = key.lower()
            if lkey not in self.kw_args:
                raise TypeError(f"{self.full_name}, unexpected argument {key}")
            setattr(self, lkey, value)
            seen.add(lkey)
        for key, default in self.kw_args.items():
            if key not in seen:
                setattr(self, key, default)
        self.parameters = parse_parameters(self.parameters, self.full_name, self)
        offset = system_common_offset = 0
        for p in reversed(self.parameters):
            p.offset = offset
            p.system_common_offset = system_common_offset
            system_common_offset += p.bits
            if not p.is_constant:
                offset += p.bits
        self.message_formatters = []
        if 'non_registered_parameter' in args:
            self.message_formatters.append(
              Non_registered_parameter(self, args['non_registered_parameter']))
        if 'control_change' in args:
            self.message_formatters.append(
              Control_change(self, args['control_change']))
        if 'system_common' in args:
            self.message_formatters.append(
              System_common(self, args['system_common']))
        Settings.append(self)

    def __str__(self):
        return f"<Setting: {self.full_name}>"

    def dump(self, indent=''):
        print(indent, f"Setting: {self.name}", sep='')

    def gen_python_code(self):
        pass

    def gen_exp_code(self):
        pass


class Message_format:
    allow_options = ("add",)

    def __init__(self, setting, config):
        self.setting = setting
        if not isinstance(config, Mapping):
            config = dict(code=config)
        assert "code" in config
        self.code = self.my_code = config.pop("code")
        for option in self.allow_options:
            if option not in config:
                setattr(self, option, None)
            else:
                name = config.pop(option)
                setattr(self, option, name)
                for p in setting.parameters:
                    if p.name == name:
                        self.param = p
                        if p.is_constant:
                            if option == "add":
                                self.my_code += p.value
                            self.constant_param = True
                        else:
                            self.constant_param = False
                        break
                else:
                    raise AssertionError(
                            f"{setting} {self.__class__.__name__}: "
                            f"{option} parameter, '{name}', not found")
        assert not config, f"{setting} {self.__class__.__name__}: " \
                           f"unknown parameters {tuple(config.keys())}"
        self.init()


class Non_registered_parameter(Message_format):
    NR_param_numbers_used = set()

    def init(self):
        if self.add is None or self.constant_param:
            codes_to_check = (self.my_code,)
        else:
            codes_to_check = range(self.code,
                                   self.code + (self.param.limit - self.param.start) + 1)
        #print("NR param nums:", ', '.join(hex(c) for c in codes_to_check))
        for code in codes_to_check:
            assert code not in self.NR_param_numbers_used, \
              f"{self.setting} non_registered_parameter: 0x{code:X} is already assigned"
            self.NR_param_numbers_used.add(code)


class Control_change(Message_format):
    Control_numbers_used = set()

    def init(self):
        if self.add is None or self.constant_param:
            codes_to_check = (self.my_code,)
        else:
            codes_to_check = range(self.my_code,
                                   self.my_code + (self.param.limit - self.param.start) + 1)
        #print("Control_change:", ', '.join(hex(c) for c in codes_to_check))
        for code in codes_to_check:
            assert code not in Reserved_control_numbers, \
              f"{self.setting} control_change: 0x{code:X} is a reserved code"
            assert code not in self.Control_numbers_used, \
              f"{self.setting} control_change: 0x{code:X} is already assigned"
            self.Control_numbers_used.add(code)



class System_common(Message_format):
    allow_options = ("key_on",)

    Offset_lengths = {}   # {code: (length, offset)}

    def init(self):
        assert self.code not in Reserved_system_common, \
          f"{self.setting} system_common: 0x{self.code:X} is a reserved code"
        if self.key_on is None:
            if self.code not in self.Offset_lengths:
                self.Offset_lengths[self.code] = (0, 0)
            else:
                assert self.Offset_lengths[self.code] == (0, 0), \
                  f"{self.setting} system_common: Conflict with other key_on setting " \
                  f"{self.Offset_lengths[self.code]}"
        else:
            assert self.constant_param, \
              f"{self.setting} system_common: key_on parameter must be constant"
            length = self.param.bits
            offset = self.param.system_common_offset
            if self.code not in self.Offset_lengths:
                self.Offset_lengths[self.code] = (length, offset)
            else:
                assert self.Offset_lengths[self.code] == (length, offset), \
                  f"{self.setting} system_common ({length}, {offset}): " \
                  f"Conflict with other key_on setting {self.Offset_lengths[self.code]}"


class Group:
    def __init__(self, name, location, thing, group, **args):
        self.name = name
        self.full_name = f"{location}>{name}"
        self.thing = thing
        self.group = group
        self.choices = parse_settings(args, self.full_name, thing, self)
        assert self.choices

    def __str__(self):
        return f"<Group: {self.full_name}>"

    def dump(self, indent=''):
        print(indent, f"Group: {self.name}", sep='')
        for choice in self.choices.values():
            choice.dump(indent + '  ')


class Parameter:
    is_constant = False
    required = frozenset()
    kw_args = {}

    def __init__(self, name, location, setting, **args):
        self.name = name
        self.full_name = f"{location}^{name}"
        self.setting = setting
        seen = set()
        for key in self.required:
            if key not in args:
                raise TypeError(f"{self.__class__.__name__}: {self.full_name}, "
                                f"unexpected argument {key}")
            setattr(self, key, args[key])
            seen.add(key)
        for key, value in args.items():
            lkey = key.lower()
            if lkey not in seen:
                if lkey not in self.kw_args:
                    raise TypeError(f"{self.__class__.__name__}: {self.full_name}, "
                                    f"unexpected argument {key}")
                setattr(self, lkey, value)
                seen.add(lkey)
        for key, default in self.kw_args.items():
            if key not in seen:
                setattr(self, key, default)
        self.init()

    def __str__(self):
        return f"<{self.__class__.__name__}: {self.full_name}>"

    def dump(self, indent=''):
        print(indent, f"{self.__class__.__name__}: {self.name}", sep='')


class Lin_param(Parameter):
    kw_args = dict(start=None,
                   bits=None,
                   signed=None,        # has bits or limit
                   steps_per_unit=None,
                   range=None,
                   limit=None,
                   step_size=None,     # has bits, sometimes start
                   units=None,
                   labels=None,
                   default=(),         # () can't be a legal value
                   alignment=None,
                   max_steps=None,     # has range
                   null_value=None,
                   kills_value=(),     # () can't be a legal value
                   to_python=None,     # ignore, as_index_in: List, Hz_to_cents, sub1,
                                       # to_cents, add_closest_semitone, as_percent_of: N
                   center_value=None,  # param: <name>, what: <known_name>
                                       # + show: true/false (incl in value displayed on
                                       #                     exp console?)
                                       # this gets added to signed value, so doesn't affect
                                       # the m/b calculations.
                   python_arg_name=None,
              )

    def init(self):
        # value = self.step_size*param + self.start + <center_value>
        if self.range is not None:
            assert self.start is None, \
              f"{self}: start not None while range is not None"
            assert self.limit is None, \
              f"{self}: limit not None while range is not None"
            self.start, self.limit = self.range

        # set self.step_size
        assert self.step_size is None or self.steps_per_unit is None, \
          f"{self}: both step_size and steps_per_unit set"
        if self.step_size is None:
            if self.steps_per_unit is not None:
                self.step_size = 1/self.steps_per_unit

        if self.limit is not None:
            #assert self.start is None or self.bits is None, \
            #  f"{self}: start not None and bits not None while limit is not None"
            assert self.start is not None or self.bits is not None, \
              f"{self}: start is None and bits is None while limit is not None"
            if self.start is not None:
                assert self.signed is None or not self.signed, \
                  f"{self}: signed while start and limit are not None"
                if self.bits is not None:
                    assert self.step_size is None, \
                      f"{self}: step_size not None while start, limit " \
                      f"and bits are not None"
                    max_param = 2**self.bits - 1
                    self.step_size = (self.limit - self.start) / max_param
                else:
                    # self.bits is None, need to set this!
                    if self.step_size is None:
                        self.step_size = 1
                    self.bits = \
                      math.ceil(math.log2((self.limit - self.start) / self.step_size))
            else:
                # self.start is None, need to set this!
                max_param = 2**self.bits - 1
                if self.signed:
                    assert self.step_size is None, \
                      f"{self}: step_size not None while signed, " \
                      f"start is None and limit is not None"
                    self.start = -self.limit
                    self.step_size = max_param / (2*self.limit)
                else:
                    if self.step_size is None:
                        self.step_size = 1
                    self.start = self.limit - self.step_size * max_param
        else: # self.limit is None
            # self.limit is None, need to set this!
            if self.start is not None:
                assert self.bits is not None, \
                  f"{self}: bits is None while start is not None, limit is None"
                max_param = 2**self.bits - 1
                if self.signed:
                    assert self.step_size is None, \
                      f"{self}: step_size is None while start is not None, " \
                      f"limit is None and signed"
                    assert self.start < 0, \
                      f"{self}: start >= 0 while limit is None and signed"
                    self.limit = -self.start
                    self.step_size = max_param / (2*self.limit)
                else:
                    if self.step_size is None:
                        self.step_size = 1
                    self.limit = self.start + max_param * self.step_size
            else:
                # self.start is None, need to set this!
                assert self.bits is not None, \
                  f"{self}: bits is None while start and limit are None"
                max_param = 2**self.bits - 1
                if self.step_size is None:
                    self.step_size = 1
                if self.signed:
                    self.limit = max_param/2 * self.step_size
                    self.start = -self.limit
                else:
                    self.start = 0
                    self.limit = max_param * self.step_size
        assert self.start is not None
        assert self.limit is not None
        assert self.bits is not None
        assert self.step_size is not None


class Geom_param(Parameter):
    required = frozenset("bits,limit,progression".split(','))

    kw_args = dict(b=0,
                   start=0,
                   units=None,
                   kills_value=(),     # () can't be a legal value

                   default=(),         # () can't be a legal value
                   to_python=None,     # ignore, as_index_in: List, Hz_to_cents, sub1,
                                       # to_cents, add_closest_semitone, as_percent_of: N
                   python_arg_name=None,
              )

    def init(self):
        # value = e**(m*param + b) + c
        assert self.progression == "geom"
        self.m, self.c = calc_geom(self.bits, self.limit, self.b, self.start)


class Constant_param(Parameter):
    is_constant = True
    required = frozenset("bits,value".split(','))

    def init(self):
        pass


class Choices_param(Parameter):
    required = frozenset("choices".split(','))

    kw_args = dict(
                   units=None,
                   default=(),         # () can't be a legal value
                   null_value=None,
                   kills_value=(),     # () can't be a legal value
                   to_python=None,     # ignore, as_index_in: List, Hz_to_cents, sub1,
                                       # to_cents, add_closest_semitone, as_percent_of: N
                   python_arg_name=None,
              )

    def init(self):
        self.start = 0
        self.bits = math.ceil(math.log2(len(self.choices)))
        if self.null_value is None:
            self.limit = len(self.choices) - 1
        else:
            assert self.null_value <= (2**self.bits - 1)
            self.limit = max(len(self.choices) - 1, self.null_value)


Param_select = (
    (frozenset(["progression"]), Geom_param),
    (frozenset(["value"]), Constant_param),
    (frozenset(["choices"]), Choices_param),
    (frozenset("bits,start,signed,steps_per_unit,range,step_size".split(',')), Lin_param),
)

def get_settings(filename="synth_settings.yaml"):
    synth_settings = read_yaml(filename)
    top_level = parse_things(synth_settings)
    assert len(top_level) == 1
    synth = top_level["Synth"]
    return synth

def parse_things(things, location=None):
    return {name: Thing(name, location, **arguments)
            for name, arguments in things.items()}

def parse_settings(settings, location, thing, group=None):
    return {name: parse_setting(name, location, thing, group, arguments)
            for name, arguments in settings.items()}

def parse_setting(name, location, thing, group, arguments):
    if not isinstance(arguments, dict):
        raise TypeError(f"{location}={name}, arguments must be mapping, not "
                          f"{arguments.__class__.__name__}")
    for key in Setting.kw_args.keys():
        if key in arguments:
            return Setting(name, location, thing, group, **arguments)
    return Group(name, location, thing, group, **arguments)

def parse_parameters(parameters, location, setting):
    return tuple(parse_parameter(p, location, setting) for p in parameters)

def parse_parameter(parameter, location, setting):
    assert len(parameter) == 1
    for name, arguments in parameter.items():
        for key in arguments.keys():
            assert isinstance(key, str), f"{location=}, got argument {key=!r}"
        for triggers, param_class in Param_select:
            if not triggers.isdisjoint(arguments.keys()):
                return param_class(name, location, setting, **arguments)
        raise AssertionError(f"{location=} No param class meets: {tuple(arguments.keys())}")



if __name__ == "__main__":
    Synth = get_settings()
    Synth.dump()
    print()
    print("Number of Settings", len(Settings))
    print("Number of Parameters", sum(len(s.parameters) for s in Settings))
