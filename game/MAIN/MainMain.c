#include <common.h>

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
#include <platform/native_perf.h>
#include <platform/native_replay_scheduler.h>
#include <platform/native_savestate.h>
#endif

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
static struct NativePerfFrameInfo MainPerf_FrameInfo(struct GameTracker *gGT)
{
	struct NativePerfFrameInfo info;

	info.frameCounter = sdata->frameCounter;
	info.timer = gGT->timer;
	info.levelID = gGT->levelID;
	info.gameMode1 = gGT->gameMode1;
	info.loadingStage = sdata->Loading.stage;
	info.boolDemoMode = gGT->boolDemoMode;
	info.numPlyrCurrGame = gGT->numPlyrCurrGame;
	info.elapsedTimeMS = gGT->elapsedTimeMS;
	info.vsyncTillFlip = sdata->vsyncTillFlip;
	info.vSync_between_drawSync = gGT->vSync_between_drawSync;
	info.frameTimer_VsyncCallback = gGT->frameTimer_VsyncCallback;

	return info;
}

static struct NativeReplaySchedulerFrameInfo MainReplayScheduler_FrameInfo(struct GameTracker *gGT)
{
	struct NativeReplaySchedulerFrameInfo info;

	info.frameTimer = gGT->frameTimer_VsyncCallback;
	info.frameCounter = sdata->frameCounter;
	info.timer = gGT->timer;
	info.framesInThisLEV = gGT->framesInThisLEV;
	info.elapsedTimeMS = gGT->elapsedTimeMS;
	info.msInThisLEV = gGT->msInThisLEV;
	info.elapsedEventTime = gGT->elapsedEventTime;
	info.mainGameState = sdata->mainGameState;
	info.loadingStage = sdata->Loading.stage;
	info.levelID = gGT->levelID;
	info.mixRandomNumber = (u32)sdata->randomNumber;
	info.audioRNG = sdata->audioRNG;
	info.deadcoed0 = (u32)gGT->deadcoed_struct.unk1;
	info.deadcoed1 = (u32)gGT->deadcoed_struct.unk2;
	info.advRng0 = (u32)sdata->const_0x30215400;
	info.advRng1 = (u32)sdata->const_0x493583fe;

	return info;
}
#endif

// NOTE(aalhendi): PSX path ASM-verified NTSC-U 926 0x8003c58c-0x8003cf7c.
#ifdef CTR_NATIVE
u32 CTR_Main(void)
#else
u32 main(void)
#endif
{
	u32 AddBitsConfig0;
	u32 RemBitsConfig0;
	u32 AddBitsConfig8;
	u32 RemBitsConfig8;
	int iVar8;
	u32 gameMode1;
	u32 gameMode2;
	u32 uVar12;

	struct GameTracker *gGT;
	gGT = sdata->gGT;

	struct GamepadSystem *gGS;
	gGS = sdata->gGamepads;

	// NOTE(aalhendi): Retail main calls __main before the state loop. Native has
	// no linked __main body, so keep this as a CTR_NATIVE-only divergence.
#ifndef CTR_NATIVE
	__main();
#endif

	do
	{
#ifndef CTR_NATIVE
		// wont happen under normal conditions
		if (sdata->mainGameState == 5)
		{
			MainKillGame_StopCTR();
			return 0;
		}
#endif

		LOAD_NextQueuedFile();
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003c5d0-0x8003c5dc for per-frame XA pause handling.
		CDSYS_XAPauseAtEnd();

		switch (sdata->mainGameState)
		{
		// Initialize Game (happens once)
		case 0:
			StateZero();
			break;

		// Happens on first frame that loading ends
		case 1:

			ElimBG_Deactivate(gGT);

			MainStats_RestartRaceCountLoss();
			// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003c9f8-0x8003ca04 for load-complete voiceline reset.
			Voiceline_ClearTimeStamp();

			// Disable End-Of-Race menu
			gGT->gameMode1 &= ~END_OF_RACE;

			if (gGT->levelID == MAIN_MENU_LEVEL)
			{
				if (RaceFlag_IsFullyOffScreen() != 0)
					RaceFlag_SetFullyOnScreen();
			}

			else
			{
				if (RaceFlag_IsFullyOnScreen() != 0)
					RaceFlag_BeginTransition(2);
			}

			DropRain_Reset(gGT);
			GAMEPROG_GetPtrHighScoreTrack();
			MainInit_FinalizeInit(gGT);
			GAMEPAD_GetNumConnected(gGS);

			sdata->boolSoundPaused = 0;
			// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003caa4-0x8003cab4 for load-complete engine audio init.
			VehBirth_EngineAudio_AllPlayers();

			// 9 = intro cutscene
			// 10 = traffic lights
			// 11 = racing

			// Arcade-Style track starts with intro cutscene
			uVar12 = 9;

			if (
			    // If Level ID is less than 18, it's one of the race tracks
			    (gGT->levelID < NITRO_COURT) || (
			                                        // Battle-Style track starts with traffic lights
			                                        uVar12 = 10,
			                                        // Level ID >= 18 and < 23
			                                        // Battle tracks
			                                        gGT->levelID - NITRO_COURT < 7))
			{
				Audio_SetState_Safe(uVar12);
			}
			sdata->mainGameState = 3;
			gGT->clockEffectEnabled &= 0xfffe;
			break;

		// Reset stage, reset music
		case 2:
			Audio_SetState_Safe(1);
			MEMPACK_PopState();

			// ignore threads, because we PopState,
			// so the threadpool will reset anyway
			LevInstDef_RePack(gGT->level1->ptr_mesh_info, 0);

			sdata->mainGameState = 1;
			break;

		// Main Gameplay Update
		// Makes up all normal interaction with the game
		case 3:

			// if loading, or gameplay interrupted
			if (sdata->Loading.stage != LOAD_IDLE)
			{
				if ((RaceFlag_IsFullyOnScreen() == 1) || (gGT->levelID == NAUGHTY_DOG_CRATE) || (sdata->pause_state != 0))
				{
					gGT->gameMode1 |= LOADING;
				}

				iVar8 = sdata->Loading.stage;

				// elapsed milliseconds per frame, locked 32 here
				// impacts speed of flag wave during "loading...", but does not impact speed of flying text
				gGT->elapsedTimeMS = 32;

				// if loading VLC
				if (iVar8 == LOAD_VLC)
				{
					// if VLC is not loaded, quit
					// we know when it's done from a load callback
					if (sdata->bool_IsLoaded_VlcTable != 1)
						break;

					// if == 1, finish the loading
					goto FinishLoading;
				}

				// if restarting race
				if (iVar8 == LOAD_RESTART)
				{
					if (RaceFlag_IsFullyOnScreen() == 1)
					{
						// reinitialize world,
						// does not reinitialize pools
						sdata->mainGameState = 2;

						// no loading, and no interruption
						sdata->Loading.stage = LOAD_IDLE;

						// Turn off the "Loading..." flag
						gGT->gameMode1 &= ~LOADING;
						break;
					}

					// if not fully on-screen, do not BREAK,
					// keep rendering the scene
				}

				// if waiting for checkered flag to cover screen,
				// right before loading the next requested level
				else if (iVar8 == -4)
				{
					RemBitsConfig8 = sdata->Loading.OnBegin.RemBitsConfig8;
					AddBitsConfig8 = sdata->Loading.OnBegin.AddBitsConfig8;
					RemBitsConfig0 = sdata->Loading.OnBegin.RemBitsConfig0;
					AddBitsConfig0 = sdata->Loading.OnBegin.AddBitsConfig0;

					if (RaceFlag_IsFullyOnScreen() == 1)
					{
						sdata->Loading.OnBegin.AddBitsConfig0 = 0;
						sdata->Loading.OnBegin.RemBitsConfig0 = 0;
						sdata->Loading.OnBegin.AddBitsConfig8 = 0;
						sdata->Loading.OnBegin.RemBitsConfig8 = 0;

						gameMode2 = gGT->gameMode2;

						gGT->hudFlags &= 0xf7;

						gameMode1 = gGT->gameMode1;
						gGT->gameMode2 = gameMode2 | AddBitsConfig8;
						gGT->gameMode1 = gameMode1 | AddBitsConfig0;
						gGT->gameMode1 = (gameMode1 | AddBitsConfig0) & ~RemBitsConfig0;
						gGT->gameMode2 = (gameMode2 | AddBitsConfig8) & ~RemBitsConfig8;

						MainRaceTrack_StartLoad(sdata->Loading.Lev_ID_To_Load);
					}

					else if (RaceFlag_IsFullyOffScreen() == 1)
						RaceFlag_BeginTransition(1);

					// do not BREAK,
					// keep rendering the scene
				}

				// if something is being loaded
				else
				{
					sdata->Loading.stage = LOAD_TenStages(gGT, iVar8, sdata->ptrBigfile1);

					// If just finished loading stage 9
					if (sdata->Loading.stage == LOAD_FINISHED)
					{
						if ((gGT->levelID == MAIN_MENU_LEVEL) || (gGT->levelID == SCRAPBOOK))
						{
							MainLoadVLC();

							// start loading VLC (scroll up to iVar8 == LOAD_VLC)
							sdata->Loading.stage = LOAD_VLC;
							break;
						}

					FinishLoading:
						// loading is finished,
						// initialize world and pools,
						// remove LOADING... flag from gGT
						sdata->Loading.stage = LOAD_IDLE;
						sdata->mainGameState = 1;
						gGT->gameMode1 &= ~LOADING;
						break;
					}

					// else, do not BREAK,
					// keep rendering the scene
					// which is the checkered flag
				}
			}

			// =========== Main Game Loop ======================

			if ((
			        // Check value of traffic lights
			        (-960 < gGT->trafficLightsTimer) &&
			        // if not drawing intro race cutscene and if not paused
			        ((gGT->gameMode1 & (START_OF_RACE | PAUSE_ALL)) == 0)) &&
			    (
			        // amount of milliseconds on Traffic Lights - elapsed milliseconds per frame, ~32
			        iVar8 = gGT->trafficLightsTimer - gGT->elapsedTimeMS,
			        // decrease amount of time on Traffic Lights
			        gGT->trafficLightsTimer = iVar8,
			        // if countdown has gone down far enough for traffic lights to go off-screen
			        iVar8 < -960))
			{
				// set a floor value, so countdown can't go farther negative
				gGT->trafficLightsTimer = 0xfffffc40;
			}

			// frame counter, not represented in common.h currently
			sdata->frameCounter++;

			// Process all gamepad input
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
			{
				struct NativeReplaySchedulerFrameInfo replayFrameInfo = MainReplayScheduler_FrameInfo(gGT);

				if (NativeReplayScheduler_BeginFrame(&replayFrameInfo) != 0)
					return 0;
				NativeSaveState_BeginFrame();
				gGT = sdata->gGT;
				gGS = sdata->gGamepads;
			}
			{
				struct NativePerfFrameInfo perfFrameInfo = MainPerf_FrameInfo(gGT);

				NativePerf_BeginFrame(&perfFrameInfo);
			}
#endif
			GAMEPAD_ProcessAnyoneVars(gGS);

			// Start new frame (ClearOTagR)
			MainFrame_ResetDB(gGT);

			if (
			    // If you're in Demo Mode
			    (gGT->boolDemoMode != 0) &&

			    (
			        // Turn off HUD
			        gGT->hudFlags &= 0xfe,
			        // if game is not loading
			        sdata->Loading.stage == LOAD_IDLE))
			{
				// All this code is for the 30-second timer within Demo Mode
				// To see 30-second timer in Main Menu, go to FUN_00001604 in 230.c
				// pressing (or holding) any button sets it to zero

				gGT->demoCountdownTimer--;

				// check to see if time ran out
				if (gGT->demoCountdownTimer < 1)
				{
					// leave demo mode, go to main menu
					gGT->boolDemoMode = 0;
					gGT->numPlyrNextGame = 1;
					sdata->mainMenuState = MAIN_MENU_TITLE;

				LAB_8003ce08:
					MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
				}

				// if time remains on the timer
				else
				{
					// if any button is pressed by anyone
					if (gGS->anyoneHeldCurr != 0)
					{
						// leave demo mode
						gGT->boolDemoMode = 0;
						goto LAB_8003ce08;
					}
				}

				// if numPlyrCurrGame is 1
				if (gGT->numPlyrCurrGame == 1)
				{
					// Draw text near top of screen
					uVar12 = 0x23;
				}

				// if this is multiplayer
				else
				{
					// draw text halfway to top of screen
					uVar12 = 100;
				}

				DecalFont_DrawMultiLine(sdata->lngStrings[LNG_DEMO_MODE_PRESS_ANY_BUTTON_TO_EXIT], 0x100, uVar12, 0x200, 2, 0xffff8000);
			}

			if ((gGT->gameMode1 & LOADING) == 0)
			{
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
				NativePerf_BeginScope(NATIVE_PERF_BUCKET_GAME_LOGIC);
#endif
				MainFrame_GameLogic(gGT, gGS);
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
				NativePerf_EndScope(NATIVE_PERF_BUCKET_GAME_LOGIC);
#endif
			}

			// If you are in demo mode
			if (gGT->boolDemoMode != '\0')
			{
				// Turn off HUD
				gGT->hudFlags &= 0xfe;
			}

			// reset vsync calls between drawsync
			gGT->vSync_between_drawSync = 0;


#ifdef CTR_NATIVE
			Platform_BeginFrame();
#endif
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
			NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDER_FRAME);
#endif
			MainFrame_RenderFrame(gGT, gGS);
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
			NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDER_FRAME);
#endif
#ifdef CTR_NATIVE
			Platform_EndFrame();
#endif


			// if mask is talking in Adventure Hub
			if (sdata->boolDraw3D_AdvMask != 0)
			{
				AH_MaskHint_Update();
			}
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
			{
				struct NativeReplaySchedulerFrameInfo replayFrameInfo = MainReplayScheduler_FrameInfo(gGT);

				if (NativeReplayScheduler_EndFrame(&replayFrameInfo) != 0)
					return 0;
			}
			{
				struct NativePerfFrameInfo perfFrameInfo = MainPerf_FrameInfo(gGT);

				NativePerf_EndFrame(&perfFrameInfo);
			}
#endif
			break;

#ifndef CTR_NATIVE
		// In theory, this is left over from the demos,
		// which would "timeout" and restart after sitting idle
		case 4:

			// erase all data past the
			// last 3 bookmarks, if there
			// that many exist
			MEMPACK_PopState();
			MEMPACK_PopState();
			MEMPACK_PopState();

			CTR_ErrorScreen(0, 0, 0);
			Music_Stop();

			// clear backup, destroy music, destroy all fx
			howl_StopAudio(1, 1, 1);
			Bank_DestroyAll();
			howl_Disable();

			GAMEPAD_SetMainMode();

			// Set vsync to 2 FPS
			VSync(30);

			// reboot game
			sdata->mainGameState = 0;
#endif
		}
	} while (true);
}

// NOTE(aalhendi): Source split of retail main's state-0 body
// 0x8003c614-0x8003c984.
// By separating this, it can be overwritten dynamically (oxide fix).
void StateZero()
{
	u16 *clockEffect;
	int vramSize;

	struct GameTracker *gGT;
	gGT = sdata->gGT;

	struct GamepadSystem *gGS;
	gGS = sdata->gGamepads;

// already zero, part of BSS
#if 0
	memset(gGT, 0, sizeof(struct GameTracker));
#endif

	// Set Video Mode to NTSC
	SetVideoMode(0);
	ResetCallback();

#define MEMPACK_SIZE 0x200000 // 2mb

	MEMPACK_Init(MEMPACK_SIZE);
	LOAD_InitCD();
	RaceFlag_SetFullyOffScreen();

	ResetGraph(0);
	SetGraphDebug(0);

	MainInit_VRAMClear();

	SetDispMask(1);

	SetDefDrawEnv(&gGT->db[0].drawEnv, 0, 0, 0x200, 0xd8);
	SetDefDrawEnv(&gGT->db[1].drawEnv, 0, 0x128, 0x200, 0xd8);
	SetDefDispEnv(&gGT->db[0].dispEnv, 0, 0x128, 0x200, 0xd8);
	SetDefDispEnv(&gGT->db[1].dispEnv, 0, 0, 0x200, 0xd8);

	gGT->db[0].dispEnv.screen.x = 0;
	gGT->db[0].dispEnv.screen.y = 0xc;
	gGT->db[0].dispEnv.screen.w = 0x100;
	gGT->db[0].dispEnv.screen.h = 0xd8;

	gGT->db[1].dispEnv.screen.x = 0;
	gGT->db[1].dispEnv.screen.y = 0xc;
	gGT->db[1].dispEnv.screen.w = 0x100;
	gGT->db[1].dispEnv.screen.h = 0xd8;

	gGT->db[0].drawEnv.isbg = 1;
	gGT->db[0].drawEnv.r0 = 0;
	gGT->db[0].drawEnv.g0 = 0;
	gGT->db[0].drawEnv.b0 = 0;

	gGT->db[1].drawEnv.isbg = 1;
	gGT->db[1].drawEnv.r0 = 0;
	gGT->db[1].drawEnv.g0 = 0;
	gGT->db[1].drawEnv.b0 = 0;

	// default number of lives in battle
	// this is left over from prototypes, useless in retail
	gGT->battleLifeLimit = 5;

	// 30 second counter?
	gGT->constVal_9000 = 9000;

	// set lap count to 3
	gGT->numLaps = 3;

	gGT->battleSetup.enabledWeapons |= 0x34de;
	gGT->numPlyrCurrGame = 1;
	gGT->numPlyrNextGame = 1;
	*(u32 *)&gGT->battleSetup.teamOfEachPlayer = 0x3020100;

	// traffic light countdown timer, set to negative one second
	gGT->trafficLightsTimer = 0xfffffc40;

	Timer_Init();
	DrawSyncCallback(&MainDrawCb_DrawSync);

	MEMCARD_InitCard();
	VSync(0);
	GAMEPAD_Init(gGS);
	VSync(0);
	GAMEPAD_GetNumConnected(gGS);

#ifdef CTR_NATIVE
#define BIGPATH "\\BIGFILE.BIG;1"
#else
#define BIGPATH rdata.s_PathTo_Bigfile
#endif

	// Get CD Position fo BIGFILE
	sdata->ptrBigfile1 = LOAD_ReadDirectory(BIGPATH);

// Defrag to save heap space,
// required because MEMPACK_Init moves heap
#if 0
	// NOTE(aalhendi): Retail main does not rewrite BIGFILE overlay sizes here.
	extern char RB_NewEndFile[4];

	// Dont load full overlay file, cut off the end
	struct BigEntry *firstEntry = BIG_GETENTRY(sdata->ptrBigfile1);
	firstEntry[231].size = 28 * 0x800;
	// firstEntry[231].size = (u32)RB_NewEndFile - (u32)OVR_Region3;
	// printf("Size: %08x\n", firstEntry[231].size);

	// Cut off Region1 overlays at 2 sectors (not 3),
	// This protects Region2 RAM so it is not overwritten
	// during Region1 disc streaming, saves loading time
	for (int i = 221; i <= 225; i++)
		firstEntry[i].size = 2 * 0x800;
#endif

	// English=1
	// PAL SCES02105 calls it multiple times
	LOAD_LangFile(sdata->ptrBigfile1, 1);
	GAMEPROG_NewGame_OnBoot();
	gGT->overlayIndex_null_notUsed = 0;

	gGT->levelID = NAUGHTY_DOG_CRATE;
	// gGT->levelID = OXIDE_TRUE_ENDING;

	InitGeom();
	SetGeomOffset(0x100, 0x78); // width/2, height/2
	SetGeomScreen(0x140);       // "distance" to screen, alters FOV

	// NOTE(aalhendi): Retail calls 0x8006ae74 here; keep the verified depth
	// scale setup on every target.
	RenderBucket_InitDepthGTE();
	// NOTE(aalhendi): Retail bakes these authored matrix tables in place.
	Vector_BakeMatrixTable();

	gGT->swapchainIndex = 0;
	gGT->backBuffer = &gGT->db[0];

	gGT->overlayIndex_EndOfRace = 0xff;
	gGT->overlayIndex_LOD = 0xff;
	gGT->overlayIndex_Threads = 0xff;

	PutDispEnv(&gGT->db[1].dispEnv);
	PutDrawEnv(&gGT->db[1].drawEnv);
	DrawSync(0);

	// Load Intro TIM for "SCEA Presents" from VRAM file
	LOAD_VramFile(sdata->ptrBigfile1, 0x1fd, NULL, &vramSize, -1);
	MainInit_VRAMDisplay();

	// \SOUNDS\KART.HWL;1
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003c8e0-0x8003c928 for startup HOWL/music/XA setup.
	howl_InitGlobals(data.kartHwlPath);

	VSyncCallback(MainDrawCb_Vsync);

	Music_SetIntro();
	CseqMusic_StopAll();
	CseqMusic_Start(CSEQ_SONG_LEVEL, 0, NULL, 0, 0);
	Music_Start(0);

	// "Start your engines, for Sony Computer..."
	CDSYS_XAPlay(CDSYS_XA_TYPE_EXTRA, 0x50);

	while (sdata->XA_State != 0)
	{
		// WARNING: Read-only address (ram, 0x8008d888) is written
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003c940-0x8003c948 for startup XA pause polling.
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Retail hardware interrupts keep XA/audio moving while
		// this loop spins. Native owns VBlank in VSync(), so pump it here.
		VSync(0);
#endif
		CDSYS_XAPauseAtEnd();
	}

	DecalGlobal_Clear(gGT);

	// This loads UI textures (shared.vrm)
	// This includes traffic lights, font, and more
	LOAD_VramFile(sdata->ptrBigfile1, 0x102, NULL, &vramSize, -1);

	sdata->mainGameState = 3;

	// start loading
	sdata->Loading.stage = LOAD_TEN_STAGES_0;

	clockEffect = &gGT->clockEffectEnabled;
	gGT->gameMode1 |= LOADING;
	gGT->clockEffectEnabled = *clockEffect & 0xfffe;
}
