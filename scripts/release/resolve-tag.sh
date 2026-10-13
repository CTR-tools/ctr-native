#!/usr/bin/env bash
# Accept a tag for release, and report the commit it points at.
#
# Input from the environment:
#   INPUT_TAG  the tag this run was triggered by
#
# Publishes tag and sha as job outputs.

set -euo pipefail

tag="${INPUT_TAG// /}"

if [ -z "$tag" ]; then
    echo "Set tag to the v1.2.3 you pushed." >&2
    exit 1
fi

if ! printf '%s' "$tag" | grep -Eq '^v[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "Tag '$tag' is not vX.Y.Z." >&2
    exit 1
fi

sha="$(git rev-parse "refs/tags/$tag^{commit}")"

git fetch --no-tags origin master
if ! git merge-base --is-ancestor "$sha" origin/master; then
    echo "Tag $tag points at $sha, which master does not contain." >&2
    echo "Merge the work first: a release ships what master accepted." >&2
    exit 1
fi

{
    echo "tag=$tag"
    echo "sha=$sha"
} >>"$GITHUB_OUTPUT"

{
    echo "## Release request"
    echo ""
    echo "| | |"
    echo "|---|---|"
    echo "| Tag | \`$tag\` |"
    echo "| Commit | \`$sha\` |"
    echo "| On master | yes |"
} >>"$GITHUB_STEP_SUMMARY"