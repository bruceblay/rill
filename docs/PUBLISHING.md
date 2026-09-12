# Publishing Rill

Release 0.2.0 is the deployment candidate for the developing-phrase sequencer. Store copy is in `release/m5burner-listing.md`; installation and source archive details are in `release/README.md`. Publication status is recorded below once verified.

## Source repository

- README covers supported hardware, controls, build, flashing and host tools.
- Dependencies are pinned; CI builds firmware and runs host tests.
- Preview images come from the actual renderer.
- GPL license and direct dependency notices are included.
- Local notes, caches, virtual environments and build outputs are ignored.

## First downloadable firmware release

1. Choose a release version and tag the exact verified source commit.
2. Build that commit with the pinned dependencies; retain the build log and dependency versions.
3. Package the application, bootloader, partition table and OTA initialization image with the verified flash offsets. Prefer build outputs to a dump of a configured device.
4. Include source, exact dependency build inputs and applicable upstream notices/corresponding-source materials. Review linked SDK/font components before claiming the bundle is complete.
5. Produce SHA-256 checksums and concise release notes describing hardware support and installation.
6. Test the packaged image on a StickS3, including recovery/reset, all controls, six visual families, sound, battery display and sustained audio timing. Existing development flashes are evidence for the code, not verification of a future release bundle.
7. Create a draft release, then publish after reviewing those exact artifacts.

M5Burner distribution is a separate step: prepare a real device cover photo and listing, confirm the uploader's current image format, and test installation through M5Burner before publishing there.

## Release status — 2026-09-12

- [Rill 0.1.0](https://github.com/bruceblay/rill/releases/tag/v0.1.0) is public, tagged at `acb96c5e15b4ae42beca8e7195472b40c4e18141`.
- Includes the expanded volume range approved after on-device listening. The startup level remains 64%.
- Firmware build and release-commit CI passed; all release checksums verified locally.
- M5Burner upload succeeded on September 12, 2026. Rill 0.1.0 is marked **Pending Public** (awaiting review), under StickS3 with Audio & Media and Display & Art categories. The selected cover is the six-visual montage (`docs/images/visuals.png`).

## 0.2.0 deployment preparation

The new sequencer passed all six host suites with address/undefined-behavior checks, including 256 initial contours and four sustained four-minute development runs. Firmware was flashed and approved through on-device listening. Device diagnostics recorded zero audio queue errors and 4.279 ms maximum rendering time per 16 ms audio block.

Release files are built with `python tools/package_release.py` (version from `VERSION`). Packaging requires a clean commit and rebuilds before merging. Add the tagged source archive and dependency/source archives described in `release/README.md`, then regenerate SHA256SUMS. Use the factory image at offset 0x0 for M5Burner. Submit as a new version of the existing Rill listing.

## 0.2.0 upload status — 2026-09-12

- GitHub release 0.2.0 is public: https://github.com/bruceblay/rill/releases/tag/v0.2.0.
- M5Burner rejected the new-version upload with `新增版本不能修改固件说明，请使用固件编辑接口` (new versions cannot modify the firmware description; use the firmware editor).
- Matching the existing editor text and saving the desired description through the separate shared-data editor did not resolve the rejection. A temporary withdrawal of 0.1.0 was also ineffective; its Pending Public review status was restored and verified.
- The shared description now documents the developing sequencer. The original visual montage cover was retained, as requested.
- 0.2.0 has NOT been accepted by M5Burner. The Chrome upload form retains its firmware attachment and version notes for follow-up. Existing firmware id: 2098855058485760002.
