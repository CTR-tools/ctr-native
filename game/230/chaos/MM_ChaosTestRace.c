#include <common.h>
#include "MM_ChaosTestRace.h"

enum
{
	CHAOS_TEST_DURATION_SECONDS = 5,
	CHAOS_TEST_INTERVAL_SECONDS = 10,
};

// Developer shortcut: Start on an effect launches a disposable Adventure race.
void MM_ChaosTestRace_Start(ChaosEffectId effect)
{
	struct GameTracker *gGT = sdata->gGT;
	sdata->chaosSettings = (struct ChaosSettings){
		.durationSeconds = CHAOS_TEST_DURATION_SECONDS,
		.triggerIntervalSeconds = CHAOS_TEST_INTERVAL_SECONDS,
		.trigger = CHAOS_TRIGGER_INTERVAL,
		.armed = true,
		.enabledCount = 1,
	};
	sdata->chaosSettings.effects[effect] = true;
	MM_NewLoad_FinishChaosSetup();
	// Unwind creation normally, then load directly instead of visiting the garage.
	MM_TITLE_MENU_STATE = TITLE_MENU_STATE_IN_MENU;
	sdata->newLoad.previousMenu = NULL;
	MM_Title_CameraReset();
	MM_Title_KillThread();
	MM_QuickRace_SetupRace();
	gGT->gameMode1 = ADVENTURE_MODE;
	// A previous Adventure reward would select the podium overlay during loading.
	gGT->podiumRewardID = NOFUNC;
	GAMEPROG_NewProfile_InsideAdv(&GAME_ADV_PROGRESS);
	GAME_ADV_PROGRESS.characterID = CRASH_BANDICOOT;
	GAMEPROG_ResetAdvMultiplayer(&sdata->advMultiplayer);
	// This shortcut skips the warp pad that normally seeds the Adventure grid.
	for (int i = 0; i < (int)len(sdata->kartSpawnOrderArray); i++)
	{
		sdata->kartSpawnOrderArray[i] = (char)(len(sdata->kartSpawnOrderArray) - 1 - i);
	}
	sdata->advProfileIndex = 0xffff;
	gGT->currLEV = N_SANITY_BEACH;
	RECTMENU_ClearInput();
	sdata->ptrDesiredMenu = NULL;
	RECTMENU_Hide(&sdata->chaosMenu.menu);
	MainRaceTrack_RequestLoad(CRASH_COVE);
}

