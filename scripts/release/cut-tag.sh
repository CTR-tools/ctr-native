#!/usr/bin/env bash
# Validate a tag request and, unless this is a dry run, cut the tag.
#
# Inputs from the environment:
#   DRY_RUN        true to validate and report without pushing
#   INPUT_VERSION  the version to cut, with or without the leading v
#   SHA            the head of master, which is the commit being tagged

set -euo pipefail

version_pattern='^v[0-9]+\.[0-9]+\.[0-9]+$'

version="${INPUT_VERSION// /}"
if [ -z "$version" ]; then
    echo "Set version to the number to tag, e.g. 1.2.0." >&2
    exit 1
fi

tag="v${version#v}"
if ! printf '%s' "$tag" | grep -Eq "$version_pattern"; then
    echo "Version '$INPUT_VERSION' is not X.Y.Z." >&2
    exit 1
fi

# Only master may be cut from. This catches a dispatch aimed at the wrong ref
# in `gh workflow run --ref`; it is not a security boundary,
# because a workflow dispatched on a branch runs that branch's copy of this
# file. The tag ruleset is what actually stops a tag being cut elsewhere.
if [ "$GITHUB_REF" != "refs/heads/master" ]; then
    echo "Run this from master; this dispatch was $GITHUB_REF." >&2
    exit 1
fi

if git ls-remote --exit-code --tags origin "refs/tags/$tag" >/dev/null 2>&1; then
    echo "Tag $tag already exists on origin." >&2
    exit 1
fi

# The gate. A tag is immutable once pushed, so nothing may be tagged that CI has
# not already cleared on this exact commit.
state="$(gh run list --workflow ci.yml --commit "$SHA" --limit 1 \
    --json status,conclusion --jq '.[0] | "\(.status) \(.conclusion)"' || true)"

case "$state" in
    "completed success")
        echo "CI passed on $SHA."
        ;;
    "" | *"None"*)
        # A feature branch has no push-triggered CI run, so this is also what
        # stops a dispatch from anywhere but master.
        echo "No CI run found for $SHA. Push it to master and let CI finish first." >&2
        exit 1
        ;;
    *)
        echo "CI on $SHA is not green: $state" >&2
        exit 1
        ;;
esac

{
    echo "## Tag request"
    echo ""
    echo "| | |"
    echo "|---|---|"
    echo "| Tag | \`$tag\` |"
    echo "| Commit | \`$SHA\` |"
    if [ "$DRY_RUN" = "true" ]; then
        echo "| Action | dry run, nothing will be pushed |"
    else
        echo "| Action | cut the tag |"
    fi
} >>"$GITHUB_STEP_SUMMARY"

if [ "$DRY_RUN" = "true" ]; then
    echo "Dry run: would push refs/tags/$tag at $SHA."
    exit 0
fi

git config user.name "github-actions[bot]"
git config user.email "41898282+github-actions[bot]@users.noreply.github.com"

git tag -a "$tag" -m "CTR Native Expanded $tag" "$SHA"
git push origin "refs/tags/$tag"

echo "Pushed refs/tags/$tag."