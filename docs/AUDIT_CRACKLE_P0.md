# SonRapTune P0 Crackle Audit

Updated: 2026-08-04

## User symptom

Real vocal testing produced a broken-radio / off-station sound with repeated crackles ("pretek-pretek").

## Confirmed root causes

### 1. Past-output grain writes

Candidate B used a two-period grain but reported only one maximum source period plus scheduling margin. At low fundamental frequencies the beginning of a grain could be written into an output sample that had already been consumed. Because the output is a ring buffer, that stale write could survive until the same slot wrapped around and appear later as an isolated click or burst.

Containment:

- latency now covers two maximum source periods plus scheduling margin;
- every grain write is checked against the realtime read cursor;
- rejected past writes are counted and the regression gate requires zero.

### 2. Stale OLA tails after voiced-state resets

When pitch reliability changed, pitch marks were reset but already scheduled overlap-add samples remained in the output ring. Old and new scheduling states could therefore meet in the same future region.

Containment:

- OLA slots are tagged with a generation number;
- reliability transitions invalidate the previous generation in O(1);
- stale scheduled samples are ignored without clearing a large ring on the audio thread.

### 3. Confidence used as dry/wet amount

The previous trajectory multiplied voicing by detector confidence and used the result as the wet mix. Real vocals rarely hold confidence at exactly 1.0, so dry and pitch-shifted signals were continuously mixed at different pitch and phase. This causes comb filtering, beating, and the audible off-station-radio effect.

Containment:

- confidence is now a voiced-state decision input, not a wet percentage;
- stable voiced frames request the processed path;
- dedicated 8 ms attack and 4 ms release crossfades protect transitions.

### 4. OLA under-coverage switching

The previous output switched immediately between aligned dry and PSOLA whenever overlap weight crossed a tiny threshold. Coverage holes therefore created waveform discontinuities.

Containment:

- OLA coverage has a meaningful minimum threshold;
- coverage is converted to a smoothed crossfade;
- under-covered samples fall back to aligned dry instead of dividing by a near-zero weight.

### 5. Fast scheduling-ratio movement

Pitch ratio was consumed directly by the synthesis scheduler. Even though correction trajectory is smoothed, rapid frame changes could still jitter synthesis spacing.

Containment:

- Candidate B now applies an additional 4 ms scheduler-ratio smoother.

## New regression gate

`crackle_smoke` renders a harmonic-rich quasi-vocal signal with:

- a 75–160 Hz glide;
- eight harmonics;
- voiced/unvoiced phrase transitions;
- deterministic breath/noise sections;
- alternating +2 and -2 semitone correction;
- smoothed wet transitions.

The test fails when:

- any grain is written to an already-consumed output sample;
- output becomes non-finite;
- output peak exceeds the containment bound;
- an adjacent-sample discontinuity exceeds the crackle threshold;
- voiced reliability transitions are not exercised.

## Remaining risk

This patch is containment, not final vocal-quality proof. Candidate B still needs:

- correlation-based and sub-sample pitch marks;
- plosive/sibilant/onset protection;
- recorded male/female rap fixtures;
- spectral-discontinuity and modulation metrics;
- blind listening against Candidate A and the future phase-locked STFT candidate;
- formant preservation.

No public production release should be tagged until those gates pass.
