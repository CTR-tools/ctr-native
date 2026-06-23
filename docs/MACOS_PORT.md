# macOS (Apple Silicon) Port — Status & Roadmap

This document tracks the work to run CTR-Native on macOS / Apple Silicon
(arm64), the milestones reached so far, and the concrete path to a playable
build. It is the macOS-specific companion to `docs/MEMORY_MODEL.md`, which
explains the underlying memory model and the two "Known Gap" sections referenced
below.

## TL;DR

- The Windows and Linux builds are **forced 32-bit** (`-m32`) so that pointers
  are 4 bytes and match the PS1-shaped, on-disc data layouts the decompiled game
  code assumes.
- macOS has no 32-bit support, so the macOS build is **64-bit arm64**. Every
  place the retail code treats a pointer as a 32-bit value has to be made
  pointer-width-correct.
- **Current state:** the arm64 binary builds, links, launches, and runs ~16 s of
  startup (audio, init, early rendering) against real NTSC-U assets, then stops
  at the model/level loader. That last stop is the one genuinely *architectural*
  task left (see Milestone M3).

## Why this is hard

The game code is decompiled PS1 retail source. On the PS1 a pointer is 32 bits,
and the on-disc file formats (BIGFILE entries, MPK model packs, level files,
HOWL audio) are **binary overlays** that embed 32-bit pointers directly. The
32-bit desktop builds work because `sizeof(void *) == 4` there, so the C structs
match the files byte-for-byte and the retail pointer arithmetic is lossless.

On a 64-bit host two distinct problems appear:

1. **Pointer-width truncation (mechanical).** Retail code that stashes a pointer
   in an `int`/`u32`, walks a buffer with `(int)ptr + n`, or hardcodes a struct
   size, silently drops the high 32 bits. Fix: widen the *arithmetic* to
   `uintptr_t` (which is 4 bytes on the 32-bit builds, so they are unaffected)
   or use `sizeof(...)` instead of a retail literal. These are the fixes that
   got us from "instant crash" to "16 s of boot".

2. **On-disc 4-byte pointers (architectural).** Pointer *tables* and pointer
   *fields inside* loaded assets are 4 bytes on disc. A 64-bit host cannot store
   its addresses in 4 bytes, and the obvious escape — mapping all game memory
   below 4 GiB so addresses fit — is **impossible on macOS**: the kernel
   `SIGKILL`s any arm64 binary with a non-default `__PAGEZERO`, and with the
   default 4 GiB pagezero the entire low 4 GiB is reserved. This is Milestone M3.

## Build & run

```sh
./build_macos.sh           # configures CMake into build/, builds build/ctr_native
```

Requires Xcode Command Line Tools and CMake (`brew install cmake`).

### Assets

The game needs NTSC-U retail assets under `assets/` (next to `build/`, or in the
repo root — the loader checks the executable dir then its parent). Extract them
from a CTR **NTSC-U** disc image (`BUILD=926` is UsaRetail; a PAL disc will not
match). A PSX `MODE2/2352` `.bin` can be unpacked with a small ISO9660 reader:
Form1 files (`BIGFILE.BIG`, `SOUNDS/KART.HWL`, `TEST.STR`, `XA/ENG.XNF`) as 2048
user-bytes/sector, and `.XA` files as raw 2352-byte sectors (the audio layer
accepts 2352 or 2336; the CD layer reads 2048-byte sectors). The disc's
directory layout matches the loader's expected paths exactly.

### Debugging recipe

The fastest loop is crash-driven, because each pointer bug surfaces as a clean
`EXC_BAD_ACCESS` at a truncated address:

```sh
cmake --build build -j
./build/ctr_native            # let it crash
```

macOS writes a fully symbolicated report to
`~/Library/Logs/DiagnosticReports/ctr_native-*.ips`. The triggered thread's
backtrace names the exact `file:line`, and the faulting address is almost always
a recognisably truncated pointer (low 32 bits only, or two 4-byte entries
concatenated). `lldb -b -o run -o bt ./build/ctr_native` also works but the
`.ips` report is more reliable for late crashes.

## Milestones

### M0 — Build system (done)

`build_macos.sh`, the `APPLE` branches in `CMakeLists.txt`, and a 64-bit arm64
configuration. No `-m32`, no MIPS toolchain.

### M1 — Compiles & links (done)

The blocker was ~546 `offsetof`/`sizeof` layout asserts that bake in retail
4-byte-pointer offsets and (correctly) fail once a struct embeds an 8-byte host
pointer. All such layout asserts now go through `CTR_STATIC_ASSERT_LAYOUT`
(`include/platform/native_static_assert.h`, force-included via `CMakeLists.txt`):
a real `_Static_assert` on the 32-bit builds (which stay the byte-parity
guardians) and a no-op on `__LP64__`. Value asserts (enum/flag constants — no
`offsetof`/`sizeof`) stay as plain `_Static_assert` everywhere. See the second
"Known Gap" in `docs/MEMORY_MODEL.md`.

### M2 — Boots into asset loading (done)

A chain of pointer-width-truncation fixes, each verified by re-running to the
next crash. All are `uintptr_t`/real-pointer widenings or `sizeof`-instead-of-
literal changes, identical in behaviour on the 32-bit builds:

- `MEMPACK.c` — arena arithmetic and `MEMPACK_NewPack`'s `(u32)start`.
- `LOAD_LangFile` — pointer-typed signature; the on-disc 4-byte string-offset
  table is resolved into a native `char *[]` on 64-bit instead of being rewritten
  in place.
- `HOWL_Load` header parse walks; `HOWL_Bank` SPU source pointer.
- `HOWL_Voiceline` — `LIST_Init` pool strides via `sizeof`, not retail `0x20`/`0x10`.
- `MainInit` — JitPool free-list walk follows real `Item` links and seeds the
  payload self-pointer at the true (header-relative) offset.
- `MainFrame` — `otSwapchainDB` local kept at pointer width.
- `ptrMPK` retyped from `int` to a real pointer (`regionsEXE.h` + callback).
- A sweep of `(T *)((int|u32)ptr …)` truncating casts across `game/`.

### M3 — Asset pointer relocation (in progress — Option A adopted)

**Approach chosen: Option A (load-time transform).** A native-only module
(`platform/native_reloc.c`, `#ifdef CTR_RELOC64`) rebuilds each binary-overlay
asset into native structs with real 8-byte pointers right after load, so every
`game/` dereference site stays byte-identical to retail. The 32-bit Win/Linux
builds keep `LOAD_RunPtrMap` and compile none of it.

**Phase 1 (model pack / MPK) — done and verified.** The boot used to fault in
`LibraryOfModels_Store` walking the MPK model-pointer table; it now boots past it
and runs stably (renderer up). `Reloc64_ModelPack` rebuilds the `PLYROBJECTLIST`
table and the full `Model → ModelHeader → {TextureLayout[], ModelAnim[], AnimTex}`
graph plus the `mpkIcons → LevTexLookup → IconGroup` icon graph; leaf blobs
(command lists, vertex/frame data, CLUTs) stay resident in the original buffer
and are pointed at directly. Length-less pointer arrays (e.g. `ptrTexLayout`) are
bounded by the embedded DRAM pointer-map; arrays with a stored count
(`numHeaders`, `numAnimations`, `numIconGroup`) use it. Hooked in
`LOAD_DramFileCallback` (dispatched by `callback == LOAD_Callback_DriverModels`);
the few raw-offset reads (`ptrMPK+4`, `*ptrMPK`, `mpkIcons+4`) now go through
`Reloc64_Mpk*` accessors / struct fields. `ModelHeader.ptrCommandList` and
`gGT->mpkIcons` were widened to `uintptr_t`.

**Remaining (Phase 2/3):** the level format (`struct Level` via
`LOAD_Callback_PatchMem`) + instances, then the individually-loaded driver models
(`driverModelExtras`, the `-2` SetPointer path) and any other overlays, found via
the crash-driven loop. Same pattern, new walkers.

Background on why this is the architectural task — the MPK (and level) data are
binary overlays whose **on-disc pointers are 4 bytes**:

- Pointer *tables* (e.g. `PLYROBJECTLIST = ptrMPK + 4`) are arrays of 4-byte
  entries; reading them as native `struct Model **` (8-byte) is a stride
  mismatch (a fault address of two 4-byte entries glued together is the tell).
- Pointer *fields inside* structs shift: `struct Model.headers` is a pointer at
  retail offset `0x14`; at 64-bit it is 8 bytes and the compiler realigns it to
  `0x18`, so the C struct no longer matches the file.
- `LOAD_RunPtrMap` relocates these 4-byte slots with a truncating
  `*(int *)&origin[off] += (int)origin`.

This cannot be solved by widening (4-byte slots cannot hold 64-bit addresses)
nor by address placement (the pagezero constraint above). It needs one of:

- **Option A — load-time transform (recommended).** Per asset format (model,
  level, instance, …) write a deserializer that, right after load/relocation,
  rebuilds the data into native structs with real 8-byte pointers. Access sites
  stay unchanged; the cost is one transform per format. Most maintainable.
- **Option B — 4-byte handles + accessors.** Keep on-disc fields 4 bytes, change
  every loaded-asset pointer field to a handle type, and resolve it against the
  asset base at each dereference (mirrors the GPU OT-link token bridge already
  used in `native_gpu_links.c`). Touches far more call sites.

Both are scoped work, not one-line fixes. `LOAD_RunPtrMap` should be addressed as
part of whichever option is chosen (Option A relocates into the rebuilt struct;
Option B keeps file-relative offsets and never adds `origin`). See the first
"Known Gap" in `docs/MEMORY_MODEL.md`.

### M4 — Playable

With M3 done, drive past the loader into menus and a race. Expect further
truncation/layout issues in the per-frame render and instance paths
(`RenderBucket`, `InstDrawPerPlayer`, etc.) — the same two bug classes, found the
same crash-driven way.

### M5 — Distribution

Ad-hoc `codesign` is already applied by the linker. For shipping: a proper
signed/notarised `.app` bundle, a Universal binary if x86_64 is also wanted, and
bundling `assets/` per the layout in `README.md`.

**Longevity risk — OpenGL → Metal.** The native renderer
(`platform/native_renderer.c`, `native_glad.c`) uses an OpenGL 3.3 Core context.
Apple deprecated OpenGL in macOS 10.14 and caps it at 4.1 (we run on Apple's
"4.1 Metal" GL-over-Metal shim — see the boot log). It works today and is not a
"does it run" blocker, but Apple could remove it. The long-term escape is to move
the backend onto Metal — either via SDL3's `SDL_GPU` abstraction or by translating
GL through ANGLE/MoltenVK. This is a renderer concern, fully orthogonal to the
64-bit memory work above; flagged here so it is on the distribution radar.

## Conventions for new 64-bit fixes

- Prefer `uintptr_t` for pointer↔integer arithmetic; it is 32-bit on the
  `-m32` builds, so they keep retail behaviour exactly.
- Never hardcode a struct/pool size that contains a pointer — use `sizeof`.
- Keep edits in `game/` minimal and shaped like the surrounding retail code;
  gate genuinely native-only divergence behind `__LP64__` (or `CTR_NATIVE`),
  leaving the 32-bit path untouched so it remains the parity reference.
- Re-run after each fix and read the `.ips` backtrace; a truncated faulting
  address pinpoints the next site.
