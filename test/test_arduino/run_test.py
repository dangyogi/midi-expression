# run_test.py

import sys
import re
import os, os.path
import shutil
import subprocess

from utils import *


def run(sketch_dir, no_compile, gen):
    global Source_dir
    Source_dir = os.path.join(sketch_dir, 'tmp')
    test = read_yaml(os.path.join(sketch_dir, 'test_compile.yaml'))
    program = test['program']
    if gen:
        shutil.rmtree(Source_dir)
        os.mkdir(Source_dir)
        generate_test_file(test, program, sketch_dir)
    if no_compile:
        print("test.cpp generated")
    else:
        compile(program)

def generate_test_file(test, program, sketch_dir):
    with open(os.path.join(Source_dir, "test.cpp"), "wt") as source_file:
        copy_files('.', program['files_start'], source_file)

        print(file=source_file)
        print("void send_sketch_dir(void) {", file=source_file)
        print(fr'  sendf("sketch_dir {sketch_dir}\n");', file=source_file)
        print("}", file=source_file)
        print(file=source_file)

        arch_defs = read_yaml(program['arch_defs'])
        arch_called_stubs_file, arch_caller_stubs_file, arch_regex = \
          gen_functions(arch_defs['functions'], "arch_")
        assert arch_regex is None
        assert arch_caller_stubs_file is None
        copy_file(arch_called_stubs_file, source_file)

        called_stubs_file, caller_stubs_file, regex = gen_functions(test['functions'])
        copy_file(called_stubs_file, source_file)

        copy_files(program['dir'], program['files'], source_file, regex)

        copy_file(gen_defines(arch_defs['defines'], "arch_"), source_file)
        copy_file(gen_defines(test['defines']), source_file)
        copy_file(gen_sub_classes(arch_defs['classes'], "arch_"), source_file)
        copy_file(gen_sub_classes(test['classes']), source_file)
        copy_file(gen_fields(arch_defs['structs'], "arch_"), source_file)
        copy_file(gen_fields(test['structs']), source_file)
        copy_file(gen_globals(arch_defs['globals'], "arch_"), source_file)
        copy_file(gen_globals(test['globals']), source_file)
        copy_file(gen_arrays(arch_defs['arrays'], "arch_"), source_file)
        copy_file(gen_arrays(test['arrays']), source_file)

        copy_file(caller_stubs_file, source_file)
        all_funs = arch_defs['functions'].copy()
        all_funs.update(test['functions'])
        copy_file(gen_send_functions(all_funs), source_file)

        copy_files('.', program['files_end'], source_file)

def gen_defines(defines, prefix=""):
    defines_file = os.path.join(Source_dir, prefix + "defines.cpp")
    with open(defines_file, "wt") as source_file:
        print(f'// {prefix}defines.cpp', file=source_file)
        print(file=source_file)
        print(f'void {prefix}send_defines(void) {{', file=source_file)
        for name in defines:
            print(fr'  sendf("#define {name} %d\n", (int)({name}));', file=source_file)
        print('}', file=source_file)
        print(file=source_file)
    return defines_file

def gen_sub_classes(classes, prefix=""):
    subclasses_file = os.path.join(Source_dir, prefix + "subclasses.cpp")
    with open(subclasses_file, "wt") as source_file:
        print(f'// {prefix}subclasses.cpp', file=source_file)
        print(file=source_file)
        print(f'void {prefix}send_classes(void) {{', file=source_file)
        for name, sub_classes in classes.items():
            print(fr'  sendf("sub_classes {name} {" ".join(sub_classes)}\n");',
                  file=source_file)
        print('}', file=source_file)
        print(file=source_file)
    return subclasses_file

def gen_fields(structs, prefix=""):
    fields_file = os.path.join(Source_dir, prefix + "fields.cpp")
    with open(fields_file, "wt") as source_file:
        print(f'// {prefix}fields.cpp', file=source_file)
        print(file=source_file)
        print(f'void {prefix}send_structs(void) {{', file=source_file)
        for name, fields in structs.items():
            gen_fields1(name, fields, structs, source_file)
        print('}', file=source_file)
        print(file=source_file)
    return fields_file

def gen_fields1(name, fields, structs, source_file):
    for field in fields:
        assert len(field) == 1
        field_name, type = field.copy().popitem()
        if field_name == 'base_class':
            gen_fields1(name, structs[type], structs, source_file)
        elif isinstance(type, dict):
            element_type = type['element_type']
            dims = type['dims']
            if not isinstance(dims, (list, tuple)):
                dims = [dims]
            type_format = f"{element_type}[{', '.join(['%d'] * len(dims))}]"
            type_str = f"{element_type}[{', '.join([str(dim) for dim in dims])}]"
            dim_exprs = ', '.join(f"(int)({dim})" for dim in dims)
            print(fr'  sendf("field {name} {field_name} %d %d {type_format}\n",',
                  file=source_file)
            print(f'    (int)offsetof({name}, {field_name}), (int)sizeof({type_str}), {dim_exprs});',
                  file=source_file)
        else:
            print(fr'  sendf("field {name} {field_name} %d %d {type}\n",',
                  file=source_file)
            print(f'    (int)offsetof({name}, {field_name}), (int)sizeof({type}));',
                  file=source_file)

def gen_globals(globals, prefix=""):
    globals_file = os.path.join(Source_dir, prefix + "globals.cpp")
    with open(globals_file, "wt") as source_file:
        print(f'// {prefix}globals.cpp', file=source_file)
        print(file=source_file)
        print(f'void {prefix}send_globals(void) {{', file=source_file)
        for name, type in globals.items():
            print(fr'  sendf("global {name} %u %d {type}\n",',
                  file=source_file)
            print(f'    (unsigned int)&{name}, (int)sizeof({name}));',
                  file=source_file)
        print('}', file=source_file)
        print(file=source_file)
    return globals_file

def gen_arrays(arrays, prefix=""):
    arrays_file = os.path.join(Source_dir, prefix + "arrays.cpp")
    with open(arrays_file, "wt") as source_file:
        print(f'// {prefix}arrays.cpp', file=source_file)
        print(file=source_file)
        print(f'void {prefix}send_arrays(void) {{', file=source_file)
        for name, info in arrays.items():
            type = info['element_type']
            print(f'  sendf("array {name} %u %d %d {type}:",',
                  file=source_file)
            print(f'    (unsigned int)&{name}, (int)sizeof({name}), (int)sizeof({type}));',
                  file=source_file)
            if 'dims' not in info:
                print(r'  sendf(" %d\n",', file=source_file)
                print(f'    calc_dim((int)sizeof({name}), (int)sizeof({type})));',
                      file=source_file)
            else:
                dims = info['dims']
                if not isinstance(dims, (list, tuple)):
                    dims = [dims]
                for dim in dims:
                    print(f'  sendf(" %d", {dim});', file=source_file)
                print(r'  sendf("\n");', file=source_file)
        print('}', file=source_file)
        print(file=source_file)
    return arrays_file

def gen_functions(functions, prefix=""):
    # The called_stubs are at the front of the generated file.  These are added to the source_file
    # at the point that gen_functions is called.
    # The caller_stubs are added at the end of the source_file followed by an array called
    # "Caller_stub_names" of (name, caller_stub) and a #define for NUM_CALLER_STUBS.
    # The function returns: called_stubs_file, caller_stubs_file, regexp matching names to be doctored
    called_stubs_file = os.path.join(Source_dir, prefix + "called_stubs_file.cpp")
    caller_stubs_file = os.path.join(Source_dir, prefix + "caller_stubs_file.cpp")
    with open(called_stubs_file, "wt") as called_stubs, \
         open(caller_stubs_file, "wt") as caller_stubs:
        print(f'// {prefix}called_stubs_file.cpp', file=called_stubs)
        print(file=called_stubs)
        print(f'// {prefix}caller_stubs_file.cpp', file=caller_stubs)
        print(file=caller_stubs)
        names = set()  # names of generated caller_stubs
        for name, info in functions.items():
            if not info or info.get('gen_caller', True):
                assert name not in names
                names.add(name)
                gen_caller_stub(name, info, caller_stubs)
            gen_called_stub(name, info, called_stubs)
        wrapup_caller_stubs(names, caller_stubs)
    if names:
        regex = re.compile(r'\b(' + '|'.join(names) + r')(?=\()')
    else:
        os.remove(caller_stubs_file)
        caller_stubs_file = None
        regex = None
    return called_stubs_file, caller_stubs_file, regex

def wrapup_caller_stubs(names, caller_stubs):
    # generate Caller_stub_names
    print(f'''
typedef struct {{
  const char *name;
  void (*caller_stub)(const char *args);
}} caller_stub_names_t;

#define NUM_CALLER_STUBS    {len(names)}

caller_stub_names_t Caller_stub_names[] = {{
''', end='', file=caller_stubs)
    for name in sorted(names):
        print(f'  {{"{name}", {name}_caller_stub}},', file=caller_stubs)
    print("};", file=caller_stubs)
    print(file=caller_stubs)

def gen_caller_stub(name, info, source_file):
    print(f'void {name}_caller_stub(const char *args) {{', file=source_file)
    indent = 2
    def gen_code(line='', extra_indent=0):
        if not line:
            print(file=source_file)
        else:
            print(' ' * (indent + extra_indent), line, sep='', file=source_file)
    if info and 'return' in info:
        ret_type = info['return']
        gen_code(f'{ret_type} ret;')
        head = 'ret = '
        if unsigned(ret_type):
            send_return = r'sendf("returned %lu\n", (unsigned long)ret);'
        else:
            send_return = r'sendf("returned %ld\n", (long)ret);'
    else:
        head = ''
        send_return = r'sendf("returned\n");'
    cpp_args = []
    if info and 'params' in info:
        check_len = None
        for i, param in enumerate(info['params'], 1):
            # unpack parameter
            pname, pinfo = param.copy().popitem()
            if not isinstance(pinfo, dict):
                pinfo = dict(type=pinfo)
            ptype = pinfo['type']

            if ptype.endswith('*'):
                if 'len' not in pinfo or 'max_len' not in pinfo:
                    print(f"ERROR gen_caller_stub {name}, {pname}: "
                          f"{type=!r} but missing 'len' or 'max_len'",
                          file=sys.stderr)
                    sys.exit(1)
                gen_code(f'{ptype[:-1]} p{i}[{pinfo["max_len"]}];')
            else:
                gen_code(f'{ptype} p{i};')

            # insert code to check that enough args were passed
            gen_code('if (!args) {')
            if 'default' in pinfo:
                gen_code(f'  p{i} = {pinfo["default"]};')
                gen_code('} else {')
                indent += 2
            else:
                gen_code(
                  fr'  fprintf(stderr, "ERROR on call {name}: param {i} missing in call request\n");')
                gen_code('  exit(2);')
                gen_code('}')

            if ptype.endswith('*'):
                gen_code(f"int data_len = load_data(p{i}, args);")
                check_len = pinfo['len']
            elif integer(ptype):
                if unsigned(ptype):
                    gen_code(f'p{i} = ({ptype})strtoul(args, NULL, 10);')
                else:
                    gen_code(f'p{i} = ({ptype})atol(args);')
                if pname == check_len:
                    gen_code(f'if (p{i} != data_len) {{')
                    gen_code(f'  fprintf(stderr, ')
                    gen_code(f'    "ERROR on call {name}: param {i}=%d does not match data_len=%d",')
                    gen_code(f'    (int)p{i}, data_len);')
                    gen_code(f'  exit(2);')
                    gen_code('}')
            elif double_float(ptype):
                gen_code(f'p{i} = ({ptype})atof(args);')
            else:
                print(f"gen_caller_stub {name}, {pname}: unknown type {ptype!r}", file=sys.stderr)
                exit(1)
            gen_code("args = strchr(args, ' ');")
            if indent > 2:
                indent -= 2
                gen_code('}')
            cpp_args.append(f"p{i}")
    gen_code('if (args) {')
    gen_code(fr'  fprintf(stderr, "ERROR on call {name}: too many arguments in call request\n");')
    gen_code('  exit(2);')
    gen_code('}')
    gen_code(f"{head}{name}_real({', '.join(cpp_args)});")
    gen_code(send_return)
    indent -= 2
    gen_code('}')
    gen_code()

def gen_called_stub(name, info, source_file):
    format = [f"fun_called {name}"]
    args = []
    initial = ''
    def do_send(final=''):
        nonlocal format, args, initial
        complete_format = f'"{initial}{" ".join(format)}{final}"'
        if complete_format:
            args.insert(0, complete_format)
            if len(args) <= 2:
                print(f"  sendf({', '.join(args)});", file=source_file)
            else:
                print(f"  sendf({args[0]},", file=source_file)
                print(f"        {', '.join(args[1:])});", file=source_file)
            format = []
            args = []
            initial = ''
        elif args:
            print(f"INTERNAL ERROR: gen_called_stub {name}, args={args} with no format", file=sys.stderr)
            exit(1)

    if info is None:
        print(f'void {name}(void) {{', file=source_file)
        ret = 'void'

        # declare variable for return value (as a string, received from test driver)
        print(f'  const char *ret_value;', file=source_file)
    else:
        # generate function declaration line
        if 'return' in info:
            ret = info['return']
        else:
            ret = 'void'
        print(f'{ret} {name}(', end='', file=source_file)
        def gen_params():
            if 'params' in info:
                for param in info['params']:
                    pname, pinfo = param.copy().popitem()
                    if not isinstance(pinfo, dict):
                        pinfo = dict(type=pinfo)
                    ptype = pinfo['type']
                    assert isinstance(ptype, str)
                    yield f"{ptype} {pname}"
        print(f'{", ".join(gen_params())}) {{', file=source_file)

        # declare variable for return value (as a string, received from test driver)
        print(f'  const char *ret_value;', file=source_file)

        # send parameters:
        if 'params' in info:
            for i, param in enumerate(info['params'], 1):
                pname, pinfo = param.copy().popitem()
                if not isinstance(pinfo, dict):
                    pinfo = dict(type=pinfo)
                ptype = pinfo['type']
                if ptype[-1] != '*':
                    if unsigned(ptype):
                        format.append('%lu')
                        args.append(f"(unsigned long){pname}")
                    else:
                        format.append('%ld')
                        args.append(f"(long){pname}")
                else:
                    do_send(' ')
                    if pinfo.get('null_terminated', False):
                        print(f"  int str_len{i} = strlen({pname});", file=source_file)
                        print(f"  send_data((const byte *){pname}, str_len{i});", file=source_file)
                    else:
                        print(f"  send_data({pname}, {pinfo['len']});", file=source_file)
                    initial = ' '

    # finish sending "fun_called" line
    initial = ''
    do_send(r'\n')

    print(f"  ret_value = run_to_return();", file=source_file)
    if ret == 'void':
        print('  if (ret_value) {', file=source_file)
        print(f'    fprintf(stderr, "ERROR on call {name} return: '
              r'''unexpected value='%s'\n", ret_value);''',
              file=source_file)
        print('    exit(2);', file=source_file)
        print('  }', file=source_file)
        print(f"  return;", file=source_file)
    else:
        print('  if (!ret_value) {', file=source_file)
        print(fr'    fprintf(stderr, "ERROR on call {name} missing return value\n");',
              file=source_file)
        print('    exit(2);', file=source_file)
        print('  }', file=source_file)
        if integer(ret):
            if unsigned(ret):
                print(f'  return ({ret})strtoul(ret_value, NULL, 10);', file=source_file)
            else:
                print(f'  return ({ret})atol(ret_value);', file=source_file)
        elif double(ret):
            print(f'  return ({ret})atof(ret_value);', file=source_file)

    print('}', file=source_file)
    print(file=source_file)

def gen_send_functions(functions):
    functions_file = os.path.join(Source_dir, "functions.cpp")
    with open(functions_file, "wt") as source_file:
        print('// functions.cpp', file=source_file)
        print(file=source_file)
        print('void send_functions(void) {', file=source_file)
        for name, info in functions.items():
            ret = 0
            pnames = []
            if info:
                if 'return' in info:
                    ret = 1
                for param in info.get('params', ()):
                    pname, pinfo = param.copy().popitem()
                    pnames.append(' ' + pname)
            pnames_str = ''.join(pnames)
            print(fr'  sendf("function {name} {ret}{pnames_str}\n");', file=source_file)
        print('}', file=source_file)
        print(file=source_file)
    return functions_file

def copy_files(source_dir, files, dest_file, regex=None):
    for file in files:
        copy_file(os.path.join(source_dir, file), dest_file, regex)

def copy_file(filename, dest_file, regex=None):
    with open(filename, 'rt') as infile:
        print(f'#line 1 "{os.path.basename(filename)}"', file=dest_file)
        for line in infile:
            if regex is not None and line[0] not in " #/":
                line = regex.sub(r'\1_real', line, 1)
            dest_file.write(line)

def compile(program):
    command = program['compile'].format(source_dir=Source_dir, **program)
    exit_status = subprocess.run(command.split()).returncode
    if exit_status:
        print("Compile Errors", file=sys.stderr)
        exit(1);



if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', action='store_true')
    parser.add_argument('-g', action='store_false')
    parser.add_argument('sketch_dir')
    args = parser.parse_args()

    run(args.sketch_dir, args.c, args.g)
