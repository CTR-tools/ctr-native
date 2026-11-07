# Language selector implementation plan

Status: implementation plan for [issue #89](https://github.com/thecodingbob/ctr-native/issues/89). The detailed file syntax and remaining technical decisions are provisional. No game content from retail discs will be distributed.

## Goals

- Add System > Language with separate Audio and Text rows, each opening a dynamically populated picker like the Controls device picker.
- Default both selections to English and apply changes immediately. Discover English from the base image, additional available languages from an optional European image, and custom languages or overrides from `assets`.
- Resolve content per language and per resource: base English, then European disc content where available, then matching asset overrides. Asset files can replace individual existing strings or audio resources. Fork-only strings come exclusively from assets.
- Move the fork's custom English strings into the agreed asset format, and document how to create translations and audio overrides.
- Evaluate disc-backed loading against extracting resources ahead of time, with measurements, before deciding the production approach.

## Current starting points

- `game/MAIN/MainMain.c` loads English bigfile slot 1 at startup. `game/LOAD/LOAD_Assets.c` loads a language file into the game's memory arena and fixes up its string pointers. The comment there lists candidate retail slots, not a verified inventory for each disc.
- `game/CDSYS.c` selects XA language paths and loads their XNF metadata. Audio coverage, other audio paths, and any language-dependent video or subtitles must be inventoried before promising coverage.
- `platform/native_disc_image.c` assumes one NTSC-U `ctr-u.bin`; `platform/native_assets.c` indexes extracted assets and gives them precedence over disc files. A second disc must not silently replace unrelated base-game content.
- `game/230/MM_ConfigMenu.c` owns System navigation; `game/230/MM_ControlsConfig.c` contains the existing device picker. Configuration is table-driven in `platform/native_config.c`.
- Initial ISO directory inspection found `assets/ctr-u.bin` (volume `SCUS-94426`) and `assets/ctr-e.bin` (root executable `SCES_021.05`). Both have `BIGFILE.BIG` and an XA directory, but their bigfiles have different LBAs and sizes. The US XA directory contains `ENG` and `ENG.XNF`; the European directory contains `DCH`, `ENG`, `FRN`, `GRM`, `ITL` and `SPN`, each with an XNF, plus music. Directory presence alone does not establish usable text or voice coverage. European language slots, individual XA files, compatibility and source-specific sector addressing still need verification.

## Proposed resource contract (to agree)

Prefer separate `assets/text/<language-id>/` and `assets/audio/<language-id>/` directories so text and audio packs can be discovered independently. Language IDs must be stable across both trees; the mechanism for display names and any manifests remains to be defined. A UTF-8, human-editable keyed text file supports partial translations; keys should be stable symbolic identifiers rather than display strings. Retail keys need a documented mapping to retail string indices; fork keys must have their own namespace. Audio overrides should use stable resource identifiers and a documented file format, with per-resource replacement rather than requiring a complete disc. Target voices first and design the replacement framework to be extensible to other asset types; do not imply other audio categories are already supported. Exact syntax, accepted codecs and encoding conversion are open until the disc and rendering/audio paths have been examined.

Use TOML for editable text catalogs and the selected [tomlc17](https://github.com/cktan/tomlc17) parser. Pin a revision, retain its license, and verify its supported TOML version, malformed-input handling, UTF-8 and memory ownership. Test both MSVC x86 and MinGW i686 and the unity-build integration before relying on it.

Discover text and audio availability independently from valid resources, not merely from a directory name. Merge sources deterministically for each selected language. Missing retail resources and fork-only translations fall back to English, including for partial packs. Validate invalid files and unknown keys without corrupting the resident language table. Keep override buffers alive as long as game pointers can reference them, and account for arena resets and checkpoints.

## Work breakdown

1. Inspect `assets/ctr-e.bin`; compare its ISO directory, bigfile language records and XA metadata to the base disc. Determine actual languages and content coverage, distinguish text slots from XA language IDs, and check image validation and raw-sector assumptions. Inventory fork-authored strings and text/font limitations, including supported accented characters and formatting tokens.
2. Trace text loads, XA and other voice playback, CD addressing, asset overrides, memory ownership, checkpoints and configuration/menu lifecycle. Design immediate reload and stream switching, including interactions with active streams, save states and extracted base assets.
3. Agree on the asset contract and fallback semantics; document discovery, keys, file encoding, text escaping/format placeholders, audio format and examples, plus error handling and content ownership. Provide a shipped English file for fork strings without including proprietary retail content.
4. Implement a source-aware language inventory and separate optional European disc access, maintaining existing base disc and extracted asset behavior. Implement deterministic overlays, safe resource lifetime and fallback; avoid treating all European disc files as base-game replacements.
5. Add persistent independent audio/text selections and dynamic pickers under System > Language. Apply confirmed changes immediately; handle missing previously selected languages, empty or invalid packs, navigation, scrolling and cancellation. Update the default configuration and user documentation.
6. Migrate fork strings and their consumers to the asset-backed catalog, including menu labels and any non-retail strings identified in the inventory. Verify placeholders, fonts, length and layout; do not assume every hard-coded string is player-facing.
7. Add focused tests for discovery, precedence, partial overrides, malformed files, persistence and fallback, plus integration coverage for both discs and individual audio overrides. Manually verify menu behavior and audio/text independently across relevant game states, with and without European disc and extracted assets.

## Performance decision gate

Compare reading only requested language resources directly from the images against one-time extraction into the proposed format. Measure startup and language-switch latency, race/menu transitions, audio interruptions, memory and disk usage, and behavior on representative storage. Include cold and warm runs and validate playback correctness, not just throughput. Consider cache invalidation and user-supplied disc ownership. Prefer runtime disc access if measured costs are acceptable; otherwise document an extraction/cache strategy and its invalidation. No performance conclusion is claimed yet.

## Open decisions

- What exact keyed text syntax and audio format best fit the existing rendering and audio paths?
- How should the optional European image be located, and how should unsupported or missing images and selections be reported?
