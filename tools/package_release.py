#!/usr/bin/env python3
# Copyright (C) 2026 Bruce Blay
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package the current built StickS3 firmware without reading device flash."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
version = sys.argv[1] if len(sys.argv) > 1 else '0.1.0'
if not version or any(c not in '0123456789.-abcdefghijklmnopqrstuvwxyz' for c in version):
    raise SystemExit('Invalid version')
out = root / 'dist' / ('rill-' + version)
out.mkdir(parents=True, exist_ok=False)
core = Path.home() / '.platformio'
framework = core / 'packages/framework-arduinoespressif32'
build = root / '.pio/build/sticks3'
segments = [(0, build / 'bootloader.bin'), (0x8000, build / 'partitions.bin'),
            (0xe000, framework / 'tools/partitions/boot_app0.bin'), (0x10000, build / 'firmware.bin')]
image = out / f'rill-{version}-factory.bin'
args = [sys.executable, str(core / 'packages/tool-esptoolpy/esptool.py'), '--chip', 'esp32s3',
        'merge_bin', '-o', str(image), '--flash_mode', 'dio', '--flash_freq', '80m',
        '--flash_size', '8MB', '--fill-flash-size', '8MB']
for offset, path in segments:
    args.extend([hex(offset), str(path)])
subprocess.run(args, check=True)
data = image.read_bytes()
assert len(data) == 8 * 1024 * 1024
assert data[0x9000:0xe000] == b'\xff' * 0x5000
assert (build / 'firmware.bin').stat().st_size <= 0x330000
for offset, path in segments[1:]:
    assert data[offset:offset + path.stat().st_size] == path.read_bytes()
shutil.copy2(build / 'firmware.bin', out / f'rill-{version}-app.bin')
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
manifest = {'version': version, 'device': 'M5Stack StickS3', 'factory_offset': '0x0',
            'factory_size': len(data), 'blank_nvs_verified': True,
            'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
            'segments': [{'offset': hex(o), 'file': p.name, 'sha256': sha(p)} for o, p in segments]}
(out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
(out / 'SHA256SUMS').write_text(''.join(f'{sha(p)}  {p.name}\n' for p in sorted(out.iterdir()) if p.is_file()))
print(out)
