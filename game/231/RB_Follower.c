#include <common.h>

static struct InstDrawPerPlayer *RB_Follower_GetIDPP(struct Instance *inst, int playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)inst + sizeof(struct Instance) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b6d58-0x800b6e10
void RB_Follower_ProcessBucket(struct Thread *t)
{
	int i;
	int numPlyr;
	int driverID;
	struct Follower *fObj;
	struct Instance *inst;
	struct InstDrawPerPlayer *idpp;
	struct GameTracker *gGT = sdata->gGT;

	numPlyr = gGT->numPlyrNextGame;

	for (/**/; t != 0; t = t->siblingThread)
	{
		// skip dead threads
		if ((t->flags & THREAD_FLAG_DEAD) != 0)
			continue;

		fObj = t->object;
		driverID = fObj->driver->driverID;

		inst = t->inst;
		idpp = RB_Follower_GetIDPP(inst, 0);

		// make Follower invisible to all other players
		for (i = 0; i < numPlyr; i++)
			if (i != driverID)
				idpp[i].instFlags &= ~DRAW_SUCCESSFUL;

		// make Mine invisible to this player
		inst = fObj->mineTh->inst;
		idpp = RB_Follower_GetIDPP(inst, driverID);
		idpp->instFlags &= ~DRAW_SUCCESSFUL;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b6e10-0x800b6f00.
void RB_Follower_ThTick(struct Thread *t)
{
	int kartState;
	struct Driver *d;
	struct Follower *fObj;
	struct Instance *inst;

	inst = t->inst;
	fObj = t->object;
	d = fObj->driver;
	kartState = d->kartState;

	fObj->frameCount--;

	if ((fObj->frameCount > 0) && ((kartState == KS_NORMAL) || (kartState == KS_DRIFTING)) &&

	    // terrible way of checking if mineTh was destroyed
	    // before the follower thread was destroyed
	    (fObj->mineTh->timesDestroyed == fObj->backupTimesDestroyed) &&

	    (d->speedApprox > -1))
	{
		if (inst->scale.x < 0x800)
		{
			inst->scale.x = inst->scale.x << 1;
			inst->scale.y = inst->scale.y << 1;
			inst->scale.z = inst->scale.z << 1;
		}

		// midpoint between real mine position, and driver position
		inst->matrix.t[0] = (fObj->realPos.x + (d->posCurr.x >> 8)) >> 1;
		inst->matrix.t[1] = (fObj->realPos.y + (d->posCurr.y >> 8)) >> 1;
		inst->matrix.t[2] = (fObj->realPos.z + (d->posCurr.z >> 8)) >> 1;

		return;
	}

	// kill thread
	t->flags |= THREAD_FLAG_DEAD;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b6f00-0x800b706c.
void RB_Follower_Init(struct Driver *d, struct Thread *mineTh)
{
	struct Thread *t;
	struct Instance *iVar1;
	struct Follower *fObj;
	struct Instance *iVar3;

	// disable for slow speed
	if (d->speedApprox <= 0x1e00)
		return;

	// disable for AI
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
		return;

	// disable for airborne camera
	if (((sdata->gGT->cameraDC[d->driverID].flags) & 0x10000) != 0)
		return;

	// create a thread and an Instance
	iVar1 = INSTANCE_BirthWithThread(mineTh->modelIndex, "follower", SMALL, FOLLOWER, RB_Follower_ThTick, sizeof(struct Follower), 0);

	if (iVar1 == NULL)
		return;

	// followerInst scale
	iVar1->scale.x = 0x200;
	iVar1->scale.y = 0x200;
	iVar1->scale.z = 0x200;

	// mineInst
	iVar3 = mineTh->inst;

	memcpy(&iVar1->matrix, &iVar3->matrix, sizeof(iVar1->matrix));

	t = iVar1->thread;
	t->funcThDestroy = PROC_DestroyInstance;

	fObj = t->object;
	fObj->frameCount = 7;
	fObj->driver = d;
	fObj->mineTh = mineTh;
	fObj->backupTimesDestroyed = mineTh->timesDestroyed;

	// backup original position
	for (int i = 0; i < 3; i++)
	{
		fObj->realPos.v[i] = iVar3->matrix.t[i];
	}
}
