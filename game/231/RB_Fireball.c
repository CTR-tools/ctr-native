#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b5f50-0x800b64c0.

int RB_Fireball_ThCollide(struct Thread *fireballThread, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	(void)fireballThread;
	(void)driverTh;
	(void)funcThCollide;

	return sps->Input1.modelID == DYNAMIC_PLAYER;
}

struct ParticleEmitter emSet_Fireball[10] = {[0] =
                                                 {
                                                     .flags = 1,

                                                     // invalid axis, assume FuncInit
                                                     .initOffset = 0xC,

                                                     .InitTypes.FuncInit =
                                                         {
                                                             .particle_funcPtr = 0,
                                                             .particle_colorFlags = 0x4A1,
                                                             .particle_lifespan = 0x400,
                                                             .particle_Type = 0,
                                                         }

                                                     // last 0x10 bytes are blank
                                                 },

                                             [1] =
                                                 {
                                                     .flags = 1,

                                                     // posX
                                                     .initOffset = 0,

                                                     .InitTypes.AxisInit.baseValue.startVal = 1,

                                                     // The rest is blank

                                                     // last 0x10 are blank
                                                 },

                                             [2] =
                                                 {
                                                     .flags = 1,

                                                     // posZ
                                                     .initOffset = 2,

                                                     .InitTypes.AxisInit.baseValue.startVal = 1,

                                                     // The rest is blank

                                                     // last 0x10 are blank
                                                 },

                                             [3] = {.flags = 3,

                                                    // posY
                                                    .initOffset = 1,

                                                    .InitTypes.AxisInit = {.baseValue =
                                                                               {
                                                                                   .startVal = 1,
                                                                                   .velocity = 1,
                                                                                   .accel = 0,
                                                                               }}},

                                             [4] = {.flags = 9,

                                                    // rotY
                                                    .initOffset = 4,

                                                    .InitTypes.AxisInit = {.baseValue = {.startVal = 1, .velocity = 0, .accel = 0},

                                                                           .rngSeed = {.startVal = 0x1000, .velocity = 0, .accel = 0}}},

                                             [5] = {.flags = 3,

                                                    // scale
                                                    .initOffset = 5,

                                                    .InitTypes.AxisInit =
                                                        {
                                                            .baseValue = {.startVal = 0xA00, .velocity = -0xB0, .accel = 0},
                                                        }},

                                             [6] = {.flags = 0xB,

                                                    // colorR
                                                    .initOffset = 7,

                                                    .InitTypes.AxisInit = {.baseValue = {.startVal = 0xFF00, .velocity = 0xC000, .accel = 0},

                                                                           .rngSeed =
                                                                               {
                                                                                   .startVal = 0x5F00,
                                                                                   .velocity = 0,
                                                                                   .accel = 0,
                                                                               }}},

                                             [7] = {.flags = 3,

                                                    // colorG
                                                    .initOffset = 8,

                                                    .InitTypes.AxisInit = {.baseValue = {.startVal = 0x8000, .velocity = 0xE000, .accel = 0},

                                                                           .rngSeed =
                                                                               {
                                                                                   .startVal = 0,
                                                                                   .velocity = 0,
                                                                                   .accel = 0,
                                                                               }}},

                                             [8] = {.flags = 3,

                                                    // colorB
                                                    .initOffset = 9,

                                                    .InitTypes.AxisInit = {.baseValue = {.startVal = 0x4000, .velocity = 0xF000, .accel = 0}}},

                                             // null terminator
                                             [9] = {0}};

void RB_Fireball_ThTick(struct Thread *t)
{
	struct Instance *fireInst;
	struct Fireball *fireObj;
	struct Particle *particle;
	int velY;
	int oldVelY;
	int resetPosY;

	struct GameTracker *gGT;
	int elapsedTimeMS;

	gGT = sdata->gGT;
	elapsedTimeMS = gGT->elapsedTimeMS;

	fireInst = t->inst;
	fireObj = t->object;

	if (fireObj->cooldown != 0)
	{
		fireObj->cooldown -= elapsedTimeMS;

		if (fireObj->cooldown < 0)
		{
			fireObj->cooldown = 0;
		}

		return;
	}

	fireInst->flags |= HIDE_MODEL;

	oldVelY = 0;
	resetPosY = fireInst->instDef->pos.y - 0x440;

	// if fireball isn't below the lava,
	// handle all particle spawning
	if (fireInst->matrix.t[1] >= resetPosY)
	{
		// move based on velocity
		velY = fireObj->velY;
		oldVelY = velY;
		fireInst->matrix.t[1] += (velY * elapsedTimeMS) >> 5;

		// reduce velocity (gravity)
		velY -= ((elapsedTimeMS * 10) >> 5);

		// terminal velocity
		if (velY < -200)
		{
			velY = -200;
		}

		// set new velY
		fireObj->velY = velY;

		// fire particles
		particle = Particle_Init(0, gGT->iconGroup[0xA], &emSet_Fireball[0]);

		if (particle != 0)
		{
			// adjust positions,
			// dont bitshift, must multiply, or negatives break
			particle->axis[0].startVal += fireInst->matrix.t[0] * 0x100;
			particle->axis[1].startVal += fireInst->matrix.t[1] * 0x100;
			particle->axis[2].startVal += fireInst->matrix.t[2] * 0x100;

			particle->renderDepthLimit = 0x1e00;

			// reuse "velY" variable for particles
			velY *= -0x180;

			// range check
			if (velY < -0x7fff)
			{
				velY = -0x7fff;
			}
			if (velY > 0x7fff)
			{
				velY = 0x7fff;
			}
			velY = (s16)velY;

			particle->axis[1].velocity = (int)velY;
		}

		Seal_CheckColl(fireInst, t, 4, 0x10000, 0);
	}

	// === rest of movement behavior ===

	fireObj->cycleTimer -= elapsedTimeMS;

	// if animation is not over
	if ((fireInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(fireInst, 0))
	{
		// increment frame
		fireInst->animFrame = fireInst->animFrame + 1;
	}

	// if animation ended
	else
	{
		// reset
		fireInst->animFrame = 0;
	}

	if ((oldVelY >= 0) && (fireObj->velY < 0))
	{
		fireObj->direction = 1;
	}

	// if cycle is over
	if (fireObj->cycleTimer < 1)
	{
		// == first frame of fireball rising ==

		// reset timer
		fireObj->cycleTimer = 0xb40;

		// upward velocity
		fireObj->velY = 200;
		fireObj->direction = 0;

		// reset position under lava
		fireInst->matrix.t[1] = resetPosY;

		// reset animation
		fireInst->animFrame = 0;

		// fwooooossssssssshhhh
		PlaySound3D(0x81, fireInst);
	}
}

void RB_Fireball_LInB(struct Instance *inst)
{
	struct Fireball *fireObj;
	struct Thread *t;
	int fireballID;

	if (inst->thread != 0)
	{
		return;
	}

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Fireball), NONE, SMALL, STATIC),

	    RB_Fireball_ThTick, // behavior
	    "fireball",         // debug name
	    0                   // thread relative
	);

	if (t == 0)
	{
		return;
	}
	inst->thread = t;
	t->inst = inst;
	t->funcThCollide = (void *)RB_Fireball_ThCollide;

	inst->scale.x = 0x4000;
	inst->scale.y = 0x4000;
	inst->scale.z = 0x4000;

	inst->animFrame = 0;
	inst->animIndex = 0;

	fireObj = ((struct Fireball *)t->object);
	fireObj->cycleTimer = 0;
	fireObj->cooldown = 0;
	fireObj->rot_unused.x = 0;
	fireObj->velY = 96;
	fireObj->direction = 0;

	fireballID = inst->name[strlen(inst->name) - 1] - '0';
	fireObj->fireballID = fireballID;

	if ((fireballID & 1) != 0)
	{
		// 1.44s, this is a ms-based timer, not a frame-based
		// counter, so t->cooldownFrameCount is not allowed
		fireObj->cooldown = 1440;
	}
}
