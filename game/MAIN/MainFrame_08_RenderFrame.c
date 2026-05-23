#include <common.h>

// all in this file
void DrawUnpluggedMsg(struct GameTracker *gGT, struct GamepadSystem *gGamepads);
void DrawFinalLap(struct GameTracker *gGT);
void RainLogic(struct GameTracker *gGT);
void MenuHighlight();
void RenderAllWeather(struct GameTracker *gGT);
void RenderAllConfetti(struct GameTracker *gGT);
void RenderAllHUD(struct GameTracker *gGT);
void RenderAllBeakerRain(struct GameTracker *gGT);
void RenderAllBoxSceneSplitLines(struct GameTracker *gGT);
void RenderBucket_QueueAllInstances(struct GameTracker *gGT);
void RenderAllNormalParticles(struct GameTracker *gGT);
void RenderDispEnv_World(struct GameTracker *gGT);
void RenderAllFlag0x40(struct GameTracker *gGT);
void RenderAllTitleDPP(struct GameTracker *gGT);
void RenderBucket_ExecuteAllInstances(struct GameTracker *gGT);
void RenderAllTires(struct GameTracker *gGT);
void RenderAllShadows(struct GameTracker *gGT);
void RenderAllHeatParticles(struct GameTracker *gGT);
void RenderAllLevelGeometry(struct GameTracker *gGT);
void MultiplayerWumpaHUD(struct GameTracker *gGT);
void WindowBoxLines(struct GameTracker *gGT);
void WindowDivsionLines(struct GameTracker *gGT);
void RenderDispEnv_UI(struct GameTracker *gGT);
void RenderVSYNC(struct GameTracker *gGT);
void RenderFMV();
void RenderSubmit(struct GameTracker *gGT);

// original CTR funcs
void UI_CupStandings_InputAndDraw();
void VB_EndEvent_DrawMenu();
void RR_EndEvent_DrawMenu();
void AA_EndEvent_DrawMenu();
void TT_EndEvent_DrawMenu();
void CC_EndEvent_DrawMenu();
void OVR_Region1(void);
void CS_BoxScene_InstanceSplitLines();
void RB_Player_ToggleInvisible();
void RB_Player_ToggleFlicker();
void RB_Burst_ProcessBucket(struct Thread *thread);
void RB_Blowup_ProcessBucket(struct Thread *thread);
void RB_Follower_ProcessBucket(struct Thread *thread);
void RB_StartText_ProcessBucket(struct Thread *thread);
u32 MM_Video_CheckIfFinished(int param_1);

#ifdef CTR_INTERNAL
volatile int gCtrDebugSkipLevelGeometry = 0;
#endif

void DECOMP_MainFrame_RenderFrame(struct GameTracker *gGT, struct GamepadSystem *gGamepads)
{
	struct Level *lev = gGT->level1;


	DrawUnpluggedMsg(gGT, gGamepads);
	DrawFinalLap(gGT);

	DECOMP_ElimBG_HandleState(gGT);

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	if ((gGT->renderFlags & 0x21) != 0)
		MainFrame_VisMemFullFrame(gGT, gGT->level1);
#endif


	if ((gGT->renderFlags & 1) != 0)
		if (gGT->visMem1 != 0)
			if (lev != 0)
				DECOMP_CTR_CycleTex_LEV(lev->ptr_anim_tex, gGT->timer);

	if ((sdata->ptrActiveMenu != 0) || ((gGT->gameMode1 & END_OF_RACE) != 0))
	{
		DECOMP_RECTMENU_CollectInput();
	}

	if (sdata->ptrActiveMenu != 0)
		if (sdata->Loading.stage == -1)
			DECOMP_RECTMENU_ProcessState();

	RainLogic(gGT);
	DECOMP_DropRain_MakeSound(gGT);
	MenuHighlight();

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	RenderAllWeather(gGT);
#endif
#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	RenderAllConfetti(gGT);
#endif
#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	// NOTE(aalhendi): ASM-verified NTSC-U 926 subrange 0x800364f8-0x80036538.
	if ((gGT->renderFlags & 8) != 0 && gGT->stars.numStars != 0)
		RenderStars(&gGT->pushBuffer[0], &gGT->backBuffer->primMem, &gGT->stars, gGT->numPlyrCurrGame);
#endif

	RenderAllHUD(gGT);

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	RenderAllBeakerRain(gGT);

// DEAD CODE
#if 0
	RenderAllBoxSceneSplitLines(gGT);
#endif
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	// NOTE(aalhendi): ctr-native routes instances through the retail
	// RenderBucket queue/execute contract. The TEST_DrawInstances fallback below
	// is reserved for non-CTR_NATIVE rebuild paths until they are cleaned up.
	RenderBucket_QueueAllInstances(gGT);
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	RenderAllNormalParticles(gGT);
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	RenderDispEnv_World(gGT); // == RenderDispEnv_World ==
#endif

#ifndef REBUILD_PS1
	RenderAllFlag0x40(gGT); // I need a better name
	RenderAllTitleDPP(gGT);
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	RenderBucket_ExecuteAllInstances(gGT);
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	RenderAllTires(gGT);
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	RenderAllShadows(gGT);
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
	RenderAllHeatParticles(gGT);

#elif !defined(CTR_NATIVE)

	// TODO(aalhendi): Remove this legacy test renderer after non-CTR_NATIVE
	// rebuild paths are routed through RenderBucket too.
	// PC port version of ExecuteAllInstances
	if ((gGT->renderFlags & 0x20) != 0)
	{
		RenderDispEnv_World(gGT); // == RenderDispEnv_World ==

		void TEST_DrawInstances(struct GameTracker * gGT);
		TEST_DrawInstances(gGT);
	}
#endif

	DECOMP_PushBuffer_FadeAllWindows();

	if ((gGT->renderFlags & 1) != 0)
	{
#ifdef CTR_INTERNAL
		if (gCtrDebugSkipLevelGeometry == 0)
#endif
		{
#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)

			RenderAllLevelGeometry(gGT);

#else

			for (int i = 0; i < gGT->numPlyrCurrGame; i++)
			{
				// 226-229
				// placeholder for DrawLevelOvr1P
				TEST_226(0, &gGT->pushBuffer[i], gGT->level1->ptr_mesh_info, &gGT->backBuffer->primMem, gGT->visMem1->visFaceList[i],
				         0); // waterEnvMap?

#ifdef CTR_NATIVE
				DrawSky_Full(gGT->level1->ptr_skybox, &gGT->pushBuffer[i], &gGT->backBuffer->primMem);

				if (((gGT->level1->configFlags & 1) != 0) || (gGT->numPlyrCurrGame > 1))
				{
					CAM_SkyboxGlow((s16 *)&gGT->level1->glowGradient[0], &gGT->pushBuffer[i], &gGT->backBuffer->primMem, &gGT->pushBuffer[i].ptrOT[0x3ff]);
				}
#else
				// placeholder for DrawSky_Full
				TEST_DrawSkybox(gGT->level1->ptr_skybox, &gGT->pushBuffer[i], &gGT->backBuffer->primMem);
#endif
			}

#endif
		}

		RenderDispEnv_World(gGT); // == RenderDispEnv_World ==

		MultiplayerWumpaHUD(gGT);

		if (
		    // if not cutscene
		    // if not in adventure arena
		    // if not in main menu
		    ((gGT->gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA | MAIN_MENU)) == 0) &&

		    // if loading is 100% finished
		    (sdata->Loading.stage != -4))
		{
			DECOMP_DotLights_AudioAndVideo(gGT);

			if ((gGT->renderFlags & 0x8000) != 0)
			{
				WindowBoxLines(gGT);

				WindowDivsionLines(gGT);
			}
		}

#if !defined(REBUILD_PS1)
		// if game is not loading
		if (sdata->Loading.stage == -1)
		{
			// If game is not paused
			if ((gGT->gameMode1 & PAUSE_ALL) == 0)
			{
				PickupBots_Update();
			}

			PlayLevel_UpdateLapStats();
		}
#elif defined(CTR_NATIVE)
		// TODO(aalhendi): Retail also updates PickupBots here. Keep the native
		// PlayLevel path live first; port PickupBots_Update before enabling it.
		if (sdata->Loading.stage == -1)
		{
			PlayLevel_UpdateLapStats();
		}
#endif
	}

#ifndef REBUILD_PS1
	// If in main menu, or in adventure arena,
	// or in End-Of-Race menu
	if ((gGT->gameMode1 & (ADVENTURE_ARENA | END_OF_RACE | MAIN_MENU)) != 0)
	{
		RefreshCard_Entry();
	}
#endif

	// clear swapchain
	if (((gGT->renderFlags & 0x2000) != 0) && ((lev->clearColor[0].enable != 0) || (lev->clearColor[1].enable != 0)))
	{
		DECOMP_CAM_ClearScreen(gGT);
	}

	if ((gGT->renderFlags & 0x1000) != 0)
	{
		DECOMP_RaceFlag_DrawSelf();
	}

	RenderDispEnv_UI(gGT);

	RenderVSYNC(gGT);

#ifndef REBUILD_PS1
	RenderFMV();
#endif

	RenderSubmit(gGT);
}

void DrawUnpluggedMsg(struct GameTracker *gGT, struct GamepadSystem *gGamepads)
{
	int posY;
	int lngArrStart;
	RECT window;
	int i;

	// dont draw error if demo mode, or cutscene,
	// or if no controllers are missing currently
	if (gGT->boolDemoMode == 1)
		return;
	if ((gGT->gameMode1 & GAME_CUTSCENE) != 0)
		return;

#ifndef REBUILD_PS1
	if (DECOMP_MainFrame_HaveAllPads(gGT->numPlyrNextGame) == 1)
		return;
#else
	// assume all connected on PC
	return;
#endif

	// if main menu is open, assume 230 loaded,
	// quit if menu is at highest level (no ptrNext to draw)
	if (sdata->ptrActiveMenu == (struct RectMenu *)0x800B4540) // maybe a member of D230.c?
		if ((*(int *)0x800b4548 & 0x10) == 0)
			return;

	// position of error
	posY = data.errorPosY[sdata->errorMessagePosIndex];

	// "Controller 1" or "Controller 2"
	lngArrStart = 0;

	window.x = 0xffec;
	window.y = posY - 3;
	window.w = 0x228;
	window.h = 0;

	// if more than 2 players, or if multitap used
	if ((gGT->numPlyrNextGame > 2) || (gGamepads->slotBuffer[0].controllerData == (PAD_ID_MULTITAP << 4)))
	{
		// change to "1A", "1B", "1C", "1D",
		lngArrStart = 2;
	}

	for (i = 0; i < gGT->numPlyrNextGame; i++)
	{
		struct ControllerPacket *ptrControllerPacket = gGamepads->gamepad[i].ptrControllerPacket;

		if (ptrControllerPacket != 0)
			if (ptrControllerPacket->plugged == PLUGGED)
				continue;

		// if controller is unplugged

		DECOMP_DecalFont_DrawLine(sdata->lngStrings[data.lngIndex_gamepadUnplugged[lngArrStart + i]], 0x100, posY + window.h, FONT_SMALL,
		                          (JUSTIFY_CENTER | ORANGE));

		// add for each line
		window.h += 8;
	}

	// PLEASE CONNECT A CONTROLLER
	DECOMP_DecalFont_DrawLine(sdata->lngStrings[0xac / 4], 0x100, posY + window.h, FONT_SMALL, (JUSTIFY_CENTER | ORANGE));

	// add for each line
	window.h += 8;

	// add 3 pixels above, 3 pixels bellow
	window.h += 6;

	DECOMP_RECTMENU_DrawInnerRect(&window, 1, gGT->backBuffer->otMem.startPlusFour);
}

void DrawFinalLap(struct GameTracker *gGT)
{
	int i;
	int textTimer;
	struct PushBuffer *pb;

	int startX;
	int endX;
	int posY;

	s16 resultPos[2];

	// number of players
	for (i = 0; i < 4; i++)
	{
		// time remaining in animation
		textTimer = sdata->finalLapTextTimer[i];

		// skip if not drawing "FINAL LAP"
		if (textTimer == 0)
			continue;

		// turn "time remaining" into "time elapsed",
		// 90 frames total in animation, 1.5 seconds
		textTimer = 90 - textTimer;

		// camera
		pb = &gGT->pushBuffer[i];

		// << 0x10, >> 0x12
		posY = pb->rect.h / 4;

		// fly from right to center
		if (textTimer <= 10)
		{
			startX = pb->rect.w + 100;
			endX = pb->rect.w / 2;

			goto DrawFinalLapString;
		}

		// sit in center
		if (textTimer <= 0x50)
		{
			startX = pb->rect.w / 2;
			endX = startX;

			// for duration
			textTimer -= 10;

			goto DrawFinalLapString;
		}

		// fly from center to left
		startX = pb->rect.w / 2;
		endX = -100;
		textTimer -= 0x50;

	DrawFinalLapString:

		DECOMP_UI_Lerp2D_Linear(&resultPos[0], (s16)startX, (s16)posY, (s16)endX, (s16)posY, textTimer, 10);

		// need to specify OT, or else "FINAL LAP" will draw on top of character icons,
		// and by doing this, "FINAL LAP" draws under the character icons instead
		DECOMP_DecalFont_DrawLineOT(sdata->lngStrings[0x8cc / 4], resultPos[0], resultPos[1], FONT_BIG, (JUSTIFY_CENTER | ORANGE), pb->ptrOT);

		sdata->finalLapTextTimer[i]--;
	}
}

void RainLogic(struct GameTracker *gGT)
{
	int i;
	struct QuadBlock *camQB;
	int numPlyrCurrGame;

	numPlyrCurrGame = gGT->numPlyrCurrGame;

	for (i = 0; i < numPlyrCurrGame; i++)
	{
#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
		// NOTE(aalhendi): Native defines REBUILD_PS1 too, but 226 render lists need pb->bbox.
		PushBuffer_UpdateFrustum(&gGT->pushBuffer[i]);
#else
		// temporary until PushBuffer_UpdateFrustum is done
		DECOMP_PushBuffer_SetMatrixVP(&gGT->pushBuffer[i]);
#endif

		camQB = gGT->cameraDC[i].ptrQuadBlock;

		// skip if camera isn't over quadblock
		if (camQB == 0)
			continue;

		// assume numPlayers is never zero,
		// assume weather_intensity is always valid

		gGT->rainBuffer[i].numParticles_max = (camQB->weather_intensity << 2) / numPlyrCurrGame;

		gGT->rainBuffer[i].vanishRate = (camQB->weather_vanishRate << 2) / numPlyrCurrGame;
	}
}

void MenuHighlight()
{
	int fc;
	int trig;

	fc = sdata->frameCounter << 7;
	trig = DECOMP_MATH_Sin(fc);

	trig = (trig << 6) >> 0xc;

	// sine curve of green, plus base color
	sdata->menuRowHighlight_Normal.self = ((trig + 0x40) * 0x100) | 0x80;
	sdata->menuRowHighlight_Green.self = ((trig + 0xA0) * 0x100) | 0x400040;
}

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderAllWeather(struct GameTracker *gGT)
{
	int numPlyrCurrGame = gGT->numPlyrCurrGame;

	// only for single player,
	// probably Naughty Dog's last-minute hack
	if (numPlyrCurrGame != 1)
		return;

	// only if rain is enabled
	if ((gGT->renderFlags & 2) == 0)
		return;

	RenderWeather(&gGT->pushBuffer[0], &gGT->backBuffer->primMem, &gGT->rainBuffer[0], numPlyrCurrGame, gGT->gameMode1 & PAUSE_ALL);
}
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderAllConfetti(struct GameTracker *gGT)
{
	int i;
	int numWinners = gGT->numWinners;

	// only if someone needs confetti
	if (numWinners == 0)
		return;

	// only if confetti is enabled
	if ((gGT->renderFlags & 4) == 0)
		return;

	for (i = 0; i < numWinners; i++)
	{
		DrawConfetti(&gGT->pushBuffer[gGT->winnerIndex[i]], &gGT->backBuffer->primMem, &gGT->confetti, gGT->frameTimer_Confetti, gGT->gameMode1 & PAUSE_ALL);
	}
}

#endif

void RenderAllHUD(struct GameTracker *gGT)
{
	int hudFlags;
	int gameMode1;

	hudFlags = gGT->hudFlags;
	gameMode1 = gGT->gameMode1;

// Why is this needed? What's broken
// that causes this to run premature?
#ifdef REBUILD_PS1
	// LOADING... and pause screen (see adv pause)
	if ((gGT->gameMode1 & 0x4000000f) != 0)
		return;

	// before level is done loading
	if (gGT->level1 == 0)
		return;
#endif

	// if drawing intro-race title bars
	if ((gGT->numPlyrCurrGame == 1) && ((hudFlags & 8) != 0) && ((gameMode1 & START_OF_RACE) != 0))
	{
#ifndef REBUILD_PS1
		UI_RaceStart_IntroText1P();
#endif
	}

	// if not drawing intro-race title bars
	{
		// if no hud
		if ((hudFlags & 1) == 0)
		{
#ifndef REBUILD_PS1
			// if standings
			if ((hudFlags & 4) != 0)
			{
				UI_CupStandings_InputAndDraw();
			}
#endif
		}

		// if hud
		else
		{
			// if not adv hub
			if ((gameMode1 & ADVENTURE_ARENA) == 0)
			{
				// if not drawing end of race
				if (
				    // end of race is not reached
				    ((gameMode1 & END_OF_RACE) == 0) ||

				    // cooldown after end of race not expired
				    (gGT->timerEndOfRaceVS != 0))
				{
					// not crystal challenge
					if ((gameMode1 & CRYSTAL_CHALLENGE) == 0)
					{
						DECOMP_UI_RenderFrame_Racing();
					}

					// if crystal challenge
					else
					{
						DECOMP_UI_RenderFrame_CrystChall();
					}
				}

				// drawing end of race
				else
				{
					if (
					    // VS mode, and Cup
					    ((gGT->gameMode1 & ARCADE_MODE) == 0) && ((gGT->gameMode2 & CUP_ANY_KIND) != 0))
					{
						// disable drawing hud,
						// enable drawing "standings"
						gGT->hudFlags = (hudFlags & 0xfe) | 4;
						return;
					}

#if defined(CTR_NATIVE)
					// NOTE(aalhendi): Preserve the original OVR_Region1 overlay-entry contract with a dispatcher.
					OVR_Region1();
#elif !defined(REBUILD_PS1)
					// temporary, until we rewrite MainGameEnd_Initialize
					if ((gGT->gameMode1 & RELIC_RACE) == 0)
					{
						// all 221-225 overlays share the same
						// function address, so call as one func
						void OVR_Region1();
						OVR_Region1();
					}

					// except relic, until we rewrite MainGameEnd_Initialize
					else
					{
						void DECOMP_RR_EndEvent_DrawMenu();
						DECOMP_RR_EndEvent_DrawMenu();
					}
#endif

					return;
				}
			}

			// if adv hub
			else
			{
				// load on last frame of waiting to load 232,
				// leave transition at 1 (see later in func),
				// and load the 232 overlay
				if (gGT->overlayTransition > 1)
				{
#ifndef REBUILD_PS1
					gGT->overlayTransition--;
					if (gGT->overlayTransition == 1)
						DECOMP_LOAD_OvrThreads(2);
#endif
				}

				// if 233 is still loaded
				if (DECOMP_LOAD_IsOpen_AdvHub() == 0)
				{
					// if any transition is over
					if (gGT->pushBuffer_UI.fadeFromBlack_currentValue > 0xfff)
					{
						DECOMP_UI_RenderFrame_AdvHub();
					}
				}

				// if 232 overlay is loaded
				else
				{
					// if any transition is over
					if (gGT->pushBuffer_UI.fadeFromBlack_currentValue > 0xfff)
					{
						DECOMP_AH_Map_Main();

						if (sdata->AkuHint_RequestedHint != -1)
						{
							DECOMP_AH_MaskHint_Start(sdata->AkuHint_RequestedHint, sdata->AkuHint_boolInterruptWarppad);

							// erase submitted request
							sdata->AkuHint_RequestedHint = -1;
							sdata->AkuHint_boolInterruptWarppad = 0;
						}
					}

					// if first frame of transition to 232
					if (gGT->overlayTransition != 0)
					{
						gGT->overlayTransition = 0;

#ifndef REBUILD_PS1
						INSTANCE_LevDelayedLInBs(gGT->level1->ptrInstDefs, gGT->level1->numInstances);
#endif

						// allow instances again
						gGT->gameMode2 &= ~(NO_LEV_INSTANCE);

						// fade transition
						gGT->pushBuffer_UI.fadeFromBlack_desiredResult = 0x1000;
						gGT->pushBuffer_UI.fade_step = 0x2aa;
					}
				}
			}
		}
	}
}

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderAllBeakerRain(struct GameTracker *gGT)
{
	int numPlyrCurrGame = gGT->numPlyrCurrGame;

	// only for 1P/2P
	if (numPlyrCurrGame > 2)
		return;

	// only if beaker rain is enabled
	if ((gGT->renderFlags & 0x10) == 0)
		return;

	RedBeaker_RenderRain(&gGT->pushBuffer[0], &gGT->backBuffer->primMem, &gGT->JitPools.rain, numPlyrCurrGame, gGT->gameMode1 & PAUSE_ALL);
}

// DEAD CODE,
// vertSplit is set to zero from INSTANCE_Birth,
// and the value set in this function is zero anyway
#if 0
void RenderAllBoxSceneSplitLines(struct GameTracker* gGT)
{
	// check 233 overlay, cause levelID is set
	// and MainFrame_RenderFrame runs, before 233 loads
	if(DECOMP_LOAD_IsOpen_Podiums() != 0)
	{
		// ND Box Scene
		if(gGT->levelID == NAUGHTY_DOG_CRATE)
		{
			CS_BoxScene_InstanceSplitLines();
		}
	}
}
#endif

#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderBucket_QueueAllInstances(struct GameTracker *gGT)
{
	int lod;
	int *RBI;
	int numPlyrCurrGame = gGT->numPlyrCurrGame;

	if ((gGT->renderFlags & 0x20) == 0)
		return;

	lod = numPlyrCurrGame - 1;
	if ((gGT->gameMode1 & RELIC_RACE) != 0)
		lod |= 4;

	RBI = RenderBucket_QueueLevInstances(&gGT->cameraDC[0], (u_long *)&gGT->backBuffer->otMem, gGT->ptrRenderBucketInstance,
	                                     (char *)(u32)(u8)sdata->LOD[lod], // this weird cast is what ghidra does
	                                     (char)numPlyrCurrGame, gGT->gameMode1 & PAUSE_ALL);

	RBI = RenderBucket_QueueNonLevInstances(gGT->JitPools.instance.taken.first, (u_long *)&gGT->backBuffer->otMem, (void *)RBI,
	                                        (char *)(u32)(u8)sdata->LOD[lod], // this weird cast is what ghidra does
	                                        (char)numPlyrCurrGame, gGT->gameMode1 & PAUSE_ALL);

	// Aug prototype
#if 0
		// ptrEnd of otmem is less than ptrCurr otmem
    if (*(uint *)(*(int *)(PTR_DAT_8008d2ac + 0x10) + 0x98) <
        *(uint *)(*(int *)(PTR_DAT_8008d2ac + 0x10) + 0x9c)) {
      printf("OTMEM OVERFLOW!\n");
    }
#endif

	// null terminator at end of list
	*RBI = 0;
}
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderAllNormalParticles(struct GameTracker *gGT)
{
	int i;

	if ((gGT->renderFlags & 0x200) == 0)
		return;

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		Particle_RenderList(&gGT->pushBuffer[i], gGT->particleList_ordinary);
	}
}
#endif

void RenderDispEnv_World(struct GameTracker *gGT)
{
	int i;
	struct PushBuffer *pb;
	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		pb = &gGT->pushBuffer[i];
		DECOMP_PushBuffer_SetDrawEnv_Normal(&pb->ptrOT[0x3ff], pb, gGT->backBuffer, 0, 0);
	}
}

#ifndef REBUILD_PS1
// I need a better name
void RenderAllFlag0x40(struct GameTracker *gGT)
{
	if ((gGT->renderFlags & 0x40) == 0)
		return;

	if (DECOMP_LOAD_IsOpen_RacingOrBattle() != 0)
	{
		RB_Player_ToggleInvisible();
		RB_Player_ToggleFlicker();
		RB_Burst_ProcessBucket(gGT->threadBuckets[BURST].thread);
		RB_Blowup_ProcessBucket(gGT->threadBuckets[BLOWUP].thread);

		DECOMP_RB_Spider_DrawWebs(gGT->threadBuckets[SPIDER].thread, &gGT->pushBuffer[0]);
		RB_Follower_ProcessBucket(gGT->threadBuckets[FOLLOWER].thread);
		RB_StartText_ProcessBucket(gGT->threadBuckets[STARTTEXT].thread);
	}

	if (DECOMP_LOAD_IsOpen_AdvHub() != 0)
	{
		if ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)
		{
			DECOMP_AH_WarpPad_AllWarppadNum();
		}
	}

	VehTurbo_ProcessBucket(gGT->threadBuckets[TURBO].thread);

	int i;
	struct PushBuffer *pb;
	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		pb = &gGT->pushBuffer[i];
		VehGroundSkids_Main(gGT->threadBuckets[PLAYER].thread, pb);
		VehGroundSkids_Main(gGT->threadBuckets[ROBOT].thread, pb);
	}
}

void RenderAllTitleDPP(struct GameTracker *gGT)
{
	if ((gGT->gameMode1 & MAIN_MENU) == 0)
		return;
	if (DECOMP_LOAD_IsOpen_MainMenu() == 0)
		return;
	DECOMP_MM_Title_SetTrophyDPP();
}
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderBucket_ExecuteAllInstances(struct GameTracker *gGT)
{
	if ((gGT->renderFlags & 0x20) == 0)
		return;

	RenderBucket_Execute(gGT->ptrRenderBucketInstance, &gGT->backBuffer->primMem);
}
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderAllTires(struct GameTracker *gGT)
{
	int i;
	struct Thread *th;
	int numPlyrCurrGame;
	struct PrimMem *gGT_primMem;
	if ((gGT->renderFlags & 0x80) == 0)
		return;

	// replace checking number of AIs, with
	// checking if the threadBucket exists,
	// then roll this up into a loop

	gGT_primMem = &gGT->backBuffer->primMem;
	numPlyrCurrGame = gGT->numPlyrCurrGame;

	// player, robot, ghost
	for (i = 0; i < 3; i++)
	{
		th = gGT->threadBuckets[i].thread;
		if (th == 0)
			continue;

		DrawTires_Solid(th, gGT_primMem, numPlyrCurrGame);
		DrawTires_Reflection(th, gGT_primMem, numPlyrCurrGame);
	}
}
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderAllShadows(struct GameTracker *gGT)
{
	if ((gGT->renderFlags & 0x200) == 0)
		return;
	VehGroundShadow_Main();
}
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderAllHeatParticles(struct GameTracker *gGT)
{
	if ((gGT->renderFlags & 0x800) == 0)
		return;

	Torch_Main(gGT->particleList_heatWarp, &gGT->pushBuffer[0], &gGT->backBuffer->primMem, gGT->numPlyrCurrGame, gGT->swapchainIndex * 0x128);
}
#endif

#if !defined(REBUILD_PS1) || defined(CTR_NATIVE)
void RenderAllLevelGeometry(struct GameTracker *gGT)
{
	int i;
	int distToScreen;
	int numPlyrCurrGame;
	struct Level *level1;
	struct PushBuffer *pushBuffer;
	struct mesh_info *ptr_mesh_info;

	level1 = gGT->level1;
	if (level1 == 0)
		return;

	ptr_mesh_info = level1->ptr_mesh_info;
	if (ptr_mesh_info == 0)
		return;

	numPlyrCurrGame = gGT->numPlyrCurrGame;

	if (numPlyrCurrGame == 1)
	{
		CTR_ClearRenderLists_1P2P(gGT, 1);

		// === Temporary 60FPS macros ===
		// Emulate 30fps on 60fps for SCVert and OVert

		// if no SCVert
		if ((level1->configFlags & 4) == 0)
		{
			// assume OVert (no primitives generated here)
			AnimateWater1P(gGT->timer, level1->numWaterVertices, level1->ptr_water, level1->ptr_tex_waterEnvMap, gGT->visMem1->visOVertList[0]);
		}

		// if SCVert
		else
		{
			// draw SCVert (no primitives generated here
			AnimateQuad(gGT->timer << 7, level1->numSCVert, level1->ptrSCVert, gGT->visMem1->visSCVertList[0]);
		}

		// camera of player 1
		pushBuffer = &gGT->pushBuffer[0];

		if (
		    // adv character selection screen
		    (gGT->levelID == ADVENTURE_GARAGE) ||

		    // cutscene that's not Crash Bandicoot intro
		    // where he's sleeping and snoring on a hill
		    (((gGT->gameMode1 & GAME_CUTSCENE) != 0) && (gGT->levelID != INTRO_CRASH)))
		{
			// relationship between near-clip and far-clip,
			// for each RenderList LOD set in the level
			*(int *)0x1f800014 = 0x1e00;
			*(int *)0x1f800018 = 0x640;
			*(int *)0x1f80001c = 0x640;
			*(int *)0x1f800020 = 0x500;
			*(int *)0x1f800024 = 0x280;
			*(int *)0x1f800028 = 0x140;
			*(int *)0x1f80002c = 0x640 + 0x140;
		}

		// every non-cutscene,
		// except for Crash Bandicoot intro
		else
		{
			// 0x1c2 in 1P mode
			distToScreen = pushBuffer->distanceToScreen_PREV;

			// int and u32 have specific purposes
			*(u32 *)0x1f800014 = distToScreen * 0x2080;
			if (*(int *)0x1f800014 < 0)
				*(int *)0x1f800014 = *(int *)0x1f800014 + 0xff;
			*(int *)0x1f800014 = *(int *)0x1f800014 >> 8; // 0x3921

			*(int *)0x1f800018 = distToScreen * 0x1a;        // 0x2DB4
			*(int *)0x1f80001c = distToScreen * 0x18;        // 0x2A30
			*(int *)0x1f800020 = distToScreen * 0xc;         // 0x1518
			*(int *)0x1f800024 = distToScreen * 7;           // 0xC4E
			*(int *)0x1f80002c = *(int *)0x1f800018 + 0x140; // 0x2EF4

			// int and u32 have specific purposes
			*(u32 *)0x1f800028 = distToScreen * 0x380;
			if (*(int *)0x1f800028 < 0)
				*(int *)0x1f800028 = *(int *)0x1f800028 + 0xff;
			*(int *)0x1f800028 = *(int *)0x1f800028 >> 8; // 0x627
		}

		RenderLists_PreInit();
		gGT->bspLeafsDrawn = 0;

		gGT->bspLeafsDrawn += RenderLists_Init1P2P(ptr_mesh_info->bspRoot, level1->visMem->visLeafList[0], pushBuffer, (u32)&gGT->LevRenderLists[0],
		                                           level1->visMem->bspList[0], (char)numPlyrCurrGame);

		// 226-229
		DrawLevelOvr1P(&gGT->LevRenderLists[0], pushBuffer, (struct BSP *)ptr_mesh_info, &gGT->backBuffer->primMem, gGT->visMem1->visFaceList[0],
		               level1->ptr_tex_waterEnvMap); // waterEnvMap?

		DrawSky_Full(level1->ptr_skybox, pushBuffer, &gGT->backBuffer->primMem);

		// skybox gradient
		if ((level1->configFlags & 1) != 0)
		{
			goto SkyboxGlow;
		}

		return;
	}

	if (numPlyrCurrGame == 2)
	{
		CTR_ClearRenderLists_1P2P(gGT, 2);

		// if no SCVert
		if ((level1->configFlags & 4) == 0)
		{
			// assume OVert (no primitives generated here)
			AnimateWater2P(gGT->timer, level1->numWaterVertices, level1->ptr_water, level1->ptr_tex_waterEnvMap, gGT->visMem1->visOVertList[0],
			               gGT->visMem1->visOVertList[1]);
		}

		RenderLists_PreInit();
		gGT->bspLeafsDrawn = 0;

		for (i = 0; i < numPlyrCurrGame; i++)
		{
			gGT->bspLeafsDrawn += RenderLists_Init1P2P(ptr_mesh_info->bspRoot, level1->visMem->visLeafList[i], &gGT->pushBuffer[i],
			                                           (u32)&gGT->LevRenderLists[i], level1->visMem->bspList[i], (char)numPlyrCurrGame);
		}

		// 226-229
		DrawLevelOvr2P(&gGT->LevRenderLists[0], &gGT->pushBuffer[0], (struct BSP *)ptr_mesh_info, &gGT->backBuffer->primMem, gGT->visMem1->visFaceList[0],
		               gGT->visMem1->visFaceList[1],
		               level1->ptr_tex_waterEnvMap); // waterEnvMap?

		goto SkyboxGlow;
	}

	// 3P or 4P
	CTR_ClearRenderLists_3P4P(gGT, numPlyrCurrGame);

	// if no SCVert
	if ((level1->configFlags & 4) == 0)
	{
		if (numPlyrCurrGame == 3)
		{
			// assume OVert (no primitives generated here)
			AnimateWater3P(gGT->timer, level1->numWaterVertices, level1->ptr_water, level1->ptr_tex_waterEnvMap, gGT->visMem1->visOVertList[0],
			               gGT->visMem1->visOVertList[1], gGT->visMem1->visOVertList[2]);
		}

		else // 4P mode
		{
			// assume OVert (no primitives generated here)
			AnimateWater4P(gGT->timer, level1->numWaterVertices, level1->ptr_water, level1->ptr_tex_waterEnvMap, gGT->visMem1->visOVertList[0],
			               gGT->visMem1->visOVertList[1], gGT->visMem1->visOVertList[2], gGT->visMem1->visOVertList[3]);
		}
	}

	RenderLists_PreInit();
	gGT->bspLeafsDrawn = 0;

	for (i = 0; i < numPlyrCurrGame; i++)
	{
		gGT->bspLeafsDrawn += RenderLists_Init3P4P(ptr_mesh_info->bspRoot, level1->visMem->visLeafList[i], &gGT->pushBuffer[i], (u32)&gGT->LevRenderLists[i],
		                                           level1->visMem->bspList[i]);
	}

	if (numPlyrCurrGame == 3)
	{
		// 226-229
		DrawLevelOvr3P(&gGT->LevRenderLists[0], &gGT->pushBuffer[0], (struct BSP *)ptr_mesh_info, &gGT->backBuffer->primMem, gGT->visMem1->visFaceList[0],
		               gGT->visMem1->visFaceList[1], gGT->visMem1->visFaceList[2],
		               level1->ptr_tex_waterEnvMap); // waterEnvMap?
	}

	else // 4P mode
	{
		// 226-229
		DrawLevelOvr4P(&gGT->LevRenderLists[0], &gGT->pushBuffer[0], (struct BSP *)ptr_mesh_info, &gGT->backBuffer->primMem, gGT->visMem1->visFaceList[0],
		               gGT->visMem1->visFaceList[1], gGT->visMem1->visFaceList[2], gGT->visMem1->visFaceList[3],
		               level1->ptr_tex_waterEnvMap); // waterEnvMap?
	}

SkyboxGlow:

	// skybox gradient
	for (i = 0; i < numPlyrCurrGame; i++)
	{
		pushBuffer = &gGT->pushBuffer[i];
		CAM_SkyboxGlow((s16 *)&level1->glowGradient[0], pushBuffer, &gGT->backBuffer->primMem, &pushBuffer->ptrOT[0x3ff]);
	}

	return;
}

#endif // !defined(REBUILD_PS1) || defined(CTR_NATIVE)

void MultiplayerWumpaHUD(struct GameTracker *gGT)
{
	if ((gGT->hudFlags & 1) == 0)
		return;

	// Remove manually at end-of-race
	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		struct Driver *d = gGT->drivers[i];

		// if race is over for driver
		if ((d->actionsFlagSet & 0x2000000) != 0)
		{
			struct Instance *instFruitDisp = d->instFruitDisp;

			instFruitDisp->scale[0] = 0;
			instFruitDisp->scale[1] = 0;
			instFruitDisp->scale[2] = 0;
		}
	}
}

void WindowBoxLines(struct GameTracker *gGT)
{
	int i;

	// only battle and 3P4P mode allowed
	if ((gGT->gameMode1 & BATTLE_MODE) == 0)
		return;
	if (gGT->numPlyrCurrGame < 3)
		return;

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		Color color;
		color.self = *data.ptrColor[gGT->drivers[i]->BattleHUD.teamID + PLAYER_BLUE];
		DECOMP_RECTMENU_DrawOuterRect_LowLevel(

		    // dimensions, thickness
		    &gGT->pushBuffer[i].rect, 4, 2,

		    // color data
		    color,

		    0,

		    // pushBuffer_UI = 0x1388
		    &gGT->pushBuffer_UI.ptrOT[3]);
	}
}

void WindowDivsionLines(struct GameTracker *gGT)
{
	POLY_F4 *p;
	int numPlyrCurrGame;

	numPlyrCurrGame = gGT->numPlyrCurrGame;

	// horizontal bar
	if (numPlyrCurrGame > 1)
	{
		p = gGT->backBuffer->primMem.curr;

		// set R, G, B, CODE, all to zero,
		// this makes black color, and invalid CODE
		*(int *)&p->r0 = 0;

		// this sets CODE to the proper value
		setPolyF4(p);

		// Make four (x,y) coordinates
		p->y0 = 0x6a;
		p->y1 = 0x6a;
		p->x0 = 0;
		p->x1 = 0x200;

		p->x2 = 0;
		p->y2 = 0x6e;
		p->x3 = 0x200;
		p->y3 = 0x6e;

		// Draw a bar from left to right,
		// dividing the screen in half on top and bottom
		AddPrim(&gGT->pushBuffer_UI.ptrOT[3], p);

		gGT->backBuffer->primMem.curr = (void *)(p + 1);
	}

	// vertical bar
	if (numPlyrCurrGame > 2)
	{
#if 0
		gGT->drivers[0]->numWumpas = 10;
		gGT->drivers[0]->heldItemID = 3;
		gGT->drivers[1]->numWumpas = 10;
		gGT->drivers[1]->heldItemID = 3;
#endif

		p = gGT->backBuffer->primMem.curr;

		// set R, G, B, CODE, all to zero,
		// this makes black color, and invalid CODE
		*(int *)&p->r0 = 0;

		// this sets CODE to the proper value
		setPolyF4(p);

		// Make four (x,y) coordinates
		p->x0 = 0xfd;
		p->x2 = 0xfd;
		p->y0 = 0;
		p->x1 = 0x103;

		p->y1 = 0;
		p->y2 = 0xd8;
		p->x3 = 0x103;
		p->y3 = 0xd8;

		// Draw a bar from left to right,
		// dividing the screen in half on top and bottom
		AddPrim(&gGT->pushBuffer_UI.ptrOT[3], p);

		// backBuffer->primMem.curr
		gGT->backBuffer->primMem.curr = (void *)(p + 1);
	}

	// if numPlyrCurrGame is 3
	if (numPlyrCurrGame == '\x03')
	{
		// This is useless, cause it gets cleared
		// to black anyway, even without this block,
		// at least it does it Crash Cove, does it always?

		p = gGT->backBuffer->primMem.curr;

		// set R, G, B, CODE, all to zero,
		// this makes black color, and invalid CODE
		*(int *)&p->r0 = 0;

		// this sets CODE to the proper value
		setPolyF4(p);

		// xy0
		p->x0 = 0x100;
		p->x2 = 0x100;
		p->y0 = 0x6c;
		p->y1 = 0x6c;
		p->x1 = 0x200;
		p->y2 = 0xd8;
		p->x3 = 0x200;
		p->y3 = 0xd8;

		// Draw a bar from left to right,
		// dividing the screen in half on top and bottom
		AddPrim(&gGT->pushBuffer_UI.ptrOT[3], p);

		// backBuffer->primMem.curr
		gGT->backBuffer->primMem.curr = (void *)(p + 1);
	}
}

void RenderDispEnv_UI(struct GameTracker *gGT)
{
	struct PushBuffer *pb = &gGT->pushBuffer_UI;

	DECOMP_PushBuffer_SetDrawEnv_Normal(&pb->ptrOT[4], pb, gGT->backBuffer, 0, 0);
}

__attribute__((optimize("O0"))) int ReadyToFlip(struct GameTracker *gGT)
{
	return
	    // two VSYNCs passed, 30fps lock
	    (sdata->vsyncTillFlip < 1) &&

	    // if DrawOTag finished
	    (gGT->bool_DrawOTag_InProgress == 0);
}

__attribute__((optimize("O0"))) int ReadyToBreak(struct GameTracker *gGT)
{
	return

	    // if more than 6 VSYNCs passed since
	    // the last successful draw, FPS < 10fps
	    gGT->vSync_between_drawSync > 6;
}

void RenderVSYNC(struct GameTracker *gGT)
{
	// render checkered flag
	if ((gGT->renderFlags & 0x1000) != 0)
	{
		// Wait until "next" vsync,
		// Main Menu at 45fps will cap to 30fps
		// Levels+RaceFlag at 25fps will cap to 20fps
		VSync(0);
	}


	while (1)
	{
#ifdef REBUILD_PC
		// must be called in the loop,
		// or else it wont properly sync
		DrawSync(0);
#endif

		if (ReadyToFlip(gGT))
		{
			// quit, end of stall
			return;
		}

#ifndef REBUILD_PC
		if (ReadyToBreak(gGT))
		{
			// just quit and try the next frame
			BreakDraw();
			return;
		}
#endif
	}
}

#ifndef REBUILD_PS1
void RenderFMV()
{
	if (sdata->boolPlayVideoSTR == 1)
	{
		MM_Video_CheckIfFinished(1);

		MoveImage(&sdata->videoSTR_src_vramRect, sdata->videoSTR_dst_vramX, sdata->videoSTR_dst_vramY);

		DrawSync(0);
	}
}
#endif

void RenderSubmit(struct GameTracker *gGT)
{
	// 1 VSYNC = 60fps
	// 2 VSYNCs = 30fps

#ifdef REBUILD_PC

	sdata->vsyncTillFlip = 2;
#else

	// do I need the "if"? will it ever be nullptr?
	if (gGT->frontBuffer != 0)
	{
		sdata->vsyncTillFlip = 2;

		// skip debug stuff

		PutDispEnv(&gGT->frontBuffer->dispEnv);
		PutDrawEnv(&gGT->frontBuffer->drawEnv);
	}

	// swap=0, get db[1]
	// swap=1, get db[0]
	gGT->frontBuffer = &gGT->db[1 - gGT->swapchainIndex];
#endif

	gGT->bool_DrawOTag_InProgress = 1;

	void *ot = &gGT->pushBuffer[0].ptrOT[0x3ff];

	DrawOTag(ot);

	gGT->frameTimer_notPaused = gGT->frameTimer_VsyncCallback;
}
