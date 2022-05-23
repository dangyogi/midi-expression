# yield_tests.py


class AbortException(Exception):
    pass


class Linked_chain:
    r'''Like itertools.chain, but passes the last value returned from one iter to the next.

    Expects each iter to have a start method to accept the last value of the preceeding
    iter.  This a generator method that replaces the __iter__ method.
    '''
    def __init__(self, *iters):
        self.iters = iters

    def start(self, start):
        return Linked_chain_gen(self, start)


class Linked_chain_gen:
    def __init__(self, lc, start):
        self.lc = lc
        self.start = start

    def __iter__(self):
        next = self.start
        for iter in self.lc.iters:
            self.current_iter = iter.start(next)
            yield from self.current_iter
            next = self.current_iter.next_value()

    def next_value(self):
        return self.current_iter.next_value()


class foo:
    def __init__(self, n):
        self.n = n

    def start(self, start=0):
        return foo_gen(self, start)


class foo_gen:
    def __init__(self, foo, start):
        self.foo = foo
        self.start = start

    def __iter__(self):
        for x in range(self.start, self.start + self.foo.n):
            self.next = x + 1  # This has to come before the yield statement!
            yield x

    def next_value(self):
        return self.next


lc = Linked_chain(foo(5), foo(3))
lc_gen = lc.start(44)
lc_iter = iter(lc_gen)

for i in range(5):
    print(i, next(lc_iter, "done"))

print("done", lc_gen.next_value())

