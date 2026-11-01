#include "VehCommon.h"

enum
{
	VEH_LAP_INVALID_CHECKPOINT = 0xff,
	VEH_LAP_WORLD_POS_SHIFT = 8,
	VEH_LAP_MOVING_DIR_SHIFT = 5,
	VEH_LAP_TRACK_DISTANCE_SCALE_SHIFT = 3,
	VEH_LAP_PROJECTED_DISTANCE_SHIFT = 0xc,
	VEH_LAP_WRONG_WAY_DOT_LIMIT = 0x5a801,
};


void VehLap_UpdateProgress(struct Driver *driver)
{
	s16 checkpointIndex;
	register struct GameTracker *gameTracker CTR_PSX_REGISTER("$4");
	struct Level *level;
	struct CheckpointNode *nodes;
	struct CheckpointNode *checkpointNode;
	struct CheckpointNode *progressNode;
	struct CheckpointNode *nextNode;
	SVec3 delta;
	SVec3 position;
	SVec3Slot nodeDelta;
	SVec3 *nodeDeltaPtr;
	s32 projection;
	s32 wrongWayTest;
	s32 progress;
	s32 trackLength;

	if (driver == NULL)
	{
		return;
	}

	checkpointIndex = -1;

	if ((driver->actionsFlagSet & ACTION_BOT) != 0)
	{
		checkpointIndex = driver->botData.ai_quadblock_checkpointIndex;
	}
	else
	{
		struct QuadBlock *quad = driver->lastValid;

		if ((quad != NULL) && (quad->checkpointIndex != VEH_LAP_INVALID_CHECKPOINT))
		{
			checkpointIndex = quad->checkpointIndex;
		}
	}
	level = GAME_TRACKER->level1;
	if (((u32)(level->cnt_restart_points - 1) >= VEH_LAP_INVALID_CHECKPOINT) || (checkpointIndex < 0))
	{
		return;
	}

	nodes = level->ptr_restart_points;
	checkpointNode = &nodes[checkpointIndex];
	progressNode = &nodes[checkpointNode->nextIndex_forward];
	nextNode = &nodes[progressNode->nextIndex_forward];

	// NOTE(aalhendi): Retail snapshots the world position before normalizing
	// the checkpoint direction, then forms the relative vector afterward.
	position.x = (s16)CTR_MipsSra(driver->posCurr.x, VEH_LAP_WORLD_POS_SHIFT);
	position.y = (s16)CTR_MipsSra(driver->posCurr.y, VEH_LAP_WORLD_POS_SHIFT);
	position.z = (s16)CTR_MipsSra(driver->posCurr.z, VEH_LAP_WORLD_POS_SHIFT);

	nodeDelta.x = (s16)CTR_MipsSubLo((u16)progressNode->pos.x, (u16)nextNode->pos.x);
	nodeDelta.y = (s16)CTR_MipsSubLo((u16)progressNode->pos.y, (u16)nextNode->pos.y);
	nodeDelta.z = (s16)CTR_MipsSubLo((u16)progressNode->pos.z, (u16)nextNode->pos.z);
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): The GTE ignores the upper half of VZ0. Retail leaves it
	// untouched, while native initializes it before the host shim reads it.
	nodeDelta.w = 0;
#endif

	nodeDeltaPtr = SVec3Slot_AsVec3(&nodeDelta);
	MATH_VectorNormalize(nodeDeltaPtr);

	delta.x = (s16)CTR_MipsSubLo((u16)position.x, (u16)progressNode->pos.x);
	delta.y = (s16)CTR_MipsSubLo((u16)position.y, (u16)progressNode->pos.y);
	delta.z = (s16)CTR_MipsSubLo((u16)position.z, (u16)progressNode->pos.z);

	CTC2(CTR_ReadU32AlignedLE(&delta), 0);
	CTC2(CTR_PackS16Pair(delta.z, CTR_MipsSra(driver->matrixMovingDir.m[0][2], VEH_LAP_MOVING_DIR_SHIFT)), 1);
	{
		s16 movingYRaw;
		s16 movingZRaw;
		u32 movingY;
		u32 movingZ;

		// NOTE(aalhendi): Snapshot both rows before shifting them so GCC 2.8
		// retains retail's load-delay schedule for the packed GTE word.
		movingYRaw = driver->matrixMovingDir.m[1][2];
		movingZRaw = driver->matrixMovingDir.m[2][2];
		movingY = (u16)CTR_MipsSra(movingYRaw, VEH_LAP_MOVING_DIR_SHIFT);
		CTR_PSX_KEEP_VALUE(movingY);
		movingZ = (u16)CTR_MipsSra(movingZRaw, VEH_LAP_MOVING_DIR_SHIFT);
		CTC2(movingY | (movingZ << 16), 2);
	}

#if defined(CTR_NATIVE)
	CTR_GteLoadSVec3SlotV0(&nodeDelta);
#else
	// NOTE(aalhendi): PsyQ emits direct coprocessor loads here; native routes
	// the same two words through its GTE shim above.
	__asm__ volatile("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)" : : "r"(nodeDeltaPtr), "m"(nodeDelta));
#endif
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_mvmva(0, 0, 0, 3, 0);

	projection = MFC2_S(25);
	wrongWayTest = MFC2_S(26);
	progress = ((u32)progressNode->distToFinish << VEH_LAP_TRACK_DISTANCE_SCALE_SHIFT) + (projection >> VEH_LAP_PROJECTED_DISTANCE_SHIFT);
	gameTracker = GAME_TRACKER;
	CTR_PSX_KEEP_VALUE(gameTracker);

	driver->distanceToFinish_curr = progress;
	// NOTE(aalhendi): Retail publishes the projected distance before wrapping
	// it by the track length, so preserve both writes.
	CTR_PSX_OBSERVE_MEMORY(driver->distanceToFinish_curr);
	trackLength = (s32)gameTracker->level1->ptr_restart_points[0].distToFinish << VEH_LAP_TRACK_DISTANCE_SCALE_SHIFT;
	// NOTE(aalhendi): Retail uses signed div/mfhi for this remainder.
	driver->distanceToFinish_curr = progress % trackLength;

	if (wrongWayTest >= VEH_LAP_WRONG_WAY_DOT_LIMIT)
	{
		driver->actionsFlagSet |= ACTION_DRIVING_WRONG_WAY;
	}
	else
	{
		driver->actionsFlagSet &= ~ACTION_DRIVING_WRONG_WAY;
	}

	if (((driver->actionsFlagSet & ACTION_CHECKPOINT_BRANCH_PENDING) != 0) && (driver->checkpoint.currentIndex != checkpointIndex))
	{
		driver->checkpoint.branchChoiceIndex = checkpointIndex;
		driver->actionsFlagSet &= ~ACTION_CHECKPOINT_BRANCH_PENDING;
	}

	if (checkpointNode->nextIndex_left != VEH_LAP_INVALID_CHECKPOINT)
	{
		driver->actionsFlagSet |= ACTION_CHECKPOINT_BRANCH_PENDING;
	}

	driver->checkpoint.currentIndex = checkpointIndex;
}
