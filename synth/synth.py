# synth.py

from collections import defaultdict
import numpy as np
import math
from itertools import repeat

from .notify import Var, Notify_actors, Actor
from .utils import Num_harmonics, two_byte_value, Cross_setter
from .tuning_systems import freq_to_Hz
from .envelope import *
from .synth_settings import process_MIDI_control_change, process_MIDI_system_common


class Synth(Var):
    r'''This is the top-level object representing the entire synthesizer.

    Only expect one instance of this for the entire program.

    This includes all channels for the synth.
    '''

    volume = Notify_actors()
    key_signature = Notify_actors()
    tuning_system = Notify_actors()

    def __init__(self, soundcard, volume=0.3, tuning_system=None, key_signature=None):
        super().__init__("My synth")
        self.channels = {}  # {channel_number: Instrument}
        self.soundcard = soundcard
        self.block_duration = soundcard.block_duration
        self.block_size = soundcard.block_size
        self.dtype = soundcard.dtype
        self.delta_times = soundcard.delta_times
        self.volume = volume
        if tuning_system is not None:
            self.tuning_system = tuning_system
        if key_signature is not None:
            self.key_signature = key_signature
        self.last_status_byte = None
        self.idle_fun_caught_running = 0
        self.num_unknown_MIDI_commands = 0
        self.unknown_MIDI_commands = set()
        self.num_unknown_channels = 0
        self.unknown_channels = set()
        self.keep_running = True

        self.system_funs = {   # these are passed (command, data)
            # system common:
            0x00: self.sysex,
            0x01: self.unknown_command,         # MIDI Time Code Quarter Frame
            0x02: self.unknown_command,         # Song Position Pointer
            0x03: self.unknown_command,         # Song Select
            0x04: self.system_common,           # <undefined>
            0x05: self.system_common,           # <undefined>
            0x06: self.tune_request,
            0x07: self.unknown_command,         # <end of sysex>, not a command!

            0x08: self.unknown_command,         # Timing Clock
            0x09: self.unknown_command,         # <undefined>
            0x0A: self.start,
            0x0B: self.cont,
            0x0C: self.stop,
            0x0D: self.unknown_command,         # <undefined>
            0x0E: self.unknown_command,         # Active Sensing
            0x0F: self.system_reset,
        }
        self.channel_funs = {  # these are passed (command, channel, data)
            0x80: "note_off",
            0x90: "note_on",
            0xA0: "aftertouch",
            0xB0: "control_change",
            0xC0: "patch_change",
            0xD0: "channel_pressure",
            0xE0: "pitch_bend",
        }

    def report(self):
        print(f"synth caught idle_fn running {self.idle_fun_caught_running}")
        print(f"synth got {self.num_unknown_MIDI_commands} unknown MIDI system commands",
              self.unknown_MIDI_commands)
        print(f"synth got {self.num_unknown_channels} commands for unknown channels",
              self.unknown_channels)
        for inst in self.channels.values():
            inst.report()

    def register_instrument(self, channel, instrument):
        self.channels[channel] = instrument

    def process_MIDI(self, message):
        r'''Processes a MIDI message.  This is a list of numbers...
        '''
        #print("synth.process_MIDI", message)
        #command, rest = message[0], message[1:]
        command, *rest = message
        #print("synth.process_MIDI", hex(command), rest)
        first_nibble = command & 0xF0
        if first_nibble & 0x80:
            # got status byte
            self.last_status_byte = command
        else:
            # status byte continued from last message...
            assert self.last_status_byte is not None
            command, rest = self.last_status_byte, message
            first_nibble = command & 0xF0
        if first_nibble == 0xF0:
            # System message:
            self.system_funs[command & 0x0F](command, rest)
        else:
            channel = command & 0x0F
            if channel in self.channels:
                getattr(self.channels[channel],
                        self.channel_funs[first_nibble])(command, channel, rest)
            else:
                print("process_MIDI: got command", self.channel_funs[first_nibble],
                      "for unknown channel", channel)
                self.num_unknown_channels += 1
                self.unknown_channels.add(channel)

    def unknown_command(self, command, bytes):
        self.num_unknown_MIDI_commands += 1
        self.unknown_MIDI_commands.add(command)
        print("unknown MIDI command:", command, "ignored...")

    def system_common(self, command, bytes):
        process_MIDI_system_common(self, command, bytes)

    def sysex(self, command, bytes):         # 0xF0
        manuf_id, model_id, *rest = bytes
        print("MIDI sysex:", manuf_id, model_id, "len of bytes", len(bytes))

    def tune_request(self, command, bytes):  # 0xF6, not sure how this works...
        print("MIDI tune_request")

    def start(self, command, bytes):         # 0xFA, does this have data?
        print("MIDI start")

    def cont(self, command, bytes):          # 0xFB, does this have data?
        print("MIDI continue")

    def stop(self, command, bytes):          # 0xFC, does this have data?
        print("MIDI stop")

    def system_reset(self, command, bytes):  # 0xFF, does this have data?
        print("MIDI system reset")

    def schedule_quit(self):
        #print("synth.schedule_quit called")
        self.keep_running = False

    def fill_sound_block(self, block):
        r'''Returns True to continue, False to quit (last sound block).
        '''
        for instrument in self.channels.values():
            instrument.populate_sound_block(block)
        if not self.keep_running:
            print("synth.fill_sound_block returning False to quit")
        return self.keep_running


class Instrument(Actor):
    r'''
    Provides notifications for key_signature changes, tuning_system changes and
    volume changes.
    '''
    #scale_volume = 1/12 * 0.5
    scale_volume = 0

    volume = Notify_actors()
    key_signature = Notify_actors()
    tuning_system = Notify_actors()

    def __init__(self, name, synth, volume=None, tuning_system=None, key_signature=None):
        super().__init__(synth, name=name)
        self.synth = synth
        self.harmonics = []     # indexed by harmonic number - 1
        if volume is None:
            self.volume_from_synth = True
            self.volume = self.synth.volume
        else:
            self.volume_from_synth = False
            self.volume = volume
        if key_signature is None:
            self.key_sig_from_synth = True
            self.key_signature = synth.key_signature
        else:
            self.key_sig_from_synth = False
            self.key_signature = key_signature
        if tuning_system is None:
            self.tuning_system_from_synth = True
            self.tuning_system = synth.tuning_system
        else:
            self.tuning_system_from_synth = False
            self.tuning_system = tuning_system
        self.notes_playing = defaultdict(list)  # {midi_note: [Play_harmonic]}
        self.num_notes_already_playing = 0
        self.num_notes_not_playing = 0
        self.num_note_ons = 0
        self.num_note_offs = 0
        self.num_aftertouches = 0
        self.num_control_changes = 0
        self.num_patch_changes = 0
        self.num_channel_pressures = 0
        self.num_pitch_bends = 0
        self.harmonic_focus = 0x7F
        self.non_registered_param_LSB = 0x7F
        self.non_registered_param_MSB = 0
        self.data_entry_course = None
        self.data_entry_fine = None

        self.control_numbers = {
            # Anything not in here is sent to process_MIDI_control_change()
            # General MIDI level 1 (some, not all that are required):
            0x40: self.sustain,
            0x79: self.reset_all_controllers,
            0x7B: self.all_notes_off,
        }

    def report(self):
        print(self.name, "got:")
        print("   ", self.num_note_ons, "note_ons")
        print("   ", self.num_note_offs, "note_offs")
        print("   ", self.num_notes_already_playing, "notes already playing")
        print("   ", self.num_notes_not_playing, "notes not playing")
        print("   ", self.num_aftertouches, "aftertouches")
        print("   ", self.num_control_changes, "control_changes")
        print("   ", self.num_patch_changes, "patch_changes")
        print("   ", self.num_channel_pressures, "channel_pressures")
        print("   ", self.num_pitch_bends, "pitch_bends")

    def recalc(self):
        if self.volume_from_synth:
            self.volume = self.synth.volume
        if self.key_sig_from_synth:
            self.key_signature = self.synth.key_signature
        if self.tuning_system_from_synth:
            self.tuning_system = self.synth.tuning_system

    def add_harmonic(self, harmonic):
        # FIX: Should these all just be created at the beginning?
        self.harmonics.append(harmonic)

    def note_on(self, command, channel, bytes):             # 0x90
        midi_note, velocity = bytes
        if velocity == 0:
            self.note_off(command, channel, bytes)
        else:
            self.num_note_ons += 1

            #scaled_velocity = velocity / (math.exp(midi_note - 21) / self.scale_volume)

            # scale_volume ~ 1/12 * 0.125
            #scaled_velocity = velocity / ((midi_note - 21) * self.scale_volume + 1)

            scaled_velocity = velocity - (midi_note - 21) * self.scale_volume
            my_velocity = scaled_velocity / 127
            print(f"note_on {midi_note=}, {velocity=}, {scaled_velocity=:.0f}, "
                  f"{my_velocity=:.3f}")

            # kill note if already playing...
            if midi_note in self.notes_playing:
                print(f"killing {midi_note=}: already playing")
                self.num_notes_already_playing += 1
                Num_harmonics.value -= len(self.notes_playing[midi_note])
                for ph in self.notes_playing[midi_note]:
                    ph.delete()
                del self.notes_playing[midi_note]
            harmonics_added = 0
            for h in self.harmonics:
                ph_list = h.play(midi_note, my_velocity)
                #print("note_on got ph_list", ph_list)
                if ph_list:
                    self.notes_playing[midi_note].extend(ph_list)
                    harmonics_added += len(ph_list)
            #print("note_on", midi_note, len(self.harmonics), harmonics_added,
            #      self.synth.idle_fun_running)
            if self.synth.idle_fun_running:
                self.synth.idle_fun_caught_running += 1
            if harmonics_added:
                Num_harmonics.value += harmonics_added
            #print("note_on", midi_note, len(self.harmonics), harmonics_added,
            #      self.synth.idle_fun_running, Num_harmonics.value)

    def note_off(self, command, channel, bytes):            # 0x80
        midi_note, velocity = bytes
        self.num_note_offs += 1
        #print(f"note_off {midi_note=}, {velocity=}")
        if midi_note in self.notes_playing:
            for ph in self.notes_playing[midi_note]:
                ph.note_off(velocity)
        else:
            self.num_notes_not_playing += 1
            print(f"note_off: note {midi_note=} not playing!")

    def aftertouch(self, command, channel, bytes):          # 0xA0
        midi_note, pressure = bytes
        # aka, "polyphonic aftertouch" -- rarely implemented by keyboards,
        # see channel_pressure for what's typically provided by keyboards...
        self.num_aftertouches += 1
        if midi_note in self.notes_playing:
            for ph in self.notes_playing[midi_note]:
                ph.aftertouch(pressure)
        else:
            self.num_notes_not_playing += 1
            print(f"aftertouch: note {midi_note=} not playing!")

    def control_change(self, command, channel, bytes):    # 0xB0
        control_number, value = bytes
        if control_number not in self.control_numbers:
            process_MIDI_control_change(self.synth, channel, control_number, value)
        else:
            self.control_numbers[control_number](control_number, value)
        self.num_control_changes += 1

    def sustain(self, control_number, value):
        r'''value <= 63 off; >= 64 on.
        '''
        pass

    def reset_all_controllers(self, control_number, value):
        r'''value always 0.
        '''
        pass

    def all_notes_off(self, control_number, value):
        r'''value always 0.
        '''
        pass

    def patch_change(self, command, channel, bytes):             # 0xC0
        program_number, = bytes
        print("patch change", program_number)
        self.num_patch_changes += 1

    def channel_pressure(self, command, channel, bytes):         # 0xD0
        pressure, = bytes
        # aka, "Channel aftertouch"... not used
        print("channel pressure", pressure)
        self.num_channel_pressures += 1

    def pitch_bend(self, command, channel, bytes):               # 0xE0
        lsb, msb = bytes
        # bend_amount is 14 bits (max 16383)
        # 8192 is no bend (the zero value)
        # lower numbers bend down, higher numbers bend up
        # range of pitch bend often 2 semitones (200 cents).  (But this range is often a
        # settable parameter).
        bend_amount = ((msb << 7) | lsb) - 8192   # -8192 to 8191
        print("pitch bend", bend_amount)
        self.num_pitch_bends += 1

    def populate_sound_block(self, block):
        r'''Doesn't return anything.
        '''
        if self.notes_playing:
            if self.synth.idle_fun_running:
                self.synth.idle_fun_caught_running += 1
            num_harmonics_deleted = 0
            note_deletes = defaultdict(list)  # {note: [ph_list indexes to delete]}
            for midi_note, ph_list in self.notes_playing.items():
                assert ph_list
                for i, ph in enumerate(ph_list):
                    if not ph.populate_sound_block(block):
                        note_deletes[midi_note].append(i)
            for note, ph_indexes in note_deletes.items():
                ph_list = self.notes_playing[note]
                for i in sorted(ph_indexes, reverse=True):
                    del ph_list[i]
                num_harmonics_deleted += len(ph_indexes)
                if not ph_list:
                    del self.notes_playing[note]
            if num_harmonics_deleted:
                Num_harmonics.value -= num_harmonics_deleted


class Harmonic(Var):
    r'''Represents a single harmonic for an Instrument.

    Reports changes in ampl_offset, freq_offset, attack_env, decay_env, sustain_env,
    release_env and freq_env.
    '''
    def __init__(self, instrument, harmonic, ampl_offset, freq_offset,
                 attack_env=None, decay_env=None, sustain_env=None, release_env=None,
                 freq_env=None):
        super().__init__(name=f"{instrument.name}-H{harmonic}")
        self.instrument = instrument
        self.harmonic = harmonic

        self.ampl_offset = ampl_offset   # multiplied by note_on/off_envelope
        self.note_on_sequence = [None, None, None]  # attack, decay, sustain
        self.note_on_env = Sequence(self.note_on_sequence)
        self.attack_env = attack_env
        self.decay_env = decay_env
        if attack_env is None and decay_env is None and sustain_env is None:
            self.sustain_env = Constant(f"{self.name}-Constant", self, self.ampl_offset)
        else:
            self.sustain_env = sustain_env
        self.release_env = release_env

        self.freq_offset = freq_offset   # multiplied into freq_to_Hz(base_freq) for note
        self.sin_env = Sin(f"{self.name}-sin", self)
        if freq_env is not None:
            assert freq_env.add_base_freq, \
              f"{self.name}: freq_env must have add_base_freq set"
        self.freq_env = freq_env         # must have center_ampl = 'base_freq'

    ampl_offset = Notify_actors()   # 0 turns off this Harmonic
    freq_offset = Notify_actors()

    @Cross_setter
    def freq_offset_course(self, value):
        self.freq_offset = value + self.freq_offset_fine

    @Cross_setter
    def freq_offset_find(self, value):
        self.freq_offset = self.freq_offset_course + value

    attack_env = Notify_actors()

    @Cross_setter
    def _attack_env(self, value):
        self.note_on_sequence[0] = value

    decay_env = Notify_actors()

    @Cross_setter
    def _decay_env(self, value):
        self.note_on_sequence[1] = value

    sustain_env = Notify_actors()

    @Cross_setter
    def _sustain_env(self, value):
        self.note_on_sequence[2] = value

    release_env = Notify_actors()
    freq_env = Notify_actors()

    def play(self, note, velocity):
        r'''Returns a (possibly empty) list of Play_harmonics.
        '''
        if self.ampl_offset == 0:
            # I'm turned off right now..
            return []
        base_freq = self.instrument.tuning_system.note_to_freq(note)
        if self.freq_env is not None:
            freq_gen = self.freq_env.start(self.base_freq)
            waveform_gen = self.sin_env.start(freq_gen)
        else:
            waveform_gen = self.sin_env.start(base_freq)
        ampl_gen = self.note_on_env.start(base_freq)
        return [Play_harmonic(self, base_freq, velocity, waveform_gen, ampl_gen)]


class Play_harmonic(Actor):
    r'''
    Doesn't report changes to any variables, only acts as Actor...
    '''
    # FIX: add panning
    def __init__(self, harmonic, base_freq, velocity, waveform_gen, ampl_gen):
        r'''Neither base_freq nor velocity can be changed while the note is playing...
        '''
        super().__init__(harmonic.instrument,
                         name=f"Play_harmonic_{harmonic.instrument.num_note_ons}")
        self.harmonic = harmonic
        self.base_freq = base_freq
        self.velocity = velocity
        self.waveform_gen = waveform_gen
        self.waveform_it = iter(self.waveform_gen)
        self.ampl_gen = ampl_gen
        self.ampl_it = iter(self.ampl_gen)
        self.recalc()

    def recalc(self):
        self.volume = self.harmonic.instrument.volume * self.velocity

    def aftertouch(self, pressure):
        # FIX
        pass

    def note_off(self, velocity):
        r'''velocity ignored...
        '''
        start = self.ampl_gen.next_value()
        if self.harmonic.release_env is not None:
            self.ampl_gen = self.harmonic.release_env.start(self.base_freq, start)
        else:
            self.ampl_gen = ()
        self.ampl_it = iter(self.ampl_gen)

    def populate_sound_block(self, block):
        r'''Returns True to continue, False if done.
        '''
        try:
            base_waveform = next(self.waveform_it)
        except StopIteration:
            self.delete()
            return False
        try:
            ampl_envelope = next(self.ampl_it)
        except StopIteration:
            self.delete()
            return False
        block += base_waveform * ampl_envelope * self.volume
        return True

