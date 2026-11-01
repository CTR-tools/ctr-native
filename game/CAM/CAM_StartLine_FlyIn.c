#include <common.h>

void CAM_StartLine_FlyIn(struct FlyInData *flyInData, s16 maxFrames, s32 frame, SVec3 *desiredPos, SVec3 *desiredRot)
{
	struct GameTracker *gGT;
	struct Level *lev;
	SVec3 *spawn1;
	SVec3 *spawn2;
	s32 frameIndex = CTR_MipsSra((s32)((u32)frame << 16), 4);
	s32 frameRatio = CTR_MipsDiv(frameIndex, maxFrames);
	s32 countEnd = flyInData->frameCount1;
	// NOTE(aalhendi): Retain the signed comparison value separately from the halfword path length.
	s32 countStart = flyInData->frameCount2;
	s16 count = countStart;
	SVECTOR eyeOffset;
	SVECTOR lookAtOffset;
	MATRIX matrix;
	SVec3 rot;
	VECTOR transformed;
	SVec3 delta;
	s32 flags[2];
	s16 *pathEnd;
	s16 *pathStart;
	s32 pathRatioEnd = frameRatio;

	if (countStart < countEnd)
	{
		count = flyInData->frameCount1;
	}

	{
		s16 pathIndex = (s16)(CTR_MipsSra(CTR_MipsMulLo(count, frameRatio), 12));
		if (pathIndex < countEnd - 1)
		{
			pathEnd = (s16 *)(flyInData->ptrEnd + pathIndex * 6);
		}
		else
		{
			pathEnd = (s16 *)(flyInData->ptrEnd + countEnd * 6 - 0xc);
			pathRatioEnd = 0x1000;
		}

		if (pathIndex < flyInData->frameCount2 - 1)
		{
			pathStart = (s16 *)(flyInData->ptrStart + pathIndex * 6);
		}
		else
		{
			pathStart = (s16 *)(flyInData->ptrStart + flyInData->frameCount2 * 6 - 0xc);
			frameRatio = 0x1000;
		}

		{
			s32 ratio = CTR_MipsMulLo(count, pathRatioEnd) & 0xfff;
			eyeOffset.vx = pathEnd[0] + (s16)(((pathEnd[3] - pathEnd[0]) * ratio) >> 0xc);
			eyeOffset.vy = pathEnd[1] + (s16)(((pathEnd[4] - pathEnd[1]) * ratio) >> 0xc);
			eyeOffset.vz = pathEnd[2] + (s16)(((pathEnd[5] - pathEnd[2]) * ratio) >> 0xc);

			ratio = CTR_MipsMulLo(count, frameRatio) & 0xfff;
			lookAtOffset.vx = pathStart[0] + (s16)(((pathStart[3] - pathStart[0]) * ratio) >> 0xc);
			lookAtOffset.vy = pathStart[1] + (s16)(((pathStart[4] - pathStart[1]) * ratio) >> 0xc) - 0x60;
			lookAtOffset.vz = pathStart[2] + (s16)(((pathStart[5] - pathStart[2]) * ratio) >> 0xc);

			gGT = GAME_TRACKER;
			rot.x = gGT->level1->DriverSpawn[0].rot.x;
			rot.y = gGT->level1->DriverSpawn[0].rot.y;
			rot.z = gGT->level1->DriverSpawn[0].rot.z;
			rot.y += 0x400;

			ConvertRotToMatrix(&matrix, &rot);

			lev = GAME_TRACKER->level1;
			spawn1 = &lev->DriverSpawn[1].pos;
			CAM_StartLine_FlyIn_FixY(spawn1);
			spawn2 = &lev->DriverSpawn[2].pos;
			CAM_StartLine_FlyIn_FixY(spawn2);
			CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[5].pos);

			matrix.t[0] = lev->DriverSpawn[1].pos.x + (lev->DriverSpawn[2].pos.x - lev->DriverSpawn[1].pos.x) / 2;
			matrix.t[1] = spawn1->y + (spawn2->y - spawn1->y) / 2 + 0x40;
			matrix.t[2] = spawn1->z + (spawn2->z - spawn1->z) / 2;

			SetRotMatrix(&matrix);
			SetTransMatrix(&matrix);

			RotTrans(&eyeOffset, &transformed, flags);
			desiredPos->x = (s16)transformed.vx;
			desiredPos->y = (s16)transformed.vy;
			desiredPos->z = (s16)transformed.vz;

			RotTrans(&lookAtOffset, &transformed, flags);

			{
				delta.x = desiredPos->x - (s16)transformed.vx;
				delta.y = desiredPos->y - (s16)transformed.vy;
				delta.z = desiredPos->z - (s16)transformed.vz;

				desiredRot->y = (s16)ratan2(delta.x, delta.z);
				desiredRot->x = 0x800 - (s16)ratan2(delta.y, SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsMulLo(delta.x, delta.x), CTR_MipsMulLo(delta.z, delta.z))));
				desiredRot->z = 0;
			}
		}
	}
}
