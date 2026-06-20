#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae54c-0x800ae81c
void CS_Thread_ThTick(struct Thread *t)
{
	// Retail uses scratchpad 0x1f800108/0x1f800118 for parent frame-data temporaries.
	s16 *parentPos = CTR_SCRATCHPAD_PTR(s16, 0x108);
	s16 *parentRot = CTR_SCRATCHPAD_PTR(s16, 0x118);
	s16 bonePos[3];
	struct CutsceneObj *cs = t->object;
	struct Instance *inst = t->inst;
	struct Instance *parentInst;
	struct Thread *parentThread;

	if (CS_Thread_UseOpcode(inst, cs))
	{
		t->flags |= THREAD_FLAG_DEAD;

		if ((sdata->gGT->gameMode2 & 0x80) != 0)
			return;
	}

	CS_Thread_MoveOnPath(t);
	CS_Thread_AnimateScale(t);
	CS_Thread_Particles(t);

	if ((cs->flags & 0x40) != 0)
		CS_Thread_InterpolateFramesMS(t);

	// ASM: 0x800ae5dc - parent-thread frameOverrideRoot processing
	if (inst != 0)
	{
		parentThread = t->parentThread;

		if (parentThread != 0)
		{
			if ((cs->flags & 0x4) == 0)
			{
				parentInst = parentThread->inst;

				CS_Instance_GetFrameData(parentInst, parentInst->animIndex, parentInst->animFrame, (u16 *)parentPos, (u16 *)parentRot, 0);

				inst->matrix.t[0] = parentInst->matrix.t[0] + parentPos[0];
				inst->matrix.t[1] = parentInst->matrix.t[1] + parentPos[1];
				inst->matrix.t[2] = parentInst->matrix.t[2] + parentPos[2];

				if ((cs->flags & 0x10) == 0)
				{
					ConvertRotToMatrix(&inst->matrix, parentRot);
				}
			}
		}

		inst = t->inst;
		if (inst == 0)
			goto thTick_subtitles;

		// ASM: 0x800ae6b4 - flag 0x8 writes bone Y to overlay-233 mutable state.
		if ((cs->flags & 0x8) != 0)
		{
			CS_Instance_GetFrameData(inst, inst->animIndex, inst->animFrame, (u16 *)bonePos, 0, 0);

			D233.VertSplitLine = bonePos[1];

			inst = t->inst;
			if (inst == 0)
				goto thTick_subtitles;
		}

		// ASM: 0x800ae6fc - flag 0x2: random alphaScale for fade effect
		if ((cs->flags & 0x2) != 0)
		{
			inst->alphaScale = 0;

			if ((sdata->gGT->timer & 0x1) != 0)
			{
				inst->alphaScale = (MixRNG_Scramble() & 0x7ff) + 1024;
			}
		}
	}

	// ASM: 0x800ae744 - subtitle rendering
thTick_subtitles:
	if (cs->Subtitles.lngIndex > 0)
	{
		struct GameTracker *gGT = sdata->gGT;
		int textWidth;
		u16 textRect[4];

		textWidth = DecalFont_DrawMultiLine(sdata->lngStrings[cs->Subtitles.lngIndex], cs->Subtitles.textPos.x, cs->Subtitles.textPos.y, 460,
		                                    cs->Subtitles.font, cs->Subtitles.colors);

		textRect[0] = (u16)((u16)cs->Subtitles.textPos.x - 236);
		textRect[1] = (u16)((u16)cs->Subtitles.textPos.y - 4);
		textRect[2] = 472;
		textRect[3] = (u16)((s16)textWidth + 8);

		RECTMENU_DrawInnerRect((RECT *)textRect, 4, gGT->backBuffer->otMem.uiOT);
	}

thTick_epilogue:
	// ASM: 0x800ae7dc - check isCutsceneOver, re-apply death flag
	if (D233.isCutsceneOver != 0)
	{
		t->flags |= THREAD_FLAG_DEAD;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af328-0x800af7c0
struct Thread *CS_Thread_Init(s16 modelID, const char *name, s16 *param_3, s16 param_4, struct Thread *parent)
{
	struct GameTracker *gGT = sdata->gGT;
	struct CutsceneObj *cs;
	struct Instance *inst;
	struct Thread *t;
	char *scriptPtr;
	u32 bucket;
	s16 *meta;

	if (modelID == NOFUNC)
	{
		inst = NULL;

		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(0x60, NONE, MEDIUM, CAMERA), CS_Thread_ThTick, name, parent);

		if (t == NULL)
			return NULL;
	}
	else
	{
		bucket = OTHER;

		if ((u32)(modelID - NDI_KART6) < 2)
			bucket = AKUAKU;

		if ((u32)(modelID - NDI_KART0) < 4)
			bucket = GHOST;

		inst = INSTANCE_BirthWithThread(modelID, name, MEDIUM, bucket, CS_Thread_ThTick, 0x60, parent);

		if (inst == NULL)
			return NULL;

		t = inst->thread;
		t->funcThDestroy = PROC_DestroyInstance;
	}

	cs = t->object;

	cs->metadata = (int *)&cs->decodedOpcode;
	cs->frameOverrideRoot = NULL;
	cs->prevOpcode = (char *)-1;
	cs->Subtitles.lngIndex = -1;

	if (modelID == NOFUNC)
	{
		int level = gGT->levelID;

		if (level == NAUGHTY_DOG_CRATE)
		{
			scriptPtr = (char *)&R233.creditsOpcodeData[0x18];
		}
		else if (level == OXIDE_ENDING)
		{
			scriptPtr = (char *)&R233.introEndingOpcodeData[0];
		}
		else if (level == OXIDE_TRUE_ENDING)
		{
			scriptPtr = (char *)&R233.introEndingOpcodeData[0x30];
		}
		else if ((gGT->gameMode2 & CREDITS) == 0)
		{
			scriptPtr = R233.introCutsceneOpcodes[level - INTRO_RACE_TODAY];
		}
		else
		{
			scriptPtr = R233.creditsCutsceneOpcodes[level - CREDITS_CRASH];
		}
	}
	else
	{
		if (modelID >= NDI_BOX_BOX_01)
		{
			if ((u32)(modelID - NDI_BOX_BOX_01) < 0x2b)
			{
				scriptPtr = R233.boxModelScripts[modelID - NDI_BOX_BOX_01];
			}
			else
			{
				scriptPtr = (char *)R233.script_default;
			}

			CS_ScriptCmd_OpcodeAt(cs, scriptPtr);

			if ((u32)(modelID - NDI_KART0) < 4)
			{
				cs->frameOverrideRoot = (int *)&D233.cs_initMatrixTable[modelID - NDI_KART0];
			}

			goto after_opcode;
		}

		if ((u32)(modelID - STATIC_PINHEAD) < 5)
		{
			scriptPtr = (char *)R233.script_default;
		}
		else if (modelID == STATIC_DINGOFIRE)
		{
			scriptPtr = (char *)R233.script_dingofire;
		}
		else if ((u32)(modelID - STATIC_TAWNA1) < 4)
		{
			if (gGT->gameMode2 & CREDITS)
				scriptPtr = (char *)R233.script_tawnaCredits;
			else
				scriptPtr = (char *)R233.script_tawnaNormal;
		}
		else if ((u32)(modelID - STATIC_CRASHDANCE) < 0x10)
		{
			char *const *base;
			int off = (modelID - STATIC_CRASHDANCE);

			if (modelID == gGT->podium_modelIndex_First)
				base = R233.danceFirstScripts;
			else
				base = R233.danceOtherScripts;

			scriptPtr = base[off];
		}
		else
		{
			scriptPtr = (char *)R233.script_default;
		}
	}

	CS_ScriptCmd_OpcodeAt(cs, scriptPtr);

after_opcode:

	cs->unk18 = cs->metadata[2];

	meta = (s16 *)cs->metadata;
	cs->opcodeDuration = meta[2] + (s16)(((MixRNG_Scramble() >> 2 & 0xfff) * ((meta[3] - meta[2]) + 1)) >> 0xc);

	if (inst != NULL)
	{
		MTC2(*(int *)(param_3 + 4), 0);
		MTC2(*(int *)(param_3 + 6), 1);
		gte_llv0();

		int rx = MFC2(25);
		int ry = MFC2(26);
		int rz = MFC2(27);

		inst->matrix.t[0] = rx + param_3[0];
		inst->matrix.t[1] = ry + param_3[1];
		inst->matrix.t[2] = rz + param_3[2];

		if (gGT->levelID != NAUGHTY_DOG_CRATE)
		{
			inst->scale.x = 0x2800;
			inst->scale.y = 0x2800;
			inst->scale.z = 0x2800;
		}

		if ((u32)(gGT->levelID - GEM_STONE_VALLEY) < 5)
		{
			inst->depthBiasNormal -= 4;
			inst->depthBiasSecondary -= 4;
		}

		param_3[0xc] = param_3[8];
		param_3[0xe] = param_3[10];
		param_3[0xd] = param_3[9] + param_4;

		ConvertRotToMatrix(&inst->matrix, param_3 + 0xc);

		cs->unk1c = param_3[0xd] & 0xfff;
		cs->unk20 = param_3[0xc] & 0xfff;
		cs->unk22 = param_3[0xd] & 0xfff;
		cs->unk24 = param_3[0xe] & 0xfff;
	}

	cs->particleID = 0xff;
	cs->pathProgress32 = 0;
	cs->unk1e = 0;
	cs->flags = 0;
	cs->scaleSpeed = 0;
	cs->desiredScale = 0x2800;

	cs->unk4 = 0;
	cs->unk6 = 0;
	cs->unk8 = 0x2e808080;
	cs->unk_C = 0;
	cs->unk_E = 0;

	cs->ptrIcons = (struct IconGroup *)((char *)gGT->iconGroup[0] + sizeof(struct IconGroup));

	return t;
}
