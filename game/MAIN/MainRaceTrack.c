#include <common.h>

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
	// Turn off HUD
	sdata->gGT->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;

	// A menu can leave the flag fully on screen. Start a fresh transition for
	// the loading fade instead of immediately treating the screen as covered.
	if (g_config.loadingScreen == LOADING_SCREEN_FADE || RaceFlag_IsFullyOffScreen())
	{
		RaceFlag_BeginTransition(1);
	}
	RaceFlag_ResetTextAnim();

	sdata->Loading.stage = LOAD_REQUESTED;
	sdata->Loading.Lev_ID_To_Load = levelID;
	return;
}
