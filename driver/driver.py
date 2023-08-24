# driver.py

import sys
import time
import select
import glob
import termios
import io
import os
import codecs

from interfaces import read_interfaces
import master_requests


Ascii_encoder = codecs.getencoder('ascii')
Ascii_decoder = codecs.getdecoder('ascii')


def get_serial_devices():
    return glob.glob("/dev/ttyACM*")

def read_master():
    #print("copy_to_stdout", Master_in.name)
    line = Master_in.readline()   # binary files only use \n as newline char
    if line:
        if line[0] != b'$':
            # Assume that this is a response from master to a user command.
            # Show line to user.
            print(Ascii_decoder(line[:-1])[0])
        else:
            # Request from master to driver, letter after '$' is command.
            getattr(master_requests, Ascii_decoder(line[1])[0])(line, Master_out)

def read_stdin():
    line = sys.stdin.readline()
    if line:
        bytes = Ascii_encoder(line.rstrip())[0]
        Master_out.write(bytes)
        Master_out.flush()
    else:
        return True  # quit!

def drive():
    global Controllers, Master_in, Master_out
    Controllers = read_interfaces()
    serial_devices = get_serial_devices()
    assert serial_devices, "Expression console not plugged in"
    serial_device = serial_devices[0]
    with open(serial_device, 'rb') as Master_in, \
         open(serial_device, 'wb') as Master_out:
        old = termios.tcgetattr(Master_in)
        assert Master_in.isatty(), f"input {serial_device} not a tty"
        assert Master_out.isatty(), f"output {serial_device} not a tty"
        #dump_termios(old)
        termios.tcsetattr(Master_in, termios.TCSANOW, old[0:4] + [termios.B230400]*2 + old[6:])
        if False:
            new_in = termios.tcgetattr(Master_in)
            print("new_in baud rates", new_in[4], new_in[5])
            new_out = termios.tcgetattr(Master_out)
            print("new_out baud rates", new_out[4], new_out[5])

        #print("drive: waiting for startup messages") 
        #process(2)
        #print("?", file=Master_out)
        #print("drive: waiting for help messages") 
        #process(2)
        process()
    print("drive done") 

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
        ready = select.select([sys.stdin, Master_in], [], [], select_timeout)[0]
        if Master_in in ready:
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
