# channel.py

from operator import attrgetter


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
    recalc_order = 0  # to make Actor.__init__ easier...

    def __init__(self, name=None, **init_values):
        self.name = name
        self.actors = set()   # {Actor}
        for name, value in init_values.items():
            self.set(name, value)

    def __str__(self):
        return f"{self.__class__.__name__}(<{self.name}>)"

    def register(self, actor):
        assert actor not in self.actors
        self.actors.add(actor)

    def deregister(self, actor):
        assert actor in self.actors
        self.actors.remove(actor)

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
        Actors_to_recalc.update(self.actors)


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
# These are all recalc-ed after each MIDI events and before generating the next sound block.
Actors_to_recalc = set()

class Actor(Var):
    r'''Actor acts on Var values.  These are its inputs.
    '''
    def __init__(self, *inputs, name=None):
        super().__init__(name)
        self.inputs = inputs
        if not self.inputs:
            self.recalc_order = 1
        else:
            self.recalc_order = max(self.inputs, key=attrgetter('recalc_order')) \
                                  .recalc_order + 1
            Actors_to_recalc.add(self)
            for i in self.inputs:
                i.register(self)

    def delete(self):
        if self.actors:
            raise AssertionError(f"{self} deleted before dependant actors {self.actors}")
        for i in self.inputs:
            i.deregister(self)


class Block_generator(Actor):
    def recalc(self):
        pass

    def __iter__(self):
        pass


r'''FIX: delete
def tsort_actors():
    # Step 1: What Actors does each Actor depend on (inputs)?
    predecessor_successors = defaultdict(set)  # {predecessor: {successor}}
    for a in Actors:
        for i in a.inputs:
            if isinstance(i, Actor):
                predecessor_successors[i].add(a)

    # Step 2: Figure out predecessor -> successor links for tsort:
    successor_predecessors = defaultdict(set)     # {successor: {predecessor}}
    for predecessor, successors in predecessor_successors.items():
        for successor in successors:
            successor_predecessors[successor].add(predecessor)

    # Step 3: Do tsort.
    remaining_actors = Actors.copy()
    recalc_order = 1
    while remaining_actors:
        next_available_actors = remaining_actors - successor_predecessors.keys()
        if not next_available_actors:
            raise AssertionError(f"tsort_actors: cycle detected {successor_predecessors=}")
        for a in next_available_actors:
            a.recalc_order = recalc_order
            for successor in downstream_links[a]:
                successor_predecessors[successor].remove(a)
                if not successor_predecessors[successor]:
                    del successor_predecessors[successor]
            remaining_actors.remove(a)
        recalc_order += 1
'''


def recalc_actors():
    while Actors_to_recalc:
        next_actor = min(Actors_to_recalc, key=attrgetter('recalc_order'))
        next_actor.recalc()
        Actors_to_recalc.remove(next_actor)
