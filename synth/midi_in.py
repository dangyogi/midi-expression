# midi_in.py

r'''
midi_in = Midi_in()

Then, for each time segment, call Target_time.set_target_time(target_time, gen_time)
then midi_in.process_until(callback).  Callback takes the MIDI message as a list of numbers
for each byte in the message.

Target_time is defined in utils.py.
'''

import time
from queue import SimpleQueue, Empty

import rtmidi
import rtmidi.midiutil

from .utils import Largest_value_calculator, Target_time
from .notify import Actor, recalc_actors


#def sig_handler(signum, stackframe):
#    print("sig_handler caught", signum, "at", time.perf_counter())
#    Midiin.cancel_callback()


class Midi_in(Actor):
    process_until_overhead = 0.0008         # sec, reported as not quite 0.0003

    def __init__(self, name="midi_in"):
        super().__init__(Target_time, name=name)
        self.midiin, self.port_name = rtmidi.midiutil.open_midiinput(
                                        port=None,          # default None
                                        api=0,              # default 0
                                        use_virtual=True,   # default False
                                        interactive=False,  # default True
                                        client_name="My Synth",
                                        port_name="Synth In")

        # ignore_types(sysex=True, timing=True, active_sense=True)  # defaults
        self.midiin.ignore_types(sysex=False)

        print("current_api", self.midiin.get_current_api())
        print("port_count", self.midiin.get_port_count())
        print("ports", self.midiin.get_ports())
        print("port open", self.midiin.is_port_open())
        # self.midiin.set_client_name("synth client name")
        # self.midiin.set_port_name("synth port name")
        # self.midiin.open_port(port_num, "port name")
        # self.midiin.open_virtual_port("port name")

        #signal.signal(signal.SIGALRM, sig_handler)
        self.largest_callback_time = \
          Largest_value_calculator("Midi_in.largest_callback_time")
        self.process_time_overruns = 0
        self.total_process_time_over = 0
        self.in_callback = False
        self.queue = SimpleQueue()
        self.midiin.set_callback(self.queue.put, True)
        self.process_until_calls = 0

    def report(self):
        self.largest_callback_time.report(1e3, "msec")
        print(f"Midi_in: {self.process_until_calls} process_until calls")
        print(f"Midi_in: {self.process_time_overruns} process_until time overruns", end='')
        if self.process_time_overruns:
            print(f", avg "
                  f"{self.total_process_time_over / self.process_time_overruns * 1e3} msec")
        else:
            print()

    def close(self):
        self.midiin.close_port()

    def recalc(self):
        self.target_time = Target_time.target_time - self.process_until_overhead
        #print("Midi_in.recalc set self.target_time to", self.target_time)

    def process_until(self, callback):
        recalc_actors()  # to set self.target_time
        self.process_until_calls += 1
        time_left = self.target_time - time.perf_counter() \
                      - self.largest_callback_time.get_largest_value()
        try:
            while time_left > 0:
                #signal.setitimer(signal.ITIMER_REAL, target_time - now)  # raises SIGALRM
                #print("Midi_in.process_until: calling self.queue.get with timeout",
                #      time_left)
                message, delta_time = self.queue.get(timeout=time_left)
                #print("Midi_in.process_until: got", message)

                assert not self.in_callback, "Midi_in: callback called while still running!"
                self.in_callback = True
                start_time = time.perf_counter()
                callback(message)
                self.in_callback = False
                recalc_actors()
                now = time.perf_counter()
                self.largest_callback_time.add(now - start_time)

                time_left = self.target_time - now \
                              - self.largest_callback_time.get_largest_value()
        except Empty:
            #print("Midi_in.process_until: timeout")
            pass
        time_over = time.perf_counter() - self.target_time
        if time_over > 0:
            self.process_time_overruns += 1
            self.total_process_time_over += time_over
            #print("process_until ran over target time by", time_over * 1e3,
            #      "msec with largest_callback_time",
            #      self.largest_callback_time.get_largest_value())




if __name__ == "__main__":
    from .utils import Target_time

    print("api_from_env", rtmidi.midiutil.get_api_from_environment())
    print("API_UNSPECIFIED", rtmidi.API_UNSPECIFIED)
    print("API_LINUX_ALSA", rtmidi.API_LINUX_ALSA)
    print("API_UNIX_JACK", rtmidi.API_UNIX_JACK)
    print("API_RTMIDI_DUMMY", rtmidi.API_RTMIDI_DUMMY)
    print("compiled_apis", rtmidi.get_compiled_api())
    print("version", rtmidi.get_rtmidi_version())
    #print("avail_ports", rtmidi.midiutil.list_available_ports())   # needs midiio module
    print("avail_input_ports", rtmidi.midiutil.list_input_ports())
    print("avail_output_ports", rtmidi.midiutil.list_output_ports())

    midiin = Midi_in()

    def callback(message):
        print("callback got", hex(message[0]), message[1:])

    for _ in range(15*20):
        start_time = time.perf_counter()
        #print("starting at",  - start_time)
        Target_time.set_target_time(start_time + 0.050, 0.001)
        midiin.process_until(callback)
    print("done calling process_until")
    time.sleep(5)
    print("exiting")
    midiin.close()

    if False:
        valid_signals = signal.valid_signals()
        print("valid signals", valid_signals)

        def sig_handler(signum, stackframe):
            print("sig_handler caught", signum, "at", time.perf_counter() - start_time)

        signal.signal(signal.SIGALRM, sig_handler)

        #signal.siginterrupt(signal.SIGPROF, True)
        start_time = time.perf_counter()
        signal.setitimer(signal.ITIMER_REAL, 0.050)  # raises SIGALRM
        time.sleep(2)
        signal.setitimer(signal.ITIMER_REAL, 0.050)  # raises SIGALRM
        time.sleep(1)
