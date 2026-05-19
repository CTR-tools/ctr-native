#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af994-0x800afbc8
void CS_Podium_Prize_ThTick3(struct Thread *th)
{
	struct GameTracker *gGT;
	struct Instance *inst = th->inst;
	short *prize = th->object;
	short framesLeft;

	framesLeft = prize[0x13] - 1;
	prize[0x13] = framesLeft;

	if (framesLeft != 0)
	{
		int frameMax = prize[0x14];
		int xInterp = framesLeft * (0x100 - prize[8]);
		int yInterp = framesLeft * (0x6c - prize[9]);
		int x;
		int y;
		short scale;

		x = (prize[8] + xInterp / frameMax - 0x100) * -inst->matrix.t[2];
		if (x < 0)
			x += 0xff;

		inst->matrix.t[0] = x >> 8;

		y = (prize[9] + yInterp / frameMax - 0x6c) * inst->matrix.t[2];
		if (y < 0)
			y += 0xff;

		inst->matrix.t[1] = y >> 8;

		scale = inst->scale[0] - 0x4b0;
		if (scale < 0x1001)
			scale = 0x1000;

		inst->scale[0] = scale;
		inst->scale[1] = scale;
		inst->scale[2] = scale;

		CS_Podium_Prize_Spin(inst, prize);
		return;
	}

	if (DECOMP_CS_Camera_BoolGotoBoss() == 0)
	{
		u_int rewards = sdata->advProgress.rewards[4];
		short hintID = 0;

		if ((rewards & 0x4000) == 0)
			hintID = 0x18;
		else if ((rewards & 0x1000) == 0)
			hintID = 0x16;
		else if ((rewards & 0x2000) == 0)
			hintID = 0x17;
		else if ((rewards & 0x10) == 0)
			hintID = 0xe;
		else if ((rewards & 0x20) == 0)
			hintID = 0xf;
		else if ((rewards & 0x40) == 0)
			hintID = 0x10;
		else if ((rewards & 0x80) == 0)
			hintID = 0x11;

		if (hintID != 0)
			DECOMP_MainFrame_RequestMaskHint(hintID, 0);
	}

	gGT = sdata->gGT;
	gGT->overlayTransition = 2;
	gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;

	DECOMP_OtherFX_Play(0x67, 1);

	th->flags |= 0x800;
}
