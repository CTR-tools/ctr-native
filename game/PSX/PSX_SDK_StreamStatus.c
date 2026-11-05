#include "PSX_SDK_StreamState.h"

void StRingStatus(s16 *freeSectors, s16 *overSectors)
{
	s32 i = (s32)St_ringCount;
	*freeSectors = 0;
	// NOTE(aalhendi): Status consumes the low half of the genuine index words, not a second layout.
	*overSectors = (s16)(*(CtrPackedU16 *)&St_state8009ec28 - *(CtrPackedU16 *)&St_state8009ec30);
	if (*overSectors < 0)
	{
		for (--i; i >= 0; --i)
		{
			StHEADER *entry = (StHEADER *)((u8 *)St_ringBase + ((u32)i << 5));
			if (*(volatile u16 *)&entry->id == StREWIND)
			{
				break;
			}
		}
		// NOTE(aalhendi): The scan bound itself advances before counting, not just the overcount expression.
		++i;
		*overSectors = (s16)((u16)*overSectors + (u32)i);
	}
	goto CheckFree;
	while (i >= 0)
	{
		StHEADER *entry = (StHEADER *)((u8 *)St_ringBase + ((u32)i << 5));
		// Count every free header in this domain, including free headers after busy entries.
		if (*(volatile u16 *)&entry->id == StFREE)
		{
			*freeSectors = (s16)((u16)*freeSectors + 1u);
		}
	CheckFree:
		--i;
	}
}
