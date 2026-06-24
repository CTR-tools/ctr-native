#include <common.h>

#include <platform/native_reloc.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800319e8-0x800319f4.
void LOAD_Callback_Overlay_Generic(struct LoadQueueSlot *lqs)
{
	(void)lqs;
	sdata->load_inProgress = 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800319f4-0x80031a08.
void LOAD_Callback_Overlay_230(void)
{
	sdata->load_inProgress = 0;
	sdata->gGT->overlayIndex_Threads = 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031a08-0x80031a20.
void LOAD_Callback_Overlay_231(void)
{
	sdata->load_inProgress = 0;
	sdata->gGT->overlayIndex_Threads = 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031a20-0x80031a38.
void LOAD_Callback_Overlay_232(void)
{
	sdata->load_inProgress = 0;
	sdata->gGT->overlayIndex_Threads = 2;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031a38-0x80031a50.
void LOAD_Callback_Overlay_233(void)
{
	sdata->load_inProgress = 0;
	sdata->gGT->overlayIndex_Threads = 3;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031a50-0x80031a64.
void LOAD_Callback_MaskHints3D(struct LoadQueueSlot *lqs)
{
	sdata->load_inProgress = 0;
	sdata->modelMaskHints3D = (struct Model *)lqs->ptrDestination;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031a64-0x80031a78.
void LOAD_Callback_Podiums(struct LoadQueueSlot *lqs)
{
	sdata->load_inProgress = 0;
	data.podiumModel_podiumStands = (int)lqs->ptrDestination;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031a78-0x80031aa4.
void LOAD_Callback_LEV(struct LoadQueueSlot *lqs)
{
	if ((lqs->flags & LT_GETADDR) == 0)
		sdata->load_inProgress = 0;

	sdata->ptrLevelFile = (struct Level *)lqs->ptrDestination;
}

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

#ifdef CTR_RELOC64
	// arm64: rebuild the level into native-pointer structs instead of the
	// truncating in-place LOAD_RunPtrMap. See platform/native_reloc.c.
	sdata->ptrLevelFile = Reloc64_Level((char *)sdata->ptrLevelFile, (int *)patchStart, patchNum);
#else
	LOAD_RunPtrMap((char *)sdata->ptrLevelFile, (int *)patchStart, patchNum);
#endif

	MEMPACK_SwapPacks(0);
	MEMPACK_ClearHighMem();
	MEMPACK_SwapPacks(sdata->gGT->activeMempackIndex);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031b00-0x80031b14.
void LOAD_Callback_DriverModels(struct LoadQueueSlot *lqs)
{
	sdata->load_inProgress = 0;
	sdata->ptrMPK = lqs->ptrDestination;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031b14-0x80031b50.
void LOAD_HubCallback(struct LoadQueueSlot *lqs)
{
	sdata->load_inProgress = 0;
	LOAD_Callback_PatchMem(lqs);

	sdata->gGT->level2 = sdata->ptrLevelFile;
	MEMPACK_SwapPacks(sdata->gGT->activeMempackIndex);
}
