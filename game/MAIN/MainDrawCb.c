#include <common.h>

void MainDrawCb_DrawSync()
{
	struct GameTracker *gGT;
	gGT = sdata->gGT;

	if (gGT->bool_DrawOTag_InProgress == 1)
	{
		gGT->bool_DrawOTag_InProgress = 0;
	}

	return;
}

// NOTE(aalhendi): CTR_NATIVE keeps native input/audio/VRAM bridge work.
void MainDrawCb_Vsync()
{
	register struct GameTracker *gGT CTR_PSX_REGISTER("$4");
	register struct GameTracker *drawSyncTracker CTR_PSX_REGISTER("$5");
	register u32 rootCounter CTR_PSX_REGISTER("$4");
	register s32 flipTicks CTR_PSX_REGISTER("$2");
	register s32 betweenSync CTR_PSX_REGISTER("$3");

	gGT = GAME_TRACKER;
	gGT->frameTimer_VsyncCallback++;
	if ((gGT->gameMode1 & PAUSE_ALL) == 0)
	{
		gGT->frameTimer_Confetti++;
	}

	// NOTE(aalhendi): Retail reloads the tracker before updating the draw-sync
	// counter; these value cues retain its load and store order.
	rootCounter = 0xf2000001;
	CTR_PSX_OBSERVE_VALUE(rootCounter);
	drawSyncTracker = GAME_TRACKER_RELOAD();
	CTR_PSX_OBSERVE_VALUE(drawSyncTracker);
	flipTicks = sdata->vsyncTillFlip;
	CTR_PSX_OBSERVE_VALUE(flipTicks);
	betweenSync = drawSyncTracker->vSync_between_drawSync;
	CTR_PSX_OBSERVE_VALUE(betweenSync);
	sdata->vsyncTillFlip = flipTicks - 1;
	CTR_PSX_OBSERVE_MEMORY(sdata->vsyncTillFlip);
	drawSyncTracker->vSync_between_drawSync = betweenSync + 1;

	// NOTE(aalhendi): Accumulate root-counter 1 units between VSync callbacks;
	// Timer_GetTime_Total converts them through the retail 0x147e divisor.
	GAME_RCNT_TOTAL_UNITS += GetRCnt(rootCounter);
	rootCounter = 0xf2000001;
	CTR_PSX_OBSERVE_VALUE(rootCounter);
	ResetRCnt(rootCounter);

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail calls HOWL unconditionally. Native skips only while
	// channel lists are mid-edit.
	if (sdata->criticalSectionCount == 0)
#endif
	{
		howl_PlayAudio_Update();
	}

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native owns host input and writes PSX-shaped pad
	// snapshots before retail GAMEPAD_PollVsync consumes them.
	Platform_PollInput();
#endif

	GAMEPAD_PollVsync(sdata->gGamepads);

	return;
}
