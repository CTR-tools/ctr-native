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
		if ((t->flags & 0x800) != 0)
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
void DECOMP_RB_Follower_ThTick(struct Thread *t)
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
		if (inst->scale[0] < 0x800)
		{
			inst->scale[0] = inst->scale[0] << 1;
			inst->scale[1] = inst->scale[1] << 1;
			inst->scale[2] = inst->scale[2] << 1;
		}

		// midpoint between real mine position, and driver position
		inst->matrix.t[0] = (fObj->realPos[0] + (d->posCurr.x >> 8)) >> 1;
		inst->matrix.t[1] = (fObj->realPos[1] + (d->posCurr.y >> 8)) >> 1;
		inst->matrix.t[2] = (fObj->realPos[2] + (d->posCurr.z >> 8)) >> 1;

		return;
	}

	// kill thread
	t->flags |= 0x800;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b6f00-0x800b706c.
void DECOMP_RB_Follower_Init(struct Driver *d, struct Thread *mineTh)
{
	struct Thread *t;
	struct Instance *iVar1;
	struct Follower *fObj;
	struct Instance *iVar3;

	// disable for slow speed
	if (d->speedApprox <= 0x1e00)
		return;

	// disable for AI
	if ((d->actionsFlagSet & 0x100000) != 0)
		return;

	// disable for airborne camera
	if (((sdata->gGT->cameraDC[d->driverID].flags) & 0x10000) != 0)
		return;

	// create a thread and an Instance
	iVar1 = DECOMP_INSTANCE_BirthWithThread(mineTh->modelIndex, "follower", SMALL, FOLLOWER, DECOMP_RB_Follower_ThTick, sizeof(struct Follower), 0);

	if (iVar1 == NULL)
		return;

	// followerInst scale
	iVar1->scale[0] = 0x200;
	iVar1->scale[1] = 0x200;
	iVar1->scale[2] = 0x200;

	// mineInst
	iVar3 = mineTh->inst;

	memcpy(&iVar1->matrix, &iVar3->matrix, sizeof(iVar1->matrix));

	t = iVar1->thread;
	t->funcThDestroy = DECOMP_PROC_DestroyInstance;

	fObj = t->object;
	fObj->frameCount = 7;
	fObj->driver = d;
	fObj->mineTh = mineTh;
	fObj->backupTimesDestroyed = mineTh->timesDestroyed;

	// backup original position
	for (int i = 0; i < 3; i++)
	{
		fObj->realPos[i] = iVar3->matrix.t[i];
	}
}
