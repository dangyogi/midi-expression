# var_attr_test.py


from synth.channel import Var, Notify_actors


class foo(Var):
    x = Notify_actors()


f = foo()
f.name = "foo_name"
print("before", f.var_attrs)
#print("f.x", f.x)
f.x = 7
print("after", f.var_attrs)
print("f.x", f.x)
#print("__get__", f.x.__get__())  # doesn't work, but don't care...
f.x = 4
print("after setting to 4, f.x", f.x, "var_attrs", f.var_attrs)
f.x += 1
print("after += 1, f.x", f.x, "var_attrs", f.var_attrs)
del f.x
print("f.x deleted, var_attrs", f.var_attrs)
f.x = 2
print("after setting to 2, f.x", f.x, "var_attrs", f.var_attrs)
#print("f.x", f.x)
del f.x
