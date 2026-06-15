#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041dc0-0x80041dfc.
void PROC_DestroyTracker(struct Thread *t)
{
	struct GameTracker *gGT = sdata->gGT;

	if (gGT->numMissiles > 0)
		gGT->numMissiles--;

	PROC_DestroyInstance(t);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041dfc-0x80041e20.
void PROC_DestroyInstance(struct Thread *t)
{
	INSTANCE_Death(t->inst);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041e20-0x80041e9c.
void PROC_DestroyObject(void *object, int threadFlags)
{
	struct JitPool *myPool;

	if (object == NULL)
		return;

	if ((threadFlags & 0x300) == 0x100)
		myPool = &sdata->gGT->JitPools.largeStack;
	else if ((threadFlags & 0x300) == 0x200)
		myPool = &sdata->gGT->JitPools.mediumStack;
	else
		myPool = &sdata->gGT->JitPools.smallStack;

	// in allocation, "next" and "prev" are abstracted
	// with obj+=8, so not all structs need "next" and "prev",
	// now subtract 8 bytes to access those two pointers
	object = (void *)((u8 *)object - 8);

	// add object back to free list
	LIST_AddFront(&myPool->free, (struct Item *)object);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041e9c-0x80041f04.
void PROC_DestroySelf(struct Thread *t)
{
	// thread must exist
	if (t == 0)
		return;

	// this is usuallly PROC_DestroyInstance
	if (t->funcThDestroy != 0)
		t->funcThDestroy(t);

	// used by RB_Follower
	t->timesDestroyed++;

	// destroy object attached,
	// guaranteed all threads have one
	PROC_DestroyObject(t->object, t->flags);

	// recycle thread
	LIST_AddFront(&sdata->gGT->JitPools.thread.free, (struct Item *)t);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041f04-0x80041f58.
void PROC_DestroyBloodline(struct Thread *t)
{
	while (t != 0)
	{
		struct Thread *siblingThread;

		// recursively find all children
		if (t->childThread != 0)
			PROC_DestroyBloodline(t->childThread);

		siblingThread = t->siblingThread;
		PROC_DestroySelf(t);
		t = siblingThread;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041f58-0x80041ff4.
void PROC_CheckBloodlineForDead(struct Thread **replaceSelf, struct Thread *th)
{
	while (th != 0)
	{
		struct Thread *siblingThread = th->siblingThread;

		// if this thread is alive
		if ((th->flags & 0x800) == 0)
		{
			// [wow this sounds dark]
			// check child's bloodline, and if child is dead, then
			// "childThread" inside "th" will be replaced with the
			// child's sibling, or nullptr if all children are dead

			// recursively find all children
			if (th->childThread != 0)
				PROC_CheckBloodlineForDead(&th->childThread, th->childThread);

			// current thread is alive, doesn't need to be overwritten,
			// next check sibling, so sibling will be replaced by the
			// sibling's sibling, if the sibling's sibling is dead
			replaceSelf = &th->siblingThread;
		}

		// if this thread needs to die
		else
		{
			// recursively find all children
			if (th->childThread != 0)
				PROC_DestroyBloodline(th->childThread);

			PROC_DestroySelf(th);

			// replace thread with pointer to it's own sibling
			*replaceSelf = siblingThread;

			// dont overwrite replaceSelf like in previous
			// "if" block, cause the next dead sibling can
			// still take the place in replaceSelf
		}

		th = siblingThread;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041ff4-0x8004205c.
void PROC_CheckAllForDead()
{
	int i;

	struct ThreadBucket *tb = &sdata->gGT->threadBuckets[0];

	for (i = 0; i < NUM_BUCKETS; i++)
	{
		PROC_CheckBloodlineForDead(&tb[i].thread, tb[i].thread);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004205c-0x8004228c.
struct Thread *PROC_BirthWithObject(int flags, void *funcThTick, const char *name, struct Thread *relativeTh)
{
	int bucketID;
	struct JitPool *stackPool;
	void *stackObj;
	struct Thread *th;
	struct GameTracker *gGT;

	gGT = sdata->gGT;

	// determine bucketID from relativeTh or flags
	if (relativeTh != 0)
		bucketID = relativeTh->flags & 0xff;
	else
		bucketID = flags & 0xff;

	// select stack pool based on flags & 0x300
	switch (flags & 0x300)
	{
	case 0x100: // largeStack
		stackPool = &gGT->JitPools.largeStack;
		break;
	case 0x200: // mediumStack
		stackPool = &gGT->JitPools.mediumStack;
		break;
	default: // 0x300 = smallStack
		stackPool = &gGT->JitPools.smallStack;
		break;
	}

	// allocate stack object FIRST
	stackObj = LIST_RemoveFront(&stackPool->free);

	// validate bucket
	if (bucketID >= NUM_BUCKETS)
	{
		if (stackObj != 0)
			PROC_DestroyObject((void *)((u32)stackObj + 8), flags);
		return 0;
	}

	// validate size fits in pool
	if ((u32)(flags >> 0x10) >= (stackPool->itemSize - 8))
	{
		if (stackObj != 0)
			PROC_DestroyObject((void *)((u32)stackObj + 8), flags);
		return 0;
	}

	// check stack object allocated
	if (stackObj == 0)
		return 0;

	// allocate thread SECOND
	th = (struct Thread *)LIST_RemoveFront(&gGT->JitPools.thread.free);

	// check thread allocated
	if (th == 0)
	{
		PROC_DestroyObject((void *)((u32)stackObj + 8), flags);
		return 0;
	}

	// initialize thread fields
	th->flags = flags;
	th->cooldownFrameCount = 0;
	th->funcThCollide = 0;
	th->funcThDestroy = 0;
	th->inst = 0;

	// handle relative thread linking
	if (relativeTh == 0)
	{
		struct ThreadBucket *tb = &gGT->threadBuckets[bucketID];

		th->siblingThread = tb->thread;
		tb->thread = th;
		th->parentThread = 0;
		th->childThread = 0;
	}
	else if (flags & SELF_SIBLING)
	{
		th->siblingThread = relativeTh->siblingThread;
		relativeTh->siblingThread = th;
		th->childThread = 0;
		th->parentThread = relativeTh->parentThread;
	}
	else if (flags & CHILD_BETWEEN)
	{
		th->childThread = relativeTh->childThread;
		relativeTh->childThread = th;
		th->parentThread = relativeTh;
		th->siblingThread = 0;
	}
	else
	{
		th->childThread = 0;
		th->siblingThread = relativeTh->childThread;
		relativeTh->childThread = th;
		th->parentThread = relativeTh;
	}

	// set remaining fields AFTER linking (ASM order)
	th->funcThTick = funcThTick;
	th->name = name;
	th->object = (void *)(((u32)stackObj) + 8);

	return th;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004228c-0x80042348.
void PROC_CollidePointWithSelf(struct Thread *th, struct BucketSearchParams *buf)
{
	struct Instance *inst;
	int distX;
	int distY;
	int distZ;
	int dist;

	// if collision disabled, or thread is dead, quit
	if ((th->flags & 0x1800) != 0)
		return;

	inst = th->inst;

	// Do not try to optimize this with loops,
	// it will not compile to less assembly,
	// 180 bytes is as low as this will go

	distX = (int)buf->pos.x - (int)inst->matrix.t[0];
	distY = (int)buf->pos.y - (int)inst->matrix.t[1];
	distZ = (int)buf->pos.z - (int)inst->matrix.t[2];

	if (distX * distX >= 0x10000000)
		return;
	if (distY * distY >= 0x10000000)
		return;
	if (distZ * distZ >= 0x10000000)
		return;

	dist = distX * distX + distY * distY + distZ * distZ;

	// if outside hit radius
	if (dist >= buf->bestDistSq)
		return;

	// return distance to center
	buf->bestDistSq = dist;

	// save the thread collided with
	buf->th = th;

	CTR_SET_VEC3(buf->dist.v, (s16)distX, (s16)distY, (s16)distZ);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80042348-0x80042394.
void PROC_CollidePointWithBucket(struct Thread *th, struct BucketSearchParams *buf)
{
	// only used with drivers colliding
	// with other drivers, disabled online
	while (th != 0)
	{
		PROC_CollidePointWithSelf(th, buf);

		// next
		th = th->siblingThread;
	}
}


// search starts with driver thread's child
// searches for turbo model
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80042394-0x800423fc.
struct Thread *PROC_SearchForModel(struct Thread *th, s16 modelID)
{
	while (th != 0)
	{
		// if found, quit
		if (th->modelIndex == modelID)
			return th;

		// check children recursively, quit if found
		struct Thread *other = PROC_SearchForModel(th->childThread, modelID);
		if (other != 0)
			return other;

		th = th->siblingThread;
	}

	return th;
}


static s32 PROC_PerBspLeaf_MipsSquare(s32 value)
{
	return (s32)(u32)((s64)value * (s64)value);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800423fc-0x80042544
void PROC_PerBspLeaf_CheckInstances(struct BSP *bspLeaf, struct ScratchpadStruct *sps)
{
	s32 distX;
	s32 distY;
	s32 distZ;
	s32 dist;
	struct BSP *bspHitbox;
	struct InstDef *instDef;
	CollThBuckCallback callback;

	bspHitbox = bspLeaf->data.leaf.bspHitboxArray;
	if (bspHitbox == NULL)
		return;

	if (*(int *)bspHitbox == 0)
		return;

	for (/**/; *(int *)bspHitbox != 0; bspHitbox++)
	{
		if ((bspHitbox->flag & BSP_HITBOX_COLLIDABLE) == 0)
			continue;

		instDef = bspHitbox->data.hitbox.instDef;
		if ((instDef != NULL) && ((instDef->ptrInstance->flags & 0xf) == 0))
			continue;

		distX = (int)sps->Input1.pos.x - (int)bspHitbox->data.hitbox.center.x;
		distY = (int)sps->Input1.pos.y - (int)bspHitbox->data.hitbox.center.y;
		distZ = (int)sps->Input1.pos.z - (int)bspHitbox->data.hitbox.center.z;

		dist = PROC_PerBspLeaf_MipsSquare(distX);
		if (dist > 0x0fffffff)
			continue;

		s32 distYSquared = PROC_PerBspLeaf_MipsSquare(distY);
		dist += distYSquared;
		if (distYSquared > 0x0fffffff)
			continue;

		s32 distZSquared = PROC_PerBspLeaf_MipsSquare(distZ);
		dist += distZSquared;
		if (distZSquared > 0x0fffffff)
			continue;

		if (dist >= sps->Input1.hitRadiusSquared)
			continue;

		CTR_SET_VEC3(sps->Union.ThBuckColl.centerDelta.v, (s16)distX, (s16)distY, (s16)distZ);

		callback = sps->Union.ThBuckColl.funcCallback;
		callback(sps, bspHitbox);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80042544-0x800425d4
void PROC_StartSearch_Self(struct ScratchpadStruct *sps)
{
	s16 hitRadius;
	struct GameTracker *gGT;

	hitRadius = sps->Input1.hitRadius;

	sps->Union.ThBuckColl.bbox.min.x = (s16)((u16)sps->Input1.pos.x - (u16)hitRadius);
	sps->Union.ThBuckColl.bbox.min.y = (s16)((u16)sps->Input1.pos.y - (u16)hitRadius);
	sps->Union.ThBuckColl.bbox.min.z = (s16)((u16)sps->Input1.pos.z - (u16)hitRadius);

	sps->Union.ThBuckColl.bbox.max.x = (s16)((u16)sps->Input1.pos.x + (u16)hitRadius);
	sps->Union.ThBuckColl.bbox.max.y = (s16)((u16)sps->Input1.pos.y + (u16)hitRadius);
	sps->Union.ThBuckColl.bbox.max.z = (s16)((u16)sps->Input1.pos.z + (u16)hitRadius);

	gGT = sdata->gGT;

	COLL_SearchBSP_CallbackPARAM(gGT->level1->ptr_mesh_info->bspRoot, &sps->Union.ThBuckColl.bbox, PROC_PerBspLeaf_CheckInstances, sps);
}


static s32 PROC_CollideHitbox_MipsSquare(s32 value)
{
	return (s32)(u32)((s64)value * (s64)value);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800425d4-0x800426f8
void PROC_CollideHitboxWithBucket(struct Thread *collThread, struct ScratchpadStruct *sps, struct Thread *ignoredThread)
{
	s32 distX;
	s32 distY;
	s32 distZ;
	s32 dist;
	struct Instance *inst;
	CollThBuckCallback callback;

	for (/**/; collThread != NULL; collThread = collThread->siblingThread)
	{
		if (collThread->childThread != NULL)
			PROC_CollideHitboxWithBucket(collThread->childThread, sps, ignoredThread);

		if (collThread == ignoredThread)
			continue;

		if ((collThread->flags & 0x1800) != 0)
			continue;

		inst = collThread->inst;

		distX = (int)sps->Input1.pos.x - inst->matrix.t[0];
		distY = (int)sps->Input1.pos.y - inst->matrix.t[1];
		distZ = (int)sps->Input1.pos.z - inst->matrix.t[2];

		dist = PROC_CollideHitbox_MipsSquare(distX);
		if (dist > 0x0fffffff)
			continue;

		s32 distYSquared = PROC_CollideHitbox_MipsSquare(distY);
		dist += distYSquared;
		if (distYSquared > 0x0fffffff)
			continue;

		s32 distZSquared = PROC_CollideHitbox_MipsSquare(distZ);
		dist += distZSquared;
		if (distZSquared > 0x0fffffff)
			continue;

		if (dist >= sps->Input1.hitRadiusSquared)
			continue;

		CTR_SET_VEC3(sps->Union.ThBuckColl.centerDelta.v, (s16)distX, (s16)distY, (s16)distZ);

		callback = sps->Union.ThBuckColl.funcCallback;
		callback(sps, collThread);
	}
}


enum
{
	THTICK_MAX_PENDING = 128
};

static void ThTick_PushPending(struct Thread **pending, int *count, struct Thread *thread)
{
	if (thread == NULL)
		return;

	if (*count >= THTICK_MAX_PENDING)
		return;

	pending[*count] = thread;
	(*count)++;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800715e8-0x80071694.
void ThTick_RunBucket(struct Thread *thread)
{
	struct Thread *pending[THTICK_MAX_PENDING];
	int count = 0;

	ThTick_PushPending(pending, &count, thread);

	while (count > 0)
	{
		struct Thread *t = pending[--count];

		ThTick_PushPending(pending, &count, t->siblingThread);

		if (t->cooldownFrameCount < 0)
			continue;

		if (t->cooldownFrameCount != 0)
		{
			t->cooldownFrameCount--;
			continue;
		}

		if (t->funcThTick != NULL)
			t->funcThTick(t);

		ThTick_PushPending(pending, &count, t->childThread);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80071694-0x800716ec as native-equivalent divergence.
void ThTick_FastRET(struct Thread *thread)
{
	(void)thread;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800716ec-0x80071704 as native-equivalent divergence.
void ThTick_SetAndExec(struct Thread *thread, void (*funcThTick)(struct Thread *))
{
	thread->funcThTick = funcThTick;
	funcThTick(thread);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80071704-0x8007170c.
void ThTick_Set(struct Thread *thread, void (*funcThTick)(struct Thread *))
{
	thread->funcThTick = funcThTick;
}
