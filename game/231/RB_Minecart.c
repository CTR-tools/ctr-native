#include <common.h>

// budget: 2132 bytes
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b7338-0x800b7b8c.

extern s16 minecartArr[50];

void RB_Minecart_CheckColl(struct Instance *minecartInst, struct Thread *minecartTh)
{
	struct Driver *hitDriver;
	struct Instance *hitInst;
	struct GameTracker *gGT = sdata->gGT;

	// check players
	hitInst = (struct Instance *)LinkedCollide_Radius(minecartInst, minecartTh, gGT->threadBuckets[PLAYER].thread, 0x10000);

	if (hitInst == 0)
	{
		// check robots
		hitInst = (struct Instance *)LinkedCollide_Radius(minecartInst, minecartTh, gGT->threadBuckets[ROBOT].thread, 0x10000);
	}

	if (hitInst != 0)
	{
		// get driver from instance
		hitDriver = (struct Driver *)hitInst->thread->object;

		// attempt to harm driver (squish or spin-out)
		RB_Hazard_HurtDriver(hitDriver, (minecartInst->model->id == DYNAMIC_SKUNK) ? 1 : 3, 0, 0);
	}
}

void RB_Minecart_NewPoint(struct Instance *minecartInst, struct Minecart *minecartObj, struct SpawnType2 *spawnType2)
{
	int pointIndex = minecartObj->posIndex * 3;

	for (int i = 0; i < 3; i++)
	{
		int start = spawnType2->posCoords[pointIndex + i - 3];
		int end = spawnType2->posCoords[pointIndex + i];

		minecartObj->posStart.v[i] = start;
		minecartObj->posEnd.v[i] = end;
		minecartInst->matrix.t[i] = start;
		minecartObj->dir.v[i] = start - end;
	}

#if defined(CTR_NATIVE)
	minecartObj->rotDesired.x = ratan2(minecartObj->dir.y, SquareRoot0_stub(minecartObj->dir.x * minecartObj->dir.x + minecartObj->dir.z * minecartObj->dir.z));
#endif

	minecartObj->rotDesired.y = ratan2(minecartObj->dir.x, minecartObj->dir.z) - 0x800;
}

void RB_Minecart_ThTick(struct Thread *t)
{
	struct Instance *minecartInst;
	struct Minecart *minecartObj;
	struct Level *level;
	struct SpawnType2 *spawnType2;
	int numCoords;

	s16 i;

	minecartInst = t->inst;
	minecartObj = (struct Minecart *)t->object;
	level = sdata->gGT->level1;

	// if animation is not over
	if ((minecartInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(minecartInst, 0))
	{
		// increment frame
		minecartInst->animFrame = minecartInst->animFrame + 1;
	}

	// if animation is done
	else
	{
		// reset animation
		minecartInst->animFrame = 0;
	}

	if (level->numSpawnType2 == 0)
		return;

	// path coordinates for minecarts
	spawnType2 = &level->ptrSpawnType2[0];
	numCoords = spawnType2->numCoords;

	// between two points
	if (minecartObj->betweenPoints_currFrame < minecartObj->betweenPoints_numFrames)
	{
		minecartObj->betweenPoints_currFrame++;
	}

	// reached point
	else
	{
		minecartObj->betweenPoints_currFrame = 1;

		// if not at end of path
		if (minecartObj->posIndex + 1 < numCoords)
		{
			minecartObj->posIndex++;
		}

		// end of path, reset
		else
		{
			minecartObj->posIndex = 1;
		}

		RB_Minecart_NewPoint(minecartInst, minecartObj, spawnType2);

		if ((minecartObj->posIndex == 1) && (minecartInst->model->id == DYNAMIC_MINE_CART))
		{
			for (i = 0; i < 3; i++)
			{
				minecartObj->rotCurr.v[i] = minecartObj->rotDesired.v[i];
			}
		}
	}

	// what is this?
	minecartInst->depthBiasNormal = minecartArr[minecartObj->posIndex];
	minecartInst->depthBiasSecondary = minecartArr[minecartObj->posIndex];

	for (i = 0; i < 3; i++)
	{
		minecartInst->matrix.t[i] =
		    minecartObj->posStart.v[i] - ((minecartObj->betweenPoints_currFrame * minecartObj->dir.v[i]) / minecartObj->betweenPoints_numFrames);
	}

	minecartObj->rotCurr.y = RB_Hazard_InterpolateValue(minecartObj->rotCurr.y, minecartObj->rotDesired.y, minecartObj->rotSpeed);
	minecartObj->rotCurr.x = RB_Hazard_InterpolateValue(minecartObj->rotCurr.x, minecartObj->rotDesired.x, minecartObj->rotSpeed);

	// converted to TEST in rebuildPS1
	ConvertRotToMatrix(&minecartInst->matrix, &minecartObj->rotCurr);

	PlaySound3D_Flags(&minecartObj->audioPtr,
	                  0x72, // minecart sound
	                  minecartInst);

	RB_Minecart_CheckColl(minecartInst, t);
}

void RB_Minecart_LInB(struct Instance *inst)
{
	struct Minecart *minecartObj;
	struct SpawnType2 *spawnType2;
	struct Thread *t;
	int minecartID;
	int startIndex;

	if (inst->thread != 0)
		return;

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Minecart), NONE, SMALL, STATIC),

	    RB_Minecart_ThTick, // behavior
	    "minecart",         // debug name
	    0                   // thread relative
	);

	if (t == 0)
		return;
	inst->thread = t;
	t->inst = inst;

	// memset is faster than erasing the following
	// betweenPoints_currFrame, rotDesired[2], audioPtr,
	// rotCurr[0], rotCurr[1], rotCurr[2]

	minecartObj = ((struct Minecart *)t->object);
	memset(minecartObj, 0, sizeof(struct Minecart));
	minecartObj->betweenPoints_numFrames = 8;
	minecartObj->rotSpeed = 0x20;

	inst->scale.x = 0x1000;
	inst->scale.y = 0x1000;
	inst->scale.z = 0x1000;

	if (inst->model->id == DYNAMIC_SKUNK)
	{
		inst->scale.x = 0x2000;
		inst->scale.y = 0x2000;
		inst->scale.z = 0x2000;
		minecartObj->betweenPoints_numFrames = 4;
		minecartObj->rotSpeed = 0x18;
	}

	else if (inst->model->id == DYNAMIC_VONLABASS)
	{
		inst->scale.x = 0x800;
		inst->scale.y = 0x800;
		inst->scale.z = 0x800;
		minecartObj->betweenPoints_numFrames = 4;
		minecartObj->rotSpeed = 0x18;
	}

	// path coordinates for minecarts
	spawnType2 = &sdata->gGT->level1->ptrSpawnType2[0];

	// from instance
	minecartID = inst->name[strlen(inst->name) - 1] - '0';

	// minecart#0
	startIndex = 1;

	// #1 and #2
	if (minecartID != 0)
	{
		// #1
		// 50 points (0x32)
		startIndex = spawnType2->numCoords;

		// #2 and any other non-0/non-1 suffix
		if (minecartID != 1)
			startIndex = startIndex << 1;

		startIndex = startIndex / 3;
	}

	// #0 = 0%
	// #1 = 33%
	// #2 = 66%

	minecartObj->posIndex = startIndex;

	RB_Minecart_NewPoint(inst, minecartObj, spawnType2);

	return;
}

s16 minecartArr[50] = {0xC,  0xC,  0xC,  0xC,  0xC,  0xC,  0x6,  0x6,  0xC,  0xC,  0x9,  0x9,  0xC,  0xC,  0xC, 0x18, 0x18,
                       0x18, 0x1A, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xC,  0xC, 0xC,  0xC,
                       0xC,  0xC,  0xC,  0xC,  0xC,  0x0,  0x0,  0x6,  0x6,  0x6,  0x18, 0x18, 0x18, 0x18, 0xC, 0xC};
