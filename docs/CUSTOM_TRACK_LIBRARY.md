# Custom track library: initial disk format

The library scanner accepts one directory per track. Each directory contains
`track.lev` and `track.vrm`; `track.sca` is optional. For example:

```text
tracks/
  example/
    track.lev
    track.vrm
    track.sca
```

No metadata file is required. Without one, the directory name is the title and
`local:<directory name>` is the identity; renaming the directory changes that
identity. A future importer can write `track.ini` as a
cache for metadata obtained from the source, for example:

```ini
id = saphi:1
title = Drive Thru Danger
```

Each field is optional. IDs must be unique. Lines
starting with `#` or `;` are comments. Duplicate or unknown keys are rejected.
The cache does not specify filenames: importers place files under the names
above. The format and local identity policy remain subject to verification
against actual track files and the import workflow.

Lap counts are controlled by the game mode, not track metadata. The initial
Time Trial implementation uses the standard three laps.

Scanning reads the metadata and checks file existence and type; it does not
open or validate LEV, VRM or SCA contents. The scanner returns a new registry,
which the caller owns, and reports accepted and rejected directory counts.
This is an initial internal format: import UI, automatic scanning and race
loading are not implemented yet.
