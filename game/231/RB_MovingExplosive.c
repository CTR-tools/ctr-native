#include <common.h>

static inline void RB_MovingExplosive_CollideAndExplode(struct Thread *hit, struct Thread *t, struct Instance *inst, struct TrackerWeapon *tw)
{
	if (!Chaos_DeferImpact(CHAOS_IMPACT_HAZARD_THREAD, t, hit, NULL))
	{
		((ThreadScratchCollideFunc)hit->funcThCollide)(hit, t, hit->funcThCollide, NULL);
	}
	// NOTE(aalhendi): The callback can rebind thread state; cleanup uses the tick's original instance and weapon.
	RB_MovingExplosive_Explode(t, inst, tw);
}

static inline void RB_MovingExplosive_NotifyCollision(struct Thread *hit, struct Thread *other)
{
	if (!Chaos_DeferImpact(CHAOS_IMPACT_HAZARD_THREAD, other, hit, NULL))
	{
		((ThreadScratchCollideFunc)hit->funcThCollide)(hit, other, hit->funcThCollide, NULL);
	}
	// NOTE(aalhendi): Keep this callback site separate from the mine callback.
	// GCC 2.8.1 otherwise folds their identical tails. This compiler-only boundary
	// emits no PSX instructions and is a no-op on native; the call stays typed C.
	CTR_PSX_MEMORY_BARRIER();
}

// Update a rolling bomb, launched shield, or homing missile.
void RB_MovingExplosive_ThTick(struct Thread *t)
{
	struct Driver *driverTarget;
	struct ScratchpadStruct *sps;
	struct Instance *hitInst;
	s16 desiredRotY;
	s32 deltaX;
	s32 deltaZ;
	struct TrackerWeapon *tw;
	struct Instance *inst;
	SVec3 posA;
	SVec3 posB;

	inst = t->inst;
	tw = inst->thread->object;
	tw->timeAlive += GAME_TRACKER->elapsedTimeMS;
	sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);

	// Start or update the weapon's looping sound.
	if (inst->model->id == DYNAMIC_ROCKET)
	{
		if ((t->flags & THREAD_FLAG_DEAD) == 0)
		{
			PlaySound3D_Flags(&tw->soundIDCount, 0x4b, inst);
		}
	}
	else
	{
		if (inst->model->id == DYNAMIC_BOMB)
		{
			PlaySound3D_Flags(&tw->soundIDCount, 0x48, inst);
		}
		else
		{
			if ((inst->model->id == DYNAMIC_SHIELD) || (inst->model->id == DYNAMIC_SHIELD_GREEN))
			{
				PlaySound3D_Flags(&tw->soundIDCount, 0x59, inst);
			}
		}
	}

	{
		struct Driver *visibleTarget = tw->driverTarget;

		if (
		    // NOTE(aalhendi): Shieldbombs can have no target; native cannot read PS1 null space.
		    visibleTarget == NULL ||
		    visibleTarget->invisibleTimer != 0)
		{
			tw->driverTarget = NULL;
		}
		else
		{
			if (
			    // A thrown mine distracts an incoming missile for ten frames.
			    ((visibleTarget->actionsFlagSetPrevFrame & ACTION_DROPPING_MINE) != 0) &&

			    (inst->model->id == DYNAMIC_ROCKET))
			{
				tw->framesSeekTargetTnt = 10;
			}
		}
	}

	driverTarget = tw->driverTarget;

	if ((driverTarget == NULL) || (tw->blindFrames != 0))
	{
		if (tw->blindFrames != 0)
		{
			tw->blindFrames--;
		}
	}
	else
	{
		if (tw->framesSeekTargetTnt != 0)
		{
			struct Instance *instTNT;
			tw->framesSeekTargetTnt--;

			// if target shot a TNT
			instTNT = tw->driverTarget->instTntSend;

			if (instTNT != 0)
			{
				// Aim at the thrown TNT instead of the driver.
				deltaX = instTNT->matrix.t[0] - inst->matrix.t[0];
				deltaZ = instTNT->matrix.t[2] - inst->matrix.t[2];
				desiredRotY = ratan2(deltaX, deltaZ);
			}
			else
			{
				// The target no longer has a thrown TNT to seek.
				desiredRotY = tw->rotY;
				tw->framesSeekTargetTnt = 0;
			}
		}
		else
		{
			// Get distance and heading to the driver being chased.
			deltaX = (driverTarget->posCurr.x >> 8) - inst->matrix.t[0];
			deltaZ = (driverTarget->posCurr.z >> 8) - inst->matrix.t[2];
			tw->distanceToTarget = (u32)deltaX * deltaX + (u32)deltaZ * deltaZ;

			desiredRotY = ratan2(deltaX, deltaZ);
		}

		if ((inst->model->id == DYNAMIC_BOMB) || (inst->model->id == DYNAMIC_SHIELD))
		{
			tw->rotY = RB_Hazard_InterpolateValue(tw->rotY, (int)desiredRotY, 4);

			// NOTE(aalhendi): The shifted result is stored in a signed halfword;
			// logical and arithmetic shifts have the same retained low 16 bits here.
			tw->vel.x = (u32)(MATH_Sin(tw->rotY) * 3) >> 7;
			tw->vel.z = (u32)(MATH_Cos(tw->rotY) * 3) >> 7;

			if ((tw->flags & TRACKER_FLAG_BOMB_BACKWARD) != 0)
			{
				tw->vel.z = -tw->vel.z;
				tw->vel.x = -tw->vel.x;
			}

		        if (inst->model->id == DYNAMIC_BOMB)
		        {
		                tw->vel.x = tw->vel.x * g_config.bombSpeedMultiplier / 100;
		                tw->vel.z = tw->vel.z * g_config.bombSpeedMultiplier / 100;
		        }
		}

		// Missiles steer and rotate to face their target.
		else
		{
			// A powered-up missile turns faster and uses the bomb's speed.
			if ((tw->flags & TRACKER_FLAG_POWERED_UP) != 0)
			{
				tw->rotY = RB_Hazard_InterpolateValue(tw->rotY, (int)desiredRotY, 0x80);

				tw->vel.x = (u32)(MATH_Sin(tw->rotY) * 3) >> 7;
				tw->vel.z = (u32)(MATH_Cos(tw->rotY) * 3) >> 7;
			}

			else
			{
				tw->rotY = RB_Hazard_InterpolateValue(tw->rotY, (int)desiredRotY, 0x40);

				tw->vel.x = (u32)(MATH_Sin(tw->rotY) * 5) >> 8;
				tw->vel.z = (u32)(MATH_Cos(tw->rotY) * 5) >> 8;
			}

		        tw->vel.x = tw->vel.x * g_config.missileSpeedMultiplier / 100;
		        tw->vel.z = tw->vel.z * g_config.missileSpeedMultiplier / 100;

			tw->dir.x = 0;
			tw->dir.z = 0;
			tw->dir.y = tw->rotY;

			// convert 3 rotation shorts into rotation matrix
			ConvertRotToMatrix(&inst->matrix, &tw->dir);
		}
	}

	// Loop the weapon animation.
	if (inst->animFrame + 1 < INSTANCE_GetNumAnimFrames(inst, 0))
	{
		inst->animFrame += 1;
	}

	else
	{
		inst->animFrame = 0;
	}

	// Missile exhaust is only emitted in single-player.
	if ((inst->model->id == DYNAMIC_ROCKET) && (GAME_TRACKER->numPlyrCurrGame < 2))
	{
		struct Particle *p;
		p = Particle_Init(0, GAME_TRACKER->iconGroup[0], &R231.emSet_Missile[0]);

		if (p != 0)
		{
			Chaos_InheritParticleOwner(p, t);
			p->axis[0].startVal = (u32)inst->matrix.t[0] << 8;
			p->axis[1].startVal = (u32)inst->matrix.t[1] << 8;
			p->axis[2].startVal = (u32)inst->matrix.t[2] << 8;
		}
	}

	inst->matrix.t[0] += (((int)tw->vel.x * GAME_TRACKER->elapsedTimeMS) >> 5);
	inst->matrix.t[1] += (((int)tw->vel.y * GAME_TRACKER->elapsedTimeMS) >> 5);
	inst->matrix.t[2] += (((int)tw->vel.z * GAME_TRACKER->elapsedTimeMS) >> 5);

	// Roll the bomb in its direction of travel.
	if (inst->model->id == DYNAMIC_BOMB)
	{
		if ((tw->flags & TRACKER_FLAG_BOMB_BACKWARD) != 0)
		{
			tw->dir.x -= 0x200;
		}

		else
		{
			tw->dir.x += 0x200;
		}

		// convert 3 rotation shorts into rotation matrix
		ConvertRotToMatrix(&inst->matrix, &tw->dir);
	}

	posA.x = inst->matrix.t[0];
	posA.y = inst->matrix.t[1] + -0x40;
	posA.z = inst->matrix.t[2];

	posB.x = inst->matrix.t[0];
	posB.y = inst->matrix.t[1] + 0x100;
	posB.z = inst->matrix.t[2];

	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_TRIGGER;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_FORCE_INSTANCE_HIT;

	if (GAME_TRACKER->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD | COLL_SEARCH_FORCE_INSTANCE_HIT;
	}

	sps->ptr_mesh_info = GAME_TRACKER->level1->ptr_mesh_info;

	COLL_SearchBSP_CallbackQUADBLK(&posA, &posB, sps, 0);

	RB_MakeInstanceReflective(sps, inst);

	if ((sps->collision.stepFlags & COLL_STEP_TRIGGER_WEAPON_REACT) != 0)
	{
		// move backward one frame
		tw->vel.x = -tw->vel.x;
		tw->vel.y = -tw->vel.y;
		tw->vel.z = -tw->vel.z;

		inst->matrix.t[0] += ((int)tw->vel.x * GAME_TRACKER->elapsedTimeMS) >> 5;
		inst->matrix.t[1] += ((int)tw->vel.y * GAME_TRACKER->elapsedTimeMS) >> 5;
		inst->matrix.t[2] += ((int)tw->vel.z * GAME_TRACKER->elapsedTimeMS) >> 5;

		RB_MovingExplosive_Explode(t, inst, tw);
		return;
	}

	if (sps->boolDidTouchHitbox != 0)
	{
		s32 ret;
		sps->Input1.modelID = inst->model->id;

		ret = RB_Hazard_CollLevInst(sps, t);

		if (ret == 1)
		{
			struct InstDef *instDef;
			struct BSP *bspHitbox = sps->bspHitbox;

			// Temple teeth open when a weapon hits their level instance.
			if (((u8)bspHitbox->flag & 0x80) && (instDef = bspHitbox->data.hitbox.instDef) != NULL && instDef->ptrInstance != NULL &&
			    (s16)instDef->modelID == STATIC_TEETH &&
			    !Chaos_DeferImpact(CHAOS_IMPACT_HAZARD_LEVEL, t, NULL, bspHitbox))
			{
				RB_Teeth_OpenDoor(instDef->ptrInstance);
			}
			goto explode;
		}
	}
	else
	{
		if (sps->boolDidTouchQuadblock != 0)
		{
			tw->vel.y = 0;

			// missile model
			if (inst->model->id == DYNAMIC_ROCKET)
			{
				VehPhysForce_RotAxisAngle(&inst->matrix, CTR_VECTOR_DATA(&(sps->hit.plane.normal)), tw->rotY);
			}

			// position
			inst->matrix.t[0] = sps->Union.QuadBlockColl.hitPos.x;
			inst->matrix.t[1] = sps->Union.QuadBlockColl.hitPos.y + 0x30;
			inst->matrix.t[2] = sps->Union.QuadBlockColl.hitPos.z;
		}

		else
		{
			s32 elapsedTime;
			// Search farther down before applying gravity; explode if there is no ground.

			inst->vertSplit = 0;
			posA.x = inst->matrix.t[0];
			posA.y = inst->matrix.t[1] - 0x900;
			posA.z = inst->matrix.t[2];

			COLL_SearchBSP_CallbackQUADBLK(&posA, &posB, sps, 0);

			// if still nothing, then explode
			if (sps->boolDidTouchQuadblock == 0)
			{
				goto explode;
			}

			elapsedTime = GAME_TRACKER->elapsedTimeMS;
			tw->vel.y = tw->vel.y - ((inst->model->id == DYNAMIC_ROCKET ? elapsedTime << 3 : elapsedTime << 2) >> 5);

			if (tw->vel.y < -0x60)
			{
				tw->vel.y = -0x60;
			}
		}
	}

	hitInst = RB_Hazard_CollideWithDrivers(inst, tw->parentSafetyFrames, 0x2400, tw->instParent);
	if (hitInst != NULL)
	{
		RB_MovingExplosive_CollideDriver(t, hitInst->thread);
		return;
	}

	hitInst = RB_Hazard_CollideWithBucket(inst, t, GAME_TRACKER->threadBuckets[MINE].thread, tw->parentSafetyFrames, 0x2400, tw->instParent);
	if (hitInst != NULL)
	{
		RB_MovingExplosive_CollideAndExplode(hitInst->thread, t, inst, tw);
		return;
	}

	if (inst->model->id == DYNAMIC_BOMB)
	{
		hitInst = RB_Hazard_CollideWithBucket(inst, t, GAME_TRACKER->threadBuckets[TRACKING].thread, tw->parentSafetyFrames, 0x2400, tw->instParent);
		if (hitInst != NULL)
		{
			// Bombs hit missiles, but pass through warpballs.
			if (hitInst->model->id != DYNAMIC_ROCKET)
			{
				return;
			}
			RB_MovingExplosive_NotifyCollision(hitInst->thread, t);
			RB_MovingExplosive_Explode(t, inst, tw);
			return;
		}
		if ((tw->flags & 2) != 0)
		{
		explode:
			RB_MovingExplosive_Explode(t, inst, tw);
			return;
		}
	}
	if (tw->parentSafetyFrames != 0)
	{
		tw->parentSafetyFrames--;
	}
}

void RB_MovingExplosive_CollideDriver(struct Thread *t, struct Thread *driverThread)
{
	if (Chaos_DeferImpact(CHAOS_IMPACT_MOVING_DRIVER, t, driverThread, NULL))
	{
		return;
	}
	struct TrackerWeapon *tw = t->object;
	struct Driver *driver = driverThread->object;
	driver->damageColorTimer = 0x1e;
	if (driver == tw->driverTarget)
	{
		tw->flags |= 0x10;
	}
	RB_MovingExplosive_Explode(t, t->inst, tw);
}

void RB_MovingExplosive_Explode(struct Thread *t, struct Instance *inst, struct TrackerWeapon *tw)
{
	struct Driver *d;

	// bomb
	if (inst->model->id == DYNAMIC_BOMB)
	{
		// bomb explode
		tw->driverParent->instBombThrow = NULL;
		PlaySound3D(0x49, inst);
	}

	// missile
	else
	{
		d = tw->driverTarget;
		if (d != NULL)
		{
			// remove 2D square-target being drawn on the player's screen
			d->actionsFlagSet &= ~ACTION_TRACKER_TARGETED;
		}
		// missile explode
		PlaySound3D(0x4c, inst);
	}

	// stop audio of rolling
	OtherFX_RecycleMute(&tw->soundIDCount);

	RB_Burst_Init(inst);

	// This thread is now dead
	t->flags |= THREAD_FLAG_DEAD;
	return;
}
