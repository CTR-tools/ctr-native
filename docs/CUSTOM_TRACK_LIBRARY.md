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
The files must use the names listed above. The track appears in the Time Trial
selector after the library scan at startup.

In **Options > Custom Tracks > Available Tracks**, imported tracks can be
enabled or disabled. The selection persists in a `.ctr-disabled` marker in the
track directory. Each track shows its author and declared supported modes under
**Game modes**, or **Unknown** when no mode metadata is available. Time Trial
is the only currently implemented custom mode.

Alternatively, select **Options > Custom Tracks > Import** to choose a folder
in the system dialog. It must contain one LEV and one VRM, and may also contain
one SCA, one JPEG preview and `track.ini`. File names can vary; multiple files
of the same type are rejected. The folder name supplies the destination and
fallback identity. The game copies the files and refreshes the library.
The importer checks LEV and VRM structure but cannot guarantee gameplay
compatibility, and will not overwrite an existing destination or identity.

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

Scanning reads the metadata and checks file existence and type; it does not
open or validate LEV, VRM or SCA contents. The scanner returns a new registry,
which the caller owns, and reports accepted and rejected directory counts.
The game scans `assets/tracks` at startup and logs accepted and rejected counts
when the directory exists. Time Trial supports loading custom tracks; the
online catalog is still to come.
