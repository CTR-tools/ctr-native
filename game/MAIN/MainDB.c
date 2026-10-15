#include <common.h>
#include <platform/native_pgxp.h>

int MainDB_GetClipSize(u32 levelID, int numPlyrCurrGame)
{
	if (sdata->highDetailSplitScreenLevel && (numPlyrCurrGame > 1))
	{
		// High-detail LEVs can emit the 1P clipped geometry for every viewport.
		numPlyrCurrGame = 1;
	}

	switch (levelID)
	{
	case ADVENTURE_GARAGE:
		return 24000;

	case MAIN_MENU_LEVEL:
		return 16;

	case SEWER_SPEEDWAY:
		return 6000;

	case MYSTERY_CAVES:
		return 2500;

	case PAPU_PYRAMID:
	case POLAR_PASS:
		if (numPlyrCurrGame < 3)
		{
			return 3000;
		}

		return 2500;

	default:
		return 3000;
	}
}

void MainDB_PrimMem(struct PrimMem *primMem, u32 size)
{
	u32 alignedSize;
	void *pvVar1;

	pvVar1 = MEMPACK_AllocMem(size, NULL);
	primMem->capacityBytes = size;
	primMem->allocationStart = pvVar1;
	primMem->cursor = pvVar1;
	primMem->start = pvVar1;

	alignedSize = (size >> 2) << 2;
	pvVar1 = (void *)((int)pvVar1 + alignedSize);
	primMem->end = pvVar1;
	primMem->guardEnd = (void *)((int)pvVar1 - 0x100);
}

// Registers a primitive memory allocation with the PGXP shadow tables so a
// precise vertex bound to a packet address in it can be found without hashing.
// MEMPACK hands the address back per level load, so a moved region reallocates
// its own table and leaves the other one alone.
void MainDB_BindPrimMemRegion(int index, const void *start, size_t size)
{
	NativePgxp_SetPrimRegion(index, start, size);
}

void MainDB_OTMem(struct OTMem *otMem, u32 size)
{
	u32 alignedSize;
	void *pvVar1;

	pvVar1 = MEMPACK_AllocMem(size, NULL);
	otMem->capacityBytes = size;
	otMem->cursor = pvVar1;
	otMem->start = pvVar1;

	alignedSize = (size >> 2) << 2;
	otMem->end = (void *)((int)pvVar1 + alignedSize);
}
