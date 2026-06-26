#include <common.h>

static struct InstDrawPerPlayer *RB_Burst_GetIDPP(struct Instance *inst, int playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)inst + sizeof(struct Instance) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static void RB_Burst_CopyDrawState(struct Instance *dstInst, struct Instance *srcInst, int playerIndex)
{
	struct InstDrawPerPlayer *src = RB_Burst_GetIDPP(srcInst, playerIndex);
	struct InstDrawPerPlayer *dst = RB_Burst_GetIDPP(dstInst, playerIndex);

	dst->instFlags &= src->instFlags | ~DRAW_SUCCESSFUL;
	dst->otRangeNormal = src->otRangeNormal;
	dst->depthOffset[0] = src->depthOffset[0];
	dst->depthOffset[1] = src->depthOffset[1];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b1bd8-0x800b1d2c
void RB_Burst_ProcessBucket(struct Thread *thread)
{
	struct GameTracker *gGT = sdata->gGT;

	for (; thread != NULL; thread = thread->siblingThread)
	{
		u32 *burst = thread->object;

		for (int i = 0; i < gGT->numPlyrCurrGame; i++)
		{
			struct Instance *shockwaveInst = (struct Instance *)(uintptr_t)burst[0];
			struct Instance *burstInst = (struct Instance *)(uintptr_t)burst[1];
			struct Instance *warpedBurstInst = (struct Instance *)(uintptr_t)burst[2];

			if (burstInst == NULL)
			{
				continue;
			}

			if (shockwaveInst != NULL)
			{
				RB_Burst_CopyDrawState(shockwaveInst, burstInst, i);
			}

			if (warpedBurstInst != NULL)
			{
				RB_Burst_CopyDrawState(warpedBurstInst, burstInst, i);
			}
		}
	}
}

static void RB_Burst_UpdateSlot(int *slot)
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

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b1d2c-0x800b1e90.
void RB_Burst_ThTick(struct Thread *t)
{
	int *burst;
	burst = t->object;

	RB_Burst_UpdateSlot(&burst[1]);
	RB_Burst_UpdateSlot(&burst[2]);
	RB_Burst_UpdateSlot(&burst[0]);

	if ((burst[1] == 0) && (burst[2] == 0))
	{
		t->flags |= THREAD_FLAG_DEAD;
	}
}

typedef int (*BurstThreadCollideFunc)(struct Thread *, struct Thread *, void *, int);

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b1e90-0x800b20a4.
void RB_Burst_CollThBucket(struct ScratchpadStruct *sps, void *hitObject)
{
	struct Thread *t = hitObject;
	struct GameTracker *gGT;
	struct TrackerWeapon *tw;
	s16 model;
	u16 reason;
	struct Thread *weaponTh;

	gGT = sdata->gGT;

	weaponTh = sps->Union.ThBuckColl.thread;
	tw = weaponTh->object;
	void *weaponObj = weaponTh->object;

	struct Driver *attacker;
	struct Driver *victim = t->object;
	model = t->modelIndex;

	b32 hitDriver = (model == DYNAMIC_PLAYER) || (model == DYNAMIC_ROBOT_CAR);
	if (hitDriver)
	{
		model = weaponTh->modelIndex;

		b32 weaponIsMineHazard = (model == PU_EXPLOSIVE_CRATE) || (model == STATIC_BEAKER_RED) || (model == STATIC_BEAKER_GREEN) || (model == STATIC_CRATE_TNT);
		if (weaponIsMineHazard)
		{
			attacker = ((struct MineWeapon *)weaponObj)->instParent->thread->object;

			// blasted anyone?
			RB_Hazard_HurtDriver(victim, 2, attacker, 2);
		}
		else
		{
			// bomb
			reason = 1;

			// missile
			if (model == DYNAMIC_ROCKET)
			{
				// missile
				reason = 3;
			}

			attacker = ((struct TrackerWeapon *)weaponObj)->instParent->thread->object;

			// blasted anyone?
			RB_Hazard_HurtDriver(victim, 2, attacker, reason);

			if (attacker->longestShot < tw->timeAlive)
			{
				attacker->longestShot = tw->timeAlive;
			}
		}

		// if this driver is not an AI
		if ((victim->actionsFlagSet & ACTION_BOT) == 0)
		{
			struct PushBuffer *pb = &gGT->pushBuffer[victim->driverID];

			pb->fadeFromBlack_currentValue = 0x1fff;
			pb->fadeFromBlack_desiredResult = 0x1000;
			pb->fade_step = 0xff78;
		}

		// icon damage timer, draw icon as red
		victim->damageColorTimer = 0x1e;

		// get modelID from thread
		model = t->modelIndex;
	}

	// not DYNAMIC_ROCKET
	if (model != DYNAMIC_ROCKET)
	{
		if (model < DYNAMIC_BIGROCKET)
		{
			// not nitro and not STATIC_CRATE_TNT
			if ((model != PU_EXPLOSIVE_CRATE) && (model != STATIC_CRATE_TNT))
			{
				return;
			}
		}
		else
		{
			// return if anything that isn't beakers
			if ((STATIC_BEAKER_GREEN < model) || (model < STATIC_BEAKER_RED))
			{
				return;
			}
		}
	}

	// if function pointer is valid
	if (t->funcThCollide != NULL)
	{
		((BurstThreadCollideFunc)t->funcThCollide)(t, weaponTh, t->funcThCollide, 3);
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b20a4-0x800b2154.
void RB_Burst_CollLevInst(struct ScratchpadStruct *sps, void *hitObject)
{
	struct BSP *bspHitbox = hitObject;
	s16 model;
	struct Instance *inst;
	struct InstDef *instdef;
	struct MetaDataMODEL *meta;

	instdef = bspHitbox->data.hitbox.instDef;
	if (instdef == NULL)
	{
		return;
	}

	inst = instdef->ptrInstance;
	if (inst == NULL)
	{
		return;
	}

	model = instdef->modelID;
	if (model < PU_FRUIT_CRATE)
	{
		return;
	}

	// check 7 and 8,
	// 7: PU_FRUIT_CRATE
	// 8: PU_RANDOM_CRATE (weapon box)
	if (model < PU_TIME_CRATE_1)
	{
		meta = COLL_LevModelMeta(model);
		if (meta == NULL)
		{
			return;
		}

		if (meta->LInC == NULL)
		{
			return;
		}

		meta->LInC(inst, sps->Union.ThBuckColl.thread, sps);
		return;
	}

	if (model == STATIC_TEETH)
	{
		RB_Teeth_OpenDoor(inst);
	}

	return;
}

static char s_burst_explosion1[] = "explosion1";
static char s_burst_explosion2[] = "explosion2";
static char s_burst_shockwave1[] = "shockwave1";

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b2154-0x800b25b8.
void RB_Burst_Init(struct Instance *weaponInst)
{
	struct GameTracker *gGT = sdata->gGT;
	struct ModelHeader *headers;
	struct Instance *currInst;
	struct Thread *t;
	int *burst;

	// initialize thread for burst
	currInst = INSTANCE_BirthWithThread(0x2b, s_burst_explosion1, SMALL, BURST, RB_Burst_ThTick, 0xc, 0);

	// get thread from instance
	t = currInst->thread;

	// get object from thread
	burst = t->object;

	// ====== First Instance =========

	burst[1] = (int)currInst;
	currInst->depthBiasNormal += -2;

	// set rotation to identity matrix
	*(int *)&currInst->matrix.m[0][0] = 0x1000;
	*(int *)&currInst->matrix.m[0][2] = 0;
	*(int *)&currInst->matrix.m[1][1] = 0x1000;
	*(int *)&currInst->matrix.m[2][0] = 0;
	currInst->matrix.m[2][2] = 0x1000;

	// set flag to always point to camera
	headers = currInst->model->headers;
	headers[0].flags |= 2;

	// ======== Next one ===========

	currInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_WARPEDBURST], s_burst_explosion2, t);

	burst[2] = (int)currInst;
	currInst->depthBiasNormal += -2;

	currInst->flags |= VISIBLE_DURING_GAMEPLAY;

	// rotate 90 degrees
	currInst->matrix.m[0][0] = 0;
	currInst->matrix.m[0][1] = 0xf000;
	currInst->matrix.m[0][2] = 0;
	currInst->matrix.m[1][0] = 0x1000;
	*(int *)&currInst->matrix.m[1][1] = 0;
	*(int *)&currInst->matrix.m[2][0] = 0;
	currInst->matrix.m[2][2] = 0x1000;

	// set flag to always point to camera
	headers = currInst->model->headers;
	headers[0].flags |= 2;

	// ======= Next One ===========

	currInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_SHOCKWAVE_RED], s_burst_shockwave1, t);

	burst[0] = (int)currInst;
	currInst->depthBiasNormal += -2;

	// instance flags
	currInst->flags |= (VISIBLE_DURING_GAMEPLAY | DRAW_BILLBOARD);

	// set flag to always point to camera
	headers = currInst->model->headers;
	headers[0].flags |= 2;
	headers[1].flags |= 2;

	// ======= End of Instance =========

	for (int i = 0; /*i < 3*/; i++)
	{
		currInst = (struct Instance *)burst[i];

		currInst->matrix.t[0] = weaponInst->matrix.t[0];
		currInst->matrix.t[1] = weaponInst->matrix.t[1] + -0x30;
		currInst->matrix.t[2] = weaponInst->matrix.t[2];

		// if more than two screens
		if (2 < gGT->numPlyrCurrGame)
		{
			// set scale (x, y, z)
			currInst->scale.x = currInst->scale.x >> 1;
			currInst->scale.y = currInst->scale.y >> 1;
			currInst->scale.z = currInst->scale.z >> 1;
		}

		// identity matrix (z)
		*(int *)&currInst->matrix.m[2][0] = 0;
		currInst->matrix.m[2][2] = 0x1000;

		if (i == 2)
		{
			break;
		}

		// identity matrix (x, y)
		*(int *)&currInst->matrix.m[0][0] = 0x1000;
		*(int *)&currInst->matrix.m[0][2] = 0;
		*(int *)&currInst->matrix.m[1][1] = 0x1000;
	}

	// currInst is burst[2]

	// rotate 90 degrees (X -> -Y)
	currInst->matrix.m[0][0] = 0;
	currInst->matrix.m[0][1] = 0xf000;
	currInst->matrix.m[0][2] = 0;

	// rotate 90 degrees (Y -> X)
	currInst->matrix.m[1][0] = 0x1000;
	*(int *)&currInst->matrix.m[1][1] = 0;

	// ========= Collisions ===========

	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);

	// put weapon position on scratchpad
	sps->Input1.pos.x = weaponInst->matrix.t[0];
	sps->Input1.pos.y = weaponInst->matrix.t[1];
	sps->Input1.pos.z = weaponInst->matrix.t[2];

	struct TrackerWeapon *tw = weaponInst->thread->object;

	int modelID = weaponInst->model->id;

	// missile
	if (modelID == DYNAMIC_ROCKET)
	{
		// hitRadius and hitRadiusSquared
		sps->Input1.hitRadius = 0x80;
		sps->Input1.hitRadiusSquared = 0x4000;
	}
	else
	{
		if ((tw->flags & 1) == 0)
		{
			// hitRadius and hitRadiusSquared
			sps->Input1.hitRadius = 0x140;
			sps->Input1.hitRadiusSquared = 0x19000;
		}
		else
		{
			// hitRadius and hitRadiusSquared
			sps->Input1.hitRadius = 0x200;
			sps->Input1.hitRadiusSquared = 0x40000;
		}
	}

	sps->Input1.modelID = modelID;

	sps->Union.ThBuckColl.thread = weaponInst->thread;
	sps->Union.ThBuckColl.funcCallback = RB_Burst_CollThBucket;

	struct Thread *driverTh = tw->driverParent->instSelf->thread;

	// check collision with all Player thread
	PROC_CollideHitboxWithBucket(gGT->threadBuckets[PLAYER].thread, sps, driverTh);

	// check collision with all Robotcar thread
	PROC_CollideHitboxWithBucket(gGT->threadBuckets[ROBOT].thread, sps, driverTh);

	// check collision with all Mine thread
	PROC_CollideHitboxWithBucket(gGT->threadBuckets[MINE].thread, sps, 0);

	// check collision with all Tracking thread
	PROC_CollideHitboxWithBucket(gGT->threadBuckets[TRACKING].thread, sps, 0);

	sps->Union.ThBuckColl.funcCallback = RB_Burst_CollLevInst;

	PROC_StartSearch_Self(sps);
	return;
}

static struct InstDrawPerPlayer *RB_Burst_DrawAll_GetIDPP(struct Instance *inst, int playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)inst + sizeof(struct Instance) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static void RB_Burst_DrawAll_SetPushBuffer(struct Instance *inst, int playerIndex, struct PushBuffer *pb)
{
	if (inst != NULL)
	{
		RB_Burst_DrawAll_GetIDPP(inst, playerIndex)->pushBuffer = pb;
	}
}

static struct Instance *RB_Burst_DrawAll_GetSlot(u32 *burst, int index)
{
	// NOTE(aalhendi): burst thread object is retail-width instance slots.
	return (struct Instance *)(uintptr_t)burst[index];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b25b8-0x800b28c0.
void RB_Burst_DrawAll(struct GameTracker *gGT)
{
	struct Thread *selectedThread[4];
	int selectedFrame[4];
	int playerIndex;
	struct Thread *thread;

	for (playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		struct PushBuffer *pb = &gGT->pushBuffer[playerIndex];

		selectedFrame[playerIndex] = 0x10000;
		selectedThread[playerIndex] = NULL;

		SetRotMatrix(&pb->matrix_ViewProj);
		SetTransMatrix(&pb->matrix_ViewProj);

		for (thread = gGT->threadBuckets[BURST].thread; thread != NULL; thread = thread->siblingThread)
		{
			u32 *burst = thread->object;
			struct Instance *burstInst = RB_Burst_DrawAll_GetSlot(burst, 1);
			SVECTOR pos;
			VECTOR transformed;
			int absX;
			int absY;
			int absZ;

#ifdef CTR_NATIVE
			// NOTE(aalhendi): Retail can survive the one-frame null low-RAM read.
			if (burstInst == NULL)
			{
				continue;
			}
#endif

			pos.vx = burstInst->matrix.t[0];
			pos.vy = burstInst->matrix.t[1];
			pos.vz = burstInst->matrix.t[2];

			CTR_GteLoadSV0(&pos);
			gte_mvmva(0, 0, 0, 3, 0);
			CTR_GteStoreMAC(&transformed.vx);

			absX = transformed.vx;
			if (absX < 0)
			{
				absX = -absX;
			}

			absY = transformed.vy;
			if (absY < 0)
			{
				absY = -absY;
			}

			absZ = transformed.vz;
			if (absZ < 0)
			{
				absZ = -absZ;
			}

			if ((absX < 0x100) && (absY < 0x100) && (absZ < (pb->distanceToScreen_PREV << 1)))
			{
				if (burstInst->animFrame < selectedFrame[playerIndex])
				{
					selectedFrame[playerIndex] = burstInst->animFrame;
					selectedThread[playerIndex] = thread;
				}

				if (burstInst->animFrame == 1)
				{
					pb->fadeFromBlack_desiredResult = 0x1000;
					pb->fade_step = -0x88;
					pb->fadeFromBlack_currentValue = 0x1fff - ((absX + absY) << 3);
				}
			}
		}
	}

	for (playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		struct PushBuffer *pb = &gGT->pushBuffer[playerIndex];

		for (thread = gGT->threadBuckets[BURST].thread; thread != NULL; thread = thread->siblingThread)
		{
			u32 *burst = thread->object;
			struct PushBuffer *targetPB = pb;

			if ((selectedThread[playerIndex] != NULL) && (selectedThread[playerIndex] != thread))
			{
				targetPB = NULL;
			}

			RB_Burst_DrawAll_SetPushBuffer(RB_Burst_DrawAll_GetSlot(burst, 1), playerIndex, targetPB);
			RB_Burst_DrawAll_SetPushBuffer(RB_Burst_DrawAll_GetSlot(burst, 2), playerIndex, targetPB);
			RB_Burst_DrawAll_SetPushBuffer(RB_Burst_DrawAll_GetSlot(burst, 0), playerIndex, targetPB);
		}
	}
}
