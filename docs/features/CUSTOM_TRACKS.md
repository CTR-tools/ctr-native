# Custom Tracks

CTR Native Expanded can import custom tracks from a local folder or attempt
to load one from an xdelta patch. Tracks and patches are not included with the game.

## Import a track

From the **main menu**, open **Options > Custom Tracks > Import**. 

### From a folder

Choose **Folder** and select the folder containing the track files. It must
contain exactly one `.lev` and one `.vrm` file. It can also contain one `.sca`
music file, one `.jpg` or `.jpeg` preview and an optional `track.ini` metadata
file. The files may have different names; if there is more than one file of
the same type, the importer asks you to resolve the ambiguity.

The game copies the files into its library and refreshes the track list. It
does not overwrite an existing track or accept a duplicate track identity.

### From an xdelta patch

Choose **Xdelta patch** and select a local `.xdelta` file. You need your own
NTSC-U retail disc image at `assets/ctr-u.bin`, in the raw MODE2/2352 format
used by the game. The patch must target that base revision. Keep enough free
disk space for temporary copies and extracted files.

Windows release packages include the required tools. CTR-tools also requires
**.NET Framework 4.6.2 or later**; the game does not install it. Windows source
builds set up the tools automatically. Patch import runs locally and can be
canceled with Triangle or Start. It never modifies the original disc image.

A patch can add more than one entry. The importer extracts changed race-track
assets, not arbitrary PS1 code, arena or hub modifications. Compatible music is
imported when possible; otherwise the game uses its normal audio fallback.
This way of importing is a best-effort hack, and is not guaranteed to work with
any xdelta game patch.

## Manage and play tracks

Open **Options > Custom Tracks > Available Tracks** to see imported tracks and
toggle them on or off. Disabling a track does not delete its files or records.
This list is also available from the pause menu; return to track selection to
choose an enabled track.

Use **Options > Custom Tracks > Refresh** from the main menu after adding,
removing or changing files directly in the library. Restarting the game also
rescans the library.

Enabled tracks appear alongside retail tracks in the compatible single-race
selectors:

- **VS:** available for every installed track.
- **Arcade:** requires a detected bot path. Tracks whose mode detection is
  unknown are not offered in Arcade.
- **Time Trial:** available for every installed track.

The manager displays detected mode hints: TT (Time Trial), ARC (Arcade), VS,
CTR, REL (Relic) and CRY (Crystal). These are best-effort hints, not a
guarantee that the track works. CTR, Relic and Crystal hints do **not** enable
those custom modes yet. Custom tracks are not added to cups or Adventure mode.

## Manual installation

As an alternative to the Folder importer, place each prepared track in its own
directory under `assets/tracks/`:

```text
assets/tracks/My Track/
  track.lev       required
  track.vrm       required
  track.sca       optional music
  preview.jpg     optional image
  track.ini       optional metadata
```

Then refresh the library or restart the game. Without `track.ini`, the folder name is 
the displayed title and identifies the track. Renaming that folder changes its identity, so
existing Time Trial records and ghosts will no longer be associated with it.

The optional metadata file can specify a stable ID and display information:

```ini
id = example:my-track
title = My Track
author = Track Creator
```

All fields are optional. IDs must be unique. Unknown or duplicate keys make the
manifest invalid. 

## Records, previews and troubleshooting

Custom Time Trial records are stored in `custom-records/` and ghosts in
`custom-records/ghosts/`, next to the game. Keep those folders to back up your
results. Records and ghosts are tied to the track identity and LEV/VRM file
contents: a changed revision starts a separate record. The one-lap debug option
does not save three-lap records or ghosts.

An optional `preview.jpg` is displayed in track selection; missing or unreadable
previews show **NO PREVIEW** instead. The game also tries to extract a minimap
from the track files. Neither is required to race.

- **Track missing from the list:** check the required files, enable the track in
  Available Tracks and refresh from the main menu. Check mode compatibility
  above and any import error message.
- **Patch import fails:** check the disc revision and format, required tools,
  .NET installation on Windows, free disk space and whether the destination or
  identity already exists.
- **Track crashes, is incomplete or runs poorly:** not every custom track is
  compatible with the port. Try disabling **Options > System > Graphics >
  Increase Draw Distance**. Import checks cannot guarantee playability.
- **Music is missing:** an SCA file or patch music may be unsupported or
  incompatible with the available audio resources; the track can still use
  the game audio fallback.
