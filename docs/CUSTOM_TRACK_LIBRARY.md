# Custom track library: developer reference

For installation, supported modes and troubleshooting, see the
[player guide](features/CUSTOM_TRACKS.md).

## Library and identity

`platform/native_track_library.c` scans immediate subdirectories of
`assets/tracks/`. Each track requires `track.lev` and `track.vrm`; `track.sca`,
`preview.jpg` and `track.ini` are optional. Missing metadata falls back to the
directory name for the title and `local:<directory name>` for the identity.
The manifest accepts `id`, `title` and `author` once each; unknown or duplicate
keys are rejected. Mode hints and selection eligibility are determined by the
application. Identities must be unique. `.ctr-disabled` marks a disabled track.

`NativeTrackLibrary_Refresh` replaces the registry. Metadata pointers borrowed
from it are invalid after refresh; runtime IDs are assigned in scan order and
must not be persisted or used as retail table indices. Custom IDs range from
`0x100` to `0x7fff`. High scores and ghosts use the stable identity plus hashes
of both LEV and VRM, not the runtime ID. One-lap debug races do not persist
three-lap records.

The scan checks file presence and derives mode hints from the LEV, but does not
certify that a track is playable. Folder import and on-demand reads check LEV
pointer-map bounds and a minimum level header, and VRM image bounds. These are
not comprehensive gameplay or asset validation.

## Import and runtime

Folder import copies files into a temporary directory, validates them, checks
for duplicate destination and identity, then publishes the track. The xdelta
importer uses a temporary copy of the NTSC-U disc, compares the 18 retail 1P
circuit LEV/VRM pairs and installs changed pairs through the same importer.
For recognized adventure loaders it also follows their BIGFILE index arrays to
changed pairs across the circuit and arena groups; these LOD slots can hold
independent tracks. Byte-identical pairs are deduplicated within each patch.
Recognized loader signatures can also supply titles and music mappings; when
an adventure mapping cannot be established, music is skipped rather than
assigned from an unrelated physical slot. Extraction remains best-effort:
unrecognized loaders and non-track groups are not fully supported. It
also extracts author credits and compatible music where possible; executable patches,
XA audio and arbitrary custom PS1 code are not imported. Failed multi-track
imports roll back newly installed tracks and report cleanup failures.

The game queues custom LEV and VRM files through the normal level-loading
callbacks. Split-screen uses the imported 1P assets. Custom music is loaded
from optional SCA files with runtime SPU and sample compatibility checks; an
incompatible or missing bank falls back to game audio. Preview and minimap
decoding are independent and optional.

## Tools and tests

Windows builds set up verified xdelta3 and CTR-tools under `assets/tools/`;
release archives include them. See `packaging/TRACK_TOOLS.txt` and
`scripts/setup-track-tools.ps1` for versions, licenses and manual setup.
Other hosts must provide compatible executables at the paths expected by
`platform/native_track_patch.c`.

The automated track suites use synthetic fixtures. Optional integration
diagnostics can inspect or import a supplied patch into disposable storage:

```text
build/tests/test_track_patch.exe --inspect-loader <base-bin> <patched-bin>
build/tests/test_track_patch.exe --import-patch-live <absolute-assets-path> <absolute-patch-path>
```

An optional integration
check uses a supplied retail image and installed tools in a disposable library:

```text
build/tests/test_track_patch.exe --retail-live <absolute-assets-path>
```
