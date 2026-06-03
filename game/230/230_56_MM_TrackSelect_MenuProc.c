#include <common.h>

void MM_TrackSelect_MenuProc(struct RectMenu *menu)
{
	char bVar1;
	char bVar2;
	char bVar3;
	char bVar4;
	s16 sVar5;
	s16 currTrack;
	s16 sVar7;
	s16 elapsedFrames;
	u32 uVar8;
	int iVar9;
	int iVar10;
	int iVar11;
	int *starColor;
	u32 uVar14;
	u32 uVar15;
	int iVar17;
	int iVar18;
	RECT r;
	RECT q;
	RECT p;
	u32 local_44;
	s16 numTracks;

	struct MainMenu_LevelRow *selectMenu;
	struct GameTracker *gGT = sdata->gGT;

	elapsedFrames = D230.trackSel_transitionFrames;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800b00d4-0x800b02b0.
	// if you are not in track selection menu
	if (D230.trackSel_transitionState != IN_MENU)
	{
		// if transitioning in
		if (D230.trackSel_transitionState == ENTERING_MENU)
		{
			// make error message posY appear
			// near bottom of screen
			sdata->errorMessagePosIndex = 1;

			// if you are in Battle mode
			if ((gGT->gameMode1 & BATTLE_MODE) != 0)
			{
				// make error message posY appear
				// near top of screen
				sdata->errorMessagePosIndex = 2;
			}

			MM_TransitionInOut(&D230.transitionMeta_trackSel[0], elapsedFrames, 8);

			// ran out of frames
			if (elapsedFrames == 0)
			{
				// menu is now in focus
				D230.trackSel_transitionState = IN_MENU;
			}
			else
			{
				elapsedFrames--;
			}
		}
		// transitioning out
		else if (D230.trackSel_transitionState == EXITING_MENU)
		{
			MM_TransitionInOut(&D230.transitionMeta_trackSel[0], elapsedFrames, 8);
			elapsedFrames++;

			if (elapsedFrames > 12)
			{
				sdata->errorMessagePosIndex = 0;

				// if track has not been chosen
				if (D230.trackSel_StartRaceAfterFadeOut == 0)
				{
					// return to character selection
					sdata->ptrDesiredMenu = &D230.menuCharacterSelect;
					MM_Characters_RestoreIDs();
					return;
				}

				// if track has been chosen

				// if you are in battle mode
				if ((gGT->gameMode1 & BATTLE_MODE) != 0)
				{
					// open weapon selection menu
					sdata->ptrDesiredMenu = &D230.menuBattleWeapons;
					MM_Battle_Init();
					return;
				}

				// if you are not in battle mode

				// if you are in time trial mode
				if ((gGT->gameMode1 & TIME_TRIAL) != 0)
				{
					// allocate room at the end of RAM for ghosts
					sdata->ptrGhostTapePlaying = MEMPACK_AllocHighMem(0x3e00 /*, R230.s_loaded_ghost_data*/);

					memset(sdata->ptrGhostTapePlaying, 0, 0x28);

					// by default, dont show ghost in race
					sdata->boolReplayHumanGhost = 0;

					SelectProfile_ToggleMode(0x30);

					// open the ghost selection menu
					sdata->ptrDesiredMenu = &data.menuGhostSelection;
					return;
				}

				// passthrough Menu for the function
				// QueueLoadTrack
				sdata->ptrDesiredMenu = &data.menuQueueLoadTrack;

				// make error message posY appear
				// near middle of screen
				sdata->errorMessagePosIndex = 0;

				return;
			}
		}
	}
	D230.trackSel_transitionFrames = elapsedFrames;

	// default arcade tracks
	selectMenu = &D230.arcadeTracks[0];
	numTracks = 18;

	// if you are in battle mode
	if ((gGT->gameMode1 & BATTLE_MODE) != 0)
	{
		selectMenu = &D230.battleTracks[0];
		numTracks = 7;
	}

	currTrack = menu->rowSelected;
	sdata->trackSelBackup = currTrack;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800b02b0-0x800b04c8.
	// if lap selection menu is closed
	if (D230.trackSel_boolOpenLapBox == 0)
	{
		int importantButton = sdata->buttonTapPerPlayer[0] & (BTN_UP | BTN_DOWN | BTN_TRIANGLE | BTN_SQUARE_one | BTN_CROSS_one | BTN_CIRCLE);

		if (
		    // if not changing levels
		    (D230.trackSel_changeTrack_frameCount == 0) &&

		    // only check buttons if IN_MENU
		    (D230.trackSel_transitionState == IN_MENU) &&

		    // desired button pressed
		    (importantButton != 0))
		{
			switch (importantButton)
			{
			case BTN_UP:

				// look for unlocked track
				do
				{
					currTrack--;

					// if index is negative
					if (currTrack < 0)
						// set to the last track
						currTrack = numTracks - 1;

				} while (!MM_TrackSelect_boolTrackOpen(&selectMenu[currTrack]));

				D230.trackSel_currTrack = currTrack;
				D230.trackSel_changeTrack_frameCount = 3;
				D230.trackSel_direction = 1;

				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b034c-0x800b035c for track-select previous SFX.
				OtherFX_Play(0, 1);
				break;

			case BTN_DOWN:

				// look for unlocked track
				do
				{
					currTrack++;

					// if you go beyond max number of tracks
					if (currTrack >= numTracks)
						// set to the first trrack
						currTrack = 0;

				} while (!MM_TrackSelect_boolTrackOpen(&selectMenu[currTrack]));

				D230.trackSel_currTrack = currTrack;
				D230.trackSel_changeTrack_frameCount = 3;
				D230.trackSel_direction = -1;

				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b03bc-0x800b03cc for track-select next SFX.
				OtherFX_Play(0, 1);
				break;

			case BTN_CROSS_one:
			case BTN_CIRCLE:

				// "enter/confirm" sound
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0434-0x800b0444 for track-select confirm SFX.
				OtherFX_Play(1, 1);

				// if not Battle or Time Trial, open LapSelectMenu
				if ((gGT->gameMode1 & (BATTLE_MODE | TIME_TRIAL)) == 0)
				{
					// open lap select menu
					D230.trackSel_boolOpenLapBox = D230.trackSel_transitionState;
					break;
				}

				// if Battle or Time Trial, skip straight to level
				D230.trackSel_StartRaceAfterFadeOut = D230.trackSel_transitionState;
				D230.trackSel_transitionState = EXITING_MENU;
				break;

			case BTN_TRIANGLE:
			case BTN_SQUARE_one:

				// "go back" sound
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0490-0x800b04a4 for track-select back SFX.
				OtherFX_Play(2, 1);

				D230.trackSel_StartRaceAfterFadeOut = 0;
				D230.trackSel_transitionState = EXITING_MENU;
				break;
			default:
				break;
			}

			// clear gamepad input (for menus)
			RECTMENU_ClearInput();
		}
	}

	// if lap selection menu is open
	else
	{
		s16 lapSelTransitionState = 0;

		// copy LapRow from 8d920 to temp variable b55ae
		D230.menuLapSel.rowSelected = sdata->uselessLapRowCopy;

		// If you're in track selection menu
		if (D230.trackSel_transitionState == IN_MENU)
		{
			lapSelTransitionState = RECTMENU_ProcessInput(&D230.menuLapSel);
		}

		RECTMENU_DrawSelf(&D230.menuLapSel, D230.transitionMeta_trackSel[2].currX, D230.transitionMeta_trackSel[2].currY, 0xa4);

		// put LapRow back into 8d920
		sdata->uselessLapRowCopy = D230.menuLapSel.rowSelected;

		// get lap count
		gGT->numLaps = D230.lapRowVal[D230.menuLapSel.rowSelected];

		// if it is time to start the race
		if (lapSelTransitionState == 1)
		{
			// try to start the race
			D230.trackSel_transitionState = EXITING_MENU;

			// if this is 1 (which it is), the race starts,
			// otherwise, you go back to character selection
			D230.trackSel_StartRaceAfterFadeOut = lapSelTransitionState;
		}

		// If it is not time to start the race
		else
		{
			if (lapSelTransitionState == -1)
			{
				// close lap selection menu
				D230.trackSel_boolOpenLapBox = 0;
			}
		}

		// If "One Lap Race" Cheat is enabled
		if ((gGT->gameMode2 & CHEAT_ONELAP) != 0)
		{
			// Set number of Laps to 1
			gGT->numLaps = 1;
		}
	}

	// decrease frame from track list motion
	iVar9 = D230.trackSel_changeTrack_frameCount + -1;
	if ((0 < D230.trackSel_changeTrack_frameCount) && (D230.trackSel_changeTrack_frameCount = iVar9, iVar9 == 0))
	{
		menu->rowSelected = D230.trackSel_currTrack;
	}

	// not transitioning
	uVar14 = 0;

	// if you are transitioning out of level selection
	if ((D230.trackSel_changeTrack_frameCount != 0) || (D230.trackSel_transitionState == EXITING_MENU))
	{
		// transitioning,
		// which means stop drawing track video,
		// just draw icon
		uVar14 = 1;
	}

	MM_TrackSelect_Video_State(uVar14);

	gGT->currLEV = selectMenu[menu->rowSelected].levID;
	iVar9 = (int)menu->rowSelected + -1;

	for (iVar18 = 0; iVar18 < 4; iVar18++)
	{
		do
		{
			iVar10 = iVar9;
			if (iVar9 < 0)
			{
				iVar10 = numTracks - 1;
			}

			uVar8 = MM_TrackSelect_boolTrackOpen(&selectMenu[iVar10]);

			iVar9 = iVar10 - 1;
		} while ((uVar8 & 0xffff) == 0);

		iVar9 = iVar10 - 1;
	}
	iVar18 = 0;
	iVar9 = 0;

	// loop through tracks in track list
	do
	{
		// This part actually "moves" the rows,
		// when pressing the Up and Down buttons on D-Pad
		uVar15 = ((iVar9 >> 0x10) + -4) * 0x73;
		if (0 < D230.trackSel_changeTrack_frameCount)
		{
			uVar15 = uVar15 + (((3 - D230.trackSel_changeTrack_frameCount) * 0x73) / 3) * (int)D230.trackSel_direction;
		}

// This is just MATH_Cos and Math_Sin
#if 0
		// approximate trigonometry
		sVar7 = (s16)data.trigApprox[uVar15];
		iVar9 = data.trigApprox[uVar15] >> 0x10;

		if ((uVar15 & 0x400) == 0)
		{
			iVar17 = (int)sVar7;
			iVar11 = iVar9;
			if ((uVar15 & 0x800) != 0)
			{
				iVar11 = -iVar9;
				goto LAB_800b0774;
			}
		}
		else
		{
			iVar11 = (int)sVar7;
			iVar17 = iVar9;
			if ((uVar15 & 0x800) == 0)
			{
				iVar11 = -iVar11;
			}
			else
			{
			LAB_800b0774:
				iVar17 = -iVar17;
			}
		}
#endif
		r.w = 256;
		r.h = 0x19;

		// posX of track list
		iVar11 = (u32)D230.transitionMeta_trackSel[0].currX + (MATH_Cos(uVar15) * 0x19 >> 9) + -0xb4;

		// posY of track list
		iVar9 = (u32)D230.transitionMeta_trackSel[0].currY + (MATH_Sin(uVar15) * 200 >> 0xc);

		sVar7 = (s16)iVar9 + 0x60;
		r.x = (s16)iVar11;
		r.y = sVar7;

		// if you are in time trial mode
		if ((gGT->gameMode1 & TIME_TRIAL) != 0)
		{
			// backup level ID
			sVar5 = gGT->levelID;

			// draw stars if N Tropy or Oxide are beaten,
			// loop twice
			for (iVar17 = 0; iVar17 < 2; iVar17++)
			{
				// set level ID to the level you're hovering on, in the main menu
				gGT->levelID = selectMenu[iVar10].levID;

				// (useless?)
				GAMEPROG_GetPtrHighScoreTrack();

				int timeTrialFlags = sdata->gameProgress.highScoreTracks[gGT->levelID].timeTrialFlags;

				// if star is earned
				if (((timeTrialFlags >> D230.timeTrialFlagGet[iVar17]) & 1) != 0)
				{
					// pointer to color data of star
					starColor = data.ptrColor[D230.timeTrialStarCol[iVar17]];

					struct Icon **iconPtrArray = ICONGROUP_GETICONS(gGT->iconGroup[5]);

					DecalHUD_DrawPolyGT4(iconPtrArray[0x37], iVar11 + 256 + 4, (int)sVar7 + iVar17 * 8 + 4,

					                     // pointer to PrimMem struct
					                     &gGT->backBuffer->primMem,

					                     // pointer to OT mem
					                     gGT->pushBuffer_UI.ptrOT,

					                     // color data
					                     starColor[0], starColor[1], starColor[2], starColor[3],

					                     0, FP(1.0));
				}
			}
			// restore levelID
			gGT->levelID = sVar5;

			// (useless?)
			GAMEPROG_GetPtrHighScoreTrack();
		}

		// alphabet
		// DAT_80083A88 goes to "n"
		// + 0x18 -> "o"
		// + 0x18 -> "p"
		// + 0x18 -> "q"
		// and so on

		// Draw string
		DecalFont_DrawLine(sdata->lngStrings[data.metaDataLEV[selectMenu[iVar10].levID].name_LNG], (iVar11 + 8), (iVar9 + 0x65), FONT_BIG, ORANGE);

		if ((D230.trackSel_changeTrack_frameCount == 0) && ((s16)iVar18 == 4))
		{
			// if you are in time trial mode
			if ((gGT->gameMode1 & TIME_TRIAL) != 0)
			{
				// Check if this track has Ghost Data
				uVar15 = RefreshCard_BoolGhostForLEV(selectMenu[iVar10].levID);

				// If this track has Ghost Data
				if ((uVar15 & 0xffff) != 0)
				{
					// Flash Colors
					if ((sdata->frameCounter & 4) == 0)
					{
						uVar14 = (JUSTIFY_CENTER | WHITE);
					}
					else
					{
						uVar14 = (JUSTIFY_CENTER | PERIWINKLE);
					}

					// "GHOST DATA EXISTS"
					DecalFont_DrawLine(sdata->lngStrings[0x6B], (iVar11 + 8 + 0x78), (iVar9 + 0x76), FONT_SMALL, uVar14);
				}
			}
			q.x = r.x + 6;
			q.y = r.y + 4;
			q.w = r.w - 12;
			q.h = r.h - 8;

			CTR_Box_DrawClearBox(&q, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, gGT->backBuffer->otMem.startPlusFour);
		}

		// Draw 2D Menu rectangle background
		RECTMENU_DrawInnerRect(&r, 0, gGT->backBuffer->otMem.startPlusFour);

		do
		{
			iVar10++;

			if (numTracks <= iVar10)
			{
				iVar10 = 0;
			}
			uVar8 = MM_TrackSelect_boolTrackOpen(&selectMenu[iVar10]);

		} while ((uVar8 & 0xffff) == 0);

		iVar18 = iVar18 + 1;
		iVar9 = iVar18 * 0x10000;
		if (8 < iVar18 * 0x10000 >> 0x10)
		{
			p.w = 0xb0;
			p.h = 0x4b;

			// posX of "SELECT LEVEL"
			p.x = D230.transitionMeta_trackSel[1].currX + 0x134;

			// posY of "SELECT LEVEL"
			// near-top if map exists, near-mid if no map
			p.y = D230.transitionMeta_trackSel[1].currY + 0x3a;

			if (-1 < selectMenu[menu->rowSelected].mapTextureID)
				p.y = D230.transitionMeta_trackSel[1].currY + 5;

			// _D230.trackSel_boolOpenLapBox is the boolean to show
			// the selection menu for number of laps:
			// 3, 5, 7

			// If the lap selection menu is closed
			if (D230.trackSel_boolOpenLapBox == 0)
			{
				// "SELECT"
				DecalFont_DrawLine(sdata->lngStrings[0x69], (D230.transitionMeta_trackSel[3].currX + 0x18c), (D230.transitionMeta_trackSel[3].currY + (u32)p.y),
				                   FONT_BIG, (JUSTIFY_CENTER | ORANGE));

				// "LEVEL"
				DecalFont_DrawLine(sdata->lngStrings[0x6a], (D230.transitionMeta_trackSel[3].currX + 0x18c),
				                   (D230.transitionMeta_trackSel[3].currY + (u32)p.y + 0x10), FONT_BIG, (JUSTIFY_CENTER | ORANGE));
			}

			// next, draw the map icon, below "SELECT LEVEL",
			// exactly 0x22 (34) pixels below the text
			p.y += 0x22;

			if ((-1 < selectMenu[menu->rowSelected].mapTextureID) &&

			    // If lap selection menu is closed
			    (D230.trackSel_boolOpenLapBox == 0))
			{
				const int mapBoxHeight = 100;
				int mapID = selectMenu[menu->rowSelected].mapTextureID;
				struct Icon *iconMap0 = gGT->ptrIcons[mapID + 0];
				struct Icon *iconMap1 = gGT->ptrIcons[mapID + 1];

				// icon data
				bVar1 = iconMap0->texLayout.v2;
				bVar2 = iconMap0->texLayout.v0;
				bVar3 = iconMap1->texLayout.v2;
				bVar4 = iconMap1->texLayout.v0;

				iVar9 = (iconMap0->texLayout.u1 - iconMap0->texLayout.u0);

				// draw six track minimaps on menu
				// map 1 is the regular color, which is white
				// map 2 is blue and shifted 2px to the left
				// map 3 is blue and shifted 2px to the right
				// map 4 is blue and shifted 1px downwards
				// map 5 is blue and shifted 1px upwards
				// map 6 is black and shifted 6px downwards and 12px to the right
				for (iVar18 = 0; iVar18 < 6; iVar18++)
				{
					iVar10 = ((((u32)bVar1 - (u32)bVar2) + (u32)bVar3) - (u32)bVar4);

					UI_Map_DrawMap(
					    // top half
					    iconMap0,

					    // bottom half
					    iconMap1,

					    // X
					    D230.drawMapOffset[iVar18].offsetX + p.x + (D230.transitionMeta_trackSel[2].currX - D230.transitionMeta_trackSel[1].currX) +
					        (0xb0 >> 1) + (iVar9 >> 1),

					    // Y
					    D230.drawMapOffset[iVar18].offsetY + p.y + (D230.transitionMeta_trackSel[2].currY - D230.transitionMeta_trackSel[1].currY) + 0x49 +
					        (mapBoxHeight >> 1) + (iVar10 >> 1),

					    // pointer to PrimMem struct
					    &gGT->backBuffer->primMem,

					    // pointer to OT mem
					    gGT->pushBuffer_UI.ptrOT,

					    // 1 = draw map with regular color (white) - used for the main layer of the minimap in the track select screen
					    // 2 = draw map blue - used for the outline of the minimap in the track select screen
					    // 3 = draw map black - used for the shadow of the minimap in the track select screen
					    D230.drawMapOffset[iVar18].type);
				}
			}

			MM_TrackSelect_Video_Draw(&p, selectMenu, (int)(s16)D230.trackSel_currTrack, (u32)(D230.trackSel_transitionState == EXITING_MENU), 0);

			return;
		}
	} while (true);
}
