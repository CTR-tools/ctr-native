#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 PS1 path 0x8003e740-0x8003e80c; CTR_NATIVE uses host RAM.
void MEMPACK_Init(int ramSize)
{
	struct Mempack *ptrMempack;
	uintptr_t startPtr;
	u32 maxOverlayEnd;
	int packSize;

#if defined(CTR_NATIVE)

	const struct PlatformMempackArena *arena = Platform_InitMempackArena();

	// Keep arena pointer arithmetic at host pointer width. Retail does this in
	// 32-bit (uintptr_t == u32 there); on a 64-bit host the arena lives above
	// 4 GiB, so truncating to u32 would corrupt every allocation. See
	// docs/MEMORY_MODEL.md.
	startPtr = (uintptr_t)arena->start;
	packSize = arena->size;

	printf("[CTR] MEMPACK native backing: base=%p\n", arena->base);

	MEMPACK_NewPack((void *)startPtr, packSize);
	sdata->PtrMempack->endOfMemory = arena->endOfMemory;

	printf("[CTR] MEMPACK native arena: start=%p size=%08x end=%p\n", (void *)startPtr, packSize, sdata->PtrMempack->endOfAllocator);

#else

	maxOverlayEnd = (u32)AH_EndOfFile;
	if (maxOverlayEnd < (u32)RB_EndOfFile)
		maxOverlayEnd = (u32)RB_EndOfFile;
	if (maxOverlayEnd < (u32)MM_EndOfFile)
		maxOverlayEnd = (u32)MM_EndOfFile;
	if (maxOverlayEnd < (u32)CS_EndOfFile)
		maxOverlayEnd = (u32)CS_EndOfFile;

	startPtr = (u32)OVR_Region3 + (((maxOverlayEnd - (u32)OVR_Region3) + 0x7ff) & ~0x7ff);
	packSize = ramSize - (int)(startPtr & 0xffffff) - 0x800;

	ptrMempack = sdata->PtrMempack;
	ptrMempack->start = (void *)startPtr;
	ptrMempack->endOfAllocator = (void *)(startPtr + packSize);
	ptrMempack->lastFreeByte = (void *)(startPtr + packSize);
	ptrMempack->packSize = packSize;
	ptrMempack->numBookmarks = 0;
	ptrMempack->endOfMemory = (void *)0x80200000;
	ptrMempack->firstFreeByte = (void *)startPtr;
#endif
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e80c-0x8003e830.
void MEMPACK_SwapPacks(int index)
{
	sdata->PtrMempack = &sdata->mempack[index];
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e830-0x8003e85c.
void MEMPACK_NewPack(void *start, int size)
{
	struct Mempack *ptrMempack = sdata->PtrMempack;
	void *end = (void *)((uintptr_t)start + size);

	ptrMempack->packSize = size;
	ptrMempack->start = start;
	ptrMempack->lastFreeByte = end;
	ptrMempack->endOfAllocator = end;
	ptrMempack->endOfMemory = end;
	ptrMempack->firstFreeByte = start;
	ptrMempack->numBookmarks = 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e85c-0x8003e874.
int MEMPACK_GetFreeBytes()
{
	struct Mempack *ptrMempack = sdata->PtrMempack;

	return (u32)ptrMempack->lastFreeByte - (u32)ptrMempack->firstFreeByte;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e874-0x8003e8e8.
void *MEMPACK_AllocMem(int allocSize)
{
	uintptr_t firstFreeByte;
	int newAllocSize;
	struct Mempack *ptrMempack = sdata->PtrMempack;

	if (MEMPACK_GetFreeBytes() < allocSize)
	{
		CTR_ErrorScreen(0xFF, 0, 0);
		for (;;)
		{
		}
	}

	newAllocSize = (allocSize + 3) & 0xfffffffc;
	ptrMempack->sizeOfPrevAllocation = newAllocSize;

	firstFreeByte = (uintptr_t)ptrMempack->firstFreeByte;
	ptrMempack->firstFreeByte = (void *)(firstFreeByte + newAllocSize);

	return (void *)firstFreeByte;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e8e8-0x8003e938.
void *MEMPACK_AllocHighMem(int allocSize)
{
	uintptr_t newLastFreeByte;

	while (MEMPACK_GetFreeBytes() < allocSize)
	{
	}

	allocSize = (allocSize + 3) & 0xfffffffc;
	sdata->PtrMempack->sizeOfPrevAllocation = allocSize;

	newLastFreeByte = (uintptr_t)sdata->PtrMempack->lastFreeByte - allocSize;
	sdata->PtrMempack->lastFreeByte = (void *)newLastFreeByte;

	return (void *)newLastFreeByte;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e938-0x8003e94c.
void MEMPACK_ClearHighMem()
{
	sdata->PtrMempack->lastFreeByte = sdata->PtrMempack->endOfAllocator;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e94c-0x8003e978.
void *MEMPACK_ReallocMem(int allocSize)
{
	int newAllocSize;
	struct Mempack *ptrMempack = sdata->PtrMempack;

	newAllocSize = (allocSize + 3) & 0xfffffffc;
	ptrMempack->firstFreeByte = (void *)((uintptr_t)ptrMempack->firstFreeByte - ptrMempack->sizeOfPrevAllocation + newAllocSize);
	ptrMempack->sizeOfPrevAllocation = newAllocSize;

	return ptrMempack->firstFreeByte;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e978-0x8003e9b8.
int MEMPACK_PushState()
{
	struct Mempack *ptrMempack = sdata->PtrMempack;
	int numBookmarks;

	numBookmarks = ptrMempack->numBookmarks;
	if (numBookmarks < 0x10)
	{
		ptrMempack->bookmarks[numBookmarks] = ptrMempack->firstFreeByte;
		ptrMempack->numBookmarks++;
	}

	return numBookmarks;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e9b8-0x8003e9d0.
void MEMPACK_ClearLowMem()
{
	struct Mempack *ptrMempack = sdata->PtrMempack;

	ptrMempack->numBookmarks = 0;
	ptrMempack->firstFreeByte = ptrMempack->start;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e9d0-0x8003ea08.
void MEMPACK_PopState()
{
	struct Mempack *ptrMempack = sdata->PtrMempack;
	int numBookmarks;

	numBookmarks = ptrMempack->numBookmarks;
	if (numBookmarks > 0)
	{
		numBookmarks--;
		ptrMempack->firstFreeByte = ptrMempack->bookmarks[numBookmarks];
		ptrMempack->numBookmarks = numBookmarks;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003ea08-0x8003ea28.
void MEMPACK_PopToState(int id)
{
	struct Mempack *ptrMempack = sdata->PtrMempack;

	ptrMempack->numBookmarks = id;
	ptrMempack->firstFreeByte = ptrMempack->bookmarks[id];
}
