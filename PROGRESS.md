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
- [x] Confidence used as a hysteretic voiced-state decision, not continuous dry/wet amount
- [x] Dedicated voiced-path attack/release crossfade
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
- [x] Safe fixed host latency: two maximum 55 Hz periods plus 8 ms scheduling margin
- [x] Latency-aligned dry and internal bypass
- [x] Generation-tagged OLA slots prevent stale ring-buffer tails
- [x] Realtime guard rejects writes to already-consumed output samples
- [x] Continuous OLA coverage fade instead of hard dry/OLA switching
- [x] Four-millisecond scheduler-ratio smoothing
- [x] Smooth cents-domain unity transition with an 8 ms crossfade
- [x] Fixed-capacity 128-mark history
- [x] No-allocation callback contract across 44.1/48/96 kHz and 32–1024 samples
- [x] Pitch-mark period test: 220 Hz error below 1 sample
- [x] Synthetic shift matrix: 36/36 cases across 44.1/48/96 kHz, 90–440 Hz, ±1/±2 semitones
- [x] Local matrix maximum absolute frequency error approximately 7.8 cents
- [x] Harmonic quasi-vocal crackle regression with glide and voiced/unvoiced transitions
- [x] Deterministic five-sample unity-crossing dry island eliminated
- [x] P0 regression gate requires zero past-output grain writes and zero crackle-threshold violations
- [ ] Recorded-vocal quality and consonant tests
- [ ] Plosive, sibilant, and onset transition handling
- [ ] Mix, output-trim, and external bypass automation smoothing audit
- [ ] Latency/quality optimisation
- [ ] Blind listening comparison against Candidate A

Candidate B remains the active engineering engine. Passing synthetic and harmonic-regression gates validates scheduling containment and continuity for the tested fixtures, but does not prove production vocal quality.

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
- [x] Candidate B successful three-platform CI run — run 29, commit `a0afc31`
- [x] P0 crackle-containment gate — run 45, six CTest targets passed on all three platforms
- [x] Windows, macOS, and Linux VST3/Standalone installers and portable artifacts verified for P0 containment
- [ ] First published signed production release

## E6 — Bake-off and gate

- [ ] Batch render matrix
- [ ] CSV/JSON metric reports
- [ ] Blind listening renders
- [ ] Full callback deadline benchmark under host-like load
- [ ] GO / ITERATE / STOP decision

## Current milestone

**Reached:** P0 crackle containment merged to `main` in commit `adfabb9278f9879eeb5a652ea12410684f794791`. The deterministic paired crackles were traced to a hard aligned-dry bypass that became active for only five samples while the smoothed correction ratio crossed unity. It has been replaced by a continuous cents-domain transition.

**Validated:** GitHub Actions run 45 compiled VST3 and Standalone, passed all six CTest targets—including `crackle_smoke`—created installers, and uploaded artifacts on Windows x64, macOS Universal, and Linux x64. The regression also exercises harmonic-rich quasi-vocal input, pitch glide, voiced/unvoiced transitions, alternating ±2-semitone correction, and realtime scheduling guards.

**Current release status:** engineering alpha for controlled local vocal listening. The reported deterministic crackle path is fixed in automated regression, but recorded male/female rap tests, consonant/onset protection, parameter-automation smoothing, formant preservation, and E4 phase-locked STFT remain open before any production audio-quality claim.
