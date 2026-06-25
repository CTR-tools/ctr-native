#include <common.h>

#if defined(CTR_NATIVE)
static void MainFrame_RegisterGpuLinkRanges(struct GameTracker *gGT)
{
	static const char *const primLabels[2] = {"db0 prim", "db1 prim"};
	static const char *const otLabels[2] = {"db0 OT", "db1 OT"};
	static const char *const swapchainLabels[2] = {"swapchain OT0", "swapchain OT1"};

	NativeGpuLinks_Reset();

	// NOTE(aalhendi): Retail links PS1 RAM addresses directly in 24-bit GPU
	// tags. Native keeps the same packet shape, but maps the double-buffered
	// host draw arenas to stable 24-bit tokens before any OT/tag writer runs.
	for (int i = 0; i < 2; i++)
	{
		struct DB *db = &gGT->db[i];
		NativeGpuLinks_RegisterRangeChecked(primLabels[i], db->primMem.start, db->primMem.capacityBytes);
		NativeGpuLinks_RegisterRangeChecked(otLabels[i], db->otMem.start, db->otMem.capacityBytes);
	}

	u32 swapchainOTBytes = ((u32)gGT->numPlyrCurrGame << 12) | 0x18u;
	for (int i = 0; i < 2; i++)
	{
		NativeGpuLinks_RegisterRangeChecked(swapchainLabels[i], gGT->otSwapchainDB[i], swapchainOTBytes);
	}
}
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80034b48-0x80034bbc.
void MainFrame_TogglePauseAudio(b32 bool_pause)
{
	if (bool_pause == 0)
	{
		if (sdata->boolSoundPaused)
		{
			howl_StopAudio(0, 0, 1);
			howl_UnPauseAudio();
			sdata->boolSoundPaused = 0;
		}
	}
	else if (sdata->boolSoundPaused == 0)
	{
		OtherFX_Stop2(1);
		howl_PauseAudio();
		sdata->boolSoundPaused = 1;
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80034bbc-0x80034d54 for the retail path.
void MainFrame_ResetDB(struct GameTracker *gGT)
{
	uint32_t *puVar3;
	int iVar4;
	struct DB *db;
	int otSwapchainDB;

	// check if new adv hub should be loaded,
	// this was a random place for ND to put it
	LOAD_Hub_Main(sdata->ptrBigfile1);

	gGT->swapchainIndex = 1 - gGT->swapchainIndex;

	gGT->backBuffer = &gGT->db[gGT->swapchainIndex];
	gGT->frameTimer_MainFrame_ResetDB++;

	otSwapchainDB = (int)gGT->otSwapchainDB[gGT->swapchainIndex];

	db = gGT->backBuffer;
	db->blurCameraMask = 0;
	db->primMem.cursor = db->primMem.start;
	db->primMem.primitiveCount = 0;
	db->otMem.cursor = db->otMem.start;

#if defined(CTR_NATIVE)
	MainFrame_RegisterGpuLinkRanges(gGT);
#endif

	CTR_EmptyFunc_MainFrame_ResetDB();
	DecalGlobal_EmptyFunc_MainFrame_ResetDB();

	ClearOTagR((u32 *)otSwapchainDB, sdata->gGT->numPlyrCurrGame << 10 | 6);

	for (iVar4 = 0; iVar4 < sdata->gGT->numPlyrCurrGame; iVar4++)
	{
		gGT->pushBuffer[iVar4].ptrOT = (uint32_t *)((int)otSwapchainDB + (sdata->gGT->numPlyrCurrGame - iVar4 - 1) * 0x1000 + 0x18);
	}

	for (iVar4; iVar4 < 4; iVar4++)
	{
		// but why?
		gGT->pushBuffer[iVar4].ptrOT = (uint32_t *)((int)otSwapchainDB + 3 * 0x1000 + 0x18);
	}

	puVar3 = (uint32_t *)((int)otSwapchainDB + 4);
	gGT->pushBuffer_UI.ptrOT = puVar3;
	db->otMem.uiOT = puVar3;

#if defined(CTR_NATIVE)
	if (sdata->ptrPushBufferUI != 0)
	{
		struct PushBuffer *wumpaPushBuffer = (struct PushBuffer *)(uintptr_t)sdata->ptrPushBufferUI;

		// NOTE(aalhendi): Retail stores PS1 RAM OT addresses here. Native stores
		// host pointers, so reset the fake UI pushbuffer to the current backbuffer
		// before RenderBucket can publish this frame's range metadata.
		wumpaPushBuffer->ptrOT = gGT->pushBuffer_UI.ptrOT;
		wumpaPushBuffer->renderBucketOTRangeEnd = NULL;
		wumpaPushBuffer->renderBucketOTByteOffset = 0;
	}
#endif

	return;
}

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
#include <platform/native_replay_scheduler.h>
#endif

void MainFrame_GameLogic(struct GameTracker *gGT, struct GamepadSystem *gGamepads)
{
	char bVar1;
	s16 sVar2;
	u32 uVar3;
	int iVar4;
	DriverFunc pcVar5;
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
						{
							goto LAB_80034e74;
						}
						uVar3 = 10000;
					}
				}
				else
				{
					if ((psVar9->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
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
				{
					continue;
				}
#endif
				if ((u8)psVar9->numTimesAttacking < (u8)psVar8->numTimesAttacking)
				{
					goto LAB_80035098;
				}
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
					// all drivers must run the same DRIVER_FUNC_* stage
					// at the same time, that's why the stages exist
					for (iVar11 = 0; iVar11 < DRIVER_FUNC_COUNT; iVar11++)
					{
						for (psVar12 = gGT->threadBuckets[iVar4].thread; psVar12 != 0; psVar12 = psVar12->siblingThread)
						{
							// if PLYR converted to robotcar at end of race,
							// dont run funcPtrs from inside driver struct
							if (psVar12->funcThTick != 0)
							{
								continue;
							}

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
			{
				if (sdata->ptrActiveMenu == 0)
				{
					if (sdata->AkuAkuHintState == 0)
					{
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 PSX path 0x80035d30-0x80035d70.
void MainFrame_InitVideoSTR(u32 boolPlayVideoStr, RECT *r, s16 posX, s16 posY)
{
#ifdef CTR_NATIVE
	if (r == NULL)
	{
		// NOTE(aalhendi): Native overlay 230 uses NULL to disable STR copy;
		// retail blindly copies the RECT fields in the PSX path below.
		sdata->videoSTR_src_vramRect.x = 0;
		sdata->videoSTR_src_vramRect.y = 0;
		sdata->videoSTR_src_vramRect.w = 0;
		sdata->videoSTR_src_vramRect.h = 0;
	}
	else
#endif
	{
		sdata->videoSTR_src_vramRect.x = r->x;
		sdata->videoSTR_src_vramRect.y = r->y;
		sdata->videoSTR_src_vramRect.w = r->w;
		sdata->videoSTR_src_vramRect.h = r->h;
	}

	sdata->boolPlayVideoSTR = boolPlayVideoStr;
	sdata->videoSTR_dst_vramX = posX;
	sdata->videoSTR_dst_vramY = posY;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80035d70-0x80035e20.
int MainFrame_HaveAllPads(s16 numPlyrNextGame)
{
	// if game is not loading
	if (sdata->Loading.stage == LOAD_IDLE)
	{
		struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[0];

		if (numPlyrNextGame == 0)
		{
			return 0;
		}

		for (int i = 0; i < numPlyrNextGame; i++)
		{
			struct ControllerPacket *packet = gb->ptrControllerPacket;

			if (packet == NULL)
			{
				return 0;
			}
			if (packet->plugged != PLUGGED)
			{
				return 0;
			}

			gb++;
		}
	}

	return 1;
}

static void MainFrame_ReplacePackedVisList(int *dst, void *src, int byteCount)
{
	u32 srcWord = (u32)src;

	if ((srcWord & 1) == 0)
	{
		memcpy(dst, src, byteCount);
		return;
	}

	CTR_unknownMaybeThunk1(dst, (void *)(srcWord & ~(u32)3));
}

static void MainFrame_OrPackedVisList(int *dst, void *src, int byteCount)
{
	u32 srcWord = (u32)src;

	if ((srcWord & 1) == 0)
	{
		CTR_unknownMaybeThunk3(dst, src, byteCount);
		return;
	}

	CTR_unknownMaybeThunk2(dst, (void *)(srcWord & ~(u32)3));
}

static int MainFrame_VisMemHasQuad(const int *visFaceList, const struct QuadBlock *quad, const struct mesh_info *mesh)
{
	int quadIndex = (int)(quad - mesh->ptrQuadBlockArray);

	return (visFaceList[quadIndex >> 5] & (1 << (quadIndex & 0x1f))) != 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80035684-0x800357b8, unnamed in syms926.
static void MainFrame_VisMemAddDriverPVS(struct GameTracker *gGT, int playerIndex)
{
	struct Driver *driver = gGT->drivers[playerIndex];
	struct mesh_info *mesh = gGT->level1->ptr_mesh_info;
	struct QuadBlock *quad = driver->underDriver;
	struct PVS *pvs;

	if (quad == NULL)
	{
		return;
	}

	pvs = quad->pvs;
	if (pvs == NULL)
	{
		return;
	}

	if (pvs->visLeafSrc != NULL)
	{
		MainFrame_OrPackedVisList(gGT->visMem1->visLeafList[playerIndex], pvs->visLeafSrc, ((mesh->numBspNodes + 0x1f) >> 5) << 2);
	}

	if (pvs->visFaceSrc != NULL)
	{
		MainFrame_OrPackedVisList(gGT->visMem1->visFaceList[playerIndex], pvs->visFaceSrc, ((mesh->numQuadBlock + 0x1f) >> 5) << 2);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800357b8-0x80035d30.
void MainFrame_VisMemFullFrame(struct GameTracker *gGT, struct Level *level)
{
	struct VisMem *visMem;
	struct mesh_info *mesh;
	int playerIndex;

	visMem = gGT->visMem1;
	if (visMem == NULL)
	{
		return;
	}

	if (level == NULL)
	{
		return;
	}

	if (gGT->numPlyrCurrGame == 0)
	{
		return;
	}

	mesh = level->ptr_mesh_info;

	for (playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		struct CameraDC *camDC = &gGT->cameraDC[playerIndex];
		struct Driver *driver = gGT->drivers[playerIndex];
		struct QuadBlock *driverQuad = driver->underDriver;
		struct PVS *driverPVS = NULL;

		if (driverQuad != NULL)
		{
			driverPVS = driverQuad->pvs;
		}

		camDC->flags &= ~0x4000;

		if (camDC->visLeafSrc == NULL)
		{
			if ((driverPVS != NULL) && (driverPVS->visLeafSrc != NULL))
			{
				visMem->visLeafSrc[playerIndex] = driverPVS->visLeafSrc;
				MainFrame_ReplacePackedVisList(visMem->visLeafList[playerIndex], driverPVS->visLeafSrc, ((mesh->numBspNodes + 0x1f) >> 5) << 2);
			}
		}
		else if (visMem->visLeafSrc[playerIndex] != camDC->visLeafSrc)
		{
			visMem->visLeafSrc[playerIndex] = camDC->visLeafSrc;
			MainFrame_ReplacePackedVisList(visMem->visLeafList[playerIndex], camDC->visLeafSrc, ((mesh->numBspNodes + 0x1f) >> 5) << 2);
		}

		if (camDC->visFaceSrc == NULL)
		{
			if ((driverPVS != NULL) && (driverPVS->visFaceSrc != NULL))
			{
				visMem->visFaceSrc[playerIndex] = driverPVS->visFaceSrc;
				MainFrame_ReplacePackedVisList(visMem->visFaceList[playerIndex], driverPVS->visFaceSrc, ((mesh->numQuadBlock + 0x1f) >> 5) << 2);
			}
		}
		else if (visMem->visFaceSrc[playerIndex] != camDC->visFaceSrc)
		{
			visMem->visFaceSrc[playerIndex] = camDC->visFaceSrc;
			MainFrame_ReplacePackedVisList(visMem->visFaceList[playerIndex], camDC->visFaceSrc, ((mesh->numQuadBlock + 0x1f) >> 5) << 2);

			if ((driverPVS == NULL) || (driverPVS->visLeafSrc == NULL) || (driverPVS->visFaceSrc == NULL) || (driverPVS->visInstSrc == NULL) ||
			    MainFrame_VisMemHasQuad(visMem->visFaceList[playerIndex], driverQuad, mesh))
			{
				camDC->flags &= ~0x2000;
			}
			else
			{
				camDC->flags |= 0x2000;
			}

			if ((camDC->flags & 0x2000) != 0)
			{
				MainFrame_VisMemAddDriverPVS(gGT, playerIndex);
				camDC->flags |= 0x4000;
			}
		}

		if ((camDC->flags & 0x5000) == 0x1000)
		{
			MainFrame_VisMemAddDriverPVS(gGT, playerIndex);
		}

		if ((camDC->cameraMode == 0) && ((camDC->flags & 0x2000) != 0) && (driverPVS != NULL) && (driverPVS->visInstSrc != NULL))
		{
			camDC->visInstSrc = driverPVS->visInstSrc;
		}

		if ((level->configFlags & 4) == 0)
		{
			if (visMem->visOVertSrc[playerIndex] != camDC->visOVertSrc)
			{
				visMem->visOVertSrc[playerIndex] = camDC->visOVertSrc;

				if (camDC->visOVertSrc != NULL)
				{
					MainFrame_ReplacePackedVisList(visMem->visOVertList[playerIndex], camDC->visOVertSrc, ((level->numWaterVertices + 0x1f) >> 5) << 2);
				}
				else
				{
					memcpy(visMem->visOVertList[playerIndex], level->unk5, ((level->numWaterVertices + 0x1f) >> 5) << 2);
				}
			}
			else if (visMem->visOVertSrc[playerIndex] == NULL)
			{
				memcpy(visMem->visOVertList[playerIndex], level->unk5, ((level->numWaterVertices + 0x1f) >> 5) << 2);
			}
		}
		else
		{
			if (visMem->visSCVertSrc[playerIndex] != camDC->visSCVertSrc)
			{
				visMem->visSCVertSrc[playerIndex] = camDC->visSCVertSrc;

				if (camDC->visSCVertSrc != NULL)
				{
					MainFrame_ReplacePackedVisList(visMem->visSCVertList[playerIndex], camDC->visSCVertSrc, ((level->numSCVert + 0x1f) >> 5) << 2);
				}
				else
				{
					memcpy(visMem->visSCVertList[playerIndex], level->unk_170, ((level->numSCVert + 0x1f) >> 5) << 2);
				}
			}
			else if (visMem->visSCVertSrc[playerIndex] == NULL)
			{
				memcpy(visMem->visSCVertList[playerIndex], level->unk_170, ((level->numSCVert + 0x1f) >> 5) << 2);
			}
		}
	}
}

// Request Aku Hint, doesn't start till FUN_800b3dd8
// hintId:
//	0x00 - Welcome to Adventure Arena
//	0x01 - using a warp pad (part of welcome)
// 	0x02 - Need more trophies
//	0x03 - Need 4 trophies for Boss
//	0x04 - Need 4 keys for oxide
//	0x05 - Must have 1 Boss Key
//	0x06 - This is the load/save screen
// 	0x07 - Congrats on opening new area
//	0x12 - Must have 2 Boss Key
//	0x19 - Collect every crystal in arena
//	0x1a - CTR Token
//	0x1b - Gem Cups
//	0x1c - Must get 10 relics
//	0x1d - Relic
// param2:
//	0x00 - not interrupting a warppad load screen
// 	0x01 - interrupting (CTR, Relic, or Crystal hints)
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80035e20-0x80035e70.
void MainFrame_RequestMaskHint(s16 hintId, char interruptWarpPad)
{
	struct GameTracker *gGT = sdata->gGT;

	if (((gGT->gameMode1 & PAUSE_ALL) == 0) && (sdata->AkuHint_RequestedHint == -1))
	{
		sdata->AkuAkuHintState = 1;

		gGT->drivers[0]->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_FreezeEndEvent_Init;

		sdata->AkuHint_RequestedHint = hintId;
		sdata->AkuHint_boolInterruptWarppad = interruptWarpPad;
	}
	return;
}
