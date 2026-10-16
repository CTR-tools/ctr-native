#include <common.h>
#include <platform/native_pgxp.h>

int MainDB_GetClipSize(u32 levelID, int numPlyrCurrGame)
{
	int retailSize;

	if (sdata->highDetailSplitScreenLevel && (numPlyrCurrGame > 1))
	{
		// High-detail LEVs can emit the 1P clipped geometry for every viewport.
		numPlyrCurrGame = 1;
	}

	switch (levelID)
	{
	case ADVENTURE_GARAGE:
		retailSize = 24000;
		break;

	case MAIN_MENU_LEVEL:
		retailSize = 16;
		break;

	case SEWER_SPEEDWAY:
		retailSize = 6000;
		break;

	case MYSTERY_CAVES:
		retailSize = 2500;
		break;

	case PAPU_PYRAMID:
	case POLAR_PASS:
		retailSize = (numPlyrCurrGame < 3) ? 3000 : 2500;
		break;

	default:
		retailSize = 3000;
		break;
	}

	return retailSize * MAINDB_ARENA_SCALE;
}

int MainDB_GetOTMemSize(u32 gameMode1, int numPlyrCurrGame)
{
	int retailSize;

	if ((gameMode1 & MAIN_MENU) != 0)
	{
		retailSize = 0x1800;
	}
	else if ((gameMode1 & ADVENTURE_ARENA) != 0)
	{
		retailSize = 0x2c00;
	}
	else if ((gameMode1 & BATTLE_MODE) != 0)
	{
		retailSize = 0x8000;
	}
	else if (numPlyrCurrGame < 3)
	{
		retailSize = 0x2000;
	}
	else
	{
		retailSize = 0x3000;
	}

	return retailSize * MAINDB_ARENA_SCALE;
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
