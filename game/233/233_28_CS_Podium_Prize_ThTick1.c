#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800afcc4-0x800afe58
void CS_Podium_Prize_ThTick1(struct Thread *th)
{
	struct Instance *inst = th->inst;
	short *prize = th->object;
	int trig;

	if (OVR_233.PodiumInitUnk3 != 0)
	{
		if (th->modelIndex != STATIC_BIG1)
			inst->flags &= ~HIDE_MODEL;

		prize[0x12] = DECOMP_VehCalc_InterpBySpeed(prize[0x12], 0x14, 0);
		prize[0x11] = DECOMP_VehCalc_InterpBySpeed(prize[0x11], 1, 0);
	}

	trig = DECOMP_MATH_Sin(prize[5]);
	inst->matrix.t[0] = prize[0] + ((prize[0x11] * trig) >> 12);
	inst->matrix.t[1] = prize[1] + prize[0x12];

	trig = DECOMP_MATH_Cos(prize[5]);
	inst->matrix.t[2] = prize[2] + ((prize[0x11] * trig) >> 12);

	if (OVR_233.isCutsceneOver == 0)
	{
		CS_Podium_Prize_Spin(inst, prize);
		return;
	}

	prize[0x14] = 0xf;
	prize[0x13] = 0xf;
	prize[0x15] = 0;

	inst->unk50 = 0x80;
	inst->unk51 = 0x80;

	inst->scale[0] = 0x1000;
	inst->scale[1] = 0x1000;
	inst->scale[2] = 0x1000;

	inst->matrix.t[0] = 0;
	inst->matrix.t[1] = 0;
	inst->matrix.t[2] = prize[10];

	{
		struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);
		idpp[0].pushBuffer = &sdata->gGT->pushBuffer_UI;
	}

	DECOMP_OtherFX_Stop2(0xaf);
	DECOMP_OtherFX_Stop2(0xae);
	DECOMP_OtherFX_Play(0x9a, 1);

	ThTick_SetAndExec(th, CS_Podium_Prize_ThTick2);
}
