#include <common.h>

// NOTE(aalhendi): The unit's resident binding names the existing requested-level
// slot; native and unbound PSX code use the canonical loading field.
#ifndef MAIN_REQUESTED_LEVEL
#define MAIN_REQUESTED_LEVEL (sdata->Loading.Lev_ID_To_Load)
#endif

void MainRaceTrack_StartLoad(s16 levelID)
{
	// clear backup,
	// keep music,
	// destroy "most" fx, let menu fx play to end
	howl_StopAudio(1, 0, 0);

	ElimBG_Deactivate(sdata->gGT);

	LOAD_LevelFile(levelID);
	return;
}

void MainRaceTrack_RequestLoad(s16 levelID)
{
	struct GameTracker *gt = GAME_TRACKER;

	// Turn off HUD
	gt->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;

	if (RaceFlag_IsFullyOffScreen() == 1)
	{
		RaceFlag_BeginTransition(1);
	}
	RaceFlag_ResetTextAnim();

	sdata->Loading.stage = LOAD_REQUESTED;
	MAIN_REQUESTED_LEVEL = levelID;
	return;
}
