#include <common.h>


int RB_Hazard_HurtDriver(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason)
{
	int result;
	if (Chaos_DeferDriverDamage(driverVictim, damageType, driverAttacker, reason))
	{
		return 0;
	}

	if ((driverVictim->actionsFlagSet & ACTION_BOT) != 0)
	{
		struct GameTracker *gGT = GAME_TRACKER;
		if ((gGT->levelID == OXIDE_STATION) && (IS_BOSS_RACE(gGT->gameMode1)))
		{
			damageType = 1;
		}

		result = (int)BOTS_ChangeState(driverVictim, damageType, driverAttacker, reason);
	}
	else
	{
		result = VehPickState_NewState(driverVictim, damageType, driverAttacker, reason);
	}
	return result;
}

struct Instance *RB_Hazard_CollideWithDrivers(struct Instance *weaponInst, s16 parentSafetyFrames, int hitRadius, struct Instance *mineDriverInst)
{
	s32 dx, dz, dy;
	u16 modelID;
	u32 distCheck;

	{
		int i;
		for (i = 0; i < 8; i++)
		{
			struct Instance *driverInst;
			struct Driver *driver = GAME_TRACKER->drivers[i];

			if (driver == 0)
			{
				continue;
			}
			if (driver->kartState == KS_MASK_GRABBED)
			{
				continue;
			}
			if (Chaos_IsFrozenWorldThread(weaponInst->thread) && !Chaos_IsTimeStopImmune(driver->instSelf->thread))
			{
				continue;
			}
			driverInst = driver->instSelf;

			// NOTE(aalhendi): Distance arithmetic wraps at 32 bits on both targets.
			dx = (u32)driverInst->matrix.t[0] - (u32)weaponInst->matrix.t[0];
			dz = (u32)driverInst->matrix.t[2] - (u32)weaponInst->matrix.t[2];
			dy = (u32)driverInst->matrix.t[1] - (u32)weaponInst->matrix.t[1];

			modelID = weaponInst->model->id;

			if (((u32)modelID - STATIC_BEAKER_RED < 2) || // red or green potion
			    ((s16)modelID == PU_EXPLOSIVE_CRATE) ||   // Nitro
			    ((s16)modelID == STATIC_CRATE_TNT)        // TNT
			)
			{
				distCheck = (u32)dx * dx + (u32)dz * dz + (u32)dy * dy;
			}
			else
			{
				distCheck = (u32)dx * dx + (u32)dz * dz;
			}

			// 2D collision, or 3D sphere
			if (distCheck < (u32)hitRadius)
			{
				if ((parentSafetyFrames != 0) && (driverInst == mineDriverInst))
				{
					continue;
				}

				// The vertical bound also turns the planar test into a cylinder.
				if ((s32)((u32)dy * dy) < (s32)((u32)hitRadius << 2))
				{
					return driverInst;
				}
			}
		}
	}

	return 0;
}

struct Instance *RB_Hazard_CollideWithBucket(struct Instance *weaponInst, struct Thread *weaponTh, struct Thread *bucket, s16 parentSafetyFrames, int hitRadius,
                                             struct Instance *mineDriverInst)
{
	s32 dx, dz, dy;
	u32 distCheck;

	(void)weaponTh;

	for (; bucket != 0; bucket = bucket->siblingThread)
	{
		struct Instance *threadInst;
		threadInst = bucket->inst;

		dx = (u32)threadInst->matrix.t[0] - (u32)weaponInst->matrix.t[0];
		dz = (u32)threadInst->matrix.t[2] - (u32)weaponInst->matrix.t[2];
		dy = (u32)threadInst->matrix.t[1] - (u32)weaponInst->matrix.t[1];
		distCheck = (u32)dx * dx + (u32)dz * dz + (u32)dy * dy;

		if (distCheck < (u32)hitRadius)
		{
			if ((parentSafetyFrames != 0) && (threadInst == mineDriverInst))
			{
				continue;
			}

			return threadInst;
		}
	}

	return 0;
}


int RB_Hazard_ThCollide_Generic_Alt(struct ThreadCollisionArgs *collision)
{
	return RB_Hazard_ThCollide_Generic(collision->self, collision->other, collision->funcThCollide, collision->sps);
}

int RB_Hazard_ThCollide_Missile(struct Thread *thread, struct Thread *other, void *funcThCollide, struct ScratchpadStruct *sps)
{
	struct Instance *inst = thread->inst;
	struct TrackerWeapon *tw = inst->thread->object;
	(void)other;
	(void)funcThCollide;
	(void)sps;

	if (inst->model->id == DYNAMIC_ROCKET)
	{
		// get driver
		struct Driver *driver = tw->driverTarget;

		// if driver is valid
		if (driver != 0)
		{
			// remove 2D square-target being drawn on the player's screen
			driver->actionsFlagSet &= ~ACTION_TRACKER_TARGETED;
		}

		// play audio of explosion
		PlaySound3D(0x4c, inst);

		// stop audio of moving
		OtherFX_RecycleMute(&tw->soundIDCount);

		// kill thread
		thread->flags |= THREAD_FLAG_DEAD;
	}

	return 1;
}

int RB_Hazard_ThCollide_Generic(struct Thread *thread, struct Thread *other, void *funcThCollide, struct ScratchpadStruct *sps)
{
	s16 modelID;
	struct Instance *inst = thread->inst;
	struct MineWeapon *mw = inst->thread->object;
	int soundID;

	struct Instance *crateInst = mw->crateInst;
	(void)other;
	(void)funcThCollide;
	(void)sps;
	if (crateInst != 0)
	{
		struct Thread *crateThread = crateInst->thread;
		if (crateThread != NULL)
		{
			struct Crate *crateObj = crateThread->object;
			if (crateObj != NULL)
			{
				crateObj->boolPauseCooldown = 0;
			}
		}
	}

	modelID = inst->model->id;

	// if red beaker or green beaker
	if ((u32)((u16)modelID - STATIC_BEAKER_RED) < 2)
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
				goto CollisionDone;
			}

			// at this point, must be TNT

			// if driver hit TNT
			if (mw->driverTarget != 0)
			{
				// quit, explosion handled
				// by TNT thread
				goto CollisionDone;
			}

			// if no driver hit TNT,
			// then handle explosion here
			soundID = HOWL_SFX_TNT_EXPLOSION;
		}

		PlaySound3D(soundID, inst);

		RB_MinePool_Remove(mw);

		RB_Explosion_InitGeneric(inst);

		inst->scale.z = 0;
		inst->scale.y = 0;
		inst->scale.x = 0;

		inst->flags |= HIDE_MODEL;
	}

	// kill thread
	thread->flags |= THREAD_FLAG_DEAD;
CollisionDone:
	return 1;
}

s32 RB_Hazard_CollLevInst(struct ScratchpadStruct *sps, struct Thread *th)
{
	struct InstDef *instdef;

	if (((u8)sps->bspHitbox->flag & 0x80) && (instdef = sps->bspHitbox->data.hitbox.instDef) != NULL)
	{
		s16 model;
		struct MetaDataMODEL *meta;
		struct Instance *inst = instdef->ptrInstance;
		if (inst == NULL)
		{
			goto noCallback;
		}

		model = instdef->modelID;

		meta = COLL_LevModelMeta(model);

		if ((meta != NULL) && (meta->LInC != NULL))
		{
			s32 flag = 1;
			if (!Chaos_DeferImpact(CHAOS_IMPACT_HAZARD_LEVEL, th, NULL, sps->bspHitbox))
			{
				flag = meta->LInC(inst, th, sps);
			}

			// NOTE(aalhendi): Pickups receive the collision, but do not stop the hazard.
			// Classify the original level model, before its callback can change state.
			switch (model)
			{
			case PU_WUMPA_FRUIT:
			case PU_FRUIT_CRATE:
			case PU_RANDOM_CRATE:
				return 0;
			default:
				return flag;
			}
		}
	}

noCallback:
	// An unhandled level collision can open teeth or turn a warpball around.
	return 1;
}

int RB_Hazard_InterpolateValue(s16 currRot, s16 desiredRot, s16 rotSpeed)
{
	int diff;
	s16 result = currRot;

	if (currRot != desiredRot)
	{
		// Take the shortest signed arc in a 4096-unit revolution.
		diff = (desiredRot - currRot) & 0xfff;
		if (diff > 0x7ff)
		{
			diff -= 0x1000;
		}

		if (abs(diff) < rotSpeed)
		{
			result = desiredRot;
		}
		else if (diff > 0)
		{
			result = currRot + ((rotSpeed * GAME_TRACKER->elapsedTimeMS) >> 5);
		}
		else if (diff < 0)
		{
			result = currRot - ((rotSpeed * GAME_TRACKER->elapsedTimeMS) >> 5);
		}
		result &= 0xfff;
	}
	return result;
}
