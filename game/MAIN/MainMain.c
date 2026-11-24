#include <common.h>

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
#include <platform/native_perf.h>
#include <platform/native_replay_scheduler.h>
#include <platform/native_savestate.h>
#endif

#ifndef MAIN_GAME_STATE
#define MAIN_GAME_STATE     (sdata->mainGameState)
#define MAIN_LOAD_STAGE     (sdata->Loading.stage)
#define MAIN_LEV_ID_TO_LOAD (sdata->Loading.Lev_ID_To_Load)
#define MAIN_ADD_BITS_0     (sdata->Loading.OnBegin.AddBitsConfig0)
#define MAIN_REM_BITS_0     (sdata->Loading.OnBegin.RemBitsConfig0)
#define MAIN_ADD_BITS_8     (sdata->Loading.OnBegin.AddBitsConfig8)
#define MAIN_REM_BITS_8     (sdata->Loading.OnBegin.RemBitsConfig8)
#define MAIN_GT             (sdata->gGT)
#define MAIN_GS             (sdata->gGamepads)
#define MAIN_DRAW_SYNC_PTR  (sdata->MainDrawCb_DrawSyncPtr)
#define MAIN_VLC_LOADED     (sdata->bool_IsLoaded_VlcTable)
#define MAIN_BIGFILE_1      (sdata->ptrBigfile1)
#define MAIN_LNG_STRINGS    (sdata->lngStrings)
#define MAIN_ADV_MASK_3D    (sdata->boolDraw3D_AdvMask)
#define MAIN_SOUND_PAUSED   (sdata->boolSoundPaused)
#define MAIN_S_NDI_WORD     (*(const u32 *)sdata->s_ndi_needToRename)
#define MAIN_PAUSE_STATE    (sdata->pause_state)
#define MAIN_XA_STATE       (sdata->XA_State)
#define MAIN_FRAME_COUNTER  (sdata->frameCounter)
#define MAIN_MENU_STATE     (sdata->mainMenuState)
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
	info.deadcoed0 = (u32)gGT->deadcoed_struct.state0;
	info.deadcoed1 = (u32)gGT->deadcoed_struct.state1;
	info.advRng0 = sdata->advRng.state0;
	info.advRng1 = sdata->advRng.state1;

	return info;
}
#endif

// NOTE(aalhendi): Retail compiles state-0 initialization inline inside main
// (0x8003c614-0x8003c984). Native also exports StateZero() so host harnesses
// can observe boot initialization directly.
static inline void Main_StateZeroBody(void)
{
	u32 vramSize;

	memset(MAIN_GT, 0, sizeof(*MAIN_GT));

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

	SetDefDrawEnv(&MAIN_GT->db[0].drawEnv, 0, 0, 0x200, 0xd8);
	SetDefDrawEnv(&MAIN_GT->db[1].drawEnv, 0, 0x128, 0x200, 0xd8);
	SetDefDispEnv(&MAIN_GT->db[0].dispEnv, 0, 0x128, 0x200, 0xd8);
	SetDefDispEnv(&MAIN_GT->db[1].dispEnv, 0, 0, 0x200, 0xd8);

	MAIN_GT->db[0].dispEnv.screen.x = 0;
	MAIN_GT->db[0].dispEnv.screen.y = 0xc;
	MAIN_GT->db[0].dispEnv.screen.w = 0x100;
	MAIN_GT->db[0].dispEnv.screen.h = 0xd8;

	MAIN_GT->db[1].dispEnv.screen.x = 0;
	MAIN_GT->db[1].dispEnv.screen.y = 0xc;
	MAIN_GT->db[1].dispEnv.screen.w = 0x100;
	MAIN_GT->db[1].dispEnv.screen.h = 0xd8;

	MAIN_GT->db[0].drawEnv.isbg = 1;
	MAIN_GT->db[0].drawEnv.r0 = 0;
	MAIN_GT->db[0].drawEnv.g0 = 0;
	MAIN_GT->db[0].drawEnv.b0 = 0;

	MAIN_GT->db[1].drawEnv.isbg = 1;
	MAIN_GT->db[1].drawEnv.r0 = 0;
	MAIN_GT->db[1].drawEnv.g0 = 0;
	MAIN_GT->db[1].drawEnv.b0 = 0;

	// default number of lives in battle
	// this is left over from prototypes, useless in retail
	MAIN_GT->battleLifeLimit = 5;

	// 30 second counter
	MAIN_GT->constVal_9000 = 9000;

	MAIN_GT->battleSetup.enabledWeapons |= BATTLE_DEFAULT_WEAPON_FLAGS;

	// set lap count to 3
	MAIN_GT->numLaps = 3;
	MAIN_GT->numPlyrCurrGame = 1;
	MAIN_GT->numPlyrNextGame = 1;
#ifdef CTR_NATIVE
	MAIN_GT->battleSetup.teamOfEachPlayer[0] = 0;
	MAIN_GT->battleSetup.teamOfEachPlayer[1] = 1;
	MAIN_GT->battleSetup.teamOfEachPlayer[2] = 2;
	MAIN_GT->battleSetup.teamOfEachPlayer[3] = 3;
#else
	*(u32 *)&MAIN_GT->battleSetup.teamOfEachPlayer = 0x3020100;
#endif

	// traffic light countdown timer, set to negative one second
	MAIN_GT->trafficLightsTimer = 0xfffffc40;

	Timer_Init();
	EnterCriticalSection();
	MAIN_DRAW_SYNC_PTR = (void *)(u32)DrawSyncCallback(&MainDrawCb_DrawSync);
	ExitCriticalSection();

	MEMCARD_InitCard();
	VSync(0);
	GAMEPAD_Init(MAIN_GS);
	VSync(0);
	GAMEPAD_GetNumConnected(MAIN_GS);

#ifdef CTR_NATIVE
#define BIGPATH "\\BIGFILE.BIG;1"
#else
#define BIGPATH rdata.s_PathTo_Bigfile
#endif

	// Get CD Position of BIGFILE
	MAIN_BIGFILE_1 = LOAD_ReadDirectory(BIGPATH);

	// English=1
	LOAD_LangFile((int)MAIN_BIGFILE_1, 1);
	GAMEPROG_NewGame_OnBoot();
	MAIN_GT->overlayIndex_null_notUsed = 0;

	MAIN_GT->levelID = NAUGHTY_DOG_CRATE;
#ifdef CTR_NATIVE
	memcpy(MAIN_GT->levelName, sdata->s_ndi, sizeof(sdata->s_ndi));
#else
	*(u32 *)MAIN_GT->levelName = MAIN_S_NDI_WORD;
#endif

	InitGeom();
	SetGeomOffset(0x100, 0x78); // width/2, height/2
	SetGeomScreen(0x140);       // "distance" to screen, alters FOV

	// NOTE(aalhendi): Retail calls 0x8006ae74 here; keep the verified depth
	// scale setup on every target.
	RenderBucket_InitDepthGTE();
	// NOTE(aalhendi): Retail bakes these authored matrix tables in place.
	Vector_BakeMatrixTable();

	MAIN_GT->swapchainIndex = 0;
	MAIN_GT->backBuffer = &MAIN_GT->db[0];

	MAIN_GT->overlayIndex_EndOfRace = 0xff;
	MAIN_GT->overlayIndex_LOD = OVERLAY_INDEX_NONE;
	MAIN_GT->overlayIndex_Threads = OVERLAY_INDEX_NONE;

	PutDispEnv(&MAIN_GT->db[1].dispEnv);
	PutDrawEnv(&MAIN_GT->db[1].drawEnv);
	DrawSync(0);

	// Load Intro TIM for "SCEA Presents" from VRAM file
	LOAD_VramFile(MAIN_BIGFILE_1, 0x1fd, NULL, &vramSize, -1);
	MainInit_VRAMDisplay();

	// \SOUNDS\KART.HWL;1
	howl_InitGlobals(data.kartHwlPath);

	VSyncCallback(MainDrawCb_Vsync);

	Music_SetIntro();
	CseqMusic_StopAll();
	CseqMusic_Start(CSEQ_SONG_LEVEL, 0, NULL, 0, 0);
	Music_Start(0);

	// "Start your engines, for Sony Computer..."
	CDSYS_XAPlay(CDSYS_XA_TYPE_EXTRA, 0x50);

	while (MAIN_XA_STATE != 0)
	{
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Retail hardware interrupts keep XA/audio moving while
		// this loop spins. Native owns VBlank in VSync(), so pump it here.
		VSync(0);
#endif
		CDSYS_XAPauseAtEnd();
	}

	DecalGlobal_Clear(MAIN_GT);

	// This loads UI textures (shared.vrm)
	// This includes traffic lights, font, and more
	LOAD_VramFile(MAIN_BIGFILE_1, 0x102, NULL, &vramSize, -1);

	MAIN_GAME_STATE = 3;

	// start loading
	MAIN_LOAD_STAGE = LOAD_TEN_STAGES_0;
	MAIN_GT->gameMode1 |= LOADING;
	MAIN_GT->clockEffectEnabled &= 0xfffe;
}

#ifdef CTR_NATIVE
void StateZero(void)
{
	Main_StateZeroBody();
}
#endif

#ifdef CTR_NATIVE
u32 CTR_Main(void)
#else
int main(void)
#endif
{
	while (MAIN_GAME_STATE != 5)
	{
		LOAD_NextQueuedFile();
		CDSYS_XAPauseAtEnd();

		switch (MAIN_GAME_STATE)
		{
		// Initialize Game (happens once)
		case 0:
#ifdef CTR_NATIVE
			StateZero();
#else
			Main_StateZeroBody();
#endif
			break;

		// Reset stage, reset music
		case 2:
			Audio_SetState_Safe(1);
			MEMPACK_PopState();

			// ignore threads, because we PopState,
			// so the threadpool will reset anyway
			LevInstDef_RePack(MAIN_GT->level1->ptr_mesh_info, 0);

			MAIN_GAME_STATE = 1;
			break;

		// Happens on first frame that loading ends
		case 1:
			ElimBG_Deactivate(MAIN_GT);
			MainStats_RestartRaceCountLoss();
			Voiceline_ClearTimeStamp();

			// Disable End-Of-Race menu
			MAIN_GT->gameMode1 &= ~END_OF_RACE;

			if ((MAIN_GT->levelID != MAIN_MENU_LEVEL) && (RaceFlag_IsFullyOnScreen() != 0))
			{
				RaceFlag_BeginTransition(2);
			}
			else if ((MAIN_GT->levelID == MAIN_MENU_LEVEL) && (RaceFlag_IsFullyOffScreen() != 0))
			{
				RaceFlag_SetFullyOnScreen();
			}

			DropRain_Reset(MAIN_GT);
			GAMEPROG_GetPtrHighScoreTrack();
			MainInit_FinalizeInit(MAIN_GT);
			GAMEPAD_GetNumConnected(MAIN_GS);

			MAIN_SOUND_PAUSED = 0;
			VehBirth_EngineAudio_AllPlayers();

			// 9 = intro cutscene, 10 = traffic lights, 11 = racing
			if ((u32)MAIN_GT->levelID < NITRO_COURT)
			{
				Audio_SetState_Safe(9);
			}
			else if ((u32)(MAIN_GT->levelID - NITRO_COURT) < 7)
			{
				Audio_SetState_Safe(10);
			}

			MAIN_GAME_STATE = 3;
			MAIN_GT->clockEffectEnabled &= 0xfffe;
			break;

		// Main Gameplay Update
		// Makes up all normal interaction with the game
		case 3:
			// if loading, or gameplay interrupted
			if (MAIN_LOAD_STAGE != LOAD_IDLE)
			{
				struct GameTracker *gGT;
				int stage;

				if ((RaceFlag_IsFullyOnScreen() == 1) || (MAIN_GT->levelID == NAUGHTY_DOG_CRATE) || (MAIN_PAUSE_STATE != 0))
				{
					MAIN_GT->gameMode1 |= LOADING;
				}

				gGT = MAIN_GT;
				stage = MAIN_LOAD_STAGE;

				// elapsed milliseconds per frame, locked 32 here
				// impacts speed of flag wave during "loading...", but does not impact speed of flying text
				gGT->elapsedTimeMS = 32;

				// if restarting race
				if (stage == LOAD_RESTART)
				{
					if (RaceFlag_IsFullyOnScreen() == 1)
					{
						// reinitialize world, does not reinitialize pools
						MAIN_GAME_STATE = 2;
						MAIN_LOAD_STAGE = LOAD_IDLE;
						MAIN_GT->gameMode1 &= ~LOADING;
						break;
					}
				}
				// if waiting for checkered flag to cover screen,
				// right before loading the next requested level
				else if (stage == -4)
				{
					if (RaceFlag_IsFullyOnScreen() == 1)
					{
						struct GameTracker *loadGT = MAIN_GT;
						u32 add0 = MAIN_ADD_BITS_0;
						u32 rem0 = MAIN_REM_BITS_0;
						u32 add8 = MAIN_ADD_BITS_8;
						u32 rem8 = MAIN_REM_BITS_8;

						MAIN_ADD_BITS_0 = 0;
						MAIN_REM_BITS_0 = 0;
						MAIN_ADD_BITS_8 = 0;
						MAIN_REM_BITS_8 = 0;

						loadGT->hudFlags &= HUD_FLAG_CLEAR_INTRO_RACE_TITLE_BARS_MASK;

						loadGT->gameMode1 |= add0;
						loadGT->gameMode2 |= add8;
						loadGT->gameMode1 &= ~rem0;
						loadGT->gameMode2 &= ~rem8;

						MainRaceTrack_StartLoad(MAIN_LEV_ID_TO_LOAD);
					}
					else if (RaceFlag_IsFullyOffScreen() == 1)
					{
						RaceFlag_BeginTransition(1);
					}
				}
				// if loading VLC
				else if (stage == LOAD_VLC)
				{
					if (MAIN_VLC_LOADED == 1)
					{
						MAIN_LOAD_STAGE = LOAD_IDLE;
						MAIN_GAME_STATE = 1;
						gGT->gameMode1 &= ~LOADING;
					}
					break;
				}
				// if something is being loaded
				else
				{
					MAIN_LOAD_STAGE = LOAD_TenStages(gGT, stage, MAIN_BIGFILE_1);

					// If just finished loading stage 9
					if (MAIN_LOAD_STAGE == LOAD_FINISHED)
					{
						gGT = MAIN_GT;
						if ((gGT->levelID == MAIN_MENU_LEVEL) || (gGT->levelID == SCRAPBOOK))
						{
							MainLoadVLC();
							MAIN_LOAD_STAGE = LOAD_VLC;
							break;
						}

						MAIN_LOAD_STAGE = LOAD_IDLE;
						MAIN_GAME_STATE = 1;
						gGT->gameMode1 &= ~LOADING;
						break;
					}
				}
			}

			// =========== Main Game Loop ======================

			if ((-960 < MAIN_GT->trafficLightsTimer) && ((MAIN_GT->gameMode1 & (START_OF_RACE | PAUSE_ALL)) == 0))
			{
				MAIN_GT->trafficLightsTimer -= MAIN_GT->elapsedTimeMS;
				if (MAIN_GT->trafficLightsTimer < -960)
				{
					MAIN_GT->trafficLightsTimer = 0xfffffc40;
				}
			}

			MAIN_FRAME_COUNTER++;

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
			{
				struct NativeReplaySchedulerFrameInfo replayFrameInfo = MainReplayScheduler_FrameInfo(MAIN_GT);

				if (NativeReplayScheduler_BeginFrame(&replayFrameInfo) != 0)
				{
					return 0;
				}
				NativeSaveState_BeginFrame();
			}
			{
				struct NativePerfFrameInfo perfFrameInfo = MainPerf_FrameInfo(MAIN_GT);

				NativePerf_BeginFrame(&perfFrameInfo);
			}
#endif
			GAMEPAD_ProcessAnyoneVars(MAIN_GS);

			// Start new frame (ClearOTagR)
			MainFrame_ResetDB(MAIN_GT, MAIN_GS);

			if ((MAIN_GT->boolDemoMode != 0) && (MAIN_GT->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK, MAIN_LOAD_STAGE == LOAD_IDLE))
			{
				// 30-second timer within Demo Mode
				MAIN_GT->demoCountdownTimer--;

				if (MAIN_GT->demoCountdownTimer < 1)
				{
					MAIN_GT->boolDemoMode = 0;
					MAIN_GT->numPlyrNextGame = 1;
					MAIN_MENU_STATE = MAIN_MENU_TITLE;
					MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
				}
				else if (MAIN_GS->anyoneHeldCurr != 0)
				{
					MAIN_GT->boolDemoMode = 0;
					MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
				}

				if (MAIN_GT->numPlyrCurrGame == 1)
				{
					DecalFont_DrawMultiLine(MAIN_LNG_STRINGS[LNG_DEMO_MODE_PRESS_ANY_BUTTON_TO_EXIT], 0x100, 0x23, 0x200, FONT_SMALL, JUSTIFY_CENTER | ORANGE);
				}
				else
				{
					DecalFont_DrawMultiLine(MAIN_LNG_STRINGS[LNG_DEMO_MODE_PRESS_ANY_BUTTON_TO_EXIT], 0x100, 100, 0x200, FONT_SMALL, JUSTIFY_CENTER | ORANGE);
				}
			}

			if ((MAIN_GT->gameMode1 & LOADING) == 0)
			{
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
				NativePerf_BeginScope(NATIVE_PERF_BUCKET_GAME_LOGIC);
#endif
				MainFrame_GameLogic(MAIN_GT, MAIN_GS);
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
				NativePerf_EndScope(NATIVE_PERF_BUCKET_GAME_LOGIC);
#endif
			}

			if (MAIN_GT->boolDemoMode != 0)
			{
				MAIN_GT->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;
			}

			// reset vsync calls between drawsync
			MAIN_GT->vSync_between_drawSync = 0;

#ifdef CTR_NATIVE
			Platform_BeginFrame();
#endif
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
			NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDER_FRAME);
#endif
			MainFrame_RenderFrame(MAIN_GT, MAIN_GS);
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
			NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDER_FRAME);
#endif
#ifdef CTR_NATIVE
			Platform_EndFrame();
#endif

			// if mask is talking in Adventure Hub
			if (MAIN_ADV_MASK_3D != 0)
			{
				AH_MaskHint_Update();
			}
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
			{
				struct NativeReplaySchedulerFrameInfo replayFrameInfo = MainReplayScheduler_FrameInfo(MAIN_GT);

				if (NativeReplayScheduler_EndFrame(&replayFrameInfo) != 0)
				{
					return 0;
				}
			}
			{
				struct NativePerfFrameInfo perfFrameInfo = MainPerf_FrameInfo(MAIN_GT);

				NativePerf_EndFrame(&perfFrameInfo);
			}
#endif
			break;

#ifndef CTR_NATIVE
		// Left over from demos that timed out and restarted after sitting idle
		case 4:
			MEMPACK_PopState();
			MEMPACK_PopState();
			MEMPACK_PopState();

			CTR_ErrorScreen(0, 0, 0);
			Music_Stop();

			howl_StopAudio(1, 1, 1);
			Bank_DestroyAll();
			howl_Disable();

			GAMEPAD_SetMainMode(MAIN_GS);

			VSync(30);

			MAIN_GAME_STATE = 0;
			break;
#endif
		}
	}

#ifndef CTR_NATIVE
	MainKillGame_StopCTR();
#endif
	return 0;
}
