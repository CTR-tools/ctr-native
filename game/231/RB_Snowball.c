#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b92ac-0x800b95fc.

void RB_Snowball_ThTick(struct Thread *t)
{
	struct Instance *snowInst;
	struct Snowball *snowObj;

	int modelID;
	int soundID;
	int baseShort;
	struct SpawnType2 *ptrSpawnType2;

	snowInst = t->inst;
	snowObj = (struct Snowball *)t->object;

	modelID = snowInst->model->id;

	if (sdata->gGT->level1->numSpawnType2_PosRot != 0)
	{
		ptrSpawnType2 = &sdata->gGT->level1->ptrSpawnType2_PosRot[snowObj->snowID];

		// Retail checks DYNAMIC_SNOWBALL, but Blizzard Bluff uses TEMP_SNOWBALL.
		if (modelID == DYNAMIC_SNOWBALL)
		{
			// snowball roll
			soundID = 0x73;
			PlaySound3D_Flags(&snowObj->audioPtr, soundID, snowInst);
		}

		// sewer speedway barrel
		else if (modelID == DYNAMIC_BARREL)
		{
			// barrel roll
			soundID = 0x74;
			PlaySound3D_Flags(&snowObj->audioPtr, soundID, snowInst);
		}

		baseShort = snowObj->pointIndex;
		if (baseShort > snowObj->numPoints)
			baseShort = (snowObj->numPoints * 2) - baseShort;

		baseShort *= 6;

		ConvertRotToMatrix(&snowInst->matrix, &ptrSpawnType2->posCoords[baseShort + 3]);

		snowInst->matrix.t[0] = ptrSpawnType2->posCoords[baseShort + 0];
		snowInst->matrix.t[1] = ptrSpawnType2->posCoords[baseShort + 1];
		snowInst->matrix.t[2] = ptrSpawnType2->posCoords[baseShort + 2];

		RB_Minecart_CheckColl(snowInst, t);
	}

	snowObj->pointIndex = (snowObj->pointIndex + 1) % (snowObj->numPoints * 2);
}

void RB_Snowball_LInB(struct Instance *inst)
{
	struct Snowball *snowObj;
	struct Thread *t;

	if (inst->thread != 0)
		return;

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Snowball), NONE, SMALL, STATIC),

	    RB_Snowball_ThTick, // behavior
	    "snowball",         // debug name
	    0                   // thread relative
	);

	if (t == 0)
		return;
	inst->thread = t;
	t->inst = inst;

	snowObj = ((struct Snowball *)t->object);
	snowObj->rot_unused.x = 0;

	snowObj->snowID = inst->name[strlen(inst->name) - 1] - '0';

	snowObj->numPoints = sdata->gGT->level1->ptrSpawnType2_PosRot[snowObj->snowID].numCoords - 1;

	inst->scale.x = 0x1000;
	inst->scale.y = 0x1000;
	inst->scale.z = 0x1000;

	snowObj->rot_unused.y = inst->matrix.m[0][2] >> 2;
	snowObj->rot_unused.z = inst->matrix.m[2][2] >> 2;
	snowObj->audioPtr = 0;
}
