#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af994-0x800afbc8
void CS_Podium_Prize_ThTick3(struct Thread *th)
{
	struct GameTracker *gGT;
	struct Instance *inst = th->inst;
	s16 *prize = th->object;
	s16 framesLeft;

	framesLeft = prize[0x13] - 1;
	prize[0x13] = framesLeft;

	if (framesLeft != 0)
	{
		int frameMax = prize[0x14];
		int xInterp = framesLeft * (0x100 - prize[8]);
		int yInterp = framesLeft * (0x6c - prize[9]);
		int x;
		int y;
		s16 scale;

		x = (prize[8] + xInterp / frameMax - 0x100) * -inst->matrix.t[2];
		if (x < 0)
			x += 0xff;

		inst->matrix.t[0] = x >> 8;

		y = (prize[9] + yInterp / frameMax - 0x6c) * inst->matrix.t[2];
		if (y < 0)
			y += 0xff;

		inst->matrix.t[1] = y >> 8;

		scale = inst->scale.x - 0x4b0;
		if (scale < 0x1001)
			scale = 0x1000;

		inst->scale.x = scale;
		inst->scale.y = scale;
		inst->scale.z = scale;

		CS_Podium_Prize_Spin(inst, prize);
		return;
	}

	if (CS_Camera_BoolGotoBoss() == 0)
	{
		u32 rewards = sdata->advProgress.hintFlags;
		s16 hintID = 0;

		if ((rewards & ADV_REWARD_HINT_MAP_INFORMATION_MASK) == 0)
			hintID = ADV_MASK_HINT_ID_MAP_INFORMATION;
		else if ((rewards & ADV_REWARD_HINT_WUMPA_FRUIT_MASK) == 0)
			hintID = ADV_MASK_HINT_ID_WUMPA_FRUIT;
		else if ((rewards & ADV_REWARD_HINT_TNT_MASK) == 0)
			hintID = ADV_MASK_HINT_ID_TNT;
		else if ((rewards & ADV_REWARD_HINT_HANG_TIME_TURBO_MASK) == 0)
			hintID = ADV_MASK_HINT_ID_HANG_TIME_TURBO;
		else if ((rewards & ADV_REWARD_HINT_POWER_SLIDE_MASK) == 0)
			hintID = ADV_MASK_HINT_ID_POWER_SLIDE;
		else if ((rewards & ADV_REWARD_HINT_TURBO_BOOST_MASK) == 0)
			hintID = ADV_MASK_HINT_ID_TURBO_BOOST;
		else if ((rewards & ADV_REWARD_HINT_BRAKE_SLIDE_MASK) == 0)
			hintID = ADV_MASK_HINT_ID_BRAKE_SLIDE;

		if (hintID != 0)
			MainFrame_RequestMaskHint(hintID, 0);
	}

	gGT = sdata->gGT;
	gGT->overlayTransition = 2;
	gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;

	OtherFX_Play(0x67, 1);

	th->flags |= THREAD_FLAG_DEAD;
}
