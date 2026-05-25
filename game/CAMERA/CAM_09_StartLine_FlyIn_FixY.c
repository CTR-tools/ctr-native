#include <common.h>

static void CAM_StartLine_FlyIn_FixY_SetPoint(u32 point[2], s16 x, s16 y, s16 z)
{
	point[0] = (u16)x | ((u32)(u16)y << 0x10);
	point[1] = (u16)z;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018ec0-0x80018fec.
void CAM_StartLine_FlyIn_FixY(s16 *posRot)
{
	struct ScratchpadStruct *sps = &sdata->scratchpadStruct;
	s16 pos[3];
	u32 posTop[2];
	u32 posBottom[2];
	int i;

	sps->Union.QuadBlockColl.qbFlagsWanted = 0x3000;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = 2;
	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;

	pos[0] = posRot[0];
	pos[1] = posRot[1];
	pos[2] = posRot[2];

	for (i = 0; i < 8; i++)
	{
		s16 probeOffset = i * 0x400;

		CAM_StartLine_FlyIn_FixY_SetPoint(posTop, pos[0], pos[1] - (probeOffset + 0x400), pos[2]);
		CAM_StartLine_FlyIn_FixY_SetPoint(posBottom, pos[0], pos[1] - (probeOffset - 0x100), pos[2]);

		COLL_SearchBSP_CallbackQUADBLK(posTop, posBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			pos[0] = sps->Union.QuadBlockColl.hitPos[0];
			pos[1] = sps->Union.QuadBlockColl.hitPos[1];
			pos[2] = sps->Union.QuadBlockColl.hitPos[2];
			break;
		}
	}

	posRot[1] = pos[1];
}
