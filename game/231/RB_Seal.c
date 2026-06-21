#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b8c00-0x800b92ac.


// one seal can not collide with more than one other thread,
// then quits, it was like that in the original game too,
// but one seal likely-wont collide with two threads at the same time
void Seal_CheckColl(struct Instance *sealInst, struct Thread *sealTh, int damage, int radius, int sound)
{
	struct GameTracker *gGT;
	struct Instance *hitInst;
	struct Driver *hitDriver;
	int boolHurt;
	char kartStatePrev;

	gGT = sdata->gGT;

	// check players
	hitInst = (struct Instance *)LinkedCollide_Radius(sealInst, sealTh, gGT->threadBuckets[PLAYER].thread, radius);

	// if hit a player
	if (hitInst != 0)
	{
		// get driver from instance
		hitDriver = (struct Driver *)hitInst->thread->object;

		// backup
		kartStatePrev = hitDriver->kartState;

		// attempt to harm driver (spin out)
		boolHurt = RB_Hazard_HurtDriver(hitDriver, damage, 0, 0);

		// if failed, due to mask grab or mask weapon
		if (boolHurt == 0)
			return;

		// if driver was already spinning out
		if (kartStatePrev == KS_SPINNING)
			return;

		if (sound == 0)
			return;

		// play seal sound, with echo if driver is on an echo quadblock
		OtherFX_Play_Echo(sound, 1, hitDriver->actionsFlagSet & ACTION_ENGINE_ECHO);

		// dont check other buckets
		return;
	}

	// check robots
	hitInst = (struct Instance *)LinkedCollide_Radius(sealInst, sealTh, gGT->threadBuckets[ROBOT].thread, radius);

	// if hit a robot
	if (hitInst != 0)
	{
		// get driver from instance
		hitDriver = (struct Driver *)hitInst->thread->object;

		// attempt to harm driver (spin out)
		RB_Hazard_HurtDriver(hitDriver, damage, 0, 0);

		// dont check other buckets
		return;
	}

	// check mines
	hitInst = (struct Instance *)LinkedCollide_Radius(sealInst, sealTh, gGT->threadBuckets[MINE].thread, radius);

	// if hit a mine
	if (hitInst != 0)
	{
		// all mine ThCollide functions only take one parameter,
		// all other ThCollide functions are erased due to redundancy
		hitInst->thread->funcThCollide(hitInst->thread);

		// dont check other bucket
		return;
	}
}

int RB_Seal_ThCollide(struct Thread *sealThread, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	(void)sealThread;
	(void)driverTh;
	(void)funcThCollide;

	return sps->Input1.modelID == DYNAMIC_PLAYER;
}

void RB_Seal_ThTick_TurnAround(struct Thread *t)
{
	struct Instance *sealInst;
	struct Seal *sealObj;

	sealInst = t->inst;
	sealObj = (struct Seal *)t->object;

	// if animation is not over
	if ((sealInst->animFrame + 2) < INSTANCE_GetNumAnimFrames(sealInst, 0))
	{
		// increment frame
		sealInst->animFrame = sealInst->animFrame + 2;
	}

	// if animation is done
	else
	{
		// reset animation
		sealInst->animFrame = 0;

		// only play sound in TurnAround
		PlaySound3D(0x77, sealInst);
	}

	// if rotation is finished
	if (sealObj->rotCurr.y == sealObj->rotDesiredAlt.y)
	{
		sealObj->numFramesSpinning = 0;

		for (int i = 0; i < 3; i++)
		{
			sealObj->rotDesired.v[i] = sealObj->rotCurr.v[i];
		}

		ConvertRotToMatrix(&sealInst->matrix, &sealObj->rotCurr);

		ThTick_SetAndExec(t, RB_Seal_ThTick_Move);
		return;
	}

	else
	{
		// spin rotCurrY 180 degrees (turn around)
		sealObj->rotCurr.y = RB_Hazard_InterpolateValue(sealObj->rotCurr.y, sealObj->rotDesiredAlt.y, 0x80);

		// negate rotCurrX (slant)
		sealObj->rotCurr.x = RB_Hazard_InterpolateValue(sealObj->rotCurr.x, -sealObj->rotDesired.x, 0x14);

		// negate rotCurrZ (slant)
		sealObj->rotCurr.z = RB_Hazard_InterpolateValue(sealObj->rotCurr.z, -sealObj->rotDesired.z, 0x14);

		sealObj->numFramesSpinning++;

		// converted to TEST in rebuildPS1
		ConvertRotToMatrix(&sealInst->matrix, &sealObj->rotCurr);
	}

	Seal_CheckColl(sealInst, t, 1, 0x4000, 0x78);
}

void RB_Seal_ThTick_Move(struct Thread *t)
{
	struct Instance *sealInst;
	struct Seal *sealObj;
	int i;

	sealInst = t->inst;
	sealObj = (struct Seal *)t->object;

	// if animation is not over
	if ((sealInst->animFrame + 2) < INSTANCE_GetNumAnimFrames(sealInst, 0))
	{
		// increment frame
		sealInst->animFrame = sealInst->animFrame + 2;
	}

	// if animation is done
	else
	{
		// reset animation
		sealInst->animFrame = 0;

		// no sound here
	}

	// move seal
	for (i = 0; i < 3; i++)
	{
		sealInst->matrix.t[i] = (int)sealObj->spawnPos.v[i] - (sealObj->distFromSpawn * (int)sealObj->vel.v[i]) / 0x2d;
	}

	// moving towards spawn (0)
	if (sealObj->direction == 0)
	{
		if (sealObj->distFromSpawn > 0)
		{
			sealObj->distFromSpawn--;
			Seal_CheckColl(sealInst, t, 1, 0x4000, 0x78);
			return;
		}

		if (sealObj->distFromSpawn != 0)
		{
			Seal_CheckColl(sealInst, t, 1, 0x4000, 0x78);
			return;
		}

		sealObj->direction = 1;
	}

	// moving away from spawn (1)
	else
	{
		if (sealObj->distFromSpawn < 0x2d)
		{
			sealObj->distFromSpawn++;
			Seal_CheckColl(sealInst, t, 1, 0x4000, 0x78);
			return;
		}

		if (sealObj->distFromSpawn != 0x2d)
		{
			Seal_CheckColl(sealInst, t, 1, 0x4000, 0x78);
			return;
		}

		sealObj->direction = 0;
	}

	// === end of Move state ===

	// flip Y 180 degrees (turn around)
	sealObj->rotDesiredAlt.y = (sealObj->rotCurr.y + 0x800) & 0xfff;

	// turn around
	ThTick_SetAndExec(t, RB_Seal_ThTick_TurnAround);
	return;
}

void RB_Seal_LInB(struct Instance *inst)
{
	struct Seal *sealObj;
	struct SpawnType2 *spawnType2;
	struct InstDef *instDef;
	struct Thread *t;

	if (inst->thread != 0)
		return;

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Seal), NONE, SMALL, STATIC),

	    RB_Seal_ThTick_Move, // behavior
	    "seal",              // debug name
	    0                    // thread relative
	);

	if (t == 0)
		return;
	inst->thread = t;
	t->inst = inst;
	t->funcThCollide = (void (*)(struct Thread *))RB_Seal_ThCollide;

	inst->scale.x = 0x2000;
	inst->scale.y = 0x2000;
	inst->scale.z = 0x2000;

	sealObj = ((struct Seal *)t->object);
	sealObj->distFromSpawn = 0;
	sealObj->direction = 1;
	sealObj->sealID = inst->name[strlen(inst->name) - 1] - '0';

	if (sdata->gGT->level1->numSpawnType2 != 0)
	{
		spawnType2 = &sdata->gGT->level1->ptrSpawnType2[sealObj->sealID];

		// spawnPos
		*(int *)&sealObj->spawnPos.x = *(int *)&spawnType2->posCoords[0];
		sealObj->spawnPos.z = spawnType2->posCoords[2];

		// endPos
		*(int *)&sealObj->endPos.x = *(int *)&spawnType2->posCoords[3];
		sealObj->endPos.z = spawnType2->posCoords[5];
	}

	// distance between points
	for (int i = 0; i < 3; i++)
	{
		sealObj->vel.v[i] = sealObj->spawnPos.v[i] - sealObj->endPos.v[i];
	}

	// rotCurr
	instDef = inst->instDef;
	sealObj->rotCurr.x = instDef->rot.x;
	sealObj->rotCurr.y = instDef->rot.y;
	sealObj->rotCurr.z = instDef->rot.z;

	for (int i = 0; i < 3; i++)
	{
		sealObj->rotDesired.v[i] = sealObj->rotCurr.v[i];
	}

	sealObj->numFramesSpinning = 0;

	// converted to TEST in rebuildPS1
	ConvertRotToMatrix(&inst->matrix, &sealObj->rotCurr);

	// dont call RB_Default_LInB(inst),
	// we know seal is never over ice
}
