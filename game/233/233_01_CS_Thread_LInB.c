#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b06ac-0x800b087c
void DECOMP_CS_Thread_LInB(struct Instance *inst)
{
	struct Thread *t;
	struct CutsceneObj *cs;
	short modelID;
	char *scriptPtr;

	OVR_233.isCutsceneOver = 0;

	if (inst->thread != 0)
		goto check_polar;

	t = DECOMP_PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(0x60, NONE, MEDIUM, STATIC), DECOMP_CS_Thread_ThTick, OVR_233.s_introguy, 0);

	inst->thread = t;

	if (t == 0)
		return;

	cs = t->object;

	t->inst = inst;

	cs->metadata = (int *)&cs->decodedOpcode;
	cs->prevOpcode = (char *)-1;
	cs->Subtitles.lngIndex = -1;

	modelID = inst->model->id;

	if (modelID < NDI_BOX_BOX_01)
	{
		if ((u_short)(modelID - STATIC_CRASHINTRO) < 0x10)
		{
			scriptPtr = OVR_233.introModelScripts[modelID - STATIC_CRASHINTRO];
		}
		else
		{
			scriptPtr = OVR_233.script_default;
		}
	}
	else
	{
		scriptPtr = OVR_233.boxModelScripts[modelID - NDI_BOX_BOX_01];
	}

	DECOMP_CS_ScriptCmd_OpcodeAt(cs, scriptPtr);

	cs->unk18 = *(int *)(cs->metadata + 2);

	{
		int rng = DECOMP_MixRNG_Scramble();
		short *meta = (short *)cs->metadata;
		short frameStart = meta[2];
		short frameEnd = meta[3];

		cs->unk1c = 0;
		cs->unk20 = 0;
		cs->unk22 = 0;
		cs->unk24 = 0;
		cs->unk28 = 0;
		cs->unk1e = 0;
		cs->flags = 0;
		cs->scaleSpeed = 0;
		cs->frameOverrideRoot = 0;
		cs->desiredScale = 0x1000;
		cs->particleID = 0xff;

		cs->unk14 = frameStart + (short)(((rng >> 2 & 0xfff) * ((frameEnd - frameStart) + 1)) >> 0xc);

		struct GameTracker *gGT = sdata->gGT;

		cs->unk4 = 0;
		cs->unk6 = 0;
		cs->unk8 = 0x2e808080;
		cs->unk_C = 0;
		cs->unk_E = 0;

		cs->ptrIcons = (struct IconGroup *)((char *)gGT->iconGroup[0] + sizeof(struct IconGroup));
	}

check_polar:
	if (sdata->gGT->levelID == 0x21)
	{
		inst->vertSplit = 0;
		inst->flags |= REFLECTIVE;
	}
}

#define CS_Thread_LInB DECOMP_CS_Thread_LInB
