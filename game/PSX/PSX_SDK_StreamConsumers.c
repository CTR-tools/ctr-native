#include "PSX_SDK_StreamState.h"
#include <ctr_math.h>

// NOTE(aalhendi): The CD interrupt and DMA completion callback also publish this status halfword.
#define ST_RING_ID(entry) (*(volatile u16 *)&(entry)->id)

u32 StGetNext(u32 **address, StHEADER **header)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	// NOTE(aalhendi): Keep the real V0 return value at the common join; soft preference changes allocation.
	register u32 result CTR_PSX_REGISTER("$2");
	StHEADER *entry = (StHEADER *)((u8 *)St_ringBase + (St_state8009ec30 << 5));
	if (ST_RING_ID(entry) == StREWIND)
	{
		u32 maskEnd = St_maskEnd;
		ST_STORE(u32, St_state8009ec30, 5072u, 0);
		if (maskEnd != 0)
		{
			entry->id = StFREE;
		}
		entry = (StHEADER *)((u8 *)St_ringBase + (St_state8009ec30 << 5));
	}
	if (ST_RING_ID(entry) == StCOMPLETE)
	{
		ST_RING_ID(entry) = StLOCK;
		result = 0;
		*address = (u32 *)((u8 *)St_ringBase + (St_ringCount << 5) + ((St_state8009ec30 * 63u) << 5));
		*header = entry;
	}
	else
	{
		result = 1;
	}
	CTR_PSX_KEEP_VALUE(result);
	return result;
}
#undef ST_RING_ID

u32 StFreeRing(u32 *address)
{
	u32 result;
	// NOTE(aalhendi): Ring addresses/offsets wrap at 32 bits; do not subtract pointers to different objects.
	s32 first = CTR_MipsSra((s32)((u32)address - ((u32)St_ringBase + (St_ringCount << 5))), 2) / 504;
	StHEADER *entry = (StHEADER *)((u8 *)St_ringBase + ((u32)first << 5));
	u16 sectors = entry->nSectors;
	s32 i;
	if ((s16)entry->id == StLOCK)
	{
		i = 0;
		if ((s16)sectors > 0)
		{
			s32 count = (s16)sectors;
			do
			{
				StHEADER *current = (StHEADER *)((u8 *)St_ringBase + (((u32)i + (u32)first) << 5));
				current->id = StFREE;
				++i;
			} while (i < count);
		}
		St_state8009ec30 = (u32)i + (u32)first;
		result = 0;
	}
	else
	{
		result = 1;
	}
	// NOTE(aalhendi): Retain the shared return join instead of duplicating the two epilogues.
	CTR_PSX_KEEP_VALUE(result);
	return result;
}
