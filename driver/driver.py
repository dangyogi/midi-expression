# driver.py

import sys
import time
import select
import glob
import termios
import io
import os

from interfaces import read_interfaces
import master_requests
import utils


def get_serial_devices():
    return glob.glob("/dev/ttyACM*")

def read_master():
    #print("read_master", utils.Master_in.name)
    while True:
        next = utils.Master_in.peek(1)
        #print("peek got", repr(next))
        if not next: break
        if next[0] == b'$'[0]:
            # next == b'$'
            # Request from master to driver, letter after '$' is command.
            command = utils.to_str(utils.Master_in.read(2))
            assert len(command) == 2, f"read_master got '$', expected 2 bytes on read got {command!r}"
            #print(f"got ${command=}")
            getattr(master_requests, command[1])()
        else:
            # Assume that this is a response from master to a user command.
            # Show line to user.
            #print("reading line")
            line = utils.to_str(utils.Master_in.readline())   # binary files only use \n as newline char
            #print("got line", repr(line))
            if not line: break
            print(line, end='')
    #print("read_master done")

def read_stdin():
    #print("read_stdin called")
    line = sys.stdin.readline()
    #print("read_stdin got", repr(line))
    if line:
        bytes = utils.to_bytes(line)
        utils.Master_out.write(bytes)
        utils.Master_out.flush()
    else:
        return True  # quit!

def drive():
    read_interfaces()
    serial_devices = get_serial_devices()
    assert serial_devices, "Expression console not plugged in"
    serial_device = serial_devices[0]
    with open(serial_device, 'rb') as master_in, \
         open(serial_device, 'wb') as master_out:
        os.set_blocking(master_in.fileno(), False)
        utils.Master_in = master_in
        utils.Master_out = master_out
        old = termios.tcgetattr(utils.Master_in)
        assert utils.Master_in.isatty(), f"input {serial_device} not a tty"
        assert utils.Master_out.isatty(), f"output {serial_device} not a tty"
        #dump_termios(old)
        termios.tcsetattr(utils.Master_in, termios.TCSANOW, old[0:4] + [termios.B230400]*2 + old[6:])
        if False:
            new_in = termios.tcgetattr(utils.Master_in)
            print("new_in baud rates", new_in[4], new_in[5])
            new_out = termios.tcgetattr(utils.Master_out)
            print("new_out baud rates", new_out[4], new_out[5])

        #print("drive: waiting for startup messages") 
        #process(2)
        #print("?", file=utils.Master_out)
        #print("drive: waiting for help messages") 
        #process(2)
        init()
        process()
    print("drive done") 

def init():
    # Request initialization
    print("Sending I")
    bytes_sent = utils.Master_out.write(b'I\n')
    utils.Master_out.flush()
    assert bytes_sent == 2, f"init: expected bytes_sent == 2, got {bytes_sent}"
    process(0.1)

    # Perform V test (can Master receive all 256 byte values?)
    first, last = 0, 255
    command = b'V%i,%i\n' % (first, last)
    print("Sending", command)
    bytes_sent = utils.Master_out.write(command)
    assert bytes_sent== len(command), f"V: expected bytes_sent == {len(command)}, got {bytes_sent}"
    utils.Master_out.write(bytes(range(first, last + 1)))
    utils.Master_out.flush()
    process(0.3)

    # Perform G test (can Master send all 256 byte values?)
    first, last = 0, 255
    command = b'G%i,%i\n' % (first, last)
    print("Sending", command)
    bytes_sent = utils.Master_out.write(command)
    assert bytes_sent== len(command), f"G: expected bytes_sent == {len(command)}, got {bytes_sent}"
    utils.Master_out.flush()
    time.sleep(0.3)
    num_errors = 0
    for i in range(first, last + 1):
        x = utils.Master_in.read(1)
        assert len(x) == 1, f"G: read(1) returned {x!r}"
        if x[0] != i:
            print(f"G: expected {i}, got {x[0]}")
            num_errors += 1
    if num_errors == 0:
        print("G: no errors")
    print("init done")

def process(timeout=None):
    if timeout is not None:
        end_time = time.time() + timeout
    quit = False
    while not quit:
        if timeout is None:
            select_timeout = None
        else:
            now = time.time()
            if end_time > now:
                select_timeout = end_time - now
            else:
                break
        ready = select.select([sys.stdin, utils.Master_in], [], [], select_timeout)[0]
        if utils.Master_in in ready:
            quit = read_master()
        if sys.stdin in ready:
            quit = read_stdin()

def dump_termios(old):
    print("iflag", hex(old[0]))
    print("iflag INPCK", old[0] & termios.INPCK)
    print("oflag", hex(old[1]))
    print("cflag", hex(old[2]))
    print("cflag CSIZE", hex(old[2] & termios.CSIZE))
    print("termios.CS8", hex(termios.CS8))
    print("cflag CSTOPB", old[2] & termios.CSTOPB)
    print("cflag PARENB", old[2] & termios.PARENB)
    print("cflag PARODD", old[2] & termios.PARODD)
    print("lflag", hex(old[3]))
    print("lflag ECHO", old[3] & termios.ECHO)
    print("lflag ECHONL", old[3] & termios.ECHONL)
    print("default baud rates", old[4], old[5])
    print("termios.B9600", termios.B9600)
    print("termios.B230400", termios.B230400)
    print("cc", old[6])



if __name__ == "__main__":
    #print("stdin isatty?", sys.stdin.isatty())
    #print("stdin class", sys.stdin.__class__.__name__)  # TextIOWrapper
    #print("dir(stdin)", dir(sys.stdin))
    drive()
