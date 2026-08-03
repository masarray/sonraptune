# P0 validation summary

Date: 2026-08-03

## Passed

- Core C++17 library configures and compiles with GCC 14.2.
- Scale/MIDI mapping and correction trajectory smoke test passes.
- Realtime-contract smoke produces finite output at 44.1, 48, and 96 kHz with block sizes 32, 64, 128, 256, 512, and 1024.
- Synthetic detector benchmark from 70 to 880 Hz: median fine error 0.13 cent; gross error 0%.
- Octave/subharmonic ranking defect found in the first run and fixed by prioritizing the first reliable CMNDF period.

## Not yet passed

- Plugin binary was not compiled in this sandbox because JUCE FetchContent could not resolve github.com. The plugin configuration and source are present, but Windows/CI compilation remains an open gate.
- The correctness benchmark processed 8.25 seconds of synthetic material in 1.84 seconds on one container core. This is faster than real time but far above the final per-instance CPU target, so detector optimization remains mandatory.
- No audible pitch shifter is active. Output remains passthrough by design until E3/E4 bake-off.
