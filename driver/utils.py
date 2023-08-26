# utils.py

import os.path
import codecs


SRC_DIR = os.path.dirname(__file__)
PROJECT_DIR = os.path.dirname(SRC_DIR)


class Attrs:
    pass


Ascii_encoder = codecs.getencoder('ascii')
Ascii_decoder = codecs.getdecoder('ascii')

def to_bytes(str):
    return Ascii_encoder(str)[0]

def to_str(bytes):
    return Ascii_decoder(bytes)[0]


if __name__ == "__main__":
    import utils
    print(f"{utils.SRC_DIR}")
    print(f"{utils.PROJECT_DIR}")
