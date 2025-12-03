#!/usr/bin/env python

# Cake wrapper for compiler

import os
import pathlib
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

DEBUG_WRAPPER = os.getenv("DEBUG_CAKE_WRAPPER") is not None
CAKE = os.getenv("CAKE") or "cake"

# cake-wrapper.py test: skip cake
PASSTHROUGH = os.getenv("CAKE_WRAPPER_DISABLE_CAKE") is not None

def ok_to_include(s: str) -> bool:
    assert s.startswith("-I")
    s = s.removeprefix("-I")
    p = str(pathlib.Path(s).absolute())
    return p != "/usr/include"


def main() -> int:
    compiler = sys.argv[1]
    command = sys.argv[1:]
    pos_c = command.index("-c")
    if pos_c >= 0:
        assert pos_c >= 0
        command[pos_c + 0] = "-E"
        input_src = command[pos_c + 1]
        assert input_src.endswith(".c") or input_src.endswith(".cpp")
        preproc_src = Path(tempfile.gettempdir()) / Path(input_src).with_suffix(".preproc.c").name
        pos_o = command.index("-o")
        assert pos_o >= 0
        assert pos_o + 1 < len(command)
        output_file = command[pos_o + 1]
        command[pos_o + 1] = str(preproc_src)
        preproc_cmd = command + ["-P"]
        if DEBUG_WRAPPER:
            print(f"Running preproc: '{shlex.join(preproc_cmd)}'", file=sys.stderr)
        rc = subprocess.call(preproc_cmd)
        if rc:
            return rc

        if PASSTHROUGH:
            caked_src = preproc_src
        else:
            caked_src = Path(tempfile.gettempdir()) / Path(input_src).with_suffix(".cake.c").name
            cake_cmd = [CAKE, str(preproc_src), "-o", str(caked_src), "-test-mode"]
            if DEBUG_WRAPPER:
                print(f"Running cake: '{shlex.join(cake_cmd)}'", file=sys.stderr)
            rc = subprocess.call(cake_cmd)
            if rc:
                return rc

        caked_src = preproc_src

        command[pos_c + 0] = "-c"
        command[pos_c + 1] = str(caked_src)
        command[pos_o + 1] = output_file
        if DEBUG_WRAPPER:
            print(f"Running compiler: '{shlex.join(command)}'", file=sys.stderr)
        rc = subprocess.check_call(command)

        return rc
    else:
        if DEBUG_WRAPPER:
            print(f"Running linker: '{shlex.join(command)}'", file=sys.stderr)
        rc = subprocess.check_call(command)
        return c


if __name__ == "__main__":
    raise SystemExit(main())
