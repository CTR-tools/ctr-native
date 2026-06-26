#include <common.h>

// budget: 2584

enum PlantAnim
{
	PlantAnim_Rest,
	PlantAnim_TransitionRestHungry,
	PlantAnim_Hungry,
	PlantAnim_GrabDriver,
	PlantAnim_GrabMine, // unused
	PlantAnim_StartEat,
	PlantAnim_Chew,
	PlantAnim_Spit
};

struct HitboxDesc plantBoxDesc = {.inst = (struct Instance *)0,
                                  .thread = (struct Thread *)0,
                                  .bucket = (struct Thread *)0,
                                  .bbox = {.min = {0xFFC0, 0xFFC0, 0}, .max = {0x40, 0x80, 0x1E0}},
                                  .threadHit = (struct Thread *)0,
                                  .funcThCollide = (void *)0};

extern struct ParticleEmitter emSet_PlantTires[8];

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b81e8-0x800b84f0.
void RB_Plant_ThTick_Eat(struct Thread *t)
{
	int i;
	struct Particle *particle;
	struct Instance *plantInst;
	struct Plant *plantObj;

	plantInst = t->inst;
	plantObj = (struct Plant *)t->object;

	if (plantInst->animIndex == PlantAnim_StartEat)
	{
		// if animation is not over
		if ((plantInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(plantInst, PlantAnim_StartEat))
		{
			// increment frame
			plantInst->animFrame = plantInst->animFrame + 1;
		}

		// if animation is over
		else
		{
			plantInst->animFrame = 0;
			plantInst->animIndex = PlantAnim_Chew;

		PlayChewSound:

			if (plantObj->boolEatingPlayer != 0)
			{
				OtherFX_Play(0x6e, 0);
			}
		}
	}

	else if (plantInst->animIndex == PlantAnim_Chew)
	{
		// if animation is not over
		if ((plantInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(plantInst, PlantAnim_Chew))
		{
			// increment frame
			plantInst->animFrame = plantInst->animFrame + 1;

			// last frame
			if (plantInst->animFrame == 0xf)
			{
				goto PlayChewSound;
			}
		}

		// if animation is done
		else
		{
			// reset animation
			plantInst->animFrame = 0;

			// After 4 cycles, transition to rest
			plantObj->cycleCount++;
			if (plantObj->cycleCount == 1)
			{
				plantObj->cycleCount = 0;

				plantInst->animFrame = 0;
				plantInst->animIndex = PlantAnim_Spit;
			}
		}
	}

	else if (plantInst->animIndex == PlantAnim_Spit)
	{
		// if animation is not over
		if ((plantInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(plantInst, PlantAnim_Spit))
		{
			// increment frame
			plantInst->animFrame = plantInst->animFrame + 1;

			// last frame
			if (plantInst->animFrame == 0x19)
			{
				if (plantObj->boolEatingPlayer != 0)
				{
					// play PlantSpit sound
					OtherFX_Play(0x6f, 0);
				}

				for (i = 0; i < 4; i++)
				{
					// spit tires
					particle = Particle_Init(0, sdata->gGT->iconGroup[0], &emSet_PlantTires[0]);

					if (particle == 0)
					{
						continue;
					}

					particle->funcPtr = Particle_FuncPtr_SpitTire;
					particle->plantInst = plantInst;

					particle->axis[0].startVal += (plantInst->matrix.t[0] + (plantInst->matrix.m[0][2] * 9 >> 7)) * 0x100;

					particle->axis[1].startVal += (plantInst->matrix.t[1] + 0x20) * 0x100;

					particle->axis[2].startVal += (plantInst->matrix.t[2] + (plantInst->matrix.m[2][2] * 9 >> 7)) * 0x100;

					particle->axis[0].velocity += (
					                                  // 6 - 26
					                                  (((MixRNG_Scramble() % 10) + 0x10) * plantInst->matrix.m[0][2]) >> 0xC) *
					                              0x100;

					// axis[1].velocity is untouched

					particle->axis[2].velocity += (
					                                  // 6 - 26
					                                  (((MixRNG_Scramble() % 10) + 0x10) * plantInst->matrix.m[2][2]) >> 0xC) *
					                              0x100;
				}
			}
		}

		// animation done
		else
		{
			plantInst->animFrame = 0;
			plantInst->animIndex = PlantAnim_Rest;

			plantObj->boolEatingPlayer = 0;
			ThTick_SetAndExec(t, RB_Plant_ThTick_Rest);
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b84f0-0x800b8650.
void RB_Plant_ThTick_Grab(struct Thread *t)
{
	struct Instance *plantInst;
	struct HitboxDesc plantBoxDescLocal;

	struct Instance *hitInst;
	struct Thread *threadHit;
	struct GameTracker *gGT = sdata->gGT;

	plantInst = t->inst;

	plantBoxDescLocal = plantBoxDesc;
	plantBoxDescLocal.inst = plantInst;
	plantBoxDescLocal.thread = t;

	if (plantInst->animIndex == PlantAnim_GrabDriver)
	{
		// if animation is not over
		if ((plantInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(plantInst, PlantAnim_GrabDriver))
		{
			// increment frame
			plantInst->animFrame = plantInst->animFrame + 1;

			plantBoxDescLocal.bucket = gGT->threadBuckets[MINE].thread;
			hitInst = LinkedCollide_Hitbox_Desc(&plantBoxDescLocal);

			if (hitInst != 0)
			{
				threadHit = hitInst->thread;

				plantBoxDescLocal.threadHit = threadHit;
				plantBoxDescLocal.funcThCollide = threadHit->funcThCollide;

				RB_Hazard_ThCollide_Generic_Alt(&threadHit);
			}
		}

		else
		{
			plantInst->animFrame = 0;
			plantInst->animIndex = PlantAnim_StartEat;
			ThTick_SetAndExec(t, RB_Plant_ThTick_Eat);
		}
	}

	else if (plantInst->animIndex == PlantAnim_GrabMine)
	{
		if ((plantInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(plantInst, PlantAnim_GrabMine))
		{
			plantInst->animFrame = plantInst->animFrame + 1;
		}
		else
		{
			plantInst->animFrame = 0;
			plantInst->animIndex = PlantAnim_Rest;
			ThTick_SetAndExec(t, RB_Plant_ThTick_Rest);
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b8650-0x800b86b4.
void RB_Plant_ThTick_Transition_HungryToRest(struct Thread *t)
{
	struct Instance *plantInst = t->inst;

	// if animation is not over (backwards)
	if ((plantInst->animFrame - 1) > 0)
	{
		// increment frame
		plantInst->animFrame = plantInst->animFrame - 1;
	}

	// animation is done
	else
	{
		// reset animation
		plantInst->animFrame = 0;

		plantInst->animIndex = PlantAnim_Rest;
		ThTick_SetAndExec(t, RB_Plant_ThTick_Rest);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b86b4-0x800b88a8.
void RB_Plant_ThTick_Hungry(struct Thread *t)
{
	struct Instance *plantInst;
	struct Plant *plantObj;
	struct HitboxDesc plantBoxDescLocal;

	struct Instance *hitInst;
	struct Driver *hitDriver;

	struct GameTracker *gGT = sdata->gGT;

	plantInst = t->inst;
	plantObj = (struct Plant *)t->object;
	plantBoxDescLocal = plantBoxDesc;

	// if animIndex == PlantAnim_Hungry

	// if animation is not over
	if ((plantInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(plantInst, PlantAnim_Hungry))
	{
		// increment frame
		plantInst->animFrame = plantInst->animFrame + 1;
	}

	// if animation is done
	else
	{
		// reset animation
		plantInst->animFrame = 0;

		// After 4 cycles, transition to rest
		plantObj->cycleCount++;
		if (plantObj->cycleCount == 4)
		{
			plantObj->cycleCount = 0;

			// end of animation
			plantInst->animFrame = INSTANCE_GetNumAnimFrames(plantInst, PlantAnim_TransitionRestHungry);

			plantInst->animIndex = PlantAnim_TransitionRestHungry;
			ThTick_SetAndExec(t, RB_Plant_ThTick_Transition_HungryToRest);
			return;
		}
	}

	// === collision ===

	plantBoxDescLocal.inst = plantInst;
	plantBoxDescLocal.thread = t;

	plantBoxDescLocal.bucket = gGT->threadBuckets[PLAYER].thread;
	hitInst = LinkedCollide_Hitbox_Desc(&plantBoxDescLocal);

	if (hitInst != 0)
	{
		// get driver from instance
		hitDriver = (struct Driver *)hitInst->thread->object;

		// attempt to harm driver (eat)
		int didHit = RB_Hazard_HurtDriver(hitDriver, 5, 0, 0);

		if (didHit != 0)
		{
			// play PlantGrab sound
			OtherFX_Play(0x6d, 0);
			plantObj->boolEatingPlayer = 1;

		EatDriver:

			plantInst->animFrame = 0;
			plantInst->animIndex = PlantAnim_GrabDriver;

			plantObj->cycleCount = 0;
			hitDriver->plantEatingMe = t;

			ThTick_SetAndExec(t, RB_Plant_ThTick_Grab);
		}

		return;
	}

	// === did not collide with PLAYER ===

	// bosses are immune
	if ((gGT->gameMode1 & ADVENTURE_BOSS) != 0)
	{
		return;
	}

	plantBoxDescLocal.bucket = gGT->threadBuckets[ROBOT].thread;
	hitInst = LinkedCollide_Hitbox_Desc(&plantBoxDescLocal);

	if (hitInst != 0)
	{
		// get driver from instance
		hitDriver = (struct Driver *)hitInst->thread->object;

		RB_Hazard_HurtDriver(hitDriver, 5, 0, 0);

		plantObj->boolEatingPlayer = 0;

		goto EatDriver;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b88a8-0x800b89a4.
void RB_Plant_ThTick_Rest(struct Thread *t)
{
	struct Instance *plantInst;
	struct Plant *plantObj;

	plantInst = t->inst;
	plantObj = (struct Plant *)t->object;

	if (plantObj->cooldown != 0)
	{
		plantObj->cooldown--;
		return;
	}

	if (plantInst->animIndex == PlantAnim_Rest)
	{
		// if animation is not over
		if ((plantInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(plantInst, PlantAnim_Rest))
		{
			// increment frame
			plantInst->animFrame = plantInst->animFrame + 1;
		}

		// if animation is done
		else
		{
			// reset animation
			plantInst->animFrame = 0;

			// After 3 cycles, transition to hungry
			plantObj->cycleCount++;
			if (plantObj->cycleCount == 3)
			{
				plantObj->cycleCount = 0;
				plantInst->animIndex = PlantAnim_TransitionRestHungry;
			}
		}
	}

	else if (plantInst->animIndex == PlantAnim_TransitionRestHungry)
	{
		// if animation is not over
		if ((plantInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(plantInst, PlantAnim_TransitionRestHungry))
		{
			// increment frame
			plantInst->animFrame = plantInst->animFrame + 1;
		}

		// animation is done
		else
		{
			plantInst->animFrame = 0;
			plantInst->animIndex = PlantAnim_Hungry;
			ThTick_SetAndExec(t, RB_Plant_ThTick_Hungry);
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b89a4-0x800b8c00.
void RB_Plant_LInB(struct Instance *inst)
{
	struct Plant *plantObj;
	struct SpawnType1 *ptrSpawnType1;
	s16 *metaArray;
	int plantID;
	struct Thread *t;

	if (inst->thread != NULL)
	{
		return;
	}

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Plant), NONE, SMALL, STATIC),
	    RB_Plant_ThTick_Rest, // behavior
	    "plant",              // debug name
	    0                     // thread relative
	);

	inst->thread = t;
	if (t == 0)
	{
		return;
	}

	t->inst = inst;

	inst->scale.x = 0x2800;
	inst->scale.y = 0x2800;
	inst->scale.z = 0x2800;
	inst->animFrame = 0;
	inst->animIndex = PlantAnim_Rest;

	plantObj = ((struct Plant *)t->object);
	plantObj->cycleCount = 0;
	plantObj->cooldown = 0;
	plantObj->boolEatingPlayer = 0;

	plantBoxDesc.bbox.min.x = 0xffc0;
	plantBoxDesc.bbox.min.y = 0xffc0;
	plantBoxDesc.bbox.min.z = 0;
	plantBoxDesc.bbox.max.x = 0x40;
	plantBoxDesc.bbox.max.y = 0x80;
	plantBoxDesc.bbox.max.z = 0x1e0;

	ptrSpawnType1 = sdata->gGT->level1->ptrSpawnType1;
	if (ptrSpawnType1->count > 0)
	{
		// puts plants on separate cycles
		void **pointers = ST1_GETPOINTERS(ptrSpawnType1);
		metaArray = (s16 *)pointers[ST1_SPAWN];

		plantID = inst->name[strlen(inst->name) - 1] - '0';
		plantObj->cooldown = metaArray[plantID * 2 + 0];
		plantObj->LeftOrRight = metaArray[plantID * 2 + 1];
	}
}

struct ParticleEmitter emSet_PlantTires[8] = {[0] =
                                                  {
                                                      .flags = 1,

                                                      // invalid axis, assume FuncInit
                                                      .initOffset = 0xC,

                                                      .InitTypes.FuncInit =
                                                          {
                                                              .particle_funcPtr = 0,
                                                              .particle_colorFlags = 0x121,
                                                              .particle_lifespan = 0x50,
                                                              .particle_Type = 0,
                                                          }

                                                      // last 0x10 bytes are blank
                                                  },

                                              [1] =
                                                  {
                                                      .flags = 0x13,

                                                      // posX
                                                      .initOffset = 0,

                                                      .InitTypes.AxisInit = {.baseValue =
                                                                                 {
                                                                                     .startVal = 1,
                                                                                     .velocity = -0x320,
                                                                                     .accel = 0,
                                                                                 },

                                                                             .rngSeed =
                                                                                 {
                                                                                     .startVal = 0,
                                                                                     .velocity = 0x640,
                                                                                     .accel = 0,
                                                                                 }}

                                                      // last 0x10 are blank
                                                  },

                                              [2] =
                                                  {
                                                      .flags = 0x13,

                                                      // posZ
                                                      .initOffset = 2,

                                                      .InitTypes.AxisInit = {.baseValue =
                                                                                 {
                                                                                     .startVal = 1,
                                                                                     .velocity = -0x320,
                                                                                     .accel = 0,
                                                                                 },

                                                                             .rngSeed =
                                                                                 {
                                                                                     .startVal = 0,
                                                                                     .velocity = 0x640,
                                                                                     .accel = 0,
                                                                                 }}

                                                      // last 0x10 are blank
                                                  },

                                              [3] =
                                                  {
                                                      .flags = 0x17,

                                                      // posY
                                                      .initOffset = 1,

                                                      .InitTypes.AxisInit = {.baseValue =
                                                                                 {
                                                                                     .startVal = 1,
                                                                                     .velocity = -0x640,
                                                                                     .accel = -0x320,
                                                                                 },

                                                                             .rngSeed =
                                                                                 {
                                                                                     .startVal = 0,
                                                                                     .velocity = 0x320,
                                                                                     .accel = 0,
                                                                                 }}

                                                      // last 0x10 are blank
                                                  },

                                              [4] =
                                                  {
                                                      .flags = 1,

                                                      // Scale
                                                      .initOffset = 5,

                                                      // 100% scale
                                                      .InitTypes.AxisInit.baseValue.startVal = 0x1000,

                                                      // all the rest is untouched
                                                  },

                                              [5] = {.flags = 0x1A,

                                                     // RotX
                                                     .initOffset = 4,

                                                     .InitTypes.AxisInit = {.baseValue =
                                                                                {
                                                                                    .startVal = 0,
                                                                                    .velocity = 0xC0,
                                                                                    .accel = 0,
                                                                                },

                                                                            .rngSeed =
                                                                                {
                                                                                    .startVal = 0x400,
                                                                                    .velocity = 0x40,
                                                                                    .accel = 0,
                                                                                }}},

                                              [6] = {.flags = 0xA,

                                                     // only for SpitTire
                                                     .initOffset = 0xA,

                                                     .InitTypes.AxisInit = {.baseValue =
                                                                                {
                                                                                    .startVal = 0,
                                                                                    .velocity = 0x100,
                                                                                    .accel = 0,
                                                                                },

                                                                            .rngSeed =
                                                                                {
                                                                                    .startVal = 0xE00,
                                                                                    .velocity = 0,
                                                                                    .accel = 0,
                                                                                }}},

                                              // null terminator
                                              [7] = {0}};
