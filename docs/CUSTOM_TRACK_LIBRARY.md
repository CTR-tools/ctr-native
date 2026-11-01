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
```

Each field is optional. When present, the author is shown above the selected
custom track. IDs must be unique. Lines
starting with `#` or `;` are comments. Duplicate or unknown keys are rejected.
The cache does not specify filenames: importers place files under the names
above. The format and local identity policy remain subject to verification
against actual track files and the import workflow.

Lap counts are controlled by the game mode, not track metadata. The initial
Time Trial implementation uses the standard three laps.

Custom runtime IDs currently occupy `0x100..0x7fff`: the load request interface
uses signed 16-bit IDs. The runtime ID is not a persistent identity. This range
alone does not make a custom level loadable: direct retail-table indexing,
special level ranges, ghost/card storage and the BIGFILE queue still require
explicit handling before a custom ID can enter a race.

Scanning reads the metadata and checks file existence and type; it does not
open or validate LEV, VRM or SCA contents. The scanner returns a new registry,
which the caller owns, and reports accepted and rejected directory counts.
The game scans `assets/tracks` at startup and logs accepted and rejected counts
when the directory exists. This is an initial internal format: import UI and
race loading are not implemented yet.
