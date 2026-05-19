#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800afe90-0x800b021c
void CS_Podium_Prize_Init(u_int prizeModel, char *prizeName, short *posOnScreen)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst;
	short *prize;
	int tx;
	int ty;
	int tz;

	inst = DECOMP_INSTANCE_BirthWithThread(prizeModel, prizeName, MEDIUM, OTHER, CS_Podium_Prize_ThTick1, 0x2c, NULL);

	if (inst == NULL)
	{
		if (OVR_233.cutsceneState < 1)
			OVR_233.cutsceneState = 1;

		gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;
		return;
	}

	inst->scale[0] = 0x2000;
	inst->scale[1] = 0x2000;
	inst->scale[2] = 0x2000;
	inst->flags |= HIDE_MODEL;

	prize = inst->thread->object;
	inst->thread->funcThDestroy = DECOMP_CS_Podium_Prize_ThDestroy;

	prize[0x11] = 0x40;
	prize[0x12] = 0x200;
	prize[4] = 0;
	prize[5] = 0;
	prize[6] = 0;

	MTC2(0, 0);
	MTC2(0x40, 1);
	gte_llv0();

	tx = MFC2(25);
	ty = MFC2(26);
	tz = MFC2(27);

	prize[0] = posOnScreen[0] + (short)tx;
	prize[1] = posOnScreen[1] + (short)ty + 0x1c0;
	prize[2] = posOnScreen[2] + (short)tz;
	prize[10] = -0x200;

	switch (prizeModel)
	{
	case STATIC_BIG1:
		inst->flags |= HIDE_MODEL;
		goto center_target;

	case STATIC_GEM:
	{
		short *gemColor = data.AdvCups[gGT->cup.cupID].color;

		inst->colorRGBA = (gemColor[0] << 20) | (gemColor[1] << 12) | (gemColor[2] << 4);
		prize[0xc] = 0x5d3;
		prize[0xd] = 0x718;
		prize[0xe] = 0x590;
		prize[0xf] = 0x609;
		inst->flags |= USE_SPECULAR_LIGHT;
		goto center_target;
	}

	default:
	center_target:
		prize[8] = 0x100;
		prize[9] = 0x6c;
		return;

	case STATIC_RELIC:
	{
		struct UiElement2D *hud = data.hudStructPtr[0];
		u_int bitIndex = gGT->prevLEV + 0x3a;
		u_int relicColor;

		prize[8] = hud[0xe].x;
		prize[9] = hud[0xe].y - 0x3c;

		if (CHECK_ADV_BIT(sdata->advProgress.rewards, bitIndex) == 0)
		{
			bitIndex = gGT->prevLEV + 0x28;

			if (CHECK_ADV_BIT(sdata->advProgress.rewards, bitIndex) == 0)
				relicColor = 0x20a5ff0;
			else
				relicColor = 0xd8d2090;
		}
		else
		{
			relicColor = 0xffede90;
		}

		inst->colorRGBA = relicColor;
		prize[0xc] = 0x2ab;
		prize[0xd] = 0x436;
		prize[0xe] = 0x1eb;
		prize[0xf] = 0x670;
		inst->flags |= USE_SPECULAR_LIGHT;

		gGT->gameMode2 |= INC_RELIC;
		return;
	}

	case STATIC_TROPHY:
	{
		struct UiElement2D *hud = data.hudStructPtr[0];

		prize[8] = hud[0x10].x;
		prize[9] = hud[0x10].y - 0x3c;
		prize[10] = -200;

		inst->scale[0] = 0x4000;
		inst->scale[1] = 0x4000;
		inst->scale[2] = 0x4000;

		gGT->gameMode2 |= INC_TROPHY;
		return;
	}

	case STATIC_KEY:
	{
		struct UiElement2D *hud = data.hudStructPtr[0];

		inst->colorRGBA = 0xdca6000;
		prize[0xc] = 0x1d9;
		prize[0xd] = 0x5db;
		prize[0xe] = 0x2da;
		prize[0xf] = 0x54b;
		inst->flags |= USE_SPECULAR_LIGHT;

		prize[8] = hud[0xf].x;
		prize[9] = hud[0xf].y - 0x3c;

		gGT->gameMode2 |= INC_KEY;
		return;
	}
	}
}
