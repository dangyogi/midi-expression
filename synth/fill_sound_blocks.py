# fill_sound_blocks.py


r'''This provides `fill_sound_block` as the callback for the Soundcard.

Must call init to get this started.
'''

import time

from .utils import Largest_value_calculator, Num_harmonics, Target_time


Largest_gen_time = Largest_value_calculator("Largest gen_time/harmonic")


def init(synth, midiin):
    global Synth, Midiin, overruns, time_over, idle_running
    Synth = synth
    Midiin = midiin
    overruns = 0
    time_over = 0
    idle_running = 0


def fill_sound_block(block, target_time):
    r'''Callback for soundcard.

    Processes MIDI events and generates one sound segment, aiming to complete before
    target_time.

    Returns True to continue, False to quit (last sound block).
    '''
    #print("fill_sound_block called with target_time", target_time)
    global overruns, time_over, idle_running
    Target_time.set_target_time(target_time, Largest_gen_time.get_largest_value())
    Midiin.process_until(Synth.process_MIDI)
    start_time = time.perf_counter()
    ans = Synth.fill_sound_block(block)
    now = time.perf_counter()
    if now > target_time:
        overruns += 1
        time_over += now - target_time
    if Synth.idle_fun_running:
        idle_running += 1
    #print(f"fill_sound_block: {id(Num_harmonics)=}, {Num_harmonics.value=}")
    #print(f"fill_sound_block: {Synth.idle_fun_running=}, {Num_harmonics.value=}, {ans=}")
    if Num_harmonics.value > 0:
        Largest_gen_time.add((now - start_time) / Num_harmonics.value)
    #print("fill_sound_block returning", ans)
    return ans


def report():
    print(f"fill_sound_block: {overruns=}", end='') 
    if overruns:
        print(f", avg {time_over / overruns * 1e3} msec") 
    else:
        print()
    Largest_gen_time.report(1e3, "msec")
    print(f"fill_sound_block: {idle_running=}") 
