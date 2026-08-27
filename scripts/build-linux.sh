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
BUILD_DIR="$ROOT/build/linux"
DIST_DIR="$ROOT/dist/linux"
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

ARCH="$(uname -m)"
case "$ARCH" in
  x86_64) DEB_ARCH=amd64 ;;
  aarch64|arm64) DEB_ARCH=arm64 ;;
  *) DEB_ARCH="$ARCH" ;;
esac

echo "Building SonRapTune $VERSION for Linux $ARCH..."
cmake -S "$ROOT" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DSONRAPTUNE_BUILD_PLUGIN=ON \
  -DSONRAPTUNE_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure

ARTEFACTS="$BUILD_DIR/SonRapTune_artefacts/Release"
VST3="$ARTEFACTS/VST3/SonRapTune.vst3"
APP="$ARTEFACTS/Standalone/SonRapTune"
[[ -d "$VST3" ]] || { echo "VST3 not found: $VST3" >&2; exit 1; }
[[ -f "$APP" ]] || { echo "Standalone not found: $APP" >&2; exit 1; }

rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/VST3" "$STAGE_DIR/Standalone"
cp -R "$VST3" "$STAGE_DIR/VST3/"
cp "$APP" "$STAGE_DIR/Standalone/SonRapTune"
chmod +x "$STAGE_DIR/Standalone/SonRapTune"

TAR="$DIST_DIR/SonRapTune-$VERSION-Linux-$ARCH.tar.gz"
rm -f "$TAR"
tar -C "$STAGE_DIR" -czf "$TAR" .
echo "Portable package: $TAR"

if [[ "$PACKAGE" == "1" ]]; then
  PKG_ROOT="$DIST_DIR/debroot"
  rm -rf "$PKG_ROOT"
  mkdir -p \
    "$PKG_ROOT/DEBIAN" \
    "$PKG_ROOT/usr/lib/vst3" \
    "$PKG_ROOT/usr/bin" \
    "$PKG_ROOT/usr/share/applications"

  cp -R "$STAGE_DIR/VST3/SonRapTune.vst3" "$PKG_ROOT/usr/lib/vst3/"
  cp "$STAGE_DIR/Standalone/SonRapTune" "$PKG_ROOT/usr/bin/sonraptune"
  chmod 0755 "$PKG_ROOT/usr/bin/sonraptune"

  cat > "$PKG_ROOT/DEBIAN/control" <<EOF
Package: sonraptune
Version: $VERSION
Section: sound
Priority: optional
Architecture: $DEB_ARCH
Maintainer: MasArray
Depends: libasound2, libfreetype6, libfontconfig1, libx11-6, libxext6, libxinerama1, libxrandr2, libxcursor1
Description: SonRapTune intelligent rap pitch and melody processor
 VST3 plug-in and standalone application for monophonic rap-vocal tuning.
EOF

  cat > "$PKG_ROOT/usr/share/applications/sonraptune.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=SonRapTune
Comment=Intelligent Rap Pitch & Melody Processor
Exec=/usr/bin/sonraptune
Terminal=false
Categories=AudioVideo;Audio;
EOF

  DEB="$DIST_DIR/SonRapTune-$VERSION-Linux-$DEB_ARCH.deb"
  rm -f "$DEB"
  dpkg-deb --build --root-owner-group "$PKG_ROOT" "$DEB"
  echo "Linux installer: $DEB"
fi

echo "Done."
