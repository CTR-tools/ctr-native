#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800aca50-0x800acb60.
void RB_GenericMine_LInB(struct Instance *inst)
{
	struct Thread *t;
	struct GameTracker *gGT;
	struct MineWeapon *mw;
	struct Instance *parentInst;

	gGT = sdata->gGT;

	RB_Default_LInB(inst);

	if (inst->thread != NULL)
	{
		return;
	}

	// if loaded Rocky Road or Nitro Court
	// from outside crystal challenge, skip boxes
	if ((gGT->gameMode1 & CRYSTAL_CHALLENGE) == 0)
	{
		return;
	}

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct MineWeapon), NONE, SMALL, MINE),

	    RB_GenericMine_ThTick, // behavior
	    "nitro",               // debug name
	    0                      // thread relative
	);

	if (t == 0)
	{
		return;
	}
	inst->thread = t;
	t->inst = inst;

	// Do we need the parent?
	// why are these paired to P1 in Crystal Challenge?
	parentInst = gGT->drivers[0]->instSelf;

	t->funcThCollide = RB_Hazard_ThCollide_Generic;
	t->parentThread = parentInst->thread;
	t->modelIndex = inst->model->id;

	mw = t->object;
	mw->instParent = parentInst;
	mw->driverTarget = NULL;
	mw->crateInst = NULL;
	mw->velocity.x = 0;
	mw->velocity.y = 0;
	mw->velocity.z = 0;
	mw->boolDestroyed = 0;
	mw->frameCount_DontHurtParent = 0;
	mw->extraFlags = 0;
	mw->stopFallAtY = inst->matrix.t[1];

	RB_MinePool_Add(mw);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800acb60-0x800ad250.
void RB_GenericMine_ThTick(struct Thread *t)
{
	struct GameTracker *gGT;
	struct Instance *inst;
	struct Instance *coll;
	struct MineWeapon *mw;
	struct Driver *d;
	struct Crate *crate;
	struct Instance *instCrate;
	struct MineWeapon *tnt;
	u32 model;
	int numFrames;
	void (*func)(struct Thread *);
	int param;
	b32 boolPotion;

	gGT = sdata->gGT;
	inst = t->inst;
	mw = inst->thread->object;
	model = inst->model->id;

	boolPotion = (u32)(model - 0x46) < 2;

	// if weapon is "thrown" like Komodo Joe
	if ((mw->extraFlags & 2) != 0)
	{
		if (model == STATIC_CRATE_TNT)
		{
			func = RB_TNT_ThTick_ThrowOffHead;

			// set scale (x, y, z)
			inst->scale.x = 0x800;
			inst->scale.y = 0x800;
			inst->scale.z = 0x800;
		}
		else
		{
			// cooldown of 0.24s
			mw->cooldown = 0xf0;

			func = RB_Potion_ThTick_InAir;
		}

		// this also quits the function
		ThTick_SetAndExec(t, func);
		return;
	}

	// === If not "thrown" ===

	// reduce cooldown
	mw->cooldown -= gGT->elapsedTimeMS;

	if (mw->cooldown < 0)
	{
		mw->cooldown = 0;
	}

	numFrames = (int)INSTANCE_GetNumAnimFrames(inst, 0);

	// if animation is not over
	if (inst->animFrame < numFrames - 1)
	{
		// increment animation frame
		inst->animFrame++;
	}
	// if animation is over
	else
	{
		// restart animation
		inst->animFrame = 0;
	}

	// increment posY by velY * time
	// do NOT use parenthesis
	inst->matrix.t[1] += (mw->velocity.y * gGT->elapsedTimeMS) >> 5;

	if (inst->matrix.t[1] < mw->stopFallAtY)
	{
		inst->matrix.t[1] = mw->stopFallAtY;
	}

	// decrease velocity by time, this is artificial gravity (negative acceleration)
	mw->velocity.y -= ((gGT->elapsedTimeMS << 2) >> 5);

	// terminal velocity
	if (mw->velocity.y < -0x60)
	{
		mw->velocity.y = -0x60;
	}

	// If scale is not big enough
	if (inst->scale.x < 0x1000)
	{
		// make scale larger each frame
		inst->scale.x += 0x200;
		inst->scale.y += 0x200;
		inst->scale.z += 0x200;
	}
	else
	{
		inst->scale.x = 0x1000;
		inst->scale.y = 0x1000;
		inst->scale.z = 0x1000;
	}

	param = 0x3840;

	// red beaker or green beaker
	if (boolPotion != 0)
	{
		param = 0x1900;
	}

	coll = RB_Hazard_CollideWithDrivers(inst, mw->frameCount_DontHurtParent, param, mw->instParent);

	// if no collision
	if (coll == 0)
	{
		goto LAB_800ad17c;
	}

	// get driver who hit tnt (or nitro)
	// from the object attached to thread
	d = coll->thread->object;

	if (((mw->crateInst != 0) && (mw->crateInst->thread != 0)) && (mw->crateInst->thread->object != 0))
	{
		crate = mw->crateInst->thread->object;
		crate->boolPauseCooldown = 0;
	}

	// red beaker or green beaker
	if (boolPotion != 0)
	{
		// count times hit by motionless potion
		param = 2;

		// if fly-forward timer is still active
		if (mw->cooldown != 0)
		{
			// moving potion
			param = 4;
		}

		// spin driver
		coll = (struct Instance *)RB_Hazard_HurtDriver(d, 1, mw->instParent->thread->object, param);

		// if collision, and if this was a red potion
		if ((coll != 0) && (mw->extraFlags & 1) != 0)
		{
			RB_RainCloud_Init(d);
		}

		// if this driver is not an AI
		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			// current fade value (bright white)
			gGT->pushBuffer[d->driverID].fadeFromBlack_currentValue = 0x1fff;

			// desired fade value (neutral)
			gGT->pushBuffer[d->driverID].fadeFromBlack_desiredResult = 0x1000;

			// fade step
			gGT->pushBuffer[d->driverID].fade_step = 0xff78;
		}

		// make player icon red
		// If this is a red beaker
		param = 0x1e;

		// green beaker
		if (model == 0x47)
		{
			// make player icon green
			param = -0x1e;
		}

	LAB_800ace88:

		// set icon damage timer
		d->damageColorTimer = param;

	LAB_800ad174:

		RB_GenericMine_ThDestroy(t, inst, mw);
	}

	// TNT/Nitro
	else
	{
		// if driver->instTntRecv is valid
		if (d->instTntRecv != NULL)
		{
			// blasted driver
			RB_Hazard_HurtDriver(d, 2, 0, 2);

			// icon damage timer, draw icon as red
			d->damageColorTimer = 0x1e;

			// set scale (x, y, z) to zero
			d->instTntRecv->scale.x = 0;
			d->instTntRecv->scale.y = 0;
			d->instTntRecv->scale.z = 0;

			d->instTntRecv->flags |= HIDE_MODEL;

			// this thread is now dead
			d->instTntRecv->thread->flags |= THREAD_FLAG_DEAD;

			// erase instTntRecv
			d->instTntRecv = 0;

			goto LAB_800ad174;
		}

		// if driver has squished timer
		if (d->squishTimer != 0)
		{
			RB_Hazard_HurtDriver(d, 2, 0, 2);

			param = 0x1e;
			goto LAB_800ace88;
		}

		// if model is Nitro
		if (model == 6)
		{
			RB_Hazard_HurtDriver(d, 2, mw->instParent->thread->object, 2);

			// Why does icon turn red in gameplay?

			// icon damage timer, draw icon as green
			d->damageColorTimer = -0x1e;

			goto LAB_800ad174;
		}

		// if model is TNT
		if (model == 0x27)
		{
			// RB_Hazard_HurtDriver (keep driving?)
			crate = (struct Crate *)RB_Hazard_HurtDriver(d, 0, mw->instParent->thread->object, 2);

			if (crate == 0)
			{
				goto LAB_800ad174;
			}

			// if Instance has no InstDef,
			// if this TNT is not part of the level,
			// use existing thread
			if (inst->instDef == 0)
			{
				// icon damage timer, draw icon as red
				d->damageColorTimer = 0x1e;

				// give driver to tnt object
				mw->driverTarget = d;

				// driver -> instTntRecv
				d->instTntRecv = inst;

				RB_MinePool_Remove(mw);

				// play Hit TNT "bounce" sound
				PlaySound3D(0x50, inst);

				inst->bitCompressed_NormalVector_AndDriverIndex = 0;
				inst->flags |= PIXEL_LOD;
				mw->velocity.y = 0x30;
				mw->velocity.x = 0;
				mw->velocity.z = 0;
				mw->deltaPos.x = 0;
				mw->deltaPos.y = 0;
				mw->deltaPos.z = 0;
				mw->stopFallAtY = 0x3fff;

				ThTick_SetAndExec(t, RB_TNT_ThTick_ThrowOnHead);
				return;
			}

			// if this TNT has an InstDef, then it is part of LEV,
			// kill the old thread and birth a new one (why?)
			else
			{
				// DAT_800ab9fc
				// "tnt1"

				// create thread for TNT, get an Instance
				instCrate = INSTANCE_BirthWithThread(0x27, sdata->s_tnt1, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);

				instCrate->matrix = inst->matrix;

				instCrate->thread->funcThDestroy = PROC_DestroyInstance;

				instCrate->thread->funcThCollide = RB_Hazard_ThCollide_Generic;

				// Get object from thread
				tnt = instCrate->thread->object;

				tnt->instParent = d->instSelf;

				tnt->frameCount_DontHurtParent = 10;
				tnt->boolDestroyed = 0;
				tnt->tntSpinY = 0;
				tnt->crateInst = 0;
				tnt->extraFlags = 0;

				// give driver to tnt object
				tnt->driverTarget = d;

				// stopFallAtY (where it explodes)
				tnt->stopFallAtY = inst->matrix.t[1];

				// driver -> instTntRecv
				d->instTntRecv = instCrate;

				// TNT bounce sound
				PlaySound3D(0x50, instCrate);

				instCrate->bitCompressed_NormalVector_AndDriverIndex = 0;
				instCrate->flags |= PIXEL_LOD;
				tnt->velocity.x = 0;
				tnt->velocity.y = 0x30;
				tnt->velocity.z = 0;
				tnt->deltaPos.x = 0;
				tnt->deltaPos.y = 0;
				tnt->deltaPos.z = 0;
				instCrate->thread->funcThTick = RB_TNT_ThTick_ThrowOnHead;

				RB_MinePool_Remove(mw);

				// set scale (x, y, z) to zero
				inst->scale.x = 0;
				inst->scale.y = 0;
				inst->scale.z = 0;

				// make invisible
				inst->flags |= HIDE_MODEL;

				// this thread is now dead
				t->flags |= THREAD_FLAG_DEAD;
			}
		}
	}
LAB_800ad17c:

	if (mw->frameCount_DontHurtParent != 0)
	{
		mw->frameCount_DontHurtParent--;
	}

	// if mineWeapon->boolDestroyed == 0
	if (mw->boolDestroyed == 0)
	{
		return;
	}

	// if thread is dead, quit function
	// this is if GenericMine_ThDestroy already ran
	if ((t->flags & THREAD_FLAG_DEAD) != 0)
	{
		return;
	}

	// === If destroyed from MinePool overflow ===

	// instance -> model -> modelID
	model = inst->model->id;

	if (model == PU_EXPLOSIVE_CRATE)
	{
		// glass shatter sound
		PlaySound3D(0x3f, inst);

		RB_Blowup_Init(inst);
	}
	else if (model == STATIC_CRATE_TNT)
	{
		// tnt explosion sound
		PlaySound3D(0x3d, inst);

		RB_Blowup_Init(inst);
	}
	else if (boolPotion != 0)
	{
		// glass shatter sound
		PlaySound3D(0x3f, inst);

		RB_Explosion_InitPotion(inst);
	}

	// this thread is now dead
	t->flags |= THREAD_FLAG_DEAD;

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ad250-0x800ad310.
void RB_GenericMine_ThDestroy(struct Thread *t, struct Instance *inst, struct MineWeapon *mw)
{
	u32 model;
	u16 param;

	model = inst->model->id;

	if (model == PU_EXPLOSIVE_CRATE)
	{
		// glass shatter
		param = 0x3f;

		PlaySound3D(param, inst);

		RB_Blowup_Init(inst);
	}
	else if (model == STATIC_CRATE_TNT)
	{
		// tnt explosion sound
		param = 0x3d;

		PlaySound3D(param, inst);

		RB_Blowup_Init(inst);
	}
	else
	{
		// play sound of glass shatter
		PlaySound3D(0x3f, inst);

		RB_Explosion_InitPotion(inst);
	}

	// Set scale (x, y, z) to zero
	inst->scale.x = 0;
	inst->scale.y = 0;
	inst->scale.z = 0;

	// make invisible
	inst->flags |= HIDE_MODEL;

	RB_MinePool_Remove(mw);

	// this thread is now dead
	t->flags |= THREAD_FLAG_DEAD;

	return;
}
