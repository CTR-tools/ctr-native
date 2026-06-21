#include <common.h>

struct HitboxDesc fjBoxDesc = {.inst = (struct Instance *)0,
                               .thread = (struct Thread *)0,
                               .bucket = (struct Thread *)0,
                               .bbox = {.min = {0xFFC0, 0xFFC0, 0}, .max = {0x40, 0x80, 0x140}},
                               .threadHit = (struct Thread *)0,
                               .funcThCollide = (void *)0};

SVec3 fjLightDir = {0x8B8, 0xD6A, 0};

struct ParticleEmitter emSet_fjHeat[0xb] = {[0] =
                                                {
                                                    .flags = 1,

                                                    // invalid axis, assume FuncInit
                                                    .initOffset = 0xC,

                                                    .InitTypes.FuncInit =
                                                        {
                                                            .particle_funcPtr = 0,
                                                            .particle_colorFlags = 0xA1,
                                                            .particle_lifespan = 6,
                                                            .particle_Type = 1,
                                                        }

                                                    // last 0x10 bytes are blank
                                                },

                                            [1] =
                                                {
                                                    .flags = 0x1b,

                                                    // posX
                                                    .initOffset = 0,

                                                    .InitTypes.AxisInit.baseValue.startVal = -0x40,
                                                    .InitTypes.AxisInit.baseValue.velocity = -0x320,

                                                    .InitTypes.AxisInit.rngSeed.startVal = 0x80,
                                                    .InitTypes.AxisInit.rngSeed.velocity = 0x640,
                                                },

                                            [2] =
                                                {
                                                    .flags = 0x1b,

                                                    // posZ
                                                    .initOffset = 2,

                                                    .InitTypes.AxisInit.baseValue.startVal = -0x40,
                                                    .InitTypes.AxisInit.baseValue.velocity = -0x320,

                                                    .InitTypes.AxisInit.rngSeed.startVal = 0x80,
                                                    .InitTypes.AxisInit.rngSeed.velocity = 0x640,
                                                },

                                            [3] =
                                                {
                                                    .flags = 5,

                                                    // posY
                                                    .initOffset = 1,

                                                    .InitTypes.AxisInit.baseValue.startVal = -0x40,
                                                    .InitTypes.AxisInit.baseValue.accel = 1,
                                                },

                                            [4] =
                                                {
                                                    .flags = 0xb,

                                                    .initOffset = 3,

                                                    .InitTypes.AxisInit.baseValue.startVal = 0x1200,
                                                    .InitTypes.AxisInit.baseValue.velocity = 1,

                                                    .InitTypes.AxisInit.rngSeed.startVal = 0x1000,
                                                },

                                            [5] =
                                                {
                                                    .flags = 3,

                                                    .initOffset = 4,

                                                    .InitTypes.AxisInit.baseValue.startVal = 1,
                                                    .InitTypes.AxisInit.baseValue.velocity = 1,
                                                },

                                            [6] =
                                                {
                                                    .flags = 3,

                                                    .initOffset = 5,

                                                    .InitTypes.AxisInit.baseValue.startVal = 1,
                                                    .InitTypes.AxisInit.baseValue.velocity = 1,
                                                },

                                            [7] =
                                                {
                                                    .flags = 1,

                                                    .initOffset = 7,

                                                    .InitTypes.AxisInit.baseValue.startVal = 0x8000,
                                                },

                                            [8] =
                                                {
                                                    .flags = 1,

                                                    .initOffset = 8,

                                                    .InitTypes.AxisInit.baseValue.startVal = 0x8000,
                                                },

                                            [9] =
                                                {
                                                    .flags = 1,

                                                    .initOffset = 9,

                                                    .InitTypes.AxisInit.baseValue.startVal = 0x8000,
                                                },

                                            // null terminator
                                            {}};

struct ParticleEmitter emSet_fjFire[0x8] = {[0] =
                                                {
                                                    .flags = 1,

                                                    // invalid axis, assume FuncInit
                                                    .initOffset = 0xC,

                                                    .InitTypes.FuncInit =
                                                        {
                                                            .particle_funcPtr = 0,
                                                            .particle_colorFlags = 0xA1,
                                                            .particle_lifespan = 7,
                                                            .particle_Type = 0,
                                                        }

                                                    // last 0x10 bytes are blank
                                                },

                                            [1] =
                                                {
                                                    .flags = 0x1b,

                                                    // posX
                                                    .initOffset = 0,

                                                    .InitTypes.AxisInit.baseValue.startVal = -0x40,
                                                    .InitTypes.AxisInit.baseValue.velocity = -0x320,

                                                    .InitTypes.AxisInit.rngSeed.startVal = 0x80,
                                                    .InitTypes.AxisInit.rngSeed.velocity = 0x640,
                                                },

                                            [2] =
                                                {
                                                    .flags = 0x1b,

                                                    // posZ
                                                    .initOffset = 2,

                                                    .InitTypes.AxisInit.baseValue.startVal = -0x40,
                                                    .InitTypes.AxisInit.baseValue.velocity = -0x320,

                                                    .InitTypes.AxisInit.rngSeed.startVal = 0x80,
                                                    .InitTypes.AxisInit.rngSeed.velocity = 0x640,
                                                },

                                            [3] =
                                                {
                                                    .flags = 5,

                                                    // posY
                                                    .initOffset = 1,

                                                    .InitTypes.AxisInit.baseValue.startVal = -0x40,
                                                    .InitTypes.AxisInit.baseValue.accel = 1,
                                                },

                                            [4] =
                                                {
                                                    .flags = 0x1a,

                                                    .initOffset = 4,

                                                    .InitTypes.AxisInit.baseValue.velocity = 0x10,

                                                    .InitTypes.AxisInit.rngSeed.startVal = 0x800,
                                                    .InitTypes.AxisInit.rngSeed.velocity = 0x10,
                                                },

                                            [5] =
                                                {
                                                    .flags = 0xb,

                                                    .initOffset = 5,

                                                    .InitTypes.AxisInit.baseValue.startVal = 0x4e2,
                                                    .InitTypes.AxisInit.baseValue.velocity = 0x100,

                                                    .InitTypes.AxisInit.rngSeed.startVal = 0x80,
                                                },

                                            [6] =
                                                {
                                                    .flags = 0xd,

                                                    .initOffset = 7,

                                                    .InitTypes.AxisInit.baseValue.startVal = 0x8000,
                                                    .InitTypes.AxisInit.baseValue.accel = -0x800,

                                                    .InitTypes.AxisInit.rngSeed.startVal = 0x5f00,
                                                },

                                            // null terminator
                                            {}};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b64c0-0x800b6728.
void RB_FlameJet_Particles(struct Instance *inst, struct FlameJet *fjObj)
{
	int result;
	struct Particle *particle1;
	struct Particle *particle2;
	struct GameTracker *gGT = sdata->gGT;

	particle1 = Particle_Init(0, gGT->iconGroup[0xA], &emSet_fjFire[0]);

	// fire particle
	if (particle1 != 0)
	{
		particle1->axis[0].startVal += (inst->matrix.t[0]) * 0x100;
		particle1->axis[1].startVal += (inst->matrix.t[1] + 0x32) * 0x100;
		particle1->axis[2].startVal += (inst->matrix.t[2]) * 0x100;

		particle1->axis[0].velocity = (s16)fjObj->dirX;
		particle1->axis[1].velocity = 0;
		particle1->axis[2].velocity = (s16)fjObj->dirZ;

		result = RngDeadCoed((u32 *)&gGT->deadcoed_struct);
		result = MATH_Sin(gGT->timer * 0x100 + (result >> 0x18) & 0xfff);
		particle1->axis[1].accel = result >> 4;

		particle1->unk1A = 0x1e00;
		particle1->otIndexOffset = inst->depthBiasNormal - 1;

		if ((gGT->timer & 1) != 0)
		{
			particle1->axis[4].startVal = -particle1->axis[4].startVal;
			particle1->axis[4].velocity = -particle1->axis[4].velocity;
		}
	}

	// heat particle is 1P only
	if (gGT->numPlyrCurrGame > 1)
		return;

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail would read PS1 null-space if the fire particle allocation failed.
	if (particle1 == NULL)
		return;
#endif

	particle2 = Particle_Init(0, (struct IconGroup *)gGT->ptrSparkle, &emSet_fjHeat[0]);

	// heat particle
	if (particle2 != 0)
	{
		particle2->axis[0].startVal += particle1->axis[0].startVal;
		particle2->axis[1].startVal += particle1->axis[1].startVal + 0x1000;
		particle2->axis[2].startVal += particle1->axis[2].startVal;

		// register sharing
		result = particle2->axis[3].startVal;

		particle2->axis[4].startVal = result - 0x400;
		particle2->axis[5].startVal = result - 0x600;

		particle2->axis[0].velocity = (s16)fjObj->dirX;
		particle2->axis[1].velocity = 0;
		particle2->axis[2].velocity = (s16)fjObj->dirZ;

		particle2->axis[3].velocity = (0x4a00 - particle2->axis[3].startVal) / 7;
		particle2->axis[4].velocity = (0x4600 - particle2->axis[4].startVal) / 7;
		particle2->axis[5].velocity = (0x4400 - particle2->axis[5].startVal) / 7;

		particle2->axis[1].accel = particle1->axis[1].accel;

		particle2->unk1A = 0x1e00;
		particle2->otIndexOffset = inst->depthBiasNormal;
	}
}

void RB_FlameJet_ThTick(struct Thread *t)
{
	struct Instance *fjInst;
	struct FlameJet *fjObj;

	struct Instance *hitInst;
	struct Driver *hitDriver;

	struct GameTracker *gGT = sdata->gGT;

	fjInst = t->inst;
	fjObj = (struct FlameJet *)t->object;

	// NOTE(aalhendi): ASM-verified audio/lifecycle path for NTSC-U 926 0x800b6728-0x800b6938.
	if (fjObj->cooldown != 0)
	{
		fjObj->cooldown--;
		return;
	}

	// in first 45 frames (1.5s)
	if (fjObj->cycleTimer < 0x2d)
	{
		PlaySound3D_Flags((u32 *)&fjObj->audioPtr, 0x68, fjInst);

		// [unused variable?]
		fjObj->unk += 0x100;

		RB_FlameJet_Particles(fjInst, fjObj);

		// === Collision ===

		fjBoxDesc.inst = fjInst;
		fjBoxDesc.thread = t;

		fjBoxDesc.bucket = gGT->threadBuckets[PLAYER].thread;
		hitInst = LinkedCollide_Hitbox_Desc(&fjBoxDesc);

		// no PLAYER
		if (hitInst == 0)
		{
			fjBoxDesc.bucket = gGT->threadBuckets[ROBOT].thread;
			hitInst = LinkedCollide_Hitbox_Desc(&fjBoxDesc);

			// no ROBOT
			if (hitInst == 0)
			{
				fjBoxDesc.bucket = gGT->threadBuckets[MINE].thread;
				hitInst = LinkedCollide_Hitbox_Desc(&fjBoxDesc);

				// hit MINE
				if (hitInst != 0)
				{
					struct Thread *threadHit = hitInst->thread;

					fjBoxDesc.threadHit = threadHit;
					fjBoxDesc.funcThCollide = threadHit->funcThCollide;

					// optimization
					RB_Hazard_ThCollide_Generic(threadHit);
				}

				// if no player or robot was hit,
				// regardless if mine was hit or not
				goto EndFjThTick;
			}
		}

		// === Hit Player or Robot ===

		// get driver from instance
		hitDriver = (struct Driver *)hitInst->thread->object;
		RB_Hazard_HurtDriver(hitDriver, 4, 0, 0);
	}

	// on 45th frame (1.5s)
	else if (fjObj->cycleTimer == 0x2d)
	{
		if (fjObj->audioPtr != 0)
			OtherFX_RecycleMute(&fjObj->audioPtr);
	}

	// repeat cycle every 105 (3.5s)
	else if (fjObj->cycleTimer > 0x69)
		fjObj->cycleTimer = 0;

EndFjThTick:

	fjObj->cycleTimer++;
	Vector_SpecLightNoSpin3D(fjInst, &fjInst->instDef->rot.x, &fjLightDir);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b6938-0x800b6d58.
void RB_FlameJet_LInB(struct Instance *inst)
{
	int fjID;
	struct Thread *t;
	struct FlameJet *fjObj;
	s16 *metaArray;

	// color
	inst->colorRGBA = 0xdca6000;

	// yellow
	inst->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);

	if (inst->thread != 0)
		return;

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct FlameJet), NONE, SMALL, STATIC),

	    RB_FlameJet_ThTick, // behavior
	    "flamejet",         // debug name
	    0                   // thread relative
	);

	if (t == 0)
		return;
	inst->thread = t;
	t->inst = inst;

	fjObj = (struct FlameJet *)t->object;
	fjObj->cycleTimer = 0;
	fjObj->cooldown = 0;
	fjObj->dirX = inst->matrix.m[0][2] * -0x4b >> 5;
	fjObj->dirZ = inst->matrix.m[2][2] * 0x4b >> 5;
	fjObj->audioPtr = 0;

	fjBoxDesc.bbox.min.x = -0x40;
	fjBoxDesc.bbox.min.y = -0x40;
	fjBoxDesc.bbox.min.z = 0;
	fjBoxDesc.bbox.max.x = 0x40;
	fjBoxDesc.bbox.max.y = 0x80;
	fjBoxDesc.bbox.max.z = 0x140;

	if (sdata->gGT->level1->ptrSpawnType1->count > 0)
	{
		// put on separate cycles
		void **pointers = ST1_GETPOINTERS(sdata->gGT->level1->ptrSpawnType1);
		metaArray = (s16 *)pointers[ST1_SPAWN];

		fjID = inst->name[strlen(inst->name) - 1] - '0';
		fjObj->cooldown = metaArray[fjID];
	}
}
