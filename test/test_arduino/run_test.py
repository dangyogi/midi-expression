# run_test.py

import sys
import os.path
import subprocess
from yaml import safe_load


def read_yaml(filename):
    if not filename.endswith(".yaml"):
        filename += '.yaml'
    with open(filename, "r") as yaml_file:
        return safe_load(yaml_file)

def generate(program, specs):
    for spec in specs:
        generate_spec(spec)

def run(filename):
    test = read_yaml(filename)
    generate_test_file(test)
    program = test['program']
    compile(program)

def generate_test_file(test):
    program = test['program']
    generate(program, test['generate'])
    with open("test.cpp", "wt") as source_file:
        copy_files('.', program['files_start'], source_file)
        copy_files(program['dir'], program['files'], source_file)
        copy_files('.', program['files_end'], source_file)

def copy_files(source_dir, files, dest_file):
    for file in files:
        copy_file(os.path.join(source_dir, file), dest_file)

def copy_file(filename, dest_file):
    with open(filename, 'rt') as infile:
        print(f'#line 1 "{os.path.basename(filename)}"', file=dest_file)
        for line in infile:
            dest_file.write(line)

def compile(program):
    command = program['compile'].format(**program)
    exit_status = subprocess.run(command.split()).returncode
    if exit_status:
        print("Compile Errors", file=sys.stderr)
        exit(1);



if __name__ == "__main__":
    run(sys.argv[1])
