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

- [x] Candidate A: causal dual-read-head granular shifter
- [x] Fixed latency: 20 ms grain + 6 ms base delay
- [x] Latency-aligned dry and internal bypass
- [x] Voiced wet-mask crossfade
- [x] Ratio automation from correction trajectory
- [x] Deterministic one-semitone frequency smoke test
- [x] Finite and bounded output test
- [x] No-allocation callback contract across 44.1/48/96 kHz and 32–1024 samples
- [ ] True pitch-mark estimator
- [ ] Period-synchronous grain extraction
- [ ] Recorded-vocal quality and consonant tests
- [ ] Latency/quality optimisation

Candidate A makes tuning audible for engineering tests, but is not yet the final TD-PSOLA product engine.

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
- [ ] First published signed production release

## E6 — Bake-off and gate

- [ ] Batch render matrix
- [ ] CSV/JSON metric reports
- [ ] Blind listening renders
- [ ] Full callback deadline benchmark under host-like load
- [ ] GO / ITERATE / STOP decision

## Current milestone

**Reached:** E0 complete, E1 synthetic correctness gate passed, E2 foundation active, and E3 Candidate A audible with fixed reported latency. Single-click local builds and cross-platform release automation are operational.

**Validated:** Windows x64, macOS Universal, and Linux x64 all compiled VST3/Standalone, passed detector/core/realtime/shifter tests, produced installers and portable packages, and uploaded artifacts in GitHub Actions run 15.

**Current release status:** engineering alpha suitable for local listening tests. Formant preservation, true period-synchronous TD-PSOLA, recorded-vocal evaluation, and E4 phase-locked STFT remain open before a production audio-quality claim.
