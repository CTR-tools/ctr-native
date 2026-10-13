#!/usr/bin/env bash
# Create the draft release and attach the packaged archive.
#
# Inputs from the environment:
#   GH_TOKEN  token with contents: write
#   TAG       the tag being released

set -euo pipefail

gh release create "$TAG" \
    --draft \
    --title "CTR Native Expanded ${TAG#v}" \
    --generate-notes \
    "dist/*"

echo "Draft release $TAG is ready."