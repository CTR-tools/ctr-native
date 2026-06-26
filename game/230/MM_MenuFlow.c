#include <common.h>

extern struct MenuRow s_rowsMainMenuBasicConfig[];
extern struct MenuRow s_rowsMainMenuWithSBConfig[];
extern struct RectMenu g_configMenu;

// NOTE(aalhendi): ASM-verified against retail 230 0x800abaf0-0x800abcac.
u8 MM_TransitionInOut(struct TransitionMeta *meta, int framesPassed, int numFrames)
{
	u8 bool_Transitioning;
	int transitionIndex;
	s16 start;
	s16 framesLeft;

	bool_Transitioning = 1;
	transitionIndex = 0;

	// last member of array is null-terminated with 0xFFFF
	for (/**/; meta->headStart > -1; meta++, transitionIndex++)
	{
		start = meta->headStart;
		framesLeft = ((s16)framesPassed - start);

		if ((framesLeft == 4) && (transitionIndex == 0))
		{
			// Play "swoosh" sound for menu transition
			OtherFX_Play(0x65, 0);
		}

		if (framesLeft < 1)
		{
			bool_Transitioning = 0;
			meta->currX = 0;
			meta->currY = 0;
			continue;
		}

		// else if
		if (framesLeft < (s16)numFrames)
		{
			bool_Transitioning = 0;
			meta->currX = framesLeft * meta->distX / (s16)numFrames;
			meta->currY = framesLeft * meta->distY / (s16)numFrames;
			continue;
		}

		// else
		meta->currX = meta->distX;
		meta->currY = meta->distY;
	}
	return bool_Transitioning;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800acff4-0x800ad448.
void MM_MenuProc_Main(struct RectMenu *mainMenu)
{
	s16 choose;
	struct GameTracker *gGT = sdata->gGT;

	// if scrapbook is unlocked, change "rows" to extended array
	if (CHECK_ADV_BIT(sdata->gameProgress.unlocks, GAME_UNLOCK_BIT_SCRAPBOOK) != 0)
		mainMenu->rows = &s_rowsMainMenuWithSBConfig[0];
	else
		mainMenu->rows = &s_rowsMainMenuBasicConfig[0];

	MM_ParseCheatCodes();
	MM_ToggleRows_Difficulty();
	MM_ToggleRows_PlayerCount();

	// If you are at the highest hierarchy level of main menu
	if (mainMenu->unk1e == 1)
	{
		MM_Title_MenuUpdate();

		if (
		    // main menu, "title" exists, and timer >= 230
		    (D230.MM_State == 1) && (D230.titleObj != NULL) && (229 < D230.timerInTitle))
		{
			DecalFont_DrawLineOT(sdata->lngStrings[LNG_TM], 0x10e, 0x9c, FONT_SMALL, ORANGE, &gGT->backBuffer->otMem.uiOT[3]);
		}

		if ((D230.menuMainMenu.state & DRAW_NEXT_MENU_IN_HIERARCHY) == 0)
		{
			gGT->numPlyrNextGame = 1;

			// if no buttons pressed, check demo mode
			if (sdata->gGamepads->anyoneHeldCurr == 0)
			{
				gGT->demoCountdownTimer--;

				// If time runs out
				if (gGT->demoCountdownTimer < 1)
				{
					// Transition out of main menu
					D230.MM_State = 2;

					// Go to a cutscene of some kind
					// (either oxide intro or demo mode)
					D230.desiredMenuIndex = 4;
				}
			}

			// if button pressed, reset timer
			else
			{
				// 900 = 30 seconds at 30fps
				gGT->demoCountdownTimer = 900;
			}
		}
	}

	MM_Title_Init();

	// if drawing ptrNextBox_InHierarchy
	if ((mainMenu->state & DRAW_NEXT_MENU_IN_HIERARCHY) != 0)
	{
		D230.timerInTitle = 1000;
	}

	// if funcPtr is null
	if ((mainMenu->state & EXECUTE_FUNCPTR) == 0)
	{
		return;
	}

	struct Title *titleObj = D230.titleObj;

	// if "title" object exists
	if (titleObj != NULL)
	{
		// CameraPosOffset X
		titleObj->cameraPosOffset.x = 0;
	}

	// if you are at highest level of menu hierarchy
	if (mainMenu->unk1e != 0)
	{
		// leave the function
		return;
	}

	// If you are here, then you must not be
	// at the highest level of menu hierarchy

	// if row is negative, do nothing
	if ((mainMenu->rowSelected) < 0)
	{
		return;
	}

	// clear flags from game mode
	gGT->gameMode1 &= ~(BATTLE_MODE | ADVENTURE_MODE | TIME_TRIAL | ADVENTURE_ARENA | ARCADE_MODE | ADVENTURE_CUP);

	// clear more game mode flags
	gGT->gameMode2 &= ~(CUP_ANY_KIND);

	mainMenu->state |= ONLY_DRAW_TITLE;

	// Default to 3,
	// this intentionally disables the 1-lap cheat
	// in Time Trial and Adventure, DONT change it
	gGT->numLaps = 3;

	// get LNG index of row selected
	choose = mainMenu->rows[mainMenu->rowSelected].stringIndex;

	// Adventure Mode
	if (choose == 0x4c)
	{
		// Turn on Adventure Mode, turn off item cheats
		gGT->gameMode1 |= ADVENTURE_MODE;
		gGT->gameMode2 &= ~(CHEAT_WUMPA | CHEAT_MASK | CHEAT_TURBO | CHEAT_ENGINE | CHEAT_BOMBS);

		// menu for new/load
		mainMenu->ptrNextBox_InHierarchy = &D230.menuAdventure;
		mainMenu->state |= DRAW_NEXT_MENU_IN_HIERARCHY;
		return;
	}

	// Time Trial
	if (choose == 0x4d)
	{
		// Leave main menu hierarchy
		D230.MM_State = 2;

		// Set next stage to 2 for Time Trial
		D230.desiredMenuIndex = 2;

		// set game mode to Time Trial Mode
		gGT->numPlyrNextGame = 1;
		gGT->gameMode1 |= TIME_TRIAL;
		gGT->gameMode2 &= ~(CHEAT_WUMPA | CHEAT_MASK | CHEAT_TURBO | CHEAT_ENGINE | CHEAT_BOMBS);

		return;
	}

	// Arcade Mode
	if (choose == 0x4e)
	{
		// DONT change, should only work in Arcade, and VS
		if ((gGT->gameMode2 & CHEAT_ONELAP) != 0)
		{
			gGT->numLaps = 1;
		}

		// set game mode to Arcade Mode
		gGT->gameMode1 |= ARCADE_MODE;

		// set next menu
		mainMenu->ptrNextBox_InHierarchy = &D230.menuRaceType;
		mainMenu->state |= DRAW_NEXT_MENU_IN_HIERARCHY;
		return;
	}

	// Versus
	if (choose == 0x4f)
	{
		// DONT change, should only work in Arcade, and VS
		if ((gGT->gameMode2 & CHEAT_ONELAP) != 0)
		{
			gGT->numLaps = 1;
		}

		// next menu is choosing single+cup
		mainMenu->ptrNextBox_InHierarchy = &D230.menuRaceType;
		mainMenu->state |= DRAW_NEXT_MENU_IN_HIERARCHY;
		return;
	}

	// Battle
	if (choose == 0x50)
	{
		D230.characterSelect_transitionState = 2;

		// set game mode to Battle Mode
		gGT->gameMode1 |= BATTLE_MODE;

		// set next menu to 2P,3P,4P
		mainMenu->ptrNextBox_InHierarchy = &D230.menuPlayers2P3P4P;
		mainMenu->state |= DRAW_NEXT_MENU_IN_HIERARCHY;
		return;
	}

	// High Score
	if (choose == 0x51)
	{
		// Set next stage to high score menu
		D230.desiredMenuIndex = 3;

		// Leave main menu hierarchy
		D230.MM_State = 2;

		return;
	}

	// Scrapbook
	if (choose == 0x234)
	{
		// Set next stage to Scrapbook
		D230.desiredMenuIndex = 5;

		// Leave main menu hierarchy
		D230.MM_State = 2;

		return;
	}

	// Config / Options
	if (choose == 0x0E)
	{
		sdata->ptrDesiredMenu = &g_configMenu;
		return;
	}
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ad448-0x800ad560.
void MM_ToggleRows_PlayerCount()
{
	int i;
	struct MenuRow *row;

	for (i = 0; i < 2; i++)
	{
		row = &D230.rowsPlayers1P2P[i];

		// unlock row
		row->stringIndex &= 0x7fff;

		if ((MainFrame_HaveAllPads(i + 1) & 0xffff) == 0)
		{
			// lock row
			row->stringIndex |= 0x8000;
		}
	}

	for (i = 0; i < 3; i++)
	{
		row = &D230.rowsPlayers2P3P4P[i];

		// unlock row
		row->stringIndex &= 0x7fff;

		if ((MainFrame_HaveAllPads(i + 2) & 0xffff) == 0)
		{
			// lock row
			row->stringIndex |= 0x8000;
		}
	}
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ad560-0x800ad5e8.
void MM_MenuProc_1p2p(struct RectMenu *menu)

{
	s16 row;

	struct GameTracker *gGT;
	gGT = sdata->gGT;

	row = menu->rowSelected;

	// if uninitialized
	if (row == -1)
	{
		menu->ptrPrevBox_InHierarchy->state &= ~(ONLY_DRAW_TITLE | DRAW_NEXT_MENU_IN_HIERARCHY);

		gGT->numPlyrNextGame = 1;

		D230.characterSelect_transitionState = 0;
	}

	else
	{
		// if on row 0 or 1
		if ((row >= 0) && (row < 2))
		{
			// row 0 is 1P, row 1 is 2P
			gGT->numPlyrNextGame = menu->rowSelected + 1;

			// go to difficulty box
			menu->ptrNextBox_InHierarchy = &D230.menuDifficulty;

			menu->state |= 0x14;
			return;
		}
	}
	return;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ad5e8-0x800ad678.
void MM_MenuProc_2p3p4p(struct RectMenu *menu)
{
	s16 row;

	struct GameTracker *gGT;
	gGT = sdata->gGT;

	row = menu->rowSelected;

	// if uninitialized
	if (row == -1)
	{
		menu->ptrPrevBox_InHierarchy->state &= ~(ONLY_DRAW_TITLE | DRAW_NEXT_MENU_IN_HIERARCHY);

		gGT->numPlyrNextGame = 1;

		D230.characterSelect_transitionState = 0;
	}
	else
	{
		// row is 0, 1, 2
		if ((row >= 0) && (row < 3))
		{
			// row 0 is 2P, row 1 is 3P, row 2 is 4P
			gGT->numPlyrNextGame = menu->rowSelected + 2;

			D230.MM_State = 2;
			D230.desiredMenuIndex = 2;

			menu->state |= 4;
			return;
		}
	}
	return;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ad678-0x800ad7a4.
void MM_ToggleRows_Difficulty(void)
{
	char bVar1;
	struct GameTracker *gGT;
	int iVar3;
	s16 bitIndex;
	u16 uVar5;
	u32 uVar6;
	int iVar7;

	gGT = sdata->gGT;
	iVar3 = 0;

	// check 3 mods (easy, medium, hard)
	for (iVar7 = 0; iVar7 < 3; iVar7++)
	{
		bitIndex = D230.cupDifficultyUnlockFlags[iVar7];

		// if -1 (for EASY row), skip
		if (-1 == bitIndex)
		{
			continue;
		}

		// assume unlocked
		uVar6 = 1;

		// check 4 bits starting at bitIndex,
		// one for each track in cup
		for (iVar3 = 0; iVar3 < 4; iVar3++)
		{
			bVar1 = (uVar6 != 0);
			uVar6 = 0;

			// if not determined locked
			if (bVar1)
			{
				uVar6 = (int)bitIndex + iVar3;

				// check what is unlocked
				uVar6 = (sdata->gameProgress.unlocks[uVar6 >> 5] >> (uVar6 & 0x1f)) & 1;
			}
		}

		// get current value of lng index,
		// for easy, medium, hard
		uVar5 = D230.cupDifficultyLngIndex[iVar7];

		if (
		    // if locked
		    (uVar6 == 0) &&

		    // If you're in Arcade mode
		    ((gGT->gameMode1 & ARCADE_MODE) != 0) &&

		    // if you are in Arcade or VS cup
		    ((gGT->gameMode2 & CUP_ANY_KIND) != 0))
		{
			// use high bits for "LOCKED"
			uVar5 = uVar5 | 0x8000;
		}

		// save new value
		D230.rowsDifficulty[iVar7].stringIndex = uVar5;
	}
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ad7a4-0x800ad828.
void MM_MenuProc_Difficulty(struct RectMenu *menu)
{
	s16 row;

	row = menu->rowSelected;

	// if uninitialized
	if (row == -1)
	{
		menu->ptrPrevBox_InHierarchy->state &= ~(ONLY_DRAW_TITLE | DRAW_NEXT_MENU_IN_HIERARCHY);
	}

	else
	{
		// if you are on a valid row
		if ((row >= 0) && (row < 3))
		{
			// set difficulty to value, from array of fixed difficulty values
			sdata->gGT->arcadeDifficulty = D230.cupDifficultySpeed[row];

			D230.MM_State = 2;
			D230.desiredMenuIndex = 2;

			menu->state |= 4;
			return;
		}
	}
	return;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ad828-0x800ad8f0.
void MM_MenuProc_SingleCup(struct RectMenu *menu)
{
	s16 row;
	struct GameTracker *gGT;

	gGT = sdata->gGT;
	row = menu->rowSelected;

	if (row == -1)
	{
		menu->ptrPrevBox_InHierarchy->state &= ~(ONLY_DRAW_TITLE | DRAW_NEXT_MENU_IN_HIERARCHY);
		return;
	}

	if ((row >= 0) && (row < 2))
	{
		// disable Cup mode
		gGT->gameMode2 &= ~(CUP_ANY_KIND);

		// if you choose cup mode
		if (menu->rowSelected != 0)
		{
			// enable cup mode
			gGT->gameMode2 |= CUP_ANY_KIND;
		}

		menu->state |= 0x14;

		// if mode is Arcade
		if ((gGT->gameMode1 & ARCADE_MODE) != 0)
		{
			// set next menu to 1P+2P select
			menu->ptrNextBox_InHierarchy = &D230.menuPlayers1P2P;
			D230.characterSelect_transitionState = 1;
			return;
		}

		// if mode is VS

		// set next menu to 2P+3P+4P (vs or battle)
		menu->ptrNextBox_InHierarchy = &D230.menuPlayers2P3P4P;
		D230.characterSelect_transitionState = 2;
	}
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ad8f0-0x800ad980.
void MM_MenuProc_NewLoad(struct RectMenu *menu)
{
	s16 row;

	// row number
	row = menu->rowSelected;

	if (row == -1)
	{
		menu->ptrPrevBox_InHierarchy->state &= ~(ONLY_DRAW_TITLE | DRAW_NEXT_MENU_IN_HIERARCHY);
		return;
	}

	if ((row < 0) || (row > 1))
	{
		return;
	}

	// if Load was chosen
	D230.desiredMenuIndex = row;

	// MM_Title transitioning out
	D230.MM_State = 2;

	menu->state |= 4;
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ad980-0x800ad98c.
struct RectMenu *MM_AdvNewLoad_GetMenuPtr(void)
{
	// menu for new/load
	return &D230.menuAdventure;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800b42b0-0x800b4334.
void MM_ResetAllMenus(void)
{
	for (int i = 0; i < 9; i++)
	{
		struct RectMenu *menu = D230.arrayMenuPtrs[i];

// NOTE(aalhendi): Retail resets one menu per array slot; native walks chained
// menus because overlay 230 data is not reloaded.
#ifdef CTR_NATIVE
		do
		{
			struct RectMenu *next = menu->ptrNextBox_InHierarchy;
#endif

			// Close menu
			menu->state |= 8;
			menu->state &= ~(ONLY_DRAW_TITLE | DRAW_NEXT_MENU_IN_HIERARCHY);

			// Reset ptrNext and ptrPrev
			menu->ptrNextBox_InHierarchy = 0;
			menu->ptrPrevBox_InHierarchy = 0;

#ifdef CTR_NATIVE
			menu = next;
		} while (menu != 0);
#endif
	}

	// unused
	sdata->framesRemainingInMenu = 0xF;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4334-0x800b4364.
void MM_JumpTo_Title_Returning(void)
{
	// return to main menu from another menu
	D230.MM_State = 3;

	// return to main menu
	sdata->ptrDesiredMenu = &D230.menuMainMenu;

	D230.countMeta0xD = D230.title_numFrameTotal;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800b4364-0x800b43f4.
void MM_JumpTo_Title_FirstTime(void)
{
	struct GameTracker *gGT = sdata->gGT;

	MM_ResetAllMenus();

	MainStats_ClearBattleVS();

#if BUILD == EurRetail
	// if you have not chose a language or skipped the language menu
	if (sdata->boolLangChosen == 0)
	{
		sdata->ptrActiveMenu = &D230.menuLngBoot;
		D230.langMenuTimer = 900;
	}
	else
	{
		// if not set to normal main menu
		sdata->ptrActiveMenu = &D230.menuMainMenu;
	}
#else
	// open Main Menu for the first time
	sdata->ptrActiveMenu = &D230.menuMainMenu;
#endif

	D230.timerInTitle = 0;

	// first time in main menu
	// (play crash trophy anim)
	D230.MM_State = 0;

	// reset countdown clock for battle or crystal challenge
	gGT->originalEventTime = 0x2a300;

	D230.menuMainMenu.state &= ~(EXECUTE_FUNCPTR | ONLY_DRAW_TITLE);
	D230.menuMainMenu.state |= DISABLE_INPUT_ALLOW_FUNCPTRS;

	// distance to screen (perspective)
	gGT->pushBuffer[0].distanceToScreen_PREV = 0x100;
	gGT->pushBuffer[0].distanceToScreen_CURR = 0x100;
	gGT->gameMode1 &= ~(TIME_TRIAL);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b43f4-0x800b4430.
void MM_JumpTo_BattleSetup(void)
{
	// Go to battle setup
	sdata->ptrActiveMenu = &D230.menuBattleWeapons;

	D230.menuBattleWeapons.state &= ~(ONLY_DRAW_TITLE);

	MM_Battle_Init();
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4430-0x800b446c.
void MM_JumpTo_TrackSelect(void)
{
	// return to track selection
	sdata->ptrActiveMenu = &D230.menuTrackSelect;

	D230.menuTrackSelect.state &= ~(ONLY_DRAW_TITLE);

	MM_TrackSelect_Init();
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b446c-0x800b44a8.
void MM_JumpTo_Characters(void)
{
	// return to character selection
	sdata->ptrActiveMenu = &D230.menuCharacterSelect;

	D230.menuCharacterSelect.state &= ~(ONLY_DRAW_TITLE);

	MM_Characters_RestoreIDs();
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800b44a8-0x800b44e4.
void MM_JumpTo_Scrapbook(void)
{
	// go to scrapbook
	sdata->ptrActiveMenu = &D230.menuScrapbook;

	D230.menuScrapbook.state &= ~(ONLY_DRAW_TITLE);

	MM_Scrapbook_Init();
}


