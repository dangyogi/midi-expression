# descriptors.py


class Cross_setter:
    def __init__(self, fn):
        self.fn = fn
    
    def __get__(self, instance, owner=None):
        print("Cross_setter.__get__", self.name)
        return getattr(instance, self.name)

    def __set__(self, instance, value):
        print("Cross_setter.__set__", self.name, value)
        setattr(instance, self.name, value)
        self.fn(instance, value)

    def __delete__(self, instance):
        print("Cross_setter.__delete__", self.name)

    def __set_name__(self, owner, name):
        print(f"Cross_setter.__set_name__", name)
        self.name = '_' + name


class Notify:
    def __get__(self, instance, owner=None):
        print("Notify.__get__", self.name)
        return getattr(instance, self.name)

    def __set__(self, instance, value):
        print("Notify.__set__", self.name, value)
        setattr(instance, self.name, value)

    def __delete__(self, instance):
        print("Notify.__delete__", self.name)

    def __set_name__(self, owner, name):
        print("Notify.__set_name__", name)
        self.name = '_' + name


class foo:
    bar = Notify()

    @Cross_setter
    def _bar(self, value):
        print("foo._bar called with", value)


f = foo()
f.bar = 27
