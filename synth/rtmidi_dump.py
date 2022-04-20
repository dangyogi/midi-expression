# rtmidi.py

import time

import rtmidi
import rtmidi.midiutil


#def sig_handler(signum, stackframe):
#    print("sig_handler caught", signum, "at", time.perf_counter())
#    Midiin.cancel_callback()


class Midi_in:
    process_until_overhead = 0.0005         # sec, reported as not quite 0.0003

    def __init__(self):
        self.midiin, self.port_name = rtmidi.midiutil.open_midiinput(
                                        port=None,          # default None
                                        api=0,              # default 0
                                        use_virtual=True,   # default False
                                        interactive=False,  # default True
                                        client_name="synth_client",
                                        port_name="synth port")

        # ignore_types(sysex=True, timing=True, active_sense=True)  # defaults
        #self.midiin.ignore_types(sysex=False)

        print("current_api", self.midiin.get_current_api())
        print("port_count", self.midiin.get_port_count())
        print("ports", self.midiin.get_ports())
        print("port open", self.midiin.is_port_open())
        # self.midiin.set_client_name("synth client name")
        # self.midiin.set_port_name("synth port name")
        # self.midiin.open_port(port_num, "port name")
        # self.midiin.open_virtual_port("port name")

        #signal.signal(signal.SIGALRM, sig_handler)
        self.process_time_overruns = 0
        self.total_process_time_over = 0
        self.in_callback = False
        self.num_waits_for_callback = 0

        self.midiin.set_callback(self.call_callback)

    def report(self):
        print(f"Midi_in: {self.process_time_overruns} process_until time overruns", end='')
        if self.process_time_overruns:
            print(f", avg "
                  f"{self.total_process_time_over / self.process_time_overruns * 1e3} msec")
        else:
            print()
        print(f"Midi_in: {self.num_waits_for_callback} waits for callback to complete")

    def close(self):
        self.midiin.cancel_callback()
        self.midiin.close_port()

    def call_callback(self, message, bogus):
        assert not self.in_callback, "Midi_in: call_callback called while still running!"
        self.in_callback = True
        midi_message, delta_time = message
        if midi_message[0] in (0x80, 0x90):
            if midi_message[0] == 0x80:
                print("Note Off:",
                      hex(midi_message[1]).upper(),
                      hex(midi_message[2]).upper())
            else:
                print("Note On:",
                      hex(midi_message[1]).upper(),
                      hex(midi_message[2]).upper())
        self.in_callback = False




if __name__ == "__main__":

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

    print("done creating Midi_in")
    time.sleep(520)
    print("exiting")
    midiin.close()
