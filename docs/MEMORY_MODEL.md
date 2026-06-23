# CTR Memory Model

CTR's runtime is built around the PlayStation 1 memory map. Native builds do
not run inside that hardware address space, but a lot of game code still relies
on the same allocation model, pointer packing, and scratchpad conventions.

## PS1 Address Spaces

| Region | Address Range | Size | Owner | Purpose |
|--------|---------------|------|-------|---------|
| Main RAM, cached | `0x80000000`-`0x801fffff` | 2 MiB | CPU | Executable, data, overlays, heap, game state |
| Main RAM, uncached | `0xa0000000`-`0xa01fffff` | 2 MiB mirror | CPU | Same physical RAM, uncached view |
| Scratchpad | `0x1f800000`-`0x1f8003ff` | 1 KiB | CPU | Fast temporary storage |
| VRAM | GPU-local | 1 MiB | GPU | Framebuffers, textures, CLUTs |
| SPU RAM | SPU-local | 512 KiB | SPU | Samples, music, voice data |

Retail code normally uses `0x80000000` addresses for main RAM. The address is a 32-bit CPU pointer, not a 24-bit pointer.
The 24-bit constraint appears in GPU primitive tags and ordering table links, not in ordinary CPU loads and stores.

## Resident Executable Memory

The main executable stays resident in RAM. `include/regionsEXE.h` maps its fixed data ranges as C structs.

See `docs/DATA_SECTIONS.md` for the full table. The NTSC-U retail executable has
these relevant ranges:

| Section | Address Range | Native Symbol |
|---------|---------------|---------------|
| `.rdata` | `0x80010000`-`0x800123df` | `rdata` |
| `.text` | `0x800123e0`-`0x8008099f` | function symbols |
| `.data` | `0x800809a0`-`0x8008cf6b` | `data` |
| `.sdata` | `0x8008cf6c`-`0x8008d667` | `sdata_static` |
| `.bss` | `0x8008d668`-`0x8009f6fc` | `bss` |

The named structs are layout maps. Their field offsets matter because retail code often cares about the address, not only the C field name.

## Overlay Regions

CTR has three overlay regions. Each region occupies a fixed RAM range and can be overwritten by one of several overlay binaries.

See `docs/OVERLAYS.md` for the overlay list.

The important memory rule is that only one overlay per region exists on PS1 at a time.
Native builds compile every overlay function into the host binary, but the game logic still tracks the currently resident overlay index.
Native code should not assume (for now) that "compiled in" means "logically resident."

## MEMPACK

`MEMPACK` is the main runtime heap allocator. It owns the dynamic memory left after the resident executable, static data, BSS, and largest relevant overlay region.

Startup calls:

```c
#define MEMPACK_SIZE 0x200000
MEMPACK_Init(MEMPACK_SIZE);
```

On PS1, `MEMPACK_Init` computes the heap start from the end of the largest
region 3 overlay:

```c
maxOverlayEnd = max(AH_EndOfFile, RB_EndOfFile, MM_EndOfFile, CS_EndOfFile);
startPtr = OVR_Region3 + align_up(maxOverlayEnd - OVR_Region3, 0x800);
packSize = ramSize - (startPtr & 0xffffff) - 0x800;
```

For NTSC-U 926:

| Symbol | Address |
|--------|---------|
| `OVR_Region3` | `0x800ab9f0` |
| largest region 3 end, `RB_EndOfFile` | `0x800ba548` |
| aligned mempack start | `0x800ba9f0` |
| mempack end | `0x801ff800` |
| mempack size | `0x144e10` |

That gives retail about 1.27 MiB of allocator space, not the full 2 MiB of main RAM.

### Low And High Allocation

Each mempack has two moving pointers:

| Field | Direction | Purpose |
|-------|-----------|---------|
| `firstFreeByte` | grows upward | Normal low-memory allocations |
| `lastFreeByte` | grows downward | High-memory allocations |

`MEMPACK_AllocMem` consumes from `firstFreeByte`.
`MEMPACK_AllocHighMem` consumes from `lastFreeByte`. `MEMPACK_GetFreeBytes` returns the gap between the two pointers.

`MEMPACK_PushState`, `MEMPACK_PopState`, and `MEMPACK_PopToState` bookmark and restore `firstFreeByte`.
`MEMPACK_ClearLowMem` resets the low side of the pack. `MEMPACK_ClearHighMem` resets the high side.

## Native MEMPACK

Native cannot use real `0x80000000` PS1 addresses directly for main RAM. Instead the platform layer owns a host-side backing arena and the game allocator consumes that arena through `MEMPACK_Init`.

The split is intentional:

| Layer | Responsibility |
|-------|----------------|
| Platform, `Platform_InitMempackArena` | Creates and clears the host backing storage, chooses the native test mode, reports the exposed arena |
| Game, `MEMPACK_Init` | Keeps the retail allocator lifecycle and initializes `sdata->PtrMempack` from the platform-provided arena |

`CTR_NATIVE_MEMPACK_RETAIL_PRESSURE` controls the current mode:

| Mode | Backing Buffer | Exposed Mempack Window | Purpose |
|------|----------------|------------------------|---------|
| `1` | `0x200000` bytes | `0xba9f0`-`0x1ff800` | Test native under retail-like memory pressure |
| `0` | `0x800000` bytes | `0x0`-`0x800000` | Expanded native mode with extra low-address host memory |

The pressure mode is useful for finding native paths that silently rely on more dynamic memory than retail had.
The PS1 path does not call the platform arena hook; it computes the allocator window from retail overlay symbols.

## Native VRAM

Native keeps the PS1-visible VRAM contract at `1024x512x16`, but the platform layer owns the host backing and presentation path.

| Layer | Responsibility |
|-------|----------------|
| Native renderer/platform | Owns the CPU VRAM mirror, OpenGL textures, uploads, and native presentation bridge |
| Game/GPU compatibility layer | Keeps retail `LoadImage`, `MoveImage`, `StoreImage`, draw packets, asset rects, and upload timing |

## Scratchpad

The PS1 scratchpad is 1 KiB:

```text
0x1f800000 - 0x1f8003ff
```

Native uses a process-local scratchpad buffer and routes `CTR_SCRATCHPAD_PTR`
through that runtime base. Retail absolute scratchpad addresses are translated
back to offsets from this buffer.

Scratchpad-heavy areas include:

- collision searches
- camera helpers
- RenderBucket setup and emission
- overlay 226 level rendering
- vehicle effects such as skid and warp dust helpers

Retail scratchpad offsets are often part of the algorithm. Reusing an offset too
early, or assuming scratchpad contents survive across a helper that retail would
overwrite, can cause parity bugs.

## 24-bit Primitive Links

PS1 CPU pointers are 32-bit, but GPU primitive and ordering-table links store
only a 24-bit address plus an 8-bit packet length:

```c
u32 addr : 24;
u32 size : 8;
```

Native keeps the retail packet layout and routes primitive/OT links through the
native GPU link bridge. Under `CTR_NATIVE`, the 24-bit field is a bridge token,
not a truncated host pointer.

Keep game-visible primitive packets retail-shaped; do not reintroduce
PsyCross's widened primitive packets.

## Known Gap: Loaded-Asset Pointer Relocation (64-bit hosts)

`LOAD_RunPtrMap` (`game/LOAD/LOAD_Assets.c`) patches file-relative offsets into
absolute pointers in place, directly inside loaded asset blobs (model data,
level data, etc. -- anything using the `DramPointerMap` scheme):

```c
*(int *)&origin[offset] = *(int *)&origin[offset] + (int)origin;
```

This is a 4-byte read-modify-write that truncates `origin` (the load buffer's
base pointer) to `int`. On a 64-bit host this silently corrupts every
pointer field relocated this way -- including fields already declared with a
real pointer type in their C struct (e.g. `ModelHeader.ptrFrameData`), since
the relocator only ever touches 4 of those 8 bytes regardless of the field's
declared width.

This is distinct from, and deeper than, a simple "field typed `u32` instead of
a pointer" bug: the structs this scheme patches (e.g. `struct ModelHeader`,
size-pinned via `_Static_assert(sizeof(struct ModelHeader) == 0x40)`) are
direct binary overlays of the on-disk file format, so widening a patched field
to a real 64-bit pointer would misalign every subsequent record read from
disk -- it would not even fix the bug, since the relocator still performs a
4-byte patch regardless of the C struct's field width.

Known affected fields include `ModelHeader.ptrCommandList` (declared `u32`,
inconsistently with its already-pointer-typed siblings) and the runtime copies
made from it in `InstDrawPerPlayer.ptrCommandList`/`.ptrColorLayout` and
`InstDrawPerPlayer.ptrDeltaArray` (copied from `ModelAnim.ptrDeltaArray`) in
`game/RenderBucket/RenderBucket_QueueExecute.c`. These are left unfixed for
now -- not because the field types are correct, but because correcting them
requires redesigning `LOAD_RunPtrMap` itself (most likely: keep relocated
fields as file-relative offsets and resolve them against the asset's base
pointer at the point of use, mirroring the token-bridge pattern already used
for GPU primitive/OT links above, rather than patching absolute pointers in
place). That is a separate, larger piece of work, not a mechanical retype.

This means assets loaded through `LOAD_RunPtrMap` are not yet known-safe on a
genuinely 64-bit host (e.g. macOS) where heap/static addresses can exceed
4GB. The Linux/Windows builds are unaffected today only because they are
forced 32-bit.

**Address placement cannot rescue this on macOS.** The obvious shortcut --
keep every truncated pointer lossless by mapping the game's memory below 4 GiB
-- is not available on Apple Silicon: with the default 4 GiB `__PAGEZERO` the
entire low 4 GiB is reserved (statics land at `0x1_xxxx_xxxx`, `malloc` at
`0x8_xxxx_xxxx`, `mmap` at `0x1_xxxx_xxxx`), and shrinking `__PAGEZERO` to free
that region makes recent macOS kernels SIGKILL the binary at launch (confirmed
on Darwin 25.x / macOS arm64, even after re-signing, for any non-default
pagezero size). So the relocation must be made 64-bit-correct in code -- most
likely the offset-resolved-at-use / token-bridge redesign described above --
rather than by where the buffers live.

## Known Gap: Retail-Offset Asserts vs. Embedded Pointers (64-bit hosts)

Many structs carry ASM-verified layout checks like
`_Static_assert(offsetof(struct CameraDC, driverOffset_CamLookAtPos) == 0x7c)`
(there are roughly 1,065 `offsetof`/`sizeof` static asserts across `include/`
and `game/`). These assume every field is retail/32-bit-pointer-shaped. When a
struct embeds a real C pointer field ahead of the asserted field, the pointer
is 4 bytes in retail but 8 bytes on a 64-bit host, so every later field's real
offset grows -- and the assert (correctly) fails to compile.

`struct CameraDC` is a confirmed example: building 64-bit (attempted for the
macOS port) hits this immediately and exhausts the compiler's error limit on
that one struct alone. This is a structural property of the codebase, not a
handful of bad casts, and it almost certainly recurs in other structs once
`CameraDC` is addressed.

Fixing this for real requires, per struct, working out whether each failing
assert is a pure ASM-parity diagnostic (safe to relax for native 64-bit
builds) or backed by actual hardcoded-offset pointer math elsewhere in `game/`
(which would need a real fix, not just relaxing an assert) -- not a blanket
search-and-replace.

**Resolved for the compile blocker.** All `offsetof`/`sizeof` layout asserts
are now written through `CTR_STATIC_ASSERT_LAYOUT`
(`include/platform/native_static_assert.h`, force-included via `CMakeLists.txt`):
it expands to a real `_Static_assert` on 32-bit hosts (so the Win/Linux builds
remain the byte-parity guardians) and to nothing on 64-bit hosts (`__LP64__`),
where embedded host pointers make the retail offsets unsatisfiable. Value
asserts (enum/flag constants, anything without `offsetof`/`sizeof`) stay as
plain `_Static_assert` everywhere. With this in place `./build_macos.sh`
produces a working arm64 executable that launches and reaches asset loading.

This trades away the 64-bit build's compile-time layout checks for structs that
embed host pointers; that is acceptable because (a) pure PSX-shaped structs have
identical layout at 64-bit anyway and (b) the 32-bit builds still verify every
assert. It does **not** fix the runtime consequences -- code that reads a
pointer-shifted struct field via a hardcoded byte offset (rather than by C
field name) would still be wrong on 64-bit. Auditing for such hardcoded-offset
access is part of the same remaining work as the pointer-truncation gap above.
