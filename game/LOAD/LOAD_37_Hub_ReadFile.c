#include <common.h>

// NOTE(aalhendi): ASM-audited NTSC-U 926 0x80032ffc-0x80033108.
// packID will always be 3-gGT->activeMempackIndex
void LOAD_Hub_ReadFile(int bigfile, int levID, int packID)
{
	int iVar2;
	struct GameTracker *gGT = sdata->gGT;

	// if level is already loaded, quit
	if (gGT->levID_in_each_mempack[packID] == levID)
		return;

	gGT->levID_in_each_mempack[packID] = levID;

	gGT->level2 = 0;
	sdata->modelMaskHints3D = 0;

	// Swap to pack of hub you're NOT on,
	// wipe the pack to reload the new hub
	MEMPACK_SwapPacks(packID);
	MEMPACK_ClearLowMem();

	sdata->PatchMem_Size = 1;

	// base index for group
	iVar2 = LOAD_GetBigfileIndex(levID, 1);

	LOAD_AppendQueue(bigfile, LT_SETVRAM, iVar2 + LVI_VRAM, 0, LOAD_VramFileCallback);
	LOAD_AppendQueue(bigfile, LT_GETADDR, iVar2 + LVI_LEV, &sdata->ptrLevelFile, LOAD_DramFileCallback);
	LOAD_AppendQueue(bigfile, LT_SETADDR, iVar2 + LVI_PTR, (void *)sdata->PatchMem_Ptr, LOAD_HubCallback);
}
