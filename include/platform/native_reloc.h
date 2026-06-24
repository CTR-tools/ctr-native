#ifndef NATIVE_RELOC_H
#define NATIVE_RELOC_H

#include <stddef.h>
#include <stdint.h>

// NOTE(aalhendi): macOS/arm64 (and any LP64/Win64 host) cannot run the retail
// in-place pointer relocation: binary-overlay assets embed 4-byte file-relative
// pointers, and LOAD_RunPtrMap fixes them up with a truncating
// `*(int*)&origin[off] += (int)origin`. On a 64-bit host the 4-byte slots cannot
// hold real addresses, and pointer *fields* grow 4->8 bytes so the C structs no
// longer match the file. Instead of relocating in place, these transforms walk
// each asset format right after load and rebuild the pointer-bearing structs
// into native structs with real 8-byte pointers (Option A in docs/MACOS_PORT.md).
// Leaf blobs (command lists, vertex/frame data, CLUTs, ...) stay resident in the
// original file buffer and are pointed at directly. The 32-bit Win/Linux builds
// keep LOAD_RunPtrMap and never compile any of this.

#if defined(__LP64__) || defined(_WIN64)
#define CTR_RELOC64 1
#endif

#ifdef CTR_RELOC64

struct Model;
struct Level;

// Rebuild a just-loaded model pack (MPK) into a native structure and return the
// pointer to store as sdata->ptrMPK. `mpkBase` is the loaded file body (what
// retail would keep as ptrMPK); `ptrMapOffsets`/`numPtrs` is the embedded DRAM
// pointer map (DRAM_GETOFFSETS / numBytes>>2) used to bound length-less pointer
// arrays. Reads raw (unrelocated) file-relative offsets.
void *Reloc64_ModelPack(void *mpkBase, const int *ptrMapOffsets, int numPtrs);

// Accessors for the native MPK header produced by Reloc64_ModelPack. They
// replace the retail raw reads `*(int*)ptrMPK` (icons) and `ptrMPK + 4`
// (PLYROBJECTLIST), whose byte offsets no longer hold those values natively.
// MpkIcons returns the (resolved) LevTexLookup pointer as an integer the way
// retail stored gGT->mpkIcons; 0 when the pack has no icons.
uintptr_t Reloc64_MpkIcons(const void *mpkHeader);
struct Model **Reloc64_MpkModels(const void *mpkHeader);

// Rebuild a just-loaded level file into a native structure and return the
// pointer to store as sdata->ptrLevelFile. `levelBase` is the loaded file
// body; `ptrMapOffsets`/`numPtrs` is the embedded DRAM pointer map (kept for
// signature symmetry with Reloc64_ModelPack, unused since every level array
// has an explicit stored count or fixed size).
void *Reloc64_Level(void *levelBase, const int *ptrMapOffsets, int numPtrs);

#endif // CTR_RELOC64

#endif // NATIVE_RELOC_H
