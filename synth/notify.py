# notify.py

from operator import attrgetter



class Notify_actors:
    r'''Identifies an attribute that should trigger the Actor.recalc calls.

    Can only be used in classes derived from Var.

    This is a class property/descriptor that is used as follows:

        class foo(Var):
            ...
            x = Notify_actors()
            y = Notify_actors()
    '''
    def __get__(self, instance, owner=None):
        try:
            return instance.var_attrs[self.name]
        except KeyError:
            raise AttributeError(f"{instance.name} has no attribute {self.name!r}") \
                  from None

    def __set__(self, instance, value):
        if self.name not in instance.var_attrs or instance.var_attrs[self.name] != value:
            instance.var_attrs[self.name] = value
            instance.changed()

    def __delete__(self, instance):
        try:
            del instance.var_attrs[self.name]
        except KeyError:
            raise AttributeError(f"{instance.name} has no attribute {self.name!r}") \
                  from None

    def __set_name__(self, owner, name):
        self.name = name


Num_vars = 0

class Var:
    r'''These can trigger on multiple attributes.

    If any of these attributes change (or are del-ed), all registered Actors are notified.

    Identify the attributes to monitor for changes by declaring them at the class level
    with Notify_actors() as follows:

        class foo(Var):
            ...
            x = Notify_actors()
            y = Notify_actors()
    '''
    recalc_order = 0  # to make Actor.__init__ easier...

    def __init__(self, name=None):
        global Num_vars
        self.name = name
        self.actors = set()   # {Actor}
        self.var_attrs = {}
        Num_vars += 1

    def __str__(self):
        return f"{self.__class__.__name__}(<{self.name}>)"

    def register(self, actor):
        assert actor not in self.actors
        self.actors.add(actor)

    def deregister(self, actor):
        assert actor in self.actors
        self.actors.remove(actor)

    def changed(self):
        Actors_to_recalc.update(self.actors)


# Program wide (all channels)
#
# These are all recalc-ed after each MIDI events and before generating the next sound block.
Actors_to_recalc = set()

Num_actors = 0
Num_actors_deleted = 0
Max_num_actors = 0

class Actor(Var):
    r'''Actor acts on Var values.  These are its inputs.
    '''
    def __init__(self, *inputs, name=None):
        global Num_actors, Max_num_actors
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
        Num_actors += 1
        if Num_actors > Max_num_actors:
            Max_num_actors = Num_actors

    def delete(self):
        global Num_vars, Num_actors, Num_actors_deleted
        if self.actors:
            raise AssertionError(f"{self} deleted before dependant actors {self.actors}")
        for i in self.inputs:
            i.deregister(self)
        Num_vars -= 1
        Num_actors -= 1
        Num_actors_deleted += 1


Max_starting_num_actors = 0
Max_num_actors_recalced = 0
Max_num_actors_added = 0

def recalc_actors():
    global Max_starting_num_actors, Max_num_actors_recalced, Max_num_actors_added
    starting_num_actors = len(Actors_to_recalc)
    if starting_num_actors > Max_starting_num_actors:
        Max_starting_num_actors = starting_num_actors
    num_actors_recalced = 0
    while Actors_to_recalc:
        next_actor = min(Actors_to_recalc, key=attrgetter('recalc_order'))
        next_actor.recalc()
        num_actors_recalced += 1
        Actors_to_recalc.remove(next_actor)
    if num_actors_recalced > Max_num_actors_recalced:
        Max_num_actors_recalced = num_actors_recalced
    if num_actors_recalced - starting_num_actors > Max_num_actors_added:
        Max_num_actors_added = num_actors_recalced - starting_num_actors


def report():
    print(f"channel: {Num_vars=}, {Num_actors=}, {Num_actors_deleted=}, {Max_num_actors=}")
    print(f"channel: {Max_starting_num_actors=}, {Max_num_actors_recalced=}, "
          f"{Max_num_actors_added=}")
