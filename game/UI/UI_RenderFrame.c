#include <common.h>

// To do: add a header

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80052f98-0x80054298.
// CTR_NATIVE only adds an ST1 map-metadata null guard below.
void UI_RenderFrame_Racing()
{
	s16 sVar1;
	s16 sVar2;
	u8 bVar3;
	int partTimeVariable1;
	int *ptrColor;
	u8 *pbVar6;
	int i;
	struct PushBuffer *pb;
	u32 partTimeVariable5;
	struct Icon *iconPtr;
	u_long *primMemCurr;
	char *fmt;
	int partTimeVariable3;
	POLY_G4 *TurboCounterBar;
	s16 sVar17;
	u32 local_7c;
	u32 local_78;
	u32 local_70;
	struct Driver *playerStruct;
	struct UiElement2D *hudStructPtr;
	void *levPtrMap;
	char cVar22;
	SVec2 wumpaModelPos;
	SVec2 letterCtrPos;
	char string[24];
	SVec2 turboCountPos;
	u16 local_30[2];
	struct Thread *playerThread;
	u_long *ptrOT;
	struct DB *backBuffer;
	struct Thread *turboThread;
	struct Turbo *turboThreadObject;
	int offset;
	u32 mapPosX;
	u32 mapPosY;

	offset = 0;

	struct GameTracker *gGT;
	gGT = sdata->gGT;

	int numPlyr = gGT->numPlyrCurrGame;
	int gameMode1 = gGT->gameMode1;

	// Get pointer to array of HUD structs
	hudStructPtr = (struct UiElement2D *)data.hudStructPtr[numPlyr - 1];

	levPtrMap = 0;

	// adding this here so the compiler doesn't complain
	wumpaModelPos.x = 0;
	wumpaModelPos.y = 0;
	turboCountPos.x = 0;
	turboCountPos.y = 0;

	UI_WeaponBG_AnimateShine();

	// if time on clock is zero
	if (gGT->elapsedEventTime == 0)
	{
		for (i = 0; i < 8; i++)
		{
			data.rankIconsTransitionTimer[i] = 0;

			pbVar6 = &sdata->kartSpawnOrderArray[i];

			data.rankIconsCurr[i] = (u16)*pbVar6;
			data.rankIconsDesired[i] = (u16)*pbVar6;
		}
	}

	// If not drawing intro-race cutscene
	if ((gameMode1 & START_OF_RACE) == 0)
	{
		if ((gGT->hudFlags & 0x20) == 0)
		{
			// If you press Triangle
			if ((sdata->gGamepads->gamepad[0].buttonsTapped & 0x40000) != 0)
			{
				// if & 8, remove bit 8,
				// if !& 8, add bit 8,
				// toggle map and speedometer
				sdata->HudAndDebugFlags ^= 8;
			}
		}
		else
		{
			gGT->hudFlags &= 0xdf;
		}
	}

	// numPlyrCurrGame is 0
	if ((numPlyr == '\0') &&

	    ((gGT->drivers[0]->actionsFlagSet & ACTION_BOT) != 0))
	{
		// force draw speedometer, and not map, why?
		sdata->HudAndDebugFlags = 8;
	}

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native can load levels before ST1 map metadata is present.
	if ((gGT->level1->ptrSpawnType1 != 0) && (gGT->level1->ptrSpawnType1->count != 0))
#else
	if (gGT->level1->ptrSpawnType1->count != 0)
#endif
	{
		void **pointers = ST1_GETPOINTERS(gGT->level1->ptrSpawnType1);
		levPtrMap = pointers[ST1_MAP];
	}

	// If you are not in Relic Race, and not in battle mode,
	// and not in time trial
	if ((gameMode1 & (RELIC_RACE | TIME_TRIAL | BATTLE_MODE)) == 0)
	{
		UI_DrawRankedDrivers();
	}

	// pointer to first Player thread
	playerThread = gGT->threadBuckets[0].thread;

	cVar22 = '\0';
	if (playerThread != 0)
	{
		// Loop through all player threads
		do
		{
			// pointer to player structure
			playerStruct = (struct Driver *)playerThread->object;
			// if player has not driven backwards very far,
			if ((playerStruct->distanceDrivenBackwards < 0x1f5)

			    ||

			    ((playerStruct->actionsFlagSet & ACTION_DRIVING_WRONG_WAY) == 0))
			{
			LAB_80053260:
				if ((gameMode1 & PAUSE_ALL) == 0)
				{
					// execute Jump meter and landing boost processes
					UI_JumpMeter_Update(playerStruct);
				}
			}

			// if racer has travelled
			// wrong way for too long
			else
			{
				if ((gameMode1 & PAUSE_ALL) == 0)
				{
					pb = &gGT->pushBuffer[playerStruct->driverID];

					// if "Time on clock" last 0xXX u8 is greater than 0x80 and less than 0xFF
					if ((gGT->elapsedEventTime & 0x80) != 0)
					{
						DecalFont_DrawLine(sdata->lngStrings[LNG_WRONG_WAY],

						                   // midpointX
						                   pb->rect.x + (pb->rect.w >> 1),

						                   // midpointY, 0x1e higher
						                   pb->rect.y + (pb->rect.h >> 1) - 0x1e,

						                   FONT_BIG, (JUSTIFY_CENTER | ORANGE));
					}

					// The text will not show if the last u8 is more than 0x00 and less than 0x7F.
					// This is what makes the text flicker, rather than drawing solid

					cVar22 = '\x01';
					goto LAB_80053260;
				}
			}

			if (
			    // numPlyrCurrGame is less than 2 (1P mode)
			    (numPlyr < 2) &&
			    // if want to draw speedometer
			    ((sdata->HudAndDebugFlags & 8) != 0))
			{
				UI_DrawSpeedNeedle(hudStructPtr[9].x + offset, hudStructPtr[9].y, playerStruct);
				UI_JumpMeter_Draw(hudStructPtr[6].x, hudStructPtr[6].y, playerStruct);
				UI_DrawSlideMeter(hudStructPtr[8].x + offset, hudStructPtr[8].y, playerStruct);
				UI_DrawSpeedBG();
			}

			if ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
			{
				// If you're not in Battle Mode
				if ((gameMode1 & BATTLE_MODE) == 0)
				{
					// Draw powerslide meter
					UI_DrawSlideMeter(hudStructPtr[8].x + offset, hudStructPtr[8].y, playerStruct);
				}

				// If you are not in Time Trial or Relic Race
				if ((gameMode1 & (TIME_TRIAL | RELIC_RACE)) == 0)
				{
					UI_DrawNumWumpa(hudStructPtr[4].x, hudStructPtr[4].y, playerStruct);
				}
			}

			// If you're in a Relic Race
			if ((gameMode1 & RELIC_RACE) != 0)
			{
				UI_DrawNumTimebox(hudStructPtr[0x13].x, hudStructPtr[0x13].y, playerStruct);
			}

			// If game is not paused
			if ((gameMode1 & PAUSE_ALL) == 0)
			{
				if (playerStruct->PickupWumpaHUD.numCollected != 0)
				{
					wumpaModelPos.x = hudStructPtr[3].x;
					wumpaModelPos.y = hudStructPtr[3].y;

					// if cooldown between items is over
					if (playerStruct->PickupWumpaHUD.cooldown == 0)
					{
						// deduct from number of queued items to pick up
						playerStruct->PickupWumpaHUD.numCollected--;

						if ((LOAD_IsOpen_RacingOrBattle() != 0) &&

						    // If you're not in Adventure Arena
						    ((gameMode1 & ADVENTURE_ARENA) == 0))
						{
							RB_Player_ModifyWumpa(playerStruct, 1);
						}


						// OtherFX_Play to get wumpa fruit
						// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005347c-0x80053484 for wumpa pickup SFX.
						OtherFX_Play(0x42, 1);

						// initial timer value
						partTimeVariable1 = 5;

						// if timer is already running, set new timer value
						if (playerStruct->PickupWumpaHUD.numCollected != 0)
							goto LAB_80053498;
					}
					else
					{
						UI_Lerp2D_HUD(wumpaModelPos.v, (int)playerStruct->PickupWumpaHUD.startX, (int)playerStruct->PickupWumpaHUD.startY, hudStructPtr[3].x,
						              hudStructPtr[3].y, playerStruct->PickupWumpaHUD.cooldown, 5);

						// subtract one from timer
						partTimeVariable1 = playerStruct->PickupWumpaHUD.cooldown - 1;

					LAB_80053498:

						// set timer value
						playerStruct->PickupWumpaHUD.cooldown = partTimeVariable1;
					}

					struct Icon **iconPtrArray = ICONGROUP_GETICONS(gGT->iconGroup[0xB]);

					// "wumpaposter" icon group
					DecalHUD_DrawPolyFT4(iconPtrArray[0], (int)wumpaModelPos.x, (int)wumpaModelPos.y,

					                     // pointer to PrimMem struct
					                     &gGT->backBuffer->primMem,

					                     // pointer to OT memory
					                     gGT->pushBuffer_UI.ptrOT,

					                     0, hudStructPtr[0].scale);
				}

				if (playerStruct->PickupLetterHUD.cooldown != 0)
				{
					struct Instance *curr;
					letterCtrPos.x = hudStructPtr[0x12].x;
					letterCtrPos.y = hudStructPtr[0x12].y;

					// C-Letter
					if (playerStruct->PickupLetterHUD.modelID == STATIC_C)
					{
						curr = sdata->ptrHudC;
					}

					// T-letter
					else if (playerStruct->PickupLetterHUD.modelID == STATIC_T)
					{
						letterCtrPos.x += 0x1d;
						letterCtrPos.y -= 1;
						curr = sdata->ptrHudT;
					}

					// R-Letter
					else
					{
						letterCtrPos.x += 0x3a;
						curr = sdata->ptrHudR;
					}

					// make visible
					*(u32 *)&curr->flags &= 0xffffff7f;

					// reduce frame counter
					playerStruct->PickupLetterHUD.cooldown--;

					// PickupLetterHUD.startX and PickupLetterHUD.startY are start position of animation

					// Interpolate from start pos to end pos
					UI_Lerp2D_HUD(letterCtrPos.v, playerStruct->PickupLetterHUD.startX, playerStruct->PickupLetterHUD.startY, (int)letterCtrPos.x,
					              (int)letterCtrPos.y, (int)playerStruct->PickupLetterHUD.cooldown, 10);

					curr->matrix.t[0] = UI_ConvertX_2((int)letterCtrPos.x, 0x200);
					curr->matrix.t[1] = UI_ConvertY_2((int)letterCtrPos.y, 0x200);
					curr->matrix.t[2] = 0x200;
				}
			}

			// If you're not in a Relic Race
			if ((gameMode1 & RELIC_RACE) == 0)
			{
				if ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
				{
					// Draw weapon and number of wumpa fruit in HUD
					UI_Weapon_DrawSelf(hudStructPtr[0].x, hudStructPtr[0].y, hudStructPtr[0].scale, playerStruct);
				}
			}

			// if you are in relic mode
			else
			{
				// If you smashed a time crate, this variable is set to 10
				if (playerStruct->PickupTimeboxHUD.cooldown != 0)
				{
					// DAT_8008d530
					// -%ld

					// Make string with number of time crate
					// print "-x" where x is the amount of seconds
					sprintf(&string[0], &sdata->s_subtractLongInt[0], gGT->timeCrateTypeSmashed);

					// 4b4 and 4b6 are WindowStartPos(x,y) from PushBuffer, inside Driver
					UI_Lerp2D_HUD(wumpaModelPos.v, playerStruct->PickupTimeboxHUD.startX, playerStruct->PickupTimeboxHUD.startY, 0x14, 8,
					              playerStruct->PickupTimeboxHUD.cooldown, 10);

					// Decrease remaining number of frames for this to be on screen
					playerStruct->PickupTimeboxHUD.cooldown--;

					// Put string on the screen
					// This happens for 10 frames
					DecalFont_DrawLine(&string[0], (int)wumpaModelPos.x, (int)wumpaModelPos.y, FONT_BIG, PERIWINKLE);
				}
			}

			// === Naughty Dog Bug ===
			// This block will never execute in Life Limit,
			// even though original code has a block inside this IF
			// that checks for Life Limit

			// if you're in battle mode, while not paused
			// and you do not have a life limit
			if ((gameMode1 & (LIFE_LIMIT | BATTLE_MODE | PAUSE_ALL)) == BATTLE_MODE)
			{
				// If the animation for adding points is over
				if (playerStruct->BattleHUD.cooldown == 0)
				{
					// Delete the change that in score that was queued
					playerStruct->BattleHUD.scoreDelta = 0;
				}

				// if the animation is not done
				else
				{
					wumpaModelPos.x = hudStructPtr[0xD].x + 0x20;
					wumpaModelPos.y = hudStructPtr[0xD].y;

					partTimeVariable1 = playerStruct->BattleHUD.scoreDelta;

					if ((gameMode1 & LIFE_LIMIT) == 0)
					{
						if (partTimeVariable1 < 1)
						{
							fmt = &sdata->s_subtractInt[0];

							if (partTimeVariable1 < 0)
								partTimeVariable1 = -partTimeVariable1;
						}
						else
						{
							fmt = &sdata->s_additionLongInt[0];
						}
					}
					else
					{
						fmt = &sdata->s_subtractLongInt[0];
					}

					sprintf((char *)&string[0], fmt, partTimeVariable1);

					UI_Lerp2D_HUD(wumpaModelPos.v, (int)playerStruct->BattleHUD.startX, (int)playerStruct->BattleHUD.startY, (int)(hudStructPtr[0xD].x + 0x20),
					              (int)(hudStructPtr[0xD].y + 8), playerStruct->BattleHUD.cooldown, 5);

					// subtract one from the number of frames that the animation lasts
					playerStruct->BattleHUD.cooldown--;

					// print the string that shows the change in your score
					DecalFont_DrawLine((char *)&string[0], (int)wumpaModelPos.x, (int)wumpaModelPos.y, FONT_SMALL, RED);
				}
			}

			if ((gameMode1 & BATTLE_MODE) == 0)
			{
				if ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
				{
					UI_DrawLapCount(hudStructPtr[1].x, hudStructPtr[1].y, (u32)hudStructPtr[1].scale, playerStruct);
				}
			}
			else
			{
				// Draw how many points or lifes the player has
				UI_DrawBattleScores((int)hudStructPtr[0xD].x, (int)hudStructPtr[0xD].y, playerStruct);
			}

			if (((gameMode1 & (ARCADE_MODE | ADVENTURE_MODE)) != 0) && ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) != 0))
			{
				AA_EndEvent_DisplayTime((u32)playerStruct->driverID, 0);
			}

			partTimeVariable5 = gameMode1;

			// If you are in Relic Race, and not in battle mode, and not in time trial
			if ((partTimeVariable5 & 0x4020020) == 0)
			{
				if (((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0) || ((
				                                                                        // if numPlyrCurrGame is 2
				                                                                        numPlyr == '\x02' &&

				                                                                        // AND

				                                                                        // Not Arcade Mode (must be VS or Battle)
				                                                                        ((partTimeVariable5 & 0x400000) == 0))))
				{
					sVar17 = 0;
					partTimeVariable5 = 0;
				}
				else
				{
					// if numPlyrCurrGame is less than 3
					if (numPlyr < 3)
						goto LAB_80053af4;

					bVar3 = (gGT->timer & 1) == 0;
					sVar17 = (u16)bVar3 << 2;
					partTimeVariable5 = ((u32)bVar3 << 0x12) >> 0x10;
				}

				sVar1 = hudStructPtr[5].x;
				sVar2 = hudStructPtr[5].y;
				UI_DrawPosSuffix(sVar1, sVar2, playerStruct, (s16)partTimeVariable5);

				if (numPlyr > 2)
				{
					// Get Color Data
					ptrColor = data.ptrColor[sVar17];

					// icon pointer, specifically for the big rank icons that start at 0x19
					iconPtr = gGT->ptrIcons[(int)playerStruct->driverRank + 0x19];

				LAB_80053aec:

					DecalHUD_DrawPolyGT4(
					    // icon pointer
					    iconPtr,

					    // position
					    (int)hudStructPtr[2].x, (int)hudStructPtr[2].y,

					    &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT,

					    // color data
					    ptrColor[0], ptrColor[1], ptrColor[2], ptrColor[3],

					    0, FP(1.0));
				}
			}

			// If you're in end-of-race and Battle
			else if ((partTimeVariable5 & 0x200020) == 0x200020)
			{
				partTimeVariable5 = (u32)((gGT->timer & 1) == 0) << 2;

				// Draw the "st", "nd", "rd" suffix after "1st, 2nd, 3rd, etc"
				UI_DrawPosSuffix(hudStructPtr[5].x, hudStructPtr[5].y, playerStruct, (s16)partTimeVariable5);

				// Get Color Data
				ptrColor = data.ptrColor[partTimeVariable5];

				// pointer to icon
				// get rank icon of each battle team after battle is over
				// OH GOD THIS IS CONVOLUTED and probably wrong --Super
				iconPtr = gGT->ptrIcons[gGT->battleSetup.finishedRankOfEachTeam[playerStruct->BattleHUD.teamID] + 0x19];

				goto LAB_80053aec;
			}
		LAB_80053af4:

			// draw lock-on target for driver, if
			// a missile or warpball is chasing them
			UI_TrackerSelf(playerStruct);

			// If you're in Battle
			if ((gameMode1 & BATTLE_MODE) != 0)
			{
				// Draw arrows over the heads of other players (not AIs)
				UI_BattleDrawHeadArrows(playerStruct);
			}

			if ((playerStruct->numWumpas >= 10) && ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0))
			{
				// draw shining background behind wumpa fruit
				UI_Weapon_DrawBG(hudStructPtr[0xC].x, hudStructPtr[0xC].y, hudStructPtr[0xC].scale, playerStruct);

				// If your weapon is not "no weapon"
				if (playerStruct->heldItemID != 0xF)
				{
					// draw shining background behind weapon
					UI_Weapon_DrawBG(hudStructPtr[0xB].x, hudStructPtr[0xB].y, hudStructPtr[0xB].scale, playerStruct);
				}
			}

			// go to next player
			// thread = thread->sibling
			playerThread = playerThread->siblingThread;

			// TODO: use num where 0x14 = NUM_HUD
			hudStructPtr += 0x14;

		} while (playerThread != 0);
	}

	if (sdata->WrongWayDirection_bool != cVar22)
	{
		sdata->framesDrivingSameDirection = 0;
		sdata->WrongWayDirection_bool = cVar22;
	}

	sdata->framesDrivingSameDirection++;

	if (numPlyr == 1)
	{
		playerStruct = gGT->drivers[0];

		UI_DrawRaceClock(0x14, 8, 0, playerStruct);

		turboThread = 0;
		turboThreadObject = 0;

		if ((gGT->gameMode2 & CHEAT_TURBOCOUNT) != 0)
		{
			// Get number of boosts
			sVar1 = playerStruct->numTurbos;

			// If number of boosts is not zero
			if (sVar1 != 0)
			{
				// Read pointer from address
				turboThread = gGT->threadBuckets[TURBO].thread;

				while (
				    // Pointer != nullptr
				    turboThread != 0 &&

				    // Adds 0x30 to turboThread pointer, gets value (new address) then adds 4.
				    // If   [something]   != pointer that holds boost counter
				    (turboThreadObject = (struct Turbo *)turboThread->object, turboThreadObject->driver != playerStruct))
				{
					turboThread = turboThread->siblingThread;
				}

				// Get number of boosts
				sVar1 = playerStruct->numTurbos;
			}

			// sdata->TurboDisplayPos_Only1P
			// Position of counter
			// 0 = offscreen
			// 1 = just barely on screen
			// ...
			// 10 = clearly on screen

			if (
			    // If number boosts is less than 3
			    (sVar1 < 3) ||

			    // If display counter is fully on screen
			    (9 < sdata->TurboDisplayPos_Only1P))
			{
				// If pointer == nullptr
				if (turboThread == 0)
					goto LAB_80053c98;
			LAB_80053cac:

				// Set sVar1 to current display counter position
				sVar1 = sdata->TurboDisplayPos_Only1P;
				if (
				    // If number boosts is less than 3
				    (playerStruct->numTurbos < 3) &&

				    // If turbo counter is on screen
				    (0 < sdata->TurboDisplayPos_Only1P))
				{
				LAB_80053cd4:

					// Animate counter to move off screen
					sVar1 = sdata->TurboDisplayPos_Only1P - 1;
				}
			}

			// If you have more than 3 boosts, and
			// display counter is not fully on screen
			else
			{
				// Animate counter to move onto screen
				sVar1 = sdata->TurboDisplayPos_Only1P + 1;

				// If pointer == nullptr
				if (turboThread == 0)
				{
				LAB_80053c98:
					// If counter is off screen
					if (sdata->TurboDisplayPos_Only1P < 1)
					{
						// set svar1 to display position
						goto LAB_80053cac;
					}

					// If counter is on screen
					// decrease boost counter,
					goto LAB_80053cd4;
				}
			}

			// Set display position value
			sdata->TurboDisplayPos_Only1P = sVar1;

			// If display counter is on screen (fully or not fully)
			if (sdata->TurboDisplayPos_Only1P != 0)
			{
				// Interpolate the turbo counter slide in from the right
				UI_Lerp2D_Linear(turboCountPos.v, 0x2c8, 0x20, 500, 0x20, sdata->TurboDisplayPos_Only1P, 10);

				// The actual counter number will continue to
				// increase past 1000, but the on-screen text
				// will cap at 999

				// If you have less than 1000 boosts
				if (playerStruct->numTurbos < 1000)
				{
					// DAT_8008d54c
					// %d

					// build string for on-screen boost counter
					sprintf((char *)&string[0], &sdata->s_intSpace[0], playerStruct->numTurbos);
				}

				// If you have more than 1000 boosts
				else
				{
					// 8d544
					// "999" <-- ascii string, not 2-u8 value

					// Cap the on-screen counter at 999
					sprintf((char *)&string[0], &sdata->s_999[0]);
				}

				i = DecalFont_GetLineWidth(sdata->lngStrings[LNG_TURBOS], 1);

				// Draw the string
				DecalFont_DrawLine((char *)&string[0], (int)(((u32)turboCountPos.x - i) * 0x10000) >> 0x10, (int)turboCountPos.y, FONT_BIG,
				                   (JUSTIFY_RIGHT | ORANGE_RED));


				sprintf((char *)&string[0], &sdata->s_str[0], sdata->lngStrings[LNG_TURBOS]);

				// Draw the string
				DecalFont_DrawLine((char *)&string[0], (int)(s16)turboCountPos.x, (int)turboCountPos.y, FONT_BIG, (JUSTIFY_RIGHT | ORANGE));

				backBuffer = gGT->backBuffer;
				primMemCurr = backBuffer->primMem.cursor;
				TurboCounterBar = 0;

				if ((int)primMemCurr <= (int)backBuffer->primMem.guardEnd)
				{
					backBuffer->primMem.cursor = primMemCurr + 9;
					TurboCounterBar = (POLY_G4 *)primMemCurr;
				}

				if (TurboCounterBar == 0)
				{
					return;
				}

				*(u32 *)&TurboCounterBar->r0 = 0x3800c8ff;
				*(u32 *)&TurboCounterBar->r1 = 0x3800c8ff;
				*(u32 *)&TurboCounterBar->r2 = 0x380000ff;
				*(u32 *)&TurboCounterBar->r3 = 0x380000ff;
				TurboCounterBar->x0 = turboCountPos.x - 0xaa;
				TurboCounterBar->y0 = turboCountPos.y + 9;
				TurboCounterBar->x1 = turboCountPos.x + 0x32;
				TurboCounterBar->y1 = turboCountPos.y + 9;
				TurboCounterBar->x2 = turboCountPos.x - 0x96;
				TurboCounterBar->y2 = turboCountPos.y + 0x12;
				TurboCounterBar->x3 = turboCountPos.x + 0x32;
				TurboCounterBar->y3 = turboCountPos.y + 0x12;

				// pointer to OT memory
				primMemCurr = gGT->pushBuffer_UI.ptrOT;

				*(int *)TurboCounterBar = CtrGpu_PackOTTag(*primMemCurr, 0x8000000);
				*primMemCurr = (u32)TurboCounterBar & 0xffffff;
			}
		}
	}

	// if numPlyrCurrGame is not 1 (multiplayer)
	else
	{
		// if you have a time limit (battle)
		if ((gameMode1 & TIME_LIMIT) != 0)
		{
			// draw countdown clock
			UI_DrawLimitClock(0xd7, 0x68, 2);
		}
	}

	if (levPtrMap != 0)
	{
		if (((numPlyr == 1)

		     // if want to draw map, not speedometer
		     && (sdata->HudAndDebugFlags & 8) == 0) ||

		    (numPlyr == 3))
		{
			local_30[0] = 0;

			UI_Map_DrawDrivers((int)levPtrMap, gGT->threadBuckets[PLAYER].thread, local_30);
			UI_Map_DrawDrivers((int)levPtrMap, gGT->threadBuckets[ROBOT].thread, local_30);

			UI_Map_DrawGhosts((int)levPtrMap, gGT->threadBuckets[GHOST].thread);

			UI_Map_DrawTracking((int)levPtrMap, gGT->threadBuckets[TRACKING].thread);

			mapPosX = 500;
			mapPosY = 195;

			if (numPlyr == 3)
			{
				mapPosX -= 60;
				mapPosY += 10;
			}

			// Draw the map
			UI_Map_DrawMap(
			    // top half and bottom half
			    gGT->ptrIcons[3], gGT->ptrIcons[4],

			    // X and Y
			    mapPosX, mapPosY,

			    // Pointer to primary memory
			    &gGT->backBuffer->primMem,

			    // pointer to OT memory
			    gGT->pushBuffer_UI.ptrOT,

			    // color, in this case white
			    1);
		}
	}

	bVar3 = false;

	// loop counter
	i = 0;

	// if numPlyrCurrGame is not 0
	if (numPlyr != '\0')
	{
		// for(int i = 0; i < numPlyrCurrGame; i++)
		do
		{
			playerStruct = gGT->drivers[i];
			pb = &gGT->pushBuffer[playerStruct->driverID];

			if ((((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) != 0) && ((gameMode1 & (ARCADE_MODE | TIME_TRIAL)) == 0)) &&
			    ((
			        // cooldown is finished
			        gGT->timerEndOfRaceVS == 0 ||

			        // cooldown has not progressed far,
			        // so you still need to draw "Finished" and "Loser"
			        (0x96 < gGT->timerEndOfRaceVS))))
			{
				if (
				    // player structure + 0x482 is your rank in the race
				    // 0 = 1st place, 1 = 2nd place, 2 = 3rd place, etc

				    // Basically, out of all human players, if you did not come in last
				    ((int)playerStruct->driverRank < (int)numPlyr - 1) &&

				    // If you're not in Battle Mode
				    // (winner of battle wont use this function)
				    ((gameMode1 & BATTLE_MODE) == 0))
				{
					pbVar6 = sdata->lngStrings[LNG_FINISHED];
				}

				// If you came in last place, or you're in battle
				else
				{
					pbVar6 = sdata->lngStrings[LNG_LOSER];
				}

				DecalFont_DrawLine(pbVar6,
				                   pb->rect.x + (pb->rect.w >> 1),        // midpointX
				                   pb->rect.y + (pb->rect.h >> 1) - 0x1e, // midpointY
				                   FONT_BIG, (JUSTIFY_CENTER | ORANGE));

				if (
				    // If you press Cross or Start
				    ((sdata->gGamepads->gamepad[i].buttonsTapped & 0x1010) != 0) &&

				    // If you're in End-Of-Race menu
				    ((gameMode1 & END_OF_RACE) != 0))
				{
					// make "Finished" and "Loser" disappear, start
					// drawing the on-screen comments
					gGT->timerEndOfRaceVS = 0x96;
				}
			}

			// if item roll is not done
			if (playerStruct->itemRollTimer != 0)
			{
				bVar3 = true;
			}

			// increment the iteration counter
			i++;

			// for(int i = 0; i < numPlyrCurrGame; i++)
		} while (i < (int)numPlyr);
	}
	if ((
	        // If game is not paused
	        ((gameMode1 & PAUSE_ALL) == 0) &&

	        // item roll is done
	        (!bVar3)) &&

	    // If you're drawing Weapon Roulette (randomizing)
	    ((gameMode1 & ROLLING_ITEM) != 0))
	{
		// stop weapon shuffle sound
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005423c-0x8005424c for roulette SFX stop.
		OtherFX_Stop2(0x5d);

		// disable the randomizing effect in the HUD
		gGT->gameMode1 &= ~ROLLING_ITEM;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80054298-0x8005435c.
void UI_RenderFrame_AdvHub(void)
{
	struct UiElement2D *hudStructPtr;
	struct GameTracker *gGT;

	gGT = sdata->gGT;
	hudStructPtr = data.hudStructPtr[gGT->numPlyrCurrGame - 1];

	UI_DrawNumRelic(hudStructPtr[0xE].x + 0x10, hudStructPtr[0xE].y - 10);
	UI_DrawNumKey(hudStructPtr[0xF].x + 0x10, hudStructPtr[0xF].y - 10);
	UI_DrawNumTrophy(hudStructPtr[0x10].x + 0x10, hudStructPtr[0x10].y - 10);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005435c-0x8005465c for the retail path.
void UI_RenderFrame_CrystChall(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *player;
	struct UiElement2D *hudStructPtr;
	struct Instance *hudCrystal;
	int iVar5;
	SVec2 crystalPos;

	player = gGT->drivers[0];
	hudStructPtr = data.hudStructPtr[0];
	hudCrystal = sdata->ptrHudCrystal;

	// If game is not paused
	if ((gGT->gameMode1 & PAUSE_ALL) == 0)
	{
		// execute Jump meter and landing boost processes
		UI_JumpMeter_Update(player);
	}

	UI_DrawSpeedNeedle(hudStructPtr[9].x, hudStructPtr[9].y, player);

	UI_JumpMeter_Draw(hudStructPtr[6].x, hudStructPtr[6].y, player);

	UI_DrawSlideMeter(hudStructPtr[8].x, hudStructPtr[8].y, player);

	UI_DrawSpeedBG();

	UI_DrawNumCrystal(hudStructPtr[0x11].x + 0x10, hudStructPtr[0x11].y - 0x10, player);

	// Draw weapon and number of wumpa fruit in HUD
	UI_Weapon_DrawSelf(hudStructPtr[0].x, hudStructPtr[0].y, hudStructPtr[0].scale, player);

	DecalFont_DrawLine(sdata->lngStrings[LNG_TIME], 0x14, 8, FONT_SMALL, ORANGE);

	// "TIME" and the actual time are printed at the same
	// X-coordinate, so we know 0x14 is the X, which only
	// leaves the next parameter as the only possible value for
	// the Y-coordinate.

	// draw countdown clock
	UI_DrawLimitClock(0x14, 0x10, 1);


	// If game is paused
	if ((gGT->gameMode1 & PAUSE_ALL) != 0)
	{
		return;
	}

	if ((player->PickupWumpaHUD.numCollected) == 0)
	{
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Menu-storage can enter crystal HUD flow without
		// a published crystal HUD model.
		if (hudCrystal == NULL)
			goto LAB_800545e8;
#endif

		// make invisible
		hudCrystal->flags |= 0x80;
		goto LAB_800545e8;
	}
	crystalPos.x = hudStructPtr[0x11].x;
	crystalPos.y = hudStructPtr[0x11].y;

	// make visible
#if defined(CTR_NATIVE)
	if (hudCrystal != NULL)
#endif
		hudCrystal->flags &= 0xffffff7f;

	// if cooldown between grabbing items is over,
	// which also means item has moved to the hud icon
	if ((player->PickupWumpaHUD.cooldown) == 0)
	{
		// add one to your crystal count
		player->numCrystals++;

		// deduct from number of queued items to pick up
		player->PickupWumpaHUD.numCollected--;

		// if you have enough crystals to win the race
		if (gGT->numCrystalsInLEV <= player->numCrystals)
		{
			player->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_FreezeEndEvent_Init;

			player->actionsFlagSet |= ACTION_RACE_FINISHED;

			MainGameEnd_Initialize();
		}

		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80054550-0x80054558 for crystal pickup SFX.
		OtherFX_Play(0x42, 1);

		if (player->PickupWumpaHUD.numCollected != 0)
			player->PickupWumpaHUD.cooldown = 5;
	}

	// if cooldown is not done
	else
	{
		// interpolate position over course of 5 frames
		UI_Lerp2D_HUD(crystalPos.v, (int)player->PickupWumpaHUD.startX, (int)player->PickupWumpaHUD.startY, (int)hudStructPtr[0x11].x,
		              (int)hudStructPtr[0x11].y, player->PickupWumpaHUD.cooldown, 5);

		// reduce cooldown between getting each wumpa (or crystal)
		player->PickupWumpaHUD.cooldown--;
	}

	// ======= This is UI_ConvertX_2 and Y_2, but inlined =======

	// posX
	iVar5 = (crystalPos.x + -0x100) * hudStructPtr[0x11].z;
	if (iVar5 < 0)
	{
		iVar5 = iVar5 + 0xff;
	}
#if defined(CTR_NATIVE)
	if (hudCrystal != NULL)
#endif
	{
		hudCrystal->matrix.t[0] = iVar5 >> 8;

		// posY
		iVar5 = (crystalPos.y + -0x6c) * hudStructPtr[0x11].z;
		if (iVar5 < 0)
		{
			iVar5 = iVar5 + 0xff;
		}
		hudCrystal->matrix.t[1] = iVar5 >> 8;

		// posZ
		hudCrystal->matrix.t[2] = hudStructPtr[0x11].z;
	}

LAB_800545e8:

	// quit if game is paused, or item is
	// rolling, or not drawing roulette
	if ((gGT->gameMode1 & PAUSE_ALL) != 0)
		return;
	if (player->itemRollTimer != 0)
		return;
	if ((gGT->gameMode1 & ROLLING_ITEM) == 0)
		return;

	// if not paused, item stopped rolling, and
	// weapon roulette sound is playing, then
	// stop the sound and remove flag
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80054618-0x80054628 for roulette SFX stop.
	OtherFX_Stop2(0x5d);
	gGT->gameMode1 &= ~(ROLLING_ITEM);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005465c-0x80054a08 for the retail path.
void UI_RenderFrame_Wumpa3D_2P3P4P(struct GameTracker *gGT)
{
	RECT viewport2P;
	RECT viewport3P4P;
	RECT *viewport;
	struct PushBuffer *wumpaPushBuffer;
	u32 packedRect;

	// NOTE(aalhendi): Retail copies packed RECT halfwords; native unpacks them
	// explicitly to avoid strict-aliasing UB from writing RECT fields as u32.
	packedRect = sdata->multiplayerWumpaHudData[0];
	viewport2P.x = (s16)packedRect;
	viewport2P.y = (s16)(packedRect >> 16);
	packedRect = sdata->multiplayerWumpaHudData[1];
	viewport2P.w = (s16)packedRect;
	viewport2P.h = (s16)(packedRect >> 16);
	packedRect = sdata->multiplayerWumpaHudData[2];
	viewport3P4P.x = (s16)packedRect;
	viewport3P4P.y = (s16)(packedRect >> 16);
	packedRect = sdata->multiplayerWumpaHudData[3];
	viewport3P4P.w = (s16)packedRect;
	viewport3P4P.h = (s16)(packedRect >> 16);

	viewport = &viewport2P;
	if (gGT->numPlyrCurrGame >= 3)
		viewport = &viewport3P4P;

	// NOTE(aalhendi): Retail reads the gp slot populated by UI_INSTANCE_InitAll
	// with ptrPushBufferUI, not the adjacent ptrFruitDisp instance slot.
	wumpaPushBuffer = (struct PushBuffer *)(uintptr_t)sdata->ptrPushBufferUI;

	if (wumpaPushBuffer != NULL)
	{
		u_long *textureStart = wumpaPushBuffer->ptrOT;
		u_long *textureEnd = wumpaPushBuffer->renderBucketOTRangeEnd;
		b32 shouldCycleTexture = (textureStart != NULL) && (textureEnd != NULL);

#ifdef CTR_NATIVE
		shouldCycleTexture = shouldCycleTexture && CtrGpu_IsCurrentOTRange(gGT->backBuffer, textureStart, textureEnd);
#endif

		PushBuffer_SetDrawEnv_DecalMP(textureEnd, gGT->backBuffer, viewport, viewport->x + (viewport->w >> 1) - 0x100, viewport->y + (viewport->h >> 1) - 0x6c,
		                              0, 0, 0, 0, 1);

		if (shouldCycleTexture)
		{
			CTR_CycleTex_2p3p4pWumpaHUD((u32 *)&gGT->pushBuffer[0].ptrOT[0x3ff], textureStart, (int)(textureEnd - textureStart) + 1);
		}
	}

	if (gGT->numPlyrCurrGame < 2)
		return;

	struct UiElement2D *hud = data.hudStructPtr[gGT->numPlyrCurrGame - 1];

	for (int playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++, hud += 0x14)
	{
		struct Driver *driver = gGT->drivers[playerIndex];

		if ((driver->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
			continue;

		if ((gGT->gameMode1 & END_OF_RACE) != 0)
			continue;

		s16 posX = hud[3].x + wumpaPushBuffer->rect.x - (viewport->w >> 1);
		s16 posY = hud[3].y + wumpaPushBuffer->rect.y - (viewport->h >> 1);

		POLY_FT4 *prim = (POLY_FT4 *)gGT->backBuffer->primMem.cursor;
		u8 u0 = (u8)(viewport->x & 0x3f);
		u8 v0 = (u8)viewport->y;
		u8 u1 = u0 + (u8)viewport->w;
		u8 v1 = v0 + (u8)viewport->h;

		prim->tag = 0x09000000;
		CtrGpu_WriteColorCode(&prim->r0, 0x2c808080);

		CtrGpu_WritePackedXY(&prim->x0, (u16)posX | ((u32)(u16)posY << 16));
		CtrGpu_WritePackedXY(&prim->x1, (u16)(posX + viewport->w) | ((u32)(u16)posY << 16));
		CtrGpu_WritePackedXY(&prim->x2, (u16)posX | ((u32)(u16)(posY + viewport->h) << 16));
		CtrGpu_WritePackedXY(&prim->x3, (u16)(posX + viewport->w) | ((u32)(u16)(posY + viewport->h) << 16));

		CtrGpu_WritePackedUV(&prim->u0, (u16)u0 | ((u16)v0 << 8));
		CtrGpu_WritePackedUV(&prim->u1, (u16)u1 | ((u16)v0 << 8));
		CtrGpu_WritePackedUV(&prim->u2, (u16)u0 | ((u16)v1 << 8));
		CtrGpu_WritePackedUV(&prim->u3, (u16)u1 | ((u16)v1 << 8));

		u16 tpage = (u16)(((viewport->y & 0x100) >> 4) | ((viewport->x & 0x3ff) >> 6) | 0x100 | ((viewport->y & 0x200) << 2));
		prim->tpage = tpage;

		if (driver->numWumpas >= 10)
		{
			u8 shineColor = sdata->wumpaShineColor1[0][0];
			prim->r0 = shineColor;
			prim->g0 = shineColor;
			prim->b0 = shineColor;
		}

		gGT->backBuffer->primMem.cursor = prim + 1;
		AddPrim(gGT->pushBuffer_UI.ptrOT, prim);
	}
}
