# SonRapTune

**Intelligent Rap Pitch & Melody Processor** — a C++/JUCE VST3 and Standalone project by MasArray.

SonRapTune is developed independently in this repository. It does not share source-tree ownership with the production `askp-vst` repository.

## v0.2.0 Public Beta

SonRapTune v0.2.0 is a public engineering beta focused on real-time rap/melodic-rap pitch correction. It includes:

- causal YIN-derived pitch candidates and four-beam pitch tracking;
- period-synchronous PSOLA pitch correction with fixed host-reported latency;
- Natural, Modern Rap, Trap Lock, and Hook correction styles;
- Tune, Speed, Stability, Feel, Formant Preserve, Consonant Protect, Mix, and Output controls;
- clickable 12-note Custom Scale editor;
- click-safe Mix, Bypass, and Output automation smoothing;
- consonant/sibilant/onset wet-path protection;
- **Auto Key v1**: causal major/minor key inference from stable incoming vocal pitches with confidence and hysteresis;
- manual Key/Scale fallback while Auto Key is still learning;
- phrase-end correction hold so the pitch ratio does not scoop back toward unity while the wet path is fading;
- MIDI note guidance, VST3, and Standalone formats.

### Auto Key scope

Auto Key v1 listens to the **vocal melody entering SonRapTune**. It does not yet analyze a backing-track sidechain and does not claim chord-by-chord accompaniment detection. Ambiguous melodies, modal material, sparse phrases, and relative major/minor pairs may need manual Key/Scale selection. The UI shows `AUTO LEARNING` until enough tonal evidence is available and then displays the inferred key/scale and confidence.

### Audio-quality status

This release is a **public beta**, not a final production-quality claim. Synthetic/harmonic regression gates cover pitch tracking, realtime callback safety, PSOLA scheduling, crackle containment, active Style/Formant/Consonant behavior, Auto Key inference, and phrase-end continuity. A larger recorded male/female rap corpus, independent LPC/cepstral formant processing, backing-track chord intelligence, and a blind Candidate A/B/STFT bake-off remain future work.

## Installation

### Windows

Use the Setup EXE, or copy the VST3 from the portable ZIP to your system VST3 folder. The current public beta is **not Authenticode signed**, so Windows may show a SmartScreen warning.

### macOS

The release contains a Universal VST3 + Standalone ZIP and PKG. Builds are currently **ad-hoc signed and not Apple-notarized**; Gatekeeper may require explicit user approval. Developer ID signing/notarisation is required before a frictionless production distribution.

### Linux

Use the DEB installer or the portable tar.gz. The DEB installs the VST3 under `/usr/lib/vst3` and the Standalone application under `/usr/bin`.

## Single-click local build

### Windows

Double-click:

```text
build-windows.bat
```

Requirements: Visual Studio 2022 or newer with Desktop development with C++, CMake, Git, and Inno Setup 6.

### macOS

Double-click `build-macos.command`, or run:

```bash
bash scripts/build-macos.sh --package
```

### Linux

Run:

```bash
bash build-linux.sh
```

## Manual CMake build

```bash
cmake -S . -B build -DSONRAPTUNE_BUILD_PLUGIN=ON -DSONRAPTUNE_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The plugin fetches pinned JUCE 8.0.14 and stores state under `SONRAPTUNE_STATE`.

## Automated build and release

- `Build and Test` runs Windows x64, macOS Universal, and Linux x64 builds on feature pushes, pull requests, and `main`.
- The test suite includes detector, core, realtime-contract, Candidate A, PSOLA, crackle, Style/Formant/Consonant, and Smart Song Intelligence regression gates.
- `Release Installers` publishes Windows EXE/ZIP, macOS PKG/ZIP, and Linux DEB/tar.gz assets to GitHub Releases.

See [`PROGRESS.md`](PROGRESS.md) for the implementation checklist and remaining production gates.

## Realtime safety contract

- No allocation, file access, networking, or blocking lock in `processBlock`.
- Fixed-capacity MIDI note state, pitch-mark history, and lossy lock-free telemetry.
- Detector, key estimator, protection stages, and shifter use preparation-time or fixed-capacity storage.
- Internal bypass remains latency-aligned.
- Unity correction uses the aligned dry path directly.
- No hidden compressor, EQ, stereo enhancement, or loudness lift.
