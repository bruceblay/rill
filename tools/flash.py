#!/usr/bin/env python3
# Copyright (c) 2026 Bruce Blay
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build, upload and restart Rill on an explicitly selected StickS3 port."""
import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True, help='Serial port from pio device list')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    subprocess.run([sys.executable, '-m', 'platformio', 'run', '-d', str(root),
                    '-t', 'upload', '--upload-port', args.port], check=True)
    core = Path(os.environ.get('PLATFORMIO_CORE_DIR', Path.home() / '.platformio'))
    esptool = core / 'packages/tool-esptoolpy/esptool.py'
    if not esptool.is_file():
        raise SystemExit('Upload completed, but esptool was not found for the final reset. '
                         'Restart the device manually or set PLATFORMIO_CORE_DIR.')
    subprocess.run([sys.executable, str(esptool), '--chip', 'esp32s3', '--port', args.port,
                    '--after', 'watchdog_reset', 'chip_id'], check=True)


if __name__ == '__main__':
    main()
