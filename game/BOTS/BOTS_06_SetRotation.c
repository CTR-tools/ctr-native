#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013444-0x800135d8
void BOTS_SetRotation(struct Driver *bot, int param_2)
{
	struct NavFrame *nf = bot->botData.botNavFrame;

	bot->botData.unk5bc.ai_velAxis[0] = 0;
	bot->botData.unk5bc.ai_velAxis[1] = 0;
	bot->botData.unk5bc.ai_velAxis[2] = 0;

	// ======== Get Driver Position =============

	bot->botData.estimatePos[0] = (s16)(bot->posCurr.x >> 8);
	bot->botData.estimatePos[1] = (s16)(bot->posCurr.y >> 8);
	bot->botData.estimatePos[2] = (s16)(bot->posCurr.z >> 8);

	// ======== Compare to Nav Position =============

	int dx = nf->pos[0] - bot->botData.estimatePos[0];
	int dy = nf->pos[1] - bot->botData.estimatePos[1];
	int dz = nf->pos[2] - bot->botData.estimatePos[2];

	// ======== Calculate Distance =============

	// xz dist from driver to nav
	int xzDist = SquareRoot0_stub(dx * dx + dz * dz);
	bot->botData.distToNextNavXZ = xzDist;
	// xyz distance from driver to nav
	int xyzDist = SquareRoot0_stub(dx * dx + dy * dy + dz * dz);
	bot->botData.distToNextNavXYZ = xyzDist;

	// ======== Calculate Rotation =============

	int rot = ratan2(dy * 0x1000, bot->botData.distToNextNavXZ * 0x1000);
	bot->botData.estimateRotCurrY = rot >> 4;
	bot->botData.unk5a8 = 0;

	// "if BOTS_ThTick_Drive or BOTS_Driver_Convert"
	if (param_2 == 0)
	{
		bot->botData.estimateRotNav[0] = nf->rot[0];
		rot = ratan2(-dx, -dz);
		bot->botData.estimateRotNav[1] = ((rot + 0x800) >> 4);
		bot->botData.estimateRotNav[2] = nf->rot[1];
	}
	else
	{
		bot->botData.estimateRotNav[1] = (char)((sdata->gGT->level1->DriverSpawn[0].rot[1] + 0x400) >> 4);
	}

	s16 v = bot->botData.estimateRotNav[1] << 4;

	// why does the Driver class have so many ways to store y rotation >:(
	bot->botData.ai_rotY_608 = v;
	bot->angle = v;
	bot->rotCurr.y = v;
	bot->rotPrev.y = v;
	bot->botData.ai_rot4[1] = v;

	bot->botData.botFlags |= 1;
}
