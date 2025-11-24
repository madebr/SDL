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

def ok_to_include(s: str) -> bool:
    assert s.startswith("-I")
    s = s.removeprefix("-I")
    p = str(pathlib.Path(s).absolute())
    return p != "/usr/include"


def main() -> int:
    compiler_cmd = []
    cake_args = []
    i = 0
    original_src = None
    caked_rc = None
    dep_path = None
    while i < len(sys.argv):
        consumed = 1
        arg = sys.argv[i]
        if arg.startswith("-I"):
            if len(arg) > 2:
                inc_arg = arg
            else:
                consumed = 2
                inc_arg = f"-I{sys.argv[i + 1]}"
            if ok_to_include(inc_arg):
                cake_args.append(inc_arg)
        elif arg.startswith("-isystem"):
            # cake does not support -isystem
            if len(arg) > 8:
                inc_arg = f"-I{arg[8:]}"
            else:
                consumed = 2
                inc_arg = f"-I{sys.argv[i + 1]}"
            if ok_to_include(inc_arg):
                cake_args.append(inc_arg)
        elif arg.startswith("-D"):
            cake_args.append(arg)
        elif arg.startswith("-U"):
            cake_args.append(arg)
        elif arg.startswith("-MF"):
            if len(arg) > 3:
                dep_path = Path(arg[2:])
            else:
                consumed = 2
                dep_path = Path(sys.argv[i + 1])
        i += consumed
    cake_args.extend([
        "-I/usr/lib/gcc/x86_64-redhat-linux/15/include",
        "-I/usr/local/include",
        "-I/usr/include",
        "-target=x86_x64_gcc",
    ])
    for arg in sys.argv[1:]:
        if arg.endswith(".c"):
            original_src = Path(arg)
            caked_src = Path(tempfile.gettempdir()) / original_src.with_suffix(".cake.c").name
            cake_cmd = [CAKE] + cake_args + [f"-I{original_src.parent}", str(original_src), "-o", str(caked_src), "-test-mode"]
            compiler_cmd.append(f"-I{caked_src.parent}")
            if DEBUG_WRAPPER:
                print(f"Running cake: '{shlex.join(cake_cmd)}'", file=sys.stderr)
            result = subprocess.call(cake_cmd)
            if DEBUG_WRAPPER:
                print(f"cake exit code: {result}", file=sys.stderr)
                sys.stderr.flush()
            if result != 0:
                print(f"Cake failed: {shlex.join(cake_cmd)}", file=sys.stderr)
                sys.stderr.flush()
                return result
            arg = str(caked_src)
        compiler_cmd.append(arg)

    if DEBUG_WRAPPER:
        print(f"Running compiler: '{shlex.join(compiler_cmd)}'", file=sys.stderr)
        sys.stderr.flush()
    rc = subprocess.call(compiler_cmd)
    if dep_path and dep_path.is_file():
        with dep_path.open("r") as f:
            original_dep = f.read()
        new_dep = original_dep.replace(str(caked_src), str(original_src))
        with dep_path.open("w") as f:
            f.write(new_dep)

    return rc


if __name__ == "__main__":
    raise SystemExit(main())
