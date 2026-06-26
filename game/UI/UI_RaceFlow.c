#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005572c-0x80055840.
void UI_RaceEnd_GetDriverClock(struct Driver *driver)
{
	u8 missileLaunched;
	int avgSpd;
	int timeElapsed;
	int numTimesAttacked;

	if ((driver->actionsFlagSet & ACTION_RACE_TIMER_FROZEN) == 0)
	{
		timeElapsed = driver->timeElapsedInRace;
		driver->actionsFlagSet |= ACTION_RACE_TIMER_FROZEN;

		if (timeElapsed != 0)
		{
			// get average speed over time
			avgSpd = driver->distanceDriven * 100;
			driver->distanceDriven = avgSpd / timeElapsed;
		}

		// if missiles launched is less than 4
		if ((u8)driver->numTimesMissileLaunched < 4)
		{
			driver->NumMissilesComparedToNumAttacks = 0xffffffff;
		}

		// if missiles launched is more than 4
		else
		{
			// number of missiles launched
			missileLaunched = driver->numTimesMissileLaunched;

			// compare number of missiles to number of attacks
			driver->NumMissilesComparedToNumAttacks = (int)(((u8)driver->numTimesAttacking << 0xc) / missileLaunched);
		}

		numTimesAttacked = 0;

		// count number of times you were attacked in race
		for (int i = 0; i < 8; i++)
		{
			numTimesAttacked += (u8)driver->numTimesAttackedByPlayer[i];
		}

		driver->numTimesAttacked = numTimesAttacked;

		// if driver is in first place
		if (driver->driverRank == 0)
		{
			// duplicate amount of time spent in last place
			driver->TimeWinningDriverSpentLastPlace = driver->timeSpentInLastPlace;
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80055840-0x80055c90.

// You see this in 1P mode, right before traffic lights count down
void UI_RaceStart_IntroText1P(void)
{
	s16 windowHeight;
	struct GameTracker *gGT;
	int iVar2;
	int gameMode;
	int posX;
	s16 *txtArray;
	char *pcVar6;
	s16 sVar7;
	s16 font;
	int textID;
	int transition;
	char trackText[24];
	RECT rect;
	int colors[2];

	gGT = sdata->gGT;

	// by default, do not transition
	// title bars to off-screen
	transition = 0;

	gameMode = gGT->gameMode1;

	// If you are not in a relic race
	if ((gameMode & RELIC_RACE) == 0)
	{
		// BONUS ROUND
		textID = 0xbe;

		// If you are not in Crystal challenge
		if ((gameMode & CRYSTAL_CHALLENGE) == 0)
		{
			// If you are not in Adventure Cup
			if ((gameMode & ADVENTURE_CUP) == 0)
			{
				// If you are not in Arcade or VS cup
				if ((gGT->gameMode2 & CUP_ANY_KIND) == 0)
				{
					// ARCADE
					textID = 0x4e;

					if (
					    // If you're in Arcade Mode
					    ((gameMode & ARCADE_MODE) != 0) ||

					    (
					        // TIME TRIAL
					        textID = 0x4d,

					        // if you are in time trial mode
					        (gameMode & TIME_TRIAL) != 0))
					{
						goto LAB_80055930;
					}

					if (-1 < gameMode)
					{
						// TROPHY RACE
						textID = 0xb7;

						// If you're in a CTR Token Race
						if ((gGT->gameMode2 & TOKEN_RACE) != 0)
						{
							// CTR CHALLENGE
							textID = 0x176;
						}
						goto LAB_80055930;
					}
					iVar2 = gGT->bossID;
					txtArray = &data.lng_challenge[0];
				}

				// If you are in Arcade or VS cup
				else
				{
					// Get Cup ID
					iVar2 = gGT->cup.cupID;
					txtArray = &data.arcadeVsCupStringIndex[0];
				}
			}

			// If you are in Adventure Cup
			else
			{
				// Get Cup ID
				iVar2 = gGT->cup.cupID;
				txtArray = &data.advCupStringIndex[0];
			}

			// Get the name of the cup
			// Wumpa, Nitro, Crystal
			// Red, Green, Purple, etc
			textID = txtArray[iVar2];
		}
	}

	// If you are in Relic Race
	else
	{
		// RELIC RACE
		textID = 0xb8;
	}
LAB_80055930:

	// if fly-in animation is one second away from finishing
	if (gGT->cameraDC->transitionFrame < 0x1f)
	{
		// use this to transition title bars to off-screen
		transition = 0x1e - gGT->cameraDC->transitionFrame;
	}

	// RaceFlag_IsFullyOnScreen
	iVar2 = RaceFlag_IsFullyOnScreen();

	// if not
	if (iVar2 == 0)
	{
		// Draw big string
		font = FONT_BIG;

		// used for transitioning bars to off-screen
		windowHeight = transition;

		if (

		    // If you are not in Adventure cup
		    ((gameMode & ADVENTURE_CUP) == 0) &&

		    // If you are not in Arcade or VS cup
		    (((gGT->gameMode2 & CUP_ANY_KIND) == 0)))
		{
			// X-value, X + W/2
			posX = gGT->pushBuffer[0].rect.x + ((gGT->pushBuffer[0].rect.w << 0x10) >> 0x11);

			// string of top title bar
			pcVar6 = sdata->lngStrings[textID];

			// Y-value that transitions title text to off-screen
			sVar7 = gGT->pushBuffer[0].rect.y - (windowHeight + -7);
		}

		// If you are in any cup of any kind
		else
		{
			// Name of Cup

			// uVar9 * 4
			DecalFont_DrawLine(sdata->lngStrings[textID],

			                   gGT->pushBuffer[0].rect.x + ((gGT->pushBuffer[0].rect.w << 0x10) >> 0x11),

			                   ((gGT->pushBuffer[0].rect.y - (transition + -7)) + -6), FONT_BIG, (JUSTIFY_CENTER | ORANGE));

			// Track 1/4, 2/4, 3/4, 4/4 in cup
			sprintf(trackText, "%s %ld/4",

			        sdata->lngStrings[LNG_TRACK],

			        // Track Index (0, 1, 2, 3) + 1
			        CTR_PRINTF_PSX_LONG((gGT->cup.trackIndex) + 1));

			// string of top title bar
			pcVar6 = trackText;

			// X-value, centered
			posX = 0x100;

			// Draw small string
			font = FONT_SMALL;

			// Y-value that transitions title text to off-screen
			sVar7 = (gGT->pushBuffer[0].rect.y - (transition + -7)) + 0xb;
		}

		// Print top title text "Arcade, Time Trial, etc"
		DecalFont_DrawLine(pcVar6, posX, sVar7, font, (JUSTIFY_CENTER | ORANGE));

		// Print the name of the level
		// Crash Cove, Roos Tubes, etc
		DecalFont_DrawLine(

		    // 83a88 = 110
		    // 8d878 + 110*4 -> Dingo Canyon

		    // Level ID
		    sdata->lngStrings[data.metaDataLEV[gGT->levelID].name_LNG],

		    gGT->pushBuffer[0].rect.x + ((gGT->pushBuffer[0].rect.w << 0x10) >> 0x11),

		    (gGT->pushBuffer[0].rect.y + gGT->pushBuffer[0].rect.h + transition + -0x17), FONT_BIG, (JUSTIFY_CENTER | ORANGE));

		// same for all
		rect.x = gGT->pushBuffer[0].rect.x;
		rect.w = gGT->pushBuffer[0].rect.w;

		// 2-pixel height
		// random generic color
		colors[0] = sdata->battleSetup_Color_UI_1;
		rect.h = 2;

		// Draw tiny rectangle near big black title bar (first)
		rect.y = gGT->pushBuffer[0].rect.y - (windowHeight + -0x1c);

		Color color;
		color.self = colors[0];

		uint32_t *ot = gGT->backBuffer->otMem.uiOT;

		CTR_Box_DrawSolidBox(&rect, color, ot);

		// Draw tiny rectangle near big black title bar (second)
		rect.y = gGT->pushBuffer[0].rect.y + gGT->pushBuffer[0].rect.h + windowHeight + -0x1e;
		CTR_Box_DrawSolidBox(&rect, color, ot);

		// 30-pixel height
		// clear RGB, keep alpha (which is zero anyway)
		colors[0] = colors[0] & 0xff000000;
		color.self = colors[0];
		rect.h = 0x1e;

		// draw big black title bar (first)
		rect.y = gGT->pushBuffer[0].rect.y - windowHeight;
		CTR_Box_DrawSolidBox(&rect, color, ot);

		// draw big black title bar (second)
		rect.y = gGT->pushBuffer[0].rect.y + gGT->pushBuffer[0].rect.h + windowHeight + -0x1e;
		CTR_Box_DrawSolidBox(&rect, color, ot);
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80055c90-0x8005607c.
void UI_RaceEnd_MenuProc(struct RectMenu *menu)
{
	s16 option;
	struct GameTracker *gGT;

	gGT = sdata->gGT;

	if (menu->unk1e != 0)
	{
		menu->drawStyle &= ~(0x100);

		// if more than 2 screens
		if (2 < gGT->numPlyrCurrGame)
		{
			menu->drawStyle |= 0x100;
		}

		return;
	}

	int row = menu->rowSelected;
	if (row < 0)
	{
		return;
	}

	option = menu->rows[row].stringIndex;

	// if not SAVE GHOST
	if (option != 9)
	{
		// make Menu invisible
		RECTMENU_Hide(menu);
	}

	sdata->framesSinceRaceEnded = 0;
	sdata->numIconsEOR = 1;

	// Press * To Continue
	// do not put this in the switch,
	// switch needs to be a "small" jump table,
	// and an offest this large could bloat table
	if (option == 0xc9)
	{
		sdata->menuReadyToPass |= 1;
		return;
	}

	switch (option)
	{
	// Quit
	case 3:
	{
		// Erase ghost of previous race from RAM
		GhostTape_Destroy();

		// go back to main menu
		sdata->mainMenuState = MAIN_MENU_TITLE;

		// load LEV of main menu
		MainRaceTrack_RequestLoad(0x27);
		break;
	}

	case 4:
	{
		// Turn off HUD
		gGT->hudFlags &= 0xfe;

		if (RaceFlag_IsFullyOffScreen() == 1)
		{
			RaceFlag_BeginTransition(1);
		}

		sdata->Loading.stage = LOAD_RESTART;

		// clear backup,
		// keep music,
		// destroy "most" fx, let menu fx play to end
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80055df0-0x80055e04 for retry stop-audio args.
		howl_StopAudio(1, 0, 0);

		// if did not improve time, then dont
		// overwrite old ghost with new ghost
		if ((gGT->gameModeEnd & PLAYER_GHOST_BEAT) == 0)
		{
			break;
		}

		sdata->boolReplayHumanGhost = 1;

		// slower than ND's copy, I know, we'll
		// come up with a modern-gcc friendly way
		// to sort the LWs and SWs later
		memcpy(sdata->ptrGhostTapePlaying, sdata->GhostRecording.ptrGhost, 0x3e00);

		// Make P2 the character that is saved in the
		// header of the ghost that you will see in the race
		data.characterIDs[1] = sdata->ptrGhostTapePlaying->characterID;

		// no ghosts are drawing
		sdata->boolGhostsDrawing = 0;

		break;
	}

	// Change Character, or Change Level
	case 5:
	case 6:
	{
		// Erase ghost of previous race from RAM
		GhostTape_Destroy();

		// MAIN_MENU_CHARACTERS or MAIN_MENU_TRACK_SELECT
		sdata->mainMenuState = (MainMenuState)(option - 4);

		// when loading is done
		// add flag for "in menus"
		sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

		// load LEV of main menu
		MainRaceTrack_RequestLoad(0x27);
		break;
	}

	// Save Ghost
	case 9:
	{
		sdata->framesSinceRaceEnded = 0x3f9;

		// Set Load/Save to Ghost mode
		SelectProfile_ToggleMode(0x31);

		// Change active Menu to GhostSelection
		sdata->ptrActiveMenu = &data.menuGhostSelection;
		break;
	}

	// Change Setup
	case 10:
	{
		// go to battle setup screen
		sdata->mainMenuState = MAIN_MENU_BATTLE_SETUP;

		// load LEV of main menu
		MainRaceTrack_RequestLoad(0x27);
		break;
	}

	// Exit To Map
	case 0xd:
	{
		sdata->Loading.OnBegin.AddBitsConfig0 |= ADVENTURE_ARENA;
		sdata->Loading.OnBegin.RemBitsConfig8 |= TOKEN_RACE;

		sdata->Loading.OnBegin.RemBitsConfig0 |= (CRYSTAL_CHALLENGE | RELIC_RACE);

		if ((gGT->gameMode1 & ADVENTURE_CUP) != 0)
		{
			sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_CUP;
			MainRaceTrack_RequestLoad(GEM_STONE_VALLEY);
			break;
		}

		if (IS_BOSS_RACE(gGT->gameMode1))
		{
			sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_BOSS;
			sdata->Loading.OnBegin.AddBitsConfig8 |= SPAWN_AT_BOSS;
		}

		MainRaceTrack_RequestLoad(gGT->prevLEV);
		break;
	}
	}
}
