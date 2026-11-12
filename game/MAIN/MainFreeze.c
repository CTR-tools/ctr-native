#include <common.h>
#include "MainFreeze_Chaos.c"

void MainFreeze_MenuPtrQuit(struct RectMenu *menu)
{
	s16 row;
	struct GameTracker *gGT = sdata->gGT;

	if (menu->funcState == RECTMENU_FUNC_STATE_INPUT)
	{
		row = menu->rowSelected;
		if (row == 0)
		{
			// Erase ghost of previous race from RAM
			GhostTape_Destroy();

			// Add bit for "in menu" when loading is done
			sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

			// Go to main menu
			sdata->mainMenuState = MAIN_MENU_TITLE;

			// Remove bit for "In Adventure Arena" when loading is done
			sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_ARENA;

			// Unpause game
			gGT->gameMode1 &= ~PAUSE_1;

			// Level ID for main menu (39)
			MainRaceTrack_RequestLoad(0x27);
			return;
		}

		if ((row == 1) || (row == -1))
		{
			sdata->ptrDesiredMenu = MainFreeze_GetMenuPtr();
		}
	}
	else
	{
		menu->drawStyle &= ~RECTMENU_DRAW_STYLE_3P4P_LAYOUT;

		// if more than 2 screens
		if (gGT->numPlyrCurrGame > 2)
		{
			menu->drawStyle |= RECTMENU_DRAW_STYLE_3P4P_LAYOUT;
		}

		MainFreeze_SafeAdvDestroy();
	}
	return;
}

void MainFreeze_SafeAdvDestroy(void)
{
	// If you're in Adventure Arena
	if ((sdata->gGT->gameMode1 & ADVENTURE_ARENA) == 0)
	{
		return;
	}

	// check if Adv Hub is loaded
	if (!LOAD_IsOpen_AdvHub())
	{
		return;
	}

	AH_Pause_Destroy();
	return;
}

void MainFreeze_MenuPtrDefault(struct RectMenu *menu)
{
	int levID = 0; // dingo canyon
	u16 stringID;
	u32 gameMode;

	struct GameTracker *gGT = sdata->gGT;
	gameMode = gGT->gameMode1;

	// if you have not waited 5 frames since the game was paused then quit
	if (gGT->cooldownfromPauseUntilUnpause != 0)
	{
		return;
	}

	// assume 5 frames have passed since paused

	if (menu->funcState != RECTMENU_FUNC_STATE_INPUT)
	{
		menu->drawStyle &= ~RECTMENU_DRAW_STYLE_3P4P_LAYOUT;

		// if more than 2 screens
		if (2 < gGT->numPlyrCurrGame)
		{
			menu->drawStyle |= RECTMENU_DRAW_STYLE_3P4P_LAYOUT;
		}

		if (((gameMode & ADVENTURE_ARENA) == 0) || (menu->state & NEEDS_TO_CLOSE))
		{
			return;
		}

		// quit adv hub if it's not loaded
		if (!LOAD_IsOpen_AdvHub())
		{
			return;
		}

		AH_Pause_Update();
		return;
	}

	if (menu->rowSelected < 0)
	{
		return;
	}

	// get stringID from selected row
	stringID = menu->rows[menu->rowSelected].stringIndex;

	if (stringID == LNG_EXPANDED_CHAOS_RULES)
	{
		MainFreeze_SafeAdvDestroy();
		MM_ChaosConfig_OpenReadOnly(menu);
		return;
	}

	if (stringID == LNG_OPTIONS)
	{
		MainFreeze_SafeAdvDestroy();
		MM_ConfigMenu_Open(menu, CONFIG_MENU_PAUSE);
		return;
	}

	// stringID 11: "AKU AKU HINTS"
	// stringID 12: "UKA UKA HINTS"
	if (stringID == 11 || stringID == 12)
	{
		// Set Menu to Hints
		sdata->ptrDesiredMenu = &D232.menuHintMenu; // in 232
		return;
	}

	// stringID 3: "QUIT"
	if (stringID == 3)
	{
		// Set Menu to Quit
		sdata->ptrDesiredMenu = &data.menuQuit;
		data.menuQuit.rowSelected = 1;
		return;
	}

	// must wait 5 frames until next pause
	gGT->cooldownFromUnpauseUntilPause = 5;

	// hide Menu
	RECTMENU_Hide(menu);

	MainFreeze_SafeAdvDestroy();

	// careful, it's stringID MINUS one
	switch (stringID)
	{
	// stringID 1: "RESTART"
	case 1:

	// stringID 4: "RETRY"
	case 4:

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;

		if (g_config.loadingScreen == LOADING_SCREEN_FADE || RaceFlag_IsFullyOffScreen())
		{
			// checkered flag, begin transition on-screen
			RaceFlag_BeginTransition(1);
		}

		// restart race
		sdata->Loading.stage = LOAD_RESTART;

		// if you are not showing a ghost during a race
		if (sdata->boolReplayHumanGhost == 0)
		{
			return;
		}

		// If the ghost playing buffer is nullptr
		if (sdata->ptrGhostTapePlaying == 0)
		{
			return;
		}

		// Make P2 the character that is saved in the header of the
		// ghost that you will see in the race
		data.characterIDs[1] = sdata->ptrGhostTapePlaying->characterID;
		return;

	// stringID 2: "RESUME"
	case 2:

		// unpause game
		ElimBG_Deactivate(gGT);

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;

		// unpause audio
		MainFrame_TogglePauseAudio(0);

		// play pause/unpause sound
		OtherFX_Play(1, 1);
		return;

	// stringID 5: "CHANGE CHARACTER"
	case 5:

		// erase ghost of previous race from RAM
		GhostTape_Destroy();

		// set level ID to main menu
		levID = MAIN_MENU_LEVEL;

		// return to character selection
		sdata->mainMenuState = MAIN_MENU_CHARACTERS;

		// when loading is done, add bit for "in mb"
		sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;
		break;

	// stringID 6: "CHANGE LEVEL"
	case 6:

		// erase ghost of previous race from RAM
		GhostTape_Destroy();

		// level ID of main mb
		levID = MAIN_MENU_LEVEL;

		// return to track selection
		sdata->mainMenuState = MAIN_MENU_TRACK_SELECT;

		// when loading is done
		// add bit for "in mb"
		sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;
		break;

	// stringID 10: "CHANGE SETUP"
	case 10:

		// set level ID to main menu
		levID = MAIN_MENU_LEVEL;

		// return to battle setup
		sdata->mainMenuState = MAIN_MENU_BATTLE_SETUP;

		// when loading is done
		// add bit for "in mb"
		sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;
		break;

	// stringID 13: "EXIT TO MAP"
	case 13:

		// when loading is done
		// add this bit for In Adventure Arena
		sdata->Loading.OnBegin.AddBitsConfig0 |= ADVENTURE_ARENA;

		// when loading is done
		// remove bits for Relic Race or Crystal Challenge
		sdata->Loading.OnBegin.RemBitsConfig0 |= RELIC_RACE | CRYSTAL_CHALLENGE;

		// when loading is done
		// remove bit for CTR Token Challenge
		sdata->Loading.OnBegin.RemBitsConfig8 |= TOKEN_RACE;

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;

		// If you are not in Adventure cup
		if ((gameMode & ADVENTURE_CUP) == 0)
		{
			// 0x80000000
			// If you're in Boss Mode
			if ((int)gameMode < 0)
			{
				// when loading is done remove bit for Boss Race, relic, and crystal challenge
				sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_BOSS | RELIC_RACE | CRYSTAL_CHALLENGE;

				// When loading is done add bit to spawn driver near boss door
				sdata->Loading.OnBegin.AddBitsConfig8 |= SPAWN_AT_BOSS;
			}

			// set levID to level you were in previously
			levID = gGT->prevLEV;
		}

		// If you're in Adventure Cup
		else
		{
			levID = GEM_STONE_VALLEY;

			// when loading is done remove bits for Adventure Cup, relic, and crystal challenge
			sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_CUP | RELIC_RACE | CRYSTAL_CHALLENGE;

			// Level ID
			gGT->levelID = gGT->cup.cupID + ADVENTURE_CUP_SYNTHETIC_LEVEL_ID_BASE;
		}
		break;
	default:
		return;
	}

	MainRaceTrack_RequestLoad(levID);
	return;
}

struct RectMenu *MainFreeze_GetMenuPtr(void)
{
	struct GameTracker *gGT = sdata->gGT;
	u32 gameMode = gGT->gameMode1;

	if ((gameMode & ADVENTURE_ARENA) != 0)
	{
		s32 hintString = LNG_UKA_UKA_HINTS;
		if (VehPickupItem_MaskBoolGoodGuy(gGT->drivers[0]) != 0)
		{
			hintString = LNG_AKU_AKU_HINTS;
		}

		data.rowsAdvHub[1].stringIndex = hintString;
		return MainFreeze_ChaosPauseMenu(&data.menuAdvHub, data.rowsAdvHub, len(data.rowsAdvHub) - 1);
	}

	if ((gameMode & ADVENTURE_MODE) != 0)
	{
		if ((gameMode & ADVENTURE_CUP) != 0)
		{
			return MainFreeze_ChaosPauseMenu(&data.menuAdvCup, data.rowsAdvCup, len(data.rowsAdvCup) - 1);
		}

		return MainFreeze_ChaosPauseMenu(&data.menuAdvRace, data.rowsAdvRace, len(data.rowsAdvRace) - 1);
	}

	if ((gameMode & BATTLE_MODE) != 0)
	{
		return &data.menuBattle;
	}

	if ((gGT->gameMode2 & CUP_ANY_KIND) != 0)
	{
		return &data.menuArcadeCup;
	}

	return &data.menuArcadeRace;
}

void MainFreeze_IfPressStart(void)
{
	struct GameTracker *gGT = sdata->gGT;
	u32 gameMode1;
	struct RectMenu *menu;

	if (RaceFlag_IsFullyOnScreen())
	{
		return;
	}

	if ((gGT->renderFlags & RENDER_FLAG_CHECKERED_FLAG) != 0)
	{
		return;
	}

	if (sdata->AkuAkuHintState != 0)
	{
		return;
	}

	if (sdata->ptrActiveMenu != NULL)
	{
		return;
	}

	gameMode1 = gGT->gameMode1;

	if ((gameMode1 & (END_OF_RACE | PAUSE_ALL)) != 0)
	{
		return;
	}

	if (gGT->levelID == MAIN_MENU_LEVEL)
	{
		return;
	}

	if ((gameMode1 & GAME_CUTSCENE) != 0)
	{
		return;
	}

	if (gGT->boolDemoMode != 0)
	{
		return;
	}

	if ((u32)(gGT->levelID - OXIDE_ENDING) < 2)
	{
		return;
	}

	if (sdata->load_inProgress != 0)
	{
		return;
	}

	if ((gGT->gameMode2 & VEH_FREEZE_PODIUM) != 0)
	{
		return;
	}

	gGT->gameMode1 = gameMode1 | PAUSE_1;

	menu = MainFreeze_GetMenuPtr();
	menu->rowSelected = 0;

	RECTMENU_Show(menu);
	MainFrame_TogglePauseAudio(1);
	OtherFX_Play(1, 1);
	ElimBG_Activate(gGT);
}
