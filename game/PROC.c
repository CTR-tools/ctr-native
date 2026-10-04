#include <common.h>

#if defined(CTR_NATIVE)
#include <setjmp.h>

struct ThTickNativeContext
{
	jmp_buf env;
	struct Thread *currentThread;
	struct ThTickNativeContext *prev;
};

static struct ThTickNativeContext *s_thTickContext;
#endif


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

		if ((th->flags & THREAD_FLAG_DEAD) != 0)
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
	register struct BucketSearchParams *results CTR_PSX_REGISTER("$9") = buf;
	struct Instance *inst;
	int distX;
	int distY;
	int distZ;
	register int dist CTR_PSX_REGISTER("$7");
	int square;

	if ((th->flags & (THREAD_FLAG_DEAD | THREAD_FLAG_DISABLE_COLLISION)) != 0)
	{
		return;
	}

	// NOTE(aalhendi): Keep the result pointer in t1 for the first position load.
	CTR_PSX_DEPEND_VALUE(results, buf);
	inst = th->inst;

	// NOTE(aalhendi): Retail accumulates each squared delta before testing that axis.

	distX = (int)results->pos.x - (int)inst->matrix.t[0];
	distY = (int)results->pos.y - (int)inst->matrix.t[1];
	distZ = (int)results->pos.z - (int)inst->matrix.t[2];

	dist = CTR_MipsMulLo(distX, distX);
	if (dist >= 0x10000000)
	{
		return;
	}
	square = CTR_MipsMulLo(distY, distY);
	dist = CTR_MipsAddLo(dist, square);
	if (square >= 0x10000000)
	{
		return;
	}
	square = CTR_MipsMulLo(distZ, distZ);
	dist = CTR_MipsAddLo(dist, square);
	if (square >= 0x10000000)
	{
		return;
	}

	// if outside hit radius
	if (dist >= results->bestDistSq)
	{
		return;
	}

	// return distance to center
	results->bestDistSq = dist;

	// save the thread collided with
	results->th = th;

	CTR_SET_VEC3(CTR_VECTOR_DATA(&(results->dist)), (s16)distX, (s16)distY, (s16)distZ);
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
struct Thread *PROC_SearchForModel(struct Thread *th, s32 modelID)
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


void PROC_PerBspLeaf_CheckInstances(struct BSP *bspLeaf, struct ScratchpadStruct *sps)
{
	// NOTE(aalhendi): Retail holds the scratchpad in s2 and the running squared distance in a2.
	register struct ScratchpadStruct *params CTR_PSX_REGISTER("$18") = sps;
	s32 distX;
	s32 distY;
	s32 distZ;
	register s32 dist CTR_PSX_REGISTER("$6");
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
		// NOTE(aalhendi): Keep the radius comparison result in v0 until its branch.
		register s32 closer CTR_PSX_REGISTER("$2");

		if (((u8)bspHitbox->flag & BSP_HITBOX_COLLIDABLE) == 0)
		{
			continue;
		}

		instDef = bspHitbox->data.hitbox.instDef;
		if ((instDef != NULL) && ((instDef->ptrInstance->flags & DRAW_COLLISION_MASK) == 0))
		{
			continue;
		}

		distX = (int)params->Input1.pos.x - (int)bspHitbox->data.hitbox.center.x;
		distY = (int)params->Input1.pos.y - (int)bspHitbox->data.hitbox.center.y;
		distZ = (int)params->Input1.pos.z - (int)bspHitbox->data.hitbox.center.z;

		dist = CTR_MipsMulLo(distX, distX);
		if (dist > 0x0fffffff)
		{
			continue;
		}

		distYSquared = CTR_MipsMulLo(distY, distY);
		dist = CTR_MipsAddLo(dist, distYSquared);
		if (distYSquared > 0x0fffffff)
		{
			continue;
		}

		distZSquared = CTR_MipsMulLo(distZ, distZ);
		dist = CTR_MipsAddLo(dist, distZSquared);
		if (distZSquared > 0x0fffffff)
		{
			continue;
		}

		closer = dist < params->Input1.hitRadiusSquared;
		if (closer == 0)
		{
			continue;
		}

		callback = params->Union.ThBuckColl.funcCallback;
		// NOTE(aalhendi): Retail writes Z and Y before the call, then X in its delay slot.
		params->Union.ThBuckColl.centerDelta.z = (s16)distZ;
		params->Union.ThBuckColl.centerDelta.y = (s16)distY;
		params->Union.ThBuckColl.centerDelta.x = (s16)distX;
		callback(params, bspHitbox);
	}
}


void PROC_StartSearch_Self(struct ScratchpadStruct *sps)
{
	register u32 radius CTR_PSX_REGISTER("$4");
	register u32 radiusOther CTR_PSX_REGISTER("$5");
	u32 posA;
	u32 posB;
	struct GameTracker *gGT;
	struct BSP *root;

	// NOTE(aalhendi): Interleave X/Z/Y in posA with Y/X/Z in posB to retain
	// retail's load/store order. The independent radius copy spans both lanes;
	// unsigned arithmetic preserves the low 16 bits stored in each bound.
	posA = (u16)sps->Input1.pos.x;
	radius = (u16)sps->Input1.hitRadius;
	posB = (u16)sps->Input1.pos.y;
	radiusOther = radius;
	CTR_PSX_KEEP_VALUE_RELAXED(radiusOther);
	posA -= radius;
	sps->Union.ThBuckColl.bbox.min.x = (s16)posA;
	posA = (u16)sps->Input1.pos.z;
	posB -= radiusOther;
	sps->Union.ThBuckColl.bbox.min.y = (s16)posB;
	posB = (u16)sps->Input1.pos.x;
	posA -= radius;
	sps->Union.ThBuckColl.bbox.min.z = (s16)posA;
	posA = (u16)sps->Input1.pos.y;
	posB += radiusOther;
	sps->Union.ThBuckColl.bbox.max.x = (s16)posB;
	posB = (u16)sps->Input1.pos.z;
	posA += radius;
	sps->Union.ThBuckColl.bbox.max.y = (s16)posA;
	// NOTE(aalhendi): These empty constraints keep late Z and the root load
	// ahead of call preparation; native retains ordinary C value semantics.
	CTR_PSX_ORDER_VALUES(posB, radius);
	gGT = GAME_TRACKER;
	posB += radiusOther;
	sps->Union.ThBuckColl.bbox.max.z = (s16)posB;
	root = gGT->level1->ptr_mesh_info->bspRoot;
	CTR_PSX_DEPEND_VALUE(sps, root);
	COLL_SearchBSP_CallbackPARAM(root, &sps->Union.ThBuckColl.bbox, PROC_PerBspLeaf_CheckInstances, sps);
}


void PROC_CollideHitboxWithBucket(struct Thread *collThread, struct ScratchpadStruct *sps, struct Thread *ignoredThread)
{
	s32 distX;
	s32 distY;
	register s32 distZ CTR_PSX_REGISTER("$3");
	register s32 dist CTR_PSX_REGISTER("$7");
	// NOTE(aalhendi): Separate Y/Z operands retain retail's position-then-matrix load order.
	register s32 posY CTR_PSX_REGISTER("$6");
	register s32 posZ CTR_PSX_REGISTER("$5");
	register s32 matrixY CTR_PSX_REGISTER("$3");
	register s32 matrixZ CTR_PSX_REGISTER("$2");
	register struct Instance *inst CTR_PSX_REGISTER("$4");
	CollThBuckCallback callback;

	for (/**/; collThread != NULL; collThread = collThread->siblingThread)
	{
		s32 distYSquared;
		s32 distZSquared;
		register s32 closer CTR_PSX_REGISTER("$2");

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

		distX = CTR_MipsSubLo((int)sps->Input1.pos.x, inst->matrix.t[0]);
		dist = CTR_MipsMulLo(distX, distX);
		posY = (int)sps->Input1.pos.y;
		posZ = (int)sps->Input1.pos.z;
		matrixY = inst->matrix.t[1];
		matrixZ = inst->matrix.t[2];
		distY = (s32)((u32)posY - (u32)matrixY);
		distZ = CTR_MipsSubLo(posZ, matrixZ);
		// NOTE(aalhendi): Keep the Z delta in v1 without a later register move.
		CTR_PSX_DEPEND_VALUE(distZ, distY);
		if (dist > 0x0fffffff)
		{
			continue;
		}

		distYSquared = CTR_MipsMulLo(distY, distY);
		dist = CTR_MipsAddLo(dist, distYSquared);
		if (distYSquared > 0x0fffffff)
		{
			continue;
		}

		distZSquared = CTR_MipsMulLo(distZ, distZ);
		dist = CTR_MipsAddLo(dist, distZSquared);
		if (distZSquared > 0x0fffffff)
		{
			continue;
		}

		closer = dist < sps->Input1.hitRadiusSquared;
		if (closer == 0)
		{
			continue;
		}

		// NOTE(aalhendi): Retail loads the callback before the delta stores and puts Z in the call delay slot.
		callback = sps->Union.ThBuckColl.funcCallback;
		sps->Union.ThBuckColl.centerDelta.x = (s16)distX;
		sps->Union.ThBuckColl.centerDelta.y = (s16)distY;
		sps->Union.ThBuckColl.centerDelta.z = (s16)distZ;
		callback(sps, collThread);
	}
}


#if defined(CTR_NATIVE)
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

internal struct Thread *ThTick_RunThreadNative(struct ThTickNativeContext *context, struct Thread *thread)
{
	context->currentThread = thread;
	if (setjmp(context->env) == 0)
	{
		thread->funcThTick(thread);
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
			t = ThTick_RunThreadNative(&context, t);
		}

		ThTick_PushPending(pending, &count, t->childThread);
	}

	s_thTickContext = context.prev;
}
#else
// NOTE(aalhendi): Retail's bucket walker uses scratchpad 0x1f8000b0..0xe8
// for saved registers and its pending-thread stack. The assembler inserts
// nops after symbolic branches here, so branch words retain retail's slots.
// The final EXE link must keep FastRET adjacent at its retail address.
__asm__(".section .ThTick_RunBucket,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent ThTick_RunBucket\n"
        ".set noreorder\n"
        ".set noat\n"
        ".globl ThTick_RunBucket\n"
        "ThTick_RunBucket:\n"
        "lui $1, 0x1f80\n"
        "sw $16, 0xb0($1)\n"
        "sw $17, 0xb4($1)\n"
        "sw $18, 0xb8($1)\n"
        "sw $19, 0xbc($1)\n"
        "sw $20, 0xc0($1)\n"
        "sw $21, 0xc4($1)\n"
        "sw $22, 0xc8($1)\n"
        "sw $23, 0xcc($1)\n"
        "sw $28, 0xd0($1)\n"
        "sw $29, 0xd4($1)\n"
        "sw $30, 0xd8($1)\n"
        "sw $31, 0xdc($1)\n"
        "sw $4, 0xe8($1)\n"
        "addiu $2, $1, 4\n"
        "sw $2, 0xe4($1)\n"
        ".LThTick_RunBucket_loop:\n"
        "lw $4, 0xe4($2)\n"
        "addiu $2, $2, -4\n"
        "subu $3, $2, $1\n"
        ".word 0x0460001f # bltz v1, FastRET_restore\n"
        "sw $2, 0xe4($1)\n"
        "lw $3, 0x10($4)\n"
        "lw $8, 0x18($4)\n"
        ".word 0x10600002 # beqz v1, no_sibling\n"
        "sw $3, 0xe8($2)\n"
        "addiu $2, $2, 4\n"
        ".LThTick_RunBucket_no_sibling:\n"
        ".word 0x0500fff5 # bltz t0, bucket_loop\n"
        "sw $2, 0xe4($1)\n"
        ".word 0x1500000b # bnez t0, decrement\n"
        "addiu $8, $8, -1\n"
        "lw $3, 0x2c($4)\n"
        "nop\n"
        ".word 0x1060000a # beqz v1, FastRET\n"
        "nop\n"
        "jalr $3\n"
        "sw $4, 0xe0($1)\n"
        "lui $1, 0x1f80\n"
        "lw $4, 0xe0($1)\n"
        ".word 0x04010004 # bgez zero, FastRET\n"
        "nop\n"
        ".LThTick_RunBucket_decrement:\n"
        ".word 0x0401ffe7 # bgez zero, bucket_loop\n"
        "sw $8, 0x18($4)\n"
        "sw $5, 0x2c($4)\n"
        ".end ThTick_RunBucket\n"
        ".set at\n"
        ".set reorder\n"
        ".text\n");
#endif

#ifndef CTR_NATIVE
// NOTE(aalhendi): The first eight instructions resume RunBucket's scratchpad
// queue. The register-restore tail is also RunBucket's shared exit target.
// PC-relative branch words preserve both delay-slot instructions.
__asm__(".section .ThTick_FastRET,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent ThTick_FastRET\n"
        ".set noreorder\n"
        ".set noat\n"
        ".globl ThTick_FastRET\n"
        "ThTick_FastRET:\n"
        "lui $1, 0x1f80\n"
        "lw $3, 0x14($4)\n"
        "lw $2, 0xe4($1)\n"
        "lw $29, 0xd4($1)\n"
        ".word 0x1060ffe0 # beqz v1, RunBucket_loop\n"
        "sw $3, 0xe8($2)\n"
        ".word 0x0401ffde # bgez zero, RunBucket_loop\n"
        "addiu $2, $2, 4\n"
        ".LThTick_FastRET_restore:\n"
        "lw $31, 0xdc($1)\n"
        "lw $30, 0xd8($1)\n"
        "lw $29, 0xd4($1)\n"
        "lw $28, 0xd0($1)\n"
        "lw $23, 0xcc($1)\n"
        "lw $22, 0xc8($1)\n"
        "lw $21, 0xc4($1)\n"
        "lw $20, 0xc0($1)\n"
        "lw $19, 0xbc($1)\n"
        "lw $18, 0xb8($1)\n"
        "lw $17, 0xb4($1)\n"
        "lw $16, 0xb0($1)\n"
        "jr $31\n"
        "nop\n"
        ".end ThTick_FastRET\n"
        ".set at\n"
        ".set reorder\n"
        ".text\n");
#else
void ThTick_FastRET(struct Thread *thread)
{
	(void)thread;
}
#endif

#ifndef CTR_NATIVE
// NOTE(aalhendi): Retail restores ThTick_RunBucket's stack from scratchpad
// 0x1f8000d4, then resumes it at 0x80071678 after the replacement tick.
// This nonlocal jump has no ordinary C equivalent.
__asm__(".section .ThTick_SetAndExec,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent ThTick_SetAndExec\n"
        ".set noreorder\n"
        ".set noat\n"
        ".globl ThTick_SetAndExec\n"
        "ThTick_SetAndExec:\n"
        "lui $1, 0x1f80\n"
        "lw $29, 0xd4($1)\n"
        "lui $31, 0x8007\n"
        "ori $31, $31, 0x1678\n"
        "jr $5\n"
        "sw $5, 0x2c($4)\n"
        ".end ThTick_SetAndExec\n"
        ".set at\n"
        ".set reorder\n"
        ".text\n");
#else
void ThTick_SetAndExec(struct Thread *thread, void (*funcThTick)(struct Thread *))
{
	thread->funcThTick = funcThTick;
	funcThTick(thread);

	// NOTE(aalhendi): Retail restores the ThTick_RunBucket stack from
	// scratchpad after the replacement tick returns. Native must not resume the
	// stale caller that requested the tick switch.
	if (s_thTickContext != NULL && s_thTickContext->currentThread != NULL)
	{
		longjmp(s_thTickContext->env, 1);
	}
}
#endif

void ThTick_Set(struct Thread *thread, void (*funcThTick)(struct Thread *))
{
	thread->funcThTick = funcThTick;
}
