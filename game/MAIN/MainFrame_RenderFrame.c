#include <common.h>
#include <ctr_trig.h>

#ifndef MAIN_FRAME_ACTIVE_MENU
#define MAIN_FRAME_ACTIVE_MENU (sdata->ptrActiveMenu)
#endif

#ifndef MAIN_FRAME_PULSE_COUNTER
#define MAIN_FRAME_PULSE_COUNTER ((s16)GAME_FRAME_COUNTER_LOW)
#define MAIN_FRAME_PULSE_NORMAL  (sdata->menuRowHighlight_Normal)
#define MAIN_FRAME_PULSE_GREEN   (sdata->menuRowHighlight_Green)
#define MAIN_FRAME_BORDER_COLORS (data.ptrColor)
#endif

#ifndef MAIN_FRAME_ERROR_POS_Y
#define MAIN_FRAME_ERROR_POS_Y       data.errorPosY
#define MAIN_FRAME_UNPLUGGED_INDICES data.lngIndex_gamepadUnplugged
#define MAIN_FRAME_FINAL_LAP_TIMERS  sdata->finalLapTextTimer
#define MAIN_FRAME_LOD               (sdata->LOD)
#endif

#ifndef MAIN_FRAME_VIDEO_SOURCE_RECT
#define MAIN_FRAME_VIDEO_SOURCE_RECT     (sdata->videoSTR_src_vramRect)
#define MAIN_FRAME_DEBUG_DISPLAY_ENABLED (sdata->boolDebugDispEnv)
#define MAIN_FRAME_BLANK_DISPLAY_ENV     (sdata->blank_debug_DispEnv)
#endif

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
#include <platform/native_perf.h>
#define MAINFRAME_PERF_BEGIN(bucket) NativePerf_BeginScope(bucket)
#define MAINFRAME_PERF_END(bucket)   NativePerf_EndScope(bucket)
#else
#define MAINFRAME_PERF_BEGIN(bucket) ((void)0)
#define MAINFRAME_PERF_END(bucket)   ((void)0)
#endif

#ifdef CTR_INTERNAL
volatile int gCtrDebugSkipLevelGeometry = 0;
#endif

static __inline__ void RainLogic(struct GameTracker *gGT)
{
	s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$17");
	struct QuadBlock *camQB;

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		PushBuffer_UpdateFrustum(&gGT->pushBuffer[i]);

		camQB = gGT->cameraDC[i].ptrQuadBlock;

		// skip if camera isn't over quadblock
		if (camQB == 0)
		{
			continue;
		}

		// assume numPlayers is never zero,
		// assume weather_intensity is always valid

		gGT->rainBuffer[i].numParticles_max = (camQB->weather_intensity << 2) / gGT->numPlyrCurrGame;

		gGT->rainBuffer[i].vanishRate = (gGT->cameraDC[i].ptrQuadBlock->weather_vanishRate << 2) / gGT->numPlyrCurrGame;
	}
}


static __inline__ void MenuHighlight(void)
{
	s32 fc;
	s32 counterWord;
	s32 trig;

	counterWord = (s32)MAIN_FRAME_PULSE_COUNTER;
	fc = counterWord * 128;
	// NOTE(aalhendi): Keep the raw tick and scaled angle in distinct operands.
	CTR_PSX_DEPEND_VALUE(fc, counterWord);
	trig = CTR_SinFromTable(data.trigApprox, fc);

	trig = CTR_MipsSra(CTR_MipsSll(trig, 6), 12);

	// sine curve of green, plus base color
	ColorCode_SetPacked(&MAIN_FRAME_PULSE_NORMAL, ((trig + 0x40) * 0x100) | 0x80);
	ColorCode_SetPacked(&MAIN_FRAME_PULSE_GREEN, ((trig + 0xA0) * 0x100) | 0x400040);
}

static __inline__ void RenderAllWeather(struct GameTracker *gGT)
{
	u32 numPlyrCurrGame;

	// only if rain is enabled
	if ((gGT->renderFlags & RENDER_FLAG_RAIN) == 0)
	{
		return;
	}

	CTR_PSX_LOAD_UNSIGNED_BYTE(numPlyrCurrGame, GAME_TRACKER, offsetof(struct GameTracker, numPlyrCurrGame), GAME_TRACKER->numPlyrCurrGame);

	// only for single player,
	// probably Naughty Dog's last-minute hack
	if (numPlyrCurrGame > 1)
	{
		return;
	}

	RenderWeather(&GAME_TRACKER->pushBuffer[0], &GAME_TRACKER->backBuffer->primMem, &GAME_TRACKER->rainBuffer[0], GAME_TRACKER->numPlyrCurrGame,
	              GAME_TRACKER->gameMode1 & PAUSE_ALL);
}

static __inline__ void RenderAllConfetti(struct GameTracker *gGT)
{
	s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$17");

	// only if confetti is enabled
	if ((gGT->renderFlags & RENDER_FLAG_CONFETTI) == 0)
	{
		return;
	}

	for (i = 0; i < (s32)GAME_TRACKER->numWinners; i++)
	{
		DrawConfetti(&GAME_TRACKER->pushBuffer[GAME_TRACKER->winnerIndex[i]], &GAME_TRACKER->backBuffer->primMem, &GAME_TRACKER->confetti,
		             GAME_TRACKER->frameTimer_Confetti, GAME_TRACKER->gameMode1 & PAUSE_ALL);
	}
}

static __inline__ void RenderAllHUD(struct GameTracker *gGT)
{
	s32 introHudFlags;
	s32 hudFlags;
	s32 gameMode1;

	introHudFlags = GAME_TRACKER->hudFlags;

	// Draw the intro title bars only for a single-player race start.
	if (((introHudFlags & HUD_FLAG_INTRO_RACE_TITLE_BARS) != 0) && (GAME_TRACKER->numPlyrCurrGame == 1) && ((GAME_TRACKER->gameMode1 & START_OF_RACE) != 0))
	{
		UI_RaceStart_IntroText1P();
		return;
	}

	hudFlags = GAME_TRACKER->hudFlags;
	if ((hudFlags & HUD_FLAG_RACE_HUD) == 0)
	{
		goto DrawCupStandings;
	}

	gameMode1 = GAME_TRACKER->gameMode1;
	if ((gameMode1 & ADVENTURE_ARENA) != 0)
	{
		// Load 232 on the last waiting frame, keeping transition at 1.
		if (GAME_TRACKER->overlayTransition > 1)
		{
			GAME_TRACKER->overlayTransition--;
			if (GAME_TRACKER->overlayTransition == 1)
			{
				LOAD_OvrThreads(2);
			}
		}

		if (LOAD_IsOpen_AdvHub())
		{
			// Draw the map/hint only after the fade transition.
			if (GAME_TRACKER->pushBuffer_UI.fadeFromBlack_currentValue > 0xfff)
			{
				AH_Map_Main();

				{
					s16 requestedHint = sdata->AkuHint_RequestedHint;
					if (requestedHint != -1)
					{
						AH_MaskHint_Start(requestedHint, sdata->AkuHint_boolInterruptWarppad);

						// Erase the submitted request.
						sdata->AkuHint_RequestedHint = -1;
						sdata->AkuHint_boolInterruptWarppad = 0;
					}
				}
			}

			// First frame with the 232 overlay loaded.
			if (GAME_TRACKER->overlayTransition != 0)
			{
				INSTANCE_LevDelayedLInBs(gGT->level1->ptrInstDefs, gGT->level1->numInstances);

				{
					struct GameTracker *transitionTracker = GAME_TRACKER;
					s32 gameMode2 = transitionTracker->gameMode2;
					struct GameTracker *fadeTracker;

					transitionTracker->overlayTransition = 0;
					fadeTracker = GAME_TRACKER;

					// Allow instances again and begin the fade.
					transitionTracker->gameMode2 = gameMode2 & ~NO_LEV_INSTANCE;
					fadeTracker->pushBuffer_UI.fadeFromBlack_desiredResult = 0x1000;
					fadeTracker->pushBuffer_UI.fade_step = 0x2aa;
				}
			}
			return;
		}

		// 233 is still loaded: use its hub UI after the fade.
		if (GAME_TRACKER->pushBuffer_UI.fadeFromBlack_currentValue > 0xfff)
		{
			UI_RenderFrame_AdvHub();
		}
		return;
	}

	// Continue drawing the race until the end-of-race cooldown expires.
	if (((gameMode1 & END_OF_RACE) == 0) || (GAME_TRACKER->timerEndOfRaceVS != 0))
	{
		if ((gameMode1 & CRYSTAL_CHALLENGE) != 0)
		{
			UI_RenderFrame_CrystChall();
		}
		else
		{
			UI_RenderFrame_Racing();
		}
		return;
	}

	// End menus exclude only these two loading stages.
	if ((u32)(sdata->Loading.stage + 5) <= 1)
	{
		return;
	}

	if ((gameMode1 & CRYSTAL_CHALLENGE) != 0)
	{
		CC_EndEvent_DrawMenu();
		return;
	}
	if ((gameMode1 & TIME_TRIAL) != 0)
	{
		TT_EndEvent_DrawMenu();
		return;
	}
	if ((gameMode1 & ARCADE_MODE) != 0)
	{
		goto DrawAdventureEndEvent;
	}
	if ((gameMode1 & RELIC_RACE) != 0)
	{
		RR_EndEvent_DrawMenu();
		return;
	}
	if ((gameMode1 & ADVENTURE_MODE) == 0)
	{
		goto DrawMultiplayerEndEvent;
	}

DrawAdventureEndEvent:
	// Arcade and adventure races share this actual end-menu path.
	AA_EndEvent_DrawMenu();
	return;

DrawMultiplayerEndEvent:
	if ((GAME_TRACKER->gameMode2 & CUP_ANY_KIND) != 0)
	{
		GAME_TRACKER->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;
		GAME_TRACKER_RELOAD()->hudFlags |= HUD_FLAG_CUP_STANDINGS;
	}
	else
	{
		VB_EndEvent_DrawMenu();
	}
	return;

DrawCupStandings:
	if ((hudFlags & HUD_FLAG_CUP_STANDINGS) != 0)
	{
		UI_CupStandings_InputAndDraw();
	}
}

static __inline__ void RenderAllBeakerRain(struct GameTracker *gGT)
{
	u32 numPlyrCurrGame;

	// only if beaker rain is enabled
	if ((gGT->renderFlags & RENDER_FLAG_BEAKER_RAIN) == 0)
	{
		return;
	}

	CTR_PSX_LOAD_UNSIGNED_BYTE(numPlyrCurrGame, GAME_TRACKER, offsetof(struct GameTracker, numPlyrCurrGame), GAME_TRACKER->numPlyrCurrGame);

	// only for 1P/2P
	if (numPlyrCurrGame > 2)
	{
		return;
	}

	RedBeaker_RenderRain(&gGT->pushBuffer[0], &gGT->backBuffer->primMem, &GAME_TRACKER->JitPools.rain, GAME_TRACKER->numPlyrCurrGame,
	                     GAME_TRACKER->gameMode1 & PAUSE_ALL);
}

static __inline__ void RenderAllBoxSceneSplitLines(struct GameTracker *gGT)
{
	(void)gGT;

	// Check 233 overlay, cause levelID is set and MainFrame_RenderFrame runs before 233 loads.
	if (LOAD_IsOpen_Podiums())
	{
		if (GAME_TRACKER->levelID == NAUGHTY_DOG_CRATE)
		{
			CS_BoxScene_InstanceSplitLines();
		}
	}
}

static __inline__ void RenderBucket_QueueAllInstances(struct GameTracker *gGT)
{
	u32 *RBI;
	s32 levPlayerCount;
	s32 numPlyrCurrGame;
	size_t lod;

	if ((gGT->renderFlags & RENDER_FLAG_RENDER_BUCKET) == 0)
	{
		return;
	}

	levPlayerCount = GAME_TRACKER->numPlyrCurrGame;
	lod = (size_t)levPlayerCount - 1;

	RBI = RenderBucket_QueueLevInstances(&gGT->cameraDC[0], &GAME_TRACKER->backBuffer->otMem, GAME_TRACKER->ptrRenderBucketInstance,
	                                     (u8)MAIN_FRAME_LOD[(lod = (GAME_TRACKER->gameMode1 & RELIC_RACE) ? (lod | 4) : lod)], levPlayerCount,
	                                     GAME_TRACKER->gameMode1 & PAUSE_ALL);

	numPlyrCurrGame = GAME_TRACKER->numPlyrCurrGame;
	lod = (size_t)numPlyrCurrGame - 1;

	RBI = RenderBucket_QueueNonLevInstances(GAME_TRACKER->JitPools.instance.taken.first, &GAME_TRACKER->backBuffer->otMem, (void *)RBI,
	                                        (u8)MAIN_FRAME_LOD[(GAME_TRACKER->gameMode1 & RELIC_RACE) ? (lod | 4) : lod], numPlyrCurrGame,
	                                        GAME_TRACKER->gameMode1 & PAUSE_ALL);

	// null terminator at end of list
	*RBI = 0;
}

static __inline__ void RenderAllNormalParticles(struct GameTracker *gGT)
{
	s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$17");

	if ((gGT->renderFlags & RENDER_FLAG_PARTICLES) == 0)
	{
		return;
	}

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		Particle_RenderList(&gGT->pushBuffer[i], GAME_TRACKER->particleList_ordinary);
	}
}

static __inline__ void RenderDispEnv_World(struct GameTracker *gGT)
{
	s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$17");
	s32 playerCount;
	s16 *overrideRect;
	struct PushBuffer *pb;
	u32 pbOffset CTR_PSX_GLOBAL_REGISTER_HINT("$18");

	i = 0;
	if (gGT->numPlyrCurrGame == 0)
	{
		return;
	}

	pbOffset = offsetof(struct GameTracker, pushBuffer);
	do
	{
		pb = (struct PushBuffer *)((u8 *)gGT + pbOffset);
		// NOTE(aalhendi): Prepare the NULL clip override before advancing the cursor.
		CTR_PSX_ZERO_VALUE_AFTER(overrideRect, pb, pbOffset);
		pbOffset += sizeof(gGT->pushBuffer[0]);
		PushBuffer_SetDrawEnv_Normal(&gGT->pushBuffer[i].ptrOT[0x3ff], pb, gGT->backBuffer, overrideRect, 0);
		playerCount = gGT->numPlyrCurrGame;
		// NOTE(aalhendi): Retain the fresh count before advancing the player index.
		CTR_PSX_DEPEND_VALUE(playerCount, i);
		i++;
	} while (i < playerCount);
}

static __inline__ void RenderAllEffectBuckets(struct GameTracker *gGT)
{
	s32 i;
	struct PushBuffer *pb;

	if ((gGT->renderFlags & RENDER_FLAG_EFFECT_BUCKETS) == 0)
	{
		return;
	}

	if (LOAD_IsOpen_RacingOrBattle())
	{
		RB_Player_ToggleInvisible();
		RB_Player_ToggleFlicker();
		RB_Burst_ProcessBucket(GAME_TRACKER->threadBuckets[BURST].thread);
		RB_Blowup_ProcessBucket(GAME_TRACKER->threadBuckets[BLOWUP].thread);

		RB_Spider_DrawWebs(GAME_TRACKER->threadBuckets[SPIDER].thread, &gGT->pushBuffer[0]);
		RB_Follower_ProcessBucket(GAME_TRACKER->threadBuckets[FOLLOWER].thread);
		RB_StartText_ProcessBucket(GAME_TRACKER->threadBuckets[STARTTEXT].thread);
	}

	if (LOAD_IsOpen_AdvHub())
	{
		if ((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) != 0)
		{
			AH_WarpPad_AllWarppadNum();
		}
	}

	VehTurbo_ProcessBucket(GAME_TRACKER->threadBuckets[TURBO].thread);

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		pb = &gGT->pushBuffer[i];
		VehGroundSkids_Main(GAME_TRACKER->threadBuckets[PLAYER].thread, pb);
		VehGroundSkids_Main(GAME_TRACKER->threadBuckets[ROBOT].thread, pb);
	}
}

static __inline__ void RenderAllTitleDPP(struct GameTracker *gGT)
{
	(void)gGT;

	if ((GAME_TRACKER->gameMode1 & MAIN_MENU) == 0)
	{
		return;
	}
	if (!LOAD_IsOpen_MainMenu())
	{
		return;
	}
	MM_Title_SetTrophyDPP();
}

static __inline__ void RenderBucket_ExecuteAllInstances(struct GameTracker *gGT)
{
	if ((gGT->renderFlags & RENDER_FLAG_RENDER_BUCKET) == 0)
	{
		return;
	}

	RenderBucket_Execute(GAME_TRACKER->ptrRenderBucketInstance, &GAME_TRACKER->backBuffer->primMem);
}

static __inline__ void RenderAllTires(struct GameTracker *gGT)
{
	if ((gGT->renderFlags & RENDER_FLAG_TIRES) == 0)
	{
		return;
	}

	if (GAME_TRACKER->threadBuckets[PLAYER].thread != 0)
	{
		DrawTires_Solid(GAME_TRACKER->threadBuckets[PLAYER].thread, &GAME_TRACKER->backBuffer->primMem, gGT->numPlyrCurrGame);
		DrawTires_Reflection(GAME_TRACKER->threadBuckets[PLAYER].thread, &GAME_TRACKER->backBuffer->primMem, gGT->numPlyrCurrGame);
	}

	if (GAME_TRACKER->numBotsNextGame != 0)
	{
		DrawTires_Solid(GAME_TRACKER->threadBuckets[ROBOT].thread, &GAME_TRACKER->backBuffer->primMem, gGT->numPlyrCurrGame);
		DrawTires_Reflection(GAME_TRACKER->threadBuckets[ROBOT].thread, &GAME_TRACKER->backBuffer->primMem, gGT->numPlyrCurrGame);
	}

	if (GAME_TRACKER->threadBuckets[GHOST].thread != 0)
	{
		DrawTires_Solid(GAME_TRACKER->threadBuckets[GHOST].thread, &GAME_TRACKER->backBuffer->primMem, gGT->numPlyrCurrGame);
		DrawTires_Reflection(GAME_TRACKER->threadBuckets[GHOST].thread, &GAME_TRACKER->backBuffer->primMem, gGT->numPlyrCurrGame);
	}
}

static __inline__ void RenderAllShadows(struct GameTracker *gGT)
{
	if ((gGT->renderFlags & RENDER_FLAG_SHADOWS) == 0)
	{
		return;
	}
	VehGroundShadow_Main();
}

static __inline__ void RenderAllHeatParticles(struct GameTracker *gGT)
{
	if ((gGT->renderFlags & RENDER_FLAG_HEAT_EFFECT) == 0)
	{
		return;
	}

	Torch_Main(GAME_TRACKER->particleList_heatWarp, &gGT->pushBuffer[0], &GAME_TRACKER->backBuffer->primMem, gGT->numPlyrCurrGame,
	           GAME_TRACKER->swapchainIndex * 0x128);
}

static __inline__ s32 RenderAllLevelGeometry_Div256(s32 product)
{
	if (product < 0)
	{
		product = CTR_MipsAddLo(product, 0xff);
	}

	return CTR_MipsSra(product, 8);
}

static __inline__ void MainFrame_RenderSkyboxGlow(struct GameTracker *gGT, struct Level *level1, s32 numPlyrCurrGame)
{
	s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$17");
	struct PushBuffer *pushBuffer;

	for (i = 0; i < numPlyrCurrGame; i++)
	{
		pushBuffer = &gGT->pushBuffer[i];
		CAM_SkyboxGlow(&level1->glowGradient[0], pushBuffer, &gGT->backBuffer->primMem, &gGT->pushBuffer[i].ptrOT[0x3ff]);
	}
}

static __inline__ void MainFrame_RenderGeometry3P4PSetup(struct GameTracker *gGT, struct Level *level1, struct mesh_info *ptr_mesh_info, s32 numPlyrCurrGame)
{
	s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$17");

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
		gGT->bspLeafsDrawn += RenderLists_Init3P4P(ptr_mesh_info->bspRoot, gGT->visMem1->visLeafList[i], &gGT->pushBuffer[i], (u32)&gGT->LevRenderLists[i],
		                                           gGT->visMem1->bspList[i]);
	}
}

static __inline__ void RenderAllLevelGeometry(struct GameTracker *gGT, struct Level *level1, struct mesh_info *ptr_mesh_info)
{
	s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$17");
	s32 distToScreen;
	s32 numPlyrCurrGame;
	struct MainRenderLevelGeometryScratch *scratch;
	struct PushBuffer *pushBuffer;

#ifdef CTR_NATIVE
	if (level1 == 0)
	{
		return;
	}
#endif

	if (ptr_mesh_info == 0)
	{
		return;
	}

	numPlyrCurrGame = GAME_TRACKER->numPlyrCurrGame;

	switch (numPlyrCurrGame)
	{
	case 1:
	{
		CTR_ClearRenderLists_1P2P(gGT, 1);

		// === Temporary 60FPS macros ===
		// Emulate 30fps on 60fps for SCVert and OVert

		// if no SCVert
		if (((level1->configFlags & 4) == 0) && (level1 != 0))
		{
			// assume OVert (no primitives generated here)
			AnimateWater1P(gGT->timer, level1->numWaterVertices, level1->ptr_water, level1->ptr_tex_waterEnvMap, gGT->visMem1->visOVertList[0]);
		}

		// if SCVert
		else
		{
			// draw SCVert (no primitives generated here
			AnimateQuad(CTR_MipsSll(gGT->timer, 7), level1->numSCVert, level1->ptrSCVert, gGT->visMem1->visSCVertList[0]);
		}

		// camera of player 1
		scratch = CTR_SCRATCHPAD_PTR(struct MainRenderLevelGeometryScratch, MAIN_RENDER_LEVEL_GEOMETRY_SCRATCH_OFFSET);
		distToScreen = GAME_TRACKER->pushBuffer[0].distanceToScreen_PREV;

		if (
		    // adv character selection screen
		    (GAME_TRACKER->levelID == ADVENTURE_GARAGE) ||

		    // cutscene that's not Crash Bandicoot intro
		    // where he's sleeping and snoring on a hill
		    (((GAME_TRACKER->gameMode1 & GAME_CUTSCENE) != 0) && (GAME_TRACKER->levelID != INTRO_CRASH)))
		{
			// relationship between near-clip and far-clip,
			// for each RenderList LOD set in the level
			scratch->depthScale = 0x1e00;
			scratch->bspLodDistanceThreshold = 0x640;
			scratch->textureLodDepthThreshold0 = 0x640;
			scratch->textureLodDepthThreshold1 = 0x500;
			scratch->topLevelNearDepthThreshold = 0x280;
			scratch->recursiveNearDepthThreshold = 0x140;
			scratch->fullDynamicFadeDepthStart = scratch->bspLodDistanceThreshold + MAIN_RENDER_LEVEL_GEOMETRY_FULL_DYNAMIC_FADE_OFFSET;
		}

		// every non-cutscene,
		// except for Crash Bandicoot intro
		else
		{
			// 0x1c2 in 1P mode
			scratch->depthScale = RenderAllLevelGeometry_Div256((s32)((u32)distToScreen * 0x2080u));
			scratch->bspLodDistanceThreshold = (s32)((u32)distToScreen * 26u);
			scratch->textureLodDepthThreshold0 = (s32)((u32)distToScreen * 24u);
			scratch->textureLodDepthThreshold1 = (s32)((u32)distToScreen * 12u);
			scratch->topLevelNearDepthThreshold = (s32)((u32)distToScreen * 7u);
			scratch->recursiveNearDepthThreshold = RenderAllLevelGeometry_Div256((s32)((u32)distToScreen * 0x380u));
			scratch->fullDynamicFadeDepthStart = CTR_MipsAddLo(scratch->bspLodDistanceThreshold, MAIN_RENDER_LEVEL_GEOMETRY_FULL_DYNAMIC_FADE_OFFSET);
		}

		RenderLists_PreInit();
		pushBuffer = &gGT->pushBuffer[0];

		gGT->bspLeafsDrawn = RenderLists_Init1P2P(ptr_mesh_info->bspRoot, gGT->visMem1->visLeafList[0], pushBuffer, (u32)&gGT->LevRenderLists[0],
		                                          gGT->visMem1->bspList[0], gGT->numPlyrCurrGame);

		// 226-229
		DrawLevelOvr1P(&gGT->LevRenderLists[0], pushBuffer, (struct BSP *)level1->ptr_mesh_info, &gGT->backBuffer->primMem, gGT->visMem1->visFaceList[0],
		               level1->ptr_tex_waterEnvMap); // waterEnvMap?

		DrawSky_Full(level1->ptr_skybox, pushBuffer, &gGT->backBuffer->primMem);

		// skybox gradient
		if ((level1->configFlags & 1) != 0)
		{
			CAM_SkyboxGlow(&level1->glowGradient[0], pushBuffer, &gGT->backBuffer->primMem, &gGT->pushBuffer[0].ptrOT[0x3ff]);
		}

		return;
	}

	case 2:
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

		for (i = 0; i < 2; i++)
		{
			gGT->bspLeafsDrawn += RenderLists_Init1P2P(ptr_mesh_info->bspRoot, gGT->visMem1->visLeafList[i], &gGT->pushBuffer[i], (u32)&gGT->LevRenderLists[i],
			                                           gGT->visMem1->bspList[i], gGT->numPlyrCurrGame);
		}

		// 226-229
		DrawLevelOvr2P(&gGT->LevRenderLists[0], &gGT->pushBuffer[0], (struct BSP *)ptr_mesh_info, &gGT->backBuffer->primMem, gGT->visMem1->visFaceList[0],
		               gGT->visMem1->visFaceList[1],
		               level1->ptr_tex_waterEnvMap); // waterEnvMap?

		MainFrame_RenderSkyboxGlow(gGT, level1, 2);
		return;
	}

	case 3:
	{
		MainFrame_RenderGeometry3P4PSetup(gGT, level1, ptr_mesh_info, 3);

		// 226-229
		DrawLevelOvr3P(&gGT->LevRenderLists[0], &gGT->pushBuffer[0], (struct BSP *)ptr_mesh_info, &gGT->backBuffer->primMem, gGT->visMem1->visFaceList[0],
		               gGT->visMem1->visFaceList[1], gGT->visMem1->visFaceList[2], level1->ptr_tex_waterEnvMap);

		MainFrame_RenderSkyboxGlow(gGT, level1, 3);
		return;
	}

	case 4:
	{
		MainFrame_RenderGeometry3P4PSetup(gGT, level1, ptr_mesh_info, 4);

		// 226-229
		DrawLevelOvr4P(&gGT->LevRenderLists[0], &gGT->pushBuffer[0], (struct BSP *)ptr_mesh_info, &gGT->backBuffer->primMem, gGT->visMem1->visFaceList[0],
		               gGT->visMem1->visFaceList[1], gGT->visMem1->visFaceList[2], gGT->visMem1->visFaceList[3], level1->ptr_tex_waterEnvMap);

		MainFrame_RenderSkyboxGlow(gGT, level1, 4);
	}
	}
}

static __inline__ void MainFrame_WindowBoxLines(struct GameTracker *gGT, RECT *rect)
{
	s32 i;

	(void)gGT;

	// only battle and 3P4P mode allowed
	if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) == 0)
	{
		return;
	}
	if (GAME_TRACKER->numPlyrCurrGame < 3)
	{
		return;
	}

	for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
#ifdef CTR_NATIVE
		Color nativeColor;
#endif

		rect->x = GAME_TRACKER->pushBuffer[i].rect.x;
		rect->y = GAME_TRACKER->pushBuffer[i].rect.y;
		rect->w = GAME_TRACKER->pushBuffer[i].rect.w;
		rect->h = GAME_TRACKER->pushBuffer[i].rect.h;
#ifdef CTR_NATIVE
		ColorCode_SetPacked(&nativeColor, *MAIN_FRAME_BORDER_COLORS[GAME_TRACKER->drivers[i]->BattleHUD.teamID + PLAYER_BLUE]);
#endif
		RECTMENU_DrawOuterRect_LowLevel(

		    // dimensions, thickness
		    rect, 4, 2,

		// color data
#ifdef CTR_NATIVE
		    &nativeColor,
#else
		    (const Color *)MAIN_FRAME_BORDER_COLORS[GAME_TRACKER->drivers[i]->BattleHUD.teamID + PLAYER_BLUE],
#endif

		    0,

		    // pushBuffer_UI = 0x1388
		    &GAME_TRACKER->pushBuffer_UI.ptrOT[3]);
	}
}

static __inline__ void WindowDivisionLines(struct GameTracker *gGT)
{
	POLY_F4 *p;

	(void)gGT;

	// horizontal bar
	if (GAME_TRACKER->numPlyrCurrGame > 1)
	{
		p = GAME_TRACKER->backBuffer->primMem.cursor;

		// Initialize the black flat-shaded divider.
		setPolyF4(p);
		p->r0 = p->g0 = p->b0 = 0;

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
		AddPrim(&GAME_TRACKER->pushBuffer_UI.ptrOT[3], p);

		GAME_TRACKER->backBuffer->primMem.cursor = (u8 *)GAME_TRACKER->backBuffer->primMem.cursor + sizeof(*p);
	}

	// vertical bar
	if (GAME_TRACKER->numPlyrCurrGame > 2)
	{
		p = GAME_TRACKER->backBuffer->primMem.cursor;

		// Initialize the black flat-shaded divider.
		setPolyF4(p);
		p->r0 = p->g0 = p->b0 = 0;

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
		AddPrim(&GAME_TRACKER->pushBuffer_UI.ptrOT[3], p);

		// backBuffer->primMem.cursor
		GAME_TRACKER->backBuffer->primMem.cursor = (u8 *)GAME_TRACKER->backBuffer->primMem.cursor + sizeof(*p);
	}

	// if numPlyrCurrGame is 3
	if (GAME_TRACKER->numPlyrCurrGame == '\x03')
	{
		// This is useless, cause it gets cleared
		// to black anyway, even without this block,
		// at least it does it Crash Cove, does it always?

		p = GAME_TRACKER->backBuffer->primMem.cursor;

		// Initialize the black flat-shaded divider.
		setPolyF4(p);
		p->r0 = p->g0 = p->b0 = 0;

		// xy0
		p->x0 = 0x100;
		p->y0 = 0x6c;
		p->x1 = 0x200;
		p->y1 = 0x6c;
		p->x2 = 0x100;
		p->y2 = 0xd8;
		p->x3 = 0x200;
		p->y3 = 0xd8;

		// Draw a bar from left to right,
		// dividing the screen in half on top and bottom
		AddPrim(&GAME_TRACKER->pushBuffer_UI.ptrOT[3], p);

		// backBuffer->primMem.cursor
		GAME_TRACKER->backBuffer->primMem.cursor = (u8 *)GAME_TRACKER->backBuffer->primMem.cursor + sizeof(*p);
	}
}

static __inline__ void RenderDispEnv_UI(struct GameTracker *gGT)
{
	struct PushBuffer *pb = &gGT->pushBuffer_UI;

	PushBuffer_SetDrawEnv_Normal(&gGT->pushBuffer_UI.ptrOT[4], pb, gGT->backBuffer, 0, 0);
}

static __inline__ void RenderVSYNC(struct GameTracker *gGT)
{
	gGT->clockDurationStall = Timer_GetTime_Total();

	// render checkered flag
	if ((GAME_TRACKER->renderFlags & RENDER_FLAG_CHECKERED_FLAG) != 0)
	{
		// Wait until "next" vsync,
		// Main Menu at 45fps will cap to 30fps
		// Levels+RaceFlag at 25fps will cap to 20fps
		VSync(0);
	}


#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native VSync delivers VBlank; poll before testing readiness.
	DrawSync(0);
#endif

	while ((GAME_TRACKER->bool_DrawOTag_InProgress != 0) || (sdata->vsyncTillFlip > 0))
	{
#ifdef CTR_NATIVE
		VSync(0);
		DrawSync(0);
#else
		if ((u32)GAME_TRACKER->vSync_between_drawSync > 6)
		{
			BreakDraw();
			return;
		}
#endif
	}
}

#ifndef CTR_NATIVE
static __inline__ void RenderFMV(void)
{
	if (sdata->boolPlayVideoSTR == 1)
	{
		MM_Video_CheckIfFinished(1);

		MoveImage(&MAIN_FRAME_VIDEO_SOURCE_RECT, MAIN_FRAME_VIDEO_DST_X, MAIN_FRAME_VIDEO_DST_Y);

		DrawSync(0);
	}
}
#endif

static __inline__ void RenderSubmit(struct GameTracker *gGT)
{
	void *ot;

	// 1 VSYNC = 60fps
	// 2 VSYNCs = 30fps

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
	NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDER_SUBMIT);
#endif

	gGT->clockDurationStall = Timer_GetTime_Elapsed(gGT->clockDurationStall, 0);

#if defined(CTR_NATIVE)

	sdata->vsyncTillFlip = 2;

	// Native still renders immediately through PsyCross, so keep the host GPU's
	// active draw/display envs in step with the retail DB selected this frame.
	PutDrawEnv(&gGT->backBuffer->drawEnv);
	gGT->frontBuffer = &gGT->db[1] - gGT->swapchainIndex;
	PutDispEnv(&gGT->frontBuffer->dispEnv);

#else

	// Publish each completed front buffer once, then clear its pending slot.
	if (gGT->frontBuffer != 0)
	{
		s32 debugFlags CTR_PSX_LOCAL_REGISTER_HINT("$3");
		s32 displayClock CTR_PSX_LOCAL_REGISTER_HINT("$5");

		debugFlags = MAIN_FRAME_DEBUG_DISPLAY_ENABLED;
		sdata->vsyncTillFlip = 2;
		displayClock = GAME_TRACKER->unk1cc4[0];
		GAME_TRACKER->unk1cc4[5] = displayClock;

		if ((debugFlags & 1) != 0)
			PutDispEnv(&MAIN_FRAME_BLANK_DISPLAY_ENV);
		else
			PutDispEnv(&gGT->frontBuffer->dispEnv);
		PutDrawEnv(&gGT->frontBuffer->drawEnv);
		gGT->frontBuffer = 0;
	}

	// swap=0, get db[1]
	// swap=1, get db[0]
	gGT->frontBuffer = &gGT->db[1] - gGT->swapchainIndex;
#endif

	GAME_TRACKER->bool_DrawOTag_InProgress = 1;

	ot = &gGT->pushBuffer[0].ptrOT[0x3ff];

	DrawOTag(ot);

	gGT->frameTimer_notPaused = gGT->frameTimer_VsyncCallback;

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
	NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDER_SUBMIT);
#endif
}

void MainFrame_RenderFrame(struct GameTracker *gGT, struct GamepadSystem *gGamepads)
{
	struct Level *lev = gGT->level1;
	struct mesh_info *ptr_mesh_info = 0;
	RECT controllerWindow;
	SVec2 finalLapPosition;
	RECT battleWindow;

	(void)gGamepads;

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_SETUP);
	{
		s32 posY CTR_PSX_GLOBAL_REGISTER_HINT("$16");
		s32 windowHeight;
		s16 lngArrStart;
		b8 skipMainMenuTopLevel;
		s32 i;
		struct RectMenu *activeMenu;

		skipMainMenuTopLevel = false;
		if (LOAD_IsOpen_MainMenu())
		{
			activeMenu = MAIN_FRAME_ACTIVE_MENU;
			if (activeMenu == (&D230.menuMainMenu))
			{
				u32 menuHierarchy = activeMenu->state & 0x10;

				skipMainMenuTopLevel = menuHierarchy == 0;
			}
		}

		if ((GAME_TRACKER->gameMode1 & GAME_CUTSCENE) != 0)
			goto DrawFinalLap;
		if ((b16)MainFrame_HaveAllPads(GAME_TRACKER->numPlyrNextGame))
			goto DrawFinalLap;
		if (GAME_TRACKER->boolDemoMode != 0)
			goto DrawFinalLap;
		if (skipMainMenuTopLevel != 0)
			goto DrawFinalLap;

		posY = (u16)MAIN_FRAME_ERROR_POS_Y[sdata->errorMessagePosIndex];
		windowHeight = 14;
		if ((GAMEPADS->slotBuffer[0].controller.controllerData == (PAD_ID_MULTITAP << 4)) || (GAME_TRACKER->numPlyrNextGame > 2))
		{
			lngArrStart = 2;
		}
		else
		{
			lngArrStart = 0;
		}

		for (i = 0; i < GAME_TRACKER->numPlyrNextGame; i++)
		{
			struct ControllerPacket *packet = GAMEPADS->gamepad[i].ptrControllerPacket;
			if ((packet == 0) || (packet->plugged != PLUGGED))
			{
				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[MAIN_FRAME_UNPLUGGED_INDICES[i + lngArrStart]], 0x100, (s16)posY, FONT_SMALL, JUSTIFY_CENTER | ORANGE);
				windowHeight += 8;
				posY += 8;
			}
		}

		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_PLEASE_INSERT_A_CONTROLLER], 0x100, (s16)posY, FONT_SMALL, JUSTIFY_CENTER | ORANGE);

		controllerWindow.x = -20;
		controllerWindow.y = (s16)((u16)MAIN_FRAME_ERROR_POS_Y[sdata->errorMessagePosIndex] - 3);
		controllerWindow.w = 552;
		controllerWindow.h = (s16)windowHeight;
		RECTMENU_DrawInnerRect(&controllerWindow, 1, GAME_TRACKER->backBuffer->otMem.uiOT);
	}
DrawFinalLap:
{
	s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$17");
	u16 timerBits;
	s32 textTimer;
	struct PushBuffer *pb;
	s32 startX;
	s32 endX;
	s32 posY;

	for (i = 0; i < 4; i++)
	{
		pb = &GAME_TRACKER->pushBuffer[i];
		if ((s16)MAIN_FRAME_FINAL_LAP_TIMERS[i] == 0)
			continue;

		timerBits = MAIN_FRAME_FINAL_LAP_TIMERS[i];
		textTimer = (s16)(90 - (s32)timerBits);

		if (textTimer > 80)
		{
			startX = CTR_MipsSra((s32)pb->rect.w, 1);
			posY = CTR_MipsSra((s32)pb->rect.h, 2);
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&finalLapPosition), (s16)startX, (s16)posY, -100, (s16)posY, textTimer - 80, 10);
		}
		else if (textTimer > 10)
		{
			startX = CTR_MipsSra((s32)pb->rect.w, 1);
			posY = CTR_MipsSra((s32)pb->rect.h, 2);
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&finalLapPosition), (s16)startX, (s16)posY, (s16)startX, (s16)posY, textTimer - 10, 10);
		}
		else
		{
			startX = (u16)pb->rect.w + 100;
			endX = CTR_MipsSra((s32)pb->rect.w, 1);
			posY = CTR_MipsSra((s32)pb->rect.h, 2);
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&finalLapPosition), (s16)startX, (s16)posY, (s16)endX, (s16)posY, textTimer, 10);
		}

		DecalFont_DrawLineOT(GAME_LANGUAGE_STRINGS[LNG_FINAL_LAP], finalLapPosition.x, finalLapPosition.y, FONT_BIG, JUSTIFY_CENTER | ORANGE,
		                     GAME_TRACKER->pushBuffer[i].ptrOT);
		MAIN_FRAME_FINAL_LAP_TIMERS[i]--;
	}
}

	ElimBG_HandleState(GAME_TRACKER);

	if (lev != 0)
	{
		ptr_mesh_info = lev->ptr_mesh_info;
	}

	if ((gGT->renderFlags & RENDER_FLAG_VISMEM_REFRESH_MASK) != 0)
	{
		MainFrame_VisMemFullFrame(gGT, lev);
	}


	if ((gGT->renderFlags & RENDER_FLAG_DRAW_LEVEL) != 0)
	{
		if (GAME_TRACKER->visMem1 != 0)
		{
			if (lev != 0)
			{
				CTR_CycleTex_LEV(lev->ptr_anim_tex, gGT->timer);
			}
		}
	}

	if ((MAIN_FRAME_ACTIVE_MENU != 0) || ((GAME_TRACKER->gameMode1 & END_OF_RACE) != 0))
	{
		RECTMENU_CollectInput();
	}

	if (MAIN_FRAME_ACTIVE_MENU != 0)
	{
		if (sdata->Loading.stage == LOAD_IDLE)
		{
			RECTMENU_ProcessState();
		}
	}

	RainLogic(gGT);
	DropRain_MakeSound(gGT);
	MenuHighlight();
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_SETUP);

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_EFFECTS);
	RenderAllWeather(gGT);
	RenderAllConfetti(gGT);
	if (((gGT->renderFlags & RENDER_FLAG_STARS) != 0) && (GAME_TRACKER->stars.numStars != 0))
	{
		RenderStars(&GAME_TRACKER->pushBuffer[0], &GAME_TRACKER->backBuffer->primMem, &GAME_TRACKER->stars, GAME_TRACKER->numPlyrCurrGame);
	}

	if (((gGT->renderFlags & RENDER_FLAG_MULTIPLAYER_DECALS) != 0) && (GAME_TRACKER->numPlyrCurrGame > 1))
	{
		DecalMP_01(gGT);
	}
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_EFFECTS);

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_HUD);
	RenderAllHUD(gGT);
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_HUD);

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_EFFECTS);
	RenderAllBeakerRain(gGT);

	RenderAllBoxSceneSplitLines(gGT);
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_EFFECTS);

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_QUEUE_INSTANCES);
	RenderBucket_QueueAllInstances(gGT);
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_QUEUE_INSTANCES);

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_EFFECTS);
	RenderAllNormalParticles(gGT);

	RenderDispEnv_World(gGT); // == RenderDispEnv_World ==

	if (((gGT->renderFlags & RENDER_FLAG_MULTIPLAYER_DECALS) != 0) && (GAME_TRACKER->numPlyrCurrGame > 1))
	{
		DecalMP_02(gGT);
	}

	RenderAllEffectBuckets(gGT);
	RenderAllTitleDPP(gGT);
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_EFFECTS);

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_EXECUTE_INSTANCES);
	RenderBucket_ExecuteAllInstances(gGT);
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_EXECUTE_INSTANCES);

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_EFFECTS);
	RenderAllTires(gGT);

	RenderAllShadows(gGT);

	RenderAllHeatParticles(gGT);

	PushBuffer_FadeAllWindows();
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_EFFECTS);

	if (((gGT->renderFlags & RENDER_FLAG_DRAW_LEVEL) != 0) && (ptr_mesh_info != 0))
	{
#ifdef CTR_INTERNAL
		if (gCtrDebugSkipLevelGeometry == 0)
#endif
		{
			MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_LEVEL_GEOMETRY);
			RenderAllLevelGeometry(gGT, lev, ptr_mesh_info);
			MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_LEVEL_GEOMETRY);
		}

		MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_POST_LEVEL);
		RenderDispEnv_World(gGT); // == RenderDispEnv_World ==

		if (((GAME_TRACKER->hudFlags & HUD_FLAG_RACE_HUD) != 0) && (GAME_TRACKER->numPlyrCurrGame > 1))
		{
			UI_RenderFrame_Wumpa3D_2P3P4P(gGT);
		}

		if (((gGT->renderFlags & RENDER_FLAG_MULTIPLAYER_DECALS) != 0) && (GAME_TRACKER->numPlyrCurrGame > 1))
		{
			DecalMP_03(gGT);
		}

		if (
		    // if not cutscene
		    // if not in adventure arena
		    // if not in main menu
		    ((gGT->gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA | MAIN_MENU)) == 0) && (sdata->Loading.stage != LOAD_REQUESTED))
		{
			DotLights_AudioAndVideo(gGT);
		}

		if ((GAME_TRACKER->renderFlags & RENDER_FLAG_SPLIT_SCREEN_LINES) != 0)
		{
			MainFrame_WindowBoxLines(gGT, &battleWindow);

			WindowDivisionLines(gGT);
		}

		// if game is not loading
		if (sdata->Loading.stage == LOAD_IDLE)
		{
			// If game is not paused
			if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
			{
				PickupBots_Update();
			}

#if defined(CTR_NATIVE)
			// NOTE(aalhendi): Native menu/adventure-hub LEVs may publish no
			// restart table. Retail lap stats assume the table exists whenever
			// this caller reaches them; keep the retail lap path intact.
			if ((gGT->level1 != NULL) && (gGT->level1->ptr_restart_points != NULL) && (gGT->level1->cnt_restart_points != 0))
			{
				PlayLevel_UpdateLapStats();
			}
#else
			PlayLevel_UpdateLapStats();
#endif
		}
		MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_POST_LEVEL);
	}

	// If in main menu, or in adventure arena,
	// or in End-Of-Race menu
	if ((GAME_TRACKER->gameMode1 & (ADVENTURE_ARENA | END_OF_RACE | MAIN_MENU)) != 0)
	{
		MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_REFRESHCARD);
		RefreshCard_Entry();
		MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_REFRESHCARD);
	}

	// clear swapchain
	if (((GAME_TRACKER->renderFlags & RENDER_FLAG_CLEAR_BACK_BUFFER) != 0) &&
	    ((gGT->level1->clearColor[0].enable != 0) || (gGT->level1->clearColor[1].enable != 0)))
	{
		MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_CLEAR_SCREEN);
		CAM_ClearScreen(gGT);
		MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_CLEAR_SCREEN);
	}

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_UI);
	if ((GAME_TRACKER->renderFlags & RENDER_FLAG_CHECKERED_FLAG) != 0)
	{
		RaceFlag_DrawSelf();
	}

	RenderDispEnv_UI(gGT);
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_UI);

	MAINFRAME_PERF_BEGIN(NATIVE_PERF_BUCKET_MAINFRAME_RENDER_VSYNC);
	RenderVSYNC(gGT);
	MAINFRAME_PERF_END(NATIVE_PERF_BUCKET_MAINFRAME_RENDER_VSYNC);

#ifndef CTR_NATIVE
	RenderFMV();
#endif

	RenderSubmit(gGT);
}
