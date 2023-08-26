# master_requests.py

r'''Inherited from RAM Controller:


Commands from Master Controller:
    Fun_values/Hm_values commands:
        synth_prog: synth = 0, prog = 1

        $I<num_ch><num_ch_funs><num_enc><num_hm><num_hm_funs>: init, MUST BE CALLED BEFORE
                                                                     ANYTHING ELSE IS DONE
        $c<synth_prog><ch><ch_fun>: get_ch_values (Num_encoder values)
        $h<synth_prog><ch><hm><hm_fun>: get_hm_values (Num_encoder values)
        $C<synth_prog><ch-bitmap><ch_fun><values>:
            set_ch_values, ch-bitmap is 2 bytes MSB first, values is Num_encoders bytes.
        $H<synth_prog><ch-bitmap><hm-bitmap><hm_fun><values>:
            set_hm_values, bitmaps are 2 bytes MSB first, values is Num_encoders bytes.
        $P: copy synth values to prog

    Other commands:
        Controller: M, P, or L

        $E<Controller><Errno><Err_data>: Report error
'''

from copy import deepcopy

import utils


# Fun_values[synth_prog][ch][ch_fun] -> byte string of Num_encoder values
# Hm_values[synth_prog][ch][hm][hm_fun] -> byte string of Num_encoder values

def I():
    r'''Init Fun_values and Hm_values

    args: num_ch, num_ch_funs, num_encoders, num_hm, num_hm_funs
    '''
    global Num_ch, Num_ch_funs, Num_encoders, Num_hm, Num_hm_funs
    global Fun_values, Hm_values
    line = utils.Master_in.read(5);
    assert len(line) == 5, f"master_command I: expected 5 bytes, got {len(line)}"
    Num_ch, Num_ch_funs, Num_encoders, Num_hm, Num_hm_funs = line
    print(f"init: {Num_ch=}, {Num_ch_funs=}, {Num_encoders=}, {Num_hm=}, {Num_hm_funs=}") 
    Fun_values = [[[bytes(Num_encoders)
                    for _ in range(Num_ch_funs)]    # ch_fun
                   for _ in range(Num_ch)]          # ch
                  for _ in range(2)]                # synth_prog
    Hm_values = [[[[bytes(Num_encoders)
                    for _ in range(Num_hm_funs)]    # hm_fun
                   for _ in range(Num_hm)]          # hm
                  for _ in range(Num_ch)]           # ch
                 for _ in range(2)]                 # synth_prog

def c():
    r'''Return Fun_values (always Num_encoders bytes).

    args: synth_prog, ch_num, ch_fun_num
    '''
    line = utils.Master_in.read(3);
    assert len(line) == 3, f"master_command c: expected 3 bytes, got {len(line)}"
    master_out.write(Fun_values[line[0]][line[1]][line[2]])
    master_out.flush()

def h():
    r'''Return Hm_values (always Num_encoders bytes).

    args: synth_prog, ch_num, hm_num, hm_fun_num
    '''
    line = utils.Master_in.read(4);
    assert len(line) == 4, f"master_command h: expected 4 bytes, got {len(line)}"
    master_out.write(Hm_values[line[0]][line[1]][line[2]][line[3]])
    master_out.flush()

def C():
    r'''Set Fun_values (always Num_encoders bytes at a time).

    args: synth_prog, ch_bitmap (2 bytes), ch_fun_num, Num_encoder values
    '''
    line = utils.Master_in.read(4 + Num_encoders);
    assert len(line) == 4 + Num_encoders, \
      f"master_command C: expected {4 + Num_encoders} bytes, got {len(line)}"
    synth_prog = line[0]
    ch_bitmap = line[1:3]
    ch_fun_num = line[3]
    values = line[4:]
    assert len(values) == Num_encoders
    for ch in gen_bits(ch_bitmap, Num_ch):
        Fun_values[synth_prog][ch][ch_fun_name] = values

def H():
    r'''Set Hm_values (always Num_encoders bytes at a time).

    args: synth_prog, ch_bitmap (2 bytes), hm_bitmap (2 bytes), hm_fun_num, Num_encoder values
    '''
    line = utils.Master_in.read(6 + Num_encoders);
    assert len(line) == 6 + Num_encoders, \
      f"master_command H: expected {6 + Num_encoders} bytes, got {len(line)}"
    synth_prog = line[0]
    ch_bitmap = line[1:3]
    hm_list = list(gen_bits(line[3:5], Num_hm))
    hm_fun_num = line[6]
    values = line[7:]
    assert len(values) == Num_encoders
    for ch in gen_bits(ch_bitmap, Num_ch):
        for hm in hm_list:
            Hm_values[synth_prog][ch][hm][hm_fun_name] = values

def gen_bits(bitmap, max_number):
    for i in range(max_number):
        if i >= 8:
            if bitmap[0] & (1 << (i - 8)):
                yield i
        else:
            if bitmap[1] & (1 << i):
                yield i

def P():
    r'''Copy synth values to prog.
    '''
    Fun_values[1] = deepcopy(Fun_values[0])
    Hm_values[1] = deepcopy(Hm_values[0])

def E():
    r'''Report error.

    $E<Controller><Errno><Err_data>: Report error
    '''
    line = utils.Master_in.read(3);
    print(f"report error {line=!r}")
    assert len(line) == 3, f"master_command E: expected 3 bytes, got {len(line)}"
    controller = utils.Controllers1[chr(line[0])]
    error_info = controller.errnos[line[1]]
    print(f"ERROR {controller.name} Controller, {error_info.Err_data}={line[2]}: {error_info.desc}")

