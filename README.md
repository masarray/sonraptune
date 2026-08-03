# SonRapTune

**Intelligent Rap Pitch & Melody Processor** — a C++/JUCE VST3 and Standalone project by MasArray.

SonRapTune is developed independently in this repository. It does not share source-tree ownership with the production `askp-vst` repository.

## Current milestone

Version 0.1.0 adds the first audible E3 bake-off path:

- causal YIN-derived pitch candidates and four-beam tracking;
- scale and MIDI target mapping;
- correction-ratio trajectory for Tune, Speed, Stability, and Feel;
- fixed-latency dual-head time-domain shifter candidate;
- latency-aligned dry/bypass path and voiced wet mask;
- JUCE VST3/Standalone with MIDI input and APVTS project state;
- deterministic detector, trajectory, realtime-contract, and shifter tests.

The audible shifter is a **prototype candidate**, not the selected final TD-PSOLA engine. It exists so end-to-end tuning can be tested locally while the period-synchronous and phase-locked STFT candidates are developed and compared.

See [`PROGRESS.md`](PROGRESS.md) and [`docs/TDD_P0_DSP_Bakeoff_v1.0.md`](docs/TDD_P0_DSP_Bakeoff_v1.0.md).

## Single-click local build

### Windows

Double-click:

```text
build-windows.bat
```

Requirements: Visual Studio 2022 with Desktop development with C++, CMake, Git, and Inno Setup 6. Outputs are placed in `dist/windows`:

- VST3 + Standalone ZIP;
- Windows x64 Setup EXE when Inno Setup is installed.

### macOS

Double-click `build-macos.command`, or run:

```bash
bash scripts/build-macos.sh --package
```

Outputs in `dist/macos`:

- VST3 + Standalone ZIP;
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
- Fixed-capacity MIDI note state and lossy lock-free telemetry.
- Detector and shifter memory are allocated during preparation, not playback.
- Internal bypass remains latency-aligned.
- No hidden compressor, EQ, stereo enhancement, or loudness lift.
