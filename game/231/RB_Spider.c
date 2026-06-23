#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b95fc-0x800b9848.
void RB_Spider_DrawWebs(struct Thread *t, struct PushBuffer *pb)
{
	typedef struct
	{
		u32 tag;
		u32 tpage;
		LINE_F2 f2;
	} multiCmdPacket;

	typedef struct
	{
		u32 topXY;
		u32 bottomXY;
		s32 z;
	} WebLine;

	CTR_STATIC_ASSERT_LAYOUT(sizeof(multiCmdPacket) == 0x18);
	CTR_STATIC_ASSERT_LAYOUT(sizeof(WebLine) == 0xc);

	struct GameTracker *gGT;
	struct PrimMem *primMem;
	MATRIX *m;
	multiCmdPacket *p;
	multiCmdPacket *nextPrim;

	u32 lineColor;
	u32 *ot;
	int depth;
	WebLine *scratchpad;
	WebLine *line;
	int numSpiders;

	gGT = sdata->gGT;
	primMem = &gGT->backBuffer->primMem;

	// quit if there are no spiders
	if (t == NULL)
		return;

	scratchpad = CTR_SCRATCHPAD_PTR(WebLine, 0);
	line = scratchpad;

	// all threads
	for (numSpiders = 0; t != NULL; numSpiders++)
	{
		struct Instance *inst = t->inst;
		struct InstDef *instDef = inst->instDef;
		u16 x = (u16)instDef->pos.x;
		s32 z = instDef->pos.z;

		line->topXY = (u32)x | ((u32)(u16)(instDef->pos.y + 0x540) << 16);
		line->bottomXY = (u32)x | ((u32)(u16)(inst->matrix.t[1] + 0x60) << 16);
		line->z = z;
		line++;

		t = t->siblingThread;
	}

	int i, j;
	int numPlyr;
	numPlyr = gGT->numPlyrCurrGame;

	p = primMem->cursor;
	nextPrim = p + (numSpiders * numPlyr);
	if (nextPrim >= (multiCmdPacket *)primMem->guardEnd)
		return;

	// loop through all players
	for (i = 0; i < numPlyr; i++)
	{
		m = &pb->matrix_ViewProj;

		// store on GTE
		gte_SetRotMatrix(m);
		gte_SetTransMatrix(m);

		line = scratchpad;

		// loop through spiders
		for (j = 0; j < numSpiders; j++)
		{
			MTC2(line->topXY, 0);
			MTC2((u32)line->z, 1);
			MTC2(line->bottomXY, 2);
			MTC2((u32)line->z, 3);
			gte_rtpt_b();
			depth = (s32)MFC2(17);

			// if line is close enough to the screen
			// to be seen, then generate primitives
			if ((u32)(depth - 1) < (0x1200 - 1))
			{
				p->tpage = 0xe1000a20;
				p->f2.tag = 0;

				*(u32 *)&p->f2.x0 = MFC2(12);
				*(u32 *)&p->f2.x1 = MFC2(13);

				lineColor = 0x3f;
				if (depth > 0xa00)
				{
					lineColor = ((0x1200 - depth) * 0x3f) >> 0xb;
				}

				p->f2.r0 = lineColor;
				p->f2.g0 = lineColor;
				p->f2.b0 = lineColor;
				p->f2.code = 0x42;

				depth = depth >> 6;
				if (depth > 0x3ff)
					depth = 0x3ff;

				// pushBuffer 0xf4, ptrOT
				ot = (u32 *)&pb->ptrOT[depth];

				// prim header, OT and prim len
				*(int *)p = CtrGpu_PackOTTag(*ot, 0x5000000);
				*ot = CtrGpu_PrimToOTLink24(p);
				p++;
			}

			line++;
		}

		pb++;
	}

	primMem->cursor = p;
}

s16 spiderArr[] = {
    // first 13
    0x4c0, 0x439, 0x3A6, 0x306, 0x266, 0x1c8, 0x139, 0xb9, 0x59, 0x17, 0, 0, 0,

    // next 13
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b9848-0x800b9bc0.

void RB_Spider_ThTick(struct Thread *t)
{
	s16 sVar2;
	int iVar3;

	u8 prevKartState;
	struct GameTracker *gGT;
	struct Instance *hitInst;
	struct Driver *victim;
	struct Instance *spiderInst;
	struct Spider *spider;

	spider = t->object;
	spiderInst = t->inst;

	if (spider->delay != 0)
	{
		spider->delay--;
		return;
	}

	spider->unused++;

	// If spider is on ground
	if (spider->boolNearRoof == 0)
	{
		if (4 < spider->animLoopCount)
		{
			// Play animation backwards
			sVar2 = spiderInst->animFrame - 1;

			// if animation is at beginning
			if (sVar2 < 1)
			{
				spiderInst->animFrame = 0;
				spider->animLoopCount = 0;
				spider->boolNearRoof = 1;
				goto setWiggleAnimation;
			}

			spiderInst->animFrame = sVar2;

			// last frame of last animation
			if (sVar2 == 0xc)
			{
				PlaySound3D(0x79, spiderInst);
			}

			goto updatePosScale;
		}

		sVar2 = spiderInst->animFrame;
		iVar3 = INSTANCE_GetNumAnimFrames(spiderInst, spiderInst->animIndex);

		if (iVar3 <= sVar2 + 1)
		{
			spiderInst->animFrame = 0;
			sVar2 = spider->animLoopCount;
			spider->animLoopCount = sVar2 + 1;

			if ((s16)(sVar2 + 1) == 5)
			{
				spiderInst->animIndex = 0;
				spiderInst->animFrame = INSTANCE_GetNumAnimFrames(spiderInst, 0) - 1;
			}

			goto checkCollision;
		}
	}

	// if spider is near ceiling
	else
	{
		if (4 < spider->animLoopCount)
		{
			sVar2 = spiderInst->animFrame;
			iVar3 = INSTANCE_GetNumAnimFrames(spiderInst, 0);

			if (sVar2 + 1 < iVar3)
			{
				spiderInst->animFrame++;
			}
			else
			{
				spider->animLoopCount = 0;
				spider->boolNearRoof = 0;
			setWiggleAnimation:
				spiderInst->animIndex = 1;
			}

		updatePosScale:
			spiderInst->matrix.t[1] = (int)spiderInst->instDef->pos.y + spiderArr[spiderInst->animFrame];

			if (spiderInst->animFrame < 0xb)
			{
				spider->shadowInst->scale.x = (s16)((spiderInst->animFrame << 0xc) / 10) + 0x1800;
				spider->shadowInst->scale.z = (s16)((spiderInst->animFrame << 0xc) / 10) + 0x1800;
			}

			goto checkCollision;
		}

		sVar2 = spiderInst->animFrame;
		iVar3 = INSTANCE_GetNumAnimFrames(spiderInst, spiderInst->animIndex);

		if (iVar3 <= sVar2 + 1)
		{
			spiderInst->animFrame = 0;
			sVar2 = spider->animLoopCount;
			spider->animLoopCount = sVar2 + 1;

			if ((s16)(sVar2 + 1) == 5)
			{
				spiderInst->animIndex = 0;
				spiderInst->animFrame = 0;
				PlaySound3D(0x7a, spiderInst);
			}

			goto checkCollision;
		}
	}

	spiderInst->animFrame++;

checkCollision:
	gGT = sdata->gGT;

	hitInst = (struct Instance *)LinkedCollide_Radius(spiderInst, t, gGT->threadBuckets[PLAYER].thread, 0x9000);
	if (hitInst == NULL)
	{
		hitInst = (struct Instance *)LinkedCollide_Radius(spiderInst, t, gGT->threadBuckets[ROBOT].thread, 0x9000);
		if (hitInst == NULL)
		{
			hitInst = (struct Instance *)LinkedCollide_Radius(spiderInst, t, gGT->threadBuckets[MINE].thread, 0x9000);
			if (hitInst != NULL)
			{
				hitInst->thread->funcThCollide(hitInst->thread);
			}

			return;
		}

		victim = (struct Driver *)hitInst->thread->object;
		RB_Hazard_HurtDriver(victim, 1, 0, 0);
		return;
	}

	victim = (struct Driver *)hitInst->thread->object;
	prevKartState = victim->kartState;
	if ((RB_Hazard_HurtDriver(victim, 1, 0, 0) != 0) && (prevKartState != KS_SPINNING))
	{
		OtherFX_Play(0x7b, 1);
		Voiceline_RequestPlay(1, data.characterIDs[victim->driverID], 0x10);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b9bc0-0x800b9bd4.

int RB_Spider_ThCollide(struct Thread *spiderThread, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	(void)spiderThread;
	(void)driverTh;
	(void)funcThCollide;

	return sps->Input1.modelID == DYNAMIC_PLAYER;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b9bd4-0x800b9dd8.

void RB_Spider_LInB(struct Instance *inst)
{
	struct Spider *spider;
	SVec3 rot;
	struct Thread *t;
	struct Instance *shadowInst;
	int spiderID;

	if (inst->thread != NULL)
		return;

	t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Spider), NONE, SMALL, SPIDER), RB_Spider_ThTick, "spider", 0);
	inst->thread = t;
	if (t == NULL)
		return;

	spider = t->object;
	t->funcThCollide = (void (*)(struct Thread *))RB_Spider_ThCollide;
	t->inst = inst;

	inst->scale.x = 0x1c00;
	inst->scale.y = 0x1c00;
	inst->scale.z = 0x1c00;
	inst->animIndex = 1;

	spiderID = inst->name[strlen(inst->name) - 1] - '0';
	spider->spiderID = spiderID;
	spider->boolNearRoof = 1;
	spider->animLoopCount = 0;

	if (spiderID == 3)
	{
		spider->delay = 91;
	}
	else if (spiderID == 2)
	{
		spider->delay = 69;
	}
	else
	{
		spider->delay = 0;
	}

	shadowInst = INSTANCE_Birth3D(sdata->gGT->modelPtr[DYNAMIC_SPIDERSHADOW], 0, t);

	spider->shadowInst = shadowInst;

	*(int *)&shadowInst->matrix.m[0][0] = *(int *)&inst->matrix.m[0][0];
	*(int *)&shadowInst->matrix.m[0][2] = *(int *)&inst->matrix.m[0][2];
	*(int *)&shadowInst->matrix.m[1][1] = *(int *)&inst->matrix.m[1][1];
	*(int *)&shadowInst->matrix.m[2][0] = *(int *)&inst->matrix.m[2][0];
	shadowInst->matrix.m[2][2] = inst->matrix.m[2][2];

	shadowInst->matrix.t[0] = inst->matrix.t[0];
	shadowInst->matrix.t[1] = inst->matrix.t[1] - 8;
	shadowInst->matrix.t[2] = inst->matrix.t[2];
	inst->matrix.t[1] += 0x4c0;

	shadowInst->scale.x = 0x2000;
	shadowInst->scale.y = 0x2000;
	shadowInst->scale.z = 0x2000;

	rot.x = 0;
	rot.y = 0x200;
	rot.z = 0;
	ConvertRotToMatrix(&shadowInst->matrix, &rot);
}
