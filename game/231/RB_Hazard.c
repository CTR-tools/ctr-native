#include <common.h>

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800ac1b0-0x800ac220.

int RB_Hazard_HurtDriver(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason)
{
	struct GameTracker *gGT = sdata->gGT;
	int result = 0;

	if ((driverVictim->actionsFlagSet & ACTION_BOT) == 0)
	{
		result = VehPickState_NewState(driverVictim, damageType, driverAttacker, reason);
	}
	else
	{
		if ((gGT->levelID == OXIDE_STATION) && (IS_BOSS_RACE(gGT->gameMode1)))
			damageType = 1;

		result = (int)BOTS_ChangeState(driverVictim, damageType, driverAttacker, reason);
	}
	return result;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800ac220-0x800ac350.
struct Instance *RB_Hazard_CollideWithDrivers(struct Instance *weaponInst, char boolCanSkipParent, int hitRadius, struct Instance *mineDriverInst)
{
	int j;
	struct Driver *driver;
	struct Instance *driverInst;

	u32 dist[3];
	int modelID;

	u32 distCheck;
	s32 delta;

	for (int i = 0; i < 8; i++)
	{
		driver = sdata->gGT->drivers[i];

		if (driver == 0)
			continue;
		if (driver->kartState == KS_MASK_GRABBED)
			continue;
		driverInst = driver->instSelf;

		for (j = 0; j < 3; j++)
		{
			delta = driverInst->matrix.t[j] - weaponInst->matrix.t[j];
			dist[j] = (u32)delta * (u32)delta;
		}

		modelID = weaponInst->model->id;

		// 2D collision (barrel, warpball)
		distCheck = dist[0] + dist[2];

		// to be more optimal, just do weaponInst->thread->funcThTick == GenericMine_ThTick
		if (((u32)modelID - STATIC_BEAKER_RED < 2) || // red or green potion
		    (modelID == PU_EXPLOSIVE_CRATE) ||        // Nitro
		    (modelID == STATIC_CRATE_TNT)             // TNT
		)
		{
			// upgrade to 2D collision to 3D sphere
			distCheck += dist[1];
		}

		// 2D collision, or 3D sphere
		if (distCheck < (u32)hitRadius)
		{
			if ((boolCanSkipParent != 0) && (driverInst == mineDriverInst))
				continue;

			// wasted check for 3D sphere,
			// also upgrades 2D collision to 3D cylinder
			if ((s32)dist[1] < (s32)((u32)hitRadius << 2))
			{
				return driverInst;
			}
		}
	}

	return 0;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800ac350-0x800ac3f8.
struct Instance *RB_Hazard_CollideWithBucket(struct Instance *weaponInst, struct Thread *weaponTh, struct Thread *bucket, char boolCanSkipParent, int hitRadius,
                                             struct Instance *mineDriverInst)
{
	int i;
	struct Instance *threadInst;

	s32 distComponent;
	u32 distCheck;

	(void)weaponTh;

	for (; bucket != 0; bucket = bucket->siblingThread)
	{
		distCheck = 0;
		threadInst = bucket->inst;

		for (i = 0; i < 3; i++)
		{
			distComponent = threadInst->matrix.t[i] - weaponInst->matrix.t[i];
			distCheck += (u32)distComponent * (u32)distComponent;
		}

		if (distCheck < (u32)hitRadius)
		{
			if ((boolCanSkipParent != 0) && (threadInst == mineDriverInst))
				continue;

			return threadInst;
		}
	}

	return 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac3f8-0x800ac42c.
void RB_Hazard_ThCollide_Generic_Alt(struct Thread **param_1)
{
	RB_Hazard_ThCollide_Generic(param_1[0]);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac42c-0x800ac4b8.
// NOTE(aalhendi): Native ThCollide ABI is void; retail returns v0=1.
void RB_Hazard_ThCollide_Missile(struct Thread *thread)
{
	struct TrackerWeapon *tw;
	struct Driver *driver;
	struct Instance *inst;

	inst = thread->inst;
	tw = inst->thread->object;

	// could I also just do thread->modelID?
	if (inst->model->id == DYNAMIC_ROCKET)
	{
		// get driver
		driver = tw->driverTarget;

		// if driver is valid
		if (driver != 0)
		{
			// remove 2D square-target being drawn on the player's screen
			driver->actionsFlagSet &= ~ACTION_TRACKER_TARGETED;
		}

		// play audio of explosion
		PlaySound3D(0x4c, inst);

		// stop audio of moving
		OtherFX_RecycleMute(&tw->audioPtr);

		// kill thread
		thread->flags |= THREAD_FLAG_DEAD;
	}

	return;
}

// I think this function should return void?

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac4b8-0x800ac5e8.
void RB_Hazard_ThCollide_Generic(struct Thread *thread)
{
	struct Instance *inst;
	struct MineWeapon *mw;

	struct Instance *crateInst;
	struct Thread *crateThread;
	struct Crate *crateObj;

	int modelID;
	int soundID;

	inst = thread->inst;
	mw = thread->object;

	crateInst = mw->crateInst;

	if (crateInst != 0)
	{
		// be careful, dont overwrite local variable
		// "thread", or else you'll kill the wrong thread
		// at the end of the function

		crateObj = (struct Crate *)crateInst->thread->object;

		if (crateObj != 0)
		{
			crateObj->boolPauseCooldown = 0;
		}
	}

	modelID = inst->model->id;

	// if red beaker or green beaker
	if ((u32)(modelID - STATIC_BEAKER_RED) < 2)
	{
		PlaySound3D(0x3f, inst);

		RB_MinePool_Remove(mw);
	}

	else
	{
		// nitro
		if (modelID == PU_EXPLOSIVE_CRATE)
		{
			// shatter sound
			soundID = 0x3f;
		}

		else
		{
			// if not TNT
			if (modelID != STATIC_CRATE_TNT)
			{
				return;
			}

			// at this point, must be TNT

			// if driver hit TNT
			if (mw->driverTarget != 0)
			{
				// quit, explosion handled
				// by TNT thread
				return;
			}

			// if no driver hit TNT,
			// then handle explosion here
			soundID = 0x3d;
		}

		PlaySound3D(soundID, inst);

		RB_MinePool_Remove(mw);

		RB_Explosion_InitGeneric(inst);

		inst->scale.x = 0;
		inst->scale.y = 0;
		inst->scale.z = 0;

		inst->flags |= HIDE_MODEL;
	}

	// kill thread
	thread->flags |= THREAD_FLAG_DEAD;
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ad9ac-0x800ada90.
u16 RB_Hazard_CollLevInst(struct ScratchpadStruct *sps, struct Thread *th)
{
	u16 flag;
	s16 model;
	struct Instance *inst;
	struct InstDef *instdef;
	struct MetaDataMODEL *meta;

	// Check if the hitbox flag has the collision bit set and if InstDef is not NULL
	if ((sps->bspHitbox->flag & 0x80) && (instdef = sps->bspHitbox->data.hitbox.instDef) != NULL)
	{
		inst = instdef->ptrInstance;
		if (inst == NULL)
			return 1;

		model = inst->model->id;

		// Get the metadata for the model
		meta = COLL_LevModelMeta(model);

		// Check if LInC is not nullptr
		if ((meta != NULL) && (meta->LInC != NULL))
		{
			// Execute LInC, create a thread for this instance, and let it run thread->funcThCollide upon collision
			flag = meta->LInC(inst, th, sps);

			// if not PU_WUMPA_FRUIT
			if (model != 2)
			{
				// useless
				if (model < 2)
				{
					return flag;
				}

				// anything except for
				// 7: PU_FRUIT_CRATE,
				// 8: PU_RANDOM_CRATE (weapon box)
				if (8 < model)
				{
					return flag;
				}
				if (model < 7)
				{
					return flag;
				}
			}
			return 0;
		}
	}

	// make potion open teeth,
	// or make warpball turn around
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ada90-0x800adb50.
int RB_Hazard_InterpolateValue(s16 currRot, s16 desiredRot, s16 rotSpeed)
{
	if (currRot == desiredRot)
		return currRot;

	int delta = (rotSpeed * sdata->gGT->elapsedTimeMS) >> 5;

	// adjust for range of "s16" [0-0xffff]
	// compared to range of degrees [0-0xfff]
	int diff = ((desiredRot - currRot) & 0xfff);
	if (diff > 0x7ff)
		diff -= 0x1000;

	// skip to end if close enough
	int diffAbs = diff;
	if (diffAbs < 0)
		diffAbs = -diffAbs;
	if (diffAbs < rotSpeed)
		return desiredRot & 0xfff;

	// interpolate
	if (diff < 0)
		currRot -= delta;
	else
		currRot += delta;
	return currRot & 0xfff;
}
