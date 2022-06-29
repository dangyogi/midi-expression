# class_var_updates.py


class foo:
    a = 3
    b = None
    def bar(self):
        self.a += 1
        self.b = ['bar', 1]

f = foo()
f.bar()
print(f"{f.a=}, {foo.a=}")
print(f"{f.b=}, {foo.b=}")
