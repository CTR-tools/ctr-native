#include <common.h>

// NOTE(aalhendi): Read the primary index before the table pointer and select
// its node before the optional branch. The loop index then becomes its length.
#define CAM_TRACK_SELECT(destination, from, mainIndex, altIndex)                               \
	do                                                                                         \
	{                                                                                          \
		s32 nodeIndex = (mainIndex);                                                           \
		struct CheckpointNode *nodes = GAME_TRACKER->level1->ptr_restart_points;               \
		(destination) = &nodes[nodeIndex];                                                     \
		if ((cDC->flags & CAMERA_FLAG_TRACK_PATH_ALT_BRANCH) != 0 && (from)->altIndex != 0xff) \
		{                                                                                      \
			(destination) = &nodes[(from)->altIndex];                                          \
		}                                                                                      \
	} while (0)

#define CAM_TRACK_LENGTH(destination)                                                                                                                          \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		delta.x = next->pos.x - curr->pos.x;                                                                                                                   \
		delta.y = next->pos.y - curr->pos.y;                                                                                                                   \
		delta.z = next->pos.z - curr->pos.z;                                                                                                                   \
		(destination) =                                                                                                                                        \
		    SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(delta.x, delta.x), CTR_MipsMulLo(delta.y, delta.y)), CTR_MipsMulLo(delta.z, delta.z))); \
	} while (0)

u32 CAM_FollowDriver_TrackPath(struct CameraDC *cDC, SVec3 *pos, s32 speed, s32 updateArg)
{
	// NOTE(aalhendi): Keep the input flag in S3 so it does not displace the
	// long-lived camera owner from S4. The later ratio is a separate value;
	// binding that arithmetic to the flag would also bind the DIV numerator.
	register s32 update CTR_PSX_REGISTER("$19") = updateArg;
	struct CheckpointNode *curr;
	struct CheckpointNode *next;
	Vec3 delta;
	s32 progress;
	s32 length;
	s32 ratio;
	s32 nextYaw;
	s32 yawDelta;

	if (speed > 0)
	{
		s32 firstLength;
		s32 pauseFlags;

		// NOTE(aalhendi): Capture pause before publishing the first length;
		// its return value remains live while the tracker mask is read.
		curr = cDC->trackPathNode;
		CAM_TRACK_SELECT(next, curr, curr->nextIndex_backward, nextIndex_right);
		CAM_TRACK_LENGTH(firstLength);
		pauseFlags = GAME_TRACKER->gameMode1 & PAUSE_ALL;
		length = firstLength;
		if (pauseFlags != 0)
		{
			progress = 0;
		}
		else
		{
			progress = CTR_MipsAddLo(cDC->trackPathProgress, speed);
		}
		while (progress >= length)
		{
			progress = CTR_MipsSubLo(progress, length);
			curr = next;
			CAM_TRACK_SELECT(next, curr, (length = curr->nextIndex_backward), nextIndex_right);
			CAM_TRACK_LENGTH(length);
		}
	}
	else
	{
		s32 firstLength;
		s32 pauseFlags;
		curr = cDC->trackPathNode;
		CAM_TRACK_SELECT(next, curr, curr->nextIndex_forward, nextIndex_left);
		CAM_TRACK_LENGTH(firstLength);
		pauseFlags = GAME_TRACKER->gameMode1 & PAUSE_ALL;
		length = firstLength;
		if (pauseFlags != 0)
		{
			progress = 0;
		}
		else
		{
			progress = CTR_MipsSubLo(cDC->trackPathProgress, speed);
		}
		while (progress >= length)
		{
			progress = CTR_MipsSubLo(progress, length);
			curr = next;
			CAM_TRACK_SELECT(next, curr, (length = curr->nextIndex_forward), nextIndex_left);
			CAM_TRACK_LENGTH(length);
		}
	}

	if (update)
	{
		cDC->trackPathProgress = progress;
		cDC->trackPathNode = curr;
	}
	if (length != 0)
	{
		ratio = CTR_MipsDiv(CTR_MipsSll(progress, 12), length);
	}
	else
	{
		ratio = 0;
	}
	pos->x = (u16)curr->pos.x + CTR_MipsSra(CTR_MipsMulLo(delta.x, ratio), 12);
	pos->y = (u16)curr->pos.y + CTR_MipsSra(CTR_MipsMulLo(delta.y, ratio), 12) + 0x80;
	pos->z = (u16)curr->pos.z + CTR_MipsSra(CTR_MipsMulLo(delta.z, ratio), 12);
	// NOTE(aalhendi): State publication ends the progress/current-node lives.
	// Reuse them for the first yaw and the lookahead node, respectively.
	progress = ratan2(delta.x, delta.z) + 0x800;
	if (speed > 0)
	{
		CAM_TRACK_SELECT(curr, next, next->nextIndex_backward, nextIndex_right);
	}
	else
	{
		CAM_TRACK_SELECT(curr, next, next->nextIndex_forward, nextIndex_left);
	}
	nextYaw = ratan2(curr->pos.x - next->pos.x, curr->pos.z - next->pos.z) + 0x800;
	yawDelta = (nextYaw - progress) & 0xfff;
	if (yawDelta >= 0x800)
	{
		yawDelta -= 0x1000;
	}
	return (progress + CTR_MipsSra(CTR_MipsMulLo(yawDelta, ratio), 12)) & 0xfff;
}

#undef CAM_TRACK_LENGTH
#undef CAM_TRACK_SELECT
