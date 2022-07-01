# gen_synth_settings.py

import os

from jinja2 import Environment, PackageLoader, FileSystemLoader

from . import parse_settings


Global_fns = \
    "Key_signature," \
    "Equal_temperament,Well_temperament,Just_intonation,Meantone,Pythagorean_tuning," \
    "Constant,Sequence,Ramp,Sin".split(',')

Synth_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "synth")
#print("Synth_dir", Synth_dir)

jinja2_env = Environment(
    #loader=PackageLoader('midi_expression', 'data'),
    loader=FileSystemLoader('data'),
    autoescape=False,
    trim_blocks=True,     # trims first newline after a block (not tag!) is removed
    lstrip_blocks=True,   # strips leading whitespace from the start of a line to a block
    newline_sequence=os.linesep,
    keep_trailing_newline=False,  # keeps last newline at end of template
)
jinja2_env.filters['hex'] = lambda s, d=2: f"0x{s:0{d}X}"

def render(context):
    with open(os.path.join(Synth_dir, "synth_settings.py"), "wt") as out_file:
        out_file.write(jinja2_env.get_template("synth_settings.py").render(context))


Control_fns = []               # [([key], name, body)]
NR_param_fns = []              # [([key], name, body)]
System_common_key = "command"  # python expression
System_common_fns = []         # [([key], name, body)]
Helpers = []                   # [line]

def compile_settings(settings):
    for setting in settings:
        for formatter in setting.message_formatters:
            formatter.compile_setting()

    return dict(control_fns=Control_fns,
                NR_param_fns=NR_param_fns,
                system_common_key=System_common_key,
                system_common_fns=System_common_fns,
                helpers=Helpers)


def compile_control_change(self):
    fn_name = f"control_change_0x{self.my_code:02X}"
    body = []
    Control_fns.append((self.keys(), fn_name, body))
    p_names = []
    body.append('kill = False')
    body.append('params = {}')
    has_kill = False
    for p in self.setting.parameters:
        if self.param == p:
            exp = f"(control_number & 0x{(1 << p.bits) - 1:02X})"
        else:
            exp = f"value.pop({p.bits})"
        p.unpack(exp, body)
        if p.kills_value != ():
            has_kill = True
        if not p.is_constant:
            p_names.append(p.python_name)
    body.append(f"obj = {get_obj_exp(self, p_names)}")
    store_value(self, p_names, has_kill, body)


def store_value(self, p_names, has_kill, body):
    attr_name = self.setting.get_attr_name()
    assert len(p_names) > 0
    indent = ''
    if len(p_names) == 1 and not self.setting.is_object:
        if has_kill:
            body.append(f"if kill:")
            body.append(f"    obj.disable()")
            body.append(f"else:")
            indent = '    '
        body.append(f"{indent}obj.{attr_name} = params[{p_names[0]!r}]")
    else:
        if has_kill:
            body.append(f"if kill:")
            body.append(f"    obj.{attr_name} = {self.setting.kill_value!r}")
            body.append(f"else:")
            indent = '    '
        fn_name = self.setting.name
        body.append(f"{indent}nulls = [name for name, value in params.items()")
        body.append(f"{indent}              if value is None]")
        body.append(f"{indent}for name in nulls:")
        body.append(f"{indent}    del params[name]")
        if fn_name in Global_fns:
            body.append(f"{indent}obj.{attr_name} = {fn_name}(**params)")
        else:
            body.append(f"{indent}obj.{attr_name}.{fn_name}(**params)")


def get_obj_exp(self, p_names):
    thing = self.setting.thing
    exp = ''
    while thing:
        identified_by = thing.identified_by
        if identified_by is not None:
            if identified_by == 'channel':
                exp = f".{identified_by}s[{identified_by}]{exp}"
            else:
                assert identified_by in p_names
                exp = f".{identified_by}s[params[{identified_by!r}]]{exp}"
                p_names.remove(identified_by)

        thing = thing.parent
    return f"synth{exp}"

parse_settings.Control_change.compile_setting = compile_control_change


def compile_NR_param(self):
    fn_name = f"NR_param_0x{self.my_code:04X}"
    body = []
    NR_param_fns.append((self.keys(), fn_name, body))
    p_names = []
    body.append('kill = False')
    body.append('params = {}')
    has_kill = False
    for p in self.setting.parameters:
        if self.param == p:
            exp = f"(param_number & 0x{(1 << p.bits) - 1:02X})"
        else:
            exp = f"value.pop({p.bits})"
        p.unpack(exp, body)
        if p.kills_value != ():
            has_kill = True
        if not p.is_constant:
            p_names.append(p.python_name)
    body.append(f"obj = {get_obj_exp(self, p_names)}")
    store_value(self, p_names, has_kill, body)

parse_settings.Non_registered_parameter.compile_setting = compile_NR_param


def compile_system_common(self):
    global System_common_key
    fn_name = f"system_common_0x{self.my_code:02X}"
    body = []
    System_common_fns.append((self.keys(), fn_name, body))
    if self.get_Length():
        System_common_key = f"(command, value.peek({self.get_Length()}))"
    p_names = []
    body.append('kill = False')
    body.append('params = {}')
    has_kill = False
    for p in self.setting.parameters:
        p.unpack(f"value.pop({p.bits})", body)
        if p.kills_value != ():
            has_kill = True
        if not p.is_constant:
            p_names.append(p.python_name)
    body.append(f"obj = synth")
    store_value(self, p_names, has_kill, body)

parse_settings.System_common.compile_setting = compile_system_common



if __name__ == "__main__":
    import argparse

    argparser = argparse.ArgumentParser()


    args = argparser.parse_args()

    synth, settings = parse_settings.get_settings()
    context = compile_settings(settings)
    render(context)
    print("Done!")
