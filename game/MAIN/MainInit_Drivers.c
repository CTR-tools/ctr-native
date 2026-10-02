#include <common.h>

void MainInit_Drivers(struct GameTracker *gGT)
{
	s32 i;

	for (i = 0; i < 8; i++)
	{
		gGT->drivers[i] = NULL;
	}

	GAME_TRACKER->numBotsNextGame = 0;
	if ((gGT->gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA | MAIN_MENU)) == 0)
	{
		BOTS_Adv_AdjustDifficulty();
	}

	GhostReplay_Init1();

	if (LOAD_IsOpen_RacingOrBattle())
	{
		RB_MinePool_Init();
	}

	// Head insertion requires reverse births to preserve player thread order.
	for (i = gGT->numPlyrCurrGame - 1; i >= 0; i--)
	{
		struct Driver *player = VehBirth_Player(i);
		if (player != NULL)
		{
			gGT->drivers[i] = player;
		}
	}

	// Select the AI roster from live canonical fields after player births.
	if ((gGT->gameMode1 & (GAME_CUTSCENE | RELIC_RACE | CRYSTAL_CHALLENGE | TIME_TRIAL | ADVENTURE_ARENA | MAIN_MENU | BATTLE_MODE)) == 0)
	{
		struct GameTracker *aiTracker = GAME_TRACKER;
		if (aiTracker->numPlyrCurrGame != 3 && (aiTracker->gameMode1 & (ARCADE_MODE | ADVENTURE_MODE)) != 0)
		{
			if ((s32)aiTracker->gameMode1 < 0)
			{
				BOTS_Driver_Init(1);
			}
			else if ((aiTracker->gameMode1 & ADVENTURE_CUP) != 0 && aiTracker->cup.cupID == 4)
			{
				for (i = 1; i < 5; i++)
				{
					BOTS_Driver_Init(i);
				}
			}
			else
			{
				s32 count;
				s32 playerCount = gGT->numPlyrCurrGame;
				// NOTE(aalhendi): Preserve the roster start while selecting its limit in the retail scratch register.
				register s32 selection CTR_PSX_REGISTER("$3") = playerCount;
				switch (selection)
				{
				case 2:
				{
					count = 6;
					break;
				}
				case 1:
				{
					count = 8;
					break;
				}
				default:
				{
					count = 4;
					break;
				}
				}
				for (i = playerCount; i < count; i++)
				{
					BOTS_Driver_Init(i);
				}
			}
		}
	}

	if (GAME_TRACKER->numBotsNextGame != 0)
	{
		EngineAudio_InitOnce(0x10, HOWL_SFX_CENTER_NO_DISTORTION);
		EngineAudio_InitOnce(0x11, HOWL_SFX_CENTER_NO_DISTORTION);
	}

	// Fill unused menu slots with player karts.
	if ((gGT->gameMode1 & MAIN_MENU) != 0)
	{
		for (i = GAME_TRACKER->numPlyrCurrGame; i < 4; i++)
		{
			struct Driver *player = VehBirth_Player(i);
			if (player != NULL)
			{
				gGT->drivers[i] = player;
			}
		}
	}

	if ((GAME_TRACKER->gameMode1 & GAME_MODE_TIME_TRIAL_GAMEPLAY_MASK) == TIME_TRIAL)
	{
		GhostReplay_Init2();
		GhostTape_Start();

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Native restores P1's intended model after ghost setup.
		gGT->threadBuckets[PLAYER].thread->inst->model = data.driverModelExtras[0].model;
#endif
	}
}
