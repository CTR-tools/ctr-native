#include <common.h>

static struct InstDrawPerPlayer *RB_Blowup_GetIDPP(struct Instance *inst, int playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)inst + sizeof(struct Instance) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static void RB_Blowup_CopyDrawState(struct Instance *dstInst, struct Instance *srcInst, int playerIndex)
{
	struct InstDrawPerPlayer *src = RB_Blowup_GetIDPP(srcInst, playerIndex);
	struct InstDrawPerPlayer *dst = RB_Blowup_GetIDPP(dstInst, playerIndex);

	dst->instFlags &= src->instFlags | ~DRAW_SUCCESSFUL;
	dst->otRangeNormal = src->otRangeNormal;
	dst->depthOffset[0] = src->depthOffset[0];
	dst->depthOffset[1] = src->depthOffset[1];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b1714-0x800b17f0
void RB_Blowup_ProcessBucket(struct Thread *thread)
{
	struct GameTracker *gGT = sdata->gGT;

	for (; thread != NULL; thread = thread->siblingThread)
	{
		u32 *blowup = thread->object;

		for (int i = 0; i < gGT->numPlyrCurrGame; i++)
		{
			struct Instance *shockwaveInst = (struct Instance *)(uintptr_t)blowup[0];
			struct Instance *explosionInst = (struct Instance *)(uintptr_t)blowup[1];

			if (shockwaveInst == NULL || explosionInst == NULL)
			{
				continue;
			}

			RB_Blowup_CopyDrawState(explosionInst, shockwaveInst, i);
		}
	}
}

static void RB_Blowup_UpdateSlot(int *slot)
{
	struct Instance *inst;
	int nextFrame;

	inst = (struct Instance *)*slot;
	if (inst == NULL)
	{
		return;
	}

	nextFrame = inst->animFrame + 1;
	if (nextFrame < INSTANCE_GetNumAnimFrames(inst, 0))
	{
		inst->animFrame++;
		return;
	}

	INSTANCE_Death(inst);
	*slot = 0;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b17f0-0x800b18f8.
void RB_Blowup_ThTick(struct Thread *t)
{
	int *blowup;
	blowup = t->object;

	RB_Blowup_UpdateSlot(&blowup[1]);
	RB_Blowup_UpdateSlot(&blowup[0]);

	if ((blowup[1] == 0) && (blowup[0] == 0))
	{
		t->flags |= THREAD_FLAG_DEAD;
	}

	ThTick_FastRET(t);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b18f8-0x800b1bd8.
void RB_Blowup_Init(struct Instance *weaponInst)
{
	struct Thread *explosionTh;
	struct Instance *explosionInst;
	struct Instance *shockwaveInst;
	struct ModelHeader *headers;
	struct GameTracker *gGT = sdata->gGT;
	u32 color;
	int *blowup;

	// initialize thread for blowup
	explosionInst = INSTANCE_BirthWithThread(0x26, 0, SMALL, BLOWUP, RB_Blowup_ThTick, 0xc, 0);

	explosionInst->flags |= (VISIBLE_DURING_GAMEPLAY | DRAW_BILLBOARD);

	explosionTh = explosionInst->thread;
	blowup = explosionTh->object;

	// set explosion instance
	blowup[1] = (s32)(uintptr_t)explosionInst;

	// copy position and rotation from weapon to explosion
	*(int *)&explosionInst->matrix.m[0][0] = *(int *)&weaponInst->matrix.m[0][0];
	*(int *)&explosionInst->matrix.m[0][2] = *(int *)&weaponInst->matrix.m[0][2];
	*(int *)&explosionInst->matrix.m[1][1] = *(int *)&weaponInst->matrix.m[1][1];
	*(int *)&explosionInst->matrix.m[2][0] = *(int *)&weaponInst->matrix.m[2][0];
	explosionInst->matrix.m[2][2] = weaponInst->matrix.m[2][2];
	explosionInst->matrix.t[0] = weaponInst->matrix.t[0];
	explosionInst->matrix.t[1] = weaponInst->matrix.t[1];
	explosionInst->matrix.t[2] = weaponInst->matrix.t[2];

	// green
	color = 0x1eac000;

	// green shockwave
	int modelID = STATIC_SHOCKWAVE_GREEN;

	// if instance -> model -> modelID == tnt
	if (weaponInst->model->id == STATIC_CRATE_TNT)
	{
		// red
		color = 0xad10000;

		// red shockwave
		modelID = STATIC_SHOCKWAVE_RED;
	}

	// set color
	explosionInst->colorRGBA = color;

	// set scale
	explosionInst->alphaScale = 0x1000;

	// ======== Next Instance ==========

	shockwaveInst = INSTANCE_Birth3D(gGT->modelPtr[modelID], 0, explosionTh);

	// set shockwave instance
	blowup[0] = (s32)(uintptr_t)shockwaveInst;

	shockwaveInst->flags |= PIXEL_LOD;

	*(int *)&shockwaveInst->matrix.m[0][0] = 0x1000;
	*(int *)&shockwaveInst->matrix.m[0][2] = 0;
	*(int *)&shockwaveInst->matrix.m[1][1] = 0x1000;
	*(int *)&shockwaveInst->matrix.m[2][0] = 0;
	shockwaveInst->matrix.m[2][2] = 0x1000;
	shockwaveInst->matrix.t[0] = weaponInst->matrix.t[0];
	shockwaveInst->matrix.t[1] = weaponInst->matrix.t[1];
	shockwaveInst->matrix.t[2] = weaponInst->matrix.t[2];

	headers = shockwaveInst->model->headers;

	// set flag to always point to camera
	headers[0].flags |= 2;
	headers[1].flags |= 2;

	// ======== End Of Instance ==========

	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);

	// put weapon position on scratchpad
	sps->Input1.pos.x = weaponInst->matrix.t[0];
	sps->Input1.pos.y = weaponInst->matrix.t[1];
	sps->Input1.pos.z = weaponInst->matrix.t[2];

	if (IS_BOSS_RACE(gGT->gameMode1))
	{
		// hitRadius and hitRadiusSquared
		sps->Input1.hitRadius = 0x100;
		sps->Input1.hitRadiusSquared = 0x10000;
	}

	// if you're not in boss mode
	else
	{
		// hitRadius and hitRadiusSquared
		sps->Input1.hitRadius = 0x140;
		sps->Input1.hitRadiusSquared = 0x19000;
	}

	sps->Input1.modelID = weaponInst->model->id;

	sps->Union.ThBuckColl.thread = weaponInst->thread;
	sps->Union.ThBuckColl.funcCallback = RB_Burst_CollThBucket;

	PROC_StartSearch_Self(sps);

	PROC_CollideHitboxWithBucket(gGT->threadBuckets[ROBOT].thread, sps, 0);
	PROC_CollideHitboxWithBucket(gGT->threadBuckets[MINE].thread, sps, 0);

	// Nitro explosion has smaller radius than TNT explosion
	if (weaponInst->model->id != STATIC_CRATE_TNT)
	{
		// hitRadius and hitRadiusSquared
		sps->Input1.hitRadius = 0x80;
		sps->Input1.hitRadiusSquared = 0x4000;
	}

	// check collision with player threads
	PROC_CollideHitboxWithBucket(gGT->threadBuckets[PLAYER].thread, sps, 0);

	sps->Union.ThBuckColl.funcCallback = RB_Burst_CollLevInst;
	return;
}
