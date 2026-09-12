# Publishing Rill

Release 0.1.0 packages the current StickS3 instrument. Store copy is in `release/m5burner-listing.md`; installation and source archive details are in `release/README.md`. Publication status is recorded below once verified.

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
- M5Burner submission is being prepared in the web developer portal; public listings require review.
