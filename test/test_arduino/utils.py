# utils.py

from yaml import safe_load

__all__ = ('read_yaml', 'unsigned', 'double_float', 'integer')


def read_yaml(filename):
    if not filename.endswith(".yaml"):
        filename += '.yaml'
    with open(filename, "r") as yaml_file:
        return safe_load(yaml_file)

def unsigned(type):
    return type[0] == 'u' or type == 'byte' or type == 'size_t'

def double_float(type):
    return 'float' in type or 'double' in type

def integer(type):
    # was: 'byte' in ptype or 'short' in ptype or 'int' in ptype or 'long' in ptype
    return not type.endswith('*') and not double_float(type)

