# Rill 0.1.0

For M5Stack StickS3 only. Flash `rill-0.1.0-factory.bin` at **0x0**. The full 8 MB image includes the bootloader, partition table, OTA initialization and application, and clears existing flash contents/settings. The application-only image belongs at 0x10000 with the matching partition layout.

Tap the front button for new music and a new visual; hold to fade out/in. Tap the side button to cycle volume and briefly show information. Shake for a new visual.

## Source and rebuilding

The release includes:

- `rill-0.1.0-source.tar.gz`: tagged Rill source and build/package scripts.
- `rill-0.1.0-dependency-inputs.tar.gz`: installed M5Unified 0.2.21, M5GFX 0.2.28, Arduino ESP32 2.0.17 framework and PlatformIO Espressif 32 6.12.0 platform, with bundled notices intact.
- `rill-0.1.0-esp-idf-source.tar.gz`: ESP-IDF v4.4.7 (38eeba213aa695aabfd6d89aa9f5078dbe5a94c3) with pinned recursive submodules and notices, excluding Git metadata.

Install requirements-dev.txt in a Python virtual environment and run `pio run` from the source directory. PlatformIO downloads the pinned dependencies and compiler tools. To use archived library sources, copy M5Unified and M5GFX into the project's `lib/` directory. The archived framework and platform preserve the installed build inputs; the framework includes vendor prebuilt SDK libraries. The ESP-IDF archive provides upstream source, not a claim of bit-for-bit SDK reproduction. Build paths and environment may affect binary output.

Run `python tools/test.py --sanitize` for host checks. After building, `python tools/package_release.py 0.1.0` creates merged firmware and its manifest. No signing key or device credential is required to build or install modified firmware through the USB bootloader.

Original code is GPL-3.0-or-later. Dependencies retain their own notices; see docs/DEPENDENCIES.md and the archives. SHA256SUMS covers release files.
