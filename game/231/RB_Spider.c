#include <common.h>
#include <ctr_gte_transfer.h>

void RB_Spider_DrawWebs(struct Thread *t, struct PushBuffer *pb)
{
	int numPlyr;
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

	struct GameTracker *gGT;
	struct DB *db;
	struct Thread *current;
	multiCmdPacket *p;
	multiCmdPacket *nextPrim;

	s32 lineColor;
	u32 *ot;
	int depth;
	WebLine *scratchpad;
	WebLine *line;
	int numSpiders;
	int i, j;

	CTR_STATIC_ASSERT(sizeof(multiCmdPacket) == 0x18);
	CTR_STATIC_ASSERT(sizeof(WebLine) == 0xc);

	if (t == NULL)
	{
		return;
	}

	scratchpad = CTR_SCRATCHPAD_PTR(WebLine, 0);

	// Cache one vertical web per spider for all viewports to share.
	for (numSpiders = 0, line = scratchpad, current = t; current != NULL; numSpiders++)
	{
		struct Instance *inst = current->inst;
		struct InstDef *instDef = inst->instDef;
		s32 y = instDef->pos.y;
		s32 z = instDef->pos.z;
		// NOTE(aalhendi): GCC 2.8.1 needs X in a2 and the packed bottom word in v0
		// to preserve the retail register/operand choices. Native treats these as
		// ordinary locals; the packing arithmetic remains C.
		register u32 x CTR_PSX_REGISTER("$6") = (u16)instDef->pos.x;
		s32 bottom = inst->matrix.t[1];
		register u32 packedBottom CTR_PSX_REGISTER("$2");

		line->topXY = x | (((u32)y + 0x540) << 16);
		packedBottom = x | (((u32)bottom + 0x60) << 16);
		line->bottomXY = packedBottom;
		line->z = z;
		line++;

		current = current->siblingThread;
	}

	gGT = GAME_TRACKER;
	numPlyr = gGT->numPlyrCurrGame;
	db = gGT->backBuffer;

	p = db->primMem.cursor;
	// Reserve the worst case before projecting, even if some webs are out of range.
	nextPrim = p + (numSpiders * numPlyr);
	if (nextPrim >= (multiCmdPacket *)db->primMem.guardEnd)
	{
		return;
	}

	for (i = numPlyr - 1; i >= 0; i--)
	{
		CTR_GteSetRotMatrix(&pb->matrix_ViewProj);
		CTR_GteSetTransMatrix(&pb->matrix_ViewProj);

		line = scratchpad;

		for (j = numSpiders; j != 0; j--)
		{
			CTR_GteLoadLineV0V1(line);
			gte_rtpt_b();
			// NOTE(aalhendi): Retail writes XY before rejecting a web by its roof
			// depth. Rejected webs leave the primitive cursor at this same slot.
			CTR_GteStoreLineXY(&p->f2.x0);
			depth = CTR_GteReadDepthZ1();

			if ((u32)(depth - 1) < (0x1200 - 1))
			{
				p->tpage = 0xe1000a20;
				p->f2.tag = 0;

				lineColor = 0x3f;
				if (depth > 0xa00)
				{
					lineColor = ((0x1200 - depth) * lineColor) / 0x800;
				}

				CtrGpu_WriteColorCode(&p->f2.r0, lineColor | (lineColor << 8) | (lineColor << 16) | 0x42000000);

				depth = depth >> 6;
				if (depth > 0x3ff)
				{
					depth = 0x3ff;
				}

				ot = pb->ptrOT;
				ot += depth;

				// The OT already holds a 24-bit link; the packet adds five command words.
				p->tag = *ot | 0x5000000;
				*ot = CtrGpu_PrimToOTLink24(p);
				p++;
			}

			line++;
		}

		pb++;
	}

	GAME_TRACKER->backBuffer->primMem.cursor = p;
}

s16 spiderArr[] = {
    // first 13
    0x4c0, 0x439, 0x3A6, 0x306, 0x266, 0x1c8, 0x139, 0xb9, 0x59, 0x17, 0, 0, 0,

    // next 13
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


void RB_Spider_ThTick(struct Thread *t)
{
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

	// Wiggle five times at the roof, then descend.
	if (spider->isNearRoof != 0)
	{
		if (spider->animLoopCount < 5)
		{
			if (spiderInst->animFrame + 1 < INSTANCE_GetNumAnimFrames(spiderInst, 1))
			{
				goto advanceWiggle;
			}

			spiderInst->animFrame = 0;
			spider->animLoopCount++;
			if (spider->animLoopCount == 5)
			{
				spiderInst->animIndex = 0;
				spiderInst->animFrame = 0;
				PlaySound3D(0x7a, spiderInst);
			}
			goto checkCollision;
		}

		if (spiderInst->animFrame + 1 < INSTANCE_GetNumAnimFrames(spiderInst, 0))
		{
			spiderInst->animFrame++;
		}
		else
		{
			spider->animLoopCount = 0;
			spider->isNearRoof = 0;
			spiderInst->animIndex = 1;
		}
		goto updatePosScale;
	}

	// Wiggle five times on the ground, then play the descent in reverse.
	if (spider->animLoopCount < 5)
	{
		if (spiderInst->animFrame + 1 < INSTANCE_GetNumAnimFrames(spiderInst, 1))
		{
		advanceWiggle:
			spiderInst->animFrame++;
			goto checkCollision;
		}

		spiderInst->animFrame = 0;
		spider->animLoopCount++;
		if (spider->animLoopCount == 5)
		{
			spiderInst->animIndex = 0;
			spiderInst->animFrame = INSTANCE_GetNumAnimFrames(spiderInst, 0) - 1;
		}
		goto checkCollision;
	}

	if (spiderInst->animFrame - 1 > 0)
	{
		spiderInst->animFrame--;
		if (spiderInst->animFrame == 0xc)
		{
			PlaySound3D(0x79, spiderInst);
		}
	}
	else
	{
		spiderInst->animFrame = 0;
		spider->animLoopCount = 0;
		spider->isNearRoof = 1;
		spiderInst->animIndex = 1;
	}

updatePosScale:
	spiderInst->matrix.t[1] = spiderInst->instDef->pos.y + spiderArr[spiderInst->animFrame];
	if (spiderInst->animFrame < 0xb)
	{
		spider->shadowInst->scale.x = (spiderInst->animFrame * 0x1000) / 10 + 0x1800;
		spider->shadowInst->scale.z = (spiderInst->animFrame * 0x1000) / 10 + 0x1800;
	}

checkCollision:
	RB_Spider_CollideDrivers(t);
}

void RB_Spider_CollideDrivers(struct Thread *t)
{
	struct Instance *spiderInst = t->inst;
	struct Instance *hitInst = LinkedCollide_Radius(spiderInst, t, GAME_TRACKER->threadBuckets[PLAYER].thread, 0x9000);
	struct Driver *victim;
	b32 wasNotSpinning;
	if (hitInst != NULL)
	{
		victim = hitInst->thread->object;
		wasNotSpinning = victim->kartState != KS_SPINNING;
		if (RB_Hazard_HurtDriver(victim, 1, 0, 0) && wasNotSpinning)
		{
			OtherFX_Play(0x7b, 1);
			Voiceline_RequestPlayOwned(1, GAME_CHARACTER_IDS[victim->driverID], 0x10, victim);
		}
		return;
	}

	hitInst = LinkedCollide_Radius(spiderInst, t, GAME_TRACKER->threadBuckets[ROBOT].thread, 0x9000);
	if (hitInst != NULL)
	{
		RB_Hazard_HurtDriver(hitInst->thread->object, 1, 0, 0);
		return;
	}

	hitInst = LinkedCollide_Radius(spiderInst, t, GAME_TRACKER->threadBuckets[MINE].thread, 0x9000);
	if (hitInst != NULL)
	{
		((ThreadScratchCollideFunc)hitInst->thread->funcThCollide)(hitInst->thread, t, hitInst->thread->funcThCollide, NULL);
	}
}


int RB_Spider_ThCollide(struct Thread *spiderThread, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	(void)spiderThread;
	(void)driverTh;
	(void)funcThCollide;

	return sps->Input1.modelID == DYNAMIC_PLAYER;
}


void RB_Spider_LInB(struct Instance *inst)
{
	struct Spider *spider;
	SVec3 rot;
	struct Thread *t;

	t = inst->thread;
	if (t != NULL)
	{
		return;
	}

	inst->thread = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Spider), NONE, SMALL, SPIDER), RB_Spider_ThTick, "spider", 0);
	t = inst->thread;
	if (t == NULL)
	{
		return;
	}

	spider = t->object;
	t->funcThCollide = (void *)RB_Spider_ThCollide;
	t->inst = inst;

	inst->scale.x = 0x1c00;
	inst->scale.y = 0x1c00;
	inst->scale.z = 0x1c00;
	inst->animIndex = 1;

	spider->spiderID = inst->name[strlen(inst->name) - 1] - '0';
	spider->isNearRoof = 1;
	spider->animLoopCount = 0;

	if (spider->spiderID == 3)
	{
		spider->delay = 91;
	}
	else if (spider->spiderID == 2)
	{
		spider->delay = 69;
	}
	else
	{
		spider->delay = 0;
	}

	spider->shadowInst = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[DYNAMIC_SPIDERSHADOW], "spidershadow", t);

	// Position the shadow on the ground before raising the spider to the roof.
	memcpy(&spider->shadowInst->matrix, &inst->matrix, sizeof(inst->matrix));

	spider->shadowInst->matrix.t[0] = inst->matrix.t[0];
	spider->shadowInst->matrix.t[1] = CTR_MipsSubLo(inst->matrix.t[1], 8);
	spider->shadowInst->matrix.t[2] = inst->matrix.t[2];

	spider->shadowInst->scale.x = 0x2000;
	spider->shadowInst->scale.y = 0x2000;
	spider->shadowInst->scale.z = 0x2000;

	rot.x = 0;
	rot.z = 0;
	rot.y = 0x200;
	ConvertRotToMatrix(&spider->shadowInst->matrix, &rot);
	inst->matrix.t[1] = CTR_MipsAddLo(inst->matrix.t[1], 0x4c0);
}
