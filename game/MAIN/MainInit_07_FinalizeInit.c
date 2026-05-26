#include <common.h>

void MainInit_FinalizeInit(struct GameTracker *gGT)
{
	int i;
	int numPlyr;
	u8 *puVar3;
	struct Driver *d;
	struct Level *lev1;
	struct Instance *inst;

	// === Naughty Dog Bug ===
	// Quitting a race while heldItem is warpball,
	// never resets this flag, and then the game
	// can not give warpball again until you reboot
	gGT->gameMode1 &= ~(WARPBALL_HELD);

	// enable collisions with all temporary walls
	// (adv hub doors, tiger temple teeth, etc)
	sdata->doorAccessFlags = 0;

	// add a bookmark
	MEMPACK_PushState();

	gGT->pushBuffer[0].distanceToScreen_PREV = 0x100;
	gGT->pushBuffer[0].distanceToScreen_CURR = 0x100;

	// reset root thread for each bucket
	for (int i = 0; i < NUM_BUCKETS; i++)
		gGT->threadBuckets[i].thread = 0;

	// particles
	gGT->particleList_ordinary = NULL;
	gGT->particleList_heatWarp = NULL;
	gGT->numParticles = 0;

	// deadc0ed, FUN_8006c684
	// RNG stuff
	gGT->deadcoed_struct.unk1 = 0x30215400;
	gGT->deadcoed_struct.unk2 = 0x493583fe;

	for (i = 0; i < 12; i++)
	{
		gGT->DecalMP[i].inst = NULL;
		*(s16 *)&gGT->DecalMP[i].data[0] = 1000;

		gGT->DecalMP[i].ptrOT1 = 0;
		gGT->DecalMP[i].ptrOT2 = 0;
	}

	MainInit_JitPoolsReset(gGT);

	lev1 = gGT->level1;

#ifndef REBUILD_PS1
	// 0x1d7c
	gGT->trackLength_x_numLaps_x_8 = lev1->ptr_restart_points[0].distToFinish * gGT->numLaps * 8;
#endif

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

	struct PushBuffer *pb;

	pb = &gGT->pushBuffer_UI;
	PushBuffer_Init(pb, 0, 1);

	pb->rot[0] = 0x800;
	PushBuffer_SetPsyqGeom(pb);
	PushBuffer_SetMatrixVP(pb);

	if ((gGT->hudFlags & 2) != 0)
	{
		UI_INSTANCE_InitAll();
	}

	gGT->unk1cac[4] = 2;

	for (i = 0; i < 8; i++)
	{
		// get pointer to player structure of each driver
		d = gGT->drivers[i];

		// if pointer is not nullptr
		if (d == NULL)
			continue;

		inst = d->instSelf;
		if (inst != 0)
		{
			inst->scale[0] = 0xccc;
			inst->scale[1] = 0xccc;
			inst->scale[2] = 0xccc;
		}

		if (i < gGT->numPlyrCurrGame)
		{
			CAM_Init(&gGT->cameraDC[i], i, d, &gGT->pushBuffer[i]);

			// freeze camera of P1, only in main menu
			if (((gGT->gameMode1 & MAIN_MENU) == 0) || (i < 1))
			{
				// remove frozen camera flag
				gGT->cameraDC[i].flags &= 0xffff7fff;
			}
			else
			{
				gGT->cameraDC[i].flags |= 0x8000;
			}
		}
	}

	if (gGT->levelID == MAIN_MENU_LEVEL)
	{
		// 30 seconds
		gGT->demoCountdownTimer = 900;
	}

	// copy InstDef to InstancePool
	INSTANCE_LevInitAll(lev1->ptrInstDefs, lev1->numInstances);

	// Debug_ToggleNormalSpawn == normal spawn
	if (gGT->Debug_ToggleNormalSpawn != 0)
	{
		MainGameStart_Initialize(gGT, 1);

		if (gGT->boolDemoMode != 0)
		{
			for (i = 0; i < gGT->numPlyrCurrGame; i++)
			{
#ifndef REBUILD_PS1
				BOTS_Driver_Convert(gGT->drivers[i]);
#endif
			}
		}
	}

#ifndef REBUILD_PS1
	// execute all camera thread update functions
	ThTick_RunBucket(gGT->threadBuckets[CAMERA].thread);
#endif

// dont write unused variables
#if 0
    // lev -> clearColor rgb
    sdata->LevClearColorRGB[0] = (u32)(char *)(lev1->clearColorRGBA)[0];
    sdata->LevClearColorRGB[1] = (u32)(char *)(lev1->clearColorRGBA)[1];
    sdata->LevClearColorRGB[2] = (u32)(char *)(lev1->clearColorRGBA)[2];
#endif

	// Used in Coco Park, encoded as Blue
	*(int *)&gGT->db[0].drawEnv.isbg = lev1->clearColorRGBA << 8;
	*(int *)&gGT->db[1].drawEnv.isbg = lev1->clearColorRGBA << 8;

	if ((gGT->numPlyrCurrGame == 1) && (lev1->clearColor[0].enable != 0) && (lev1->clearColor[1].enable != 0))
	{
		// set isbg of both DBs to false
		gGT->db[0].drawEnv.isbg = 0;
		gGT->db[1].drawEnv.isbg = 0;
	}
	else
	{
		// set isbg of both DBs to true
		gGT->db[0].drawEnv.isbg = 1;
		gGT->db[1].drawEnv.isbg = 1;
	}

	if (lev1 != NULL)
		if (lev1->ptr_mesh_info != NULL)
			LevInstDef_UnPack(lev1->ptr_mesh_info);

#ifndef REBUILD_PS1
	MainInit_VisMem(gGT);
	MainInit_RainBuffer(gGT);

	// animates water, 1P mode
	AnimateWater1P(gGT->timer, lev1->numWaterVertices, lev1->ptr_water, lev1->ptr_tex_waterEnvMap, lev1->unk5);
#elif defined(CTR_NATIVE)
	MainInit_RainBuffer(gGT);
#endif

	gGT->pushBuffer_UI.fadeFromBlack_desiredResult = 0x1000;
	gGT->pushBuffer_UI.fade_step = 0x200;

	numPlyr = gGT->numPlyrCurrGame;

	// stars
	gGT->stars.numStars = (s16)(lev1->stars.numStars / numPlyr);
	gGT->stars.spread = lev1->stars.spread;
	gGT->stars.seed = lev1->stars.seed;
	gGT->stars.distance = lev1->stars.distance;

	// confetti
	gGT->confetti.numParticles_curr = 0;
	gGT->confetti.numParticles_max = 0;
	gGT->confetti.unk2 = 0;
	gGT->confetti.velY = -10;

	for (i = 0; i < 4; i++)
	{
		gGT->winnerIndex[i] = 0;
	}

#if 0
    BOTS_EmptyFunc();
#endif

	if ((gGT->gameMode1 & GAME_CUTSCENE) != 0)
	{
		// freecam mode
		gGT->cameraDC[0].cameraMode = 3;

		// disable all HUD flags
		gGT->hudFlags = 0;

		CS_Cutscene_Start();
	}

#ifndef REBUILD_PS1
	if ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)
		if (gGT->podiumRewardID != NOFUNC) // 0
			CS_Podium_FullScene_Init();

#endif

	PickupBots_Init();
}
