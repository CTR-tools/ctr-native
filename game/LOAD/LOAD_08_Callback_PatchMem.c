#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031aa4-0x80031b00.
void LOAD_Callback_PatchMem(struct LoadQueueSlot *lqs)
{
	char *patchPtr;
	char *patchStart;
	int patchSize;
	int patchNum;

	// CTR doesn't load one lev DRAM for AdvHub,
	// it loads one ReadFile for LEV in a sub-mempack,
	// it loads one ReadFile for PtrMap with AllocHighMem

	// that's why patchPtr is here
	patchPtr = lqs->ptrDestination;
	patchStart = &patchPtr[4];
	patchSize = *(int *)&patchPtr[0];
	patchNum = patchSize >> 2;

	sdata->load_inProgress = 0;

	LOAD_RunPtrMap((char *)sdata->ptrLevelFile, (int *)patchStart, patchNum);

	MEMPACK_SwapPacks(0);
	MEMPACK_ClearLowMem();
	MEMPACK_SwapPacks(sdata->gGT->activeMempackIndex);
}
