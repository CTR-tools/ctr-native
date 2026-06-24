#include "../platform.h"
#include "ctr_scratchpad.h"

#include <macros.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CTR_NATIVE_MEMPACK_RETAIL_PRESSURE
#if defined(__LP64__) || defined(_WIN64)
// arm64/Win64 (Option A relocation, see native_reloc.c): native structs widen
// every on-disc pointer 4->8 bytes and the rebuilt "spine" persists alongside
// the original file buffer rather than reusing its bytes in place, so this
// is a real memory-budget increase over retail, not a decode bug -- retail's
// exact pressure-matched ~1.3 MiB MPK window is too tight once a level's full
// instance/model/animation graph is rebuilt at native width. CTR_RELOC64
// builds already diverge from retail numbers by construction, so use the
// full backing buffer here. 32-bit Win/Linux builds keep the original
// retail-pressure-tested window untouched.
#define CTR_NATIVE_MEMPACK_RETAIL_PRESSURE 0
#else
#define CTR_NATIVE_MEMPACK_RETAIL_PRESSURE 1
#endif
#endif

// TODO(aalhendi): Re-audit LOAD_ReadFile_ex, LOAD_DramFileCallback, LEV/PTR
// callbacks, hub swapping, MEMPACK size arithmetic + PSX shaped ptr storage before removing the expanded arena escape hatch
#if CTR_NATIVE_MEMPACK_RETAIL_PRESSURE
// NOTE(aalhendi): Retail pressure mode exposes the NTSC-U 926 mempack window
// inside a 2 MiB backing store.
#define CTR_NATIVE_MEMPACK_BUFFER_SIZE  0x200000u
#define CTR_NATIVE_MEMPACK_START_OFFSET 0xba9f0u
#define CTR_NATIVE_MEMPACK_SIZE         0x144e10u
#else
#define CTR_NATIVE_MEMPACK_BUFFER_SIZE  (8u * 1024u * 1024u)
#define CTR_NATIVE_MEMPACK_START_OFFSET 0u
#define CTR_NATIVE_MEMPACK_SIZE         CTR_NATIVE_MEMPACK_BUFFER_SIZE
#endif

union NativeScratchpadStorage
{
	u8 bytes[CTR_SCRATCHPAD_SIZE];
	u32 words[CTR_SCRATCHPAD_SIZE / sizeof(u32)];
};

_Static_assert(sizeof(union NativeScratchpadStorage) == CTR_SCRATCHPAD_SIZE);

global_variable char s_mempackMemory[CTR_NATIVE_MEMPACK_BUFFER_SIZE];
global_variable struct PlatformMempackArena s_mempackArena;
global_variable union NativeScratchpadStorage s_scratchpadMemory;
u8 *gCTRNativeScratchpadBase;

void Platform_InitScratchpad(void)
{
#if defined(CTR_NATIVE)
	gCTRNativeScratchpadBase = &s_scratchpadMemory.bytes[0];
	memset(&s_scratchpadMemory, 0, sizeof(s_scratchpadMemory));
#endif
}

internal void Platform_ConfigureMempackArena(void)
{
	s_mempackArena.base = &s_mempackMemory[0];
	s_mempackArena.start = &s_mempackMemory[CTR_NATIVE_MEMPACK_START_OFFSET];
	s_mempackArena.endOfMemory = &s_mempackMemory[CTR_NATIVE_MEMPACK_BUFFER_SIZE];
	s_mempackArena.size = CTR_NATIVE_MEMPACK_SIZE;
	s_mempackArena.backingSize = CTR_NATIVE_MEMPACK_BUFFER_SIZE;
}

const struct PlatformMempackArena *Platform_InitMempackArena(void)
{
	memset(s_mempackMemory, 0, sizeof(s_mempackMemory));
	Platform_ConfigureMempackArena();

	return &s_mempackArena;
}

const struct PlatformMempackArena *Platform_GetMempackArena(void)
{
	return &s_mempackArena;
}

internal void Platform_RepairResidentPointers(s32 activeMempackIndex)
{
	if ((activeMempackIndex < 0) || (activeMempackIndex >= 4))
		activeMempackIndex = 0;

	// NOTE(aalhendi): Native keeps retail-shaped global data, but pointer aliases
	// must target this process's static storage. This also moves GCC's
	// initializer-only memcard helper global out of the live state graph so
	// checkpoints capture the actual memcard buffer.
	sdata = &sdata_static;
	sdata_static.gGT = &sdata_static.gameTracker;
	sdata_static.gGamepads = &sdata_static.gamepadSystem;
	sdata_static.PtrMempack = &sdata_static.mempack[activeMempackIndex];
	sdata_static.ptrToMemcardBuffer1 = &sdata_static.memcardBytes[0];
	sdata_static.ptrToMemcardBuffer2 = &sdata_static.memcardBytes[0];
}
