#include <common.h>

#ifndef MAIN_FRAME_ACTIVE_MENU
#define MAIN_FRAME_ACTIVE_MENU (sdata->ptrActiveMenu)
#define MAIN_FRAME_ANY_TAP     (sdata->AnyPlayerTap)
#define MAIN_FRAME_SAVE_CUP    (sdata->boolSaveCupProgress)
#endif

#ifndef MAIN_FRAME_BOTS_UPDATE
#define MAIN_FRAME_BOTS_UPDATE(tracker)     BOTS_UpdateGlobals()
#define MAIN_FRAME_PARTICLE_UPDATE(tracker) Particle_UpdateAllParticles()
#endif

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
#include <platform/native_replay_scheduler.h>
#endif

void MainFrame_GameLogic(struct GameTracker *frameTracker, struct GamepadSystem *gGamepads)
{
	register struct GameTracker *gGT CTR_PSX_REGISTER("$21") = frameTracker;
	u16 wasPausedAtFrameStart;
	s16 sVar2;
	u32 uVar3;
	u32 frameMode;
	struct GameTracker *modeTracker;
	u32 modeMask;
	u32 initialMode;
	int iVar4;
	DriverFunc pcVar5;
	u32 uVar5;
	u32 connectedPads;
	struct Driver *psVar8;
	struct Driver *psVar9;
	int iVar11;
	struct Thread *psVar12;
	u8 *threadBucketBase;
	int threadBucketOffset;
	int stageBucketOffset;

	// NOTE(aalhendi): Observe the mode load before setting the saved pause flag;
	// otherwise GCC hoists that setup ahead of the load.
	initialMode = gGT->gameMode1;
	CTR_PSX_OBSERVE_VALUE(initialMode);
	wasPausedAtFrameStart = true;
	if ((initialMode & PAUSE_ALL) == 0)
	{
		{
			struct Driver *psVar9;
			struct PushBuffer *pushBuffer;
			struct Thread *psVar12;
			struct GameTracker *clockTracker;
			register u32 clockValue CTR_PSX_REGISTER("$2");
			s32 blurValue;
			struct PushBuffer *blurBuffer;
			clockTracker = GAME_TRACKER;
			wasPausedAtFrameStart = false;
			pushBuffer = clockTracker->pushBuffer;
			for (psVar12 = clockTracker->threadBuckets[0].thread; psVar12 != 0; psVar12 = psVar12->siblingThread)
			{
				psVar9 = (struct Driver *)psVar12->object;
				if (psVar9->clockSend)
					psVar9->clockSend--;
				clockValue = psVar9->clockFlash;
				if (clockValue != 0)
				{
					blurValue = clockValue;
					CTR_PSX_KEEP_VALUE_RELAXED(blurValue);
					DISPLAY_Blur_Main(pushBuffer, -blurValue);
					psVar9->clockFlash--;
					goto clockNext;
				}
				else if (psVar9->clockReceive != 0)
				{
					if ((psVar9->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
						psVar9->clockReceive = 0;
					blurValue = psVar9->clockReceive;
					blurBuffer = pushBuffer;
					goto drawClock;
				}
				else
				{
					clockValue = psVar9->clockSend;
					blurValue = clockValue;
					if (clockValue != 0)
					{
						blurBuffer = pushBuffer;
						goto drawClock;
					}
					clockValue = GAME_TRACKER->clockEffectEnabled & 1;
					blurBuffer = pushBuffer;
					if (clockValue == 0)
						goto clockNext;
					blurValue = 10000;
				}
				goto drawClock;
			drawClock:
				DISPLAY_Blur_Main(blurBuffer, blurValue);
			clockNext:
				pushBuffer++;
			}
		}

		gGT->timer = gGT->timer + 1;
		gGT->framesInThisLEV = gGT->framesInThisLEV + 1;
		GAME_TRACKER->unk1cc4[4] = 0;

		{
			int iVar4;
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
			if ((GAME_TRACKER->gameMode1_prevFrame & PAUSE_ALL) != 0)
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
			if (gGT->trafficLightsTimer > 0)
			{
				gGT->elapsedEventTime = 0;
			}
			else
			{
				if ((GAME_TRACKER->gameMode1 & DEBUG_MENU) == 0)
				{
					if (gGT->frozenTimeRemaining > 0)
					{
						int frozenDifference = gGT->frozenTimeRemaining - gGT->elapsedTimeMS;
						gGT->frozenTimeRemaining = frozenDifference;
						if (frozenDifference < 0)
						{
							gGT->frozenTimeRemaining = 0;
						}
						else
						{
							uVar3 = GAME_TRACKER->timer;
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
					else
					{
						if ((GAME_TRACKER->gameMode1 & END_OF_RACE) == 0)
						{
							gGT->elapsedEventTime += gGT->elapsedTimeMS;
						}
					}
				}
			}
		}

		CTR_CycleTex_AllModels(-1, (struct Model **)sdata->PLYROBJECTLIST, gGT->timer);
		CTR_CycleTex_AllModels(gGT->level1->numModels, gGT->level1->ptrModelsPtrArray, gGT->timer);

		{
			struct GameTracker *tallyTracker = GAME_TRACKER;
			struct Thread *psVar12;
			psVar8 = 0;
			psVar12 = tallyTracker->threadBuckets[0].thread;
			psVar9 = psVar8;
			if (psVar12 != 0)
			{
				for (; psVar12 != 0; psVar12 = psVar12->siblingThread)
				{
					struct Driver *currentDriver = psVar12->object;

					if (currentDriver->driverID == 0)
					{
						psVar8 = currentDriver;
						continue;
					}

					if (currentDriver->driverID == 1)
					{
						psVar9 = currentDriver;
					}

#if defined(CTR_NATIVE)
					// NOTE(aalhendi): Retail may read PSX low memory before driver 0 appears.
					if (psVar8 == NULL)
					{
						continue;
					}
#endif

					if (currentDriver->numTimesAttacking < psVar8->numTimesAttacking)
					{
						psVar9 = psVar8;
						psVar8 = currentDriver;
					}
				}
			}
		}

		{
			int attackDifference;
			if (((psVar8 != 0) && (psVar9 != 0)) &&
			    (attackDifference = (u32)psVar9->numTimesAttacking - (u32)psVar8->numTimesAttacking, psVar8->quip2 < attackDifference))
			{
				psVar8->quip2 = (s16)attackDifference;
			}
		}

		iVar4 = 0;
		threadBucketBase = (u8 *)gGT;
		stageBucketOffset = iVar4;
		threadBucketOffset = offsetof(struct GameTracker, threadBuckets);
		for (; iVar4 < PAUSE; threadBucketBase += sizeof(struct ThreadBucket), stageBucketOffset += sizeof(struct ThreadBucket), iVar4++,
		                      threadBucketOffset += sizeof(struct ThreadBucket))
		{
			struct GameTracker *bucketGlobal = GAME_TRACKER;
			struct ThreadBucket *currentBucket;
			currentBucket = (struct ThreadBucket *)((u8 *)gGT + threadBucketOffset);
			if ((((bucketGlobal->gameMode1 & DEBUG_MENU) == 0) || ((currentBucket->boolCantPause & 1) != 0)) && currentBucket->thread != 0)
			{
				if (iVar4 == 0)
				{
					for (psVar12 = bucketGlobal->threadBuckets[0].thread; psVar12 != 0; psVar12 = psVar12->siblingThread)
					{
						VehPickupItem_ShootOnCirclePress((struct Driver *)psVar12->object);
					}

					// run all driver funcPtrs,
					// all drivers must run the same DRIVER_FUNC_* stage
					// at the same time, that's why the stages exist
					{
						int driverStageOffset;
						iVar11 = 0;
						driverStageOffset = stageBucketOffset;
						for (; iVar11 < DRIVER_FUNC_COUNT; iVar11++)
						{
							struct ThreadBucket *stageBucket;
							// NOTE(aalhendi): 32-bit address sums preserve retail operand order;
							// the loads still address the actual bucket and Driver fields.
							stageBucket = (struct ThreadBucket *)((u32)GAME_TRACKER + (u32)driverStageOffset + offsetof(struct GameTracker, threadBuckets));
							psVar12 = stageBucket->thread;
							if (psVar12 != 0)
							{
								int driverFuncOffset = iVar11 * sizeof(DriverFunc);
								do
								{
									if (psVar12->funcThTick == 0)
									{
										struct Driver *stageDriver = (struct Driver *)psVar12->object;
										u8 *stageFuncAddress;
										stageFuncAddress = (u8 *)((u32)stageDriver + (u32)driverFuncOffset);
										pcVar5 = *(DriverFunc *)(stageFuncAddress + offsetof(struct Driver, funcPtrs));
										if (pcVar5 != 0)
											pcVar5(psVar12, stageDriver);
									}
									psVar12 = psVar12->siblingThread;
								} while (psVar12 != 0);
							}
						}
					}
				}

				ThTick_RunBucket(((struct ThreadBucket *)(threadBucketBase + offsetof(struct GameTracker, threadBuckets)))->thread);
			}
		}

		MAIN_FRAME_BOTS_UPDATE(GAME_TRACKER);
		GhostTape_WriteMoves(0);
		{
			struct GameTracker *particleTracker = GAME_TRACKER;
			particleTracker->unk1cc4[4] = (u32)(particleTracker->unk1cc4[4] * 10000) / 0x147e;
			MAIN_FRAME_PARTICLE_UPDATE(particleTracker);
		}
	}
	else
	{
		struct Thread *akuThread = gGT->threadBuckets[AKUAKU].thread;
		if (akuThread != 0)
		{
			ThTick_RunBucket(akuThread);
		}
	}

	uVar5 = LOAD_IsOpen_RacingOrBattle();
	if (uVar5 != 0)
	{
		if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
		{
			RB_Bubbles_RoosTubes();
		}
		if (GAME_TRACKER->threadBuckets[BURST].thread != 0)
		{
			RB_Burst_DrawAll(GAME_TRACKER);
		}
	}

	PROC_CheckAllForDead();

	if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
	{
		Audio_Update1();
	}

	gGT->gameMode1_prevFrame = gGT->gameMode1;
	connectedPads = GAMEPAD_GetNumConnected(GAMEPADS);
	modeTracker = GAME_TRACKER;
	frameMode = modeTracker->gameMode1;

	if ((frameMode & END_OF_RACE) != 0)
	{
		if (GAME_TRACKER->timerEndOfRaceVS != 0)
		{
			modeMask = frameMode & ARCADE_MODE;
			if (modeMask == 0)
			{
				if (GAME_TRACKER->timerEndOfRaceVS < 0x96)
				{
					UI_VsQuipDrawAll();
					UI_VsWaitForPressX();
				}
				if (0x1e < GAME_TRACKER->timerEndOfRaceVS)
				{
					GAME_TRACKER->timerEndOfRaceVS--;
				}
			}
			else
			{
				GAME_TRACKER->timerEndOfRaceVS = 0;
			}
		}
		else
		{
			uVar3 = GAME_TRACKER->gameModeEnd;
			if ((uVar3 & AKU_SONG) == 0)
			{
				if ((uVar3 & CRYSTAL_CHALLENGE) != 0)
				{
					if (GAME_TRACKER->unk_timerCooldown_similarTo_1d36 == 0)
					{
						u32 endPause = uVar3 & PAUSE_2;
						if (endPause == 0)
						{
							return;
						}

						sVar2 = SubmitName_DrawMenu(0x140);

						// if not done yet
						if (sVar2 == 0)
						{
							return;
						}


						// if SAVE
						if (sVar2 == 1)
						{
							MAIN_FRAME_SAVE_CUP = 0;

							SelectProfile_ToggleMode(SELECT_PROFILE_MODE_SLOT_SAVE);

							RECTMENU_Show(&data.menuWarning2);
							GAME_TRACKER->gameModeEnd |= NEW_NAME;

							return;
						}

						// if -1 (cancel)
						{
							// NOTE(aalhendi): Keep -1 full-width until the byte store.
							int cancelIndex = -1;
							GAME_TRACKER->newHighScoreIndex = cancelIndex;
						}
						GAME_TRACKER->gameModeEnd &= ~(NEW_BEST_LAP | NEW_HIGH_SCORE);
						return;
					}
				}
				else
				{
					if (GAME_TRACKER->unk_timerCooldown_similarTo_1d36 == 0)
					{
						return;
					}
				}
				GAME_TRACKER->unk_timerCooldown_similarTo_1d36--;
			}
		}
	}
	else
	{
		u32 pauseTest;
		// NOTE(aalhendi): Copy the actual spilled flag so old GCC can reuse
		// its reload scratch for this conversion and the earlier divides.
		CTR_PSX_COPY_VALUE(pauseTest, wasPausedAtFrameStart);
		if (pauseTest || ((gGT->gameMode1 & PAUSE_ALL) != 0))
		{
			if (GAME_TRACKER->cooldownfromPauseUntilUnpause != 0)
			{
				GAME_TRACKER->cooldownfromPauseUntilUnpause--;
			}
			else
			{
				if (((MAIN_FRAME_ACTIVE_MENU != &data.menuRacingWheelConfig) && (MAIN_FRAME_ACTIVE_MENU != &D232.menuHintMenu) // in 232
				     ) &&
				    ((MAIN_FRAME_ANY_TAP & BTN_START) != 0))
				{
					RECTMENU_ClearInput();
					GAME_TRACKER->gameMode1 &= ~PAUSE_1;

					MainFrame_TogglePauseAudio(0);
					OtherFX_Play(1, 1);

					MainFreeze_SafeAdvDestroy();
					ElimBG_Deactivate(GAME_TRACKER);

					RECTMENU_Hide(MAIN_FRAME_ACTIVE_MENU);
					GAME_TRACKER->cooldownFromUnpauseUntilPause = 5;
				}
			}
		}
		else
		{
			if (GAME_TRACKER->cooldownFromUnpauseUntilPause != 0)
			{
				GAME_TRACKER->cooldownFromUnpauseUntilPause--;
			}
			else
			{
				modeMask = frameMode & (GAME_CUTSCENE | END_OF_RACE | MAIN_MENU);
				if (modeMask == 0)
				{
					if (MAIN_FRAME_ACTIVE_MENU == 0)
					{
						if (sdata->AkuAkuHintState == 0)
						{
							if (!RaceFlag_IsFullyOnScreen())
							{
								{
									struct GamepadBuffer *pausePad;
									int padPlayers;
									iVar4 = 0;
									if (gGT->numPlyrCurrGame != 0)
									{
										pausePad = &gGamepads->gamepad[0];
										do
										{
											if ((((connectedPads != 0) && ((!(b16)MainFrame_HaveAllPads((u16)GAME_TRACKER->numPlyrNextGame) &&

											                                ((gGT->gameMode1 & PAUSE_ALL) == 0)))) ||
											     ((pausePad->buttonsTapped & BTN_START) != 0)) &&
											    (GAME_TRACKER->overlayIndex_Threads != OVERLAY_INDEX_NONE))
											{
												// NOTE(aalhendi): Retail writes this before freezing the game for pause.
												GAME_TRACKER->gameModeEnd = (GAME_TRACKER->gameMode1 & GAME_MODE_END_RETAINED_MODE_MASK) | PAUSE_1;

												MainFreeze_IfPressStart();

												GAME_TRACKER->cooldownfromPauseUntilUnpause = 5;
											}

											padPlayers = gGT->numPlyrCurrGame;
											CTR_PSX_KEEP_VALUE_RELAXED(padPlayers);
											iVar4++;
											pausePad++;
										} while (iVar4 < padPlayers);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return;
}
