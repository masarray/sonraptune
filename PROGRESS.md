# SonRapTune implementation checklist

Updated: 2026-08-27

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
- [x] Custom responsive SonRapTune user interface

## GUI — familiar pitch workflow, original visual system

- [x] Replaced JUCE `GenericAudioProcessorEditor`
- [x] Original SonRapTune dark visual language and geometric header mark
- [x] Familiar top-row workflow: Input, Key, Scale, Style, Mix, Output, Bypass
- [x] Large Speed and Feel controls flanking the live pitch display
- [x] Live Pitch Focus display driven by the existing lock-free telemetry tap
- [x] Tune, Stability, Formant, and Consonant macro row
- [x] Original 12-note Scale Map instead of copying another product's piano/panel geometry
- [x] All visible controls bound to existing APVTS parameters
- [x] Resizable editor with host-safe layout limits
- [x] Real Standalone render visually audited after CI packaging
- [x] Windows x64, macOS Universal, and Linux x64 build/package validation — CI run 56
- [ ] Recorded-vocal UX session: verify readability while singing/rapping in a real DAW
- [ ] Enharmonic note-name policy (flat/sharp naming according to key context)
- [ ] Final accessibility/keyboard-focus pass

The GUI intentionally keeps the workflow familiar to experienced pitch-correction users while using SonRapTune-specific geometry, colours, controls, scale visualization, branding, and spacing. It does not copy third-party logos, product marks, icons, typefaces, exact panel geometry, or effect artwork.

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
- [x] Custom GUI gate — run 56, VST3/Standalone plus all six tests passed on all three platforms
- [x] Windows, macOS, and Linux VST3/Standalone installers and portable artifacts verified
- [ ] First published signed production release

## E6 — Bake-off and gate

- [ ] Batch render matrix
- [ ] CSV/JSON metric reports
- [ ] Blind listening renders
- [ ] Full callback deadline benchmark under host-like load
- [ ] GO / ITERATE / STOP decision

## Current milestone

**Reached:** custom SonRapTune GUI merged to `main` in commit `5f1d6b6fa3ede2ccae01d66e5a69d53cfc0993c1`. The previous generic JUCE editor is gone. SonRapTune now has a dedicated pitch-correction workflow with Input/Key/Scale/Style setup, large Speed and Feel controls, live Pitch Focus telemetry, voice-shape macros, Mix/Output, Bypass, and a 12-note scale map.

**Validated:** GitHub Actions run 56 compiled VST3 and Standalone, passed all six existing DSP regression targets, created packages, and uploaded artifacts on Windows x64, macOS Universal, and Linux x64. The packaged Linux Standalone was also launched under a virtual display and visually audited at the default 1040×680 editor size.

**Current release status:** engineering alpha for controlled local vocal listening. The GUI foundation is complete enough for real DAW use and iteration; recorded male/female rap tests, consonant/onset protection, parameter-automation smoothing, formant preservation, and E4 phase-locked STFT remain open before any production audio-quality claim.
