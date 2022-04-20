# channel.py


class Channel:
    Settings = []
    def __init__(self, channel_number):
        self.channel_number = channel_number
        self.config_settings = {}   # {MIDI_config_number: Setting}
        self.vars = {}              # {name: var}
        for s in self.Settings:
            s().set_channel(self)

    def register_setting(self, setting):
        assert setting.midi_config_number not in self.config_settings
        self.config_settings[setting.midi_config_number] = setting

    def deregister_setting(self, setting):
        assert setting.midi_config_number in self.config_settings
        del self.config_settings[setting.midi_config_number]

    def register_variable(self, var):
        assert var.name not in self.vars
        self.vars[var.name] = var

    def deregister_variable(self, var):
        assert var.name in self.vars
        del self.vars[var.name]

    def get_var(self, name):
        self.vars[name]


class Var:
    r'''These can hold multiple values.

    Set the values with var.set(name, value).

    Access the values with var.name.

    If any of the values change, all interested Actors are notified.
    '''
    def __init__(self, name=None, **init_values):
        self.name = name
        self.actors = {}   # {Actor: Actor}
        for name, value in init_values.items():
            self.set(name, value)

    def register(self, actor):
        assert actor not in self.actors
        self.actors[actor] = actor

    def deregister(self, actor):
        assert actor in self.actors
        del self.actors[actor]

    def set(self, name, value):
        if not hasattr(self, name) or getattr(self, name) != value:
            setattr(self, name, value)
            self.changed()

    def inc(self, name, amount=1):
        if amount != 0:
            if not hasattr(self, name):
                setattr(self, name, amount)
            else:
                setattr(self, name, getattr(self, name) + amount)
            #print(f"{id(self)=}, {self.name}.inc {name} is now {getattr(self, name)}, "
            #      f"{self.value=}")
            self.changed()

    def dec(self, name, amount=1, no_negative=True):
        if amount != 0:
            assert hasattr(self, name)
            new_value = getattr(self, name) - amount
            assert not no_negative or new_value >= 0
            setattr(self, name, new_value)
            #print(f"{id(self)=}, {self.name}.dec {name} is now {getattr(self, name)}, "
            #      f"{self.value=}")
            self.changed()

    def changed(self):
        for a in self.actors:
            a.tickle()


class Setting(Var):
    r'''These are the config values to be changed (thru MIDI).
    '''
    def __init__(self, name=None):
        super().__init__(name)
        self.channel = None
        self.config_value = None

    def set_channel(self, channel):
        assert self.channel is None
        self.channel = channel
        self.channel.register_setting(self)

    def delete(self):
        if self.channel is not None:
            self.channel.deregister_setting(self)

    def set_config_value(self, midi_value):
        if midi_value != self.config_value:
            self.config_value = midi_value
            self.set_midi_value(midi_value)

    def set_midi_value(self, midi_value):
        r'''Only called (by set_config_value) if midi_value changed.

        Override in derived classes...
        '''
        self.set('value', midi_value)


class Channel_setting(Setting):
    def set_channel(self, channel):
        super().set_channel(channel)
        self.channel.register_variable(self)

    def delete(self):
        self.channel.deregister_variable(self)
        super().delete()


# Program wide (all channels)
#
# These are all prep-ed after catching up on MIDI events and before generating the
# next sound block.
Actors = {}   # {Actor: Actor}

class Actor(Var):
    def __init__(self, *ins, name=None):
        super().__init__(name)
        self.ins = ins
        self.needs_recalc = False
        Actors[self] = self
        for i in self.ins:
            i.register(self)

    def delete(self):
        del Actors[self]
        for i in self.ins:
            i.deregister(self)

    def tickle(self):
        self.needs_recalc = True

    def prep(self):
        if self.needs_recalc:
            self.needs_recalc = False
            self.recalc()


class Block_generator(Actor):
    def recalc(self):
        pass

    def __iter__(self):
        pass


def process_events():
    for actor in Actors.values():
        actor.prep()
