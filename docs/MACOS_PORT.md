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
- **Current state:** the arm64 binary builds, links, launches, and boots/idles
  stably against real NTSC-U assets — the model pack, level, and driver-model
  relocation gaps (Milestone M3) are all fixed. Driving an actual race
  (Milestone M4) hasn't been verified yet; that's the next manual play session.

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

### M3 — Asset pointer relocation (done — Option A adopted)

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

**Phase 2 (level + instances) — done, committed.** `Reloc64_Level`
(`platform/native_reloc.c`) rebuilds `struct Level` and its full graph (PVS, BSP
tree + hitbox array, QuadBlock array, mesh info, skybox, water verts, spawn
types, nav header/table, vis mem, InstDef array/ptr array, AnimTex chains).
Wired into `LOAD_Callback_PatchMem`. Boots and idles stably; not yet exercised
by an actual race load.

**Phase 3 (individually-loaded driver models) — done, committed.** The `-2`
sentinel callback (`LOAD_DriverMPK_SetPointer`) queues per-character driver
model files through the same MPK format as the main model pack, but
dispatched to `LOAD_DramFileCallback` differently — that branch only matched
`LOAD_Callback_DriverModels`, so these files fell through to the truncating
`LOAD_RunPtrMap`. Fixed by also matching the `-2` sentinel and routing through
`Reloc64_ModelPack`/`Reloc64_MpkModels`; `driverModelExtras` widened to
`uintptr_t[3]` (`VehBirth_GetModelByName` reinterprets its address as
`struct Model**`, needing a real 8-byte stride). Also fixed a related bug:
`LOAD_ReadFile_ex` always returns the raw pre-relocation buffer it allocated,
discarding the rebuilt native pointer `Reloc64_ModelPack` stores into
`data.currSlot.ptrDestination` — the `-2` path's result capture in
`LOAD_DramFile` now re-reads that field under `CTR_RELOC64`.

**Phase 4 (the actual post-splash freeze) — done, committed.** Phases 1-3
above were verified by "boots and idles," which never drove the loader far
enough to hit any of this. Driving a real load past the splash screen
surfaced three more bugs, all found by attaching `lldb` to the "frozen"
process rather than guessing — it wasn't a hang, the main thread was
permanently parked in `MEMPACK_AllocMem`'s retail OOM halt
(`CTR_ErrorScreen(0xFF,0,0); for(;;){}`, see `game/MEMPACK.c`), which looks
identical to a frozen window from the outside (no more frames presented) but
shows up immediately in a backtrace:
- `ModelAnim.numFrames`' top bit is an interpolation flag (retail masks it
  with `& 0x7fff` at every read site — `INSTANCE.c:407`, `CS_Instance.c:155`,
  `VehFrame.c:44`, `RenderBucket_QueueExecute.c:1876`). `Reloc64_ModelAnim`
  read it raw, so any compressed animation sized its frame-data allocation in
  the megabytes instead of low hundreds of bytes.
- Normal race tracks load their LEV through `LOAD_Callback_LEV`, not
  `LOAD_Callback_PatchMem` (that path is Adventure-Hub-only). It was never
  wired into the `CTR_RELOC64` dispatch, so it fell through to the
  truncating `LOAD_RunPtrMap` — `sdata->ptrLevelFile` ended up misread at
  native struct offsets, crashing in `LibraryOfModels_Store` with garbage
  `numModels`/NULL `ptrModelsPtrArray`.
- The native MEMPACK arena was pinned to retail's exact ~1.3 MiB MPK window
  (`CTR_NATIVE_MEMPACK_RETAIL_PRESSURE`). That budget assumed retail's packed
  4-byte pointers; Option A's rebuilt native structs (8-byte pointers, plus
  the original file buffer staying resident alongside the rebuilt spine) need
  more headroom for a whole level's instance/model/animation graph — not a
  decode bug, a real budget increase. Defaulted off on `__LP64__`/`_WIN64`.

Also added a sanity cap in `Reloc64_Alloc`: since `MEMPACK_AllocMem` can't
report OOM (it halts forever instead), a future bug of this shape would
otherwise burn the whole pool with no diagnostic.

**Remaining:** none known in the asset-relocation (Option A) work itself. Two
issues found during Phase 3's investigation are still tracked as separate
follow-up work, not part of M3:
- `game/INSTANCE.c` (`INSTANCE_LevInitAll`) copies `InstDef` fields into
  `Instance` via a raw `int*` blit at hardcoded retail byte offsets/strides —
  corrupts every placed level instance on 64-bit once a real track loads
  instances (pointer fields grow 4→8 bytes, shifting every field after them).
  Needs a field-by-field assignment instead of the blit.
- `struct InstDrawPerPlayer`'s `ptrCommandList`/`ptrColorLayout`/
  `ptrDeltaArray` fields are truncating (see M4 below).

With Phase 4's fixes, a real track now loads all the way to driver/vehicle
spawn and beyond. The `VehBirth_Player` crash and five more along the same
crash-driven chain are fixed (one commit each):

- `PROC_BirthWithObject` truncated `th->object` (`(u32)stackObj` instead of
  `(uintptr_t)stackObj`, inconsistent with three other casts in the same
  function) — the actual `VehBirth_Player` `memset` crash.
- `JitPool_Add` returned `(int)item` instead of a real pointer — corrupted
  every instance/thread spawned from a JitPool (next hit:
  `INSTANCE_Birth3D`).
- `Reloc64_InstDefPtrArray` wasn't NULL-terminated; consumers walk it as a
  NULL-terminated list, so they read one slot past the array into heap
  garbage (`LevInstDef_UnPack`). Mirror of the already-correct
  `ptrModelsPtrArray` pattern.
- `CutsceneObj.frameOverrideRoot` read only the low 32 bits of an 8-byte
  pointer (retail aliases it onto a struct's leading 4-byte field) — hit
  during a kart's intro cutscene.
- `UI_INSTANCE_BirthWithThread` carried a function pointer, a name string,
  and a `PushBuffer*` through `int` parameters, truncating all three at
  every one of 13 call sites — HUD/pickup-display init.

All found via the same technique: attach `lldb` at the crash (or set a
breakpoint just before it once the failure mode is understood), inspect the
actual pointer value, recognize it as a 32-bit-truncated address, find the
narrow field/parameter upstream.

**Next crash (start of M4, paused here deliberately):** with all of the
above fixed, the game reaches `MainFrame_RenderFrame` — the actual per-frame
render path — before crashing in `RenderBucket_CopyScratchColorCache`
reading `ctx->idpp->ptrCommandList`. This is `struct InstDrawPerPlayer`'s
already-known truncating fields (`ptrCommandList`/`ptrColorLayout` are
`u32`, `ptrDeltaArray` is `int`) — see M4 below. Tracing the write side shows
this cascades further: `RenderBucket_GetFrame`'s `deltaArrayOut` parameter is
`int *`, and `ModelHeader.unk3` (aliased the same way as `ptrDeltaArray`) is
also still `u32`. Deliberately stopped here rather than starting M4's wider
scope unannounced — picking up M4 means widening this whole field/parameter
cluster across `RenderBucket_QueueExecute.c` and `AH_WarpPad.c`, not just the
one struct.

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
