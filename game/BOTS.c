#include <common.h>

enum
{
	BOTS_ADV_MAX_LOSS_DIFFICULTY_INDEX = 10,
	BOTS_DIFFICULTY_PARAM_COUNT = 14,
	BOTS_DIFFICULTY_BLEND_DENOMINATOR = 0xf0,
	BOTS_NAV_PATH_COUNT = 3,
	BOTS_MAX_KARTS = 8,
	BOTS_BATTLE_DRIVER_COUNT = 4,
	BOTS_GRID_ROW_KART_COUNT = 4,
	BOTS_BATTLE_PATH_RANDOM_MASK = 0xfff,
	BOTS_BATTLE_PATH_RANDOM_DIVISOR = 0x555,
	BOTS_PATH_CHANGE_PATH_SHIFT = 10,
	BOTS_PATH_CHANGE_FRAME_MASK = 0x3ff,
	BOTS_LEVEL_INST_COLL_RADIUS = 0x19,
	BOTS_MASK_DROP_HEIGHT = 0x10000,
	BOTS_MASK_DURATION = 0x1e00,
	BOTS_AI_COLLISION_SPEED_PENALTY = 3000,
	BOTS_RACE_START_AI_COLLISION_DELAY_FRAMES = CTR_SECONDS_TO_FRAMES(15),

	BOTS_DAMAGE_STATE_SPIN = 1,
	BOTS_DAMAGE_STATE_BLAST = 2,
	BOTS_DAMAGE_STATE_SQUISH = 3,
	BOTS_DAMAGE_STATE_MASK_GRAB = 5,

	BOTS_NAV_FLAG_TURBO = 0x1,
	BOTS_NAV_FLAG_BACK_SKID = 0x2,
	BOTS_NAV_FLAG_FRONT_SKID = 0x4,
	BOTS_NAV_FLAG_SUPER_TURBO = 0x100,
	BOTS_NAV_FLAG_KILLPLANE = 0x200,
	BOTS_NAV_FLAG_JUMP = 0x400,
	BOTS_NAV_FLAG_DRIFT_LEFT = 0x800,
	BOTS_NAV_FLAG_DRIFT_RIGHT = 0x1000,
	BOTS_NAV_FLAG_ENGINE_ECHO = 0x2000,
	BOTS_NAV_FLAG_SPLIT_LINE = 0x8000,
	BOTS_NAV_FLAG_DRIFT_MASK = BOTS_NAV_FLAG_DRIFT_LEFT | BOTS_NAV_FLAG_DRIFT_RIGHT,

	BOTS_NAV_SPECIAL_RAMP_PHYS = 0x10,
	BOTS_NAV_SPECIAL_REFLECTIVE = 0x20,
	BOTS_NAV_SPECIAL_LEVEL_INST_COLL = 0x40,
	BOTS_NAV_SPECIAL_MOON_GRAVITY = 0x80,
	BOTS_NAV_SPECIAL_INDEX_MASK = 0xf,
};

force_inline s16 BOTS_PathChangePathID(s16 changeOpcode)
{
	return (s16)CTR_MipsSra(changeOpcode, BOTS_PATH_CHANGE_PATH_SHIFT);
}

force_inline s16 BOTS_PathChangeFrameIndex(s16 changeOpcode)
{
	return changeOpcode & BOTS_PATH_CHANGE_FRAME_MASK;
}

force_inline s16 BOTS_PathChangeCap(void)
{
	return (s16)CTR_MipsSll(BOTS_NAV_PATH_COUNT, BOTS_PATH_CHANGE_PATH_SHIFT);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012568-0x80012598.
int BOTS_Adv_NumTimesLostEvent(int numLost)
{
	// if you lost more than 10 times
	// the difficulty will not get lower.
	if ((u16)numLost > BOTS_ADV_MAX_LOSS_DIFFICULTY_INDEX)
	{
		// the array apparently has 12, not sure why it stopped at 11.
		numLost = BOTS_ADV_MAX_LOSS_DIFFICULTY_INDEX;
	}

	return data.advDifficulty[numLost];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800123e0-0x80012440
void BOTS_SetGlobalNavData(u16 index)
{
	sdata->lastPathIndex = index;

	sdata->nav_NumPointsOnPath = sdata->NavPath_ptrHeader[index]->numPoints;

	sdata->nav_ptrFirstPoint = sdata->NavPath_ptrNavFrameArray[index];

	sdata->nav_ptrLastPoint = &sdata->nav_ptrFirstPoint[sdata->nav_NumPointsOnPath];

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012440-0x80012560
void BOTS_InitNavPath(struct GameTracker *gGT, s16 index)
{
	struct NavHeader *nh = 0;
	struct NavHeader **LevNavTable = sdata->gGT->level1->LevNavTable;

	if (LevNavTable != 0)
	{
		// nullptr on Nitro Court
		nh = LevNavTable[index];
	}

	// if path exists
	if (nh != 0)
	{
		// grab the data
		sdata->NavPath_ptrHeader[index] = nh;

		sdata->NavPath_ptrNavFrameArray[index] = NAVHEADER_GETFRAME(sdata->NavPath_ptrHeader[index]);

		// if data is outdated
		if (sdata->NavPath_ptrHeader[index]->magicNumber != -0x1303)
		{
			// never mind then
			sdata->NavPath_ptrHeader[index]->numPoints = 0;
		}
	}

	// if no path data is found
	else
	{
		// use a template, which cancels AIs
		sdata->NavPath_ptrHeader[index] = &sdata->blank_NavHeader;

		sdata->NavPath_ptrNavFrameArray[index] = NAVHEADER_GETFRAME(&sdata->blank_NavHeader);

		sdata->NavPath_ptrHeader[index]->numPoints = 0;
	}

	// save number of points
	sdata->nav_NumPointsOnPath = sdata->NavPath_ptrHeader[index]->numPoints;

	// global last point
	sdata->nav_ptrLastPoint = &sdata->NavPath_ptrNavFrameArray[index][sdata->nav_NumPointsOnPath];

	// header last point
	sdata->NavPath_ptrHeader[index]->last = sdata->nav_ptrLastPoint;

	// global first point
	sdata->nav_ptrFirstPoint = sdata->NavPath_ptrNavFrameArray[index];

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012560-0x80012568.
void BOTS_EmptyFunc(void)
{
}

internal void BOTS_Adv_LerpDifficulty(s16 *dst, s16 factor)
{
	s16 *lo = sdata->difficultyParams[1];
	s16 *hi = sdata->difficultyParams[0];

	for (s32 i = 0; i < BOTS_DIFFICULTY_PARAM_COUNT; i++)
	{
		dst[i] = lo[i] + (s16)((factor * (hi[i] - lo[i])) / BOTS_DIFFICULTY_BLEND_DENOMINATOR);
	}
}

internal void BOTS_Adv_CopySpawnOrder(s32 first, s32 second)
{
	*(s32 *)&sdata->kartSpawnOrderArray[0] = first;
	*(s32 *)&sdata->kartSpawnOrderArray[4] = second;
}

internal s32 BOTS_GetTrackDistanceToFinish(struct GameTracker *gGT)
{
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Menu-storage/wrong-warp can leave stale bot threads in
	// levels without restart points. Retail blind-loads from low PSX memory;
	// native uses zero so only stale AI spacing/rubberband math is affected.
	if ((gGT->level1 == NULL) || (gGT->level1->ptr_restart_points == NULL))
	{
		return 0;
	}
#endif

	return CTR_MipsSll(gGT->level1->ptr_restart_points->distToFinish, 3);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012598-0x80013374.
void BOTS_Adv_AdjustDifficulty(void)
{
	struct GameTracker *gGT = sdata->gGT;
	u32 gameMode1 = gGT->gameMode1;
	u32 gameMode2 = gGT->gameMode2;
	s32 currDifficulty;
	s16 cupDifficulty = 0;

	// NOTE(aalhendi): Retail stores params1 in slot 1 and params2 in slot 0.
	if ((gameMode1 & ADVENTURE_BOSS) != 0)
	{
		sdata->difficultyParams[1] = data.BossDifficulty[gGT->bossID].params1;
		sdata->difficultyParams[0] = data.BossDifficulty[gGT->bossID].params2;
	}
	else
	{
		sdata->difficultyParams[1] = data.ArcadeDifficulty[gGT->levelID].params1;
		sdata->difficultyParams[0] = data.ArcadeDifficulty[gGT->levelID].params2;
	}

	if ((gameMode1 & ARCADE_MODE) != 0)
	{
		currDifficulty = (u16)gGT->arcadeDifficulty;

		if ((gameMode2 & CHEAT_SUPERHARD) != 0)
		{
			currDifficulty = 0x140;
		}

		cupDifficulty = (s16)((u16)gGT->arcadeDifficulty + 0x50);
	}
	else if ((gameMode1 & ADVENTURE_CUP) != 0)
	{
		s32 track = gGT->cup.trackIndex;
		s32 lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostCupRace[track]);
		s32 maxDifficulty = track * 5;

		if (gGT->cup.cupID == 4)
		{
			lostModifier -= 0xe1;

			if ((gameMode2 & CHEAT_ADV) != 0)
			{
				maxDifficulty = track * 7;
				lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostCupRace[track]) - 0x141;
			}
		}
		else
		{
			lostModifier -= 0xcd;

			if ((gameMode2 & CHEAT_ADV) != 0)
			{
				maxDifficulty = track * 7;
				lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostCupRace[track]) - 300;
			}
		}

		currDifficulty = maxDifficulty - lostModifier;
		cupDifficulty = (s16)(currDifficulty + 0x50);
	}
	else if ((gameMode1 & ADVENTURE_BOSS) != 0)
	{
		s32 bossID = gGT->bossID;
		s32 lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostBossRace[bossID]) - 0xe1;
		s32 maxDifficulty = bossID * 5;

		if ((gameMode2 & CHEAT_ADV) != 0)
		{
			maxDifficulty = bossID * 7;
			lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostBossRace[bossID]) - 0x141;
		}

		currDifficulty = maxDifficulty - lostModifier;
	}
	else
	{
		s16 numTrophies = (s16)gGT->currAdvProfile.numTrophies + 1;
		s32 lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostRacePerLev[gGT->levelID]) - 0x3c;
		s32 maxDifficulty = numTrophies * 0x23;

		if (maxDifficulty < 0)
		{
			maxDifficulty += 3;
		}

		maxDifficulty >>= 2;

		if ((gameMode2 & CHEAT_ADV) != 0)
		{
			maxDifficulty = numTrophies * 0xc;
			lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostRacePerLev[gGT->levelID]) - 100;
		}

		currDifficulty = maxDifficulty - lostModifier;
	}

	if ((s16)currDifficulty < 0)
	{
		currDifficulty = 0;
	}

	BOTS_Adv_LerpDifficulty(sdata->arcade_difficultyParams, (s16)currDifficulty);

	if (((gameMode1 & ADVENTURE_CUP) != 0) || ((gameMode2 & CUP_ANY_KIND) != 0))
	{
		BOTS_Adv_LerpDifficulty(sdata->cup_difficultyParams, cupDifficulty);
	}

	sdata->aiCollisionDelayFrameCount = 0;

	if ((sdata->const_0x30215400 == 0) && (sdata->const_0x493583fe == 0))
	{
		sdata->const_0x30215400 = 0x30215400;
		sdata->const_0x493583fe = 0x493583fe;
	}

	for (s16 i = 0; i < BOTS_NAV_PATH_COUNT; i++)
	{
		LIST_Clear(&sdata->navBotList[i]);

		if ((gameMode1 & (GAME_CUTSCENE | MAIN_MENU)) == 0)
		{
			BOTS_InitNavPath(gGT, i);
		}
	}

	BOTS_SetGlobalNavData(0);

	gGT->numBotsNextGame = 0;

	if (((gameMode2 & CUP_ANY_KIND) == 0) || (gGT->cup.trackIndex == 0))
	{
		if (gGT->numPlyrCurrGame == 2)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.VS_2P_1, data.kartSpawnOrder.VS_2P_2);
		}
		else if ((u8)gGT->numPlyrCurrGame > 2)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.VS_3P_4P_1, data.kartSpawnOrder.VS_3P_4P_2);
		}

		if ((gameMode1 & (RELIC_RACE | TIME_TRIAL)) != 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.time_trial_1, data.kartSpawnOrder.time_trial_2);
		}
		else if ((gameMode1 & CRYSTAL_CHALLENGE) != 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.crystal_challenge_1, data.kartSpawnOrder.crystal_challenge_2);
		}
		else if ((gameMode1 & ADVENTURE_BOSS) != 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.boss_challenge_1, data.kartSpawnOrder.boss_challenge_2);
		}
		else if (((gameMode1 & ADVENTURE_CUP) != 0) && (gGT->cup.cupID == 4))
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.purple_cup_1, data.kartSpawnOrder.purple_cup_2);
		}
		else if ((gameMode1 & ADVENTURE_MODE) == 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.arcade_1, data.kartSpawnOrder.arcade_2);
		}
	}

	u8 pathOrder[8];
	pathOrder[0] = 0;
	pathOrder[4] = 0;
	pathOrder[3] = 2;
	pathOrder[7] = 2;

	u8 firstPath = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 1;
	pathOrder[1] = firstPath;
	pathOrder[5] = firstPath ^ 1;

	u8 secondPath = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 1;
	pathOrder[2] = secondPath + 1;
	pathOrder[6] = (secondPath ^ 1) + 1;

	for (s16 i = 0; i < BOTS_MAX_KARTS; i++)
	{
		sdata->driver_pathIndexIDs[i] = pathOrder[(u8)sdata->kartSpawnOrderArray[i]];
	}

	if ((gameMode1 & ADVENTURE_BOSS) != 0)
	{
		sdata->driver_pathIndexIDs[0] = 0;
		sdata->driver_pathIndexIDs[1] = 1;
	}

	if ((gameMode1 & BATTLE_MODE) != 0)
	{
		for (s16 i = 0; i < BOTS_BATTLE_DRIVER_COUNT; i++)
		{
			sdata->driver_pathIndexIDs[i] = (RngDeadCoed((u32 *)&sdata->const_0x30215400) & BOTS_BATTLE_PATH_RANDOM_MASK) / BOTS_BATTLE_PATH_RANDOM_DIVISOR;
		}
	}

	if ((((gameMode1 & ADVENTURE_CUP) == 0) && ((gameMode2 & CUP_ANY_KIND) == 0)) || (gGT->cup.trackIndex == 0))
	{
		u8 accelOrder[8];
		u32 accel = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 3;
		u32 rearAccel = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 3;

		for (s16 i = 0; i < BOTS_GRID_ROW_KART_COUNT; i++)
		{
			accelOrder[i] = accel;
			accel = (accel + 1) & 3;

			accelOrder[i + 4] = rearAccel + 4;
			rearAccel = (rearAccel - 1) & 3;
		}

		for (s16 i = 0; i < BOTS_MAX_KARTS; i++)
		{
			sdata->accelerateOrder[i] = accelOrder[(u8)sdata->kartSpawnOrderArray[i]];
		}
	}

	if ((((gameMode1 & ADVENTURE_CUP) != 0) || ((gameMode2 & CUP_ANY_KIND) != 0)) && (gGT->cup.trackIndex > 0))
	{
		s16 bestPoints = -1;
		s16 bestDriverIndex = 0;
		s16 topAccelIndex = 0;

		for (s16 i = 0; i < BOTS_MAX_KARTS; i++)
		{
			if (((u8)gGT->numPlyrCurrGame <= i) && (bestPoints < gGT->cup.points[i]))
			{
				bestPoints = (s16)gGT->cup.points[i];
				bestDriverIndex = i;
			}

			if (sdata->accelerateOrder[i] == 0)
			{
				topAccelIndex = i;
			}
		}

		char topAccel = sdata->accelerateOrder[topAccelIndex];
		sdata->accelerateOrder[topAccelIndex] = sdata->accelerateOrder[bestDriverIndex];
		sdata->accelerateOrder[bestDriverIndex] = topAccel;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013374-0x80013444.
void BOTS_UpdateGlobals(void)
{
	struct GameTracker *gGT = sdata->gGT;

	if (gGT->numBotsNextGame != 0)
	{
		EngineSound_NearestAIs();
	}

	sdata->bestHumanRank = NULL;
	sdata->bestRobotRank = NULL;
	struct Driver *worstRobotDriver = NULL;

	for (int i = BOTS_MAX_KARTS - 1; i >= 0; i--)
	{
		struct Driver *d = gGT->driversInRaceOrder[i];

		if (d == NULL)
		{
			continue;
		}

		if ((d->actionsFlagSet & ACTION_BOT) != 0)
		{
			if (sdata->bestRobotRank == 0)
			{
				worstRobotDriver = d;
			}

			sdata->bestRobotRank = d;
		}
		else
		{
			sdata->bestHumanRank = d;
		}
	}

	if (sdata->bestHumanRank == NULL)
	{
		sdata->bestHumanRank = worstRobotDriver;
	}

	sdata->aiCollisionDelayFrameCount++;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013444-0x800135d8
void BOTS_SetRotation(struct Driver *bot, int useSpawnYaw)
{
	struct NavFrame *nf = bot->botData.botNavFrame;

	CTR_SET_VEC3(bot->botData.aiPhysics.velocity.v, 0, 0, 0);

	// ======== Get Driver Position =============

	bot->botData.estimatePosition.x = (s16)CTR_MipsSra(bot->posCurr.x, 8);
	bot->botData.estimatePosition.y = (s16)CTR_MipsSra(bot->posCurr.y, 8);
	bot->botData.estimatePosition.z = (s16)CTR_MipsSra(bot->posCurr.z, 8);

	// ======== Compare to Nav Position =============

	int dx = CTR_MipsSubLo(nf->pos.x, bot->botData.estimatePosition.x);
	int dy = CTR_MipsSubLo(nf->pos.y, bot->botData.estimatePosition.y);
	int dz = CTR_MipsSubLo(nf->pos.z, bot->botData.estimatePosition.z);

	// ======== Calculate Distance =============

	// xz dist from driver to nav
	int xzDist = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dz, dz)));
	bot->botData.distToNextNavXZ = xzDist;
	// xyz distance from driver to nav
	int xyzDist = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dy, dy)), CTR_MipsMulLo(dz, dz)));
	bot->botData.distToNextNavXYZ = xyzDist;

	// ======== Calculate Rotation =============

	int rot = ratan2(CTR_MipsSll(dy, 0xc), CTR_MipsSll(bot->botData.distToNextNavXZ, 0xc));
	bot->botData.estimateRotCurrY = (u8)CTR_MipsSra(rot, 4);
	bot->botData.navProgressRemainder = 0;

	if (!useSpawnYaw)
	{
		bot->botData.estimateRotNav[0] = nf->rot[0];
		rot = ratan2(CTR_MipsNegLo(dx), CTR_MipsNegLo(dz));
		bot->botData.estimateRotNav[1] = (u8)CTR_MipsSra(CTR_MipsAddLo(rot, 0x800), 4);
		bot->botData.estimateRotNav[2] = nf->rot[1];
	}
	else
	{
		bot->botData.estimateRotNav[1] = (u8)CTR_MipsSra(CTR_MipsAddLo(sdata->gGT->level1->DriverSpawn[0].rot.y, 0x400), 4);
	}

	s16 v = (s16)CTR_MipsSll(bot->botData.estimateRotNav[1], 4);

	// Keep every AI-facing yaw cache in sync with the nav estimate.
	bot->botData.ai_rotY_608 = v;
	bot->angle = v;
	bot->rotCurr.y = v;
	bot->rotPrev.y = v;
	bot->botData.aiRot.y = v;

	bot->botData.botFlags |= BOT_FLAG_ESTIMATE_NAV;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800135d8-0x8001372c.
void BOTS_LevInstColl(struct Thread *botThread)
{
	struct Driver *driver = (struct Driver *)botThread->object;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);

	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES;
	sps->Input1.modelID = DYNAMIC_ROBOT_CAR;
	sps->Union.QuadBlockColl.quadFlagsWanted = 0;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->Input1.hitRadius = BOTS_LEVEL_INST_COLL_RADIUS;

	SVec3 currPos = {
	    .x = (s16)CTR_MipsSra(driver->posCurr.x, 8),
	    .y = (s16)CTR_MipsAddLo(CTR_MipsSra(driver->posCurr.y, 8), BOTS_LEVEL_INST_COLL_RADIUS),
	    .z = (s16)CTR_MipsSra(driver->posCurr.z, 8),
	};
	SVec3 prevPos = {
	    .x = (s16)CTR_MipsSra(driver->posPrev.x, 8),
	    .y = (s16)CTR_MipsAddLo(CTR_MipsSra(driver->posPrev.y, 8), BOTS_LEVEL_INST_COLL_RADIUS),
	    .z = (s16)CTR_MipsSra(driver->posPrev.z, 8),
	};

	COLL_FIXED_BotsSearch(&currPos, &prevPos, sps);

	if (sps->boolDidTouchHitbox == 0)
	{
		return;
	}

	sps->Union.QuadBlockColl.searchFlags &= ~COLL_SEARCH_REUSE_NORMALS;

	if ((sps->bspHitbox->flag & BSP_HITBOX_COLLIDABLE) == 0)
	{
		return;
	}

	struct InstDef *instDef = sps->bspHitbox->data.hitbox.instDef;
	struct Instance *inst = instDef->ptrInstance;
	if (inst == NULL)
	{
		return;
	}

	struct MetaDataMODEL *mdm = COLL_LevModelMeta(instDef->modelID);
	if ((mdm != NULL) && (mdm->LInC != NULL))
	{
		mdm->LInC(inst, botThread, sps);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001372c-0x80013838.
void BOTS_ThTick_RevEngine(struct Thread *botThread)
{
	struct Driver *botDriver = (struct Driver *)botThread->object;
	struct MaskHeadWeapon *mask = botDriver->botData.maskObj;

	if (botDriver->botData.positionBackup.y < botDriver->posCurr.y)
	{ // mask grabbed
		botDriver->posCurr.y = CTR_MipsSubLo(botDriver->posCurr.y, CTR_MipsSra(CTR_MipsSll(sdata->gGT->elapsedTimeMS, 9), 5));

		if (mask != NULL)
		{
			mask->pos.x = (s16)CTR_MipsSra(botDriver->posCurr.x, 8);
			mask->pos.y = (s16)CTR_MipsSra(botDriver->posCurr.y, 8);
			mask->pos.z = (s16)CTR_MipsSra(botDriver->posCurr.z, 8);
		}

		VehPhysForce_TranslateMatrix(botThread, botDriver);
		VehFrameProc_Driving(botThread, botDriver);
		VehEmitter_DriverMain(botThread, botDriver);
	}
	else
	{ // not a mask grab
		if (mask != NULL)
		{
			mask->scale = 0x1000;
			mask->duration = 0;
			mask->rot.z &= 0xfffe;
		}

		botDriver->botData.maskObj = NULL;
		botDriver->kartState = KS_ENGINE_REVVING;
		botDriver->clockReceive = 0;
		botDriver->squishTimer = 0;

		ThTick_SetAndExec(botThread, BOTS_ThTick_Drive);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013838-0x80013a70.
void BOTS_MaskGrab(struct Thread *botThread)
{
	struct Driver *bot = botThread->object;
	struct NavFrame *frame = bot->botData.botNavFrame;
	struct NavFrame *nextFrame = frame + 1;

	// if the next nav point is a farther address than last point
	if (sdata->NavPath_ptrHeader[bot->botData.botPath]->last <= nextFrame)
	{
		// set next nav point to first nav point
		nextFrame = sdata->NavPath_ptrNavFrameArray[bot->botData.botPath];
	}

	bot->kartState = KS_MASK_GRABBED;

	bot->botData.navProgressRemainder = CTR_MipsSll(CTR_MipsDiv(frame->distToNextNavXZ, 2), 8);

	// midpointX between nav frames
	int midpoint = CTR_MipsSll(CTR_MipsAddLo(frame->pos.x, CTR_MipsDiv(CTR_MipsSubLo(nextFrame->pos.x, frame->pos.x), 2)), 8);
	bot->botData.positionBackup.x = midpoint;
	bot->posPrev.x = midpoint;

	// midpointY between nav frames
	midpoint = CTR_MipsSll(CTR_MipsAddLo(frame->pos.y, CTR_MipsDiv(CTR_MipsSubLo(nextFrame->pos.y, frame->pos.y), 2)), 8);
	bot->botData.positionBackup.y = midpoint;
	bot->posPrev.y = midpoint;
	bot->quadBlockHeight = midpoint;

	// midpointZ between nav frames
	midpoint = CTR_MipsSll(CTR_MipsAddLo(frame->pos.z, CTR_MipsDiv(CTR_MipsSubLo(nextFrame->pos.z, frame->pos.z), 2)), 8);
	bot->botData.positionBackup.z = midpoint;
	bot->posPrev.z = midpoint;

	bot->botData.aiPhysics.mulDrift = 0;
	bot->botData.aiPhysics.squishCooldown = 0;
	bot->botData.aiPhysics.reserved_0x5cc = 0;
	bot->botData.aiPhysics.speedY = 0;
	bot->botData.aiPhysics.speedLinear = 0;
	CTR_SET_VEC3(bot->botData.aiPhysics.velocity.v, 0, 0, 0);

	bot->actionsFlagSet |= ACTION_TOUCH_GROUND;

	bot->botData.botFlags &=
	    ~(BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER | BOT_FLAG_FREE_PHYSICS | BOT_FLAG_BOSS_PATH_REQUESTED);

	bot->rotCurr.x = (s16)CTR_MipsSll(frame->rot[0], 4);
	bot->rotCurr.y = (s16)CTR_MipsSll(frame->rot[1], 4);
	bot->rotCurr.z = (s16)CTR_MipsSll(frame->rot[2], 4);

	bot->turbo_MeterRoomLeft = 0;
	bot->reserves = 0;
	bot->clockReceive = 0;
	bot->squishTimer = 0;
	bot->turbo_outsideTimer = 0;
	bot->matrixArray = 0;
	bot->matrixIndex = 0;

	bot->actionsFlagSet &= ~(ACTION_AIRBORNE | ACTION_HIGH_JUMP);

	// if driver is not ghost
	if (botThread->modelIndex != DYNAMIC_GHOST)
	{
		// enable collision for this thread
		botThread->flags &= ~THREAD_FLAG_DISABLE_COLLISION;
	}

	// posY, plus height to be dropped from
	bot->posCurr.x = bot->botData.positionBackup.x;
	bot->posCurr.y = CTR_MipsAddLo(bot->botData.positionBackup.y, BOTS_MASK_DROP_HEIGHT);
	bot->posCurr.z = bot->botData.positionBackup.z;

	struct MaskHeadWeapon *mask = VehPickupItem_MaskUseWeapon(bot, 1);
	bot->botData.maskObj = mask;

	if (mask != 0)
	{
		mask->duration = BOTS_MASK_DURATION;
		mask->rot.z |= 1;
	}

	// execute, then assign per-frame to BOTS_ThTick_RevEngine
	ThTick_SetAndExec(botThread, BOTS_ThTick_RevEngine);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013a70-0x80013c18.
void BOTS_Killplane(struct Thread *botThread)
{
	struct Driver *bot = botThread->object;
	b32 usingTinyArenaOverride = false;

	// check for Tiny Arena
	if (strcmp(sdata->gGT->levelName, rdata.s_asphalt2_thisAppearsTwice) == 0)
	{
		// edge-case override?
		u8 override;
		switch (bot->checkpoint.currentIndex)
		{
		case 0x94:
			override = 0x84;
			break;
		case 0xa0:
			override = 0x80;
			break;
		default:
			override = 0xff;
		}

		if (override != 0xff)
		{
			// pointer to nav point
			struct NavFrame *frame = bot->botData.botNavFrame;

			// goBackCount
			u8 backCount = frame->goBackCount;
			usingTinyArenaOverride = (backCount < (override - 1));

			while ((usingTinyArenaOverride || (override + 1 < backCount)))
			{
				// nav path index
				s16 i = bot->botData.botPath;

				// go back to previous point
				frame -= 1;

				// if this is less than address of first nav point
				if (frame < sdata->NavPath_ptrNavFrameArray[i])
				{
					// go to last nav point
					frame = &sdata->NavPath_ptrHeader[i]->last[-1];
				}

				backCount = frame->goBackCount;
				usingTinyArenaOverride = (backCount < (override - 1));
			}
			bot->botData.botNavFrame = frame;
			usingTinyArenaOverride = true;
		}
	}

	// if not Tiny Arena, or goBackCount didn't happen
	if (!usingTinyArenaOverride)
	{
		// pointer to navFrame
		struct NavFrame *frame = bot->botData.botNavFrame;

		// current nav point (player turned AI)
		u8 currNav = bot->checkpoint.currentIndex;

		// goBackCount
		u8 backCount = frame->goBackCount;

		while ((backCount == currNav || ((frame->flags & 0x4000) != 0)))
		{
			// nav path index
			s16 i = bot->botData.botPath;

			// go back one navFrame
			frame -= 1;

			// if you go back to far
			if (frame < sdata->NavPath_ptrNavFrameArray[i])
			{
				// loop back to last navFrame
				frame = &sdata->NavPath_ptrHeader[i]->last[-1];
			}
			backCount = frame->goBackCount;
			currNav = bot->checkpoint.currentIndex;
		}
		// save ptr to nav frame
		bot->botData.botNavFrame = frame;
	}

	BOTS_MaskGrab(botThread);
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013c18-0x80016b00 for the retail path.

void BOTS_ThTick_Drive(struct Thread *botThread)
{
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	struct Driver *botDriver = (struct Driver *)botThread->object;     // iVar17
	struct Instance *botInstance = (struct Instance *)botThread->inst; // iVar22
	struct GameTracker *gGT = sdata->gGT;

	// local_34 = gGT->elapsedTimeMS;

#if 0
	if(botDriver->driverID != 0)
		return;
#endif

	botDriver->turbo_MeterRoomLeft = 0;
	botDriver->forwardDir = 0;

	botInstance->flags &= ~(SPLIT_LINE | REFLECTIVE);

	if (botDriver->botData.weaponCooldown != 0)
	{
		botDriver->botData.weaponCooldown = (s16)CTR_MipsSubLo((u16)botDriver->botData.weaponCooldown, 1);
	}

	if (botDriver->pendingDamageType == 0)
	{
		if (((botDriver->actionsFlagSet & ACTION_RACE_FINISHED) == 0) && (botDriver->botData.weaponCooldown != 0))
		{
			botDriver->botData.weaponCooldown = (s16)CTR_MipsSubLo((u16)botDriver->botData.weaponCooldown, 1);
		}
	}
	else
	{
		BOTS_ChangeState(botDriver, botDriver->pendingDamageType, botDriver->pendingDamageAttacker, botDriver->pendingDamageReasonByte);
	}

	int elapsedMilliseconds = gGT->elapsedTimeMS; // local_34

	botDriver->reserves = (s16)CTR_MipsSubLo((u16)botDriver->reserves, elapsedMilliseconds);
	if (botDriver->reserves < 0)
	{
		botDriver->reserves = 0;
	}

	botDriver->turbo_outsideTimer = (s16)CTR_MipsSubLo((u16)botDriver->turbo_outsideTimer, elapsedMilliseconds);
	if (botDriver->turbo_outsideTimer < 0)
	{
		botDriver->turbo_outsideTimer = 0;
	}

	botDriver->squishTimer = (s16)CTR_MipsSubLo((u16)botDriver->squishTimer, elapsedMilliseconds);
	if (botDriver->squishTimer < 0)
	{
		botDriver->squishTimer = 0;
	}

	botDriver->burnTimer = (s16)CTR_MipsSubLo((u16)botDriver->burnTimer, elapsedMilliseconds);
	if (botDriver->burnTimer < 0)
	{
		botDriver->burnTimer = 0;
	}

	botDriver->clockReceive = (s16)CTR_MipsSubLo((u16)botDriver->clockReceive, elapsedMilliseconds);
	if (botDriver->clockReceive < 0)
	{
		botDriver->clockReceive = 0;
	}

	botDriver->botData.aiPhysics.driftTarget = 0;
	botDriver->kartState = KS_NORMAL;

	botDriver->actionsFlagSetPrevFrame = botDriver->actionsFlagSet;
	botDriver->actionsFlagSet &= ~(ACTION_DROPPING_MINE | ACTION_JUMP_STARTED | ACTION_STARTED_TOUCH_GROUND);

	int speedApprox = botDriver->speedApprox; // iVar4

	if (speedApprox < 0)
	{
		speedApprox = CTR_MipsNegLo(speedApprox);
	}

	if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		speedApprox = CTR_MipsAddLo(speedApprox, DRIVER_TIRE_COLOR_SPEED_AIRBORNE_BONUS);
	}
	else
	{
		speedApprox = CTR_MipsSra(CTR_MipsSll(speedApprox, 1), 1);
	}

	// NOTE(aalhendi): Retail initializes this boost scale before catch-up can overwrite it.
	u32 local_38 = 1;
	s16 tireColorStep = (s16)CTR_MipsSra(CTR_MipsSll(CTR_MipsAddLo(CTR_MipsMulLo(speedApprox, DRIVER_TIRE_COLOR_SPEED_WEIGHT),
	                                                               CTR_MipsMulLo(botDriver->tireColorCycleStep, DRIVER_TIRE_COLOR_STEP_WEIGHT)),
	                                                 3),
	                                     0xC);
	botDriver->tireColorCycleStep = tireColorStep;

	if ((botDriver->actionsFlagSetPrevFrame & ACTION_ACCEL_PREVENTION) == 0)
	{
		int baseSpeed = botDriver->baseSpeed;
		if (baseSpeed < 0)
		{
			baseSpeed = CTR_MipsNegLo(baseSpeed);
		}

		if (baseSpeed <= DRIVER_TIRE_COLOR_LOW_SPEED_THRESHOLD)
		{
			baseSpeed = botDriver->speedApprox;
			if (baseSpeed < 0)
			{
				baseSpeed = CTR_MipsNegLo(baseSpeed);
			}

			if (baseSpeed <= DRIVER_TIRE_COLOR_LOW_SPEED_THRESHOLD)
			{
				goto UpdateTireColorTimer;
			}
		}

		botDriver->tireColorCycleTimer = (s16)CTR_MipsSubLo((u16)botDriver->tireColorCycleTimer, tireColorStep);
	}
UpdateTireColorTimer:

	if ((botDriver->tireColorCycleTimer < 1) && ((botDriver->tireColor & 1) == 0))
	{
		botDriver->tireColorCycleTimer = DRIVER_TIRE_COLOR_TIMER_RESET;
		botDriver->tireColor = DRIVER_TIRE_COLOR_DARK;
	}
	else
	{
		botDriver->tireColor = DRIVER_TIRE_COLOR_DEFAULT;
	}

	struct NavFrame *navFrameCurr = &botDriver->botData.estimateNavFrame; // psVar19
	struct NavFrame *navFrameNext;                                        // psVar21

	if ((botDriver->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) == 0)
	{
		navFrameCurr = botDriver->botData.botNavFrame;
		navFrameNext = navFrameCurr + 1;

		int pathID = botDriver->botData.botPath;

		if (navFrameNext >= sdata->NavPath_ptrHeader[pathID]->last)
		{
			navFrameNext = sdata->NavPath_ptrNavFrameArray[pathID];
		}
	}
	else
	{
		navFrameNext = botDriver->botData.botNavFrame;
	}

	struct DriverCollisionSearch driverSearch;
	CTR_SET_VEC3(driverSearch.bucket.pos.v, (s16)CTR_MipsSra(botDriver->posCurr.x, 8), (s16)CTR_MipsSra(botDriver->posCurr.y, 8),
	             (s16)CTR_MipsSra(botDriver->posCurr.z, 8));
	driverSearch.bucket.th = NULL;
	driverSearch.bucket.bestDistSq = 0x7fffffff;

	if ((botThread->flags & (THREAD_FLAG_DEAD | THREAD_FLAG_DISABLE_COLLISION)) == 0)
	{
		if (botThread->modelIndex == DYNAMIC_PLAYER)
		{
			PROC_CollidePointWithBucket(botThread->siblingThread, &driverSearch.bucket);
			PROC_CollidePointWithBucket(gGT->threadBuckets[ROBOT].thread, &driverSearch.bucket);
		}
		else if (botThread->modelIndex == DYNAMIC_ROBOT_CAR)
		{
			PROC_CollidePointWithBucket(botThread->siblingThread, &driverSearch.bucket);
		}
	}

	struct Thread *hitThread = driverSearch.bucket.th;
	if (hitThread != NULL)
	{
		int combinedRadius = CTR_MipsAddLo(botThread->driverHitRadius, hitThread->driverHitRadius);
		if (driverSearch.bucket.bestDistSq < CTR_MipsMulLo(combinedRadius, combinedRadius))
		{
			Vec3 selfVelocity = {
			    .x = CTR_MipsAddLo(botDriver->xSpeed, botDriver->botData.aiPhysics.accel.x),
			    .y = CTR_MipsAddLo(botDriver->ySpeed, botDriver->botData.aiPhysics.accel.y),
			    .z = CTR_MipsAddLo(botDriver->zSpeed, botDriver->botData.aiPhysics.accel.z),
			};
			VehPhysCrash_AnyTwoCars(botThread, &driverSearch, &selfVelocity);
		}
	}

	int deltaPosThisFrame; // iVar4

	if (botDriver->botData.ai_progress_cooldown == 0)
	{
		int trafficLightsTimer = gGT->trafficLightsTimer;

		if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE) // check is oxide
		{
			// Pretend there is less time (oxide is a cheater)
			trafficLightsTimer = CTR_MipsSubLo(trafficLightsTimer, 0x1e0);
		}

		if (0 < trafficLightsTimer)
		{
			goto HoldBotBeforeRaceOrCooldown; // if race not started, then skip
		}

		if ((gGT->boolDemoMode != 0) && ((botDriver->botData.botFlags & BOT_FLAG_DEMO_CAMERA_STARTED) == 0) && (botThread->modelIndex == DYNAMIC_PLAYER))
		{
			botDriver->botData.botFlags |= BOT_FLAG_DEMO_CAMERA_STARTED;
			CAM_EndOfRace(&gGT->cameraDC[botDriver->driverID], botDriver);
		}

		if ((botDriver->botData.botFlags & BOT_FLAG_STARTLINE_INIT_DONE) == 0)
		{
			// first frame of race
			botDriver->botData.botFlags |= BOT_FLAG_STARTLINE_INIT_DONE;

			if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)
			{ // if oxide, then talk
				Voiceline_RequestPlay(0, 0xf, 0x10);
			}

			if (( // if in front row & 25% chance
			        (sdata->kartSpawnOrderArray[botDriver->driverID] < 3) && ((RngDeadCoed((uint32_t *)&sdata->const_0x30215400) & 0xFF) < 0x40)) ||
			    (data.characterIDs[botDriver->driverID] == NITROS_OXIDE))
			{ // start the race with a boost
				VehFire_Increment(botDriver, 0x2d0, 1, 0x180);

				VehFire_Audio(botDriver, 0x180);
			}
		}

		// between these start and end tags, there's confusing variable lifetime that needs to be looked at closer.

		u32 botFlags = botDriver->botData.botFlags; // uVar8

		// if path change is requested, but not started (BOSS ONLY)
		if ((botFlags & (BOT_FLAG_BOSS_PATH_REQUESTED | BOT_FLAG_BOSS_PATH_ACTIVE)) == BOT_FLAG_BOSS_PATH_REQUESTED)
		{
			s16 changeOpcode = navFrameNext->pathChangeOpcode;
			s16 newPathID = BOTS_PathChangePathID(changeOpcode);
			s16 newFrameIndex = BOTS_PathChangeFrameIndex(changeOpcode);
			s16 cap = BOTS_PathChangeCap(); // 0xC00, cant have path[3]

			if ((changeOpcode < cap) && (newPathID == botDriver->botData.desiredPath_BossOnly))
			{
				// record that change is requested (boss only)
				botDriver->botData.botFlags = botFlags | BOT_FLAG_BOSS_PATH_ACTIVE;

				s16 oldPathID = botDriver->botData.botPath;
				botDriver->botData.botPath = newPathID;

				LIST_RemoveMember(&sdata->navBotList[oldPathID], &botDriver->botData.item);
				LIST_AddFront(&sdata->navBotList[newPathID], &botDriver->botData.item);

				struct NavFrame *firstNavFrameOnPath = sdata->NavPath_ptrNavFrameArray[newPathID];
				botDriver->botData.botNavFrame = &firstNavFrameOnPath[newFrameIndex];

				BOTS_SetRotation(botDriver, 0);

				navFrameNext = botDriver->botData.botNavFrame;
				navFrameCurr = &botDriver->botData.estimateNavFrame;
			}
		}
		else
		{
			if (BOTS_RACE_START_AI_COLLISION_DELAY_FRAMES < sdata->aiCollisionDelayFrameCount)
			{
				struct Driver *otherDriver = NULL; // iVar4
				if ((botFlags & BOT_FLAG_ESTIMATE_NAV) == 0)
				{
					int iVar3 = 1000;
					s16 sVar7 = 1000;
					struct BotData *botData;

					for (botData = (struct BotData *)LIST_GetFirstItem(&sdata->navBotList[botDriver->botData.botPath]); botData != NULL;
					     botData = (struct BotData *)LIST_GetNextItem((struct Item *)botData), sVar7 = (s16)iVar3)
					{
						struct Driver *driverFromBotData = (struct Driver *)((char *)botData - offsetof(struct Driver, botData));

						if (driverFromBotData == botDriver)
						{
							continue;
						}


						// Find the signed index difference between nav frames on this path.
						int iVar13 = (int)(botData->botNavFrame - botDriver->botData.botNavFrame);

						// if "other" botData driver is behind "this" botDriver driver,
						if (iVar13 < 0)
						{
							// assume number of points "away" is large (add track length)
							iVar13 = CTR_MipsAddLo(iVar13, sdata->NavPath_ptrHeader[botDriver->botData.botPath]->numPoints);
						}

						// find closest "other" botData driver
						// that is AHEAD of "this" botDriver driver
						if (iVar13 < iVar3)
						{
							// save number of points ahead,
							// and save pointer to other driver
							iVar3 = iVar13;
							otherDriver = driverFromBotData;
						}
					}

					// If two drivers are within 3 navframe points of each other
					if ((otherDriver != NULL) && (sVar7 < 3))
					{
						int diff = CTR_MipsSubLo(botDriver->distanceToFinish_curr, otherDriver->distanceToFinish_curr);

						// if "this" bot is closer to finish than "other" bot, do nothing,
						// because "diff" will be incremented largely and fail the <0x200 check
						if (diff < 0)
						{
							diff = CTR_MipsAddLo(diff, BOTS_GetTrackDistanceToFinish(gGT));
						}

						// if "this" bot driver is behind "other" driver, and very close to them,
						// while also being on the same nav path, then change the nav path
						if (diff < 0x200)
						{
							s16 changeOpcode = navFrameNext->pathChangeOpcode;
							s16 newPathID = BOTS_PathChangePathID(changeOpcode);
							s16 newFrameIndex = BOTS_PathChangeFrameIndex(changeOpcode);
							s16 cap = BOTS_PathChangeCap(); // 0xC00, cant have path[3]

							if (changeOpcode < cap)
							{
								s16 oldPathID = botDriver->botData.botPath;
								botDriver->botData.botPath = newPathID;

								LIST_RemoveMember(&sdata->navBotList[oldPathID], &botDriver->botData.item);
								LIST_AddFront(&sdata->navBotList[newPathID], &botDriver->botData.item);

								struct NavFrame *firstNavFrameOnPath = sdata->NavPath_ptrNavFrameArray[newPathID];
								botDriver->botData.botNavFrame = &firstNavFrameOnPath[newFrameIndex & BOTS_PATH_CHANGE_FRAME_MASK];

								BOTS_SetRotation(botDriver, 0);

								navFrameNext = botDriver->botData.botNavFrame;
								navFrameCurr = &botDriver->botData.estimateNavFrame;
							}
						}
					}
				}
			}
		}

		// puVar5 = gGT but different?

		if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
		{
			int ZYsqr = CTR_MipsAddLo(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, botDriver->botData.aiPhysics.speedLinear),
			                          CTR_MipsMulLo(botDriver->botData.aiPhysics.speedY, botDriver->botData.aiPhysics.speedY));

			if (0x2b110000 < ZYsqr) // sqrZY
			{
				int ZY = SquareRoot0_stub(ZYsqr);

				int zVel = CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, 0x6900); // iVar3

#if 0 // in the OG game
				if (ZY == 0)
				{
					//trap(0x1c00);
				}
				if ((ZY == -1) && (zVel == -0x80000000))
				{
					//trap(0x1800);
				}
#endif
				int yVel = CTR_MipsMulLo(botDriver->botData.aiPhysics.speedY, 0x6900); // iVar15
#if 0                                                                                  // in the OG game
				if (ZY == 0)
				{
					//trap(0x1c00);
				}
				if ((ZY == -1) && (yVel == -0x80000000))
				{
					//trap(0x1800);
				}
#endif
				botDriver->botData.aiPhysics.speedLinear = CTR_MipsDiv(zVel, ZY);
				botDriver->botData.aiPhysics.speedY = CTR_MipsDiv(yVel, ZY);
			}
		}
		else
		{
			botDriver->actionsFlagSet &= ~ACTION_AIRBORNE;

			struct Driver *bestDriverRank = sdata->bestHumanRank; // iVar4

			if ((gGT->gameMode1 & RELIC_RACE) != 0)
			{
				bestDriverRank = sdata->bestRobotRank;
			}

			int botVelocity; // iVar3

			if ((bestDriverRank == NULL) || (bestDriverRank == botDriver))
			{
				botVelocity = gGT->constVal_9000;
			}
			else
			{
				int driverRank = botDriver->driverRank; // uVar8
				b32 isInAdvArcadeOrVSCup = false;       // bVar1

				if (((gGT->gameMode1 & ADVENTURE_CUP) != 0) || ((gGT->gameMode2 & CUP_ANY_KIND) != 0))
				{
					isInAdvArcadeOrVSCup = true;
				}

				if (gGT->drivers[0]->driverRank < botDriver->driverRank)
				{
					driverRank--;
				}

				if (1 < gGT->numPlyrCurrGame && gGT->drivers[1]->driverRank <= driverRank)
				{
					driverRank--;
				}

				if (isInAdvArcadeOrVSCup)
				{
					driverRank = sdata->accelerateOrder[botDriver->driverID];
					if (sdata->accelerateOrder[0] < 2)
					{
						driverRank--;
					}
					if (sdata->accelerateOrder[1] < 2)
					{
						driverRank--;
					}
				}

				s16 difficultyStat; // sVar7

				if (bestDriverRank->lapIndex == 0)
				{
					difficultyStat = sdata->arcade_difficultyParams[0xB];
				}
				else
				{
					if (bestDriverRank->lapIndex == gGT->numLaps - 1)
					{
						difficultyStat = sdata->arcade_difficultyParams[0xD];
					}
					else
					{
						difficultyStat = sdata->arcade_difficultyParams[0xC];
					}
				}

				int distToFinish = BOTS_GetTrackDistanceToFinish(gGT); // iVar3

				int lapIndex = bestDriverRank->lapIndex; // uVar20

				if ((bestDriverRank->actionsFlagSet & ACTION_BEHIND_START_LINE) != 0)
				{
					lapIndex--;
				}

				int botLapIndex = botDriver->lapIndex; // uVar18

				if ((botDriver->actionsFlagSet & ACTION_BEHIND_START_LINE) != 0)
				{
					botLapIndex--;
				}

				// not super meaningful, probably just a difficulty statistic
				int complexDifficultyStat = CTR_MipsSubLo(
				    CTR_MipsSubLo(CTR_MipsAddLo(CTR_MipsSubLo(distToFinish, bestDriverRank->distanceToFinish_curr), CTR_MipsMulLo(lapIndex, distToFinish)),
				                  CTR_MipsAddLo(CTR_MipsSubLo(distToFinish, botDriver->distanceToFinish_curr), CTR_MipsMulLo(botLapIndex, distToFinish))),
				    CTR_MipsAddLo(sdata->arcade_difficultyParams[driverRank], difficultyStat));

				int otherDifficultyStat; // iVar13
				if (isInAdvArcadeOrVSCup && ((driverRank & 0xffff) == 0))
				{
					if (complexDifficultyStat < 1)
					{
						otherDifficultyStat = sdata->arcade_difficultyParams[0x9];
					}
					else
					{
						if (botDriver->lapIndex < gGT->numLaps - 1)
						{
							otherDifficultyStat = sdata->cup_difficultyParams[0x8];
						}
						else
						{
							otherDifficultyStat = sdata->cup_difficultyParams[0x8] + sdata->cup_difficultyParams[0xA];
						}
					}
				}
				else
				{
					if (complexDifficultyStat < 1)
					{
						otherDifficultyStat = sdata->arcade_difficultyParams[0x9];
					}
					else
					{
						if (gGT->numLaps - 1 <= botDriver->lapIndex)
						{
							otherDifficultyStat = sdata->arcade_difficultyParams[0x8] + sdata->arcade_difficultyParams[0xA];
						}
						else
						{
							otherDifficultyStat = sdata->arcade_difficultyParams[0x8];
						}
					}
				}
				botVelocity = complexDifficultyStat; // iVar3
				if (complexDifficultyStat < 0)
				{
					botVelocity = CTR_MipsNegLo(complexDifficultyStat);
				}
				int iVar9 = otherDifficultyStat;
				if (otherDifficultyStat < 0)
				{
					iVar9 = CTR_MipsNegLo(otherDifficultyStat);
				}

				botVelocity = CTR_MipsSra(CTR_MipsMulLo(iVar9, CTR_MipsDiv(CTR_MipsSll(CTR_MipsAddLo(botVelocity, 0x80), 0xc), 0xa00)), 0xc);
				local_38 = 0 < complexDifficultyStat;
				botVelocity = CTR_MipsAddLo(botVelocity, CTR_MipsMulLo(CTR_MipsDiv(CTR_MipsSll(botVelocity, 3), 100), CTR_MipsSubLo(7, driverRank)));

				if (iVar9 < botVelocity)
				{
					botVelocity = iVar9;
				}
				if (otherDifficultyStat < 0)
				{
					botVelocity = CTR_MipsNegLo(iVar9);
				}

				int bestDriverWumpaCount = bestDriverRank->numWumpas; // iVar15, assume max of 9?
				if (9 < bestDriverWumpaCount)
				{
					bestDriverWumpaCount = 9;
				}

				int turboMult = bestDriverRank->turboConst; // iVar13, max of 5
				if (5 < turboMult)
				{
					turboMult = 5;
				}

				int netSpeedStat = CTR_MipsSubLo(
				    CTR_MipsDiv(CTR_MipsSll(CTR_MipsSubLo(bestDriverRank->const_AccelSpeed_ClassStat, bestDriverRank->const_Speed_ClassStat), 0xc), 5), 1);

				int additionalVelocity = CTR_MipsAddLo(
				    CTR_MipsDiv(CTR_MipsSll(bestDriverRank->const_Speed_ClassStat, 3), 10),
				    CTR_MipsSra(CTR_MipsAddLo(CTR_MipsDiv(CTR_MipsMulLo(bestDriverWumpaCount, netSpeedStat), 10), CTR_MipsMulLo(turboMult, netSpeedStat)),
				                0xc));

				if (additionalVelocity > 0x6900)
				{
					additionalVelocity = 0x6900;
				}

				botVelocity = CTR_MipsAddLo(botVelocity, additionalVelocity);

				if (botVelocity < 0x5dc1)
				{
					if (botVelocity < 0x1c20)
					{
						botVelocity = 0x1c20;
					}
				}
				else
				{
					botVelocity = 24000;
				}
			}

			if (botDriver->reserves == 0)
			{
				botDriver->botData.botFlags &= ~BOT_FLAG_NAV_BOOST_ACTIVE;
			}
			else
			{
				if ((botDriver->botData.botFlags & BOT_FLAG_NAV_BOOST_ACTIVE) == 0)
				{
					botVelocity = CTR_MipsAddLo(botVelocity, botDriver->fireSpeedCap);
				}
				else
				{
					botVelocity = CTR_MipsAddLo(botDriver->fireSpeedCap, 10000);
				}
			}

			if (botDriver->clockReceive != 0 || botDriver->squishTimer != 0)
			{
				botVelocity = CTR_MipsSra(CTR_MipsMulLo(botVelocity, 0xc00), 0xc);
			}

			if ((botDriver->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0)
			{
				if (botDriver->instTntRecv != NULL || botDriver->thCloud != NULL)
				{
					int damagedVelocityPenalty = CTR_MipsSra(botDriver->const_DamagedSpeed, 1); // iVar4

					botVelocity = CTR_MipsSubLo(botVelocity, damagedVelocityPenalty);
				}
			}
			else
			{
				botDriver->botData.aiPhysics.turboMeter = 0;

				if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)
				{
					int damagedVelocityPenalty = CTR_MipsSra(botDriver->const_DamagedSpeed, 2); // iVar4

					botVelocity = CTR_MipsSubLo(botVelocity, damagedVelocityPenalty);
				}
				else
				{
					botVelocity = CTR_MipsSubLo(botVelocity, botDriver->const_DamagedSpeed);
				}
			}
			if (botVelocity < 0)
			{
				botVelocity = 0;
			}

			struct Terrain *botTerrain = botDriver->terrainMeta1; // iVar15

			botDriver->botData.aiPhysics.speedLinear =
			    CTR_MipsSubLo(botDriver->botData.aiPhysics.speedLinear,
			                  CTR_MipsSra(CTR_MipsMulLo(botDriver->const_PedalFriction_Forward, botTerrain->botFrictionScale), 8)); // iVar4

			if (botDriver->botData.aiPhysics.speedLinear < 0)
			{
				botDriver->botData.aiPhysics.speedLinear = 0;
			}

			if (0x6900 < botVelocity)
			{
				botVelocity = 0x6900;
			}

			int velocityAccountingForTerrain = CTR_MipsSra(CTR_MipsMulLo(botVelocity, botTerrain->botTargetSpeedScale), 8); // iVar4

			if ((botTerrain->botSpeedFlags & TERRAIN_BOT_FLAG_DECEL_TO_TARGET_SPEED) == 0)
			{
			CheckAccelerationTowardTerrainTarget:
				if (botDriver->botData.aiPhysics.speedLinear < velocityAccountingForTerrain)
				{
				ApplyTerrainAcceleration:;
					s16 accel; // sVar7
					if (botDriver->reserves < 1)
					{
						accel = botDriver->const_Accel_ClassStat;
					}
					else
					{
						accel = botDriver->const_Accel_Reserves;
					}
					botVelocity = CTR_MipsSra(CTR_MipsMulLo(accel, botTerrain->botAccelerationScale), 8);

					if (botDriver->botData.botAccel != 0)
					{
						botDriver->botData.botAccel--;
						botVelocity = CTR_MipsSra(CTR_MipsMulLo(botVelocity, CTR_MipsSubLo(0x100, CTR_MipsMulLo(sdata->AI_AccelFrameSteps,
						                                                                                        sdata->accelerateOrder[botDriver->driverID]))),
						                          8);
					}

					botDriver->botData.aiPhysics.speedLinear = CTR_MipsAddLo(botDriver->botData.aiPhysics.speedLinear, botVelocity);
				}
			}
			else
			{
				botVelocity = CTR_MipsSra(botDriver->botData.aiPhysics.speedLinear, 1);

				if (botDriver->botData.aiPhysics.speedLinear < velocityAccountingForTerrain)
				{
					goto ApplyTerrainAcceleration;
				}

				botDriver->botData.aiPhysics.speedLinear = botVelocity;
				if (botVelocity < velocityAccountingForTerrain)
				{
					botDriver->botData.aiPhysics.speedLinear = velocityAccountingForTerrain;
					goto CheckAccelerationTowardTerrainTarget;
				}
			}

			int levelID = gGT->levelID; // iVar3

			if ((levelID == HOT_AIR_SKYWAY || levelID == PAPU_PYRAMID) || levelID == POLAR_PASS)
			{
				int botPathIndex = botDriver->botData.botPath; // iVar3

				if (levelID == POLAR_PASS)
				{
					botPathIndex = CTR_MipsAddLo(botPathIndex, 3);
				}
				else
				{
					if (levelID == PAPU_PYRAMID)
					{
						botPathIndex = CTR_MipsAddLo(botPathIndex, 6);
					}
					else if (levelID == SLIDE_COLISEUM)
					{
						botPathIndex = CTR_MipsAddLo(botPathIndex, 9);
					}
				}

				int navFrameIndexOnPath = (int)(navFrameCurr - sdata->NavPath_ptrNavFrameArray[botDriver->botData.botPath]);

				if ((data.botsThrottle[botPathIndex] <= navFrameIndexOnPath) && (navFrameIndexOnPath < CTR_MipsAddLo(data.botsThrottle[botPathIndex], 0xb)) &&
				    (9000 < botDriver->botData.aiPhysics.speedLinear))
				{
					botDriver->botData.aiPhysics.turboMeter = 0;

					botDriver->botData.aiPhysics.speedLinear =
					    CTR_MipsSubLo(botDriver->botData.aiPhysics.speedLinear, CTR_MipsAddLo(100, botDriver->const_Accel_ClassStat));
				}
			}

			if ((u8)0x80u < navFrameCurr->rot[3])
			{
				velocityAccountingForTerrain = CTR_MipsAddLo(velocityAccountingForTerrain, botDriver->const_SlopeForwardSpeedBonus);

				if (botDriver->botData.aiPhysics.speedLinear < velocityAccountingForTerrain)
				{
					u32 var = (u32)navFrameCurr->rot[3];
					int sinOfAngle = MATH_Sin(CTR_MipsSll(var, 4));

					botDriver->botData.aiPhysics.speedLinear =
					    CTR_MipsSubLo(botDriver->botData.aiPhysics.speedLinear,
					                  CTR_MipsSra(CTR_MipsMulLo(botDriver->const_Gravity, sinOfAngle), 0xc)); // force on a slope due to gravity
				}

				botDriver->fireSpeed = velocityAccountingForTerrain;
			}
		}

		if (0x6400 < botDriver->botData.aiPhysics.speedLinear)
		{
			botDriver->botData.aiPhysics.speedLinear = 0x6400;
		}

		deltaPosThisFrame = CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, elapsedMilliseconds), 5); // iVar4
		if (deltaPosThisFrame < 0)
		{
			deltaPosThisFrame = 0;
		}
		deltaPosThisFrame = CTR_MipsAddLo(deltaPosThisFrame, botDriver->botData.navProgressRemainder);
	}
	else
	{
	HoldBotBeforeRaceOrCooldown:
		botInstance->scale.x = 0xccc;
		botInstance->scale.y = 0xccc;
		botInstance->scale.z = 0xccc;

		botDriver->botData.aiPhysics.speedLinear = 0;

		if (botDriver->botData.ai_progress_cooldown != 0)
		{
			botDriver->botData.ai_progress_cooldown--;
		}

		deltaPosThisFrame = botDriver->botData.navProgressRemainder;
	}

	int navFrameFlags = navFrameCurr->flags;
	int navFrameSpecialBits = navFrameCurr->specialBits;
	// local_3c == 0
	if ((navFrameSpecialBits & BOTS_NAV_SPECIAL_MOON_GRAVITY) != 0)
	{
		botDriver->botData.botFlags |= BOT_FLAG_MOON_GRAVITY;
	}

	int gravity;
	if ((botDriver->botData.botFlags & BOT_FLAG_MOON_GRAVITY) == 0)
	{
		gravity = botDriver->const_Gravity;
	}
	else
	{
		gravity = CTR_MipsDiv(CTR_MipsMulLo(botDriver->const_Gravity, 41), 100);
	}

	botDriver->botData.aiPhysics.speedY =
	    CTR_MipsSubLo(botDriver->botData.aiPhysics.speedY, CTR_MipsSra(CTR_MipsMulLo(gravity, elapsedMilliseconds), 5)); // iVar3

	if (botDriver->botData.aiPhysics.speedY < -0x5000)
	{
		botDriver->botData.aiPhysics.speedY = -0x5000;
	}

	botDriver->botData.positionBackup.y =
	    CTR_MipsAddLo(botDriver->botData.positionBackup.y, CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedY, elapsedMilliseconds), 5));

	s16 navDist; // sVar7

	if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		navDist = navFrameCurr->distToNextNavXZ;
	}
	else
	{
		navDist = navFrameCurr->distToNextNavXYZ;
	}

	int local_3c = 0;
	b32 useGroundedNavDistance = false;
	if ((navFrameCurr->specialBits & BOTS_NAV_SPECIAL_RAMP_PHYS) != 0)
	{
		local_3c = navFrameCurr->specialBits;
	}
	int iVar15 = CTR_MipsSra(deltaPosThisFrame, 8);
	int iVar3;

	for (iVar3 = navDist; iVar3 <= iVar15; /* decrement inside loop */)
	{
		navFrameCurr = navFrameNext;

		deltaPosThisFrame = CTR_MipsSubLo(deltaPosThisFrame, CTR_MipsSll(iVar3, 8));

		int index = botDriver->botData.botPath; // index = iVar13

		navFrameNext = NAVFRAME_GETNEXTFRAME(navFrameCurr);

		iVar15 = CTR_MipsSubLo(iVar15, iVar3);

		if (navFrameNext >= sdata->NavPath_ptrHeader[index]->last)
		{
			navFrameNext = sdata->NavPath_ptrNavFrameArray[index];
		}

		if ((CTR_MipsSra(botDriver->botData.positionBackup.y, 8) < navFrameNext->pos.y) && ((navFrameCurr->flags & BOTS_NAV_FLAG_KILLPLANE) != 0))
		{
			BOTS_Killplane(botThread);
		}

		navFrameFlags |= navFrameCurr->flags;
		navFrameSpecialBits |= navFrameCurr->specialBits;

		int uVar8;
		if ((botDriver->botData.botFlags & BOT_FLAG_BOSS_PATH_ACTIVE) == 0)
		{
			uVar8 = ~BOT_FLAG_ESTIMATE_NAV;
		}
		else
		{
			uVar8 = ~(BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_BOSS_PATH_REQUESTED | BOT_FLAG_BOSS_PATH_ACTIVE);
		}

		botDriver->botData.botFlags &= uVar8;

		s16 nextNavDist; // sVar7

		// MUST be unsigned,
		// this makes a range-value check, and makes negatives positive,
		// so its really checking value between 0x31 and 0x31+0x9e
		u8 compare = navFrameCurr->rot[3] - 0x31;
		if (!useGroundedNavDistance && (0x9e < compare))
		{
			if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
			{
				nextNavDist = navFrameCurr->distToNextNavXZ;
			}
			else
			{
				nextNavDist = navFrameCurr->distToNextNavXYZ;
			}
		}
		else
		{
			useGroundedNavDistance = true;

			nextNavDist = navFrameCurr->distToNextNavXZ;
		}

		iVar3 = nextNavDist;

		if ((navFrameCurr->specialBits & BOTS_NAV_SPECIAL_RAMP_PHYS) != 0)
		{
			local_3c = navFrameCurr->specialBits;
		}
	}

	botDriver->botData.navProgressRemainder = deltaPosThisFrame;

	int navActionFlags = ((navFrameFlags & BOTS_NAV_FLAG_BACK_SKID) != 0) ? ACTION_BACK_SKID : 0;

	if ((navFrameFlags & BOTS_NAV_FLAG_ENGINE_ECHO) != 0)
	{
		navActionFlags |= ACTION_ENGINE_ECHO;
	}

	if ((navFrameFlags & BOTS_NAV_FLAG_FRONT_SKID) != 0)
	{
		navActionFlags |= ACTION_FRONT_SKID;
	}

	botDriver->actionsFlagSet &= ~(ACTION_ENGINE_ECHO | ACTION_BACK_SKID | ACTION_FRONT_SKID);
	botDriver->actionsFlagSet |= navActionFlags;

	struct Terrain *terrain = VehAfterColl_GetTerrain(((uint8_t)navFrameCurr->flags) >> 3);

	botDriver->terrainMeta1 = terrain;

	if ((navFrameCurr->specialBits & BOTS_NAV_SPECIAL_REFLECTIVE) != 0)
	{
		s16 vertSplit;
		if ((navFrameCurr->specialBits & BOTS_NAV_SPECIAL_INDEX_MASK) == 0)
		{
			vertSplit = gGT->level1->splitLines[0];
		}
		else
		{
			vertSplit = gGT->level1->splitLines[1];
		}

		botInstance->vertSplit = vertSplit;

		botInstance->flags |= REFLECTIVE;
	}

	if (((navFrameCurr->specialBits & (BOTS_NAV_SPECIAL_RAMP_PHYS | BOTS_NAV_SPECIAL_REFLECTIVE)) == 0) && (botThread->modelIndex != DYNAMIC_GHOST))
	{
		int transparency = CTR_MipsMulLo(navFrameCurr->specialBits & BOTS_NAV_SPECIAL_INDEX_MASK, 0x9c00);

		botDriver->alphaScaleBackup = (s16)CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo((u16)botDriver->alphaScaleBackup, 100), transparency), 8);

		botInstance->alphaScale = (s16)CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo((u16)botInstance->alphaScale, 100), transparency), 8);
	}

	if (((botDriver->actionsFlagSet & ACTION_FRONT_SKID) == 0) || ((navActionFlags & (ACTION_BACK_SKID | ACTION_FRONT_SKID)) == 0))
	{
		botDriver->turbo_MeterRoomLeft = 0;
		botDriver->botData.aiPhysics.turboMeter = 0;
		botDriver->botData.aiPhysics.fireLevel = 0;
	}
	else
	{
		botDriver->actionsFlagSet |= ACTION_TURBO_INPUT_LATCH;
		botDriver->botData.aiPhysics.turboMeter = (s16)CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.turboMeter, elapsedMilliseconds);
		int uVar6;
		// this ugly tree of ifs may have been a switch perhaps?
		if ((botDriver->botData.aiPhysics.turboMeter < 0xb41) || (5 < botDriver->botData.aiPhysics.fireLevel))
		{
			if ((botDriver->botData.aiPhysics.turboMeter < 0x961) || (uVar6 = 5, 4 < botDriver->botData.aiPhysics.fireLevel))
			{
				if ((botDriver->botData.aiPhysics.turboMeter < 0x781) || (3 < botDriver->botData.aiPhysics.fireLevel))
				{
					if ((0x5a0 < botDriver->botData.aiPhysics.turboMeter) && (uVar6 = 3, botDriver->botData.aiPhysics.fireLevel < 3))
					{
						goto SetTurboChargeThreshold;
					}
					if ((botDriver->botData.aiPhysics.turboMeter < 0x3c1) || (1 < botDriver->botData.aiPhysics.fireLevel))
					{
						if ((0x1e0 < botDriver->botData.aiPhysics.turboMeter) && (uVar6 = 1, botDriver->botData.aiPhysics.fireLevel < 1))
						{
							goto SetTurboChargeThreshold;
						}
					}
					else
					{
						// trigger a turbo boost?
						botDriver->botData.aiPhysics.fireLevel = 2;
						botDriver->turbo_MeterRoomLeft = 0;
						VehFire_Increment(botDriver, 0xf0, 2, CTR_MipsSll(local_38, 7));
					}
				}
				else
				{
					// trigger a turbo boost?
					botDriver->botData.aiPhysics.fireLevel = 4;
					botDriver->turbo_MeterRoomLeft = 0;
					VehFire_Increment(botDriver, 0x1e0, 2, CTR_MipsSll(local_38, 8));
				}
			}
			else
			{
			SetTurboChargeThreshold:
				// trigger a turbo boost?
				botDriver->botData.aiPhysics.fireLevel = uVar6;
				botDriver->turbo_MeterRoomLeft = 0xa0;
			}
		}
		else
		{
			// trigger a turbo boost?
			botDriver->botData.aiPhysics.fireLevel = 6;
			botDriver->turbo_MeterRoomLeft = 0;
			VehFire_Increment(botDriver, 0x2d0, 2, CTR_MipsMulLo(local_38, 0x180));
		}
	}

	if ((navFrameFlags & BOTS_NAV_FLAG_SUPER_TURBO) != 0)
	{
		VehFire_Increment(botDriver, 0x78, 1, 0x900);

		botDriver->botData.botFlags |= BOT_FLAG_NAV_BOOST_ACTIVE;
	}

	if ((navFrameFlags & BOTS_NAV_FLAG_TURBO) != 0)
	{
		VehFire_Increment(botDriver, 0x2d0, 1, 0x180);

		botDriver->botData.botFlags |= BOT_FLAG_NAV_BOOST_ACTIVE;
	}

	if ((botDriver->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) == 0)
	{
		botDriver->botData.botNavFrame = navFrameCurr;

		s16 botPath = botDriver->botData.botPath;

		navFrameNext = NAVFRAME_GETNEXTFRAME(navFrameCurr);

		if (navFrameNext >= sdata->NavPath_ptrHeader[botPath]->last)
		{
			navFrameNext = sdata->NavPath_ptrNavFrameArray[botPath];
		}
	}

	int percentage; // iVar13
	if (iVar3 == 0)
	{
		percentage = 0;
	}
	else
	{
		percentage = CTR_MipsDiv(CTR_MipsSll(iVar15, 0xc), iVar3);
	}

	botDriver->posPrev.x = botDriver->posCurr.x;
	botDriver->posPrev.y = botDriver->posCurr.y;
	botDriver->posPrev.z = botDriver->posCurr.z;

	botDriver->rotPrev.x = botDriver->rotCurr.x;
	botDriver->rotPrev.y = botDriver->rotCurr.y;
	botDriver->rotPrev.z = botDriver->rotCurr.z;

	botDriver->botData.positionBackup.x = CTR_MipsSll(
	    CTR_MipsAddLo(navFrameCurr->pos.x, CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSubLo(navFrameNext->pos.x, navFrameCurr->pos.x), percentage), 0xc)), 8);
	botDriver->quadBlockHeight = CTR_MipsSll(
	    CTR_MipsAddLo(navFrameCurr->pos.y, CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSubLo(navFrameNext->pos.y, navFrameCurr->pos.y), percentage), 0xc)), 8);
	botDriver->botData.positionBackup.z = CTR_MipsSll(
	    CTR_MipsAddLo(navFrameCurr->pos.z, CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSubLo(navFrameNext->pos.z, navFrameCurr->pos.z), percentage), 0xc)), 8);

	if ((botDriver->botData.botFlags & BOT_FLAG_FREE_PHYSICS) != 0)
	{
		botDriver->botData.aiPhysics.accel.y = 0;
		botDriver->botData.aiPhysics.velocity.x = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.x, botDriver->botData.aiPhysics.accel.x);
		botDriver->botData.aiPhysics.velocity.y = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.y, botDriver->botData.aiPhysics.accel.y);
		botDriver->botData.aiPhysics.velocity.z = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.z, botDriver->botData.aiPhysics.accel.z);
		int preAccelX = botDriver->botData.aiPhysics.accel.x; // iVar3
		int preAccelZ = botDriver->botData.aiPhysics.accel.z; // iVar15
		botDriver->botData.aiPhysics.accel.x = CTR_MipsSra(botDriver->botData.aiPhysics.accel.x, 1);
		botDriver->botData.aiPhysics.accel.z = CTR_MipsSra(botDriver->botData.aiPhysics.accel.z, 1);
		botDriver->botData.aiPhysics.velocity.y = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.y, botDriver->botData.aiPhysics.accel.y);

		botDriver->botData.aiPhysics.velocity.x = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.x, preAccelX);
		botDriver->botData.aiPhysics.velocity.z = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.z, preAccelZ);

		int preX = botDriver->botData.aiPhysics.velocity.x; // iVar3
		if (preX != 0)
		{
			if (preX < 1)
			{
				botDriver->botData.aiPhysics.velocity.x = CTR_MipsAddLo(preX, 0x444);
				if (0 < CTR_MipsAddLo(preX, 0x444))
				{
					botDriver->botData.aiPhysics.velocity.x = 0;
				}
				else
				{
					if (botDriver->botData.aiPhysics.accel.x == 0)
					{
						botDriver->botData.aiPhysics.accel.x = 0x444;
					}
				}
			}
			else
			{
				botDriver->botData.aiPhysics.velocity.x = CTR_MipsSubLo(preX, 0x444);
				if (CTR_MipsSubLo(preX, 0x444) < 0)
				{
					botDriver->botData.aiPhysics.velocity.x = 0;
				}
				else
				{
					if (botDriver->botData.aiPhysics.accel.x == 0)
					{
						botDriver->botData.aiPhysics.accel.x = -0x444;
					}
				}
			}
		}
		int preZ = botDriver->botData.aiPhysics.velocity.z; // iVar3
		if (preZ != 0)
		{
			if (preZ < 1)
			{
				botDriver->botData.aiPhysics.velocity.z = CTR_MipsAddLo(preZ, 0x444);
				if (0 < CTR_MipsAddLo(preZ, 0x444))
				{
					botDriver->botData.aiPhysics.velocity.z = 0;
				}
				else
				{
					if (botDriver->botData.aiPhysics.accel.z == 0)
					{
						botDriver->botData.aiPhysics.accel.z = 0x444;
					}
				}
			}
			else
			{
				botDriver->botData.aiPhysics.velocity.z = CTR_MipsSubLo(preZ, 0x444);
				if (CTR_MipsSubLo(preZ, 0x444) < 0)
				{
					botDriver->botData.aiPhysics.velocity.z = 0;
				}
				else
				{
					if (botDriver->botData.aiPhysics.accel.z == 0)
					{
						botDriver->botData.aiPhysics.accel.z = -0x444;
					}
				}
			}
		}
		if ((botDriver->botData.aiPhysics.velocity.x == 0) && (botDriver->botData.aiPhysics.velocity.z == 0))
		{
			botDriver->botData.botFlags &= ~BOT_FLAG_FREE_PHYSICS;
		}
	}

	if ((botDriver->botData.botFlags & (BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_FREE_PHYSICS)) == 0)
	{
		botDriver->botData.ai_quadblock_checkpointIndex = navFrameCurr->goBackCount;
	}
	else
	{
		int deltaRotY = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[1], 4), CTR_MipsSll(navFrameCurr->rot[1], 4)) & 0xfff;
		if (0x7ff < deltaRotY)
		{
			deltaRotY = CTR_MipsSubLo(deltaRotY, 0x1000);
		}

		botDriver->botData.aiRot.y = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[1], 4), CTR_MipsSra(CTR_MipsMulLo(deltaRotY, percentage), 0xc)) & 0xfff;

		s16 probeY = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.y, botDriver->botData.aiPhysics.velocity.y), 8);
		s16 probeX = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.x, botDriver->botData.aiPhysics.velocity.x), 8);
		s16 probeZ = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.z, botDriver->botData.aiPhysics.velocity.z), 8);
		SVec3 probeTop = {
		    .x = probeX,
		    .y = (s16)CTR_MipsSubLo(probeY, 0x100),
		    .z = probeZ,
		};
		SVec3 probeBottom = {
		    .x = probeX,
		    .y = (s16)CTR_MipsAddLo(probeY, 0x80),
		    .z = probeZ,
		};

		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
		sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
		sps->Union.QuadBlockColl.quadFlagsIgnored = QUADBLOCK_FLAG_NO_COLLISION_RESPONSE;
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;

		COLL_SearchBSP_CallbackQUADBLK(&probeTop, &probeBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			botDriver->quadBlockHeight = CTR_MipsSll(sps->Union.QuadBlockColl.hitPos.y, 8);

			botDriver->botData.ai_quadblock_checkpointIndex = sps->hit.ptrQuadblock->checkpointIndex;

			VehPhysForce_RotAxisAngle(&botInstance->matrix, sps->hit.plane.normal.v, botDriver->botData.aiRot.y);

			botDriver->AxisAngle3_normalVec = sps->hit.plane.normal;

			botInstance->bitCompressed_NormalVector_AndDriverIndex =
			    INST_CompressNormalVectorAndDriverIndex(sps->hit.plane.normal.x, sps->hit.plane.normal.y, sps->hit.plane.normal.z, botDriver->driverID);

			if ((sps->hit.ptrQuadblock->quadFlags & QUADBLOCK_FLAG_KILL_PLANE) != 0)
			{
				BOTS_Killplane(botThread);
			}
		}
	}
	deltaPosThisFrame = CTR_MipsSra(deltaPosThisFrame, 8);
	if (botDriver->botData.positionBackup.y < botDriver->quadBlockHeight)
	{
		u32 oldBotFlags = botDriver->botData.botFlags; // uVar8
		botDriver->botData.botFlags &= ~BOT_FLAG_MOON_GRAVITY;

		if ((navFrameCurr->flags & BOTS_NAV_FLAG_KILLPLANE) == 0)
		{
			if ((oldBotFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0)
			{
				if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
				{
					if (botDriver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
					{
						int mapped = VehCalc_MapToRange(CTR_MipsSubLo(botDriver->jumpHeightCurr, botDriver->jumpHeightPrev), 0x300, 0x1400, 0x4b, 200); // uVar8

						int volume; // uVar20
						if (mapped < 0)
						{
							volume = 0;
						}
						else
						{
							volume = mapped & 0xff;

							if (0xff < mapped)
							{
								volume = 0xff;
							}
						}
						u32 echo = ((botDriver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
						int flags = (int)HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, volume, echo);

						OtherFX_Play_LowLevel(7, 1, flags);
					}
					int iVar3 = navFrameCurr->distToNextNavXZ;
					if (iVar3 != 0)
					{
#if 0
						if (iVar3 == 0)
						{
							trap(0x1c00);
						}
						if ((iVar3 == -1) && (deltaPosThisFrame * navFrameCurr->distToNextNavXYZ == -0x80000000))
						{
							trap(0x1800);
						}
#endif
						botDriver->botData.navProgressRemainder =
						    CTR_MipsSll(CTR_MipsDiv(CTR_MipsMulLo(deltaPosThisFrame, navFrameCurr->distToNextNavXYZ), iVar3), 8);
					}
					s16 sVar7 = botDriver->jump_LandingBoost;

					if (sVar7 < 0x5a1)
					{
						if (0x3c0 < sVar7)
						{
							deltaPosThisFrame = CTR_MipsMulLo(local_38, 0x60);
							goto doFireIncrement;
						}
						else if (0x280 < sVar7)
						{
							deltaPosThisFrame = 0;
							goto doFireIncrement;
						}
					}
					else
					{
						deltaPosThisFrame = CTR_MipsMulLo(local_38, 0xc0);
					doFireIncrement:
						VehFire_Increment(botDriver, 0x2d0, 2, deltaPosThisFrame);
					}

					botDriver->actionsFlagSet |= ACTION_STARTED_TOUCH_GROUND;
				}
				deltaPosThisFrame = CTR_MipsSubLo(botDriver->quadBlockHeight, botDriver->posPrev.y);

				botDriver->botData.positionBackup.y = botDriver->quadBlockHeight;

				botDriver->botData.aiPhysics.speedY = deltaPosThisFrame;

				if ((navFrameFlags & BOTS_NAV_FLAG_JUMP) != 0 || (botDriver->instTntRecv != NULL))
				{
					int oldActionsFlags = botDriver->actionsFlagSet;

					botDriver->actionsFlagSet |= ACTION_JUMP_STARTED;

					botDriver->botData.aiPhysics.speedY = CTR_MipsAddLo(deltaPosThisFrame, 0x1400);

					if (botThread->modelIndex == DYNAMIC_PLAYER)
					{
						OtherFX_Play_Echo(8, 1, (oldActionsFlags & ACTION_ENGINE_ECHO) != 0);
					}
				}
				if (16000 < botDriver->botData.aiPhysics.speedY)
				{
					botDriver->botData.aiPhysics.speedY = 16000;
				}

				if ((navFrameFlags & BOTS_NAV_FLAG_DRIFT_MASK) == 0)
				{
					botDriver->botData.aiPhysics.driftTarget = 0;

					botDriver->kartState = KS_NORMAL;
				}
				else
				{
					botDriver->kartState = KS_DRIFTING;

					if ((navFrameFlags & BOTS_NAV_FLAG_DRIFT_LEFT) == 0)
					{
						botDriver->botData.aiPhysics.driftTarget = 0x2aa;
					}
					else
					{
						botDriver->botData.aiPhysics.driftTarget = -0x2aa;
					}
				}
			}
			else
			{
				if (((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0))
				{
					int iVar3 = navFrameCurr->distToNextNavXZ;
					if (iVar3 != 0)
					{
#if 0
						if (iVar3 == 0)
						{
							trap(0x1c00);
						}
						if ((iVar3 == -1) && (deltaPosThisFrame * navFrameCurr->distToNextNavXYZ == -0x80000000))
						{
							trap(0x1800);
						}
#endif
						botDriver->botData.navProgressRemainder =
						    CTR_MipsSll(CTR_MipsDiv(CTR_MipsMulLo(deltaPosThisFrame, navFrameCurr->distToNextNavXYZ), iVar3), 8);
					}
				}
				deltaPosThisFrame = CTR_MipsSra(CTR_MipsNegLo(botDriver->botData.aiPhysics.speedY), 1);

				botDriver->botData.aiPhysics.speedY = deltaPosThisFrame;

				if (deltaPosThisFrame < 0)
				{
					botDriver->botData.aiPhysics.speedY = CTR_MipsNegLo(deltaPosThisFrame);
				}

				u8 bVar10 = (u8)CTR_MipsAddLo(botDriver->botData.blastBounceCount, 1);
				botDriver->botData.blastBounceCount = bVar10;

				if (botDriver->botData.aiDamageState == BOTS_DAMAGE_STATE_SPIN)
				{
					botDriver->actionsFlagSet |= ACTION_BACK_SKID | ACTION_FRONT_SKID;
				}
				else
				{
					if (botDriver->botData.aiDamageState == BOTS_DAMAGE_STATE_BLAST && (2 < bVar10))
					{
						botDriver->botData.ai_progress_cooldown = 10;
						botDriver->botData.aiPhysics.speedLinear = 0;
						botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
					}
				}

				botDriver->botData.positionBackup.y = botDriver->quadBlockHeight;
			}
		}
		else
		{
			BOTS_Killplane(botThread);
		}

		botDriver->jump_LandingBoost = 0;
		botDriver->actionsFlagSet |= ACTION_TOUCH_GROUND;
	}
	else
	{
		if (!useGroundedNavDistance && ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) != 0))
		{
			int iVar3 = navFrameCurr->distToNextNavXYZ;
			if (iVar3 != 0)
			{
#if 0
				if (iVar3 == 0)
				{
					trap(0x1c00);
				}
				if ((iVar3 == -1) && (deltaPosThisFrame * navFrameCurr->distToNextNavXZ == -0x80000000))
				{
					trap(0x1800);
				}
#endif
				botDriver->botData.navProgressRemainder = CTR_MipsSll(CTR_MipsDiv(CTR_MipsMulLo(deltaPosThisFrame, navFrameCurr->distToNextNavXZ), iVar3), 8);
			}
		}

		botDriver->actionsFlagSet &= ~ACTION_TOUCH_GROUND;
		botDriver->actionsFlagSet |= ACTION_AIRBORNE;

		botDriver->jump_LandingBoost = (s16)CTR_MipsAddLo((u16)botDriver->jump_LandingBoost, elapsedMilliseconds);
	}

	int iVar4_lifetime_3 = 0x18;

	if (botDriver->botData.aiPhysics.driftTarget != 0)
	{
		iVar4_lifetime_3 = 0x60;
	}

	s16 iVar3_lifetime_2 = (s16)CTR_MipsSubLo((u16)botDriver->botData.aiPhysics.mulDrift, iVar4_lifetime_3);
	if (botDriver->botData.aiPhysics.driftTarget < botDriver->botData.aiPhysics.mulDrift)
	{
		botDriver->botData.aiPhysics.mulDrift = iVar3_lifetime_2;

		if (iVar3_lifetime_2 < botDriver->botData.aiPhysics.driftTarget)
		{
			botDriver->botData.aiPhysics.mulDrift = botDriver->botData.aiPhysics.driftTarget;
		}
	}
	else
	{
		iVar4_lifetime_3 = (s16)CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.mulDrift, iVar4_lifetime_3);

		botDriver->botData.aiPhysics.mulDrift = iVar4_lifetime_3;

		if (botDriver->botData.aiPhysics.driftTarget < iVar4_lifetime_3)
		{
			botDriver->botData.aiPhysics.mulDrift = botDriver->botData.aiPhysics.driftTarget;
		}
	}
	botDriver->multDrift = botDriver->botData.aiPhysics.mulDrift;

	if ((botDriver->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) != 0)
	{
		u8 newKartState; // uVar2
		s16 sVar7 = botDriver->botData.aiDamageState;
		if (sVar7 == BOTS_DAMAGE_STATE_BLAST)
		{
			botDriver->botData.aiPhysics.rotXZ = (s16)CTR_MipsSubLo((u16)botDriver->botData.aiPhysics.rotXZ, 1);
		}
		else
		{
			if (sVar7 < BOTS_DAMAGE_STATE_SQUISH)
			{
				if (sVar7 == BOTS_DAMAGE_STATE_SPIN)
				{
					u16 squishCooldownMaybe = (u16)botDriver->botData.aiPhysics.squishCooldown;
					int alsoSquishCooldownMaybe = CTR_MipsSubLo(botDriver->botData.aiPhysics.squishCooldown, 0xc);
					botDriver->botData.aiPhysics.squishCooldown = alsoSquishCooldownMaybe;
					botDriver->botData.aiPhysics.mulDrift = (s16)CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.mulDrift, squishCooldownMaybe);
					if (alsoSquishCooldownMaybe < 0x100)
					{
						botDriver->botData.aiPhysics.squishCooldown = 0;
						botDriver->botData.aiPhysics.mulDrift = 0;
						botDriver->botData.aiPhysics.speedLinear = 0;
						botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
					}
					newKartState = 3;
					botDriver->kartState = newKartState;
				}
			}
			else
			{
				if (sVar7 == BOTS_DAMAGE_STATE_SQUISH)
				{
					u16 sVar7 = (u16)botDriver->botData.aiPhysics.squishCooldown;
					int iVar4 = CTR_MipsSubLo(botDriver->botData.aiPhysics.squishCooldown, 0xc);
					botDriver->botData.aiPhysics.squishCooldown = iVar4;
					botDriver->botData.aiPhysics.mulDrift = (s16)CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.mulDrift, sVar7);
					if (iVar4 < 0x200)
					{
						botDriver->botData.aiPhysics.squishCooldown = 0;
						botDriver->botData.aiPhysics.mulDrift = 0;
					}
					iVar4 = (s16)CTR_MipsSubLo((u16)botDriver->botData.aiPhysics.rotXZ, elapsedMilliseconds);
					botDriver->botData.aiPhysics.rotXZ = iVar4;
					if (iVar4 <= 0)
					{
						botDriver->botData.aiPhysics.speedY = 0x1400;
						botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
						botDriver->actionsFlagSet |= ACTION_JUMP_STARTED;
					}
				}
				else
				{
					if (sVar7 == BOTS_DAMAGE_STATE_MASK_GRAB)
					{
						struct Thread *plant = botDriver->plantEatingMe;

						struct Instance *plantInst = plant->inst;

						if (botDriver->botData.aiPhysics.rotXZ < 0xb40)
						{
							SVECTOR v;
							v.vx = 0xfa;
							if ((plant->object != NULL) && (((struct Plant *)plant->object)->LeftOrRight != 0))
							{
								v.vx = -0xfa;
							}
							v.vy = 0;
							v.vz = 0x2ee;

							SetRotMatrix(&plantInst->matrix);
							SetTransMatrix(&plantInst->matrix);

							VECTOR v2;
							s32 l3;

							RotTrans(&v, &v2, &l3);

							gGT->pushBuffer[botDriver->driverID].pos.x = v2.vx;
							gGT->pushBuffer[botDriver->driverID].pos.y = CTR_MipsAddLo(plantInst->matrix.t[1], 0xc0);
							gGT->pushBuffer[botDriver->driverID].pos.z = v2.vz;

							int camDriverXDelta = CTR_MipsSubLo(v2.vx, plantInst->matrix.t[0]);
							int camY = gGT->pushBuffer[botDriver->driverID].pos.y;
							int driverY = plantInst->matrix.t[1];
							int camDriverZDelta = CTR_MipsSubLo(v2.vz, plantInst->matrix.t[2]);

							int rotY = ratan2(camDriverXDelta, camDriverZDelta);
							gGT->pushBuffer[botDriver->driverID].rot.y = rotY;

							int rotX = SquareRoot0_stub(
							    CTR_MipsAddLo(CTR_MipsMulLo(camDriverXDelta, camDriverXDelta), CTR_MipsMulLo(camDriverZDelta, camDriverZDelta)));
							rotX = ratan2(CTR_MipsSubLo(camY, driverY), rotX);

							gGT->pushBuffer[botDriver->driverID].rot.x = CTR_MipsSubLo(0x800, rotX);
							gGT->pushBuffer[botDriver->driverID].rot.z = 0;
						}

						botDriver->botData.aiPhysics.speedLinear = 0;
						int iVar4 = (s16)CTR_MipsSubLo((u16)botDriver->botData.aiPhysics.rotXZ, elapsedMilliseconds);
						botDriver->botData.aiPhysics.rotXZ = iVar4;
						newKartState = 5;
						if (iVar4 < 1)
						{
							botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
							botInstance->flags &= ~HIDE_MODEL;
							botDriver->botData.ai_progress_cooldown = 1;
							BOTS_MaskGrab(botThread);
							newKartState = 5;
						}
						botDriver->kartState = newKartState;
					}
				}
			}
		}
	}

	if (botDriver->forcedJumpType != FORCED_JUMP_NONE)
	{
		int turtleJumpForce = CTR_MipsAddLo(CTR_MipsSll(botDriver->const_JumpForce, 1), botDriver->const_JumpForce); // iVar4

		if (botDriver->forcedJumpType != FORCED_JUMP_HIGH)
		{
			turtleJumpForce = CTR_MipsDiv(turtleJumpForce, 2);
		}

		if (botDriver->botData.aiPhysics.speedY < turtleJumpForce)
		{
			botDriver->botData.aiPhysics.speedY = turtleJumpForce;
		}

		botDriver->forcedJumpType = FORCED_JUMP_NONE;
	}

	if (((navFrameSpecialBits & BOTS_NAV_SPECIAL_RAMP_PHYS) != 0) &&
	    ((0x1c1f < botDriver->botData.aiPhysics.speedLinear) || (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)))
	{
		int iVar4 = (local_3c & BOTS_NAV_SPECIAL_INDEX_MASK);
		botDriver->botData.aiPhysics.speedY = sdata->NavPath_ptrHeader[botDriver->botData.botPath]->rampPhys2[iVar4];
		botDriver->botData.aiPhysics.speedLinear = sdata->NavPath_ptrHeader[botDriver->botData.botPath]->rampPhys1[iVar4];

		if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)
		{
			botDriver->botData.aiPhysics.squishCooldown = 0;
			botDriver->botData.aiPhysics.mulDrift = 0;
			botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
		}
	}

	// MUST be unsigned, same reason as the other -0x31
	u8 cmp1 = navFrameCurr->rot[3] - 0x31;
	u8 cmp2 = navFrameNext->rot[3] - 0x31;

	if ((0x9e < cmp1) && (0x9e < cmp2))
	{
		if (((botDriver->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) == 0) && ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) != 0))
		{
			// Lerp nav-frame rotation through the shortest signed 12-bit delta.
			int uVar8 = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[0], 4), CTR_MipsSll(navFrameCurr->rot[0], 4)) & 0xfff;
			if (0x7ff < uVar8)
			{
				uVar8 = CTR_MipsSubLo(uVar8, 0x1000);
			}

			botDriver->botData.aiRot.x = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[0], 4), CTR_MipsSra(CTR_MipsMulLo(uVar8, percentage), 0xc)) & 0xfff;

			uVar8 = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[2], 4), CTR_MipsSll(navFrameCurr->rot[2], 4)) & 0xfff;
			if (0x7ff < uVar8)
			{
				uVar8 = CTR_MipsSubLo(uVar8, 0x1000);
			}

			botDriver->botData.aiRot.z = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[2], 4), CTR_MipsSra(CTR_MipsMulLo(uVar8, percentage), 0xc)) & 0xfff;
		}

		int other_uVar8 = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[1], 4), CTR_MipsSll(navFrameCurr->rot[1], 4)) & 0xfff;
		if (0x7ff < other_uVar8)
		{
			other_uVar8 = CTR_MipsSubLo(other_uVar8, 0x1000);
		}

		botDriver->botData.aiRot.y = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[1], 4), CTR_MipsSra(CTR_MipsMulLo(other_uVar8, percentage), 0xc)) & 0xfff;

		if ((botDriver->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) != 0)
		{
			other_uVar8 = 0;
		}

		other_uVar8 = CTR_MipsSubLo(CTR_MipsSll(other_uVar8, 1), botDriver->turnAngleCurr) & 0xfff;

		b32 bVar1 = other_uVar8 < 0x21;
		if (0x7ff < other_uVar8)
		{
			other_uVar8 = CTR_MipsSubLo(other_uVar8, 0x1000);
			bVar1 = other_uVar8 < 0x21;
		}
		if (bVar1)
		{
			if (other_uVar8 < -0x20)
			{
				other_uVar8 = -0x20;
			}
		}
		else
		{
			other_uVar8 = 0x20;
		}
		other_uVar8 = (s16)CTR_MipsAddLo((u16)botDriver->turnAngleCurr, other_uVar8);
		botDriver->turnAngleCurr = other_uVar8;

		s16 sVar7;
		if (botDriver->botData.aiPhysics.driftTarget == 0)
		{
			other_uVar8 &= 0xfff;
			sVar7 = other_uVar8;
			botDriver->botData.aiPhysics.simpTurnState = sVar7;
			if (0x7ff < other_uVar8)
			{
				botDriver->botData.aiPhysics.simpTurnState = (s16)CTR_MipsSubLo(sVar7, 0x1000);
			}
			sVar7 = (s16)CTR_MipsSra(botDriver->botData.aiPhysics.simpTurnState, 2);
		}
		else
		{
			int uVar11 = CTR_MipsNegLo((u16)botDriver->botData.aiPhysics.mulDrift) & 0xfff;

			botDriver->botData.aiPhysics.simpTurnState = uVar11;
			if (0x7ff < uVar11)
			{
				botDriver->botData.aiPhysics.simpTurnState = (s16)CTR_MipsSubLo(uVar11, 0x1000);
			}

			sVar7 = (s16)CTR_MipsSra(botDriver->botData.aiPhysics.simpTurnState, 3);
		}
		botDriver->botData.aiPhysics.simpTurnState = sVar7;

		other_uVar8 = CTR_MipsSubLo(botDriver->botData.aiPhysics.simpTurnState, botDriver->wheelRotation) & 0xfff;
		b32 other_bVar1 = other_uVar8 < 0x21;

		if (0x7ff < other_uVar8)
		{
			other_uVar8 = CTR_MipsSubLo(other_uVar8, 0x1000);
			other_bVar1 = other_uVar8 < 0x21;
		}
		sVar7 = other_uVar8;
		if (other_bVar1)
		{
			if (other_uVar8 < -0x20)
			{
				sVar7 = -0x20;
			}
		}
		else
		{
			sVar7 = 0x20;
		}

		botDriver->wheelRotation = (s16)CTR_MipsAddLo((u16)botDriver->wheelRotation, sVar7);
		botDriver->simpTurnState = (s8)(u8)botDriver->botData.aiPhysics.simpTurnState;
	}

	botDriver->rotCurr.x = botDriver->botData.aiRot.x;
	botDriver->rotCurr.y = botDriver->botData.aiRot.y;
	botDriver->rotCurr.z = botDriver->botData.aiRot.z;

	int badnessRecieveTimer = botDriver->clockReceive; // iVar4

	if (badnessRecieveTimer == 0)
	{
		badnessRecieveTimer = botDriver->squishTimer;
		if (badnessRecieveTimer == 0)
		{
			if (botDriver->thCloud != NULL)
			{
				badnessRecieveTimer = CTR_MipsSll(gGT->timer, 2);

				goto badEffectKartWiggle;
			}
		}
	}
	else
	{
	badEffectKartWiggle:;
		int wiggleX = CTR_MipsSra(CTR_MipsMulLo(MATH_Sin(CTR_MipsMulLo(badnessRecieveTimer, 0xc)), 50), 10);
		int wiggleZ = CTR_MipsSra(CTR_MipsMulLo(MATH_Cos(CTR_MipsMulLo(badnessRecieveTimer, 0xc)), 50), 10);
		SVECTOR wiggle = {
		    .vx = (s16)wiggleX,
		    .vy = 0,
		    .vz = (s16)wiggleZ,
		    .pad = 0,
		};

		CTR_GteLoadSV0(&wiggle);
		gte_mvmva(1, 0, 0, 3, 0);

		botDriver->botData.aiPhysics.turboMeter = 0;

		botDriver->rotCurr.x = (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.x, (s16)MFC2(25));
		botDriver->rotCurr.y = (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.y, (s16)MFC2(26));
		botDriver->rotCurr.z = (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.z, (s16)MFC2(27));
	}

	if ((botDriver->botData.botFlags & (BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_FREE_PHYSICS)) == 0)
	{
		SVec3 rot;
		rot.x = (s16)CTR_MipsSll(navFrameCurr->rot[0], 4);
		rot.y = (s16)CTR_MipsSll(navFrameCurr->rot[1], 4);
		rot.z = (s16)CTR_MipsSll(navFrameCurr->rot[2], 4);
		MATRIX m;

		ConvertRotToMatrix(&m, &rot);

		botDriver->AxisAngle3_normalVec.x = m.m[0][1];
		botDriver->AxisAngle3_normalVec.y = m.m[1][1];
		botDriver->AxisAngle3_normalVec.z = m.m[2][1];

		botInstance->bitCompressed_NormalVector_AndDriverIndex = INST_CompressNormalVectorAndDriverIndex(m.m[0][1], m.m[1][1], m.m[2][1], botDriver->driverID);
	}

	ConvertRotToMatrix(&botInstance->matrix, &botDriver->rotCurr.vec);

	// c is row-major (i.e., ticking the rightmost indeces has smaller memory address delta vs ticking the leftmost indeces)
	botDriver->AxisAngle2_normalVec.x = botInstance->matrix.m[0][1];
	botDriver->AxisAngle2_normalVec.y = botInstance->matrix.m[1][1];
	botDriver->AxisAngle2_normalVec.z = botInstance->matrix.m[2][1];

	botDriver->angle = botDriver->rotCurr.y;

	botDriver->speedApprox = botDriver->botData.aiPhysics.speedLinear;

	botDriver->speed = botDriver->botData.aiPhysics.speedLinear;
	botDriver->jumpHeightPrev = botDriver->jumpHeightCurr;
	botDriver->axisRotationX = botDriver->botData.aiRot.y & 0xfff;

	int iVar4_lifetime_2 = MATH_Cos(navFrameCurr->rot[3]);

	botDriver->jumpHeightCurr = (s16)CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedY, iVar4_lifetime_2), 0xc);

	iVar4_lifetime_2 = MATH_Cos(botDriver->axisRotationX);

	botDriver->zSpeed = CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, iVar4_lifetime_2), 0xc);

	iVar4_lifetime_2 = MATH_Sin(botDriver->axisRotationX);

	int uVar11 = botDriver->rotCurr.z & 0xfff;

	botDriver->ySpeed = botDriver->botData.aiPhysics.speedY;
	botDriver->rotCurr.z = uVar11;

	botDriver->xSpeed = CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, iVar4_lifetime_2), 0xc);
	if (0x7ff < uVar11)
	{
		botDriver->rotCurr.z = (s16)CTR_MipsSubLo(uVar11, 0x1000);
	}
	botDriver->rotCurr.y =
	    (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.y, CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.mulDrift, (u16)botDriver->turnAngleCurr));

	botDriver->posCurr.x = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.x, botDriver->botData.positionBackup.x);
	botDriver->posCurr.y = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.y, botDriver->botData.positionBackup.y);
	botDriver->posCurr.z = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.z, botDriver->botData.positionBackup.z);

	botInstance->matrix.t[0] = CTR_MipsSra(botDriver->posCurr.x, 8);
	botInstance->matrix.t[1] = CTR_MipsAddLo(CTR_MipsSra(botDriver->posCurr.y, 8), botDriver->Screen_OffsetY);
	botInstance->matrix.t[2] = CTR_MipsSra(botDriver->posCurr.z, 8);

	badnessRecieveTimer = botDriver->clockReceive;

	if (badnessRecieveTimer == 0)
	{
		badnessRecieveTimer = botDriver->squishTimer;
		if (badnessRecieveTimer == 0)
		{
			if (0x100 < botDriver->speedApprox)
			{
				botDriver->hazardTimer = (s16)CTR_MipsSubLo((u16)botDriver->hazardTimer, elapsedMilliseconds);
			}

			u16 uVar11 = (u16)botDriver->hazardTimer & 0xfffe;
			botDriver->hazardTimer = (s16)uVar11;

			if ((s16)uVar11 >= 0)
			{
				botDriver->hazardTimer = -2;
			}
			goto FinishHazardTimerUpdate;
		}
	}

	if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		int iVar4 = botDriver->speedApprox;
		if (iVar4 < 0)
		{
			iVar4 = CTR_MipsNegLo(iVar4);
		}

		if (iVar4 < 0x101)
		{
			goto CheckExistingHazardTimer;
		}

		if (badnessRecieveTimer < 0)
		{
			badnessRecieveTimer = CTR_MipsNegLo(badnessRecieveTimer);
		}
	StoreNegativeHazardTimer:
		botDriver->hazardTimer = (s16)CTR_MipsNegLo(badnessRecieveTimer);
	}
	else
	{
		int iVar4 = botDriver->speedApprox;

		if (iVar4 < 0)
		{
			iVar4 = CTR_MipsNegLo(iVar4);
		}

		if (iVar4 < 0x101)
		{
		CheckExistingHazardTimer:
			badnessRecieveTimer = botDriver->hazardTimer;
			if (0 < badnessRecieveTimer)
			{
				goto StoreNegativeHazardTimer;
			}
		}
		else
		{
			if (badnessRecieveTimer < 0)
			{
				badnessRecieveTimer = CTR_MipsNegLo(badnessRecieveTimer);
			}
			botDriver->hazardTimer = (s16)badnessRecieveTimer;
		}
	}

	botDriver->hazardTimer = (s16)((u16)botDriver->hazardTimer | 1); // increment it by 1 if even.

FinishHazardTimerUpdate:

	if (((navFrameSpecialBits & BOTS_NAV_SPECIAL_LEVEL_INST_COLL) != 0) ||
	    ((botDriver->botData.botFlags & (BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_FREE_PHYSICS)) != 0))
	{
		BOTS_LevInstColl(botThread);
	}

	if ((navFrameFlags & BOTS_NAV_FLAG_SPLIT_LINE) != 0)
	{
		botInstance->flags |= SPLIT_LINE;
	}

	VehPhysForce_TranslateMatrix(botThread, botDriver);

	VehPhysForce_RotAxisAngle(&botDriver->matrixMovingDir, botDriver->AxisAngle2_normalVec.v, botDriver->angle);

	VehFrameProc_Driving(botThread, botDriver);

	if (((botDriver->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) != 0) && (botDriver->botData.aiDamageState == BOTS_DAMAGE_STATE_BLAST))
	{
		SVec3 rot;
		rot.x = (s16)CTR_MipsSll(botDriver->botData.aiPhysics.rotXZ, 8);
		rot.z = 0;
		rot.y = (s16)CTR_MipsMulLo(botDriver->botData.aiPhysics.rotXZ, 0xe0);

		ConvertRotToMatrix(&sdata->rotXZ, &rot);

		MATH_MatrixMul(&sdata->rotXYZ, &botInstance->matrix, &sdata->rotXZ);

		MATH_MatrixMul(&botInstance->matrix, &sdata->rotXYZ, &data.identity);

		botInstance->matrix.t[1] = CTR_MipsAddLo(botInstance->matrix.t[1], 0x20);
	}

	if ((botDriver->botData.botFlags & BOT_FLAG_DAMAGE_SUPPRESS_EMITTER) == 0)
	{
		VehEmitter_DriverMain(botThread, botDriver);
	}

	if (botInstance->thread->modelIndex == DYNAMIC_PLAYER)
	{
		EngineSound_Player(botDriver);
	}

	s16 camRot = CTR_MipsSubLo(botDriver->angle, botDriver->botData.ai_rotY_608) & 0xfff;

	botDriver->rotCurr.w = (s16)CTR_MipsNegLo(camRot);

	if (0x7ff < camRot)
	{
		camRot |= 0xf000;
	}

	botDriver->botData.ai_rotY_608 = (s16)CTR_MipsAddLo((u16)botDriver->botData.ai_rotY_608, CTR_MipsSra(camRot, 3));
	botDriver->botData.ai_rotY_608 &= 0xfff;

	if (botThread->modelIndex == DYNAMIC_PLAYER)
	{
		s16 probeX = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.x, botDriver->botData.aiPhysics.velocity.x), 8);
		s16 probeY = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.y, botDriver->botData.aiPhysics.velocity.y), 8);
		s16 probeZ = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.z, botDriver->botData.aiPhysics.velocity.z), 8);
		SVec3 probeTop = {
		    .x = probeX,
		    .y = (s16)CTR_MipsSubLo(probeY, 0x100),
		    .z = probeZ,
		};
		SVec3 probeBottom = {
		    .x = probeX,
		    .y = (s16)CTR_MipsAddLo(probeY, 0x40),
		    .z = probeZ,
		};

		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
		sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
		sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;

		COLL_SearchBSP_CallbackQUADBLK(&probeTop, &probeBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			botDriver->underDriver = sps->hit.ptrQuadblock;
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80016b00-0x80016ec8
u32 BOTS_ChangeState(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason)
{
	driverVictim->pendingDamageType = 0;

	if (driverVictim->kartState == KS_MASK_GRABBED)
	{
		return 0;
	}

	driverVictim->kartState = KS_NORMAL;

	switch (damageType)
	{
	case 0:
		if ((driverVictim->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) != 0)
		{
			return 0;
		}
		break;
	case 1:
	case 4:
		if ((driverVictim->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0)
		{
			driverVictim->botData.aiDamageState = BOTS_DAMAGE_STATE_SPIN;
			driverVictim->botData.aiPhysics.turboMeter = 0;

			if ((data.characterIDs[driverVictim->driverID] != NITROS_OXIDE) || ((driverVictim->actionsFlagSet & ACTION_TOUCH_GROUND) != 0))
			{
				driverVictim->reserves = 0;
				driverVictim->turbo_outsideTimer = 0;
				driverVictim->botData.aiPhysics.reserved_0x5cc = 0;

				int newSpeed;

				if (data.characterIDs[driverVictim->driverID] == NITROS_OXIDE)
				{
					newSpeed = CTR_MipsSra(driverVictim->botData.aiPhysics.speedLinear, 1);
				}
				else
				{
					newSpeed = CTR_MipsSra(driverVictim->botData.aiPhysics.speedLinear, 2);
				}

				driverVictim->botData.aiPhysics.speedLinear = newSpeed;
			}

			driverVictim->botData.aiPhysics.squishCooldown = 0x300;
			driverVictim->botData.ai_progress_cooldown = 0;

			driverVictim->botData.botFlags |= BOT_FLAG_DAMAGE_ACTIVE;
		}

		if (damageType == 4)
		{
			if (driverVictim->instSelf->thread->modelIndex == DYNAMIC_PLAYER && driverVictim->burnTimer == 0)
			{
				OtherFX_Play(0x69, 1);
			}

			driverVictim->burnTimer = 0xf00;
		}
		break;
	case 2:
		driverVictim->botData.blastBounceCount = 0;
		driverVictim->botData.aiPhysics.turboMeter = 0;
		driverVictim->botData.aiDamageState = BOTS_DAMAGE_STATE_BLAST;
		driverVictim->reserves = 0;
		driverVictim->turbo_outsideTimer = 0;
		driverVictim->botData.aiPhysics.reserved_0x5cc = 0;
		driverVictim->botData.aiPhysics.speedY = sdata->AI_VelY_WhenBlasted_0x3000;

		if ((driverVictim->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0)
		{
			driverVictim->botData.aiPhysics.speedLinear = CTR_MipsSra(driverVictim->botData.aiPhysics.speedLinear, 3);
			driverVictim->botData.positionBackup.y = CTR_MipsAddLo(driverVictim->botData.positionBackup.y, 0x4000);
		}

		driverVictim->botData.ai_progress_cooldown = 0;
		driverVictim->matrixArray = 0;
		driverVictim->botData.botFlags |= BOT_FLAG_DAMAGE_ACTIVE;

		if (driverAttacker == NULL)
		{
			return 1;
		}

		if ((driverAttacker->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(1, data.characterIDs[driverVictim->driverID], 0x10);
		}
		break;
	case 3:
		driverVictim->botData.aiPhysics.turboMeter = 0;

		if (driverVictim->instSelf->thread->modelIndex == DYNAMIC_PLAYER && driverVictim->botData.aiPhysics.squishCooldown == 0)
		{
			OtherFX_Play(0x5a, 1);
		}

		driverVictim->botData.aiDamageState = BOTS_DAMAGE_STATE_SQUISH;
		driverVictim->botData.aiPhysics.squishCooldown = 0x300;
		driverVictim->botData.aiPhysics.rotXZ = 0xf00;
		driverVictim->squishTimer = 0xf00;
		driverVictim->reserves = 0;
		driverVictim->turbo_outsideTimer = 0;
		driverVictim->botData.aiPhysics.reserved_0x5cc = 0;
		driverVictim->botData.aiPhysics.speedY = 0;
		driverVictim->botData.ai_progress_cooldown = 0;
		driverVictim->botData.aiPhysics.speedLinear = CTR_MipsSra(driverVictim->botData.aiPhysics.speedLinear, 1);
		driverVictim->botData.botFlags |= BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER;
		break;
	case 5:
		driverVictim->botData.aiPhysics.turboMeter = 0;
		driverVictim->botData.aiPhysics.speedLinear = 0;
		driverVictim->botData.aiPhysics.speedY = 0;
		driverVictim->botData.aiPhysics.reserved_0x5cc = 0;
		driverVictim->instSelf->flags |= HIDE_MODEL;
		driverVictim->botData.aiPhysics.rotXZ = 0xd20;
		driverVictim->botData.aiDamageState = BOTS_DAMAGE_STATE_MASK_GRAB;
		driverVictim->kartState = KS_MASK_GRABBED;
		driverVictim->botData.botFlags |= BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER;
		driverVictim->instSelf->thread->flags |= THREAD_FLAG_DISABLE_COLLISION;
		break;
	default:
		driverVictim->botData.ai_progress_cooldown = 0x3c;
	}

	if (driverAttacker != NULL && damageType != 0)
	{
		driverAttacker->numTimesAttacked++;
		switch (damageType)
		{
		case 1:
			driverAttacker->numTimesBombsHitSomeone++;
			break;
		case 3:
			driverAttacker->numTimesMissileHitSomeone++;
			break;
		case 4:
			driverAttacker->numTimesMovingPotionHitSomeone++;
			break;
		}
	}
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80016ec8-0x8001702c
void BOTS_CollideWithOtherAI(struct Driver *robot_1, struct Driver *robot_2)
{
	// first determine which driver bumps forward and which bumps backwards
	if (robot_1->driverRank < robot_2->driverRank)
	{
		struct Driver *temp = robot_2;
		robot_2 = robot_1;
		robot_1 = temp;
	}
	// robot_1 = iVar2
	// robot_2 = param_2

	SVec3 *navSegmentStartPos;
	SVec3 *navSegmentEndPos;
	if ((robot_1->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) == 0)
	{
		// nav path index
		s16 botPathIndex = robot_1->botData.botPath;

		// pointer to navFrame
		struct NavFrame *navFrameCurr = robot_1->botData.botNavFrame;
		struct NavFrame *navFrameNext = navFrameCurr + 1;

		// iVar4
		navSegmentStartPos = &navFrameCurr->pos;

		// if you go out of bounds
		if (sdata->NavPath_ptrHeader[botPathIndex]->last <= navFrameNext)
		{
			// loop back to first navFrame
			navFrameNext = sdata->NavPath_ptrNavFrameArray[botPathIndex];
		}
		navSegmentEndPos = &navFrameNext->pos;
	}
	else
	{
		// pointer to nav frame
		struct NavFrame *navFrameNext = robot_1->botData.botNavFrame;

		// iVar4
		navSegmentStartPos = &robot_1->botData.estimatePosition;
		navSegmentEndPos = &navFrameNext->pos;
	}

	SVec3 pos = {
	    .x = (s16)CTR_MipsSra(robot_1->posCurr.x, 8),
	    .y = (s16)CTR_MipsSra(robot_1->posCurr.y, 8),
	    .z = (s16)CTR_MipsSra(robot_1->posCurr.z, 8),
	};

	int res1 = CAM_MapRange_PosPoints(navSegmentEndPos, navSegmentStartPos, &pos);

	pos = (SVec3){
	    .x = (s16)CTR_MipsSra(robot_2->posCurr.x, 8),
	    .y = (s16)CTR_MipsSra(robot_2->posCurr.y, 8),
	    .z = (s16)CTR_MipsSra(robot_2->posCurr.z, 8),
	};

	int res2 = CAM_MapRange_PosPoints(navSegmentEndPos, navSegmentStartPos, &pos);

	// reduce speed of one AI,
	// the AI that is closer to the previous nav point,
	// who therefore is the driver in the back of the collision

	if (res1 < res2)
	{
		int speed = CTR_MipsSubLo(robot_2->botData.aiPhysics.speedLinear, BOTS_AI_COLLISION_SPEED_PENALTY);
		speed = ((speed < 0) ? 0 : speed); // clamp to 0

		robot_1->botData.aiPhysics.speedLinear = speed;
	}
	else
	{
		int speed = CTR_MipsSubLo(robot_1->botData.aiPhysics.speedLinear, BOTS_AI_COLLISION_SPEED_PENALTY);
		speed = ((speed < 0) ? 0 : speed); // clamp to 0

		robot_2->botData.aiPhysics.speedLinear = speed;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001702c-0x80017164.
void BOTS_GotoStartingLine(struct Driver *d)
{
	sdata->aiCollisionDelayFrameCount = 0;

	VehBirth_TeleportSelf(d, VEH_BIRTH_SPAWN_RACE_START, 0);

	// get position where driver should spawn
	u8 spawnPos = sdata->kartSpawnOrderArray[d->driverID];

	d->botData.aiPhysics.reserved_0x5cc = 0;
	d->botData.aiPhysics.speedY = 0;
	d->botData.aiPhysics.speedLinear = 0;
	CTR_SET_VEC3(d->botData.aiPhysics.accel.v, 0, 0, 0);
	CTR_SET_VEC3(d->botData.aiPhysics.velocity.v, 0, 0, 0);

	d->botData.positionBackup.x = d->posCurr.x;
	d->botData.positionBackup.y = d->posCurr.y;
	d->botData.positionBackup.z = d->posCurr.z;

	d->botData.navProgressRemainder = 0;

	// current navFrame pointer, first navFrame on path
	d->botData.botNavFrame = sdata->NavPath_ptrNavFrameArray[d->botData.botPath];

	BOTS_SetRotation(d, 1);

	// time until full acceleration from start
	int accelDuration = sdata->AI_AccelFrameCount;

	// get acceleration order from spawn order
	u8 accel = sdata->accelerateOrder[spawnPos];

	d->rotCurr.z = 0;
	d->rotPrev.z = 0;
	d->botData.aiRot.z = 0;
	d->rotCurr.x = 0;
	d->rotPrev.x = 0;
	d->botData.aiRot.x = 0;
	d->turnAngleCurr = 0;

	// turn on 21st flag of actions flag set, means driver is AI
	d->actionsFlagSet |= ACTION_BOT;

	// calculate Y rotation
	s16 rotY = (s16)CTR_MipsSll((u8)d->botData.estimateRotNav[1], 4);

	// every possible Y rotation
	d->botData.ai_rotY_608 = rotY;
	d->angle = rotY;
	d->rotCurr.y = rotY;
	d->rotPrev.y = rotY;
	d->botData.aiRot.y = rotY;

	// acceleration from start-line to full speed
	d->botData.botAccel = CTR_MipsMulLo(accelDuration, accel);

	// cooldown before next weapon
	int rng = RngDeadCoed(&sdata->const_0x30215400);
	d->botData.weaponCooldown = (s16)CTR_MipsAddLo(CTR_MipsSra(rng, 8) & 0xff, 300);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80017164-0x80017318.
struct Driver *BOTS_Driver_Init(int driverID)
{
	s8 initialNavPathIndex = sdata->driver_pathIndexIDs[driverID];
	s8 navPathIndex = initialNavPathIndex;
	while (1)
	{
		s16 navPathPointsCount = sdata->NavPath_ptrHeader[navPathIndex]->numPoints;
		if (1 < navPathPointsCount)
		{
			break; // success
		}

		navPathIndex--;

		// If subtracted below zero,
		// go back to highest index (2)
		if (navPathIndex < 0)
		{
			navPathIndex = 2;
		}

		// If all 3 are checked, quit
		if (navPathIndex == initialNavPathIndex)
		{
			return NULL;
		}
	}

	// path data found
	struct Thread *t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(DRIVER_NTSC_RETAIL_SIZE, NONE, LARGE, ROBOT),

	    BOTS_ThTick_Drive, // behavior
	    0,                 //"robotcar",	// debug name
	    0                  // thread relative
	);

	struct Driver *d = t->object;
	memset(d, 0x0, DRIVER_NTSC_RETAIL_SIZE);
	VehBirth_NonGhost(t, driverID);
	sdata->gGT->drivers[driverID] = d;
	t->modelIndex = DYNAMIC_ROBOT_CAR;

	d->botData.botPath = navPathIndex;
	d->botData.botNavFrame = sdata->NavPath_ptrNavFrameArray[navPathIndex];
	d->actionsFlagSet |= ACTION_BOT;
	LIST_AddFront(&sdata->navBotList[navPathIndex], (struct Item *)(&d->botData));

	sdata->gGT->numBotsNextGame++;
	BOTS_GotoStartingLine(d);
	return d;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80017318-0x800175cc.
void BOTS_Driver_Convert(struct Driver *d)
{
	// if already AI, quit
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	UI_RaceEnd_GetDriverClock(d);

	s8 initialNavPathIndex = sdata->driver_pathIndexIDs[d->driverID];
	s8 navPathIndex = initialNavPathIndex;
	while (1)
	{
		s16 navPathPointsCount = sdata->NavPath_ptrHeader[navPathIndex]->numPoints;
		if (1 < navPathPointsCount)
		{
			break; // success
		}

		navPathIndex--;

		// If subtracted below zero,
		// go back to highest index (2)
		if (navPathIndex < 0)
		{
			navPathIndex = 2;
		}

		// If all 3 are checked, quit
		if (navPathIndex == initialNavPathIndex)
		{
			return;
		}
	}

	memset(&d->botData, 0, sizeof(struct BotData));

	d->botData.aiPhysics.speedY = d->ySpeed;

	s32 speedApprox = d->speedApprox;

	d->botData.botPath = navPathIndex;

	speedApprox = ((speedApprox < 0) ? CTR_MipsNegLo(speedApprox) : speedApprox);

	d->botData.aiPhysics.speedLinear = speedApprox;

	struct NavFrame *firstNavFrame = sdata->NavPath_ptrNavFrameArray[navPathIndex];

	d->botData.navProgressRemainder = 0;
	d->turnAngleCurr = 0;
	d->multDrift = 0;
	d->ampTurnState = 0;
	d->wallRubTimer = 0;

	d->botData.botNavFrame = firstNavFrame;

	d->instSelf->thread->funcThTick = BOTS_ThTick_Drive;

	if ((sdata->gGT->gameMode1 & BATTLE_MODE) != 0)
	{ // you are in battle mode
		struct NavFrame *nf = NAVHEADER_GETFRAME(sdata->NavPath_ptrHeader[navPathIndex]);
		d->posCurr.x = CTR_MipsSll(nf->pos.x, 8);
		d->posCurr.y = CTR_MipsSll(nf->pos.y, 8);
		d->posCurr.z = CTR_MipsSll(nf->pos.z, 8);
	}

	LIST_AddFront(&sdata->navBotList[navPathIndex], (struct Item *)&d->botData);

	BOTS_SetRotation(d, 0);

	GAMEPAD_JogCon2(d, 0, 0);

	u32 oldActionFlagsSet = d->actionsFlagSet;

	d->actionsFlagSet = (oldActionFlagsSet & ~(ACTION_JUMP_BUTTON_HELD | ACTION_ACCEL_PREVENTION)) | ACTION_BOT;

	if ((oldActionFlagsSet & ACTION_RACE_FINISHED) != 0)
	{
		CAM_EndOfRace(&sdata->gGT->cameraDC[d->driverID], d);
	}

	int damageType;
	switch (d->kartState)
	{
	default:
		return;
	case KS_SPINNING:
		damageType = 1;
		break;
	case KS_BLASTED:
		damageType = 2;
		break;
	}

	BOTS_ChangeState(d, damageType, NULL, 0);
}
