#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
bash scripts/build-linux.sh --package "$@"
echo
read -r -p "SonRapTune packages are ready in dist/linux. Press Return to close..." _
