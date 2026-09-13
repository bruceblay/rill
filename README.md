# Rill

**rill** /rɪl/ *noun* — a small stream.

A pocket-sized generative instrument for the **M5Stack StickS3**. Rill composes delicate, evolving music and animated artwork entirely on the device. Tap for a new piece. Shake for a new visual.

<img src="docs/images/rill-scale.jpg" alt="Rill running on a StickS3 beside an AirPod for scale" width="640">

*Rill on the StickS3, with an AirPod for scale.*

Rill is an experimental instrument. It needs no Wi-Fi, account, audio files, or cloud service. Its sound has been developed around the StickS3's small built-in speaker.

## Play

| Gesture | Action |
| --- | --- |
| Front button: tap | Generate a new musical piece, change the visual, and play |
| Front button: hold for about 0.65 seconds | Fade sound out or in; the composition continues while quiet |
| Side button: tap | Cycle volume and show the data view for four seconds |
| Shake | Immediately switch to a different visual family and composition |

The data view shows the voice, key, mode, generation number, tempo, delay rhythm, volume and battery estimate. New musical generations also select a new visual. Shake changes only the visual. The gesture uses two acceleration peaks and a short cooldown; a single tilt is not a shake.

<img src="docs/images/rill-data.jpg" alt="Rill data view showing the voice, key, tempo, delay, volume and battery" width="640">

## What it makes

- **Seven timbres:** Bongo, Bars, Wood, Bells, Wire, Halo and Synth. These combine resonant modes, plucked tones, FM and filtered oscillators; they are interpretations, not exact hardware or acoustic-instrument emulations.
- **Generated phrases:** six contour tendencies guide newly composed melodies, variable phrase spans, interval preferences and rhythms. Ideas develop through changed endings, rhythmic rephrasing, recalled fragments and new descendants. Sparse answering parts follow their own timing. Twelve tonics, three modes, four harmonic behaviors and gradual changes in activity give each piece its own phrasing.
- **Evolving echoes:** two tempo-related taps, smooth or stepped feedback, occasional stronger repeat passages and intermittent smearing.
- **Six visual families:** elastic creatures, moving cutouts, a landscape ring sculpture, drifting particles, folding tiles and reflected rays. Each shake selects a different family. Particles, tiles and reflections develop new arrangements over time.

New music fades between generations. Generations are not saved across restarts. Device-to-device ensemble sync is a [design proposal](SYNC-DESIGN.md), not an available feature.

## On the device

Photos of all six visual families running on the StickS3. Shake to change the visual; tap the front button to generate new music and a new visual together.

| Elastic creatures | Moving cutouts |
| --- | --- |
| ![Elastic colored blobs](docs/images/rill-blobs.jpg) | ![Striped moving cutout](docs/images/rill-cutout.jpg) |
| **Ring sculpture** | **Drifting particles** |
| ![Landscape gradient ring sculpture](docs/images/rill-rings.jpg) | ![Colored particles gathering into groups](docs/images/rill-particles.jpg) |
| **Folding tiles** | **Reflected rays** |
| ![Colored folding tiles](docs/images/rill-tiles.jpg) | ![Colored reflected light rays](docs/images/rill-reflections.jpg) |

## Hardware

Supported and tested: **M5Stack StickS3**, with ESP32-S3, 8 MB flash, display, IMU and built-in speaker. Other ESP32 boards and earlier M5Stick models are not supported by this configuration.

The PlatformIO board name is `esp32-s3-devkitc-1`; the project supplies the StickS3 memory settings and uses M5Unified for board peripherals.

## Build and install

Install Python 3.11 or later, then run these commands from the repository root:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements-dev.txt
pio run
```

On Windows, activate with `.venv\Scripts\activate` instead. PlatformIO downloads the pinned platform and library dependencies on the first build.

Connect the StickS3 with a USB data cable, locate its port with `pio device list`, then install:

```sh
python tools/flash.py --port YOUR_DEVICE_PORT
```

Flashing replaces the firmware currently on the device. The script builds and uploads, then applies the watchdog reset used successfully during development; a normal RTS reset can leave this board in download mode.

To observe diagnostics:

```sh
pio device monitor --port YOUR_DEVICE_PORT --baud 115200
```

Close the monitor before another upload. If the device is not detected, check the cable and port permissions and consult the [StickS3 documentation](https://docs.m5stack.com/en/core/StickS3).

## Develop without hardware

Host tools use the same C++ synthesis and visual code as the firmware. A C++17 compiler is required.

```sh
python tools/test.py
mkdir -p build
c++ -std=c++17 -O2 tools/render.cpp -o build/render
build/render build/rill.wav 60 42 2
c++ -std=c++17 -O2 tools/visual_preview.cpp -o build/visual_preview
build/visual_preview build/preview.ppm 17 2
```

The audio arguments are output path, seconds, seed, and optional first voice (0–6). The visual arguments are output path, seed, and optional family (0–5). Audio output is mono 32 kHz / 16-bit WAV; visual output is PPM. For host address/undefined-behavior checks, run `python tools/test.py --sanitize` with a compatible compiler. Set `CXX` to choose a compiler.

Tests cover thirty simulated minutes of music, bounded output, key/register constraints, live transitions, reproducibility, all musical and visual families, shake detection, and retained historical behaviors. They do not replace listening or checking the physical screen.

## Project layout

- `src/Garden.h` — synthesis, score and effects
- `src/Light.h` — procedural visual families
- `src/main.cpp` — audio, display, buttons and motion tasks
- `src/ShakeDetector.h` — gesture recognition
- `tools/` — portable tests, auditions, previews and flashing
- `tests/` — host verification, with required historical baselines in `fixtures/`

See [publishing notes](docs/PUBLISHING.md), [changes](CHANGELOG.md), and [contributing](CONTRIBUTING.md).

## Credits and license

Created by Bruce Blay. Developed through iterative on-device listening and viewing, with Codex assisting implementation.

Brian Eno's generative work, Cyma Forma's RND synth, and Zach Lieberman's daily sketches helped inform the direction. Rill is an independent project, with no affiliation or endorsement implied. Device photos by Bruce Blay show Rill running on the hardware; no artwork by those artists is bundled.

Rill follows its parent project Pocket Radio's **GPL-3.0-or-later** license. See [LICENSE](LICENSE). Third-party components retain their own licenses; see [dependency notices](docs/DEPENDENCIES.md).
