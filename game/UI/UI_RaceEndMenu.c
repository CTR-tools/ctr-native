#include <common.h>

enum UIRaceEndMenuOption
{
	UI_RACE_END_OPTION_QUIT = 3,
	UI_RACE_END_OPTION_RETRY = 4,
	UI_RACE_END_OPTION_CHANGE_CHARACTER = 5,
	UI_RACE_END_OPTION_CHANGE_LEVEL = 6,
	UI_RACE_END_OPTION_SAVE_GHOST = 9,
	UI_RACE_END_OPTION_CHANGE_SETUP = 10,
	UI_RACE_END_OPTION_EXIT_TO_MAP = 0xd,
	UI_RACE_END_OPTION_PRESS_TO_CONTINUE = 0xc9,
};

enum UIRaceEndConstants
{
	UI_RACE_END_MIN_SPLITSCREEN_MENU_PLAYERS = 3,
	UI_RACE_END_INITIAL_ICON_COUNT = 1,
	UI_RACE_END_SAVE_GHOST_FRAME = 0x3f9,
};

void UI_RaceEnd_MenuProc(struct RectMenu *menu)
{
	s32 option;
	// NOTE(aalhendi): Keep the incoming pointer in its real argument register
	// until Hide; use the saved pointer after the call.
	register struct RectMenu *argumentMenu CTR_PSX_REGISTER("$4") = menu;
	s32 row;

	if (argumentMenu->funcState != RECTMENU_FUNC_STATE_INPUT)
	{
		argumentMenu->drawStyle &= ~RECTMENU_DRAW_STYLE_3P4P_LAYOUT;

		// if more than 2 screens
		if (UI_RACE_END_MIN_SPLITSCREEN_MENU_PLAYERS <= GAME_TRACKER->numPlyrCurrGame)
		{
			argumentMenu->drawStyle |= RECTMENU_DRAW_STYLE_3P4P_LAYOUT;
		}

		return;
	}

	row = argumentMenu->rowSelected;
	if (row < 0)
	{
		return;
	}

	// if not SAVE GHOST
	if (argumentMenu->rows[row].stringIndex != UI_RACE_END_OPTION_SAVE_GHOST)
	{
		// make Menu invisible
		RECTMENU_Hide(argumentMenu);
	}

	row = menu->rowSelected;
	sdata->framesSinceRaceEnded = 0;
	sdata->numIconsEOR = UI_RACE_END_INITIAL_ICON_COUNT;

	// Reload the selection after the menu call.
	option = menu->rows[row].stringIndex;
	switch (option)
	{
	case UI_RACE_END_OPTION_RETRY:
	{
		// Turn off HUD
		GAME_TRACKER->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;

		if (RaceFlag_IsFullyOffScreen() == 1)
		{
			RaceFlag_BeginTransition(1);
		}

		GAME_LOADING_STAGE = LOAD_RESTART;

		// clear backup,
		// keep music,
		// destroy "most" fx, let menu fx play to end
		howl_StopAudio(1, 0, 0);

		// if did not improve time, then dont
		// overwrite old ghost with new ghost
		if ((GAME_TRACKER->gameModeEnd & PLAYER_GHOST_BEAT) == 0)
		{
			break;
		}

		GAME_SDATA_NAME(boolReplayHumanGhost) = 1;

		// NOTE(aalhendi): Byte views retain memcpy's aligned and unaligned paths.
		{
			char *source = (char *)GAME_SDATA_NAME(GhostRecording).ptrGhost;
			char *destination = (char *)GAME_SDATA_NAME(ptrGhostTapePlaying);
			memcpy(destination, source, GHOST_RECORD_BUFFER_SIZE);
		}

		// Make P2 the character that is saved in the
		// header of the ghost that you will see in the race
		GAME_SDATA_NAME(boolGhostsDrawing) = 0;
		data.characterIDs[1] = GAME_SDATA_NAME(ptrGhostTapePlaying)->characterID;

		break;
	}

	case UI_RACE_END_OPTION_EXIT_TO_MAP:
	{
		u32 modeBit;

		GAME_ADD_CONFIG_0 |= ADVENTURE_ARENA;
		GAME_REMOVE_CONFIG_0 |= (CRYSTAL_CHALLENGE | RELIC_RACE);
		GAME_REMOVE_CONFIG_8 |= TOKEN_RACE;

		// Test and publish the same race-mode bit.
		modeBit = ADVENTURE_CUP;
		if ((GAME_TRACKER->gameMode1 & modeBit) != 0)
		{
			GAME_REMOVE_CONFIG_0 |= modeBit;
			MainRaceTrack_RequestLoad(GEM_STONE_VALLEY);
			break;
		}

		modeBit = ADVENTURE_BOSS;
		if ((GAME_TRACKER->gameMode1 & modeBit) != 0)
		{
			GAME_REMOVE_CONFIG_0 |= modeBit;
			GAME_ADD_CONFIG_8 |= SPAWN_AT_BOSS;
		}

		MainRaceTrack_RequestLoad(GAME_TRACKER->prevLEV);
		break;
	}

	case UI_RACE_END_OPTION_CHANGE_LEVEL:
	{
		GhostTape_Destroy();
		GAME_MAIN_MENU_STATE = MAIN_MENU_TRACK_SELECT;
		GAME_ADD_CONFIG_0 |= MAIN_MENU;
		MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
		break;
	}

	case UI_RACE_END_OPTION_CHANGE_CHARACTER:
	{
		GhostTape_Destroy();
		GAME_MAIN_MENU_STATE = MAIN_MENU_CHARACTERS;
		GAME_ADD_CONFIG_0 |= MAIN_MENU;
		MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
		break;
	}

	case UI_RACE_END_OPTION_CHANGE_SETUP:
	{
		// go to battle setup screen
		GAME_MAIN_MENU_STATE = MAIN_MENU_BATTLE_SETUP;

		GAME_ADD_CONFIG_0 |= MAIN_MENU;

		// load LEV of main menu
		MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
		break;
	}

	case UI_RACE_END_OPTION_QUIT:
	{
		// Erase ghost of previous race from RAM
		GhostTape_Destroy();

		// go back to main menu
		GAME_MAIN_MENU_STATE = MAIN_MENU_TITLE;

		// load LEV of main menu
		MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
		break;
	}

	case UI_RACE_END_OPTION_PRESS_TO_CONTINUE:
	{
		sdata->menuReadyToPass |= 1;
		// NOTE(aalhendi): Complete readiness publication before leaving the callback.
		CTR_PSX_MEMORY_BARRIER();
		break;
	}
	case UI_RACE_END_OPTION_SAVE_GHOST:
	{
		sdata->framesSinceRaceEnded = UI_RACE_END_SAVE_GHOST_FRAME;
		// NOTE(aalhendi): Publish the save delay before entering profile selection.
		CTR_PSX_OBSERVE_MEMORY(sdata->framesSinceRaceEnded);

		// Set Load/Save to Ghost mode
		SelectProfile_ToggleMode(SELECT_PROFILE_MODE_GHOST_SAVE);

		// Change active Menu to GhostSelection
		GAME_SDATA_NAME(ptrDesiredMenu) = &data.menuGhostSelection;
		break;
	}
	}
}
