# SonRapTune Style / Formant / Consonant DSP

Updated: 2026-08-27

## Active controls

### Style

Style now changes DSP behaviour rather than acting as a label.

- Natural: slower correction, wider expressive dead-zone, longer target commitment.
- Modern Rap: tighter correction and quicker note commitment.
- Trap Lock: fastest correction, smallest dead-zone, fastest target capture.
- Hook: smoother capture/release and more conservative target commitment.

### Formant Preserve

Candidate B PSOLA inherently retains more source spectral-envelope character than a resampling pitch shifter because it repositions pitch-synchronous waveform grains. The Formant control now changes grain geometry:

- 100%: grain radius follows the original source period.
- 0%: grain radius follows the target period.
- intermediate values interpolate continuously.

This is a real audible DSP control, but it is still a lightweight PSOLA formant-retention baseline. It is not yet an independent LPC or cepstral formant shifter.

### Consonant Protect

A realtime allocation-free protection stage estimates waveform roughness and zero-crossing density and combines those cues with pitch voicing/state. It smoothly reduces the pitch-shift wet path on noisy consonants, sibilants, hard onsets and unstable material.

### Custom Scale

Custom Scale is persisted as a 12-bit APVTS parameter. When Custom is selected, the Scale Map becomes interactive. Clicking note cells toggles allowed relative scale tones; the root remains enabled.

### Automation smoothing

Mix, Bypass and Output Trim use per-sample smoothing so host automation does not create block-edge gain/wet jumps.

## Validation

`style_protection_smoke` verifies:

- Natural, Modern Rap and Trap Lock produce ordered correction aggressiveness;
- vowel-like and noise-like fixtures produce different Consonant Protect wet levels;
- Formant Preserve changes PSOLA grain radius;
- Custom Scale mask changes target-note mapping;
- the existing P0 crackle and realtime no-allocation gates remain part of the full CTest suite.

## Remaining work

- recorded male/female rap vocal fixtures;
- LPC/cepstral formant candidate and blind comparison;
- phrase/end-note lock refinement;
- Smart Auto-Key and chord-aware song intelligence;
- production listening/bake-off before release claims.
