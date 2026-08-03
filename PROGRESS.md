# SonRapTune implementation checklist

Updated: 2026-08-04

## Repository separation

- [x] Dedicated `masarray/sonraptune` repository
- [x] Source placed at repository root, not under `askp-vst`
- [x] Production `askp-vst` master remains untouched
- [x] P0 source and validation results migrated
- [x] TDD added under `docs/`

## E0 — Foundation

- [x] Independent SonRapTune project structure
- [x] CMake C++17 core target
- [x] Pinned JUCE 8.0.14 plugin configuration
- [x] VST3 and Standalone target definitions
- [x] MIDI input enabled in plugin metadata and processor
- [x] Mono/stereo host bus compatibility rules
- [x] APVTS parameter model and `SONRAPTUNE_STATE`
- [x] Lock-free lossy pitch telemetry ring
- [x] Fixed 128-note MIDI active-note state
- [x] Analysis-only engine shell with honest passthrough output
- [ ] JUCE VST3/Standalone binary compile verified in CI or Windows environment
- [ ] Deterministic WAV offline renderer
- [ ] GitHub Actions build workflow

## E1 — Analysis

- [x] Full-rate input to ~12 kHz detector decimation
- [x] Preallocated circular analysis history
- [x] YIN-derived difference and CMNDF calculation
- [x] Up to four local-minimum pitch candidates
- [x] Parabolic lag interpolation
- [x] Energy-aware confidence and basic onset indicator
- [x] Causal four-beam pitch tracker
- [x] Octave-jump penalty and voiced/unvoiced transition cost
- [x] Synthetic harmonic-stack detector benchmark: 70–880 Hz, median 0.13 cent, gross error 0%
- [ ] Full voicing feature set: spectral flatness, ZCR, HF ratio, noise-floor model
- [ ] Recorded male/female rap dataset benchmark
- [ ] Flutter comparison against nearest-note baseline

## E2 — Mapping and trajectory

- [x] Major, Natural Minor, Chromatic, and custom scale masks
- [x] Key-relative nearest-note mapping
- [x] Stability-based hysteresis and minimum note commitment
- [x] Fixed-bitset MIDI nearest-note target
- [x] Tune, Speed, Stability, and Feel foundation
- [x] Sample-by-sample correction-ratio trajectory
- [x] Deterministic mapping/trajectory smoke test
- [ ] End-note lock and phrase-aware release
- [ ] Per-mode calibrated macro curves
- [ ] Sample-offset MIDI event segmentation inside a block

## E3 — TD-PSOLA candidate

- [ ] Pitch-mark estimator
- [ ] Period-synchronous grain scheduler
- [ ] Voiced/unvoiced transition handling
- [ ] Fixed latency and aligned dry path
- [ ] Quality and CPU tests

## E4 — Phase-locked STFT candidate

- [ ] STFT analysis/synthesis
- [ ] Peak identity phase locking
- [ ] Ratio automation
- [ ] Transient protection
- [ ] Quality and CPU tests

## E5 — Formant and rap protection

- [ ] No-preserve baseline
- [ ] LPC formant candidate
- [ ] Cepstral formant candidate
- [ ] Consonant/onset mask
- [ ] Latency-aligned dry reintegration

## E6 — Bake-off and gate

- [ ] Batch render matrix
- [ ] CSV/JSON metric reports
- [ ] Blind listening renders
- [ ] Callback deadline and allocation audit
- [ ] Final GO / ITERATE / STOP report

## Latest validation

- [x] Detector benchmark passes: median fine pitch error 0.13 cent
- [x] Gross/octave error 0% on the current synthetic 70–880 Hz set
- [x] Core mapping/trajectory smoke test passes
- [x] Realtime-contract smoke test passes at 44.1/48/96 kHz and block sizes 32–1024

## Current milestone

**Reached:** repository separation complete, E0 implementation foundation, E1 correctness prototype passing the first synthetic pitch gate, and the first half of E2.

**Audible tuning:** not yet active. The plugin remains passthrough by design until E3/E4 shifter candidates are implemented and compared.
