# master_requests.py

r'''Inherited from RAM Controller:


Commands from Master Controller:
    Fun_values/Hm_values commands:
        synth_prog: synth = 0, prog = 1

        $I<num_ch><num_ch_funs><num_enc><num_hm><num_hm_funs>: init, MUST BE CALLED BEFORE
                                                                     ANYTHING ELSE IS DONE
        $c<synth_prog><ch><ch_fun>: get_ch_values (report 1)
        $h<synth_prog><ch><hm><hm_fun>: get_hm_values (report 2)
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


# Fun_values[synth_prog][ch][ch_fun] -> byte string of Num_encoder values
# Hm_values[synth_prog][ch][hm][hm_fun] -> byte string of Num_encoder values

def I(line, master_out):
    r'''Init Fun_values and Hm_values

    args: num_ch, num_ch_funs, num_encoders, num_hm, num_hm_funs
    '''
    global Num_ch, Num_ch_funs, Num_encoders, Num_hm, Num_hm_funs
    global Fun_values, Hm_values
    Num_ch = line[2]
    Num_ch_funs = line[3]
    Num_encoders = line[4]
    Num_hm = line[5]
    Num_hm_funs = line[6]
    print(f"init: {Num_ch=}, {Num_ch_funs=}, {Num_encoders=}, {Num_hm=}, {Num_hm_funs=}") 
    Fun_values = [[[byte(Num_encoders)
                    for _ in range(Num_ch_funs)]    # ch_fun
                   for _ in range(Num_ch)]          # ch
                  for _ in range(2)]                # synth_prog
    Hm_values = [[[[byte(Num_encoders)
                    for _ in range(Num_hm_funs)]    # hm_fun
                   for _ in range(Num_hm)]          # hm
                  for _ in range(Num_ch)]           # ch
                 for _ in range(2)]                 # synth_prog

def c(line, master_out):
    r'''Return Fun_values (always Num_encoders bytes).

    args: synth_prog, ch_num, ch_fun_num
    '''
    master_out.write(Fun_values[line[2]][line[3]][line[4]])
    master_out.flush()

def h(line, master_out):
    r'''Return Hm_values (always Num_encoders bytes).

    args: synth_prog, ch_num, hm_num, hm_fun_num
    '''
    master_out.write(Hm_values[line[2]][line[3]][line[4]][line[5]])
    master_out.flush()

def C(line, master_out):
    r'''Set Fun_values (always Num_encoders bytes at a time).

    args: synth_prog, ch_bitmap (2 bytes), ch_fun_num, Num_encoder values
    '''
    synth_prog = line[2]
    ch_bitmap = line[3:5]
    ch_fun_num = line[5]
    values = line[6:-1]
    for ch in gen_bits(ch_bitmap, Num_ch):
        Fun_values[synth_prog][ch][ch_fun_name] = values

def H(line, master_out):
    r'''Set Hm_values (always Num_encoders bytes at a time).

    args: synth_prog, ch_bitmap (2 bytes), hm_bitmap (2 bytes), hm_fun_num, Num_encoder values
    '''
    synth_prog = line[2]
    ch_bitmap = line[3:5]
    hm_list = list(gen_bits(line[5:7], Num_hm))
    hm_fun_num = line[7]
    values = line[8:-1]
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

def P(line, master_out):
    r'''Copy synth values to prog.
    '''
    Fun_values[1] = deepcopy(Fun_values[0])
    Hm_values[1] = deepcopy(Hm_values[0])
