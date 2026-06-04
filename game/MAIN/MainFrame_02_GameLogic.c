#include <common.h>

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
#include <platform/native_replay_scheduler.h>
#endif

typedef void (*VehicleFuncPtr)(struct Thread *thread, struct Driver *driver);

void MainFrame_GameLogic(struct GameTracker *gGT, struct GamepadSystem *gGamepads)
{
	char bVar1;
	s16 sVar2;
	u32 uVar3;
	int iVar4;
	VehicleFuncPtr pcVar5;
	u32 uVar5;
	u32 uVar6;
	int *piVar7;
	struct Driver *psVar8;
	struct Driver *psVar9;
	struct Driver *psVar10;
	struct Driver *pvVar12;
	struct PushBuffer *pushBuffer;
	int iVar11;
	struct Thread *psVar12;

	bVar1 = true;
	if ((gGT->gameMode1 & PAUSE_ALL) == 0)
	{
		bVar1 = false;
		pushBuffer = gGT->pushBuffer;
		for (psVar12 = gGT->threadBuckets[0].thread; psVar12 != 0; psVar12 = psVar12->siblingThread)
		{
			psVar9 = (struct Driver *)psVar12->object;

			if (psVar9->clockSend)
			{
				psVar9->clockSend--;
			}
			uVar3 = psVar9->clockFlash;
			if (uVar3 == 0)
			{
				if (psVar9->clockReceive == 0)
				{
					uVar3 = (u32)psVar9->clockSend;
					if (uVar3 == 0)
					{
						if ((gGT->clockEffectEnabled & 1) == 0)
							goto LAB_80034e74;
						uVar3 = 10000;
					}
				}
				else
				{
					if ((psVar9->actionsFlagSet & 0x2000000) != 0)
					{
						psVar9->clockReceive = 0;
					}
					uVar3 = (u32)psVar9->clockReceive;
				}

#if defined(CTR_NATIVE)
				DISPLAY_Blur_Main(pushBuffer, uVar3);
#endif
			}
			else
			{
#if defined(CTR_NATIVE)
				DISPLAY_Blur_Main(pushBuffer, -uVar3);
#endif
				psVar9->clockFlash--;
			}
		LAB_80034e74:
			pushBuffer = pushBuffer + 1;
		}
		gGT->timer = gGT->timer + 1;
		gGT->framesInThisLEV = gGT->framesInThisLEV + 1;
		gGT->unk1cc4[4] = 0;

		iVar4 = Timer_GetTime_Elapsed(gGT->clockFrameStart, &gGT->clockFrameStart);
		iVar4 = (iVar4 << 5) / 100;

		gGT->elapsedTimeMS = iVar4;
		if (iVar4 < 0)
		{
			gGT->elapsedTimeMS = 0x20;
		}
		if (0x40 < gGT->elapsedTimeMS)
		{
			gGT->elapsedTimeMS = 0x40;
		}
		if ((gGT->gameMode1_prevFrame & PAUSE_ALL) != 0)
		{
			gGT->elapsedTimeMS = 0x20;
		}
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
		// NOTE(aalhendi): Replay playback must not let host RCNT timing decide
		// cutscene/gameplay advancement. Use the recorded PS1-shaped frame delta
		// before msInThisLEV and elapsedEventTime consume it.
		NativeReplayScheduler_ConsumeFrameElapsedTimeMS(&gGT->elapsedTimeMS);
#endif
		gGT->msInThisLEV += gGT->elapsedTimeMS;
		if (gGT->trafficLightsTimer < 1)
		{
			if ((gGT->gameMode1 & DEBUG_MENU) == 0)
			{
				if (gGT->frozenTimeRemaining < 1)
				{
					if ((gGT->gameMode1 & END_OF_RACE) == 0)
					{
						gGT->elapsedEventTime += gGT->elapsedTimeMS;
					}
				}
				else
				{
					iVar4 = gGT->frozenTimeRemaining - gGT->elapsedTimeMS;
					gGT->frozenTimeRemaining = iVar4;
					if (iVar4 < 0)
					{
						gGT->frozenTimeRemaining = 0;
					}
					else
					{
						uVar3 = gGT->timer;
						// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80034f84-0x80034fec for frozen-time tick SFX.
						if (uVar3 == (uVar3 / 6) * 6)
						{
							if (uVar3 == (uVar3 / 0xc) * 0xc)
							{
								OtherFX_Play_LowLevel(0x40, '\0', 0x8c9080);
							}
							else
							{
								OtherFX_Play_LowLevel(0x40, '\0', 0x8c8880);
							}
						}
					}
				}
			}
		}
		else
		{
			gGT->elapsedEventTime = 0;
		}

		CTR_CycleTex_AllModels(-1, (struct Model **)sdata->PLYROBJECTLIST, gGT->timer);
		CTR_CycleTex_AllModels(gGT->level1->numModels, gGT->level1->ptrModelsPtrArray, gGT->timer);

		psVar8 = 0;
		psVar9 = 0;

#if defined(CTR_NATIVE)
		for (psVar12 = gGT->threadBuckets[0].thread; psVar12 != 0; psVar12 = psVar12->siblingThread)
		{
			psVar9 = (struct Driver *)psVar12->object;
			psVar10 = psVar9;
			if (psVar9->driverID == 0)
			{
			LAB_80035098:
				psVar8 = psVar9;
				psVar9 = psVar10;
			}
			else
			{
				if (psVar9->driverID == 1)
				{
					psVar9 = psVar9;
				}
				psVar10 = psVar8;
#ifdef CTR_NATIVE
				// NOTE(aalhendi): Retail may read PSX low memory before driver 0 appears.
				if (psVar8 == NULL)
					continue;
#endif
				if ((u8)psVar9->numTimesAttacking < (u8)psVar8->numTimesAttacking)
					goto LAB_80035098;
			}
		}
#endif

		if (((psVar8 != 0) && (psVar9 != 0)) && (iVar4 = (u32)(u8)psVar9->numTimesAttacking - (u32)(u8)psVar8->numTimesAttacking, psVar8->quip2 < iVar4))
		{
			psVar8->quip2 = (s16)iVar4;
		}

		for (iVar4 = 0; iVar4 < NUM_BUCKETS; iVar4++)
		{
			if ((((gGT->gameMode1 & DEBUG_MENU) == 0) || ((gGT->threadBuckets[iVar4].boolCantPause & 1) != 0)) &&

			    // if threads exist
			    (gGT->threadBuckets[iVar4].thread != 0))
			{
				if (iVar4 == 0)
				{
					for (psVar12 = gGT->threadBuckets[iVar4].thread; psVar12 != 0; psVar12 = psVar12->siblingThread)
					{
						VehPickupItem_ShootOnCirclePress((struct Driver *)psVar12->object);
					}

					// run all driver funcPtrs,
					// all drivers must run the same stage (1-13)
					// at the same time, that's why the stages exist
					for (iVar11 = 0; iVar11 < 13; iVar11++)
					{
						for (psVar12 = gGT->threadBuckets[iVar4].thread; psVar12 != 0; psVar12 = psVar12->siblingThread)
						{
							// if PLYR converted to robotcar at end of race,
							// dont run funcPtrs from inside driver struct
							if (psVar12->funcThTick != 0)
								continue;

							psVar9 = (struct Driver *)psVar12->object;

							pcVar5 = psVar9->funcPtrs[iVar11];

							if (pcVar5 != 0)
							{
								pcVar5(psVar12, psVar9);
							}
						}
					}
				}

				ThTick_RunBucket(gGT->threadBuckets[iVar4].thread);
			}
		}

#if defined(CTR_NATIVE)
		BOTS_UpdateGlobals();
#endif
		GhostTape_WriteMoves(0);
		gGT->unk1cc4[4] = (u32)(gGT->unk1cc4[4] * 10000) / 0x147e;

#if defined(CTR_NATIVE)


		Particle_UpdateAllParticles();

#endif
	}
	else
	{
		psVar12 = gGT->threadBuckets[AKUAKU].thread;
		if (psVar12 != 0)
		{
			ThTick_RunBucket(psVar12);
		}
	}

	uVar5 = LOAD_IsOpen_RacingOrBattle();
	if (uVar5 != 0)
	{
#if defined(CTR_NATIVE)
		if ((gGT->gameMode1 & PAUSE_ALL) == 0)
		{
			RB_Bubbles_RoosTubes();
		}
#endif
		if (gGT->threadBuckets[BURST].thread != 0)
		{
			RB_Burst_DrawAll(gGT);
		}
	}

	PROC_CheckAllForDead();

	if ((gGT->gameMode1 & PAUSE_ALL) == 0)
	{
		Audio_Update1();
	}

	gGT->gameMode1_prevFrame = gGT->gameMode1;
	uVar5 = GAMEPAD_GetNumConnected(gGamepads);
	uVar3 = gGT->gameMode1;

	if ((uVar3 & END_OF_RACE) == 0)
	{
		if ((bVar1) || ((uVar3 & PAUSE_ALL) != 0))
		{
			if (gGT->cooldownfromPauseUntilUnpause == 0)
			{
				if (((sdata->ptrActiveMenu != &data.menuRacingWheelConfig) && (sdata->ptrActiveMenu != &D232.menuHintMenu) // in 232
				     ) &&
				    ((sdata->AnyPlayerTap & BTN_START) != 0))
				{
					RECTMENU_ClearInput();
					gGT->gameMode1 &= ~PAUSE_1;

					// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800354dc-0x80035508 for unpause audio side effects.
					MainFrame_TogglePauseAudio(0);
					OtherFX_Play(1, 1);

					MainFreeze_SafeAdvDestroy();
					ElimBG_Deactivate(gGT);

					RECTMENU_Hide(sdata->ptrActiveMenu);
					gGT->cooldownFromUnpauseUntilPause = 5;
				}
			}
			else
			{
				gGT->cooldownfromPauseUntilUnpause--;
			}
		}
		else if (gGT->cooldownFromUnpauseUntilPause == 0)
		{
			if ((uVar3 & (GAME_CUTSCENE | END_OF_RACE | MAIN_MENU)) == 0)
				if (sdata->ptrActiveMenu == 0)
					if (sdata->AkuAkuHintState == 0)
						if (RaceFlag_IsFullyOnScreen() == 0)
						{
							for (iVar4 = 0; iVar4 < gGT->numPlyrCurrGame; iVar4++)
							{
								if ((((uVar5 != 0) &&
								      ((
#if defined(CTR_NATIVE)
								          uVar3 = MainFrame_HaveAllPads((u16)(u8)gGT->numPlyrNextGame), (uVar3 & 0xffff) == 0 &&
#endif
								                                                                            ((gGT->gameMode1 & PAUSE_ALL) == 0)))) ||
								     ((gGamepads->gamepad[iVar4].buttonsTapped & BTN_START) != 0)) &&
								    (gGT->overlayIndex_Threads != -1))
								{
									// NOTE(aalhendi): Retail writes this before freezing the game for pause.
									gGT->gameModeEnd = (gGT->gameMode1 & 0x3e0020) | PAUSE_1;

									MainFreeze_IfPressStart();

									gGT->cooldownfromPauseUntilUnpause = 5;
								}
							}
						}
		}
		else
		{
			gGT->cooldownFromUnpauseUntilPause--;
		}
	}
	else if (gGT->timerEndOfRaceVS == 0)
	{
		uVar3 = gGT->gameModeEnd;
		if ((uVar3 & AKU_SONG) == 0)
		{
			if ((uVar3 & CRYSTAL_CHALLENGE) == 0)
			{
				if (gGT->unk_timerCooldown_similarTo_1d36 == 0)
				{
					return;
				}
			}
			else if (gGT->unk_timerCooldown_similarTo_1d36 == 0)
			{
				if ((uVar3 & PAUSE_2) == 0)
				{
					return;
				}

				sVar2 = SubmitName_DrawMenu(0x140);

				// if not done yet
				if (sVar2 == 0)
				{
					return;
				}


#if defined(CTR_NATIVE)

				// if SAVE
				if (sVar2 == 1)
				{
					sdata->boolSaveCupProgress = 0;

					SelectProfile_ToggleMode(0x41);

					RECTMENU_Show(&data.menuWarning2);
					gGT->gameModeEnd |= NEW_NAME;

					return;
				}
#endif

				// if -1 (cancel)
				gGT->newHighScoreIndex = -1;
				gGT->gameModeEnd &= ~(NEW_BEST_LAP | NEW_HIGH_SCORE);
				return;
			}
			gGT->unk_timerCooldown_similarTo_1d36--;
		}
	}
	else if ((uVar3 & ARCADE_MODE) == 0)
	{
		if (gGT->timerEndOfRaceVS < 0x96)
		{
#if defined(CTR_NATIVE)
			UI_VsQuipDrawAll();
			UI_VsWaitForPressX();
#endif
		}
		if (0x1e < gGT->timerEndOfRaceVS)
		{
			gGT->timerEndOfRaceVS--;
		}
	}
	else
	{
		gGT->timerEndOfRaceVS = 0;
	}
	return;
}
