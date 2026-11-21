#include <common.h>

#include <setjmp.h>

struct ThTickNativeContext
{
	jmp_buf env;
	struct Thread *currentThread;
	struct ThTickNativeContext *prev;
};

static struct ThTickNativeContext *s_thTickContext;


void PROC_DestroyTracker(struct Thread *t)
{
	struct GameTracker *gGT = GAME_TRACKER;

	if (gGT->numMissiles > 0)
	{
		gGT->numMissiles--;
	}

	PROC_DestroyInstance(t);
}


void PROC_DestroyInstance(struct Thread *t)
{
	INSTANCE_Death(t->inst);
}


void PROC_DestroyObject(void *object, ThreadFlags threadFlags)
{
	struct Item *item;

	if (object == NULL)
	{
		return;
	}

	// The payload follows the pool's intrusive list header.
	item = (struct Item *)((u8 *)object - sizeof(struct Item));
	switch (threadFlags & 0x300)
	{
	case LARGE:
		LIST_AddFront(&GAME_TRACKER->JitPools.largeStack.free, item);
		break;
	case MEDIUM:
		LIST_AddFront(&GAME_TRACKER->JitPools.mediumStack.free, item);
		break;
	default:
		LIST_AddFront(&GAME_TRACKER->JitPools.smallStack.free, item);
		break;
	}
}


void PROC_DestroySelf(struct Thread *t)
{
	// thread must exist
	if (t == 0)
	{
		return;
	}

	// This is usually PROC_DestroyInstance; run it before recycling either allocation.
	if (t->funcThDestroy != 0)
	{
		t->funcThDestroy(t);
	}

	// used by RB_Follower
	t->timesDestroyed++;

	// destroy object attached,
	// guaranteed all threads have one
	PROC_DestroyObject(t->object, t->flags);

	// recycle thread
	LIST_AddFront(&GAME_TRACKER->JitPools.thread.free, (struct Item *)t);
}


void PROC_DestroyBloodline(struct Thread *t)
{
	while (t != 0)
	{
		struct Thread *siblingThread;

		// recursively find all children
		if (t->childThread != 0)
		{
			PROC_DestroyBloodline(t->childThread);
		}

		siblingThread = t->siblingThread;
		PROC_DestroySelf(t);
		t = siblingThread;
	}
}


void PROC_CheckBloodlineForDead(struct Thread **replaceSelf, struct Thread *th)
{
	while (th != 0)
	{
		struct Thread *siblingThread = th->siblingThread;

		if ((th->flags & THREAD_FLAG_DEAD) != 0 && !Chaos_HasDeferredImpact(th))
		{
			if (th->childThread != 0)
			{
				PROC_DestroyBloodline(th->childThread);
			}

			PROC_DestroySelf(th);
			// Keep the incoming link: the next sibling may also need removal.
			*replaceSelf = siblingThread;
		}
		else
		{
			if (th->childThread != 0)
			{
				PROC_CheckBloodlineForDead(&th->childThread, th->childThread);
			}

			// This thread survives, so subsequent removals update its sibling link.
			replaceSelf = &th->siblingThread;
		}

		th = siblingThread;
	}
}


void PROC_CheckAllForDead(void)
{
	s32 i;

	for (i = 0; i < NUM_BUCKETS; i++)
	{
		PROC_CheckBloodlineForDead(&GAME_TRACKER->threadBuckets[i].thread, GAME_TRACKER->threadBuckets[i].thread);
	}
}


struct Thread *PROC_BirthWithObject(ThreadFlags flags, void *funcThTick, const char *name, struct Thread *relativeTh)
{
	u32 bucketID;
	u32 objectSize;
	u32 stackSize;
	void *stackObj;
	struct Thread *th;

	// determine bucketID from relativeTh or flags
	if (relativeTh != 0)
	{
		bucketID = relativeTh->flags & 0xff;
	}
	else
	{
		bucketID = flags & 0xff;
	}
	objectSize = flags >> 16;

	// NOTE(aalhendi): Retail allocates from a fixed-capacity pool before validating
	// the request. Keep the capacities shared with pool initialization.
	switch (flags & 0x300)
	{
	case LARGE:
		stackSize = THREAD_LARGE_STACK_SIZE;
		stackObj = LIST_RemoveFront(&GAME_TRACKER->JitPools.largeStack.free);
		break;
	case MEDIUM:
		stackSize = THREAD_MEDIUM_STACK_SIZE;
		stackObj = LIST_RemoveFront(&GAME_TRACKER->JitPools.mediumStack.free);
		break;
	default:
		stackSize = THREAD_SMALL_STACK_SIZE;
		stackObj = LIST_RemoveFront(&GAME_TRACKER->JitPools.smallStack.free);
		break;
	}

	// validate bucket
	if (bucketID >= NUM_BUCKETS)
	{
		if (stackObj != 0)
		{
			PROC_DestroyObject((u8 *)stackObj + sizeof(struct Item), flags);
		}
		return 0;
	}

	// NOTE(aalhendi): Retail rejects an exact payload-capacity request too.
	if (objectSize >= stackSize - sizeof(struct Item))
	{
		if (stackObj != 0)
		{
			PROC_DestroyObject((u8 *)stackObj + sizeof(struct Item), flags);
		}
		return 0;
	}

	// check stack object allocated
	if (stackObj == 0)
	{
		return 0;
	}

	// allocate thread SECOND
	th = (struct Thread *)LIST_RemoveFront(&GAME_TRACKER->JitPools.thread.free);

	// check thread allocated
	if (th == 0)
	{
		PROC_DestroyObject((u8 *)stackObj + sizeof(struct Item), flags);
		return 0;
	}

	// initialize thread fields
	th->flags = flags;
	Chaos_ClearThreadOwner(th);
	th->cooldownFrameCount = 0;
	th->funcThDestroy = 0;
	th->funcThCollide = 0;
	th->inst = 0;

	// handle relative thread linking
	if (relativeTh != 0)
	{
		if (flags & SELF_SIBLING)
		{
			struct Thread *parent;

			th->siblingThread = relativeTh->siblingThread;
			parent = relativeTh->parentThread;
			relativeTh->siblingThread = th;
			th->childThread = 0;
			th->parentThread = parent;
		}
		else if (flags & CHILD_BETWEEN)
		{
			// NOTE(aalhendi): Retail inserts the child chain without rewriting
			// the former children's parentThread pointers.
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
	}
	else
	{
		th->siblingThread = GAME_TRACKER->threadBuckets[bucketID].thread;
		GAME_TRACKER->threadBuckets[bucketID].thread = th;
		th->parentThread = 0;
		th->childThread = 0;
	}

	th->funcThTick = funcThTick;
	th->name = name;
	th->object = (u8 *)stackObj + sizeof(struct Item);

	return th;
}


void PROC_CollidePointWithSelf(struct Thread *th, struct BucketSearchParams *buf)
{
	struct Instance *inst;
	int distX;
	int distY;
	int distZ;
	int dist;

	if ((th->flags & (THREAD_FLAG_DEAD | THREAD_FLAG_DISABLE_COLLISION)) != 0)
	{
		return;
	}

	inst = th->inst;

	// Do not try to optimize this with loops,
	// it will not compile to less assembly,
	// 180 bytes is as low as this will go

	distX = (int)buf->pos.x - (int)inst->matrix.t[0];
	distY = (int)buf->pos.y - (int)inst->matrix.t[1];
	distZ = (int)buf->pos.z - (int)inst->matrix.t[2];

	if (distX * distX >= 0x10000000)
	{
		return;
	}
	if (distY * distY >= 0x10000000)
	{
		return;
	}
	if (distZ * distZ >= 0x10000000)
	{
		return;
	}

	dist = distX * distX + distY * distY + distZ * distZ;

	// if outside hit radius
	if (dist >= buf->bestDistSq)
	{
		return;
	}

	// return distance to center
	buf->bestDistSq = dist;

	// save the thread collided with
	buf->th = th;

	CTR_SET_VEC3(CTR_VECTOR_DATA(&(buf->dist)), (s16)distX, (s16)distY, (s16)distZ);
}


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
struct Thread *PROC_SearchForModel(struct Thread *th, s16 modelID)
{
	while (th != 0)
	{
		struct Thread *other;

		// if found, quit
		if (th->modelIndex == modelID)
		{
			return th;
		}

		// check children recursively, quit if found
		other = PROC_SearchForModel(th->childThread, modelID);
		if (other != 0)
		{
			return other;
		}

		th = th->siblingThread;
	}

	return th;
}


static s32 PROC_PerBspLeaf_MipsSquare(s32 value)
{
	return (s32)(u32)((s64)value * (s64)value);
}

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
	{
		return;
	}

	if (*(int *)bspHitbox == 0)
	{
		return;
	}

	for (/**/; *(int *)bspHitbox != 0; bspHitbox++)
	{
		s32 distYSquared;
		s32 distZSquared;

		if ((bspHitbox->flag & BSP_HITBOX_COLLIDABLE) == 0)
		{
			continue;
		}

		instDef = bspHitbox->data.hitbox.instDef;
		if ((instDef != NULL) && ((instDef->ptrInstance->flags & DRAW_COLLISION_MASK) == 0))
		{
			continue;
		}

		distX = (int)sps->Input1.pos.x - (int)bspHitbox->data.hitbox.center.x;
		distY = (int)sps->Input1.pos.y - (int)bspHitbox->data.hitbox.center.y;
		distZ = (int)sps->Input1.pos.z - (int)bspHitbox->data.hitbox.center.z;

		dist = PROC_PerBspLeaf_MipsSquare(distX);
		if (dist > 0x0fffffff)
		{
			continue;
		}

		distYSquared = PROC_PerBspLeaf_MipsSquare(distY);
		dist += distYSquared;
		if (distYSquared > 0x0fffffff)
		{
			continue;
		}

		distZSquared = PROC_PerBspLeaf_MipsSquare(distZ);
		dist += distZSquared;
		if (distZSquared > 0x0fffffff)
		{
			continue;
		}

		if (dist >= sps->Input1.hitRadiusSquared)
		{
			continue;
		}

		CTR_SET_VEC3(CTR_VECTOR_DATA(&(sps->Union.ThBuckColl.centerDelta)), (s16)distX, (s16)distY, (s16)distZ);

		callback = sps->Union.ThBuckColl.funcCallback;
		callback(sps, bspHitbox);
	}
}


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
		s32 distYSquared;
		s32 distZSquared;

		if (collThread->childThread != NULL)
		{
			PROC_CollideHitboxWithBucket(collThread->childThread, sps, ignoredThread);
		}

		if (collThread == ignoredThread)
		{
			continue;
		}

		if ((collThread->flags & 0x1800) != 0)
		{
			continue;
		}

		inst = collThread->inst;

		distX = (int)sps->Input1.pos.x - inst->matrix.t[0];
		distY = (int)sps->Input1.pos.y - inst->matrix.t[1];
		distZ = (int)sps->Input1.pos.z - inst->matrix.t[2];

		dist = PROC_CollideHitbox_MipsSquare(distX);
		if (dist > 0x0fffffff)
		{
			continue;
		}

		distYSquared = PROC_CollideHitbox_MipsSquare(distY);
		dist += distYSquared;
		if (distYSquared > 0x0fffffff)
		{
			continue;
		}

		distZSquared = PROC_CollideHitbox_MipsSquare(distZ);
		dist += distZSquared;
		if (distZSquared > 0x0fffffff)
		{
			continue;
		}

		if (dist >= sps->Input1.hitRadiusSquared)
		{
			continue;
		}

		CTR_SET_VEC3(CTR_VECTOR_DATA(&(sps->Union.ThBuckColl.centerDelta)), (s16)distX, (s16)distY, (s16)distZ);

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
	{
		return;
	}

	if (*count >= THTICK_MAX_PENDING)
	{
		return;
	}

	pending[*count] = thread;
	(*count)++;
}

internal struct Thread *ThTick_RunThreadNative(struct ThTickNativeContext *context, struct Thread *thread, ThreadFunc tick)
{
	context->currentThread = thread;
	if (setjmp(context->env) == 0)
	{
		tick(thread);
	}

	return context->currentThread;
}

void ThTick_RunBucket(struct Thread *thread)
{
	struct Thread *pending[THTICK_MAX_PENDING];
	int count = 0;

	struct ThTickNativeContext context;
	context.currentThread = NULL;
	context.prev = s_thTickContext;
	s_thTickContext = &context;

	ThTick_PushPending(pending, &count, thread);

	while (count > 0)
	{
		struct Thread *t = pending[--count];

		ThTick_PushPending(pending, &count, t->siblingThread);
		if ((t->flags & THREAD_FLAG_DEAD) != 0)
		{
			continue;
		}
		if (Chaos_ShouldFreezeThread(t))
		{
			const enum ChaosAudioOwner previousAudio = Chaos_SetAudioOwner(CHAOS_AUDIO_UI);
			t = ThTick_RunThreadNative(&context, t, Chaos_CollideFrozenThread);
			Chaos_SetAudioOwner(previousAudio);
			ThTick_PushPending(pending, &count, t->childThread);
			continue;
		}

		if (t->cooldownFrameCount < 0)
		{
			continue;
		}

		if (t->cooldownFrameCount != 0)
		{
			t->cooldownFrameCount--;
			continue;
		}

		if (t->funcThTick != NULL)
		{
			const enum ChaosAudioOwner previousAudio = Chaos_SetThreadAudio(t);
			t = ThTick_RunThreadNative(&context, t, t->funcThTick);
			Chaos_SetAudioOwner(previousAudio);
		}

		ThTick_PushPending(pending, &count, t->childThread);
	}

	s_thTickContext = context.prev;
}

void ThTick_FastRET(struct Thread *thread)
{
	(void)thread;
}

void ThTick_SetAndExec(struct Thread *thread, void (*funcThTick)(struct Thread *))
{
	thread->funcThTick = funcThTick;
	if (!Chaos_ShouldFreezeThread(thread))
	{
		funcThTick(thread);
	}

	// NOTE(aalhendi): Retail restores the ThTick_RunBucket stack from
	// scratchpad after the replacement tick returns. Native must not resume the
	// stale caller that requested the tick switch.
	if (s_thTickContext != NULL && s_thTickContext->currentThread != NULL)
	{
		longjmp(s_thTickContext->env, 1);
	}
}
