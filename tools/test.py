#!/usr/bin/env python3
# Copyright (c) 2026 Bruce Blay
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile and run portable tests from any working directory."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    compiler = shlex.split(os.environ.get('CXX', 'c++'))
    flags = ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-I', str(root / 'src')]
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    with tempfile.TemporaryDirectory(prefix='rill-tests-') as directory:
        for source in sorted((root / 'tests').glob('*_test.cpp')):
            target = Path(directory) / source.stem
            print(f'Testing {source.name}', flush=True)
            subprocess.run(compiler + flags + [str(source), '-o', str(target)], check=True)
            subprocess.run([str(target)], check=True)


if __name__ == '__main__':
    main()
