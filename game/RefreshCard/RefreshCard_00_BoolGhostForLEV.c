#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800469f0-0x80046a74.
int RefreshCard_BoolGhostForLEV(u16 trackID)
{
	int i;
	int count = 0;
	int numGhosts = *(s16 *)&sdata->numGhostProfilesSaved;
	s16 levelID = trackID;

	for (i = 0; i < numGhosts; i++)
	{
		if (sdata->ghostProfile_memcard[i].trackID == levelID)
			count++;
	}

	return (s16)count;
}
