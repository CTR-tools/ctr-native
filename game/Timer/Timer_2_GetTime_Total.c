#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b3a4-0x8004b41c.
int Timer_GetTime_Total()
{
	int rcntTotal = sdata->rcntTotalUnits;
	int rcnt = GetRCnt(DescRC + 1);
	int sysClock = rcntTotal + rcnt;

	if (rcnt < 100)
	{
		sysClock = sdata->rcntTotalUnits + rcnt;
	}

	return (sysClock * 1000) / 0x147e;
}
