# Custom track library: initial disk format

The library scanner accepts one directory per track. Each directory contains
`track.lev` and `track.vrm`; `track.sca` is optional. For example:

```text
tracks/
  example/
    track.lev
    track.vrm
    track.sca
    preview.jpg
```

`preview.jpg` is an optional JPEG image shown in the track selector. A missing
or unreadable image hides the preview and does not prevent the track
from loading. The image is decoded only when the track is selected; files are
limited to 4 MiB and dimensions to 4096 by 4096 pixels. It is scaled to the
selector's preview rectangle preserving its aspect ratio, with black borders
where needed.
The Time Trial selector also extracts a minimap from the selected
track's LEV icons and VRM textures when both map halves are available. This is
independent of `preview.jpg`; missing or unsupported map data hides the map.
A Project Saphi importer can use the track's
`track_thumbnail` download and save it under this name.

No metadata file is required. Without one, the directory name is the title and
`local:<directory name>` is the identity; renaming the directory changes that
identity. A future importer can write `track.ini` as a
cache for metadata obtained from the source, for example:

```ini
id = saphi:1
title = Drive Thru Danger
author = Boxic
modes = time_trial
```

Each field is optional. `modes`, when known, is a comma-separated list of Saphi
mode slugs for the installed LEV/VRM revision (for example `time_trial,arcade`).
It describes source data, not modes already implemented by this port. If it is
absent, the source modes are unknown; Time Trial remains selectable. If it is
present without `time_trial`, the track is not offered in Time Trial. Saphi's
API attaches mode tags to individual downloads, so importers must derive this
list from the actual files and revision, rather than from the track name alone.
When present, the author is shown above the selected
custom track. IDs must be unique. Lines
starting with `#` or `;` are comments. Duplicate or unknown keys are rejected.
The cache does not specify filenames: importers place files under the names
above. The format and local identity policy remain subject to verification
against actual track files and the import workflow.

## Installing a local directory

Copy the prepared track directory into `assets/tracks/` and restart the game.
The files must use the names listed above. After the library scan, enabled
tracks appear in the Time Trial, Arcade and VS single-race selectors when their
respective compatibility requirements are met.

In **Options > Custom Tracks > Available Tracks**, imported tracks can be
enabled or disabled. The selection persists in a `.ctr-disabled` marker in the
track directory. Each track shows its author and automatically detected mode hints
under **Available modes**. The installed LEV is examined on import and on each rescan,
so replacing it does not leave a stale detection. Detection reports Time Trial and
VS as a baseline, Arcade when at least one nonempty, structurally valid bot path
is present, Relic
when at least one time crate is placed, CTR when all three letters are placed,
and Crystal when crystals are placed. An unreadable or unsupported instance
table is shown as Unknown. These are heuristics, not proof that a track is
playable: they do not check reachability, bot path behavior, finish logic or relic targets.
VS is available for every installed track; Arcade requires a detected bot path.
Unknown detection is not offered in Arcade. Time Trial still follows the declared
source modes when present. Relic, CTR and Crystal hints do not enable those
custom modes in the port. Multiplayer uses the imported 1P LEV and allocates
separate visibility lists for each viewport.

Alternatively, select **Options > Custom Tracks > Import > Folder** to choose a folder
in the system dialog. It must contain one LEV and one VRM, and may also contain
one SCA, one JPEG preview and `track.ini`. File names can vary; multiple files
of the same type are rejected. The folder name supplies the destination and
fallback identity. The game copies the files and refreshes the library.
The importer checks LEV and VRM structure but cannot guarantee gameplay
compatibility, and will not overwrite an existing destination or identity.

## Importing a local xdelta patch

Select **Options > Custom Tracks > Import > Xdelta patch**, then select a local
`.xdelta` file in the system file dialog.

The importer needs `assets/ctr-u.bin` in the same raw NTSC-U MODE2/2352 format
used by the game, and the patch must target that base revision. 

The job runs in the background:

1. Create `assets/.import-xdelta-<unique ID>/` and copy the base ROM there.
2. Run `xdelta3 -d -s source.bin selected.xdelta patched.bin`.
3. Extract `BIGFILE.BIG` from both images using an independent ISO directory
   reader, then extract both BIGFILE archives with CTR-tools `bigtool`.
4. Compare the 18 retail circuit slots' `1P/data.lev` and `1P/data.vrm` pairs.
   A change in either file selects the complete patched pair. Unchanged tracks,
   relic/split-screen variants, arenas, hubs, overlays and executable patches
   are not imported.
5. Inspect the patched executable and, for supported TrackROM loaders, its menu
   overlay to associate each circuit with its PS1 logical level. Read that
   level's bank and sequence from `SOUNDS/KART.HWL` and package their ADPCM
   samples and CSEQ as `track.sca`, without re-encoding.
6. Validate all selected pairs with the shared local importer, then install
   them under `assets/tracks/<patch name>-<original slot>/`. A patch that changes
   several slots produces several entries, including distinct arcade/Time Trial
   variants when the patch stores those in different slots. Title and identity
    use the destination name. When exactly one circuit was modified, the
    importer looks for an unambiguous NUL-terminated `created by` credit in the
    patched menu overlay, executable and circuit LEV, and records it as the
    author. Missing, invalid or conflicting credits leave the author unknown;
    multi-circuit patches do not inherit a global credit. Declared modes remain
    unknown; mode hints are detected from the imported 1P LEV.
7. Remove the temporary ROMs, BIGFILE archives and extracted files. On a failed
   multi-track installation, newly published entries are rolled back; existing
   tracks are never overwritten. Cleanup errors are reported explicitly.

This requires enough temporary disk space. The music recognizer supports retail
loading and the observed NTSC-U TrackROM loader patterns, not arbitrary PS1
executable modifications. It rejects ambiguous associations, unsupported HWL
data and incompatible shared sample IDs. The import reports how many tracks
were imported without music; those tracks use the existing audio fallback.
Runtime SPU capacity and character-bank compatibility are checked again when the
track loads. XA audio and custom PS1 code are not imported.

### Local tool setup

Building the Windows game automatically runs the dependency setup before the
game target. The first build downloads the standalone tools into `assets/tools/`;
subsequent builds verify their hashes locally and need no network when all
installed files are intact. Downloaded archives and license texts are cached in
`build/_deps/track-tools/` (or the selected build directory) for offline repair.
Building only the test targets does not download tools.

Windows release archives include the verified tools, license texts and the
CTR-tools source archive. Packaging validates them offline and excludes ROMs,
track assets and download caches. Importing in-game never downloads dependencies.

The setup can also be invoked manually:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup-track-tools.ps1
```

The script downloads SHA-256-verified precompiled releases of xdelta3 **3.2.2**
and CTR-tools **r14**, installing `bigtool.exe` and its bundled DLLs without
compiling them or downloading development packages. Dependency licenses and
the r14 source archive accompany the tools in `assets/tools/`.
Running `bigtool` requires .NET Framework 4.6.2
or newer. Use `-Assets <path>` when preparing a separate game installation.
Tools are separate executables, not libraries linked into the game. The build
does not install the .NET Framework itself. Use `-VerifyOnly` to check the
installation without downloading anything, or `-Cache <path>` to select a cache.

Automated tests use synthetic disc and circuit fixtures. An opt-in integration
check uses a supplied retail ROM and tools. It extracts the original tracks,
generates a local xdelta patch that swaps two retail circuit pairs, and imports
that patch into a disposable scratch library. It checks the imported files
against the original extracted LEV/VRM, verifies that the base ROM is unchanged,
and removes the scratch files. No community tracks or downloaded patches are
used as test fixtures:

```text
build/tests/test_track_patch.exe --retail-live <absolute-assets-path>
```

## Time Trial records and runtime IDs

Lap counts are controlled by the game mode, not track metadata. The initial
Time Trial implementation uses the standard three laps.

Time Trial records for custom tracks are stored in `custom-records/` next to
the game. Each record is associated with the track identity and hashes of both
LEV and VRM, so a changed revision starts a separate table. The one-lap debug
option uses session-only records and does not modify three-lap records.
The best human ghost is saved separately under `custom-records/ghosts/` with
the same track identity and revision. It is loaded when selecting the track.

Custom runtime IDs currently occupy `0x100..0x7fff`: the load request interface
uses signed 16-bit IDs. The runtime ID is not a persistent identity. This range
alone does not make a custom level loadable: direct retail-table indexing,
special level ranges, ghost/card storage and the BIGFILE queue still require
explicit handling before a custom ID can enter a race.

Scanning reads metadata, checks file existence and type, and examines the LEV
instance table for mode hints. It does not validate gameplay, VRM or SCA
contents. The scanner returns a new registry,
which the caller owns, and reports accepted and rejected directory counts.
The game scans `assets/tracks` at startup and logs accepted and rejected counts
when the directory exists. Time Trial, Arcade and VS support locally imported tracks.
