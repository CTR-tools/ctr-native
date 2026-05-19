#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae834-0x800ae9a8
void DECOMP_CS_LoadBoss(struct BossCutsceneData *bcd)
{
	struct GameTracker *gGT = sdata->gGT;
	void (*const loadSentinel)(struct LoadQueueSlot *) = (void (*)(struct LoadQueueSlot *))-2;
	int index;

	index = 3 - gGT->activeMempackIndex;

	DECOMP_CDSYS_XAPauseRequest();

	// erase HEAD + BODY
	OVR_233.ptrModelBossBody = 0;
	OVR_233.ptrModelBossHead = 0;

	// invalidate alternative-hub, because
	// the boss will load in that level's RAM
	gGT->levID_in_each_mempack[index] = -1;

	// Swap to pack of hub you're NOT on,
	// wipe the pack to reload the new BOSS
	DECOMP_MEMPACK_SwapPacks(index);
	DECOMP_MEMPACK_ClearLowMem();

	sdata->load_inProgress = 1;

	if (bcd->vrmFile_UNUSED != 0)
	{
		DECOMP_LOAD_AppendQueue(0, LT_VRAM, bcd->vrmFile_UNUSED - 1 + index, NULL, NULL);
	}

	// CTR Model File (body)
	if (bcd->bodyFile != 0)
	{
		DECOMP_LOAD_AppendQueue(0, LT_DRAM, bcd->bodyFile - 1 + index, &OVR_233.ptrModelBossBody, loadSentinel);
	}

	// CTR Model File (head)
	DECOMP_LOAD_AppendQueue(0, LT_DRAM, bcd->headFile - 1 + index, NULL, DECOMP_CS_LoadBossCallback);
}
