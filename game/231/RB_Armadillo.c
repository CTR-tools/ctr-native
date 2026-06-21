#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b5984-0x800b5f50.


int RB_Armadillo_ThCollide(struct Thread *armadilloThread, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	(void)armadilloThread;
	(void)driverTh;
	(void)funcThCollide;

	return sps->Input1.modelID == DYNAMIC_PLAYER;
}

void RB_Armadillo_ThTick_TurnAround(struct Thread *t)
{
	struct Instance *armInst;
	struct Armadillo *armObj;
	int i;

	armInst = t->inst;
	armObj = (struct Armadillo *)t->object;

	if (armObj->rotCurr.y == armObj->rotDesired.y)
	{
		// if animation is not over
		if ((armInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(armInst, 0))
		{
			armInst->animFrame = armInst->animFrame + 1;
		}

		// === End of TurnAround ===
		else
		{
			armObj->velX = -armObj->velX;
			armObj->numFramesSpinning = 0;
			armObj->velZ = -armObj->velZ;

			armObj->direction = (armObj->direction == 0) ? 1 : 0;

			// play roll sound
			PlaySound3D(0x70, armInst);

			// rolling animation
			armInst->animIndex = 1;
			armInst->animFrame = 0;

			ThTick_SetAndExec(t, RB_Armadillo_ThTick_Rolling);
		}
	}

	else
	{
		// spin rotCurrY 180 degrees (turn around)
		armObj->rotCurr.y = RB_Hazard_InterpolateValue(armObj->rotCurr.y, armObj->rotDesired.y, 0x100);

		// converted to TEST in rebuildPS1
		ConvertRotToMatrix(&armInst->matrix, &armObj->rotCurr);

		// increment frame
		armInst->animFrame = armInst->animFrame + 1;
	}

	Seal_CheckColl(armInst, t, 1, 0x2400, 0x71);
}

void RB_Armadillo_ThTick_Rolling(struct Thread *t)
{
	struct Instance *armInst;
	struct Armadillo *armObj;
	SVECTOR rot;
	int i;

	armInst = t->inst;
	armObj = (struct Armadillo *)t->object;

	if (armObj->timeAtEdge != 0)
	{
		armObj->timeAtEdge--;
		return;
	}

	if (armObj->timeRolling < 0x500)
	{
		// 32ms, 30fps
		armObj->timeRolling += 0x20;

		if (armObj->direction == 0)
			armObj->distFromSpawn++;
		else
			armObj->distFromSpawn--;

		armInst->matrix.t[0] += armObj->velX;
		armInst->matrix.t[2] += armObj->velZ;

		// if animation is not over
		if ((armInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(armInst, 1))
		{
			// increment frame
			armInst->animFrame = armInst->animFrame + 1;
		}

		// if animation is done
		else
		{
			// reset animation
			armInst->animFrame = 0;

			// no sound here
		}

		Seal_CheckColl(armInst, t, 1, 0x2400, 0x71);
		return;
	}

	// == End of Rolling ===
	CTR_MatrixToRot(&rot, &armInst->matrix, 0x11);

	// reset
	armObj->rotCurr.x = rot.vy;
	armObj->rotCurr.y = rot.vx;
	armObj->rotCurr.z = rot.vz;
	armObj->timeRolling = 0;

	// jumping animation
	armInst->animIndex = 0;
	armInst->animFrame = 0;

	armObj->rotDesired.y = (armObj->rotCurr.y + 0x800) & 0xfff;

	ThTick_SetAndExec(t, RB_Armadillo_ThTick_TurnAround);
}

void RB_Armadillo_LInB(struct Instance *inst)
{
	struct Armadillo *armObj;
	SVECTOR rot;
	s16 *metaArray;
	void **pointers;
	struct Thread *t;

	if (inst->thread != 0)
		return;

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Armadillo), NONE, SMALL, STATIC),

	    RB_Armadillo_ThTick_Rolling, // behavior
	    "armadillo",                 // debug name
	    0                            // thread relative
	);

	if (t == 0)
		return;
	inst->thread = t;
	t->inst = inst;
	t->funcThCollide = (void (*)(struct Thread *))RB_Armadillo_ThCollide;

	// rolling animation
	inst->animIndex = 1;

	armObj = ((struct Armadillo *)t->object);
	armObj->timeRolling = 0;
	armObj->numFramesSpinning = 0;
	armObj->timeAtEdge = 0;

	CTR_MatrixToRot(&rot, &inst->matrix, 0x11);
	armObj->rotCurr.x = rot.vy;
	armObj->rotCurr.y = rot.vx;
	armObj->rotCurr.z = rot.vz;

	armObj->rotDesired.y = (armObj->rotCurr.y + 0x800) & 0xfff;

	armObj->distFromSpawn = 0;
	armObj->spawnPosX = inst->matrix.t[0];
	armObj->spawnPosZ = inst->matrix.t[2];
	armObj->direction = 0;

	armObj->velX = inst->matrix.m[0][2] >> 7;
	armObj->velZ = inst->matrix.m[2][2] >> 7;

	if (sdata->gGT->level1->ptrSpawnType1->count <= 0)
		return;

	// puts armadillos on separate cycles
	pointers = ST1_GETPOINTERS(sdata->gGT->level1->ptrSpawnType1);
	metaArray = (s16 *)pointers[ST1_SPAWN];
	armObj->timeAtEdge = metaArray[inst->name[strlen(inst->name) - 1] - '0'];
}
