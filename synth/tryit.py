# tryit.py

r'''
Overall control flow:

fill_sound_blocks.fill_sound_block is registered as Soundcard callback.

Soundcard.callback automatically called to get another sound segment:
    - calls fill_sound_blocks.fill_sound_block
       - fill_sound_blocks.fill_sound_block
          - sets Target_time
          - calls Midiin.process_until(target_time, synth.process_MIDI)
             - sets midi callback to self.call_callback
                - call_callback calls synth.process_MIDI for each midi event
             - sleeps for Target_time - fudge
             - cancels midi callback
          - calls synth.fill_sound_block
             - calls instrument.populate_sound_block
                - calls Play_harmonic.populate_sound_block
'''

import time
import signal

from .notes import Key_signature
from .tuning_systems import Equal_temperament
from .pyaud import Soundcard
from . import fill_sound_blocks
from .synth import Synth, Instrument, Harmonic
from .midi_in import Midi_in
from . import notify


def init(idle_fun=None):
    global synth, midiin, soundcard

    key_sig = Key_signature()
    tuning_system = Equal_temperament()
    midiin = Midi_in()
    soundcard = Soundcard(fill_sound_blocks.fill_sound_block, idle_fn=idle_fun)
    synth = Synth(soundcard, tuning_system=tuning_system, key_signature=key_sig)
    synth.idle_fun_running = False
    instrument = Instrument("clavier", synth)
    instrument.add_harmonic(Harmonic(instrument, 1, 1.0, 1.0))
    synth.register_instrument(0, instrument)
    fill_sound_blocks.init(synth, midiin)
    signal.signal(signal.SIGINT, sigint)


class Quit(Exception):
    pass

def sigint(sig, stackframe):
    synth.schedule_quit()
    raise Quit          # Necessary to prevent program from being forcably exitted
                        # (even with a SIGINT handler).


def fini():
    print("tryit.fini")
    # These are in the order that the time critical functions are called, so that the
    # reporting makes more sense:
    soundcard.report()
    print("Calling fill_sound_blocks.report()")
    fill_sound_blocks.report()
    print("Calling midiin.report()")
    midiin.report()
    print("Calling synth.report()")
    synth.report()
    print("Calling notify.report()")
    notify.report()

    print("tryit.fini doing closes")
    midiin.close()
    soundcard.close()  # This is the naughty one!  Do it LAST!
    print("tryit.fini done")



# MIDI notes, octave 4:
C = 60
D = C + 2
E = D + 2
F = E + 1
G = F + 2
A = G + 2
Note1 = A
Note2 = F

action = 'init'
def idle_fun():
    global action, time_to_act

    #print("idle_fun called")

    if action == 'init':
        print("****** init")
        print("****** setting time_to_act")
        time_to_act = time.perf_counter() + 0.1
        action = 'note1_on'
    elif time.perf_counter() >= time_to_act:
        if action == 'note1_on':
            print("****** calling note_on", Note1)
            synth.idle_fun_running = True
            synth.channels[0].note_on(Note1, 60)
            synth.idle_fun_running = False
            time_to_act = time.perf_counter() + 1
            action = 'note1_off'
        elif action == 'note1_off':
            print("****** calling note_off", Note1)
            synth.idle_fun_running = True
            synth.channels[0].note_off(Note1, 10)
            synth.idle_fun_running = False
            time_to_act = time.perf_counter() + 0.1
            action = 'note2_on'
        elif action == 'note2_on':
            print("****** calling note_on")
            synth.idle_fun_running = True
            synth.channels[0].note_on(Note2, 60)
            synth.idle_fun_running = False
            time_to_act = time.perf_counter() + 1
            action = 'note2_off'
        elif action == 'note2_off':
            print("****** calling note_off")
            synth.idle_fun_running = True
            synth.channels[0].note_off(Note2, 10)
            synth.idle_fun_running = False
            time_to_act = time.perf_counter() + 0.1
            action = 'done'
        elif action == 'done':
            print("****** done, calling sys.exit(0)")
            time_to_act = time.perf_counter() + 10
            sys.exit(0)
            action = 'quiet'




if __name__ == "__main__":
    import sys

    #init(idle_fun=idle_fun)
    init()
    print("tryit: calling soundcard.run()")
    try:
        soundcard.run()
    except Quit:
        pass
    except BaseException as e:
        print("tryit caught exception", e.__class__.__name__)
    fini()
