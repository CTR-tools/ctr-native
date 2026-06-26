#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003d9ec-0x8003da68.
u8 MEMCARD_GetNextSwEvent(void)
{
	// IOE = IO End, meaning "finished without error"
	if (TestEvent(sdata->SwCARD_EvSpIOE))
	{
		return MC_RETURN_IOE;
	}
	if (TestEvent(sdata->SwCARD_EvSpERROR))
	{
		return MC_RETURN_TIMEOUT;
	}
	if (TestEvent(sdata->SwCARD_EvSpTIMOUT))
	{
		return MC_RETURN_NOCARD;
	}
	if (TestEvent(sdata->SwCARD_EvSpNEW))
	{
		return MC_RETURN_NEWCARD;
	}

	return MC_RETURN_PENDING;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003da68-0x8003dae4.
u8 MEMCARD_GetNextHwEvent(void)
{
	// IOE = IO End, meaning "finished without error"
	if (TestEvent(sdata->HwCARD_EvSpIOE))
	{
		return MC_RETURN_IOE;
	}
	if (TestEvent(sdata->HwCARD_EvSpERROR))
	{
		return MC_RETURN_TIMEOUT;
	}
	if (TestEvent(sdata->HwCARD_EvSpTIMOUT))
	{
		return MC_RETURN_NOCARD;
	}
	if (TestEvent(sdata->HwCARD_EvSpNEW))
	{
		return MC_RETURN_NEWCARD;
	}

	return MC_RETURN_PENDING;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003dae4-0x8003db54.
u8 MEMCARD_WaitForHwEvent(void)
{
	while (1)
	{
		// IOE = IO End, meaning "finished without error"
		if (TestEvent(sdata->HwCARD_EvSpIOE))
		{
			return MC_RETURN_IOE;
		}
		if (TestEvent(sdata->HwCARD_EvSpERROR))
		{
			return MC_RETURN_TIMEOUT;
		}
		if (TestEvent(sdata->HwCARD_EvSpTIMOUT))
		{
			return MC_RETURN_NOCARD;
		}
		if (TestEvent(sdata->HwCARD_EvSpNEW))
		{
			return MC_RETURN_NEWCARD;
		}

		// Not allowed to return PENDING, the goal is to
		// wait until the memcard is not PENDING anymore
		// return MC_RETURN_PENDING;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003db54-0x8003db98.
void MEMCARD_SkipEvents(void)
{
	// Flush all "previous" Events until everything shows PENDING
	while (MEMCARD_GetNextSwEvent() != MC_RETURN_PENDING)
	{
		;
	}
	while (MEMCARD_GetNextHwEvent() != MC_RETURN_PENDING)
	{
		;
	}
}
