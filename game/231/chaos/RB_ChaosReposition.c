#include <common.h>
#include <game/Vehicle/VehPhysForce.h>
#include "RB_ChaosEffects.h"
#include "RB_ChaosReposition.h"

enum
{
	CHAOS_TELEPORT_LAP_DIVISOR = 10,
	CHAOS_DEFECTIVE_TELEPORT_RADIUS_DIVISOR = 5,
	CHAOS_DEFECTIVE_TELEPORT_SAMPLE_ATTEMPTS = 64,
	CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT = 0x100,
	CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN = 50,
	CHAOS_TELEPORT_PROBE_HALF_HEIGHT = 0x100,
	CHAOS_PATH_INVALID_NODE = 0xff,
	CHAOS_TRACK_DISTANCE_SCALE = 8,
};

struct ChaosTrackDestination
{
	Vec3 pos;
	int yaw;
	int checkpoint;
	int distanceToFinish;
};

static void Chaos_ReleaseRepositionHold(struct Driver *driver)
{
	struct Thread *thread = driver->instSelf->thread;
	const bool resumeDriving = Chaos_IsBeingRescued(driver) || driver->kartState == KS_ENGINE_REVVING ||
	    driver->kartState == KS_WARP_PAD;
	struct MaskHeadWeapon *mask = NULL;
	if (thread->funcThTick == BOTS_ThTick_RevEngine)
	{
		mask = driver->botData.maskObj;
	}
	else if (driver->kartState == KS_MASK_GRABBED)
	{
		mask = driver->KartStates.MaskGrab.maskObj;
	}
	else if (driver->kartState == KS_ENGINE_REVVING && driver->KartStates.RevEngine.boolMaskGrab)
	{
		mask = driver->KartStates.RevEngine.maskObj;
	}
	if (mask != NULL)
	{
		mask->scale = MASK_HEAD_SCALE_NORMAL;
		mask->rot.z &= ~MASK_HEAD_ROT_WORLD_SPACE;
		if (!driver->boolHadMaskBeforeOOB)
		{
			mask->duration = 0;
		}
	}
	struct ChaosRestart *restart = &sdata->chaosRuntime.restart[driver->driverID];
	Chaos_EndRestartHold(restart, driver);
	*restart = (struct ChaosRestart){0};
	if (resumeDriving)
	{
		thread->funcThTick = NULL;
		thread->flags &= ~THREAD_FLAG_DISABLE_COLLISION;
		driver->instSelf->flags &= ~HIDE_MODEL;
		driver->botData.maskObj = NULL;
		VehPhysProc_Driving_Init(thread, driver);
		driver->NoInputTimer = 0;
		driver->matrixArray = BAKED_GTE_MATRIX_NONE;
		driver->matrixIndex = 0;
	}
}

static void Chaos_MoveDriver(struct Driver *driver, const Vec3 *position, int yaw)
{
	Chaos_ReleaseRepositionHold(driver);
	driver->posCurr = *position;
	driver->posPrev = driver->posCurr;
	driver->underDriver = NULL;
	driver->currBlockTouching = NULL;
	driver->distanceDrivenBackwards = 0;
	driver->collisionFlags = 0;
	driver->stepFlagSet = 0;
	driver->actionsFlagSet &= ~(ACTION_DRIVING_WRONG_WAY | ACTION_CHECKPOINT_BRANCH_PENDING | ACTION_WARP);
	driver->angle = yaw;
	driver->rotCurr = (SVec3Slot){0, yaw, 0, 0};
	driver->rotPrev = driver->rotCurr;
	Chaos_ResetSteeringPose(driver);
	driver->axisRotationX = yaw;
	VehPhysForce_RotAxisAngle(&driver->matrixMovingDir, CTR_VECTOR_DATA(&driver->AxisAngle2_normalVec), yaw);
	driver->matrixFacingDir = driver->matrixMovingDir;
	VehPhysForce_ConvertSpeedToVec(driver, &driver->velocity);
	driver->xSpeed = driver->velocity.x;
	driver->ySpeed = driver->velocity.y;
	driver->zSpeed = driver->velocity.z;
	driver->accel = (SVec3){0};
	driver->instSelf->matrix.t[0] = driver->posCurr.x >> FRACTIONAL_BITS_8;
	driver->instSelf->matrix.t[1] = driver->posCurr.y >> FRACTIONAL_BITS_8;
	driver->instSelf->matrix.t[2] = driver->posCurr.z >> FRACTIONAL_BITS_8;
	Chaos_FollowBody(driver->driverID, driver);
	sdata->chaosRuntime.touchingTurboPad[driver->driverID] = false;
	OtherFX_Play(SOUND_INVISIBILITY, 1);
}

static bool Chaos_RepositionDriver(struct Driver *driver, const struct ChaosTrackDestination *destination)
{
	struct GameTracker *tracker = GAME_TRACKER;
	if (tracker->level1->ptr_mesh_info == NULL)
	{
		return false;
	}
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	const SVec3 top = {destination->pos.x >> FRACTIONAL_BITS_8,
	    (destination->pos.y >> FRACTIONAL_BITS_8) - CHAOS_TELEPORT_PROBE_HALF_HEIGHT,
	    destination->pos.z >> FRACTIONAL_BITS_8};
	const SVec3 bottom = {top.x, top.y + 2 * CHAOS_TELEPORT_PROBE_HALF_HEIGHT, top.z};
	sps->ptr_mesh_info = tracker->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
	sps->Union.QuadBlockColl.quadFlagsIgnored = QUADBLOCK_FLAG_KILL_PLANE;
	sps->Union.QuadBlockColl.searchFlags = tracker->numPlyrCurrGame < 3 ? COLL_SEARCH_HIGH_LOD : 0;
	COLL_SearchBSP_CallbackQUADBLK(&top, &bottom, sps, 0);
	if (!sps->boolDidTouchQuadblock)
	{
		return false;
	}
	struct QuadBlock *ground = sps->hit.ptrQuadblock;
	const SVec3 normal = sps->hit.plane.normal;
	Vec3 position = destination->pos;
	position.y = sps->Union.QuadBlockColl.hitPos.y * FP8(1);
	driver->AxisAngle1_normalVec = normal;
	driver->AxisAngle2_normalVec = normal;
	driver->AxisAngle3_normalVec = normal;
	driver->normalVecUP = normal;
	Chaos_MoveDriver(driver, &position, destination->yaw);
	driver->underDriver = ground;
	driver->lastValid = ground;
	driver->currBlockTouching = ground;
	driver->quadBlockHeight = position.y;
	driver->currentTerrain = ground->terrain_type;
	driver->terrainMeta1 = VehAfterColl_GetTerrain(driver->currentTerrain);
	driver->terrainMeta2 = driver->terrainMeta1;
	driver->checkpoint.currentIndex = ground->checkpointIndex != CHAOS_PATH_INVALID_NODE ?
	    ground->checkpointIndex : destination->checkpoint;
	driver->distanceToFinish_curr = destination->distanceToFinish;
	driver->distanceToFinish_checkpoint = destination->distanceToFinish;
	return true;
}

static bool Chaos_IsTeleportNearGeometry(const struct mesh_info *mesh, const Vec3 *position)
{
	const int x = position->x >> FRACTIONAL_BITS_8;
	const int y = position->y >> FRACTIONAL_BITS_8;
	const int z = position->z >> FRACTIONAL_BITS_8;
	const struct BoundingBox *bounds = &mesh->bspRoot->box;
	if (x < bounds->min.x || x > bounds->max.x || z < bounds->min.z || z > bounds->max.z)
	{
		return false;
	}
	for (int i = 0; i < mesh->numBspNodes; i++)
	{
		const struct BSP *node = &mesh->bspRoot[i];
		if ((node->flag & BSP_NODE_FLAG_LEAF) == 0 || node->data.leaf.numQuads == 0)
		{
			continue;
		}
		bounds = &node->box;
		if (x >= bounds->min.x - CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN &&
		    x <= bounds->max.x + CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN &&
		    z >= bounds->min.z - CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN &&
		    z <= bounds->max.z + CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN &&
		    y >= bounds->min.y - CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT &&
		    y <= bounds->max.y + CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT)
		{
			return true;
		}
	}
	return false;
}

void Chaos_DefectiveTeleportDriver(struct Driver *driver)
{
	const struct Level *level = GAME_TRACKER->level1;
	if (level == NULL || level->ptr_restart_points == NULL || level->cnt_restart_points == 0 ||
	    level->ptr_mesh_info == NULL || level->ptr_mesh_info->bspRoot == NULL)
	{
		return;
	}
	const int radius = level->ptr_restart_points[0].distToFinish * CHAOS_TRACK_DISTANCE_SCALE /
	    CHAOS_DEFECTIVE_TELEPORT_RADIUS_DIVISOR;
	if (radius == 0)
	{
		return;
	}
	const int yaw = driver->angle;
	const int sine = MATH_Sin(yaw);
	const int cosine = MATH_Sin(yaw + ANG_HALF_PI);
	const int heightLimit = radius < CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT ? radius : CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT;
	const s64 worldRadius = radius * FP8(1);
	const s64 worldRadiusSquared = worldRadius * worldRadius;
	Vec3 position = driver->posCurr;
	for (int attempt = 0; attempt < CHAOS_DEFECTIVE_TELEPORT_SAMPLE_ATTEMPTS; attempt++)
	{
		// Keep the rearward range, but flatten the sample vertically near level geometry.
		const int back = (u64)ChaosEffect_NextRandom(&sdata->chaosRng) * (radius + 1) / (CHAOS_RANDOM_SAMPLE_MAX + 1);
		const int side = (int)((u64)ChaosEffect_NextRandom(&sdata->chaosRng) * (2 * radius + 1) / (CHAOS_RANDOM_SAMPLE_MAX + 1)) - radius;
		const int height = (int)((u64)ChaosEffect_NextRandom(&sdata->chaosRng) * (2 * heightLimit + 1) / (CHAOS_RANDOM_SAMPLE_MAX + 1)) - heightLimit;
		// A vertical-only fallback remains possible at the world-coordinate edges.
		position.y = driver->posCurr.y + height * FP8(1);
		const Vec3 candidate = {
			driver->posCurr.x + (s64)(side * cosine - back * sine) * FP8(1) / FP_ONE,
			driver->posCurr.y + height * FP8(1),
			driver->posCurr.z + (s64)(-side * sine - back * cosine) * FP8(1) / FP_ONE,
		};
		const s64 x = candidate.x - driver->posCurr.x;
		const s64 y = candidate.y - driver->posCurr.y;
		const s64 z = candidate.z - driver->posCurr.z;
		if (x * x + y * y + z * z > worldRadiusSquared)
		{
			continue;
		}
		// Collision and camera coordinates still use signed 16-bit world units.
		if (candidate.y >= INT16_MIN * FP8(1) && candidate.y <= INT16_MAX * FP8(1) &&
		    Chaos_IsTeleportNearGeometry(level->ptr_mesh_info, &candidate))
		{
			position = candidate;
			break;
		}
	}
	position.y = position.y < INT16_MIN * FP8(1) ? INT16_MIN * FP8(1) :
	             position.y > INT16_MAX * FP8(1) ? INT16_MAX * FP8(1) : position.y;
	if (!Chaos_IsTeleportNearGeometry(level->ptr_mesh_info, &position))
	{
		return;
	}
	Chaos_MoveDriver(driver, &position, yaw);
	driver->actionsFlagSet &= ~ACTION_TOUCH_GROUND;
	driver->actionsFlagSet |= ACTION_AIRBORNE;
	driver->currentTerrain = TERRAIN_NONE;
	driver->terrainMeta1 = VehAfterColl_GetTerrain(TERRAIN_NONE);
	driver->terrainMeta2 = driver->terrainMeta1;
}

static int Chaos_GetTrackLength(const struct Level *level)
{
	if (level == NULL || level->ptr_restart_points == NULL ||
	    level->cnt_restart_points < 2 || level->cnt_restart_points > CHAOS_PATH_INVALID_NODE)
	{
		return 0;
	}
	const int length = level->ptr_restart_points[0].distToFinish * CHAOS_TRACK_DISTANCE_SCALE;
	return length > 0 ? length : 0;
}

static bool Chaos_FindTrackDestination(struct Driver *driver, int target, struct ChaosTrackDestination *destination)
{
	const struct Level *level = GAME_TRACKER->level1;
	const int length = Chaos_GetTrackLength(level);
	if (length == 0 || driver->checkpoint.currentIndex >= level->cnt_restart_points)
	{
		return false;
	}
	struct CheckpointNode *node = &level->ptr_restart_points[driver->checkpoint.currentIndex];
	for (int visited = 0; visited < level->cnt_restart_points; visited++)
	{
		struct CheckpointNode *next = RB_Warpball_NewPathNode(node,
		    driver->checkpoint.branchChoiceIndex < level->cnt_restart_points ? driver : NULL);
		const int span = ((node->distToFinish - next->distToFinish) * CHAOS_TRACK_DISTANCE_SCALE + length) % length;
		const int along = (node->distToFinish * CHAOS_TRACK_DISTANCE_SCALE - target + length) % length;
		if (span > 0 && along < span)
		{
			*destination = (struct ChaosTrackDestination){
				.pos = {
					node->pos.x * FP8(1) + (s64)(next->pos.x - node->pos.x) * FP8(1) * along / span,
					node->pos.y * FP8(1) + (s64)(next->pos.y - node->pos.y) * FP8(1) * along / span,
					node->pos.z * FP8(1) + (s64)(next->pos.z - node->pos.z) * FP8(1) * along / span,
				},
				.yaw = ratan2(next->pos.x - node->pos.x, next->pos.z - node->pos.z),
				.checkpoint = (int)(node - level->ptr_restart_points),
				.distanceToFinish = target,
			};
			return true;
		}
		node = next;
	}
	return false;
}

void Chaos_TeleportDriver(struct Driver *driver)
{
	const int length = Chaos_GetTrackLength(GAME_TRACKER->level1);
	if (length < CHAOS_TELEPORT_LAP_DIVISOR)
	{
		return;
	}
	const int unwrapped = driver->distanceToFinish_curr % length - length / CHAOS_TELEPORT_LAP_DIVISOR;
	struct ChaosTrackDestination destination;
	if (Chaos_FindTrackDestination(driver, (unwrapped + length) % length, &destination) &&
	    Chaos_RepositionDriver(driver, &destination) && unwrapped < 0 &&
	    sdata->chaosRuntime.teleportForwardCrossings[driver->driverID] < UINT8_MAX)
	{
		sdata->chaosRuntime.teleportForwardCrossings[driver->driverID]++;
	}
}

bool Chaos_ConsumeTeleportLapCrossing(const struct Driver *driver)
{
	u8 *crossings = &sdata->chaosRuntime.teleportForwardCrossings[driver->driverID];
	if (*crossings == 0)
	{
		return false;
	}
	(*crossings)--;
	return true;
}

enum
{
	CHAOS_TIME_SKIP_MS = 10000,
	// Bot navigation integrates speed * milliseconds / 32 in 24.8 positions.
	CHAOS_BOT_DISTANCE_TIME_SCALE = 32 * FP8(1),
};

static void Chaos_MoveSkippedBot(struct Driver *driver, const struct ChaosTrackDestination *destination, struct NavFrame *next)
{
	driver->posCurr = destination->pos;
	driver->posPrev = destination->pos;
	driver->botData.positionBackup = destination->pos;
	driver->botData.botNavFrame = next;
	BOTS_SetRotation(driver, 0);
	// Retail projects from the segment after the surface checkpoint.
	const int checkpoint = GAME_TRACKER->level1->ptr_restart_points[destination->checkpoint].nextIndex_backward;
	driver->checkpoint.currentIndex = checkpoint;
	driver->botData.ai_quadblock_checkpointIndex = checkpoint;
	driver->distanceToFinish_curr = destination->distanceToFinish;
	driver->distanceToFinish_checkpoint = destination->distanceToFinish;
	driver->distanceDrivenBackwards = 0;
	driver->underDriver = NULL;
	driver->currBlockTouching = NULL;
	for (int axis = 0; axis < 3; axis++)
	{
		driver->instSelf->matrix.t[axis] = CTR_VECTOR_DATA(&driver->posCurr)[axis] >> FRACTIONAL_BITS_8;
	}
}

struct ChaosBotSkip
{
	struct Driver *driver;
	struct ChaosTrackDestination destination;
	struct NavFrame *next;
	int speed;
	s64 nextCrossingDistance;
};

static void Chaos_PrepareBotSkip(struct ChaosBotSkip *skip, struct Driver *driver, int length)
{
	if (driver == NULL || (driver->actionsFlagSet & (ACTION_BOT | ACTION_RACE_FINISHED)) != ACTION_BOT)
	{
		return;
	}
	const int speed = Chaos_SpeedForDriver(driver, driver->botData.aiPhysics.speedLinear);
	if (speed <= 0)
	{
		return;
	}
	const int before = driver->distanceToFinish_curr % length;
	const s64 distance = (s64)speed * CHAOS_TIME_SKIP_MS / CHAOS_BOT_DISTANCE_TIME_SCALE;
	const int target = (before - distance % length + length) % length;
	if (!Chaos_FindTrackDestination(driver, target, &skip->destination))
	{
		return;
	}
	skip->next = BOTS_FindClosestNavFrame(driver->botData.botPath, &skip->destination.pos);
	if (skip->next != NULL)
	{
		skip->driver = driver;
		skip->speed = speed;
		skip->nextCrossingDistance = before;
	}
}

void Chaos_SkipBotTime(void)
{
	struct GameTracker *tracker = GAME_TRACKER;
	const int startTime = tracker->elapsedEventTime;
	const int length = Chaos_GetTrackLength(tracker->level1);
	if (length == 0)
	{
		return;
	}
	struct ChaosBotSkip skips[LOAD_CHARACTER_ID_COUNT] = {0};
	for (int slot = tracker->numPlyrCurrGame; slot < (int)len(tracker->drivers); slot++)
	{
		Chaos_PrepareBotSkip(&skips[slot], tracker->drivers[slot], length);
	}
	// Dispatch crossings chronologically so finish order and clocks use skip time.
	for (;;)
	{
		int first = -1;
		s64 firstMS = CHAOS_TIME_SKIP_MS + 1;
		for (int slot = 0; slot < (int)len(skips); slot++)
		{
			if (skips[slot].driver == NULL || (skips[slot].driver->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
			{
				continue;
			}
			const s64 crossingMS = (skips[slot].nextCrossingDistance * CHAOS_BOT_DISTANCE_TIME_SCALE +
			    skips[slot].speed - 1) / skips[slot].speed;
			if (crossingMS < firstMS)
			{
				first = slot;
				firstMS = crossingMS;
			}
		}
		if (first < 0)
		{
			break;
		}
		tracker->elapsedEventTime = startTime + firstMS;
		skips[first].driver->timeElapsedInRace = tracker->elapsedEventTime;
		PlayLevel_CrossFinishLine(skips[first].driver, false, false);
		skips[first].nextCrossingDistance += length;
	}
	for (int slot = 0; slot < (int)len(skips); slot++)
	{
		if (skips[slot].driver != NULL)
		{
			Chaos_MoveSkippedBot(skips[slot].driver, &skips[slot].destination, skips[slot].next);
		}
	}
	tracker->elapsedEventTime = startTime + CHAOS_TIME_SKIP_MS;
}

