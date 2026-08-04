# SonRapTune

**Intelligent Rap Pitch & Melody Processor** — a C++/JUCE VST3 and Standalone project by MasArray.

SonRapTune is developed independently in this repository. It does not share source-tree ownership with the production `askp-vst` repository.

## Current milestone

The current E3 engineering alpha includes:

- causal YIN-derived pitch candidates and four-beam tracking;
- scale and MIDI target mapping;
- correction-ratio trajectory for Tune, Speed, Stability, and Feel;
- Candidate A fixed-duration dual-head shifter retained as a baseline;
- Candidate B detector-guided waveform pitch marks and period-synchronous overlap-add;
- two-period Hann grains with normalised overlap-add;
- fixed-latency dry/bypass alignment and voiced wet mask;
- JUCE VST3/Standalone with MIDI input and APVTS project state;
- deterministic detector, trajectory, realtime-contract, Candidate A, and PSOLA tests.

Candidate B is now the active end-to-end engine. Its synthetic validation covers 36 pitch-shift cases across 44.1, 48, and 96 kHz, 90–440 Hz, and corrections of ±1 and ±2 semitones. This proves scheduling and pitch movement, not final vocal quality.

The period-synchronous engine remains an **engineering candidate**, not a production-quality claim. Recorded male/female rap material, consonant and onset reintegration, formant preservation, and comparison with the phase-locked STFT candidate are still required.

See [`PROGRESS.md`](PROGRESS.md) and [`docs/TDD_P0_DSP_Bakeoff_v1.0.md`](docs/TDD_P0_DSP_Bakeoff_v1.0.md).

## Single-click local build

### Windows

Double-click:

```text
build-windows.bat
```

Requirements: Visual Studio 2022 or newer with Desktop development with C++, CMake, Git, and Inno Setup 6. Outputs are placed in `dist/windows`:

- VST3 + Standalone ZIP;
- Windows x64 Setup EXE when Inno Setup is installed.

### macOS

Double-click `build-macos.command`, or run:

```bash
bash scripts/build-macos.sh --package
```

Outputs in `dist/macos`:

- Universal VST3 + Standalone ZIP;
- PKG installer.

Local builds use ad-hoc signing. Public macOS distribution still requires Developer ID signing and Apple notarisation.

### Linux

Run or double-click `build-linux.sh`:

```bash
bash build-linux.sh
```

Outputs in `dist/linux`:

- portable tar.gz;
- DEB installer containing `/usr/lib/vst3/SonRapTune.vst3` and `/usr/bin/sonraptune`.

## Manual CMake build

```bash
cmake -S . -B build -DSONRAPTUNE_BUILD_PLUGIN=ON -DSONRAPTUNE_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The plugin fetches pinned JUCE 8.0.14, stores state under `SONRAPTUNE_STATE`, and currently exposes the JUCE generic editor for engineering tests.

## Automated build and release

- `Build and Test` runs Windows x64, macOS Universal, and Linux x64 builds on pushes, pull requests, and manual dispatch.
- `Release Installers` runs when a tag matching `v*` is pushed, or by manual dispatch with a release tag.
- The release workflow publishes Windows EXE/ZIP, macOS PKG/ZIP, and Linux DEB/tar.gz assets to GitHub Releases.

Example release:

```bash
git tag v0.1.0
git push origin v0.1.0
```

## Realtime safety contract

- No allocation, file access, networking, or blocking lock in `processBlock`.
- Fixed-capacity MIDI note state, pitch-mark history, and lossy lock-free telemetry.
- Detector and shifter memory are allocated during preparation, not playback.
- Internal bypass remains latency-aligned.
- Unity correction uses the aligned dry path directly.
- No hidden compressor, EQ, stereo enhancement, or loudness lift.
