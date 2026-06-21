#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b7b8c-0x800b81e8.

struct ParticleEmitter emSet_OrcaSplash[7] = {
    [0] =
        {
            .flags = 1,
            .initOffset = 0xC,
            .InitTypes.FuncInit =
                {
                    .particle_funcPtr = 0,
                    .particle_colorFlags = 0x4A0,
                    .particle_lifespan = 0xF,
                    .particle_Type = 0,
                },
        },

    [1] =
        {
            .flags = 0x12,
            .initOffset = 0,
            .InitTypes.AxisInit =
                {
                    .baseValue = {.startVal = 0, .velocity = -0x1400, .accel = 0},
                    .rngSeed = {.startVal = 0, .velocity = 0x2800, .accel = 0},
                },
        },

    [2] =
        {
            .flags = 0x12,
            .initOffset = 2,
            .InitTypes.AxisInit =
                {
                    .baseValue = {.startVal = 0, .velocity = -0x1400, .accel = 0},
                    .rngSeed = {.startVal = 0, .velocity = 0x2800, .accel = 0},
                },
        },

    [3] =
        {
            .flags = 0x3E,
            .initOffset = 1,
            .InitTypes.AxisInit =
                {
                    .baseValue = {.startVal = 0, .velocity = 0x4C00, .accel = -0xDAC},
                    .rngSeed = {.startVal = 0x1000, .velocity = 0x4000, .accel = -0xC8},
                },
        },

    [4] =
        {
            .flags = 0xB,
            .initOffset = 5,
            .InitTypes.AxisInit =
                {
                    .baseValue = {.startVal = 0x3E8, .velocity = 0x28, .accel = 0x64},
                },
        },

    [5] =
        {
            .flags = 3,
            .initOffset = 7,
            .InitTypes.AxisInit =
                {
                    .baseValue = {.startVal = 0x8000, .velocity = -0x700, .accel = 0},
                },
        },

    [6] = {0},
};

static void RB_Orca_SpawnSplash(struct Instance *orcaInst)
{
	struct Particle *particle;
	struct GameTracker *gGT;
	int i;

	gGT = sdata->gGT;

	for (i = 0; i < 0xF; i++)
	{
		particle = Particle_Init(0, gGT->iconGroup[1], &emSet_OrcaSplash[0]);

		if (particle == 0)
			continue;

		particle->axis[0].startVal += (orcaInst->matrix.t[0] << 8) + (particle->axis[0].velocity << 4);
		particle->axis[1].startVal += (orcaInst->matrix.t[1] << 8) + (particle->axis[1].velocity << 1);
		particle->unk1A = 0x1000;
		particle->axis[2].startVal += (orcaInst->matrix.t[2] << 8) + (particle->axis[2].velocity << 4);
	}
}

void RB_Orca_ThTick(struct Thread *t)
{
	struct Orca *orcaObj;
	struct Instance *orcaInst;
	struct GameTracker *gGT;
	int numFrames;
	int pathFrame;
	int denominator;
	int nextFrame;
	s16 direction;

	orcaObj = (struct Orca *)t->object;
	orcaInst = t->inst;

	if (orcaObj->cooldown != 0)
	{
		orcaObj->cooldown--;

		if ((u16)orcaObj->cooldown != 0)
			return;

		orcaInst->flags &= ~HIDE_MODEL;
		return;
	}

	numFrames = orcaObj->numFrames;
	pathFrame = orcaObj->animIndex;

	if ((numFrames - 0x14) < pathFrame)
		pathFrame = numFrames - 0x14;

	direction = orcaObj->direction;
	denominator = numFrames - 0x17;

	if (direction == 0)
	{
		if ((numFrames - 0x1A) < (s16)pathFrame)
			pathFrame = numFrames - 0x1A;
	}
	else
	{
		pathFrame -= 3;
	}

	if ((s16)pathFrame < 0)
		pathFrame = 0;

	orcaInst->matrix.t[0] = orcaObj->startPos.x - ((pathFrame * orcaObj->midpoint[0]) / denominator);
	orcaInst->matrix.t[1] = orcaObj->startPos.y - ((pathFrame * orcaObj->midpoint[1]) / denominator);
	orcaInst->matrix.t[2] = orcaObj->startPos.z - ((pathFrame * orcaObj->midpoint[2]) / denominator);

	nextFrame = orcaInst->animFrame + 1;

	if (nextFrame < orcaObj->numFrames)
	{
		gGT = sdata->gGT;

		if ((gGT->numPlyrCurrGame < 2) && ((nextFrame == 5) || (nextFrame == 0x31)))
			RB_Orca_SpawnSplash(orcaInst);

		orcaInst->animFrame = nextFrame;

		if (direction == 0)
			orcaObj->animIndex--;
		else
			orcaObj->animIndex++;

		return;
	}

	orcaInst->flags |= HIDE_MODEL;
	orcaObj->cooldown = 0x5A;
	orcaInst->animFrame = 0;
	orcaObj->direction = direction ^ 1;
	orcaObj->instDefRot.y += 0x800;
	ConvertRotToMatrix(&orcaInst->matrix, &orcaObj->instDefRot);
}

int RB_Orca_ThCollide(struct Thread *orcaThread, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	(void)orcaThread;
	(void)driverTh;
	(void)funcThCollide;

	return sps->Input1.modelID == DYNAMIC_PLAYER;
}

void RB_Orca_LInB(struct Instance *inst)
{
	struct Orca *orcaObj;
	struct SpawnType2 *spawnType2;
	struct Thread *t;
	void **pointers;
	s16 *metaArray;
	int orcaID;

	if (inst->thread != 0)
		return;

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Orca), NONE, SMALL, STATIC),

	    RB_Orca_ThTick, // behavior
	    "orca",         // debug name
	    0               // thread relative
	);

	if (t == 0)
		return;

	inst->thread = t;
	t->funcThCollide = (void (*)(struct Thread *))RB_Orca_ThCollide;
	t->inst = inst;

	inst->scale.x = 0xC00;
	inst->scale.y = 0xC00;
	inst->scale.z = 0xC00;
	inst->flags |= DRAW_HUGE;

	orcaObj = (struct Orca *)t->object;
	orcaObj->animIndex = -10;
	orcaObj->direction = 1;
	orcaObj->instDefRot.x = inst->instDef->rot.x;
	orcaObj->instDefRot.y = inst->instDef->rot.y;
	orcaObj->instDefRot.z = inst->instDef->rot.z;

	orcaID = inst->name[strlen(inst->name) - 1] - '0';
	orcaObj->orcaID = orcaID;

	if (sdata->gGT->level1->numSpawnType2 != 0)
	{
		spawnType2 = &sdata->gGT->level1->ptrSpawnType2[orcaID + 4];

		*(int *)&orcaObj->startPos.x = *(int *)&spawnType2->posCoords[0];
		orcaObj->startPos.z = spawnType2->posCoords[2];

		*(int *)&orcaObj->endPos.x = *(int *)&spawnType2->posCoords[3];
		orcaObj->endPos.z = spawnType2->posCoords[5];
	}

	orcaObj->midpoint[0] = orcaObj->startPos.x - orcaObj->endPos.x;
	orcaObj->midpoint[1] = orcaObj->startPos.y - orcaObj->endPos.y;
	orcaObj->midpoint[2] = orcaObj->startPos.z - orcaObj->endPos.z;

	orcaObj->numFrames = INSTANCE_GetNumAnimFrames(inst, 0);

	if (sdata->gGT->level1->ptrSpawnType1->count <= 0)
		return;

	pointers = ST1_GETPOINTERS(sdata->gGT->level1->ptrSpawnType1);
	metaArray = (s16 *)pointers[ST1_SPAWN];
	orcaObj->cooldown = metaArray[orcaObj->orcaID];

	if (orcaObj->cooldown != 0)
		inst->flags |= HIDE_MODEL;
}
