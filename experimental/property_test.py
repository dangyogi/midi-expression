# property_tests.py


class foo:
    @property().setter
    def bar(self, value):
        print("foo.bar", value)

f = foo()
f.x = 4
f.bar = 5
