#include <common.h>

#ifdef CTR_NATIVE
// NOTE(aalhendi): Native retains its pre-callback level snapshot; PSX rereads
// the live level, including across aliasing color publications.
#define MAININIT_LEVEL(tracker, cachedLevel) (cachedLevel)
#else
#define MAININIT_LEVEL(tracker, cachedLevel) ((tracker)->level1)
#endif

void MainInit_FinalizeInit(struct GameTracker *gGT)
{
	int i;
	int numPlyr;
	struct Driver *d;
	struct Instance *inst;

#ifdef CTR_NATIVE
	struct Level *lev1;

	// NOTE(aalhendi): Native repairs the race-quit warpball lock; retail deliberately retains it.
	gGT->gameMode1 &= ~(WARPBALL_HELD);
#endif

	// enable collisions with all temporary walls
	// (adv hub doors, tiger temple teeth, etc)
	GAME_DOOR_ACCESS_FLAGS = 0;

	// add a bookmark
	MEMPACK_PushState();

	{
		struct GameTracker *tracker = GAME_TRACKER;
		tracker->pushBuffer[0].distanceToScreen_PREV = 0x100;
		tracker->pushBuffer[0].distanceToScreen_CURR = 0x100;
	}

	memset(gGT->threadBuckets, 0, sizeof(gGT->threadBuckets));
	// Initialize bucket names, pause policy, and debug colors.

	gGT->threadBuckets[PLAYER].s_longName = GAME_SDATA_NAME(s_PLAYER);
	gGT->threadBuckets[PLAYER].s_shortName = GAME_SDATA_NAME(s_PLYR);
	gGT->threadBuckets[PLAYER].boolCantPause = 0;
	gGT->threadBuckets[PLAYER].rectCol[0] = 64;
	gGT->threadBuckets[PLAYER].rectCol[1] = 64;
	gGT->threadBuckets[PLAYER].rectCol[2] = 128;

	gGT->threadBuckets[ROBOT].s_longName = GAME_SDATA_NAME(s_ROBOT);
	gGT->threadBuckets[ROBOT].s_shortName = GAME_SDATA_NAME(s_RBOT);
	gGT->threadBuckets[ROBOT].boolCantPause = 0;
	gGT->threadBuckets[ROBOT].rectCol[0] = 64;
	gGT->threadBuckets[ROBOT].rectCol[1] = 128;
	gGT->threadBuckets[ROBOT].rectCol[2] = 64;

	gGT->threadBuckets[STATIC].s_longName = GAME_SDATA_NAME(s_STATIC);
	gGT->threadBuckets[STATIC].s_shortName = GAME_SDATA_NAME(s_STAT);
	gGT->threadBuckets[STATIC].boolCantPause = 1;
	gGT->threadBuckets[STATIC].rectCol[0] = 64;
	gGT->threadBuckets[STATIC].rectCol[1] = 128;
	gGT->threadBuckets[STATIC].rectCol[2] = 128;

	gGT->threadBuckets[MINE].s_longName = GAME_SDATA_NAME(s_MINE);
	gGT->threadBuckets[MINE].s_shortName = GAME_SDATA_NAME(s_MINE);
	gGT->threadBuckets[MINE].boolCantPause = 0;
	gGT->threadBuckets[MINE].rectCol[0] = 128;
	gGT->threadBuckets[MINE].rectCol[1] = 128;
	gGT->threadBuckets[MINE].rectCol[2] = 128;

	gGT->threadBuckets[WARPPAD].s_longName = GAME_SDATA_NAME(s_WARPPAD);
	gGT->threadBuckets[WARPPAD].s_shortName = GAME_SDATA_NAME(s_WRPD);
	gGT->threadBuckets[WARPPAD].boolCantPause = 1;
	gGT->threadBuckets[WARPPAD].rectCol[0] = 128;
	gGT->threadBuckets[WARPPAD].rectCol[1] = 64;
	gGT->threadBuckets[WARPPAD].rectCol[2] = 128;

	gGT->threadBuckets[TRACKING].s_longName = GAME_RDATA_NAME(s_TRACKING);
	gGT->threadBuckets[TRACKING].s_shortName = GAME_SDATA_NAME(s_TRKG);
	gGT->threadBuckets[TRACKING].boolCantPause = 0;
	gGT->threadBuckets[TRACKING].rectCol[0] = 128;
	gGT->threadBuckets[TRACKING].rectCol[1] = 128;
	gGT->threadBuckets[TRACKING].rectCol[2] = 128;

	gGT->threadBuckets[BURST].s_longName = GAME_SDATA_NAME(s_BURST);
	gGT->threadBuckets[BURST].s_shortName = GAME_SDATA_NAME(s_BRST);
	gGT->threadBuckets[BURST].boolCantPause = 0;
	gGT->threadBuckets[BURST].rectCol[0] = 128;
	gGT->threadBuckets[BURST].rectCol[1] = 64;
	gGT->threadBuckets[BURST].rectCol[2] = 128;

	gGT->threadBuckets[BLOWUP].s_longName = GAME_SDATA_NAME(s_BLOWUP);
	gGT->threadBuckets[BLOWUP].s_shortName = GAME_SDATA_NAME(s_BLWP);
	gGT->threadBuckets[BLOWUP].boolCantPause = 0;
	gGT->threadBuckets[BLOWUP].rectCol[0] = 128;
	gGT->threadBuckets[BLOWUP].rectCol[1] = 128;
	gGT->threadBuckets[BLOWUP].rectCol[2] = 0;

	gGT->threadBuckets[TURBO].s_longName = GAME_SDATA_NAME(s_TURBO_again);
	gGT->threadBuckets[TURBO].s_shortName = GAME_SDATA_NAME(s_TRBO);
	gGT->threadBuckets[TURBO].boolCantPause = 0;
	gGT->threadBuckets[TURBO].rectCol[0] = 0;
	gGT->threadBuckets[TURBO].rectCol[1] = 0;
	gGT->threadBuckets[TURBO].rectCol[2] = 128;

	gGT->threadBuckets[SPIDER].s_longName = GAME_SDATA_NAME(s_SPIDER);
	gGT->threadBuckets[SPIDER].s_shortName = GAME_SDATA_NAME(s_SPDR);
	gGT->threadBuckets[SPIDER].boolCantPause = 0;
	gGT->threadBuckets[SPIDER].rectCol[0] = 128;
	gGT->threadBuckets[SPIDER].rectCol[1] = 64;
	gGT->threadBuckets[SPIDER].rectCol[2] = 128;

	gGT->threadBuckets[FOLLOWER].s_longName = GAME_RDATA_NAME(s_FOLLOWER);
	gGT->threadBuckets[FOLLOWER].s_shortName = GAME_SDATA_NAME(s_FLWR);
	gGT->threadBuckets[FOLLOWER].boolCantPause = 0;
	gGT->threadBuckets[FOLLOWER].rectCol[0] = 64;
	gGT->threadBuckets[FOLLOWER].rectCol[1] = 64;
	gGT->threadBuckets[FOLLOWER].rectCol[2] = 128;

	gGT->threadBuckets[STARTTEXT].s_longName = GAME_RDATA_NAME(s_STARTTEXT);
	gGT->threadBuckets[STARTTEXT].s_shortName = GAME_SDATA_NAME(s_STXT);
	gGT->threadBuckets[STARTTEXT].boolCantPause = 0;
	gGT->threadBuckets[STARTTEXT].rectCol[0] = 128;
	gGT->threadBuckets[STARTTEXT].rectCol[1] = 128;
	gGT->threadBuckets[STARTTEXT].rectCol[2] = 64;

	gGT->threadBuckets[OTHER].s_longName = GAME_SDATA_NAME(s_OTHER);
	gGT->threadBuckets[OTHER].s_shortName = GAME_SDATA_NAME(s_OTHR);
	gGT->threadBuckets[OTHER].boolCantPause = 0;
	gGT->threadBuckets[OTHER].rectCol[0] = 128;
	gGT->threadBuckets[OTHER].rectCol[1] = 64;
	gGT->threadBuckets[OTHER].rectCol[2] = 64;

	gGT->threadBuckets[AKUAKU].s_longName = GAME_SDATA_NAME(s_AKUAKU);
	gGT->threadBuckets[AKUAKU].s_shortName = GAME_SDATA_NAME(s_AKUU);
	gGT->threadBuckets[AKUAKU].boolCantPause = 0;
	gGT->threadBuckets[AKUAKU].rectCol[0] = 128;
	gGT->threadBuckets[AKUAKU].rectCol[1] = 64;
	gGT->threadBuckets[AKUAKU].rectCol[2] = 64;

	gGT->threadBuckets[CAMERA].s_longName = GAME_SDATA_NAME(s_CAMERA);
	gGT->threadBuckets[CAMERA].s_shortName = GAME_SDATA_NAME(s_CAMR);
	gGT->threadBuckets[CAMERA].boolCantPause = 1;
	gGT->threadBuckets[CAMERA].rectCol[0] = 128;
	gGT->threadBuckets[CAMERA].rectCol[1] = 128;
	gGT->threadBuckets[CAMERA].rectCol[2] = 64;

	gGT->threadBuckets[HUD].s_longName = GAME_SDATA_NAME(s_HUD);
	gGT->threadBuckets[HUD].s_shortName = GAME_SDATA_NAME(s_THUD);
	gGT->threadBuckets[HUD].boolCantPause = 1;
	gGT->threadBuckets[HUD].rectCol[0] = 128;
	gGT->threadBuckets[HUD].rectCol[1] = 128;
	gGT->threadBuckets[HUD].rectCol[2] = 128;

	gGT->threadBuckets[PAUSE].s_longName = GAME_SDATA_NAME(s_PAUSE);
	gGT->threadBuckets[PAUSE].s_shortName = GAME_SDATA_NAME(s_PAUS);
	gGT->threadBuckets[PAUSE].boolCantPause = 0;
	gGT->threadBuckets[PAUSE].rectCol[0] = 0;
	gGT->threadBuckets[PAUSE].rectCol[1] = 128;
	gGT->threadBuckets[PAUSE].rectCol[2] = 0;

	// particles
	gGT->particleList_ordinary = NULL;
	gGT->particleList_heatWarp = NULL;
	gGT->numParticles = 0;

	// deadc0ed, FUN_8006c684
	// RNG stuff
	gGT->deadcoed_struct.state0 = 0x30215400;
	gGT->deadcoed_struct.state1 = 0x493583fe;

	for (i = 0; i < 12; i++)
	{
		gGT->DecalMP[i].inst = NULL;
		gGT->DecalMP[i].timer = 1000;

		gGT->DecalMP[i].pb.ptrOT = NULL;
		gGT->DecalMP[i].pb.renderBucketOTRangeEnd = NULL;
	}

	MainInit_JitPoolsReset(gGT);

#ifdef CTR_NATIVE
	lev1 = gGT->level1;

	// NOTE(aalhendi): Native menu LEVs may publish no restart table.
	if (lev1->ptr_restart_points != NULL)
#endif
	{
		struct Level *lapLevel = gGT->level1;
		struct CheckpointNode *restartPoints = lapLevel->ptr_restart_points;
		struct GameTracker *lapTracker = GAME_TRACKER;
		int lapScale;
		int lapCount;
		int distance;
		lapCount = lapTracker->numLaps;
		distance = restartPoints[0].distToFinish;
		lapScale = lapCount * 8;
		gGT->trackLength_x_numLaps_x_8 = distance * lapScale;
	}

	MainInit_Drivers(gGT);

	// assume 1P fov
	numPlyr = 1;

	// if you are not in main menu
	if ((gGT->gameMode1 & MAIN_MENU) == 0)
	{
		numPlyr = gGT->numPlyrCurrGame;
	}

	// Initialize four PushBuffer, 4 main screens
	PushBuffer_Init(&gGT->pushBuffer[0], 0, numPlyr);
	PushBuffer_Init(&gGT->pushBuffer[1], 1, numPlyr);
	PushBuffer_Init(&gGT->pushBuffer[2], 2, numPlyr);
	PushBuffer_Init(&gGT->pushBuffer[3], 3, numPlyr);

	{
		struct PushBuffer *pb;

		pb = &gGT->pushBuffer_UI;
		PushBuffer_Init(pb, 0, 1);

		gGT->pushBuffer_UI.rot.x = 0x800;
		PushBuffer_SetPsyqGeom(pb);
		PushBuffer_SetMatrixVP(pb);

		if ((GAME_TRACKER->hudFlags & HUD_FLAG_INIT_UI_INSTANCES) != 0)
		{
			UI_INSTANCE_InitAll();
		}

		GAME_TRACKER->unk1cac[4] = 2;

		for (i = 0; i < 8; i++)
		{
			// get pointer to player structure of each driver
			struct GameTracker *driverTracker = GAME_TRACKER_RELOAD();
			d = driverTracker->drivers[i];

			// if pointer is not nullptr
			if (d == NULL)
			{
				continue;
			}

			inst = d->instSelf;
			if (i < driverTracker->numPlyrCurrGame)
			{
				CAM_Init(&gGT->cameraDC[i], i, d, &gGT->pushBuffer[i]);

				// NOTE(aalhendi): Freeze secondary cameras in the main menu.
				if ((GAME_TRACKER->gameMode1 & MAIN_MENU) != 0 && i > 0)
				{
					gGT->cameraDC[i].flags |= CAMERA_FLAG_FROZEN;
				}
				else
				{
					gGT->cameraDC[i].flags &= ~CAMERA_FLAG_FROZEN;
				}
			}
			if (inst != NULL)
			{
				inst->scale.x = 0xccc;
				inst->scale.y = 0xccc;
				inst->scale.z = 0xccc;
			}
		}

		{
			struct GameTracker *menuTracker = GAME_TRACKER;
			if (menuTracker->levelID == MAIN_MENU_LEVEL)
			{
				// 30 seconds
				menuTracker->demoCountdownTimer = 900;
			}
		}

		// copy InstDef to InstancePool
		INSTANCE_LevInitAll(MAININIT_LEVEL(gGT, lev1)->ptrInstDefs, MAININIT_LEVEL(gGT, lev1)->numInstances);

		// Debug_ToggleNormalSpawn == normal spawn
		if (GAME_TRACKER->Debug_ToggleNormalSpawn != 0)
		{
			MainGameStart_Initialize(gGT, 1);

			if (GAME_TRACKER->boolDemoMode != 0)
			{
				for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
				{
					BOTS_Driver_Convert(GAME_TRACKER->drivers[i]);
				}
			}
		}

		// execute all camera thread update functions
		ThTick_RunBucket(gGT->threadBuckets[CAMERA].thread);

		{
			struct GameTracker *clearTracker = GAME_TRACKER;
			// NOTE(aalhendi): Character alias reads expose the three unsigned bytes of the level word.
			GAME_LEVEL_CLEAR_RED = ((u8 *)&MAININIT_LEVEL(clearTracker, lev1)->clearColorRGBA)[0];
			GAME_LEVEL_CLEAR_GREEN = ((u8 *)&MAININIT_LEVEL(clearTracker, lev1)->clearColorRGBA)[1];
			GAME_LEVEL_CLEAR_BLUE = ((u8 *)&MAININIT_LEVEL(clearTracker, lev1)->clearColorRGBA)[2];
			if (clearTracker->numPlyrCurrGame == 1 && MAININIT_LEVEL(clearTracker, lev1)->clearColor[0].enable != 0 &&
			    MAININIT_LEVEL(clearTracker, lev1)->clearColor[1].enable != 0)
			{
				clearTracker->db[0].drawEnv.isbg = 0;
				GAME_TRACKER_RELOAD()->db[1].drawEnv.isbg = 0;
			}
			else
			{
				GAME_TRACKER_RELOAD()->db[0].drawEnv.isbg = 1;
				GAME_TRACKER_RELOAD()->db[1].drawEnv.isbg = 1;
			}
		}
		{
			struct GameTracker *rgbTracker;
			// NOTE(aalhendi): Each retail component publication reacquires one source/destination tracker.
			rgbTracker = GAME_TRACKER_RELOAD();
			rgbTracker->db[0].drawEnv.r0 = ((u8 *)&MAININIT_LEVEL(rgbTracker, lev1)->clearColorRGBA)[0];
			rgbTracker = GAME_TRACKER_RELOAD();
			rgbTracker->db[0].drawEnv.g0 = ((u8 *)&MAININIT_LEVEL(rgbTracker, lev1)->clearColorRGBA)[1];
			rgbTracker = GAME_TRACKER_RELOAD();
			rgbTracker->db[0].drawEnv.b0 = ((u8 *)&MAININIT_LEVEL(rgbTracker, lev1)->clearColorRGBA)[2];
			rgbTracker = GAME_TRACKER_RELOAD();
			rgbTracker->db[1].drawEnv.r0 = ((u8 *)&MAININIT_LEVEL(rgbTracker, lev1)->clearColorRGBA)[0];
			rgbTracker = GAME_TRACKER_RELOAD();
			rgbTracker->db[1].drawEnv.g0 = ((u8 *)&MAININIT_LEVEL(rgbTracker, lev1)->clearColorRGBA)[1];
			rgbTracker = GAME_TRACKER_RELOAD();
			rgbTracker->db[1].drawEnv.b0 = ((u8 *)&MAININIT_LEVEL(rgbTracker, lev1)->clearColorRGBA)[2];
		}

		{
			struct Level *meshLevel = MAININIT_LEVEL(gGT, lev1);
			if (meshLevel != NULL)
			{
				if (meshLevel->ptr_mesh_info != NULL)
				{
					LevInstDef_UnPack(meshLevel->ptr_mesh_info);
				}
			}
		}

		MainInit_VisMem(gGT);

		MainInit_RainBuffer(gGT);

		// animates water, 1P mode
		{
			struct Level *waterLevel = MAININIT_LEVEL(gGT, lev1);
			AnimateWater1P(gGT->timer, waterLevel->numWaterVertices, waterLevel->ptr_water, waterLevel->ptr_tex_waterEnvMap, waterLevel->visOVertSrc);
		}

		gGT->pushBuffer_UI.fadeFromBlack_desiredResult = 0x1000;
		gGT->pushBuffer_UI.fade_step = 0x200;

		// stars
		gGT->stars.numStars = (s16)(MAININIT_LEVEL(gGT, lev1)->stars.numStars / gGT->numPlyrCurrGame);
		gGT->stars.spread = MAININIT_LEVEL(gGT, lev1)->stars.spread;
		gGT->stars.seed = MAININIT_LEVEL(gGT, lev1)->stars.seed;
		gGT->stars.distance = MAININIT_LEVEL(gGT, lev1)->stars.distance;

		// confetti
		{
			struct GameTracker *endTracker = GAME_TRACKER;
			endTracker->confetti.numParticles_currWord = 0;
			endTracker->confetti.numParticles_max = 0;
			endTracker->confetti.vanishRate = 0;
			endTracker->confetti.velY = -10;

			for (i = 0; i < 4; i++)
			{
				endTracker->winnerIndex[i] = 0;
			}
		}

		{
			struct GameTracker *sceneTracker = GAME_TRACKER;
			if ((sceneTracker->gameMode1 & GAME_CUTSCENE) != 0)
			{
				sceneTracker->cameraDC[0].cameraMode = CAMERA_MODE_FREECAM;
				sceneTracker->hudFlags = 0;
			}
		}

		BOTS_EmptyFunc();

		{
			struct GameTracker *sceneTracker = GAME_TRACKER;
			if ((sceneTracker->gameMode1 & ADVENTURE_ARENA) != 0 && sceneTracker->podiumRewardID != NOFUNC)
			{
				CS_Podium_FullScene_Init();
			}
		}

		if ((GAME_TRACKER->gameMode1 & GAME_CUTSCENE) != 0)
		{
			CS_Cutscene_Start();
		}
		PickupBots_Init();
	}
}

#undef MAININIT_LEVEL
