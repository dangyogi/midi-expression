# cross_setter_test.py


class Cross_setter:
    def __init__(self, set_fn):
        self.set_fn = set_fn
    
    def __get__(self, instance, owner=None):
        return getattr(instance, self.name)
                  
    def __set__(self, instance, value):
        if not hasattr(instance, self.name) or getattr(instance, self.name) != value:
            setattr(instance, self.name, value)
            self.set_fn(instance, value)

    def __delete__(self, instance):
        raise AssertionError(f"del not allowed on {self.name}")
    
    def __set_name__(self, owner, name):
        print("Cross_setter.__set_name__", owner, name)
        self.name = '_' + name


class foo:
    @Cross_setter
    def scale_3(self, value):
        self.a = value / 10
        self.b = value * 10


f = foo()
f.scale_3 = 12
print(f"{f.scale_3=}, {f.a=}, {f.b=}")

del f.scale_3
