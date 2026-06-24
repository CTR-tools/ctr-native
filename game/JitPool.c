#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80030fdc-0x8003105c.
void JitPool_Clear(struct JitPool *AP)
{
	int loopIndex;
	uintptr_t currSlot;

	currSlot = (uintptr_t)AP->ptrPoolData;

	// clear list of free and taken
	LIST_Clear(&AP->free);
	LIST_Clear(&AP->taken);

	for (loopIndex = 0; loopIndex < AP->maxItems; loopIndex++)
	{
		// add all pool items to the free list
		LIST_AddFront(&AP->free, (struct Item *)currSlot);

		// same as & 0xfffffffc, but more optimal,
		// this is aligning down, not up
		currSlot += ((AP->itemSize) >> 2) << 2;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003105c-0x800310d4.
void JitPool_Init(struct JitPool *AP, int maxItems, int itemSize, char *name)
{
	(void)name;

	memset(AP, 0, sizeof(struct JitPool));
	AP->maxItems = maxItems;
	AP->itemSize = itemSize;
	AP->poolSize = maxItems * itemSize;
	AP->ptrPoolData = MEMPACK_AllocMem(AP->poolSize);
	JitPool_Clear(AP);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800310d4-0x8003112c.
void *JitPool_Add(struct JitPool *AP)
{
	struct Item *item;
	item = AP->free.first;

	if (item != 0)
	{
		LIST_RemoveMember(&AP->free, item);
		LIST_AddFront(&AP->taken, item);
	}

	return item;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003112c-0x8003116c.
void JitPool_Remove(struct JitPool *AP, struct Item *item)
{
	LIST_RemoveMember(&AP->taken, item);
	LIST_AddFront(&AP->free, item);
}
