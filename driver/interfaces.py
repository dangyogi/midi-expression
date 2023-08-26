# interfaces.py

import os.path
import utils

EXP_CONSOLE_DIR = os.path.join(utils.PROJECT_DIR, "exp_console")

Controllers = ("Master", "Pot", "LED")


def read_interfaces():
    utils.Controllers = {attr.name: attr for attr in map(read_interface, Controllers)}
    utils.Controllers1 = {attr.name[0]: attr for attr in utils.Controllers.values()}

def read_interface(name):
    r'''The Attrs returned has: name, i2c_addr, requests, reports, and errnos.
    '''
    with open(mk_path(name), "rt") as f:
        ans = utils.Attrs()
        ans.name = name
        for line in f:
            line = line.strip()
            if not line: continue
            if line.lower() == f"{name.lower()} controller":
                #print("found name line")
                pass
            elif line == "I2C interface:":
                #print("found I2C interface")
                read_i2c_interface(f, ans)
            elif line == "Error Codes:":
                #print("found Error Codes")
                ans.errnos = read_errnos(f)
                return ans
            else:
                print(f"read_interface: UNKNOWN LINE: {line}")
    return ans

def mk_path(name):
    path = os.path.join(EXP_CONSOLE_DIR, f"sketch_{name.lower()}", "INTERFACE")
    if os.path.exists(path):
        return path
    return os.path.join(EXP_CONSOLE_DIR, f"sketch_{name.lower()}s", "INTERFACE")

def read_i2c_interface(f, attrs):
    ans = {}
    for line in f:
        line = line.strip()
        if not line: continue
        if line.startswith("I2C Addr:"):
            elements = line[9:].split()
            #print("found I2C Addr", elements)
            attrs.i2c_addr = int(elements[0], base=16)
        elif line == "Requests from on high:":
            #print("found Requests from on high")
            attrs.requests = read_list(f)
        elif line == "Reports to on high:":
            #print("found Reports to on high")
            attrs.reports = read_list(f)
            return
        else:
            print(f"read_i2c_interface: UNKNOWN LINE: {line}")

def read_list(f):
    ans = []
    for line in f:
        indent = indent_level(line)
        line = line.strip()
        if not line:
            if not ans: continue
            return ans
        if indent == 4:
            ans.append(line)
        elif indent >= 6:
            assert ans, f"read_list: first line indented: {line}"
            ans[-1] = ' '.join((ans[-1], line))
        else:
            raise AssertionError(f"read_list: unexpected indent level, {indent}")
    return ans

def read_errnos(f):
    ans = {}
    for line in f:
        line = line.strip()
        if not line: continue
        if line == "Errno, Err_data":
            #print("found Errno, Err_data list")
            return read_errno_list(f)
    raise AttributeError('read_errnos: did not find "Errno, Err_data" line')

def read_errno_list(f):
    ans = {}
    last_errno = None
    for line in f:
        line = line.rstrip()
        if not line.lstrip():
            if not ans: continue
            break
        indent = indent_level(line)
        if 2 <= indent <= 5:
            if last_errno is not None:
                ans[last_errno.errno] = last_errno
            last_errno = utils.Attrs()
            errno, last_errno.err_data, last_errno.desc = line.split(maxsplit=2)
            last_errno.errno = int(errno)
        elif indent >= 6:
            assert last_errno is not None, f"read_errno_list: first line indented: {line}"
            last_errno.desc = ' '.join((last_errno.desc, line.lstrip()))
        else:
            raise AssertionError(f"read_errno_list: unexpected indent level, {indent}")
    if last_errno is not None:
        ans[last_errno.errno] = last_errno
    return ans

def indent_level(line):
    return len(line) - len(line.lstrip())



if __name__ == "__main__":
    read_interfaces()
    print("names", tuple(utils.Controllers.keys()))
    print("names1", tuple(utils.Controllers1.keys()))
    print()

    for controller in utils.Controllers.values():
        #print(mk_path(controller.name))
        print(controller.name)
        if hasattr(controller, 'i2c_addr'):
            print("i2c_addr:", hex(controller.i2c_addr))
        if hasattr(controller, 'requests'):
            print("requests:")
            for request in controller.requests:
                print(' ', request)
            print()
        if hasattr(controller, 'reports'):
            print("reports:")
            for report in controller.reports:
                print(' ', report)
            print()
        if hasattr(controller, 'errnos'):
            print("errnos:")
            for errno in controller.errnos.values():
                print(' ', errno.errno, errno.err_data, errno.desc)
        print()
