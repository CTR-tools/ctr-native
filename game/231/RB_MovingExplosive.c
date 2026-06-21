#include <common.h>

typedef int (*MovingExplosiveCollideFunc)(struct Thread *, struct Thread *, void *, struct ScratchpadStruct *);

static void RB_MovingExplosive_CallThCollide(struct Thread *hitTh, struct Thread *sourceTh)
{
	void *funcThCollide = (void *)hitTh->funcThCollide;
	((MovingExplosiveCollideFunc)funcThCollide)(hitTh, sourceTh, funcThCollide, NULL);
}

// function for moving bomb, shiledbomb, or missile
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800adb50-0x800ae478.
void RB_MovingExplosive_ThTick(struct Thread *t)
{
	s16 sVar1;
	s16 sVar3;
	struct GameTracker *gGT = sdata->gGT;
	s16 modelID;
	int iVar6;
	int iVar8;
	u16 sound;
	struct TrackerWeapon *tw;
	struct Instance *inst;
	SVec3 posA;
	SVec3 posB;

	inst = t->inst;
	modelID = inst->model->id;

	tw = t->object;
	tw->timeAlive += gGT->elapsedTimeMS;

	// NOTE(aalhendi): Retail starts/updates the bomb, missile, and shield loop SFX here.
	if (modelID == DYNAMIC_ROCKET)
	{
		if ((t->flags & THREAD_FLAG_DEAD) == 0)
		{
			sound = 0x4b;
			goto LAB_800adc00;
		}
	}
	else
	{
		if (modelID == DYNAMIC_BOMB)
		{
			sound = 0x48;
		}
		else
		{
			if ((modelID != DYNAMIC_SHIELD) && (modelID != DYNAMIC_SHIELD_GREEN))
				goto LAB_800adc08;

			// if model is blue or green shield
			sound = 0x59;
		}
	LAB_800adc00:
		PlaySound3D_Flags(&tw->audioPtr, sound, inst);
	}

LAB_800adc08:;

	struct Driver *driverTarget = tw->driverTarget;

	// NOTE(aalhendi): Native guard for retail's PS1 null-space shieldbomb path.
	// driver not invisible
	if ((driverTarget != 0) && (driverTarget->invisibleTimer == 0))
	{
		if (
		    /* ghidra snippet for this if statement's conditions.

		    (*(int *)(*piVar9 + 0x2cc) < 0) &&
		    // instance -> model -> objectID == missile
		    (*(s16 *)(*(int *)(iVar10 + 0x18) + 0x10) == 0x29)

		    */

		    // Naughty Dog Bug? That's so unlikely?
		    // Literally a ONE-frame window

		    // dropped mine previous frame
		    (*((int *)(&driverTarget->actionsFlagSetPrevFrame)) < 0) && // match ghidra behavior

		    (modelID == DYNAMIC_ROCKET))
		{
			tw->framesSeekMine = 10;
		}
	}

	// if driver is invisible
	else
	{
		// erase pointer to driver,
		// cause tracker can't find invisible driver
		tw->driverTarget = 0;
	}

	driverTarget = tw->driverTarget;

	if (
	    // if driver is invalid
	    (driverTarget == 0) || (tw->frameCount_Blind != 0))
	{
		if (tw->frameCount_Blind != 0)
			tw->frameCount_Blind--;
	}
	else
	{
		if (tw->framesSeekMine == 0)
		{
			// get distance between tracker and the driver being chased
			iVar6 = (driverTarget->posCurr.x >> 8) - inst->matrix.t[0];
			iVar8 = (driverTarget->posCurr.z >> 8) - inst->matrix.t[2];
			tw->distanceToTarget = iVar6 * iVar6 + iVar8 * iVar8;

		LAB_800add14:

			// get direction, given X and Y distance to travel
			sVar3 = ratan2(iVar6, iVar8);
		}

		// if seeking mine
		else
		{
			tw->framesSeekMine--;

			// if target shot a TNT
			struct Instance *instTNT = tw->driverTarget->instTntSend;

			if (instTNT != 0)
			{
				// Get X and Y differences between two instances
				iVar6 = instTNT->matrix.t[0] - inst->matrix.t[0];
				iVar8 = instTNT->matrix.t[2] - inst->matrix.t[2];
				goto LAB_800add14;
			}

			// if target never used a TNT
			sVar3 = tw->rotY;
			tw->framesSeekMine = 0;
		}

		if ((modelID == DYNAMIC_BOMB) || (modelID == DYNAMIC_SHIELD))
		{
			tw->rotY = RB_Hazard_InterpolateValue(tw->rotY, (int)sVar3, 4);

			tw->vel.x = (MATH_Sin(tw->rotY) * 3) >> 7;
			tw->vel.z = (MATH_Cos(tw->rotY) * 3) >> 7;

			// if bomb is rolled backwards
			if ((tw->flags & 0x20) != 0)
			{
				tw->vel.z = -tw->vel.z;
				tw->vel.x = -tw->vel.x;
			}
		}

		// if 0x29 (MISSILE)
		else
		{
			// if 10 wumpa were not used
			if ((tw->flags & 1) == 0)
			{
				tw->rotY = RB_Hazard_InterpolateValue(tw->rotY, (int)sVar3, 0x40);

				tw->vel.x = (MATH_Sin(tw->rotY) * 5) >> 8;
				tw->vel.z = (MATH_Cos(tw->rotY) * 5) >> 8;
			}

			// if 10 wumpa were used
			else
			{
				tw->rotY = RB_Hazard_InterpolateValue(tw->rotY, (int)sVar3, 0x80);

				tw->vel.x = (MATH_Sin(tw->rotY) * 3) >> 7;
				tw->vel.z = (MATH_Cos(tw->rotY) * 3) >> 7;
			}

			tw->dir.x = 0;
			tw->dir.z = 0;
			tw->dir.y = tw->rotY;

			// convert 3 rotation shorts into rotation matrix
			ConvertRotToMatrix(&inst->matrix, &tw->dir);
		}
	}

	sVar3 = inst->animFrame;
	iVar8 = INSTANCE_GetNumAnimFrames(inst, 0);

	// if instance is not at end of animation
	if ((int)sVar3 + 1 < iVar8)
	{
		// increment animation frame
		inst->animFrame += 1;
	}

	// if animation finished
	else
	{
		// go back to first frame of animation
		inst->animFrame = 0;
	}

#if defined(CTR_NATIVE)
	if (
	    // if missile
	    (modelID == DYNAMIC_ROCKET) &&

	    // numPlyrCurrGame < 2
	    (sdata->gGT->numPlyrCurrGame < 2))
	{
		// Make Instane in Particle Pool
		struct Particle *p;
		// NOTE(aalhendi): Native uses retail emitter bytes from 0x800b2ae4.
		p = Particle_Init(0, gGT->iconGroup[0], &R231.emSet_Missile[0]);

		if (p != 0)
		{
			p->axis[0].startVal = inst->matrix.t[0] << 8;
			p->axis[1].startVal = inst->matrix.t[1] << 8;
			p->axis[2].startVal = inst->matrix.t[2] << 8;
		}
	}
#endif

	int elapsedTime = gGT->elapsedTimeMS;
	inst->matrix.t[0] += (((int)tw->vel.x * elapsedTime) >> 5);
	inst->matrix.t[1] += (((int)tw->vel.y * elapsedTime) >> 5);
	inst->matrix.t[2] += (((int)tw->vel.z * elapsedTime) >> 5);

	// If this is bomb
	if (modelID == DYNAMIC_BOMB)
	{
		// if bomb is forwards
		if ((tw->flags & 0x20) == 0)
		{
			tw->dir.x += 0x200;
		}

		// if bomb is backwards
		else
		{
			tw->dir.x -= 0x200;
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

	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);

	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_FORCE_INSTANCE_HIT;
	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_TRIGGER;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;

	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD | COLL_SEARCH_FORCE_INSTANCE_HIT;
	}

	sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;

	COLL_SearchBSP_CallbackQUADBLK(&posA, &posB, sps, 0);

	RB_MakeInstanceReflective(sps, inst);

	if ((sps->collision.stepFlags & COLL_STEP_TRIGGER_WEAPON_REACT) != 0)
	{
		// move backward one frame
		tw->vel.x = -tw->vel.x;
		tw->vel.y = -tw->vel.y;
		tw->vel.z = -tw->vel.z;

		inst->matrix.t[0] += ((int)tw->vel.x * elapsedTime) >> 5;
		inst->matrix.t[1] += ((int)tw->vel.y * elapsedTime) >> 5;
		inst->matrix.t[2] += ((int)tw->vel.z * elapsedTime) >> 5;

		RB_MovingExplosive_Explode(t, inst, tw);
		return;
	}

	if (sps->boolDidTouchHitbox == 0)
	{
		if (sps->boolDidTouchQuadblock == 0)
		{
			// if no quadblock is under this item,
			// look again for another quadblock LOWER

			inst->vertSplit = 0;
			posA.x = inst->matrix.t[0];
			posA.y = inst->matrix.t[1] - 0x900;
			posA.z = inst->matrix.t[2];

			COLL_SearchBSP_CallbackQUADBLK(&posA, &posB, sps, 0);

			// if still nothing, then explode
			if (sps->boolDidTouchQuadblock == 0)
				goto LAB_800ae42c;

			// if quadblock under,
			// then set fall rate and fall

			int iVar8 = elapsedTime << 2;

			// if missile
			if (modelID == DYNAMIC_ROCKET)
			{
				iVar8 = elapsedTime << 3;
			}

			tw->vel.y -= (iVar8 >> 5);

			if (tw->vel.y < -0x60)
			{
				tw->vel.y = -0x60;
			}
		}

		// if hit quadblock
		else
		{
			tw->vel.y = 0;

			// missile model
			if (modelID == DYNAMIC_ROCKET)
			{
				VehPhysForce_RotAxisAngle(&inst->matrix, sps->hit.plane.normal.v, tw->rotY);
			}

			// position
			inst->matrix.t[0] = sps->Union.QuadBlockColl.hitPos.x;
			inst->matrix.t[1] = sps->Union.QuadBlockColl.hitPos.y + 0x30;
			inst->matrix.t[2] = sps->Union.QuadBlockColl.hitPos.z;
		}
	}
	else
	{
		sps->Input1.modelID = modelID;

		int ret;
		ret = RB_Hazard_CollLevInst(sps, t);

		// if hit bsp hitbox
		if (ret == 1)
		{
			struct InstDef *instDef;
			struct BSP *bspHitbox = sps->bspHitbox;

			// copy/paste from Potion_InAir
			if ((((bspHitbox->flag & 0x80) != 0) && (instDef = bspHitbox->data.hitbox.instDef, instDef != 0)) &&

			    (((instDef->ptrInstance != 0) && (instDef->modelID == STATIC_TEETH))))
			{
				RB_Teeth_OpenDoor(instDef->ptrInstance);
			}
			goto LAB_800ae42c;
		}
	}

	struct Instance *hitInst;

	hitInst = RB_Hazard_CollideWithDrivers(inst, tw->frameCount_DontHurtParent, 0x2400, tw->instParent);

	// if no driver hit
	if (hitInst == 0)
	{
		// check Mine threadbucket
		hitInst = RB_Hazard_CollideWithBucket(inst, t, gGT->threadBuckets[MINE].thread, tw->frameCount_DontHurtParent, 0x2400, tw->instParent);

		// if mine was not hit
		if (hitInst == 0)
		{
			// instance -> model -> modelID is not bomb
			if (modelID != DYNAMIC_BOMB)
			{
			LAB_800ae440:
				if (tw->frameCount_DontHurtParent != 0)
					tw->frameCount_DontHurtParent--;

				return;
			}

			// === Assume Bomb ===

			// check Tracking threadbucket
			hitInst = RB_Hazard_CollideWithBucket(inst, t, gGT->threadBuckets[TRACKING].thread, tw->frameCount_DontHurtParent, 0x2400, tw->instParent);

			// if no collision
			if (hitInst == 0)
			{
				if ((tw->flags & 2) == 0)
					goto LAB_800ae440;
			}

			// if bomb collides with Tracker
			else
			{
				// not a missile
				if (hitInst->model->id != DYNAMIC_ROCKET)
				{
					// quit, warpball collisions dont matter
					return;
				}

				// === missile ===
				struct Thread *hitTh = hitInst->thread;

				RB_MovingExplosive_CallThCollide(hitTh, t);
			}
		}

		// if mine was hit
		else
		{
			struct Thread *hitTh = hitInst->thread;

			RB_MovingExplosive_CallThCollide(hitTh, t);
		}
	}

	// if driver was hit
	else
	{
		struct Thread *hitTh = hitInst->thread;

		struct Driver *hitD = hitTh->object;

		hitD->damageColorTimer = 0x1e;

		if (hitD == tw->driverTarget)
		{
			// flags
			tw->flags |= 0x10;
		}
	}
LAB_800ae42c:

	RB_MovingExplosive_Explode(t, inst, tw);
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae478-0x800ae524.
void RB_MovingExplosive_Explode(struct Thread *t, struct Instance *inst, struct TrackerWeapon *tw)
{
	s16 soundId;
	struct Driver *d;

	// bomb
	if (inst->model->id == DYNAMIC_BOMB)
	{
		// bomb explode
		soundId = 0x49;
		tw->driverParent->instBombThrow = NULL;
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
		soundId = 0x4c;
	}

	// play explosion sound
	PlaySound3D(soundId, inst);

	// stop audio of rolling
	OtherFX_RecycleMute(&tw->audioPtr);

	RB_Burst_Init(inst);

	// This thread is now dead
	t->flags |= THREAD_FLAG_DEAD;
	return;
}
