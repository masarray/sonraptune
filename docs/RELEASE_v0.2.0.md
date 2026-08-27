# SonRapTune v0.2.0 — Public Beta

Release target: public engineering beta.

## Highlights

- Auto Key v1 estimates major/minor key from stable incoming vocal melody with confidence, decaying pitch-class history, and switch hysteresis.
- Manual Key/Scale remain the fallback while Auto Key is learning or uncertain.
- The live Pitch Focus display reports Auto Key learning/resolved state and confidence.
- Scale Map follows the resolved Auto Key result without overwriting the user's saved manual fallback.
- Phrase-end correction holds the last correction ratio while the wet path fades, avoiding an artificial scoop toward unity at note endings.
- Existing Natural, Modern Rap, Trap Lock, Hook, Formant Preserve, Consonant Protect, Custom Scale, and click-safe automation behavior remains active.
- Regression suite expands to eight tests with `song_intelligence_smoke`.

## Distribution

The public release workflow builds and publishes:

- Windows x64 Setup EXE and portable VST3/Standalone ZIP;
- macOS Universal PKG and VST3/Standalone ZIP;
- Linux amd64 DEB and portable tar.gz.

## Important beta limitations

- Auto Key v1 listens to the vocal input only. Backing-track sidechain analysis and chord-by-chord detection are not implemented yet.
- Ambiguous or sparse melodies and relative major/minor material may require manual Key/Scale.
- Formant Preserve is still a PSOLA grain-geometry baseline, not an independent LPC/cepstral formant shifter.
- Windows binaries are not Authenticode signed.
- macOS binaries are ad-hoc signed and not Apple-notarized.
- Recorded-vocal corpus validation and blind algorithm bake-off remain required before a production-final audio-quality claim.
