# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A native PC port of Crash Team Racing (PS1, 1999), built on top of the
[CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) decompilation project.
`game/` holds editable copies of the decompiled retail source; the rest of the
repo is native platform glue that lets that source run on Windows/Linux via
SDL3 instead of PS1 hardware/PsyQ.

Guiding constraints (see README.md "Philosophy" for the full list):
- No PSX toolchain — targets Windows/Linux, no MIPS compiler.
- `main.c` owns process startup; host-specific details live in `platform/native_*`.
- Fully static build, single executable, SDL3 vendored and statically linked.

## Build

```sh
./build.sh           # Linux — requires gcc-multilib, X11/GL/ALSA/dbus dev headers (see README.md)
build.bat             # Windows — requires MSYS2 mingw32 toolchain on PATH (see README.md)
```

Both scripts wrap CMake: configure into `build/`, then `cmake --build build`.
First build compiles SDL3 from vendored source (cached in `build/`); incremental
builds only recompile touched native sources. Because `main.c` `#include`s
nearly everything (see Architecture below), touching almost any `game/*.c` or
`platform/native_*.c` file triggers a full relink of that one translation unit.

Output: `build/ctr_native` (Linux) or `build/ctr_native.exe` (Windows).

Clean build: delete `build/` and rerun the build script.

There is no test suite, linter, or single-file test command in this repo —
verification is "does it build and run." Use `/run` (or manually run the built
binary against real game assets, see README.md "Running") to check behavior.

## Architecture

```
main.c (entrypoint)
  +-- platform/native_* (platform shell, audio, input, memcard, CD, renderer, PSX facade glue)
  +-- game_includes.h
        +-- game/ (all decompiled game source, ~940 files)
              +-- include/ (headers: structs, globals, declarations)
```

### Unity build via textual #include

`main.c` does not link separate translation units in the usual sense — it
`#include`s `game_includes.h`, which in turn `#include`s every `game/*.c` file
directly (a curated unity build), followed by `platform/native_*.c` files.
`game_includes.h`'s include order matters and mirrors retail overlay grouping
(see Overlays below). When adding a new `game/` source file, it must be added
to `game_includes.h` or it will never compile.

### PSX compatibility layer vs. native platform layer

- `include/psx/*.h` and `include/psn00bsdk/` declare the original PSX/PsyQ API
  surface (`libetc`, `libgte`, `libgpu`, `libspu`, `libcd`, `libapi`) that
  `game/` code calls into unmodified.
- `platform/native_*.c` / `include/platform/native_*.h` implement that API
  surface (and the higher-level `Platform_*` functions in `platform.h`) on top
  of SDL3, OpenGL (`native_glad.c`/`native_gpu.c`), and host filesystem APIs.
  This is where "PS1 hardware concept" maps to "host concept" — e.g. GPU
  primitive submission, pad polling, memory card emulation, CD/asset streaming.
- `CTR_NATIVE` is the compile-time flag distinguishing native-only code paths
  from retail-shape code paths still present in `game/`. `CTR_INTERNAL` gates
  developer-only tooling (replay capture, perf overlay, etc.), not shipped to
  players.

### Retail memory shape

Game code still thinks in PS1 terms (fixed RAM addresses, overlays, 24-bit GPU
links, scratchpad). Read `docs/MEMORY_MODEL.md` before touching anything that
deals with `sdata`, `MEMPACK_*`, scratchpad offsets, or GPU primitive/OT links —
struct field widths and offsets are load-bearing because retail code depends on
addresses, not just field names. Key points:
- `include/regionsEXE.h` types the resident `.rdata`/`.text`/`.data`/`.sdata`/`.bss`
  ranges as structs (full address table in `docs/DATA_SECTIONS.md`).
- `sdata` (originally addressed via MIPS `$gp`) is mirrored as a plain global
  pointer: `struct sData *sdata = &sdata_static;`.
- GPU primitive/OT links use a 24-bit address + 8-bit size bitfield in retail.
  Native treats that 24-bit field as a bridge token resolved through
  `native_gpu_links.c`, not a truncated host pointer — never widen these
  packets to native pointer size.
- Enums backing PSX struct fields use fixed-underlying-type syntax
  (`typedef enum Name : s16`, `: u8`, etc.) so `sizeof` matches the retail
  field width.

### Overlays

CTR has 13 overlay binaries across 3 fixed-address overlay regions (only one
overlay per region is "resident" on real PS1 hardware at a time). Full table in
`docs/OVERLAYS.md`. In native builds, every overlay's C code is compiled
directly into the single binary (no runtime CD streaming of overlays) — guarded
by `#ifndef CTR_NATIVE` around the retail `LOAD_AppendQueue` calls — but game
logic still tracks which overlay index is logically resident per region via
`GameTracker`. Don't assume "compiled in" implies "logically active."

### Replays / bug reports

Internal (`CTR_INTERNAL`) builds can record and play back deterministic bug
reports (input + state). See `docs/REPLAYS.md` for the `--record`/`--replay`
CLI flags and `F5`/`F8`/`F9`/`F10` hotkeys. Reports live under `debug/reports/`;
quicksave state under `debug/states/`.

## Code style

- C99, `.clang-format` enforces LLVM base style with tabs for indentation
  (width 4), Allman-ish brace wrapping (`AfterFunction/AfterControlStatement/
  AfterStruct/AfterEnum/AfterUnion: true`), right-aligned pointers.
- `game/` files are decompiled retail code kept close to original shape —
  prefer minimal, targeted edits there over rewrites/refactors, since byte-exact
  structure aids comparison against retail behavior even though there's no
  byte-budget constraint anymore.
- New native-only code belongs in `platform/native_*.c` + `include/platform/
  native_*.h`, gated by `CTR_NATIVE` where it touches shared `game/` code paths.
