#include <common.h>

// add to buildList, overwrite original
// RB_CrateAny_ThTick_Explode at 800b3d04,
// and add new LinCs to zGlobalMetaModels.c

static void RB_CrateAny_CheckBlockage(struct Thread *crateTh, int hitModelIDValue)
{
	struct Crate *crateObj;

	crateObj = crateTh->object;

	// if model is on top of crate
	if ((hitModelIDValue == PU_EXPLOSIVE_CRATE) || // nitro
	    (hitModelIDValue == STATIC_CRATE_TNT) ||   // tnt
	    (hitModelIDValue == STATIC_BEAKER_RED) ||  // red beaker
	    (hitModelIDValue == STATIC_BEAKER_GREEN)   // green beaker
	)
	{
		// prevent crate from growing back
		crateObj->boolPauseCooldown = 1;
	}
}

struct Driver *RB_CrateAny_GetDriver(struct Thread *t, struct ScratchpadStruct *sps)
{
	int hitModelID;
	int hitModelIDValue;
	struct Driver *driver;

	// get what hit the box
	hitModelID = sps->Input1.modelID;
	hitModelIDValue = hitModelID & COLL_MODELID_VALUE_MASK;

	// if moving explosive
	if ((hitModelIDValue == DYNAMIC_BOMB) ||      // bomb
	    (hitModelIDValue == DYNAMIC_ROCKET) ||    // missile
	    (hitModelIDValue == DYNAMIC_SHIELD) ||    // blue shield
	    (hitModelIDValue == DYNAMIC_SHIELD_GREEN) // green shield
	)
	{
		// get driver that used the weapon
		driver = ((struct TrackerWeapon *)t->object)->driverParent;

		// if this is an AI, quit

		// it's odd that it casts "1" as struct Driver*, but callers of this function *do* check the return value == 1, so it must be intentional.
		if ((driver->actionsFlagSet & ACTION_BOT) != 0)
		{
			return (struct Driver *)1;
		}

		return driver;
	}

	// if driver itself
	else if (hitModelIDValue == DYNAMIC_PLAYER) // //player model
	{
		driver = (struct Driver *)t->object;

		return driver;
	}

	// it's odd that it casts "1" as struct Driver*, but callers of this function *do* check the return value == 1, so it must be intentional.
	return (struct Driver *)1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3d04-0x800b3d7c.
void RB_CrateAny_ThTick_Explode(struct Thread *t)
{
	// this is an "exploded" crate, with
	// it's own instance, thread, and object,
	// separate from "solid" crate
	struct Instance *crateExplodeInst = t->inst;

	// if explosion is not over
	if ((crateExplodeInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(crateExplodeInst, 0))
	{
		// increment frame
		crateExplodeInst->animFrame = crateExplodeInst->animFrame + 1;
		return;
	}

	// if explosion is over
	t->flags |= THREAD_FLAG_DEAD;
	INSTANCE_Death(crateExplodeInst);
}

static void RB_CrateAny_ExplodeInit(struct Instance *crateInst, int color, b32 randomizeRotation)
{
	struct Instance *explosionInst;
	MATRIX matrix;
	SVec3 rot;

	// hide crate
	crateInst->scale.x = 0;
	crateInst->scale.y = 0;
	crateInst->scale.z = 0;

	// birth explosion thread
	explosionInst = INSTANCE_BirthWithThread(
	    // 0x26 - box explosion model
	    // 0x0 - debug name
	    0x26, 0,

	    // pool, bucket, ThTick
	    SMALL, OTHER, RB_CrateAny_ThTick_Explode,

	    // PushBuffer and threadRelative
	    0, 0);

	// color
	explosionInst->colorRGBA = color;
	explosionInst->alphaScale = 0x1000;

	if (randomizeRotation)
	{
		// position
		explosionInst->matrix.t[0] = crateInst->matrix.t[0];
		explosionInst->matrix.t[1] = crateInst->matrix.t[1];
		explosionInst->matrix.t[2] = crateInst->matrix.t[2];

		// rotation matrix
		rot.x = 0;
		rot.y = rand() % 0xfff;
		rot.z = 0;
		ConvertRotToMatrix(&matrix, &rot);

		// explosion matrix = rotated crate matrix
		MatrixRotate(&explosionInst->matrix, &crateInst->matrix, &matrix);
	}
	else
	{
		explosionInst->matrix = crateInst->matrix;
	}

	PlaySound3D(0x3c, crateInst);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3d7c-0x800b3e7c.
void RB_CrateAny_ThTick_Grow(struct Thread *t)
{
	struct Instance *crateInst;
	struct Crate *crateObj;
	int modelID;

	crateInst = t->inst;
	crateObj = (struct Crate *)t->object;
	modelID = crateInst->model->id;

	if ((modelID == STATIC_TIME_CRATE_01) || (modelID == STATIC_TIME_CRATE_02) || (modelID == STATIC_TIME_CRATE_03))
	{
		crateInst->thread = 0;
		t->flags |= THREAD_FLAG_DEAD;
	}

	// if cooldown is not done (about a second long)
	if (crateObj->cooldown != 0)
	{
		// if cooldown not paused,
		// (no driver or mine, sitting in the way)
		if (crateObj->boolPauseCooldown == 0)
		{
			// reduce cooldown
			crateObj->cooldown--;
		}

		// dont procede until cooldown is done
		return;
	}

	// == ready to regrow ==

	if (crateInst->scale.x < 0x1000)
	{
		crateInst->scale.x += 0x100;
		crateInst->scale.y += 0x100;
		crateInst->scale.z += 0x100;
	}
	else
	{
		crateInst->scale.x = 0x1000;
		crateInst->scale.y = 0x1000;
		crateInst->scale.z = 0x1000;

		// kill thread
		crateInst->thread = 0;
		crateInst->animFrame++;
		t->flags |= THREAD_FLAG_DEAD;
	}
}

typedef int (*CrateCollideFunc)(struct Thread *, struct Thread *, void *, struct ScratchpadStruct *);

static struct Thread *RB_CrateAny_LInC_Birth(struct Instance *crateInst, void *funcThCollide, char *debugName)
{
	struct Thread *crateThread;
	struct Crate *crateObj;

	crateThread = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Crate), NONE, SMALL, STATIC),

	    RB_CrateAny_ThTick_Grow, // behavior
	    debugName,               // debug name
	    0                        // thread relative
	);

	if (crateThread == 0)
	{
		return 0;
	}

	crateInst->thread = crateThread;
	crateThread->inst = crateInst;
	crateThread->funcThCollide = funcThCollide;

	crateObj = ((struct Crate *)crateThread->object);
	crateObj->cooldown = 0;
	crateObj->boolPauseCooldown = 0;

	return crateThread;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3e7c-0x800b4278.
int RB_CrateWeapon_ThCollide(struct Thread *crateThread, struct Thread *collidingTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	struct PushBuffer *pb;
	s16 posScreen[2];
	struct Instance *crateInst;
	struct Crate *crateObj;
	int hitModelID;
	int hitModelIDValue;
	struct Driver *driver;

	crateInst = crateThread->inst;
	crateObj = ((struct Crate *)crateThread->object);

	if (crateObj->cooldown == 0)
	{
		if ((crateInst->scale.x != 0) && (crateInst->scale.x != 0x1000))
		{
			return 0;
		}

		crateObj->cooldown = 0x1e;

		if (crateInst->scale.x == 0x1000)
		{
			RB_CrateAny_ExplodeInit(crateInst, 0xfafafa0, true);

			driver = RB_CrateAny_GetDriver(collidingTh, sps);
			if ((int)driver == 1)
			{
				return 1;
			}

			if ((driver->heldItemID != 0xf) && (driver->noItemTimer == 0))
			{
				return 1;
			}

			if (driver->numHeldItems != 0)
			{
				return 1;
			}

			if ((driver->actionsFlagSet & ACTION_WEAPON_FIRE_REQUEST) != 0)
			{
				return 1;
			}

			if (driver->thCloud != 0)
			{
				if (((struct RainCloud *)driver->thCloud->object)->effect == RAIN_CLOUD_EFFECT_ITEM_ROLL)
				{
					return 1;
				}
			}

			if (driver->clockReceive != 0)
			{
				return 1;
			}

			driver->heldItemID = 0x10;
			driver->numTimesHitWeaponBox++;
			driver->itemRollTimer = 90;

			if ((sdata->gGT->gameMode1 & ROLLING_ITEM) == 0)
			{
				OtherFX_Play(0x5d, 0);
				sdata->gGT->gameMode1 |= ROLLING_ITEM;
			}

			driver->PickupTimeboxHUD.cooldown = 5;
			driver->noItemTimer = 0;

			if (driver->numWumpas == 10)
			{
				driver->BattleHUD.juicedUpCooldown = 10;
			}

			pb = &sdata->gGT->pushBuffer[driver->driverID];
			RB_Fruit_GetScreenCoords(pb, crateInst, &posScreen[0]);

			driver->PickupTimeboxHUD.startX = pb->rect.x + posScreen[0];
			driver->PickupTimeboxHUD.startY = pb->rect.y + posScreen[1];

			return 1;
		}
	}

	hitModelID = sps->Input1.modelID;
	hitModelIDValue = hitModelID & COLL_MODELID_VALUE_MASK;

	if ((hitModelID & COLL_MODELID_BLOCKAGE_FLAG) == 0)
	{
		return 0;
	}

	sps->Input1.modelID = hitModelIDValue;
	RB_CrateAny_CheckBlockage(crateThread, hitModelIDValue);
	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4278-0x800b432c.
int RB_CrateWeapon_LInC(struct Instance *crateInst, struct Thread *collidingTh, struct ScratchpadStruct *sps)
{
	struct Thread *crateThread;

	crateThread = crateInst->thread;
	if (crateThread == NULL)
	{
		crateThread = RB_CrateAny_LInC_Birth(crateInst, (void *)RB_CrateWeapon_ThCollide, "crate");
		if (crateThread == NULL)
		{
			return 0;
		}
	}

	if (crateThread->funcThCollide == NULL)
	{
		return 0;
	}

	return ((CrateCollideFunc)crateThread->funcThCollide)(crateThread, collidingTh, crateThread->funcThCollide, sps);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b432c-0x800b471c.
int RB_CrateFruit_ThCollide(struct Thread *crateThread, struct Thread *collidingTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	struct PushBuffer *pb;
	s16 posScreen[2];
	struct Instance *crateInst;
	struct Crate *crateObj;
	int hitModelID;
	int hitModelIDValue;
	struct Driver *driver;
	int random;
	int newWumpa;

	crateInst = crateThread->inst;
	crateObj = ((struct Crate *)crateThread->object);

	if (crateObj->cooldown == 0)
	{
		if ((crateInst->scale.x != 0) && (crateInst->scale.x != 0x1000))
		{
			return 0;
		}

		crateObj->cooldown = 0x1e;

		if (crateInst->scale.x == 0x1000)
		{
			RB_CrateAny_ExplodeInit(crateInst, 0xf2953a0, false);

			driver = RB_CrateAny_GetDriver(collidingTh, sps);
			if ((int)driver == 1)
			{
				return 1;
			}

			random = MixRNG_Scramble();
			newWumpa = random;
			if (random < 0)
			{
				newWumpa = random + 3;
			}
			newWumpa = random + (newWumpa >> 2) * -4 + 5;

			driver->PickupWumpaHUD.cooldown = 5;
			driver->PickupWumpaHUD.numCollected = newWumpa;

			pb = &sdata->gGT->pushBuffer[driver->driverID];
			RB_Fruit_GetScreenCoords(pb, driver->instSelf, &posScreen[0]);

			driver->PickupWumpaHUD.startX = pb->rect.x + posScreen[0];
			driver->PickupWumpaHUD.startY = pb->rect.y + posScreen[1] - 0x14;

			return 1;
		}
	}

	hitModelID = sps->Input1.modelID;
	hitModelIDValue = hitModelID & COLL_MODELID_VALUE_MASK;

	if ((hitModelID & COLL_MODELID_BLOCKAGE_FLAG) == 0)
	{
		return 0;
	}

	sps->Input1.modelID = hitModelIDValue;
	RB_CrateAny_CheckBlockage(crateThread, hitModelIDValue);
	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b471c-0x800b47d0.
int RB_CrateFruit_LInC(struct Instance *crateInst, struct Thread *collidingTh, struct ScratchpadStruct *sps)
{
	struct Thread *crateThread;

	crateThread = crateInst->thread;
	if (crateThread == NULL)
	{
		crateThread = RB_CrateAny_LInC_Birth(crateInst, (void *)RB_CrateFruit_ThCollide, "fruit_crate");
		if (crateThread == NULL)
		{
			return 0;
		}
	}

	if (crateThread->funcThCollide == NULL)
	{
		return 0;
	}

	return ((CrateCollideFunc)crateThread->funcThCollide)(crateThread, collidingTh, crateThread->funcThCollide, sps);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b47d0-0x800b4ba8.
int RB_CrateTime_ThCollide(struct Thread *crateThread, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	struct PushBuffer *pb;
	s16 posScreen[2];
	struct Instance *crateInst;
	struct Crate *crateObj;
	struct Driver *driver;
	int modelID;
	int hitModelID;
	int hitModelIDValue;
	struct GameTracker *gGT;

	crateInst = crateThread->inst;
	crateObj = ((struct Crate *)crateThread->object);

	if (crateObj->cooldown == 0)
	{
		if ((crateInst->scale.x != 0) && (crateInst->scale.x != 0x1000))
		{
			return 0;
		}

		crateObj->cooldown = 0x1e;

		if (crateInst->scale.x == 0x1000)
		{
			RB_CrateAny_ExplodeInit(crateInst, 0x80ff000, true);

			gGT = sdata->gGT;
			driver = RB_CrateAny_GetDriver(driverTh, sps);
			if ((int)driver == 1)
			{
				return 1;
			}

			modelID = crateInst->model->id;

			if ((driver->actionsFlagSet & ACTION_BOT) != 0)
			{
				return 1;
			}

			driver->numTimeCrates++;

			if (modelID == STATIC_TIME_CRATE_01)
			{
				gGT->frozenTimeRemaining += 0x3C0;
				gGT->timeCrateTypeSmashed = 1;
			}

			else if (modelID == STATIC_TIME_CRATE_02)
			{
				gGT->frozenTimeRemaining += 0x780;
				gGT->timeCrateTypeSmashed = 2;
			}

			else
			{
				gGT->frozenTimeRemaining += 0xb40;
				gGT->timeCrateTypeSmashed = 3;

				Voiceline_RequestPlay(0x13, data.characterIDs[driver->driverID], 0x10);
			}

			driver->PickupTimeboxHUD.cooldown = 10;

			pb = &gGT->pushBuffer[driver->driverID];
			RB_Fruit_GetScreenCoords(pb, crateInst, &posScreen[0]);

			driver->PickupTimeboxHUD.startX = pb->rect.x + posScreen[0];
			driver->PickupTimeboxHUD.startY = pb->rect.y + posScreen[1];

			crateObj->boolPauseCooldown = 1;
			return 1;
		}
	}

	hitModelID = sps->Input1.modelID;
	hitModelIDValue = hitModelID & COLL_MODELID_VALUE_MASK;

	if ((hitModelID & COLL_MODELID_BLOCKAGE_FLAG) == 0)
	{
		return 0;
	}

	sps->Input1.modelID = hitModelIDValue;
	RB_CrateAny_CheckBlockage(crateThread, hitModelIDValue);
	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4ba8-0x800b4c5c.
int RB_CrateTime_LInC(struct Instance *crateInst, struct Thread *driverTh, struct ScratchpadStruct *sps)
{
	struct Thread *crateThread;

	crateThread = crateInst->thread;
	if (crateThread == NULL)
	{
		crateThread = RB_CrateAny_LInC_Birth(crateInst, (void *)RB_CrateTime_ThCollide, "fruit_crate");
		if (crateThread == NULL)
		{
			return 0;
		}
	}

	if (crateThread->funcThCollide == NULL)
	{
		return 0;
	}

	return ((CrateCollideFunc)crateThread->funcThCollide)(crateThread, driverTh, crateThread->funcThCollide, sps);
}
