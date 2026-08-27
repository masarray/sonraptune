#!/usr/bin/env bash
set -euo pipefail

PACKAGE=0
CLEAN=0
for arg in "$@"; do
  case "$arg" in
    --package) PACKAGE=1 ;;
    --clean) CLEAN=1 ;;
  esac
done

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build/macos"
DIST_DIR="$ROOT/dist/macos"
STAGE_DIR="$DIST_DIR/stage"

if [[ "$CLEAN" == "1" ]]; then
  rm -rf "$BUILD_DIR" "$DIST_DIR"
fi

if [[ -n "${SONRAPTUNE_VERSION:-}" ]]; then
  RAW_VERSION="${SONRAPTUNE_VERSION#release/}"
  VERSION="${RAW_VERSION#v}"
else
  VERSION="$(sed -nE 's/.*project\(SonRapTune VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$ROOT/CMakeLists.txt" | head -n1)"
fi
[[ -n "$VERSION" ]] || { echo "Could not determine version" >&2; exit 1; }

ARCHS="${SONRAPTUNE_ARCHS:-$(uname -m)}"
ARCH_LABEL="${ARCHS//;/_}"

echo "Building SonRapTune $VERSION for macOS ($ARCHS)..."
cmake -S "$ROOT" -B "$BUILD_DIR" -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
  -DSONRAPTUNE_BUILD_PLUGIN=ON \
  -DSONRAPTUNE_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" --config Release --parallel
ctest --test-dir "$BUILD_DIR" -C Release --output-on-failure

ARTEFACTS="$BUILD_DIR/SonRapTune_artefacts/Release"
VST3="$ARTEFACTS/VST3/SonRapTune.vst3"
APP="$ARTEFACTS/Standalone/SonRapTune.app"
[[ -d "$VST3" ]] || { echo "VST3 not found: $VST3" >&2; exit 1; }
[[ -d "$APP" ]] || { echo "Standalone app not found: $APP" >&2; exit 1; }

rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/VST3" "$STAGE_DIR/Standalone"
cp -R "$VST3" "$STAGE_DIR/VST3/"
cp -R "$APP" "$STAGE_DIR/Standalone/"

# Ad-hoc signing keeps local builds internally consistent. Public distribution
# still requires the owner's Developer ID and notarisation credentials.
codesign --force --deep --sign - "$STAGE_DIR/VST3/SonRapTune.vst3"
codesign --force --deep --sign - "$STAGE_DIR/Standalone/SonRapTune.app"

ZIP="$DIST_DIR/SonRapTune-$VERSION-macOS-$ARCH_LABEL.zip"
rm -f "$ZIP"
ditto -c -k --sequesterRsrc --keepParent "$STAGE_DIR" "$ZIP"
echo "Portable package: $ZIP"

if [[ "$PACKAGE" == "1" ]]; then
  PKG_ROOT="$DIST_DIR/pkgroot"
  rm -rf "$PKG_ROOT"
  mkdir -p "$PKG_ROOT/Library/Audio/Plug-Ins/VST3" "$PKG_ROOT/Applications"
  cp -R "$STAGE_DIR/VST3/SonRapTune.vst3" "$PKG_ROOT/Library/Audio/Plug-Ins/VST3/"
  cp -R "$STAGE_DIR/Standalone/SonRapTune.app" "$PKG_ROOT/Applications/"

  COMPONENT="$DIST_DIR/SonRapTune-component.pkg"
  FINAL_PKG="$DIST_DIR/SonRapTune-$VERSION-macOS-$ARCH_LABEL.pkg"
  rm -f "$COMPONENT" "$FINAL_PKG"
  pkgbuild \
    --root "$PKG_ROOT" \
    --identifier com.masarray.sonraptune \
    --version "$VERSION" \
    --install-location / \
    "$COMPONENT"
  productbuild --package "$COMPONENT" "$FINAL_PKG"
  rm -f "$COMPONENT"
  echo "macOS installer: $FINAL_PKG"
fi

echo "Done."
