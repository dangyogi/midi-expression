# utils.py

import os.path

SRC_DIR = os.path.dirname(__file__)
PROJECT_DIR = os.path.dirname(SRC_DIR)


class Attrs:
    pass


if __name__ == "__main__":
    import utils
    print(f"{utils.SRC_DIR}")
    print(f"{utils.PROJECT_DIR}")
