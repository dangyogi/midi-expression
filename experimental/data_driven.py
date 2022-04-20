# data_flow.py


class Var:
    r'''Each Var is only set by one Node.
    '''
    def __init__(self, initial_value=None):
        self.value = initial_value
        self.inputs = set()
        self.outputs = set()

    def set(self, value):
        if value != self.value:
            self.value = value

    def get(self):
        return self.value

    def get_by(self, *nodes):
        self.inputs.update(nodes)

    def set_by(self, *outputs):
        self.outputs.update(outputs)


class Fn_node:
    r'''Node owns its outputs, meaning that no other node shares them with this Node.
    '''
    def __init__(self, outputs, fn, *args, **kwargs):
        self.outputs = outputs
        self.fn = fn
        self.args = args
        self.kwargs = kwargs

    def recalc(self):
        ans = self.fn.get()(*(a.get() for a in self.args),
                            **{k: v.get() for k, v in self.kwargs})
        self.outputs.set(ans)


class Outputs:
    def __init__(self, vars):
        self.vars = vars

    def set(self, value):
        if isinstance(self.vars, Var):
            self.vars.set(value)
        else:
            assert len(self.vars) == len(value)
            for var, v in zip(self.vars, value):
                var.set(v)

    def downstream(self):
        if isinstance(self.vars, Var):
            yield self.vars.downstream()
        else:
            for v in self.vars:
                yield v.downstream()
