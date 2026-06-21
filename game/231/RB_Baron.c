#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3120-0x800b37d4.

static void RB_Baron_SetPathFrame(struct Instance *inst, struct SpawnType2 *spawn, int pointIndex, int offsetX, int offsetZ, int flipRotX)
{
	s16 *coord;
	SVec3 rot;

	coord = &spawn->posCoords[pointIndex * 6];

	rot.x = coord[3];
	rot.y = coord[4];
	rot.z = coord[5];

	if (flipRotX)
		rot.x = -rot.x;

	ConvertRotToMatrix(&inst->matrix, &rot);

	inst->matrix.t[0] = coord[0] + offsetX;
	inst->matrix.t[1] = coord[1];
	inst->matrix.t[2] = coord[2] + offsetZ;
}

void RB_Baron_ThTick(struct Thread *t)
{
	struct Instance *baronInst;
	struct Baron *baronObj;
	struct Level *level;
	struct GameTracker *gGT;
	struct SpawnType2 *spawn;
	int pointIndex;
	int modelID;

	struct Driver *hitDriver;
	struct Instance *hitInst;

	baronInst = t->inst;
	baronObj = (struct Baron *)t->object;
	gGT = sdata->gGT;
	level = gGT->level1;

	if ((baronInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(baronInst, 0))
		baronInst->animFrame++;
	else
		baronInst->animFrame = 0;

	if (level->numSpawnType2_PosRot == 0)
		return;

	spawn = &level->ptrSpawnType2_PosRot[0];
	pointIndex = (baronObj->pointIndex + 1) % spawn->numCoords;
	baronObj->pointIndex = pointIndex;
	modelID = baronInst->model->id;

	if (modelID == DYNAMIC_DRUM)
	{
		if (pointIndex == 0x10)
			PlaySound3D(0xc, baronInst);

		if (pointIndex < 0x11)
			OtherFX_RecycleMute(&baronObj->soundID_flags);
		else
			PlaySound3D_Flags(&baronObj->soundID_flags, 0x74, baronInst);

		RB_Baron_SetPathFrame(baronInst, spawn, pointIndex, 0x111, -0x110, 0);
	}
	else
	{
		RB_Baron_SetPathFrame(baronInst, spawn, pointIndex, 0, 0, 1);
	}

	if (baronObj->otherInst != 0)
	{
		pointIndex = (baronObj->pointIndex + 0x78) % spawn->numCoords;
		RB_Baron_SetPathFrame(baronObj->otherInst, spawn, pointIndex, 0x21f, -0x21f, 1);
	}

	if (modelID == DYNAMIC_DRUM)
	{
		hitInst = RB_Hazard_CollideWithDrivers(baronInst, 0, 0x19000, 0);
		if (hitInst != 0)
		{
			hitDriver = (struct Driver *)hitInst->thread->object;
			RB_Hazard_HurtDriver(hitDriver, 3, 0, 0);
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b37d4-0x800b38dc.

void RB_Baron_LInB(struct Instance *inst)
{
	struct Baron *baronObj;
	struct Thread *t;
	s16 pointIndex;

	if (inst->thread != 0)
		return;

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Baron), NONE, SMALL, STATIC),

	    RB_Baron_ThTick, // behavior
	    "baron",         // debug name
	    0                // thread relative
	);

	if (t == 0)
		return;
	inst->thread = t;
	t->inst = inst;

	inst->scale.x = 0x1000;
	inst->scale.y = 0x1000;
	inst->scale.z = 0x1000;

	baronObj = ((struct Baron *)t->object);
	pointIndex = 1;

	if (inst->name[strlen(inst->name) - 1] == '0')
		pointIndex = sdata->gGT->level1->ptrSpawnType2->numCoords / 2;

	baronObj->pointIndex = pointIndex;
	baronObj->unk1A = 4;
	baronObj->unk22 = 0x18;
	baronObj->unk06 = 0;
	baronObj->otherInst = 0;

	if (inst->model->id == DYNAMIC_VONLABASS)
		inst->flags |= HIDE_MODEL;

	baronObj->soundID_flags = 0;
}
