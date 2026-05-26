#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003d95c-0x8003d9ec.
void MEMCARD_CloseCard(void)
{
	// This function was copy/pasted by Naughty Dog,
	// psx\sample\memcard\CARD\CARD.C lines 355 to 365

	StopCARD();
	EnterCriticalSection();
	CloseEvent(sdata->SwCARD_EvSpIOE);
	CloseEvent(sdata->SwCARD_EvSpERROR);
	CloseEvent(sdata->SwCARD_EvSpTIMOUT);
	CloseEvent(sdata->SwCARD_EvSpNEW);
	CloseEvent(sdata->HwCARD_EvSpIOE);
	CloseEvent(sdata->HwCARD_EvSpERROR);
	CloseEvent(sdata->HwCARD_EvSpTIMOUT);
	CloseEvent(sdata->HwCARD_EvSpNEW);
	ExitCriticalSection();

	return;
}
