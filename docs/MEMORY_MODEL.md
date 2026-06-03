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
| Platform/PsyCross | Owns the CPU VRAM mirror, OpenGL textures, uploads, and native presentation bridge |
| Game/GPU compatibility layer | Keeps retail `LoadImage`, `MoveImage`, `StoreImage`, draw packets, asset rects, and upload timing |

## Scratchpad

The PS1 scratchpad is 1 KiB:

```text
0x1f800000 - 0x1f8003ff
```

Native maps an OS page at `0x1f800000` in `Platform_InitScratchpad`, but only the first `0x400` bytes are retail scratchpad.
The page is `0x1000` bytes because that is the usual minimum virtual-memory mapping unit.

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

PS1 CPU pointers are 32-bit. GPU primitive and ordering table links are not.
Primitive tags store a 24-bit address and an 8-bit packet length:

```c
u32 addr : 24;
u32 size : 8;
```

The PS1 GPU macros reconstruct a cached main-RAM pointer by OR-ing the tag address with `0x80000000`:

```c
next = 0x80000000 | tag.addr;
```

On PS1 this works because all main RAM addresses fit in the low 24 bits:

```text
0x800ba9f0 -> stored as 0x00ba9f0
0x801ff800 -> stored as 0x01ff800
```

On native, a host pointer may not fit in 24 bits. If a pointer like
`0x09ab1234` is stored in an OT link as `ptr & 0xffffff`, the stored value is
`0x00ab1234`, which is no longer the same address.

This is why some native paths need the mempack arena below `0x01000000`, or an
explicit expanded-pointer bridge. The comment "must be a 24-bit address" means
"this pointer may be packed into PS1 primitive or ordering table links."
