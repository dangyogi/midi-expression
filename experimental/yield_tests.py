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
        self.iter = iter(self.start2(start))
        return self.iter

    def start2(self, start):
        next = start
        for iter in self.iters:
            next = yield from iter.start(next)
        print("Linked_chain returning", next)
        return next

    def abort(self):
        try:
            return self.iter.throw(AbortException)
        except StopIteration as exc:
            print("abort: caught StopIteration")
            return exc.value


class foo:
    def __init__(self, n):
        self.n = n

    def start(self, start=0):
        try:
            for x in range(start, start + self.n):
                yield x
        except AbortException:
            print("foo caught AbortException")
        print("foo returning", x)
        return x

lc = Linked_chain(foo(5), foo(3))
iter = lc.start(44)

for i in range(3):
    print(i, next(iter))

try:
    print("lc.abort()", lc.abort())
    #print("close", iter.close())                  # close doesn't work!  top never catches
                                                  # anything.
    #print("throw", iter.throw(AbortException))    # throw doesn't return when on last iter
                                                  # in Linked_chain, in that case:
                                                  #  - foo converts AbortException to
                                                  #    StopIteration with the final value.
                                                  #  - Linked_chain terminates normally (no
                                                  #    exception caught), throwing
                                                  #    StopIteration with the final value.
                                                  #  - top catches StopIteration with the
                                                  #    final value.
                                                  # When throw is not on the last iter in
                                                  # Linked_chain:
                                                  #  - foo catches AbortException
                                                  #  - foo ignores that and raises
                                                  #    StopIteration with the final value.
                                                  #  - throw returns the final value.
                                                  #  - sometime later, Linked_chain catches
                                                  #    GeneratorExit, ignores it, and
                                                  #    raises StopIteration with the final
                                                  #    value, which is not seen by top.
                                                  #  - no exception is caught at the top
                                                  #    level.
    #print("throw", iter.throw(GeneratorExit))     # doesn't work, causes wrong last value
                                                  # in final StopIteration received by top
except StopIteration as exc:
    print("top caught StopIteration", str(exc))
except AbortException:
    print("top caught AbortException")
except BaseException as exc:
    print("top caught BaseException", type(exc), str(exc))
print("done")
