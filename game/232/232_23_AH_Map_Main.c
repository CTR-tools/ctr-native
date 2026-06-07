#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b1c90-0x800b1ef8.
void AH_Map_Main(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *advDriver;
	struct UiElement2D *ptrHudData;
	int iVar1;
	int hubPtrs; // int*?
	s16 local_20;
	s16 local_1e[3];

	// force disable speedometer
	sdata->HudAndDebugFlags &= 0xfffffff7;

	local_20 = 0;
	advDriver = gGT->drivers[0];
	ptrHudData = data.hudStructPtr[gGT->numPlyrCurrGame - 1];
	hubPtrs = 0;
	iVar1 = RaceFlag_GetCanDraw();
	if (iVar1 == 0)
	{
		RaceFlag_SetCanDraw(1);
	}

	if (
	    // if Aku Hint is not unlocked
	    ((sdata->advProgress.rewards[3] & 0x400000) == 0) &&

	    (iVar1 = RaceFlag_IsFullyOffScreen(), iVar1 != 0))
	{
		// Trigger Aku Hint:
		// Welcome to Adventure Arena
		MainFrame_RequestMaskHint(0, 0);
	}


	// NOTE(aalhendi): Retail keeps this AI-only Adventure Hub speedometer fallback.
	if ((gGT->numPlyrCurrGame == 0) && ((advDriver->actionsFlagSet & 0x100000) != 0))
	{
		sdata->HudAndDebugFlags = 8;
	}

	if (gGT->level1->ptrSpawnType1->count != 0)
	{
		void **pointers = ST1_GETPOINTERS(gGT->level1->ptrSpawnType1);
		hubPtrs = (int)pointers[ST1_MAP]; // cast as int*?
	}

	// if game is not paused
	if ((gGT->gameMode1 & PAUSE_ALL) == 0)
	{
		// Jump meter and landing boost
		UI_JumpMeter_Update(advDriver);
	}

	// Check a HUD flag
	if ((gGT->hudFlags & 0x10) == 0)
	{
		local_1e[0] = 0;

		D232.unkModeHubItems = 0;

		UI_Map_DrawDrivers(hubPtrs, gGT->threadBuckets[0].thread, &local_20);

		AH_Map_Warppads((s16 *)hubPtrs, gGT->threadBuckets[5].thread,
		                (s16 *)&local_1e[0]); // local_1e index 1 and 2 are never assigned to, so garbage data?

		AH_Map_HubItems((void *)hubPtrs, &local_1e[0]);

		UI_Map_DrawMap(gGT->ptrIcons[3], gGT->ptrIcons[4],

		               500, 195,

		               &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, 1);

		UI_DrawSlideMeter(ptrHudData[8].x, ptrHudData[8].y, advDriver);
	}

	UI_DrawNumRelic(ptrHudData[0xE].x + 0x10, ptrHudData[0xE].y - 10);
	UI_DrawNumKey(ptrHudData[0xF].x + 0x10, ptrHudData[0xF].y - 10);
	UI_DrawNumTrophy(ptrHudData[0x10].x + 0x10, ptrHudData[0x10].y - 10);

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail appends this prompt after DrawOTag starts; the PS1
	// GPU can still consume that late OT write. Native DrawOTag parses
	// synchronously, so emit only this static prompt during the hub UI pass and
	// leave AH_MaskHint_Update to run the real state/audio timing later.
	if (sdata->AkuAkuHintState == 5)
		AH_MaskHint_DrawRepeatPrompt();
#endif

	return;
}
