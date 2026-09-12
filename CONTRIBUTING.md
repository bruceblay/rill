# Contributing

Rill is developed by listening and watching on the StickS3. Describe the musical or visual behavior a change improves, and include a seed when reporting a reproducible issue.

Run `python tools/test.py --sanitize` and `pio run` before proposing a change. Keep the audio path allocation-free and bounded. Visual randomization must not consume score RNG state. Preserve the ability to run both engines on a host computer.

For audible changes, test through the built-in speaker and describe what you heard. For visual changes, include output from the actual renderer and check the physical screen. Do not present a passing host test as proof of aesthetic quality or hardware performance.

The audio task produces 512 samples at 32 kHz, giving a 16 ms block deadline. Runtime diagnostics report worst render time, queue errors, free heap and visual time. Check these while exercising generation, volume, mute and shake controls.
