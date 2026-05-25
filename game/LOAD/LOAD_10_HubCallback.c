#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031b14-0x80031b50.
void LOAD_HubCallback(struct LoadQueueSlot *lqs)
{
	sdata->load_inProgress = 0;
	LOAD_Callback_PatchMem(lqs);

	sdata->gGT->level2 = sdata->ptrLevelFile;
	MEMPACK_SwapPacks(sdata->gGT->activeMempackIndex);
}
