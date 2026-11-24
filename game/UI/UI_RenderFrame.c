#include <common.h>

// To do: add a header

void UI_RenderFrame_Racing()
{
	s8 bVar3;
	int partTimeVariable1;
	char *pbVar6;
	int i;
	struct PushBuffer *pb;
	u32 partTimeVariable5;
	u32 *primMemCurr;
	POLY_G4 *TurboCounterBar;
	s16 sVar17;
	struct Driver *playerStruct;
	struct UiElement2D *hudStructPtr;
	struct UiElement2D *hud;
	struct UIMap *levPtrMap;
	int cVar22;
	struct Thread *playerThread;
	struct DB *backBuffer;
	// NOTE(aalhendi): turboThread crosses zero calls in flow.c, so GCC 2.8.1's
	// global.c:find_reg prefers caller-saved a0 over retail's s0 without a hint.
	struct Thread *turboThread CTR_PSX_GLOBAL_REGISTER_HINT("$16");
	struct Turbo *turboThreadObject;
	int offset;
	int unusedZero;

	// Get pointer to array of HUD structs
	hudStructPtr = GAME_HUD_LAYOUTS[GAME_TRACKER->numPlyrCurrGame - 1];

	levPtrMap = 0;

	UI_WeaponBG_AnimateShine();

	// NOTE(aalhendi): Retail emits a dead `move s1, s5` in the bnez delay slot
	// at 0x80053004 (s1 is unread on every path). Keep it live across
	// UI_DrawRankedDrivers() with REG_N_REFS == 3 so flow.c and
	// local-alloc.c:update_equiv_regs do not delete or sink it.
	unusedZero = 0;

	// if time on clock is zero
	if (GAME_TRACKER->elapsedEventTime == 0)
	{
		int j;
		for (j = 0; j < 8; j++)
		{
			GAME_DATA_NAME(rankIconsTransitionTimer)[j] = 0;

			pbVar6 = &GAME_SDATA_NAME(kartSpawnOrderArray)[j];

			GAME_DATA_NAME(rankIconsDesired)[j] = (u16)*pbVar6;
			GAME_DATA_NAME(rankIconsCurr)[j] = (u16)*pbVar6;
		}
	}

	// If not drawing intro-race cutscene
	if ((GAME_TRACKER->gameMode1 & START_OF_RACE) == 0)
	{
		if ((GAME_TRACKER->hudFlags & HUD_FLAG_RENDER_LESS) != 0)
		{
			GAME_TRACKER->hudFlags &= HUD_FLAG_CLEAR_RENDER_LESS_MASK;
		}
		else
		{
			// If you press Triangle
			if ((GAMEPADS->gamepad[0].buttonsTapped & 0x40000) != 0)
			{
				// if & 8, remove bit 8,
				// if !& 8, add bit 8,
				// toggle map and speedometer
				GAME_SDATA_NAME(HudAndDebugFlags) ^= 8;
			}
		}
	}

	// numPlyrCurrGame is 0
	if ((GAME_TRACKER->numPlyrCurrGame == '\0') &&

	    ((GAME_TRACKER->drivers[0]->actionsFlagSet & ACTION_BOT) != 0))
	{
		// force draw speedometer, and not map, why?
		GAME_SDATA_NAME(HudAndDebugFlags) = 8;
	}

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native can load levels before ST1 map metadata is present.
	if ((GAME_TRACKER->level1->ptrSpawnType1 != 0) && (GAME_TRACKER->level1->ptrSpawnType1->count != 0))
#else
	if (GAME_TRACKER->level1->ptrSpawnType1->count != 0)
#endif
	{
		void **pointers = ST1_GETPOINTERS(GAME_TRACKER->level1->ptrSpawnType1);
		levPtrMap = pointers[ST1_MAP];
	}

	// If you are not in Relic Race, and not in battle mode,
	// and not in time trial
	if ((GAME_TRACKER->gameMode1 & (RELIC_RACE | TIME_TRIAL | BATTLE_MODE)) == 0)
	{
		UI_DrawRankedDrivers();
	}

	// pointer to first Player thread
	playerThread = GAME_TRACKER->threadBuckets[PLAYER].thread;
	CTR_PSX_OBSERVE_VALUE(unusedZero);
	CTR_PSX_OBSERVE_VALUE(unusedZero);

	cVar22 = 0;
	if (playerThread != 0)
	{
		SVec2 wumpaModelPos;
		offset = 0;

		// Loop through all player threads
		do
		{
			hud = hudStructPtr + offset;
			// pointer to player structure
			playerStruct = (struct Driver *)playerThread->object;

			// if racer has travelled wrong way for too long
			if (((int)playerStruct->distanceDrivenBackwards >= 0x1f5) && ((playerStruct->actionsFlagSet & ACTION_DRIVING_WRONG_WAY) != 0))
			{
				if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
				{
					pb = &GAME_TRACKER->pushBuffer[playerStruct->driverID];

					// if "Time on clock" last 0xXX u8 is greater than 0x80 and less than 0xFF
					if ((GAME_TRACKER->elapsedEventTime & 0x80) != 0)
					{
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_WRONG_WAY],

						                   // midpointX
						                   pb->rect.x + (pb->rect.w >> 1),

						                   // midpointY, 0x1e higher
						                   pb->rect.y + (pb->rect.h >> 1) - 0x1e,

						                   FONT_BIG, (JUSTIFY_CENTER | ORANGE));
					}

					// The text will not show if the last u8 is more than 0x00 and less than 0x7F.
					// This is what makes the text flicker, rather than drawing solid

					cVar22 = 1;
				}
			}

			if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
			{
				// execute Jump meter and landing boost processes
				UI_JumpMeter_Update(playerStruct);
			}

			if (
			    // numPlyrCurrGame is less than 2 (1P mode)
			    (GAME_TRACKER->numPlyrCurrGame < 2) &&
			    // if want to draw speedometer
			    ((GAME_SDATA_NAME(HudAndDebugFlags) & 8) != 0))
			{
				UI_DrawSpeedNeedle(hud[UI_HUD_SLOT_SPEEDOMETER].x, hud[UI_HUD_SLOT_SPEEDOMETER].y, playerStruct);
				UI_JumpMeter_Draw(hud[UI_HUD_SLOT_JUMP_METER].x, hud[UI_HUD_SLOT_JUMP_METER].y, playerStruct);
				UI_DrawSlideMeter(hud[UI_HUD_SLOT_SLIDE_METER].x, hud[UI_HUD_SLOT_SLIDE_METER].y, playerStruct);
				UI_DrawSpeedBG(hud[UI_HUD_SLOT_SPEEDOMETER].x, hud[UI_HUD_SLOT_SPEEDOMETER].y, playerStruct);
			}

			if ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
			{
				// If you're not in Battle Mode
				if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) == 0)
				{
					// Draw powerslide meter
					UI_DrawSlideMeter(hud[UI_HUD_SLOT_SLIDE_METER].x, hud[UI_HUD_SLOT_SLIDE_METER].y, playerStruct);
				}

				// If you are not in Time Trial or Relic Race
				if ((GAME_TRACKER->gameMode1 & (TIME_TRIAL | RELIC_RACE)) == 0)
				{
					UI_DrawNumWumpa(hud[UI_HUD_SLOT_WUMPA_COUNT].x, hud[UI_HUD_SLOT_WUMPA_COUNT].y, playerStruct);
				}
			}

			// If you're in a Relic Race
			if ((GAME_TRACKER->gameMode1 & RELIC_RACE) != 0)
			{
				UI_DrawNumTimebox(hud[UI_HUD_SLOT_TIMEBOX].x, hud[UI_HUD_SLOT_TIMEBOX].y, playerStruct);
			}

			// If game is not paused
			if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
			{
				if (playerStruct->PickupWumpaHUD.numCollected != 0)
				{
					struct Icon **iconPtrArray;

					wumpaModelPos.x = hud[UI_HUD_SLOT_FRUIT_MODEL].x;
					wumpaModelPos.y = hud[UI_HUD_SLOT_FRUIT_MODEL].y;

					// if cooldown between items is not over
					if (playerStruct->PickupWumpaHUD.cooldown != 0)
					{
						UI_Lerp2D_HUD(CTR_VECTOR_DATA(&(wumpaModelPos)), (int)playerStruct->PickupWumpaHUD.startX, (int)playerStruct->PickupWumpaHUD.startY,
						              hud[UI_HUD_SLOT_FRUIT_MODEL].x, hud[UI_HUD_SLOT_FRUIT_MODEL].y, playerStruct->PickupWumpaHUD.cooldown, 5);

						// subtract one from timer
						playerStruct->PickupWumpaHUD.cooldown--;
					}
					else
					{
						// deduct from number of queued items to pick up
						playerStruct->PickupWumpaHUD.numCollected--;

						if (LOAD_IsOpen_RacingOrBattle() &&

						    // If you're not in Adventure Arena
						    ((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) == 0))
						{
							RB_Player_ModifyWumpa(playerStruct, 1);
						}

						// OtherFX_Play to get wumpa fruit
						OtherFX_Play(0x42, 1);

						// if timer is already running, set new timer value
						if (playerStruct->PickupWumpaHUD.numCollected != 0)
						{
							playerStruct->PickupWumpaHUD.cooldown = 5;
						}
					}

					iconPtrArray = ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[0xB]);

					// "wumpaposter" icon group
					DecalHUD_DrawPolyFT4(iconPtrArray[0], (int)wumpaModelPos.x, (int)wumpaModelPos.y,

					                     // pointer to PrimMem struct
					                     &GAME_TRACKER->backBuffer->primMem,

					                     // pointer to OT memory
					                     GAME_TRACKER->pushBuffer_UI.ptrOT,

					                     0, hud[UI_HUD_SLOT_WEAPON].scale);
				}

				if (playerStruct->PickupLetterHUD.cooldown != 0)
				{
					SVec2 letterCtrPos;
					struct Instance *curr;

					// C-Letter
					if (playerStruct->PickupLetterHUD.modelID == STATIC_C)
					{
						struct Instance *inst = sdata->ptrHudC;

						// make visible
						inst->flags &= ~HIDE_MODEL;

						letterCtrPos.x = hud[UI_HUD_SLOT_TOKEN_OR_CTR].x;
						letterCtrPos.y = hud[UI_HUD_SLOT_TOKEN_OR_CTR].y;
						curr = inst;
					}

					// T-letter
					else if (playerStruct->PickupLetterHUD.modelID == STATIC_T)
					{
						struct Instance *inst = sdata->ptrHudT;

						// make visible
						inst->flags &= ~HIDE_MODEL;

						letterCtrPos.x = hud[UI_HUD_SLOT_TOKEN_OR_CTR].x + 0x1d;
						letterCtrPos.y = hud[UI_HUD_SLOT_TOKEN_OR_CTR].y - 1;
						curr = inst;
					}

					// R-Letter
					else
					{
						struct Instance *inst = sdata->ptrHudR;

						// make visible
						inst->flags &= ~HIDE_MODEL;

						letterCtrPos.x = hud[UI_HUD_SLOT_TOKEN_OR_CTR].x + 0x3a;
						letterCtrPos.y = hud[UI_HUD_SLOT_TOKEN_OR_CTR].y;
						curr = inst;
					}

					// reduce frame counter
					playerStruct->PickupLetterHUD.cooldown--;

					// PickupLetterHUD.startX and PickupLetterHUD.startY are start position of animation

					// Interpolate from start pos to end pos
					UI_Lerp2D_HUD(CTR_VECTOR_DATA(&(letterCtrPos)), playerStruct->PickupLetterHUD.startX, playerStruct->PickupLetterHUD.startY,
					              (int)letterCtrPos.x, (int)letterCtrPos.y, (int)playerStruct->PickupLetterHUD.cooldown, 10);

					curr->matrix.t[0] = UI_ConvertX_2((int)letterCtrPos.x, 0x200);
					curr->matrix.t[1] = UI_ConvertY_2((int)letterCtrPos.y, 0x200);
					curr->matrix.t[2] = 0x200;
				}
			}

			// if you are in relic mode
			if ((GAME_TRACKER->gameMode1 & RELIC_RACE) != 0)
			{
				// If you smashed a time crate, this variable is set to 10
				if (playerStruct->PickupTimeboxHUD.cooldown != 0)
				{
					char string[24];

					// DAT_8008d530
					// -%ld

					// Make string with number of time crate
					// print "-x" where x is the amount of seconds
					sprintf(&string[0], &GAME_SDATA_NAME(s_subtractLongInt)[0], GAME_TRACKER->timeCrateTypeSmashed);

					// 4b4 and 4b6 are WindowStartPos(x,y) from PushBuffer, inside Driver
					UI_Lerp2D_HUD(CTR_VECTOR_DATA(&(wumpaModelPos)), playerStruct->PickupTimeboxHUD.startX, playerStruct->PickupTimeboxHUD.startY, 0x14, 8,
					              playerStruct->PickupTimeboxHUD.cooldown, 10);

					// Decrease remaining number of frames for this to be on screen
					playerStruct->PickupTimeboxHUD.cooldown--;

					// Put string on the screen
					// This happens for 10 frames
					DecalFont_DrawLine(&string[0], (int)wumpaModelPos.x, (int)wumpaModelPos.y, FONT_BIG, PERIWINKLE);
				}
			}
			else
			{
				if ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
				{
					// Draw weapon and number of wumpa fruit in HUD
					UI_Weapon_DrawSelf(hud[UI_HUD_SLOT_WEAPON].x, hud[UI_HUD_SLOT_WEAPON].y, hud[UI_HUD_SLOT_WEAPON].scale, playerStruct);
				}
			}

			// === Naughty Dog Bug ===
			// This block will never execute in Life Limit,
			// even though original code has a block inside this IF
			// that checks for Life Limit

			// if you're in battle mode, while not paused
			// and you do not have a life limit
			if ((GAME_TRACKER->gameMode1 & (LIFE_LIMIT | BATTLE_MODE | PAUSE_ALL)) == BATTLE_MODE)
			{
				// if the animation is not done
				if (playerStruct->BattleHUD.cooldown != 0)
				{
					char string[8];
					int dx = 0x20;

					wumpaModelPos.x = hud[UI_HUD_SLOT_BATTLE_SCORE].x + dx;
					wumpaModelPos.y = hud[UI_HUD_SLOT_BATTLE_SCORE].y;

					if ((GAME_TRACKER->gameMode1 & LIFE_LIMIT) != 0)
					{
						sprintf((char *)&string[0], &GAME_SDATA_NAME(s_subtractLongInt)[0], playerStruct->BattleHUD.scoreDelta);
					}
					else
					{
						partTimeVariable1 = playerStruct->BattleHUD.scoreDelta;

						if (partTimeVariable1 > 0)
						{
							sprintf((char *)&string[0], &GAME_SDATA_NAME(s_additionLongInt)[0], partTimeVariable1);
						}
						else
						{
							sprintf((char *)&string[0], &GAME_SDATA_NAME(s_subtractInt)[0], abs(partTimeVariable1));
						}
					}

					UI_Lerp2D_HUD(CTR_VECTOR_DATA(&(wumpaModelPos)), (int)playerStruct->BattleHUD.startX, (int)playerStruct->BattleHUD.startY,
					              (int)(hud[UI_HUD_SLOT_BATTLE_SCORE].x + dx), (int)(hud[UI_HUD_SLOT_BATTLE_SCORE].y + 8), playerStruct->BattleHUD.cooldown, 5);

					// subtract one from the number of frames that the animation lasts
					playerStruct->BattleHUD.cooldown--;

					// print the string that shows the change in your score
					DecalFont_DrawLine((char *)&string[0], (int)wumpaModelPos.x, (int)wumpaModelPos.y, FONT_SMALL, RED);
				}
				else
				{
					// Delete the change that in score that was queued
					playerStruct->BattleHUD.scoreDelta = 0;
				}
			}

			if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) == 0)
			{
				if ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
				{
					UI_DrawLapCount(hud[UI_HUD_SLOT_LAP_COUNT].x, hud[UI_HUD_SLOT_LAP_COUNT].y, (u32)hud[UI_HUD_SLOT_LAP_COUNT].scale, playerStruct);
				}
			}
			else
			{
				// Draw how many points or lifes the player has
				UI_DrawBattleScores((int)hud[UI_HUD_SLOT_BATTLE_SCORE].x, (int)hud[UI_HUD_SLOT_BATTLE_SCORE].y, playerStruct);
			}

			if (((GAME_TRACKER->gameMode1 & (ARCADE_MODE | ADVENTURE_MODE)) != 0) && ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) != 0))
			{
				AA_EndEvent_DisplayTime((u32)playerStruct->driverID, 0);
			}

			partTimeVariable5 = GAME_TRACKER->gameMode1;

			// If you are not in Relic Race, and not in battle mode,
			// and not in time trial
			if ((partTimeVariable5 & (RELIC_RACE | TIME_TRIAL | BATTLE_MODE)) == 0)
			{
				if (((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0) || ((
				                                                                        // if numPlyrCurrGame is 2
				                                                                        GAME_TRACKER->numPlyrCurrGame == '\x02' &&

				                                                                        // AND

				                                                                        // Not Arcade Mode (must be VS or Battle)
				                                                                        ((partTimeVariable5 & ARCADE_MODE) == 0))))
				{
					sVar17 = 0;
					UI_DrawPosSuffix(hud[UI_HUD_SLOT_RANK].x, hud[UI_HUD_SLOT_RANK].y, playerStruct, 0);
				}
				else
				{
					// if numPlyrCurrGame is less than 3
					if (GAME_TRACKER->numPlyrCurrGame < 3)
					{
						goto LAB_80053af4;
					}

					sVar17 = ((GAME_TRACKER->timer & 1) == 0) << 2;
					UI_DrawPosSuffix(hud[UI_HUD_SLOT_RANK].x, hud[UI_HUD_SLOT_RANK].y, playerStruct, (u16)sVar17);
				}

				if (GAME_TRACKER->numPlyrCurrGame > 2)
				{
					DecalHUD_DrawPolyGT4(
					    // icon pointer, specifically for the big rank icons that start at 0x19
					    GAME_TRACKER->ptrIcons[(int)playerStruct->driverRank + 0x19],

					    // position
					    (int)hud[UI_HUD_SLOT_BIG1].x, (int)hud[UI_HUD_SLOT_BIG1].y,

					    &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT,

					    // color data
					    ColorCode_Load(&GAME_DATA_NAME(ptrColor)[sVar17][0]), ColorCode_Load(&GAME_DATA_NAME(ptrColor)[sVar17][1]),
					    ColorCode_Load(&GAME_DATA_NAME(ptrColor)[sVar17][2]), ColorCode_Load(&GAME_DATA_NAME(ptrColor)[sVar17][3]),

					    0, FP(1.0));
				}
			}

			// If you're in end-of-race and Battle
			else if ((partTimeVariable5 & 0x200020) == 0x200020)
			{
				s16 colorIdx = ((GAME_TRACKER->timer & 1) == 0) << 2;
				u32 **colorRow;

				// Draw the "st", "nd", "rd" suffix after "1st, 2nd, 3rd, etc"
				UI_DrawPosSuffix(hud[UI_HUD_SLOT_RANK].x, hud[UI_HUD_SLOT_RANK].y, playerStruct, colorIdx);

				colorRow = &GAME_DATA_NAME(ptrColor)[colorIdx];

				DecalHUD_DrawPolyGT4(
				    // icon pointer
				    GAME_TRACKER->ptrIcons[GAME_TRACKER->battleSetup.finishedRankOfEachTeam[playerStruct->BattleHUD.teamID] + 0x19],

				    // position
				    (int)hud[UI_HUD_SLOT_BIG1].x, (int)hud[UI_HUD_SLOT_BIG1].y,

				    &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT,

				    // color data
				    ColorCode_Load(&(*colorRow)[0]), ColorCode_Load(&(*colorRow)[1]), ColorCode_Load(&(*colorRow)[2]), ColorCode_Load(&(*colorRow)[3]),

				    0, FP(1.0));
			}
		LAB_80053af4:

			// draw lock-on target for driver, if
			// a missile or warpball is chasing them
			UI_TrackerSelf(playerStruct);

			// If you're in Battle
			if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0)
			{
				// Draw arrows over the heads of other players (not AIs)
				UI_BattleDrawHeadArrows(playerStruct);
			}

			if ((playerStruct->numWumpas >= DRIVER_WUMPA_JUICED_COUNT) && ((playerStruct->actionsFlagSet & ACTION_RACE_FINISHED) == 0))
			{
				// draw shining background behind wumpa fruit
				UI_Weapon_DrawBG(hud[UI_HUD_SLOT_RACING_WEAPON_BG].x, hud[UI_HUD_SLOT_RACING_WEAPON_BG].y, hud[UI_HUD_SLOT_RACING_WEAPON_BG].scale,
				                 playerStruct);

				// If your weapon is not "no weapon"
				if (playerStruct->heldItemID != HELD_ITEM_NONE)
				{
					// draw shining background behind weapon
					UI_Weapon_DrawBG(hud[UI_HUD_SLOT_BATTLE_WEAPON_BG].x, hud[UI_HUD_SLOT_BATTLE_WEAPON_BG].y, hud[UI_HUD_SLOT_BATTLE_WEAPON_BG].scale,
					                 playerStruct);
				}
			}

			// go to next player
			// thread = thread->sibling
			playerThread = playerThread->siblingThread;

			offset += UI_HUD_SLOT_COUNT;

		} while (playerThread != 0);
	}

	if (sdata->WrongWayDirection_bool != cVar22)
	{
		sdata->framesDrivingSameDirection = 0;
		sdata->WrongWayDirection_bool = cVar22;
	}

	sdata->framesDrivingSameDirection++;

	{
		// NOTE(aalhendi): In BB 94, sched1 queues lw s1,9452(gGT) behind a3=s1
		// and schedules a2=0 after it, so gGT does not conflict with a2; hint
		// retail's a3 ($7) before sched2 reorders lw back ahead of a2=0.
		struct GameTracker *gGT CTR_PSX_GLOBAL_REGISTER_HINT("$7") = GAME_TRACKER;

		if (gGT->numPlyrCurrGame == 1)
		{
			UI_DrawRaceClock(0x14, 8, UI_RACE_CLOCK_SHOW_CURRENT_TIME, (playerStruct = gGT->drivers[0]));

			turboThread = 0;

			if ((GAME_TRACKER->gameMode2 & CHEAT_TURBOCOUNT) != 0)
			{
				// If number of boosts is not zero
				if (playerStruct->numTurbos != 0)
				{
					// Read pointer from address
					turboThread = GAME_TRACKER->threadBuckets[TURBO].thread;

					while (
					    // Pointer != nullptr
					    turboThread != 0 &&

					    // Adds 0x30 to turboThread pointer, gets value (new address) then adds 4.
					    // If   [something]   != pointer that holds boost counter
					    (turboThreadObject = (struct Turbo *)turboThread->object, turboThreadObject->driver != playerStruct))
					{
						turboThread = turboThread->siblingThread;
					}
				}

				// sdata->TurboDisplayPos_Only1P
				// Position of counter
				// 0 = offscreen
				// 1 = just barely on screen
				// ...
				// 10 = clearly on screen

				if ((playerStruct->numTurbos >= 3) && (GAME_SDATA_NAME(TurboDisplayPos_Only1P) < 10) && (turboThread != 0))
				{
					// Animate counter to move onto screen
					GAME_SDATA_NAME(TurboDisplayPos_Only1P) = GAME_SDATA_NAME(TurboDisplayPos_Only1P) + 1;
				}
				else
				{
					if ((turboThread == 0) && (GAME_SDATA_NAME(TurboDisplayPos_Only1P) > 0))
					{
						goto dec_turbo_pos;
					}
					if ((playerStruct->numTurbos < 3) && (GAME_SDATA_NAME(TurboDisplayPos_Only1P) > 0))
					{
					dec_turbo_pos:
						// Animate counter to move off screen
						GAME_SDATA_NAME(TurboDisplayPos_Only1P) = GAME_SDATA_NAME(TurboDisplayPos_Only1P) - 1;
					}
				}

				// If display counter is on screen (fully or not fully)
				if (GAME_SDATA_NAME(TurboDisplayPos_Only1P) != 0)
				{
					char string[32];
					SVec2 turboCountPos;
					// Interpolate the turbo counter slide in from the right
					UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(turboCountPos)), 0x2c8, 0x20, 500, 0x20, GAME_SDATA_NAME(TurboDisplayPos_Only1P), 10);

					// The actual counter number will continue to
					// increase past 1000, but the on-screen text
					// will cap at 999

					// If you have 1000 or more boosts
					if (playerStruct->numTurbos >= 1000)
					{
						// 8d544
						// "999" <-- ascii string, not 2-u8 value

						// Cap the on-screen counter at 999
						sprintf((char *)&string[0], &GAME_SDATA_NAME(s_999)[0]);
					}

					// If you have less than 1000 boosts
					else
					{
						// DAT_8008d54c
						// %d

						// build string for on-screen boost counter
						sprintf((char *)&string[0], &GAME_SDATA_NAME(s_intSpace)[0], playerStruct->numTurbos);
					}

					i = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[LNG_TURBOS], 1);

					// Draw the string
					DecalFont_DrawLine((char *)&string[0], (int)(s16)(turboCountPos.x - i), (int)turboCountPos.y, FONT_BIG, (JUSTIFY_RIGHT | ORANGE_RED));


					sprintf((char *)&string[0], &GAME_SDATA_NAME(s_str)[0], GAME_LANGUAGE_STRINGS[LNG_TURBOS]);

					// Draw the string
					DecalFont_DrawLine((char *)&string[0], (int)(s16)turboCountPos.x, (int)turboCountPos.y, FONT_BIG, (JUSTIFY_RIGHT | ORANGE));

					backBuffer = GAME_TRACKER->backBuffer;
					primMemCurr = backBuffer->primMem.cursor;
					TurboCounterBar = 0;

					if ((void *)primMemCurr <= backBuffer->primMem.guardEnd)
					{
						TurboCounterBar = (POLY_G4 *)primMemCurr;
						backBuffer->primMem.cursor = primMemCurr + 9;
					}

					if (TurboCounterBar == 0)
					{
						return;
					}

					CtrGpu_WriteColorCode(&TurboCounterBar->r0, 0x3800c8ff);
					CtrGpu_WriteColorCode(&TurboCounterBar->r1, 0x3800c8ff);
					CtrGpu_WriteColorCode(&TurboCounterBar->r2, 0x380000ff);
					CtrGpu_WriteColorCode(&TurboCounterBar->r3, 0x380000ff);
					TurboCounterBar->x0 = turboCountPos.x - 0xaa;
					TurboCounterBar->y0 = turboCountPos.y + 9;
					TurboCounterBar->x1 = turboCountPos.x + 0x32;
					TurboCounterBar->y1 = turboCountPos.y + 9;
					TurboCounterBar->x2 = turboCountPos.x - 0x96;
					TurboCounterBar->y2 = turboCountPos.y + 0x12;
					TurboCounterBar->x3 = turboCountPos.x + 0x32;
					TurboCounterBar->y3 = turboCountPos.y + 0x12;

					// pointer to OT memory
					{
						u32 *ot = GAME_TRACKER->pushBuffer_UI.ptrOT;

#if defined(CTR_NATIVE)
						// NOTE(aalhendi): This emitter ORs the full old OT word, and the
						// packed color words above already carry the 0x38 command byte.
						CtrGpu_LinkPacketFullOTTag(ot, &TurboCounterBar->tag, TurboCounterBar, 0x08000000);
#else
						// NOTE(aalhendi): Retail ORs the full old OT word; the packed
						// color words already carry the 0x38 command byte.
						TurboCounterBar->tag = *ot | 0x08000000;
						*ot = (u32)TurboCounterBar & 0xffffff;
#endif
					}
				}
			}
		}

		// if numPlyrCurrGame is not 1 (multiplayer)
		else
		{
			// if you have a time limit (battle)
			if ((gGT->gameMode1 & TIME_LIMIT) != 0)
			{
				// draw countdown clock
				UI_DrawLimitClock(0xd7, 0x68, 2);
			}
		}
	}

	if (((GAME_TRACKER->numPlyrCurrGame == 1) && (levPtrMap != 0) && ((GAME_SDATA_NAME(HudAndDebugFlags) & 8) == 0)) ||
	    ((GAME_TRACKER->numPlyrCurrGame == 3) && (levPtrMap != 0)))
	{
		s16 local_30;
		local_30 = 0;

		UI_Map_DrawDrivers(levPtrMap, GAME_TRACKER->threadBuckets[PLAYER].thread, &local_30);
		UI_Map_DrawDrivers(levPtrMap, GAME_TRACKER->threadBuckets[ROBOT].thread, &local_30);

		UI_Map_DrawGhosts(levPtrMap, GAME_TRACKER->threadBuckets[GHOST].thread);

		UI_Map_DrawTracking(levPtrMap, GAME_TRACKER->threadBuckets[TRACKING].thread);

		if (levPtrMap != 0)
		{
			// Draw the map
			if (GAME_TRACKER->numPlyrCurrGame == 1)
			{
				UI_Map_DrawMap(
				    // top half and bottom half
				    GAME_TRACKER->ptrIcons[3], GAME_TRACKER->ptrIcons[4],

				    // X and Y
				    500, 195,

				    // Pointer to primary memory
				    &GAME_TRACKER->backBuffer->primMem,

				    // pointer to OT memory
				    GAME_TRACKER->pushBuffer_UI.ptrOT,

				    // color, in this case white
				    1);
			}
			else if (GAME_TRACKER->numPlyrCurrGame == 3)
			{
				UI_Map_DrawMap(
				    // top half and bottom half
				    GAME_TRACKER->ptrIcons[3], GAME_TRACKER->ptrIcons[4],

				    // X and Y
				    440, 205,

				    // Pointer to primary memory
				    &GAME_TRACKER->backBuffer->primMem,

				    // pointer to OT memory
				    GAME_TRACKER->pushBuffer_UI.ptrOT,

				    // color, in this case white
				    1);
			}
		}
	}

	bVar3 = false;

	for (i = 0; i < (int)GAME_TRACKER->numPlyrCurrGame; i++)
	{
		struct Driver *d = GAME_TRACKER->drivers[i];
		pb = &GAME_TRACKER->pushBuffer[d->driverID];

		if ((((d->actionsFlagSet & ACTION_RACE_FINISHED) != 0) && ((GAME_TRACKER->gameMode1 & (ARCADE_MODE | TIME_TRIAL)) == 0)) &&
		    ((
		        // cooldown is finished
		        GAME_TRACKER->timerEndOfRaceVS == 0 ||

		        // cooldown has not progressed far,
		        // so you still need to draw "Finished" and "Loser"
		        (0x96 < GAME_TRACKER->timerEndOfRaceVS))))
		{
			if (
			    // player structure + 0x482 is your rank in the race
			    // 0 = 1st place, 1 = 2nd place, 2 = 3rd place, etc

			    // Basically, out of all human players, if you did not come in last
			    ((int)d->driverRank < (int)GAME_TRACKER->numPlyrCurrGame - 1) &&

			    // If you're not in Battle Mode
			    // (winner of battle wont use this function)
			    ((GAME_TRACKER->gameMode1 & BATTLE_MODE) == 0))
			{
				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_FINISHED],
				                   pb->rect.x + (pb->rect.w >> 1),        // midpointX
				                   pb->rect.y + (pb->rect.h >> 1) - 0x1e, // midpointY
				                   FONT_BIG, (JUSTIFY_CENTER | ORANGE));
			}

			// If you came in last place, or you're in battle
			else
			{
				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_LOSER],
				                   pb->rect.x + (pb->rect.w >> 1),        // midpointX
				                   pb->rect.y + (pb->rect.h >> 1) - 0x1e, // midpointY
				                   FONT_BIG, (JUSTIFY_CENTER | ORANGE));
			}

			if (
			    // If you press Cross or Start
			    ((GAMEPADS->gamepad[i].buttonsTapped & 0x1010) != 0) &&

			    // If you're in End-Of-Race menu
			    ((GAME_TRACKER->gameMode1 & END_OF_RACE) != 0))
			{
				// make "Finished" and "Loser" disappear, start
				// drawing the on-screen comments
				GAME_TRACKER->timerEndOfRaceVS = 0x96;
			}
		}

		// if item roll is not done
		if (d->itemRollTimer != 0)
		{
			bVar3 = true;
		}
	}
	if ((
	        // If game is not paused
	        ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0) &&

	        // item roll is done
	        (!bVar3)) &&

	    // If you're drawing Weapon Roulette (randomizing)
	    ((GAME_TRACKER->gameMode1 & ROLLING_ITEM) != 0))
	{
		// stop weapon shuffle sound
		OtherFX_Stop2(0x5d);

		// disable the randomizing effect in the HUD
		GAME_TRACKER->gameMode1 &= ~ROLLING_ITEM;
	}
}

void UI_RenderFrame_AdvHub(void)
{
	struct UiElement2D *hudStructPtr;
	struct GameTracker *gGT;
	struct Driver *player;

	gGT = GAME_TRACKER;
	player = gGT->drivers[0];
	hudStructPtr = GAME_HUD_LAYOUTS[gGT->numPlyrCurrGame - 1];

	UI_DrawNumRelic(hudStructPtr[UI_HUD_SLOT_RELIC].x + 0x10, hudStructPtr[UI_HUD_SLOT_RELIC].y - 10, player);
	UI_DrawNumKey(hudStructPtr[UI_HUD_SLOT_KEY].x + 0x10, hudStructPtr[UI_HUD_SLOT_KEY].y - 10, player);
	UI_DrawNumTrophy(hudStructPtr[UI_HUD_SLOT_TROPHY].x + 0x10, hudStructPtr[UI_HUD_SLOT_TROPHY].y - 10, player);
}

void UI_RenderFrame_CrystChall(void)
{
	struct Driver *player;
	struct UiElement2D *hudStructPtr;
	struct Instance *hudCrystal;
	s32 scaledPos;
	s8 numCrystals;
	SVec2 crystalPos;

	player = GAME_TRACKER->drivers[0];
	hudStructPtr = GAME_HUD_LAYOUTS[GAME_TRACKER->numPlyrCurrGame - 1];

	// If game is not paused
	if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
	{
		// execute Jump meter and landing boost processes
		UI_JumpMeter_Update(player);
	}

	UI_DrawSpeedNeedle(hudStructPtr[UI_HUD_SLOT_SPEEDOMETER].x, hudStructPtr[UI_HUD_SLOT_SPEEDOMETER].y, player);

	UI_JumpMeter_Draw(hudStructPtr[UI_HUD_SLOT_JUMP_METER].x, hudStructPtr[UI_HUD_SLOT_JUMP_METER].y, player);

	UI_DrawSlideMeter(hudStructPtr[UI_HUD_SLOT_SLIDE_METER].x, hudStructPtr[UI_HUD_SLOT_SLIDE_METER].y, player);

	UI_DrawSpeedBG(hudStructPtr[UI_HUD_SLOT_SPEEDOMETER].x, hudStructPtr[UI_HUD_SLOT_SPEEDOMETER].y, player);

	UI_DrawNumCrystal(hudStructPtr[UI_HUD_SLOT_CRYSTAL].x + 0x10, hudStructPtr[UI_HUD_SLOT_CRYSTAL].y - 0x10, player);

	// Draw weapon and number of wumpa fruit in HUD
	UI_Weapon_DrawSelf(hudStructPtr[UI_HUD_SLOT_WEAPON].x, hudStructPtr[UI_HUD_SLOT_WEAPON].y, hudStructPtr[UI_HUD_SLOT_WEAPON].scale, player);

	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_TIME], 0x14, 8, FONT_SMALL, ORANGE);

	// draw countdown clock
	UI_DrawLimitClock(0x14, 0x10, 1);


	// If game is paused
	if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) != 0)
	{
		return;
	}

	if (player->PickupWumpaHUD.numCollected != 0)
	{
		hudCrystal = sdata->ptrHudCrystal;
		crystalPos.x = hudStructPtr[UI_HUD_SLOT_CRYSTAL].x;
		crystalPos.y = hudStructPtr[UI_HUD_SLOT_CRYSTAL].y;

		// make visible
#if defined(CTR_NATIVE)
		if (hudCrystal != NULL)
		{
#endif
			hudCrystal->flags &= ~HIDE_MODEL;
#if defined(CTR_NATIVE)
		}
#endif

		// Animate while cooldown is active; collect once it reaches zero.
		if (player->PickupWumpaHUD.cooldown != 0)
		{
			// interpolate position over course of 5 frames
			UI_Lerp2D_HUD(CTR_VECTOR_DATA(&(crystalPos)), (int)player->PickupWumpaHUD.startX, (int)player->PickupWumpaHUD.startY,
			              (int)hudStructPtr[UI_HUD_SLOT_CRYSTAL].x, (int)hudStructPtr[UI_HUD_SLOT_CRYSTAL].y, player->PickupWumpaHUD.cooldown, 5);

			// reduce cooldown between getting each wumpa (or crystal)
			player->PickupWumpaHUD.cooldown--;
		}
		else
		{
			// NOTE(aalhendi): Publish both pickup fields before callbacks; retain
			// the signed byte count for the completion comparison.
			player->PickupWumpaHUD.numCollected--;
			numCrystals = ++player->numCrystals;

			if (GAME_TRACKER->numCrystalsInLEV <= numCrystals)
			{
				player->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_FreezeEndEvent_Init;

				player->actionsFlagSet |= ACTION_RACE_FINISHED;

				MainGameEnd_Initialize();
			}

			OtherFX_Play(0x42, 1);

			if (player->PickupWumpaHUD.numCollected != 0)
			{
				player->PickupWumpaHUD.cooldown = 5;
			}
		}

		// Convert the HUD position to 3D icon coordinates.

		hudCrystal = sdata->ptrHudCrystal;

		// posX
		scaledPos = (crystalPos.x + -0x100) * hudStructPtr[UI_HUD_SLOT_CRYSTAL].z;
		if (scaledPos < 0)
		{
			scaledPos = scaledPos + 0xff;
		}
#if defined(CTR_NATIVE)
		if (hudCrystal != NULL)
#endif
		{
			hudCrystal->matrix.t[0] = scaledPos >> 8;

			// posY
			scaledPos = (crystalPos.y + -0x6c) * hudStructPtr[UI_HUD_SLOT_CRYSTAL].z;
			if (scaledPos < 0)
			{
				scaledPos = scaledPos + 0xff;
			}
			hudCrystal->matrix.t[1] = scaledPos >> 8;

			// posZ
			hudCrystal->matrix.t[2] = hudStructPtr[UI_HUD_SLOT_CRYSTAL].z;
		}
	}
	else
	{
		struct Instance *hiddenCrystal;

		hiddenCrystal = sdata->ptrHudCrystal;
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Menu-storage can enter crystal HUD flow without
		// a published crystal HUD model.
		if (hiddenCrystal == NULL)
		{
			goto LAB_800545e8;
		}
#endif

		// make invisible
		hiddenCrystal->flags |= HIDE_MODEL;
		goto LAB_800545e8;
	}

LAB_800545e8:

	// quit if game is paused, or item is
	// rolling, or not drawing roulette
	if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) != 0)
	{
		return;
	}
	if (player->itemRollTimer != 0)
	{
		return;
	}
	if ((GAME_TRACKER->gameMode1 & ROLLING_ITEM) == 0)
	{
		return;
	}

	// if not paused, item stopped rolling, and
	// weapon roulette sound is playing, then
	// stop the sound and remove flag
	OtherFX_Stop2(0x5d);
	GAME_TRACKER->gameMode1 &= ~(ROLLING_ITEM);

	return;
}

#include "UI_RenderFrameWumpa.c"
