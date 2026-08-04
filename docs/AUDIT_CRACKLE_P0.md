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

- OLA coverage is mapped continuously to a 0–1 crossfade;
- aligned dry remains the bounded fallback when OLA coverage is incomplete;
- normalisation never divides by a near-zero weight and never uses a binary coverage threshold.

### 5. Fast scheduling-ratio movement

Pitch ratio was consumed directly by the synthesis scheduler. Even though correction trajectory is smoothed, rapid frame changes could still jitter synthesis spacing.

Containment:

- Candidate B now applies an additional 4 ms scheduler-ratio smoother.

### 6. Five-sample hard unity bypass

The final deterministic paired crackle occurred while a downward correction crossed ratio 1.0. The shifter contained a transparent-unity shortcut that replaced the PSOLA path with aligned dry whenever the smoothed ratio entered a very narrow band around unity. The ratio remained inside that band for only five samples, creating a short dry island between two phase-different PSOLA waveform regions.

The regression located the paired discontinuities at samples 144570 and 144575. They were repeatable across operating systems and occurred in the middle of a stable voiced phrase, not at a phrase boundary or ring-buffer wrap.

Containment:

- the hard unity shortcut was removed;
- distance from unity is evaluated in cents;
- below 0.5 cent the target is aligned dry;
- above 4.5 cents the target is the processed path;
- the region between those points uses smoothstep interpolation;
- the unity mix follows its target with an 8 ms continuous smoother.

This preserves near-unity transparency without inserting a sample-scale dry island.

## Regression gate

`crackle_smoke` renders a harmonic-rich quasi-vocal signal with:

- a 75–160 Hz glide;
- eight harmonics;
- voiced/unvoiced phrase transitions;
- deterministic breath/noise sections;
- alternating +2 and -2 semitone correction;
- smoothed wet transitions;
- correction trajectories that cross unity.

The test fails when:

- any grain is written to an already-consumed output sample;
- output becomes non-finite;
- output peak exceeds the containment bound;
- an adjacent-sample discontinuity exceeds the crackle threshold;
- voiced reliability transitions are not exercised.

## Validation

GitHub Actions run 45 passed all six CTest targets on:

- Windows x64;
- macOS Universal (`arm64` and `x86_64`);
- Linux x64.

Each runner also produced VST3, Standalone, installer, portable package, and uploaded artifacts. The validated code was squash-merged to `main` as commit `adfabb9278f9879eeb5a652ea12410684f794791`.

## Remaining risk

This patch is containment, not final vocal-quality proof. Candidate B still needs:

- recorded male/female rap fixtures and controlled listening tests;
- correlation-based and sub-sample pitch marks;
- plosive/sibilant/onset protection;
- smoothing audit for Mix, output trim, and host-facing bypass automation;
- spectral-discontinuity and modulation metrics;
- blind listening against Candidate A and the future phase-locked STFT candidate;
- formant preservation.

No public production release should be tagged until those gates pass.
