#include <common.h>

void MainGameStart_Initialize(struct GameTracker *gGT, b32 boolStopAudio)
{
	// NOTE(aalhendi): Retail keeps the updated mode in v0 and its start mask in v1.
	register u32 gameModeFlag CTR_PSX_REGISTER("$2");
	register u32 startMask CTR_PSX_REGISTER("$3");

	// The global tracker selects startup behavior; the argument receives the updates.
	if ((GAME_TRACKER->gameMode1 & (GAME_CUTSCENE | MAIN_MENU)) != 0)
	{
		gGT->trafficLightsTimer = 0;
		startMask = ~START_OF_RACE;
		gameModeFlag = gGT->gameMode1 & startMask;
	}
	else
	{
		gGT->trafficLightsTimer = 0xf00;
		gameModeFlag = gGT->gameMode1 | START_OF_RACE;
	}

	// NOTE(aalhendi): Retail stores and reloads the mode before clearing END_OF_RACE.
	gGT->gameMode1 = gameModeFlag;
	CTR_PSX_RELOAD(gGT->gameMode1);
	gGT->gameMode1 &= ~END_OF_RACE;

	// Zero requests stopping music and effects; normal initialization passes one.
	if (boolStopAudio == 0)
	{
		Music_Stop();
		howl_StopAudio(0, 0, 1);
	}

	VehBirth_TeleportAll(gGT, VEH_BIRTH_SPAWN_INIT_RACE_STATE);
}
