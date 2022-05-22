# gen_synth_settings.py

import os

from jinja2 import Environment, PackageLoader, FileSystemLoader

from .parse_settings import get_settings


Synth_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "synth")
#print("Synth_dir", Synth_dir)

jinja2_env = Environment(
    #loader=PackageLoader('midi_expression', 'data'),
    loader=FileSystemLoader('data'),
    autoescape=False,
    trim_blocks=False,    # trims first newline after a block (not tag!) is removed
    lstrip_blocks=False,  # strips leading whitespace from the start of a line to a block
    newline_sequence=os.linesep,
    keep_trailing_newline=False,  # keeps last newline at end of template
)

def render(context):
    with open(os.path.join(Synth_dir, "synth_settings.py"), "wt") as out_file:
        out_file.write(jinja2_env.get_template("synth_settings.py").render(context))

def compile_settings(settings):
    control_fns = []
    NR_param_fns = []
    system_common_key = "command"
    system_common_fns = []
    helpers = []

    return dict(control_fns=control_fns,
                NR_param_fns=NR_param_fns,
                system_common_key=system_common_key,
                system_common_fns=system_common_fns,
                helpers=helpers)


if __name__ == "__main__":
    import argparse

    argparser = argparse.ArgumentParser()


    args = argparser.parse_args()

    synth, settings = get_settings()
    context = compile_settings(settings)
    render(context)
    print("Done!")
