# synth.py

from collections import defaultdict
import numpy as np
from itertools import repeat

from .channel import Var, Actor
from .utils import Num_harmonics
from .tuning_systems import freq_to_Hz


class Synth(Var):
    r'''
    Reports changes in key_signature, tuning_system, and volume.
    '''

    def __init__(self, soundcard, volume=0.3, tuning_system=None, key_signature=None):
        super().__init__("My synth")
        self.channels = {}  # {channel_number: Instrument}
        self.soundcard = soundcard
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

        self.system_funs = {
            0x00: self.sysex,
            0x01: self.unknown_command,         # MIDI Time Code Quarter Frame
            0x02: self.unknown_command,         # Song Position Pointer
            0x03: self.unknown_command,         # Song Select
            0x04: self.unknown_command,         # <undefined>
            0x05: self.unknown_command,         # <undefined>
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
        self.channel_funs = {
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
        command, rest = message[0], message[1:]
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
            self.system_funs[command & 0x0F] (command, *rest)
        else:
            channel = command & 0x0F
            if channel in self.channels:
                getattr(self.channels[channel],
                        self.channel_funs[first_nibble]) (*rest)
            else:
                print("process_MIDI: got command", self.channel_funs[first_nibble],
                      "for unknown channel", channel)
                self.num_unknown_channels += 1
                self.unknown_channels.add(channel)

    def unknown_command(self, command, *message):
        self.num_unknown_MIDI_commands += 1
        self.unknown_MIDI_commands.add(command)
        print("unknown MIDI command:", command, "ignored...")

    def sysex(self, manuf_id, model_id, *message):  # 0xF0
        print("MIDI sysex:", manuf_id, model_id, "len of message", len(message))

    def tune_request(self):  # 0xF6, not sure how this works...
        print("MIDI tune_request")

    def start(self):         # 0xFA, does this have data?
        print("MIDI start")

    def cont(self):          # 0xFB, does this have data?
        print("MIDI continue")

    def stop(self):          # 0xFC, does this have data?
        print("MIDI stop")

    def system_reset(self):  # 0xFF, does this have data?
        print("MIDI system reset")

    def fill_sound_block(self, block):
        r'''Returns True to continue, False to quit (last sound block).
        '''
        for instrument in self.channels.values():
            instrument.populate_sound_block(block)
        return True


class Instrument(Actor):
    r'''
    Provides notifications for tuning_system changes and volume changes.
    '''
    def __init__(self, name, synth, volume=None, tuning_system=None, key_signature=None):
        super().__init__(synth, name=name)
        self.synth = synth
        self.harmonics = []     # indexed by harmonic number
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
        self.velocities = {}                    # {midi_note: velocity}  # velocity is %
        self.num_notes_not_playing = 0
        self.num_note_ons = 0
        self.num_note_offs = 0
        self.num_aftertouches = 0
        self.num_control_changes = 0
        self.num_patch_changes = 0
        self.num_channel_pressures = 0
        self.num_pitch_bends = 0

    def report(self):
        print(self.name, "got:")
        print("   ", self.num_note_ons, "note_ons")
        print("   ", self.num_note_offs, "note_offs")
        print("   ", self.num_notes_not_playing, "notes not playing")
        print("   ", self.num_aftertouches, "aftertouches")
        print("   ", self.num_control_changes, "control_changes")
        print("   ", self.num_patch_changes, "patch_changes")
        print("   ", self.num_channel_pressures, "channel_pressures")
        print("   ", self.num_pitch_bends, "pitch_bends")

    def recalc(self):
        if self.volume_from_synth:
            self.set('volume', self.synth.volume)
        if self.key_sig_from_synth:
            self.set('key_signature', self.synth.key_signature)
        if self.tuning_system_from_synth:
            self.set('tuning_system', self.synth.tuning_system)

    def add_harmonic(self, harmonic):
        self.harmonics.append(harmonic)

    def note_on(self, midi_note, velocity):             # 0x90
        if velocity == 0:
            self.note_off(midi_note, 0)
        else:
            self.num_note_ons += 1
            my_note = self.key_signature.MIDI_to_note(midi_note)
            print(f"note_on {midi_note=}, {my_note=}, {velocity=}")
            velocity /= 127

            # kill note if already playing...
            if midi_note in self.notes_playing:
                print(f"killing {midi_note=}: already playing")
                Num_harmonics.dec('value', len(self.notes_playing[midi_note]))
                del self.notes_playing[midi_note]
                del self.velocities[midi_note]
            harmonics_added = 0
            for h in self.harmonics:
                ph_list = h.play(my_note, velocity)
                #print("note_on got ph_list", ph_list)
                if ph_list:
                    self.notes_playing[midi_note].extend(ph_list)
                    harmonics_added += len(ph_list)
            #print("note_on", midi_note, len(self.harmonics), harmonics_added,
            #      self.synth.idle_fun_running)
            if self.synth.idle_fun_running:
                self.synth.idle_fun_caught_running += 1
            if harmonics_added:
                self.velocities[midi_note] = velocity
                Num_harmonics.inc('value', harmonics_added)
            #print("note_on", midi_note, len(self.harmonics), harmonics_added,
            #      self.synth.idle_fun_running, Num_harmonics.value)

    def note_off(self, midi_note, velocity):            # 0x80
        self.num_note_offs += 1
        my_note = self.key_signature.MIDI_to_note(midi_note)
        print(f"note_off {midi_note=}, {my_note=}, {velocity=}")
        if midi_note in self.notes_playing:
            for ph in self.notes_playing[midi_note]:
                ph.note_off(velocity)
        else:
            self.num_notes_not_playing += 1
            print(f"note_off: note {midi_note=} not playing!")

    def aftertouch(self, midi_note, pressure):          # 0xA0
        # aka, "polyphonic aftertouch" -- rarely implemented by keyboards,
        # see channel_pressure for what's typically provided by keyboards...
        self.num_aftertouches += 1
        if midi_note in self.notes_playing:
            for ph in self.notes_playing[midi_note]:
                ph.aftertouch(pressure)
        else:
            self.num_notes_not_playing += 1
            print(f"aftertouch: note {midi_note=} not playing!")

    def control_change(self, control_number, value):    # 0xB0
        print("control change", hex(control_number), value)
        self.num_control_changes += 1

    def patch_change(self, program_number):             # 0xC0
        print("patch change", program_number)
        self.num_patch_changes += 1

    def channel_pressure(self, pressure):               # 0xD0
        # aka, "Channel aftertouch"... not used
        print("channel pressure", pressure)
        self.num_channel_pressures += 1

    def pitch_bend(self, lsb, msb):                     # 0xE0
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
            #print(f"populate_sound_block: {self.synth.idle_fun_running=}")
            if self.synth.idle_fun_running:
                self.synth.idle_fun_caught_running += 1
            instrument_block = self.synth.soundcard.new_block()
            num_harmonics_deleted = 0
            note_deletes = defaultdict(list)  # {note: [ph_list indexes to delete]}
            for midi_note, ph_list in self.notes_playing.items():
                assert ph_list
                note_block = self.synth.soundcard.new_block()
                for i, ph in enumerate(ph_list):
                    if not ph.populate_sound_block(note_block):
                        note_deletes[midi_note].append(i)
                instrument_block += note_block * self.velocities[midi_note]
            for note, ph_indexes in note_deletes.items():
                ph_list = self.notes_playing[note]
                for i in sorted(ph_indexes, reverse=True):
                    del ph_list[i]
                num_harmonics_deleted += len(ph_indexes)
                if not ph_list:
                    del self.notes_playing[note]
                    del self.velocities[note]
            #print(f"populate_sound_block: {num_harmonics_deleted=}, {note_deletes=}")
            if num_harmonics_deleted:
                Num_harmonics.dec('value', num_harmonics_deleted)
            instrument_block *= self.volume
            # FIX: add panning
            block += instrument_block


class Harmonic(Var):
    r'''Represents a single harmonic for an Instrument.

    Reports changes in ampl_offset, freq_offset, ampl_envelope, and freq_envelope.
    '''
    def __init__(self, instrument, ampl_offset, freq_offset,
                 note_on_envelope=None, note_off_envelope=None, freq_envelope=None):
        super().__init__()
        self.instrument = instrument
        self.ampl_offset = ampl_offset
        self.freq_offset = freq_offset
        if note_on_envelope is None:
            self.note_on_envelope = 1.0
        else:
            self.note_on_envelope = note_on_envelope
        if note_off_envelope is None:
            self.note_off_envelope = 1.0
        else:
            self.note_off_envelope = note_off_envelope
        if freq_envelope is None:
            self.freq_envelope = 1.0
        else:
            self.freq_envelope = freq_envelope

    def play(self, note, velocity):
        r'''Returns a (possibly empty) list of Play_harmonics.
        '''
        return [Play_harmonic(self, note, velocity)]

    def get_waveform(self, base_freq):
        # FIX
        freq_Hz = freq_to_Hz(base_freq)
        rad_cycle = freq_Hz * 2 * np.pi
        delta_times = self.instrument.synth.soundcard.delta_times
        waveform = np.cumsum(rad_cycle * delta_times)
        block_duration = self.instrument.synth.soundcard.block_duration
        inc = waveform[-1]       # freq_Hz * 2*pi * block_duration

        #print(f"get_waveform: {block_duration=}, {np.sum(delta_times)=}, {freq_Hz=}, "
        #      f"{inc=}, {inc % (2 * np.pi)=}")
        do_it = True
        if not do_it:
            yield from repeat(np.sin(waveform))
        else:
            start = 0
            while True:
                cycle_samples = waveform + start
                #print(f"{cycle_samples[0]=}, {cycle_samples[1]=}, {cycle_samples[-1]=}")
                yield np.sin(cycle_samples)
                start = (start + inc) % (2 * np.pi)

    def get_note_on_envelope(self, base_freq, velocity):
        # FIX: add ADS envelope, this is just contant ampl for now...
        return repeat(np.full(self.instrument.synth.soundcard.block_size,
                              self.ampl_offset * velocity))

    def get_note_off_envelope(self, base_freq, velocity):
        # FIX: kill note immediately for now...
        return iter(())


class Play_harmonic(Actor):
    r'''
    Doesn't report changes to any variables, only acts as Actor...
    '''
    def __init__(self, harmonic, note, velocity, **other_attrs):
        r'''Neither base_freq nor velocity can later be changed...
        '''
        super().__init__(harmonic.instrument, harmonic)
        self.harmonic = harmonic
        self.block_size = self.harmonic.instrument.synth.soundcard.block_size
        self.note = note
        self.velocity = velocity
        for key, value in other_attrs.items():
            setattr(self, key, value)
        self.state = 'note_on'
        self.recalc()

    def recalc(self):
        self.base_freq = self.harmonic.instrument.tuning_system.note_to_freq(self.note)
        self.waveform = iter(self.harmonic.get_waveform(self.base_freq))
        #print(f"recalc, {self.waveform=}")
        #print(f"recalc, {next(self.waveform)=}")
        if self.state == 'note_on':
            self.ampl_env = iter(self.harmonic.get_note_on_envelope(self.base_freq,
                                                                    self.velocity))
        else:
            self.ampl_env = iter(self.harmonic.get_note_off_envelope(self.base_freq,
                                                                     self.velocity))

    def aftertouch(self, pressure):
        # FIX
        pass

    def note_off(self, velocity):
        self.state = 'note_off'
        self.ampl_env = self.harmonic.get_note_off_envelope(self.base_freq, velocity)

    def populate_sound_block(self, block):
        r'''Returns True to continue, False if done.
        '''
        # FIX
        # try:
        #   base_waveform = next(waveform)
        #     freq_envelope = select freq slice
        #     cycle_over_time = \
        #       np.cumsum(freq_envelope * self.period * soundcard.delta_times)
        #     return np.sin(cycle_over_time)
        #   ampl_envelope = next(ampl_env)
        # except StopIteration:
        #   return False
        # num_samples = min(len(base_waveform), len(ampl_envelope))
        # np.multiply(base_waveform[:num_samples], ampl_envelope[:num_samples],
        #             out=block[:num_samples])
        # return num_samples == soundcard.block_size
        try:
            #print(f"populate_sound_block {self.waveform=}")
            base_waveform = next(self.waveform)
            #print("populate_sound_block got base_waveform")
        except StopIteration:
            #print(f"populate_sound_block: got StopIteration for note {self.note} "
            #      f"on waveform, returning", False)
            return False
        try:
            ampl_envelope = next(self.ampl_env)
            #print("populate_sound_block got ampl_envelope")
        except StopIteration:
            #print(f"populate_sound_block: got StopIteration for note {self.note} "
            #      f"on ampl_envelope, returning", False)
            return False
        num_samples = min(len(base_waveform), len(ampl_envelope))
        if num_samples == self.block_size:
            np.multiply(base_waveform, ampl_envelope, out=block)
            #print("populate_sound_block, num_samples", num_samples, True)
            return True
        #print("populate_sound_block, num_samples", num_samples, False)
        if num_samples > 0:
            np.multiply(base_waveform[:num_samples], ampl_envelope[:num_samples],
                        out=block[:num_samples])
        return False

