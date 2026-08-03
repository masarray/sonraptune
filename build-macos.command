#!/usr/bin/env bash
cd "$(dirname "$0")"
bash scripts/build-macos.sh --package "$@"
status=$?
echo
if [[ $status -eq 0 ]]; then
  echo "SonRapTune VST3, Standalone and installer are ready in dist/macos."
else
  echo "BUILD FAILED. See the messages above."
fi
read -r -p "Press Return to close..." _
exit $status
