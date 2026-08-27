#include <common.h>

enum
{
	VEH_BIRTH_ADV_RETURN_LEVEL_COUNT = 0x14,
	VEH_BIRTH_DRIVER_BOTTOM_Y_OFFSET = 0x80,
	VEH_BIRTH_COLL_PROBE_TOP_Y_OFFSET = 0x100,
	VEH_BIRTH_DOOR_FORWARD_OFFSET = 800,
	VEH_BIRTH_DOOR_SIDE_OFFSET = 0x200,
	VEH_BIRTH_DOOR_Y_OFFSET = 0x17a,
	VEH_BIRTH_DOOR_EXIT_SPEED = 0xa00,
	VEH_BIRTH_DRIVER_INSTANCE_SCALE = 0xccc,
	VEH_BIRTH_CHEAT_WUMPA_COUNT = 99,
	VEH_BIRTH_CHEAT_ITEM_TURBO = HELD_ITEM_TURBO,
	VEH_BIRTH_CHEAT_ITEM_BOMB = HELD_ITEM_BOMB_1X,
	VEH_BIRTH_CHEAT_ITEM_MASK = HELD_ITEM_MASK,
	VEH_BIRTH_CHEAT_ITEM_COUNT = 9,
	VEH_BIRTH_CHEAT_DURATION = 0x2d00,
	VEH_BIRTH_META_PHYS_COUNT = len(data.metaPhys),
	VEH_BIRTH_WHEEL_SIZE = 0xccc,
	VEH_BIRTH_RESERVED_0x412_INITIAL = 0x600,
	VEH_BIRTH_STEERING_FRAMES_RESET = 10000,
	VEH_BIRTH_QUIP_NONE = -1,
	VEH_BIRTH_PLAYER_THREAD_FLAGS = SIZE_RELATIVE_POOL_BUCKET(DRIVER_NTSC_RETAIL_SIZE, NONE, LARGE, PLAYER),
};


static int VehBirth_IsDoor5InstDef(struct InstDef *instDef)
{
	if (instDef->modelID != STATIC_DOOR)
	{
		return 0;
	}

	if ((instDef->name[0] != 'd') || (instDef->name[1] != 'o') || (instDef->name[2] != 'o') || (instDef->name[3] != 'r') || (instDef->name[4] != '#') ||
	    (instDef->name[5] != '5'))
	{
		return 0;
	}

	for (int i = 6; i < (int)len(instDef->name); i++)
	{
		if (instDef->name[i] != '\0')
		{
			return 0;
		}
	}

	return 1;
}

static struct InstDef *VehBirth_FindDoor5(struct Level *level)
{
	struct InstDef *instDef = level->ptrInstDefs;

	for (int i = 0; i < (int)level->numInstances; i++, instDef++)
	{
		if (VehBirth_IsDoor5InstDef(instDef))
		{
			return instDef;
		}
	}

	return NULL;
}

static int VehBirth_ShouldSpawnOutsideBoss(struct GameTracker *gGT)
{
	if (gGT->podiumRewardID != STATIC_TROPHY)
	{
		return 0;
	}

	for (int i = 0, base = (gGT->levelID - N_SANITY_BEACH) * 4; i < 4; i++)
	{
		int trackID = data.advHubTrackIDs[base + i];

		if (!CHECK_ADV_BIT(sdata->advProgress.rewards, trackID + ADV_REWARD_FIRST_TROPHY))
		{
			return 0;
		}
	}

	if (CHECK_ADV_BIT(sdata->advProgress.rewards, gGT->levelID - N_SANITY_BEACH + ADV_REWARD_FIRST_BOSS_KEY))
	{
		return 0;
	}

	return 1;
}

static int VehBirth_ShouldUseStartlineInAdv(struct GameTracker *gGT, s16 *warppadRot)
{
	int prevLEV = gGT->prevLEV;

	return (prevLEV == MAIN_MENU_LEVEL) || (prevLEV == ADVENTURE_GARAGE) || (prevLEV == -1) || (prevLEV == SCRAPBOOK) ||
	       ((u32)(prevLEV - CREDITS_CRASH) < VEH_BIRTH_ADV_RETURN_LEVEL_COUNT) || (warppadRot == NULL);
}

static struct SpawnPosRot *VehBirth_SpawnType2PosRot(struct Level *level)
{
	return level->ptrSpawnType2_PosRot[1].coords.posRot;
}

static void VehBirth_SetBottomFromPos(SVec3 *posBottom, const SVec3 *pos)
{
	posBottom->x = pos->x;
	posBottom->y = (s16)CTR_MipsAddLo(pos->y, VEH_BIRTH_DRIVER_BOTTOM_Y_OFFSET);
	posBottom->z = pos->z;
}

static u8 VehBirth_GetStartlineIndex(struct Driver *d)
{
	return sdata->kartSpawnOrderArray[d->driverID];
}

static void VehBirth_SetStartlinePosition(struct Driver *d, struct Level *level, SVec3 *posBottom)
{
	u8 spawnIndex = VehBirth_GetStartlineIndex(d);

	d->actionsFlagSet |= ACTION_BEHIND_START_LINE;
#ifdef CTR_NATIVE
	if (level->ptr_restart_points == NULL)
	{
		// NOTE(aalhendi): Retail does an unguarded low-address read here;
		// native cannot dereference PS1 null-space for menu/hub-style LEVs.
		d->distanceToFinish_checkpoint = 0;
	}
	else
	{
#endif
		d->distanceToFinish_checkpoint = level->ptr_restart_points[0].distToFinish << 3;
	}
	VehBirth_SetBottomFromPos(posBottom, &level->DriverSpawn[spawnIndex].pos);
}

static void VehBirth_SetStartlineRotation(struct Driver *d, struct Level *level)
{
	u8 spawnIndex = VehBirth_GetStartlineIndex(d);

	d->rotCurr.x = level->DriverSpawn[spawnIndex].rot.x;
	d->rotCurr.y = ANG_MODULO_TWO_PI(level->DriverSpawn[spawnIndex].rot.y + ANG_HALF_PI);
	d->rotCurr.z = level->DriverSpawn[spawnIndex].rot.z;
}

static int VehBirth_ScaleTrig(int trig, int scale)
{
	return CTR_MipsSra(CTR_MipsMulLo(trig, scale), FRACTIONAL_BITS);
}

void VehBirth_TeleportSelf(struct Driver *d, u8 spawnFlag, int spawnPosY)
{
	SVec3 posTop;
	SVec3 posBottom;
	SVec3 warppadPos;

	struct GameTracker *gGT = sdata->gGT;
	struct Level *level1 = gGT->level1;
	struct Instance *dInst;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);

	struct InstDef *doorInst = NULL;
	struct SpawnPosRot *advSpawn = NULL;
	s16 *warppadRot = NULL;
	int spawnAtBoss;
	int spawnOutsideBoss = 0;

	if ((level1 == NULL) || (level1->ptr_mesh_info == NULL))
	{
		return;
	}

	dInst = d->instSelf;

	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = 0;
	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	}
	sps->ptr_mesh_info = level1->ptr_mesh_info;

	gGT->gameMode2 &= ~VEH_FREEZE_DOOR;
	spawnAtBoss = gGT->gameMode2 & SPAWN_AT_BOSS;

	if ((spawnFlag & VEH_BIRTH_SPAWN_USE_LEVEL_POSITION) == 0)
	{
		posBottom.x = (s16)CTR_MipsSra(d->posCurr.x, FRACTIONAL_BITS_8);
		posBottom.y = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posCurr.y, FRACTIONAL_BITS_8), VEH_BIRTH_DRIVER_BOTTOM_Y_OFFSET);
		posBottom.z = (s16)CTR_MipsSra(d->posCurr.z, FRACTIONAL_BITS_8);
	}
	else
	{
		if ((gGT->podiumRewardID == STATIC_KEY) && (gGT->currAdvProfile.numKeys == 1))
		{
			doorInst = VehBirth_FindDoor5(level1);
		}
		else
		{
			spawnOutsideBoss = VehBirth_ShouldSpawnOutsideBoss(gGT);
		}

		if (spawnAtBoss != 0)
		{
			spawnOutsideBoss = 1;
		}

		if (doorInst != NULL)
		{
			int rotY = doorInst->rot.y;

			gGT->gameMode2 |= VEH_FREEZE_DOOR;
			posBottom.x = doorInst->pos.x + VehBirth_ScaleTrig(MATH_Cos(rotY), VEH_BIRTH_DOOR_FORWARD_OFFSET) +
			              VehBirth_ScaleTrig(MATH_Cos(rotY + ANG_HALF_PI), VEH_BIRTH_DOOR_SIDE_OFFSET);
			posBottom.y = doorInst->pos.y + VEH_BIRTH_DOOR_Y_OFFSET;
			posBottom.z = doorInst->pos.z + VehBirth_ScaleTrig(MATH_Sin(rotY), VEH_BIRTH_DOOR_FORWARD_OFFSET) +
			              VehBirth_ScaleTrig(MATH_Sin(rotY + ANG_HALF_PI), VEH_BIRTH_DOOR_SIDE_OFFSET);
		}
		else if (spawnOutsideBoss != 0)
		{
			advSpawn = VehBirth_SpawnType2PosRot(level1);
			VehBirth_SetBottomFromPos(&posBottom, &advSpawn[1].pos);
		}
		else if ((gGT->gameMode1 & ADVENTURE_ARENA) == 0)
		{
			VehBirth_SetStartlinePosition(d, level1, &posBottom);
		}
		else if (gGT->podiumRewardID == NOFUNC)
		{
			if ((gGT->prevLEV == MAIN_MENU_LEVEL) || (gGT->prevLEV == ADVENTURE_GARAGE) || (gGT->prevLEV == -1) || (gGT->prevLEV == SCRAPBOOK) ||
			    ((u32)(gGT->prevLEV - CREDITS_CRASH) < VEH_BIRTH_ADV_RETURN_LEVEL_COUNT))
			{
				VehBirth_SetStartlinePosition(d, level1, &posBottom);
			}
			else
			{
				warppadRot = AH_WarpPad_GetSpawnPosRot(CTR_VECTOR_DATA(&(warppadPos)));
				VehBirth_SetBottomFromPos(&posBottom, &warppadPos);
			}
		}
		else
		{
			advSpawn = VehBirth_SpawnType2PosRot(level1);
			VehBirth_SetBottomFromPos(&posBottom, &advSpawn[0].pos);
		}
	}

	posTop.x = posBottom.x;
	posTop.y = (s16)CTR_MipsSubLo(posBottom.y, VEH_BIRTH_COLL_PROBE_TOP_Y_OFFSET);
	posTop.z = posBottom.z;

	COLL_SearchBSP_CallbackQUADBLK(&posTop, &posBottom, sps, 0);

	if (sps->boolDidTouchQuadblock == 0)
	{
		d->AxisAngle3_normalVec = (SVec3){.x = 0, .y = FP_ONE, .z = 0};
	}
	else
	{
		d->AxisAngle3_normalVec = sps->hit.plane.normal;
		d->lastValid = sps->hit.ptrQuadblock;
	}

	d->AxisAngle1_normalVec = d->AxisAngle3_normalVec;
	d->AxisAngle2_normalVec = d->AxisAngle3_normalVec;
	d->AxisAngle4_normalVec = d->AxisAngle2_normalVec;

	d->posCurr.x = CTR_MipsSll(sps->Union.QuadBlockColl.hitPos.x, FRACTIONAL_BITS_8);
	d->posCurr.y = CTR_MipsSll(CTR_MipsAddLo(sps->Union.QuadBlockColl.hitPos.y, spawnPosY), FRACTIONAL_BITS_8);
	d->posCurr.z = CTR_MipsSll(sps->Union.QuadBlockColl.hitPos.z, FRACTIONAL_BITS_8);
	d->posPrev.x = d->posCurr.x;
	d->posPrev.y = d->posCurr.y;
	d->posPrev.z = d->posCurr.z;
	d->quadBlockHeight = CTR_MipsSll(sps->Union.QuadBlockColl.hitPos.y, FRACTIONAL_BITS_8);

	if ((spawnFlag & VEH_BIRTH_SPAWN_USE_LEVEL_POSITION) != 0)
	{
		if (doorInst != NULL)
		{
			d->rotCurr.y = ANG_MODULO_TWO_PI(doorInst->rot.y + ANG_PI);
			gGT->gameMode2 &= ~GAME_MODE2_SPAWN_CLEAR_MASK;
		}
		else if (spawnOutsideBoss != 0)
		{
			if (advSpawn == NULL)
			{
				advSpawn = VehBirth_SpawnType2PosRot(level1);
			}

			u16 rotY = ANG_MODULO_TWO_PI(advSpawn[1].rot.y + ANG_HALF_PI);

			d->rotCurr.y = rotY;
			if (spawnAtBoss != 0)
			{
				if (gGT->levelID == CITADEL_CITY)
				{
					rotY += ANG_HALF_PI;
				}
				else if (!((gGT->levelID == GEM_STONE_VALLEY) && (gGT->prevLEV == HOT_AIR_SKYWAY)))
				{
					rotY += ANG_PI;
				}

				d->rotCurr.y = ANG_MODULO_TWO_PI(rotY);
			}

			gGT->gameMode2 &= ~GAME_MODE2_SPAWN_CLEAR_MASK;
		}
		else if ((gGT->gameMode1 & ADVENTURE_ARENA) == 0)
		{
			VehBirth_SetStartlineRotation(d, level1);
		}
		else if (gGT->podiumRewardID != NOFUNC)
		{
			if (advSpawn == NULL)
			{
				advSpawn = VehBirth_SpawnType2PosRot(level1);
			}

			d->rotCurr.y = ANG_MODULO_TWO_PI(advSpawn[0].rot.y);
		}
		else if (VehBirth_ShouldUseStartlineInAdv(gGT, warppadRot))
		{
			VehBirth_SetStartlineRotation(d, level1);
		}
		else
		{
			d->rotCurr.x = warppadRot[0];
			d->rotCurr.y = ANG_MODULO_TWO_PI(warppadRot[1] + ANG_HALF_PI);
			d->rotCurr.z = warppadRot[2];
		}
	}

	d->speed = 0;
	d->speedApprox = 0;
	d->jumpHeightCurr = 0;
	d->jumpHeightPrev = 0;
	d->forwardAccelImpulse = 0;
	d->matrixArray = BAKED_GTE_MATRIX_NONE;
	d->matrixIndex = 0;
	d->jump_LandingBoost = 0;
	d->jumpMeter = 0;
	d->jumpMeterTimer = 0;
	d->turnAngleCurr = 0;
	d->turnAngleLerpVel = 0;
	d->turnAnglePrev = 0;
	d->rotCurr.w = 0;
	d->rotPrev.w = 0;
	d->pendingDamageType = 0;
	d->jumpSquishStretch = 0;
	d->underDriver = 0;
	d->distanceDrivenBackwards = 0;
	d->clockReceive = 0;
	d->revEngineState = 0;

	d->angle = d->rotCurr.y;
	d->rotPrev.x = d->rotCurr.x;
	d->rotPrev.y = d->rotCurr.y;
	d->rotPrev.z = d->rotCurr.z;

	if ((doorInst != NULL) && ((spawnFlag & VEH_BIRTH_SPAWN_USE_LEVEL_POSITION) != 0))
	{
		d->speed = VEH_BIRTH_DOOR_EXIT_SPEED;
	}

	// set animation to zero
	dInst->animIndex = 0;

	dInst->animFrame = VehFrameInst_GetStartFrame(0, VehFrameInst_GetNumAnimFrames(dInst, 0));

	// Set Scale (x, y, z)
	dInst->scale.x = VEH_BIRTH_DRIVER_INSTANCE_SCALE;
	dInst->scale.y = VEH_BIRTH_DRIVER_INSTANCE_SCALE;
	dInst->scale.z = VEH_BIRTH_DRIVER_INSTANCE_SCALE;

	d->actionsFlagSet &= ~(ACTION_AIRBORNE | ACTION_HIGH_JUMP);

	if ((spawnFlag & VEH_BIRTH_SPAWN_INIT_RACE_STATE) == 0)
	{
		return;
	}

	if (dInst->thread->modelIndex == DYNAMIC_PLAYER)
	{
		for (int i = 0; i < DRIVER_FUNC_COUNT; i++)
		{
			d->funcPtrs[i] = NULL;
		}

		CAM_StartOfRace(&gGT->cameraDC[d->driverID]);

		dInst->thread->funcThTick = ((gGT->gameMode1 & (GAME_CUTSCENE | MAIN_MENU)) == 0) ? NULL : VehBirth_NullThread;

		// set OnInit function
		d->funcPtrs[DRIVER_FUNC_INIT] = ((gGT->gameMode1 & ADVENTURE_ARENA) == 0) ? VehStuckProc_RevEngine_Init : VehPhysProc_Driving_Init;
	}

	d->lapIndex = 0;
	d->numWumpas = 0;
	d->lapTime = 0;
	d->distanceToFinish_curr = 0;

	d->actionsFlagSet &= ~(ACTION_RACE_FINISHED | ACTION_BOT);

	if ((gGT->gameMode2 & CHEAT_WUMPA) != 0)
	{
		d->numWumpas = VEH_BIRTH_CHEAT_WUMPA_COUNT;
	}

	d->numHeldItems = 0;
	d->PickupLetterHUD.numCollected = 0;
	d->BattleHUD.numLives = gGT->battleSetup.lifeLimit;

	// no item
	DriverHeldItem weaponId = HELD_ITEM_NONE;
	u32 gameMode2 = gGT->gameMode2;
	if ((gameMode2 & CHEAT_MASK) != 0)
	{
		weaponId = VEH_BIRTH_CHEAT_ITEM_MASK;
	}
	else if ((gameMode2 & CHEAT_TURBO) != 0)
	{
		weaponId = VEH_BIRTH_CHEAT_ITEM_TURBO;
	}
	else if ((gameMode2 & CHEAT_BOMBS) != 0)
	{
		weaponId = VEH_BIRTH_CHEAT_ITEM_BOMB;
	}
	d->heldItemID = weaponId;

	if (weaponId != HELD_ITEM_NONE)
	{
		d->numHeldItems = VEH_BIRTH_CHEAT_ITEM_COUNT;
	}

	if (
	    // If Permanent Invisibility Cheat is Enabled
	    ((gameMode2 & CHEAT_INVISIBLE) != 0) &&

	    // only make players invisible, not AIs
	    (d->driverID < gGT->numPlyrCurrGame))
	{
		dInst->flags &= ~(DRAW_TRANSPARENT | GHOST_DRAW_TRANSPARENT);

		dInst->flags |= GHOST_DRAW_TRANSPARENT;

		d->invisibleTimer = VEH_BIRTH_CHEAT_DURATION;
	}

	if ((gameMode2 & CHEAT_ENGINE) != 0)
	{
		d->superEngineTimer = VEH_BIRTH_CHEAT_DURATION;
	}
}

void VehBirth_TeleportAll(struct GameTracker *gGT, u32 spawnFlags)
{
	// NOTE(aalhendi): Retail ignores this parameter and reloads gGT from globals.
	(void)gGT;
	gGT = sdata->gGT;

	struct Driver *d;

	for (int i = 0; i < (int)len(gGT->drivers); i++)
	{
		d = gGT->drivers[i];

		if (d == NULL)
		{
			continue;
		}

		if (d->instSelf->thread->modelIndex == DYNAMIC_ROBOT_CAR)
		{
			BOTS_GotoStartingLine(d);
		}

		else
		{
			VehBirth_TeleportSelf(d, spawnFlags | VEH_BIRTH_SPAWN_USE_LEVEL_POSITION, 0);
		}
	}
}

enum
{
	VEH_EXTRA_DRIVER_MODEL_COUNT = 3,
};

internal b32 VehBirth_ModelNameEquals(const struct Model *model, const char *name)
{
	for (s32 wordIndex = 0; wordIndex < MODEL_NAME_WORD_COUNT; wordIndex++)
	{
		if (ModelName_ReadWord(model->name, wordIndex) != ModelName_ReadWord(name, wordIndex))
		{
			return false;
		}
	}

	return true;
}

struct Model *VehBirth_GetModelByName(char *searchName)
{
	// array to character models loaded,
	// maximum of 4, used in VS mode
	for (int i = 0; i < VEH_EXTRA_DRIVER_MODEL_COUNT; i++)
	{
		struct Model *m = data.driverModelExtras[i].model;

		if ((m != NULL) && VehBirth_ModelNameEquals(m, searchName))
		{
			// character found, return pointer
			return m;
		}
	}

	struct Model **looseModels = LOAD_GetLooseRacerModelList();

	if ((looseModels != NULL) && (looseModels[0] != NULL))
	{
		for (int i = 0; looseModels[i] != NULL; i++)
		{
			struct Model *m = looseModels[i];

			if (VehBirth_ModelNameEquals(m, searchName))
			{
				return m;
			}
		}
	}

	struct Model **models = (struct Model **)sdata->PLYROBJECTLIST;

	if (
	    // list is valid, and first element is valid
	    (models != NULL) && (models[0] != NULL))
	{
		// loop until all strings are checked (until current is not nullptr)
		for (int i = 0; models[i] != NULL; i++)
		{
			struct Model *m = models[i];

			if (VehBirth_ModelNameEquals(m, searchName))
			{
				// character found, return pointer
				return m;
			}
		}
	}
	return NULL;
}

void VehBirth_SetConsts(struct Driver *driver)
{
	u8 *d = (u8 *)driver;

	int engineID = data.MetaDataCharacters[data.characterIDs[driver->driverID]].engineID;

	for (u32 i = 0; i < VEH_BIRTH_META_PHYS_COUNT; i++)
	{
		struct MetaPhys *metaPhys = &data.metaPhys[i];

		u32 metaPhysSize = metaPhys->size;

		u32 rawValue = (u32)metaPhys->value[engineID];
		u8 *dst = &d[metaPhys->offset];

		if (metaPhysSize == 1)
		{
			dst[0] = (u8)rawValue;
			continue;
		}

		if (metaPhysSize == 2)
		{
			dst[0] = (u8)rawValue;
			dst[1] = (u8)(rawValue >> 8);
			continue;
		}

		if (metaPhysSize == 4)
		{
			dst[0] = (u8)rawValue;
			dst[1] = (u8)(rawValue >> 8);
			dst[2] = (u8)(rawValue >> 16);
			dst[3] = (u8)(rawValue >> 24);
		}
	}

	return;
}

void VehBirth_EngineAudio_AllPlayers(void)
{
	struct GameTracker *gGT = sdata->gGT;

	for (struct Thread *th = gGT->threadBuckets[PLAYER].thread; th != 0; th = th->siblingThread)
	{
		struct Driver *d = th->object;

		u8 driverID = d->driverID;

		int engine = data.MetaDataCharacters[data.characterIDs[driverID]].engineID;

		EngineAudio_InitOnce((engine * 4) + driverID, HOWL_SFX_CENTER_NO_DISTORTION);
	}
}

void VehBirth_NullThread(struct Thread *t)
{
	(void)t;
}

void VehBirth_TireSprites(struct Thread *t)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *d = t->object;
	struct IconGroup *tireAnim = gGT->iconGroup[0];
	int driverID = d->driverID;

	struct Icon **tire = ICONGROUP_GETICONS(tireAnim);
	d->wheelSprites = tire;

	d->wheelSize = VEH_BIRTH_WHEEL_SIZE;

	// compiler might reuse these registers in the IF,
	// first set item to "none" and driverID, then
	// check for Oxide in characterIDs

	d->heldItemID = HELD_ITEM_NONE;
	d->BattleHUD.teamID = driverID;

	if (
	    // if character ID is oxide
	    (data.characterIDs[driverID] == NITROS_OXIDE) && (gGT->levelID != MAIN_MENU_LEVEL))
	{
		d->wheelSize = 0;
	}

	d->tireColor = DRIVER_TIRE_COLOR_DEFAULT;
	d->tireColorCycleTimer = DRIVER_TIRE_COLOR_TIMER_INITIAL;

	d->engineSoundMode = ENGINE_SOUND_DYNAMIC;

	d->AxisAngle1_normalVec.y = FP_ONE;
	d->AxisAngle2_normalVec.y = FP_ONE;
	d->reserved_0x412 = VEH_BIRTH_RESERVED_0x412_INITIAL;
	d->numFramesSpentSteering = VEH_BIRTH_STEERING_FRAMES_RESET;

	d->terrainMeta1 = VehAfterColl_GetTerrain(TERRAIN_NONE);

	d->BattleHUD.numLives = gGT->battleLifeLimit;

	d->quip1 = VEH_BIRTH_QUIP_NONE;
	d->quip3 = VEH_BIRTH_QUIP_NONE;
}

void VehBirth_NonGhost(struct Thread *t, int index)
{
	// model index = DYNAMIC_PLAYER,
	// AI will override this right after
	// the end of the function
	t->modelIndex = DYNAMIC_PLAYER;

	t->driverHitRadius = THREAD_DRIVER_HIT_RADIUS;
	t->driverHitRadiusSquared = THREAD_DRIVER_HIT_RADIUS_SQUARED;
	t->driverCollisionReserved_0x3e = THREAD_DRIVER_HIT_RADIUS;
	t->driverCollisionReserved_0x3c = 0;
	t->driverCollisionReserved_0x40 = 0;

	struct Driver *d = t->object;
	struct GameTracker *gGT = sdata->gGT;

	int id = data.characterIDs[0];
	if ((gGT->gameMode1 & MAIN_MENU) == 0)
	{
		id = data.characterIDs[index];
	}

	struct Model *m = VehBirth_GetModelByName(data.MetaDataCharacters[id].name_Debug);

	struct Instance *inst = INSTANCE_Birth3D(m, m->name, t);

	t->inst = inst;

	// Wake
	m = gGT->modelPtr[STATIC_WAKE];
	if (m != 0)
	{
		inst = INSTANCE_Birth3D(m, m->name, 0);
		d->wakeInst = inst;

		if (inst != 0)
		{
			inst->flags |= HIDE_MODEL | ANIM_LOOP;
		}
	}

	inst = t->inst;
	if (index < gGT->numPlyrCurrGame)
	{
		inst->flags |= OWNER_PUSHBUFFER_GATE;
	}

	d->driverID = index;
	d->instSelf = inst;

	VehBirth_TireSprites(t);
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail leaves terrainMeta2 unset until COLL_FIXED;
	// native cannot dereference the PS1 low-memory null-space before then.
	d->terrainMeta2 = d->terrainMeta1;
#endif
	VehBirth_SetConsts(d);

	// if you are in cutscene or in main menu
	if ((gGT->gameMode1 & GAME_MODE_MENU_OR_CUTSCENE_MASK) != 0)
	{
		// dont update, make invisible
		t->funcThTick = VehBirth_NullThread;
		inst->flags |= HIDE_MODEL;
	}
}

struct Driver *VehBirth_Player(int index)
{
	struct Thread *t = PROC_BirthWithObject(VEH_BIRTH_PLAYER_THREAD_FLAGS, 0, sdata->s_player, 0);

	struct Driver *d = t->object;
	memset(d, 0, DRIVER_NTSC_RETAIL_SIZE);

	VehBirth_NonGhost(t, index);

	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_Driving_Init;

	d->BattleHUD.teamID = sdata->gGT->battleSetup.teamOfEachPlayer[index];

	return d;
}
