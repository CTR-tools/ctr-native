#include <common.h>

int MainDB_GetClipSize(u32 levelID, int numPlyrCurrGame)
{
	// NOTE(aalhendi): Keep the shared result in v0 so GCC retains the retail return join.
	register s32 clipSize CTR_PSX_REGISTER("$2");

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
		clipSize = 2500;
		if (numPlyrCurrGame < 3)
		{
			clipSize = 3000;
		}
		break;
	default:
		clipSize = 3000;
		break;
	}
	CTR_PSX_KEEP_VALUE_RELAXED(clipSize);
	return clipSize;
}

void MainDB_PrimMem(struct PrimMem *primMem, u32 sizeInput)
{
	// NOTE(aalhendi): Retail keeps the requested and aligned size in s0 across allocation.
	register u32 size CTR_PSX_REGISTER("$16") = sizeInput;
	u8 *allocation;
	u8 *end;

	allocation = MEMPACK_AllocMem(size, sdata_static.s_PrimMem);
	primMem->capacityBytes = size;
	// Capacity keeps the request; the usable end is rounded down to a word.
	size = (size >> 2) << 2;
	primMem->allocationStart = allocation;
	primMem->cursor = allocation;
	primMem->start = allocation;
	end = allocation + size;
	primMem->end = end;
	primMem->guardEnd = end - 0x100;
}

void MainDB_OTMem(struct OTMem *otMem, u32 sizeInput)
{
	// NOTE(aalhendi): Retail keeps the requested and aligned size in s0 across allocation.
	register u32 size CTR_PSX_REGISTER("$16") = sizeInput;
	u8 *allocation;
	u8 *end;

	allocation = MEMPACK_AllocMem(size, sdata_static.s_OTMem);
	otMem->capacityBytes = size;
	// Capacity keeps the request; the usable end is rounded down to a word.
	size = (size >> 2) << 2;
	otMem->cursor = (u32 *)allocation;
	otMem->start = (u32 *)allocation;
	end = allocation + size;
	otMem->end = (u32 *)end;
}
