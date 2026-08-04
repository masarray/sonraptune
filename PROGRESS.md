# SonRapTune implementation checklist

Updated: 2026-08-04

## Repository separation

- [x] Dedicated `masarray/sonraptune` repository
- [x] Source placed at repository root, not under `askp-vst`
- [x] Production `askp-vst` remains untouched
- [x] P0 source, TDD, and validation results migrated

## E0 — Foundation

- [x] Independent C++17/CMake project
- [x] Pinned JUCE 8.0.14
- [x] VST3 and Standalone targets
- [x] MIDI input and fixed 128-note state
- [x] Mono/stereo host bus compatibility
- [x] APVTS project state `SONRAPTUNE_STATE`
- [x] Lock-free lossy pitch telemetry
- [x] Host-reported shifter latency
- [ ] Final custom user interface

## E1 — Analysis

- [x] Full-rate input to approximately 12 kHz detector decimation
- [x] Preallocated circular history
- [x] YIN-derived CMNDF and four candidates
- [x] Parabolic interpolation
- [x] Causal four-beam tracker
- [x] Octave-jump and voiced-transition penalties
- [x] Synthetic 70–880 Hz benchmark: median 0.13 cent, gross error 0%
- [ ] Spectral flatness, ZCR, HF ratio, and adaptive noise floor
- [ ] Recorded male/female rap dataset
- [ ] Flutter comparison against nearest-note baseline

## E2 — Mapping and trajectory

- [x] Major, Natural Minor, Chromatic, and custom masks
- [x] Key-relative scale mapping
- [x] Stability hysteresis and note commitment
- [x] Nearest active MIDI note
- [x] Tune, Speed, Stability, and Feel trajectory
- [x] Sample-by-sample correction ratio
- [ ] End-note lock and phrase-aware release
- [ ] Calibrated Natural, Modern Rap, Trap Lock, and Hook curves
- [ ] Sample-offset MIDI event segmentation

## E3 — Time-domain pitch-shifter candidates

### Candidate A — fixed-duration baseline

- [x] Causal dual-read-head granular shifter
- [x] Fixed latency: 20 ms grain + 6 ms base delay
- [x] Latency-aligned dry and internal bypass
- [x] Voiced wet-mask crossfade
- [x] Deterministic one-semitone frequency smoke test
- [x] Finite and bounded output test

Candidate A remains compiled and tested as the non-period-synchronous baseline.

### Candidate B — period-synchronous overlap-add

- [x] Detector-guided causal waveform pitch-mark estimator
- [x] Positive-peak selection constrained to the tracked source period
- [x] Period-synchronous synthesis-mark scheduler
- [x] Two-period Hann grain extraction
- [x] Normalised overlap-add for mono/stereo
- [x] Ratio automation from the correction trajectory
- [x] Fixed host latency: maximum 55 Hz period plus 8 ms scheduling margin
- [x] Latency-aligned dry and internal bypass
- [x] Unity-ratio aligned-dry fast path
- [x] Fixed-capacity 128-mark history
- [x] No-allocation callback contract across 44.1/48/96 kHz and 32–1024 samples
- [x] Pitch-mark period test: 220 Hz error below 1 sample
- [x] Synthetic shift matrix: 36/36 cases across 44.1/48/96 kHz, 90–440 Hz, ±1/±2 semitones
- [x] Local matrix maximum absolute frequency error approximately 7.8 cents
- [ ] Recorded-vocal quality and consonant tests
- [ ] Plosive, sibilant, and onset transition handling
- [ ] Latency/quality optimisation
- [ ] Blind listening comparison against Candidate A

Candidate B is now the active end-to-end engineering engine. Passing synthetic frequency tests validates causal scheduling and pitch movement, but does not prove production vocal quality.

## E4 — Phase-locked STFT candidate

- [ ] STFT analysis/synthesis
- [ ] Spectral peak detection
- [ ] Identity phase locking
- [ ] Ratio automation
- [ ] Transient protection
- [ ] Fixed latency and quality/CPU tests

## E5 — Formant and rap protection

- [ ] No-preserve baseline
- [ ] LPC formant candidate
- [ ] Cepstral formant candidate
- [ ] Consonant/onset mask
- [ ] Latency-aligned unvoiced reintegration

## Cross-platform local build

- [x] Windows single-click `build-windows.bat`
- [x] Windows VST3 + Standalone ZIP
- [x] Windows Inno Setup EXE installer
- [x] Visual Studio 2026/2022 automatic generator detection
- [x] macOS `build-macos.command`
- [x] macOS Universal VST3 + Standalone ZIP
- [x] macOS PKG installer with ad-hoc local signing
- [x] Linux `build-linux.sh`
- [x] Linux VST3 + Standalone tar.gz
- [x] Linux DEB installer
- [ ] Developer ID signing and notarisation for public macOS distribution
- [ ] Windows Authenticode signing

## Automation

- [x] Build/test workflow for Windows, macOS, and Linux
- [x] Package upload as workflow artifacts
- [x] Tag-driven `v*` release workflow
- [x] Manual release dispatch with tag input
- [x] GitHub Release publishing via official `gh` CLI
- [x] First successful three-platform CI run — run 15, commit `3f52fce`
- [x] Windows, macOS, and Linux package artifacts verified
- [ ] Candidate B successful three-platform CI run
- [ ] First published signed production release

## E6 — Bake-off and gate

- [ ] Batch render matrix
- [ ] CSV/JSON metric reports
- [ ] Blind listening renders
- [ ] Full callback deadline benchmark under host-like load
- [ ] GO / ITERATE / STOP decision

## Current milestone

**Reached:** E0 complete, E1 synthetic correctness gate passed, E2 foundation active, Candidate A retained as baseline, and Candidate B period-synchronous overlap-add active in the engine.

**Locally validated:** Candidate B pitch marks track a 220 Hz waveform within one sample. Its 36-case synthetic shift matrix passed across three sample rates, three source pitches, and four correction ratios with maximum absolute error around 7.8 cents.

**Pending gate:** Windows x64, macOS Universal, and Linux x64 must all compile VST3/Standalone, pass five CTest targets, produce installers, and upload artifacts before Candidate B is merged.

**Current release status:** engineering alpha suitable for controlled local listening tests after CI passes. Formant preservation, consonant/onset reintegration, recorded-vocal evaluation, and E4 phase-locked STFT remain open before a production audio-quality claim.
