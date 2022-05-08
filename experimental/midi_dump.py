# midi_dump.py

r'''Dumps midi file.
'''

def dump(filename):
    with open(filename, 'rb') as f:
        dumpf(f)

def read2(f):
    temp = f.read(2)
    ans = temp[0] * 256 + temp[1]
    #print("read2", ans)
    return ans

def read4(f):
    ms2 = read2(f)
    ans = (ms2 << 16) + read2(f)
    #print("read4:", ms2, ans)
    return ans

def readv(f):
    r'''Returns vlen, value.
    '''
    ans = 0
    b = f.read(1)[0]
    vlen = 1
    while b & 0x80:
        ans <<= 7
        ans |= b & 0x7F
        #print("readv:", "read", b, "length", ans)
        b = f.read(1)[0]
        vlen += 1
    ans <<= 7
    ans |= b
    #print("readv:", "read", b, "length", ans)
    #print("readv:", "vlen", vlen, "length", ans)
    return vlen, ans

def dumpf(f):
    chunk_id = f.read(4)
    assert chunk_id == b"MThd", f"expected 'MThd', got {chunk_id} at offset 0x{offset:x}"
    vtime = 0
    vtime, length = Formats[chunk_id](f, vtime, 0)
    offset = length + 4
    while True:
        chunk_id = f.read(4)
        assert len(chunk_id) in (0, 4), \
               f"got EOF reading chunk ID: got {chunk_id} at offset 0x{offset:x}"
        if not chunk_id:
            break
        vtime, length = Formats[chunk_id](f, vtime, offset)
        offset += length + 4


def dump_MThd(f, vtime, offset):
    r'''Returns final vtime, length of chunk.
    '''
    length = read4(f)
    assert length >= 6, f"MThd: expected length >= 6, got {length} at offset 0x{offset:x}"
    print(f"Header Chunk at offset 0x{offset:x}")
    offset += 4
    format = read2(f)
    print("    format:", {0: "single track file format",
                           1: "multiple track file format",
                           2: "multiple song file format"}[format])
    offset += 2
    print("    tracks:", read2(f))
    offset += 2
    print("    division:", read2(f))  # if bit 15 is set:
                                      #     the first byte is: -SMPTE format
                                      #     the second byte is: ticks/frame
                                      # else:
                                      #     the whole number is the vtime
                                      #     ticks/quarter-note
    offset += 2
    if length > 6:
        f.read(length - 6)
    return vtime, length + 4


def dump_MTrk(f, vtime, offset):
    r'''Returns final vtime, length
    '''
    length = remaining = read4(f)
    print(f"Track Chunk: length {length} at offset 0x{offset:x}")
    offset += 8
    last_status = None
    while remaining > 0:
        vlen, vtime_delta = readv(f)
        vtime += vtime_delta
        #print(f"    Track element: remaining length {remaining} at offset 0x{offset:x}")
        last_status, element_length = dump_track_element(f, vtime, offset, last_status)
        remaining -= vlen + element_length
        offset += vlen + element_length
    assert remaining == 0, f"expected 0 remaining, got {remaining} at offset 0x{offset:x}"
    return vtime, length


def dump_track_element(f, vtime, offset, last_status):
    r'''Returns last_status, element_length.
    '''
    status_byte = f.read(1)[0]
    added_len = 1
    if status_byte == 0xff:
        return None, dump_meta_event(f, vtime, offset) + added_len
    if status_byte == 0xf0 or status_byte == 0xf7:
        return None, dump_sysex_event(f, vtime, status_byte, offset) + added_len
    assert status_byte < 0xF0, \
           f"got unknown status byte, 0x{status_byte:x} at offset 0x{offset:x}"
    last_status, length = dump_midi_event(f, vtime, status_byte, offset, last_status)
    return last_status, length + added_len


def dump_meta_event(f, vtime, offset):
    meta_type = f.read(1)[0]
    vlen, length = readv(f)
    event = {0x00: "Sequence number",
             0x01: "Text event",              # text
             0x02: "Copyright notice",        # text
             0x03: "Sequence or track name",  # text
             0x04: "Instrument name",         # text
             0x05: "Lyric text",              # text
             0x06: "Marker text",             # text
             0x07: "Cue point",               # text
             0x20: "MIDI channel prefix assignment",
             0x2F: "End of track",
             0x51: "Tempo setting",  # 1 3-byte param: uSec/quarter-note (24 MIDI clocks)
             0x54: "SMPTE offset",
             0x58: "Time signature", # 4 params: nn dd cc bb
                                     # time sig is nn/2**dd
                                     #    (e.g., 6/8 time is nn of 6, dd of 3)
                                     # cc is num of MIDI clocks in a metronome click.
                                     # bb is num of notated 32nd notes in
                                     #    a MIDI quarter-note (24 MIDI clocks)
             0x59: "Key signature",  # 2 params: sf, mi
                                     # sf is #sharps or flats
                                     #    (+for sharps, -for flats), so -7 to 7
                                     # mi is minor key flag
                                     #    (1 for minor key, 0 for major key)
             0x7F: "Sequencer specific event",
            }[meta_type]
    if length == 0:
        print(f"@{vtime}-{event}.")
    elif meta_type == 0x51:
        assert length == 3, \
               f"expected length 3 for Tempo setting, got {length} at offset 0x{offset:x}"
        b1 = f.read(1)[0] << 16
        print(f"@{vtime}-{event}:", b1 + read2(f))
    else:
        print(f"@{vtime}-{event}:", f.read(length))
    return 1 + vlen + length

def dump_sysex_event(f, vtime, status_byte, offset):
    vlen, length = readv(f)
    if status_byte == 0xF0:
        data = bytes((0xF0,)) + f.read(length)
    else:
        data = f.read(length)
    print(f"@{vtime}-SysEx {status_byte:x} event at offset 0x{offset:x}:", data)
    return vlen + length

def dump_midi_event(f, vtime, status_byte, offset, last_status):
    r'''Returns last_status, length.

    Returned length does not include status_byte.
    '''
    length = 0
    if status_byte & 0x80:
        p1 = f.read(1)[0]
        length += 1
    else:
        p1 = status_byte
        status_byte = last_status
    #print(f"MIDI event: 0x{status_byte:x} at offset 0x{offset:x}")
    assert status_byte & 0x80, \
           "expected high bit set in status_byte, " \
           f"got 0x{status_byte:x} at offset 0x{offset:x}"
    assert status_byte < 0xF0, \
           "expected channel voice status_byte, " \
           f"got 0x{status_byte:x} at offset 0x{offset:x}"
    command, num_params = {
        0x80: ("Note Off", 2),
        0x90: ("Note On", 2),
        0xA0: ("Aftertouch", 2),
        0xB0: ("Control Change", 2),
        0xC0: ("Patch change", 1),
        0xD0: ("Channel Pressure", 1),  # Similiar to Aftertouch.
                                        # Single greatest pressure value
                                        # of all depressed keys.  I guess this
                                        # scales the volume of all depressed
                                        # keys based on their percentage of 
                                        # the greatest value?  Also sounds like
                                        # it's of limited duration (only the 
                                        # duration of depressed keys).
        0xE0: ("Pitch Bend", 2),
    }[status_byte & 0xF0]
    if num_params == 1:
        params = (hex(p1),)
    else:
        params = (hex(p1), hex(f.read(1)[0]))
        length += 1
    channel = status_byte & 0x0F
    print(f"@{vtime}, ch: {channel}, {command}:", ', '.join(params))
    return status_byte, length


Formats = {
    b"MThd": dump_MThd,
    b"MTrk": dump_MTrk,
}


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("file")

    #print(dir(parser))
    args = parser.parse_args()
    
    dump(args.file)
