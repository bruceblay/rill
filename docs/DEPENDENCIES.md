# Dependency notices

Original Still code is copyright 2026 Bruce Blay, licensed under GPL version 3 or, at your option, any later version. The shake detector originated in Bruce's GPL-3.0-or-later Pocket Radio project. Other source in this repository was developed for Still; historical copies are retained under `studies/`.

| Build input | Pinned version | License/source reference |
| --- | --- | --- |
| PlatformIO Core | 6.2.0 | Build tool, specified in requirements-dev.txt |
| PlatformIO Espressif 32 platform | 6.12.0 | [Platform source](https://github.com/platformio/platform-espressif32/tree/v6.12.0) |
| Arduino ESP32 framework | 2.0.17, resolved by the platform | [Framework source and component notices](https://github.com/espressif/arduino-esp32/tree/2.0.17); core uses LGPL-2.1-or-later |
| M5Unified | 0.2.21 | [Source](https://github.com/m5stack/M5Unified/tree/0.2.21); [MIT notice](../licenses/M5Unified-MIT.txt) |
| M5GFX | 0.2.28 | [Source](https://github.com/m5stack/M5GFX/tree/0.2.28); [MIT notice](../licenses/M5GFX-MIT.txt) |

Dependencies are downloaded by PlatformIO and are not vendored into this source repository. Bundled SDK components, fonts and other upstream material retain their individual notices. This inventory identifies direct inputs; it is not a complete inventory of every statically linked SDK component.

For a downloadable firmware release, include the exact dependency inputs and applicable notices and corresponding-source materials alongside the source for the tagged build. A link to this repository alone does not include all dependency source. See PUBLISHING.md.
