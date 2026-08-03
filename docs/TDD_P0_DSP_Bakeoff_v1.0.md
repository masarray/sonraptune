# SonRapTune Technical Design — P0 DSP Bake-off

Version 1.0 · 2026-08-04

## Purpose

P0 proves that SonRapTune can detect, track, map, and eventually shift a monophonic rap vocal with predictable latency and realtime safety. It intentionally separates analysis correctness from audible pitch shifting so an immature shifter is not mistaken for a finished product.

## Product constraints

- Windows x64 VST3 and Standalone first.
- C++17, CMake, JUCE 8.0.14.
- Mono or stereo input; mono analysis sum.
- MIDI input required for future Melody Guide.
- No allocation, file access, networking, UI calls, or blocking lock in the audio callback.
- No hidden EQ, compression, stereo enhancement, or loudness lift.
- Host latency must always match the selected shifter path.

## P0 signal flow

```text
Input
  → analysis mono sum
  → detector decimation (~12 kHz)
  → YIN-derived candidate detector
  → causal multi-candidate tracker
  → scale/MIDI target mapper
  → correction trajectory
  → candidate pitch shifter (E3/E4)
  → optional formant/consonant path (E5)
  → mix/output trim
```

Until E3/E4 are implemented, the audio path is honest passthrough while telemetry and trajectory continue to run.

## Analysis architecture

### Candidate detector

The detector uses a causal, YIN-derived difference function and cumulative mean normalized difference function (CMNDF). Up to four local candidates are retained per analysis frame.

Required outputs:

- frequency in hertz;
- MIDI pitch;
- candidate confidence;
- CMNDF score;
- RMS/energy estimate;
- basic onset flag;
- sample timestamp.

The current implementation prioritizes the first reliable CMNDF period rather than the deepest later minimum. This avoids selecting a subharmonic when stronger harmonics create a deeper long-lag minimum.

### Causal tracker

A fixed four-beam tracker evaluates candidate continuity across frames. Transition cost includes:

- pitch distance;
- octave-jump penalty;
- voiced/unvoiced transition cost;
- confidence penalty;
- continuity preference.

No dynamic allocation is permitted while processing.

### Future voicing features

E1 is not complete until it adds and validates:

- spectral flatness;
- zero-crossing rate;
- high-frequency ratio;
- adaptive noise-floor estimate;
- recorded male/female rap material;
- explicit flutter comparison against nearest-note mapping.

## Target mapping

The mapper supports:

- Major;
- Natural Minor;
- Chromatic;
- custom 12-note mask;
- key-relative pitch classes;
- nearest active MIDI note.

Stability applies hysteresis and minimum note commitment so an input near a semitone boundary does not rapidly alternate targets.

## Correction trajectory

The trajectory produces a sample-by-sample pitch ratio from:

- Tune amount;
- Speed in milliseconds;
- Stability;
- Feel/dead-zone;
- detected pitch;
- committed target pitch.

E2 remains incomplete until end-note lock, phrase-aware release, calibrated product-mode curves, and sample-offset MIDI segmentation are implemented.

## Pitch-shifter bake-off

Two independent candidates must be implemented before selecting the product engine.

### E3 — TD-PSOLA-derived candidate

Required work:

- robust pitch-mark estimator;
- period-synchronous grain extraction;
- overlap-add scheduler;
- voiced/unvoiced transition handling;
- ratio smoothing;
- fixed and reported latency;
- aligned dry path;
- CPU and quality tests.

Expected advantage: low latency and strong monophonic-vocal suitability.

Primary risks: unstable pitch marks, buzz, grain doubling, rough consonants, and failure on breathy/fry vocals.

### E4 — Phase-locked STFT candidate

Required work:

- STFT analysis/synthesis;
- spectral peak detection;
- identity phase locking;
- ratio automation;
- transient protection;
- fixed and reported latency;
- CPU and quality tests.

Expected advantage: controlled spectrum/formant experimentation and stable offline/HQ operation.

Primary risks: latency, transient smear, metallic phase artifacts, and CPU cost.

## Formant and rap protection

E5 compares:

1. no-preserve baseline;
2. LPC-derived formant preservation;
3. cepstral-envelope preservation.

Rap articulation protection must preserve or reintegrate unvoiced/onset material using a latency-aligned path. Sibilants and plosives must not be forced into a tonal shifter path at full strength.

## Realtime contract

The callback may use only preallocated storage and lock-free/fixed-capacity communication.

The following are prohibited in `processBlock`:

- heap allocation;
- container growth;
- filesystem access;
- networking;
- blocking mutexes;
- synchronous logging;
- message-thread/UI calls.

Lossy telemetry is acceptable; audio waiting for telemetry is not.

## Validation matrix

### Sample rates

- 44.1 kHz
- 48 kHz
- 96 kHz

### Block sizes

- 32
- 64
- 128
- 256
- 512
- 1024 samples

### Synthetic detector set

- 70 Hz
- 82.41 Hz
- 98 Hz
- 110 Hz
- 146.83 Hz
- 196 Hz
- 220 Hz
- 293.66 Hz
- 440 Hz
- 659.25 Hz
- 880 Hz

Current result on this deterministic set:

- median fine pitch error: 0.13 cent;
- gross/octave error: 0%;
- core mapping/trajectory smoke: pass;
- realtime-contract matrix smoke: pass.

Synthetic accuracy is necessary but not sufficient. Recorded rap, vibrato, vocal fry, breath, room noise, consonants, and phrase endings remain mandatory before selecting a shifter.

## E6 decision gate

A candidate receives **GO** only when it satisfies all of the following:

- no callback allocation or blocking behavior;
- finite output under automation and sample-rate changes;
- correct reported latency and aligned bypass/dry path;
- acceptable octave and flutter error on recorded material;
- intelligible consonants and phrase attacks;
- no persistent metallic, granular, or doubled-pitch artifact;
- CPU reserve suitable for multiple live instances;
- blind listening preference or parity against the alternative candidate.

Use **ITERATE** when architecture is viable but one or more bounded issues remain. Use **STOP** when the candidate cannot satisfy latency, quality, or realtime constraints without redesign.

## Current implementation boundary

Completed:

- repository and core target;
- JUCE plugin scaffolding;
- MIDI state and APVTS state;
- detector and causal tracker;
- scale/MIDI mapper;
- correction trajectory foundation;
- deterministic core tests.

Not completed:

- audible shifter;
- formant preservation;
- consonant reintegration;
- recorded-vocal dataset;
- CI binary verification;
- final bake-off decision.
