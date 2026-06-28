#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005ca24-0x8005cd1c
void VehLap_UpdateProgress(struct Driver *driver)
{
	struct GameTracker *gGT = sdata->gGT;
	s16 checkpointIndex = -1;

	if (driver == NULL)
	{
		return;
	}

	if ((driver->actionsFlagSet & ACTION_BOT) == 0)
	{
		struct QuadBlock *quad = driver->lastValid;

		if ((quad != NULL) && (quad->checkpointIndex != 0xff))
		{
			checkpointIndex = quad->checkpointIndex;
		}
	}
	else
	{
		checkpointIndex = driver->botData.ai_quadblock_checkpointIndex;
	}

	struct Level *level = gGT->level1;
	if (((u32)(level->cnt_restart_points - 1) >= 0xff) || (checkpointIndex < 0))
	{
		return;
	}

	struct CheckpointNode *nodes = level->ptr_restart_points;
	struct CheckpointNode *checkpointNode = &nodes[checkpointIndex];
	struct CheckpointNode *progressNode = &nodes[checkpointNode->nextIndex_forward];
	struct CheckpointNode *nextNode = &nodes[progressNode->nextIndex_forward];

	SVec4 nodeDelta;
	nodeDelta.x = (s16)CTR_MipsSubLo((u16)progressNode->pos.x, (u16)nextNode->pos.x);
	nodeDelta.y = (s16)CTR_MipsSubLo((u16)progressNode->pos.y, (u16)nextNode->pos.y);
	nodeDelta.z = (s16)CTR_MipsSubLo((u16)progressNode->pos.z, (u16)nextNode->pos.z);
	nodeDelta.w = 0;

	MATH_VectorNormalize((SVec3 *)&nodeDelta);

	s16 deltaX = (s16)CTR_MipsSubLo((u16)CTR_MipsSra(driver->posCurr.x, 8), (u16)progressNode->pos.x);
	s16 deltaY = (s16)CTR_MipsSubLo((u16)CTR_MipsSra(driver->posCurr.y, 8), (u16)progressNode->pos.y);
	s16 deltaZ = (s16)CTR_MipsSubLo((u16)CTR_MipsSra(driver->posCurr.z, 8), (u16)progressNode->pos.z);

	CTC2(CTR_PackS16Pair(deltaX, deltaY), 0);
	CTC2(CTR_PackS16Pair(deltaZ, CTR_MipsSra(driver->matrixMovingDir.m[0][2], 5)), 1);
	CTC2(CTR_PackS16Pair(CTR_MipsSra(driver->matrixMovingDir.m[1][2], 5), CTR_MipsSra(driver->matrixMovingDir.m[2][2], 5)), 2);

	CTR_GteLoadSVec4V0(&nodeDelta);
	gte_mvmva(0, 0, 0, 3, 0);

	s32 projection = MFC2_S(25);
	s32 wrongWayTest = MFC2_S(26);
	s32 progress = ((u32)progressNode->distToFinish << 3) + (projection >> 0xc);
	s32 trackLength = (s32)nodes[0].distToFinish << 3;

	driver->distanceToFinish_curr = progress;
	// NOTE(aalhendi): Retail uses signed div/mfhi for this remainder.
	driver->distanceToFinish_curr = progress % trackLength;

	if (wrongWayTest < 0x5a801)
	{
		driver->actionsFlagSet &= ~ACTION_DRIVING_WRONG_WAY;
	}
	else
	{
		driver->actionsFlagSet |= ACTION_DRIVING_WRONG_WAY;
	}

	if (((driver->actionsFlagSet & ACTION_CHECKPOINT_BRANCH_PENDING) != 0) && (driver->checkpoint.currentIndex != (u8)checkpointIndex))
	{
		driver->checkpoint.branchChoiceIndex = checkpointIndex;
		driver->actionsFlagSet &= ~ACTION_CHECKPOINT_BRANCH_PENDING;
	}

	if (checkpointNode->nextIndex_left != 0xff)
	{
		driver->actionsFlagSet |= ACTION_CHECKPOINT_BRANCH_PENDING;
	}

	driver->checkpoint.currentIndex = checkpointIndex;
}
