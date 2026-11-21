#include "RB_Pickup.h"

char rb_crateExplosionName[28] = "explosion1";

void RB_CrateAny_ThTick_Explode(struct Thread *t)
{
	// The break-up visual has its own instance and thread, separate from the solid crate.
	struct Instance *crateExplodeInst = t->inst;

	for (;;)
	{
		if ((crateExplodeInst->animFrame + 1) < INSTANCE_GetNumAnimFrames(crateExplodeInst, 0))
		{
			crateExplodeInst->animFrame++;
		}
		else
		{
			t->flags |= THREAD_FLAG_DEAD;
			INSTANCE_Death(crateExplodeInst);
		}

		ThTick_FastRET(t);
		// NOTE(aalhendi): Native ticks return as callbacks; retail resumes after the yield.
		return;
	}
}

void RB_CrateAny_ThTick_Grow(struct Thread *t)
{
	struct Instance *crateInst;
	struct Crate *crateObj;
	int modelID;

	crateObj = (struct Crate *)t->object;
	crateInst = t->inst;
	for (;;)
	{
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
				crateObj->cooldown = (u32)crateObj->cooldown - 1;
			}
		}
		else if (crateInst->scale.x < 0x1000)
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

		ThTick_FastRET(t);
		// NOTE(aalhendi): Native ticks return as callbacks; retail resumes after the yield.
		return;
	}
}

static inline struct Thread *RB_CrateAny_LInC_Birth(const char *debugName)
{
	return PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Crate), NONE, SMALL, STATIC),

	    RB_CrateAny_ThTick_Grow, // behavior
	    debugName,               // debug name
	    0                        // thread relative
	);
}

static inline void RB_CrateAny_LInC_Init(struct Instance *crateInst, struct Thread *crateThread, void *funcThCollide)
{
	struct Crate *crateObj = crateThread->object;
	crateThread->inst = crateInst;
	crateThread->funcThCollide = funcThCollide;

	crateObj->cooldown = 0;
	crateObj->boolPauseCooldown = 0;
}

static int RB_CrateAny_Collide(struct Instance *crateInst, struct Thread *collidingTh, struct ScratchpadStruct *sps)
{
	struct Thread *crateThread = crateInst->thread;
	const bool directPickup = sps->Input1.modelID == DYNAMIC_PLAYER && crateInst->scale.x == 0x1000;
	const int result = ((ThreadScratchCollideFunc)crateThread->funcThCollide)(
	    crateThread, collidingTh, crateThread->funcThCollide, sps);
	if (directPickup && result == 1 && crateInst->scale.x == 0)
	{
		Chaos_OnPickup(collidingTh->object);
	}
	return result;
}

int RB_CrateWeapon_ThCollide(struct Thread *crateThread, struct Thread *collidingTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	struct InstanceBirthParams birth;
	SVec3 rot;
	MATRIX matrix;
	SVec4 posWorld;
	union RBPickupScreen posScreen;
	struct Instance *crateInst;
	struct Instance *explosionInst;
	struct Crate *crateObj;
	struct Driver *driver;
	(void)funcThCollide;

	crateObj = crateThread->object;
	crateInst = crateThread->inst;
	if (Chaos_IsHardCrateForDriver(crateInst, collidingTh, sps))
	{
		return COLL_HITBOX_FORCE_SOLID;
	}

	if ((crateObj->cooldown == 0) && ((crateInst->scale.x == 0) || (crateInst->scale.x == 0x1000)))
	{
		crateObj->cooldown = 0x1e;

		if (crateInst->scale.x == 0x1000)
		{
			crateInst->scale.x = 0;
			crateInst->scale.y = 0;
			crateInst->scale.z = 0;
			birth.modelID = STATIC_CRATE_EXPLOSION;
			birth.name = rb_crateExplosionName;
			birth.poolType = SMALL;
			birth.bucket = OTHER;
			birth.funcThTick = RB_CrateAny_ThTick_Explode;
			birth.objSize = 0;
			birth.parent = NULL;
			explosionInst = INSTANCE_BirthWithThread_Stack(&birth);
			explosionInst->colorRGBA = 0xfafafa0;
			explosionInst->alphaScale = 0x1000;
			rot.x = 0;
			rot.y = rand() % 0x1000;
			rot.z = 0;
			explosionInst->matrix.t[0] = crateInst->matrix.t[0];
			explosionInst->matrix.t[1] = crateInst->matrix.t[1];
			explosionInst->matrix.t[2] = crateInst->matrix.t[2];
			ConvertRotToMatrix(&matrix, &rot);
			MatrixRotate(&explosionInst->matrix, &crateInst->matrix, &matrix);
			PlaySound3D(HOWL_SFX_TIME_CRATE_BREAK, crateInst);

			if ((sps->Input1.modelID != DYNAMIC_BOMB) && (sps->Input1.modelID != DYNAMIC_ROCKET) && (sps->Input1.modelID != DYNAMIC_PLAYER) &&
			    (sps->Input1.modelID != DYNAMIC_SHIELD) && (sps->Input1.modelID != DYNAMIC_SHIELD_GREEN))
			{
				return 1;
			}
			if (sps->Input1.modelID != DYNAMIC_PLAYER)
			{
				driver = ((struct TrackerWeapon *)collidingTh->object)->driverParent;
				if ((driver->actionsFlagSet & ACTION_BOT) != 0)
				{
					return 1;
				}
			}
			else
			{
				driver = collidingTh->object;
				Chaos_ApplyCrateRules(driver, crateInst);
			}

			if ((driver->heldItemID != HELD_ITEM_NONE) && (driver->noItemTimer == 0))
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

			// A received clock normally suppresses the roulette reward, but still consumes the crate.
			if (driver->clockReceive == 0 || g_config.allowWeaponsDuringClock)
			{
				driver->heldItemID = HELD_ITEM_ROULETTE;
				driver->numTimesHitWeaponBox++;
				driver->itemRollTimer = 90;

				if ((GAME_TRACKER->gameMode1 & ROLLING_ITEM) == 0)
				{
					OtherFX_Play(HOWL_SFX_ITEM_ROULETTE, 0);
					GAME_TRACKER->gameMode1 |= ROLLING_ITEM;
				}

				driver->PickupTimeboxHUD.cooldown = 5;
				driver->noItemTimer = 0;

				if (driver->numWumpas == DRIVER_WUMPA_JUICED_COUNT)
				{
					driver->BattleHUD.juicedUpCooldown = DRIVER_WUMPA_JUICED_HUD_COOLDOWN_FRAMES;
				}

				posWorld.x = (s16)crateInst->matrix.t[0];
				posWorld.y = (s16)crateInst->matrix.t[1];
				posWorld.z = (s16)crateInst->matrix.t[2];
				RB_Pickup_SetCamera(driver);
				CTR_GteLoadPositionV0(&posWorld);
				gte_rtps();
				CTR_GteStorePositionXY(posScreen.coords);

				driver->PickupTimeboxHUD.startX = posScreen.coords[0] + GAME_TRACKER->pushBuffer[driver->driverID].rect.x;
				driver->PickupTimeboxHUD.startY = posScreen.coords[1] + GAME_TRACKER->pushBuffer[driver->driverID].rect.y;
			}

			return 1;
		}
	}

	if ((sps->Input1.modelID & COLL_MODELID_BLOCKAGE_FLAG) != 0)
	{
		s32 blockageModel;
		sps->Input1.modelID &= COLL_MODELID_VALUE_MASK;
		blockageModel = sps->Input1.modelID;
		if ((blockageModel == PU_EXPLOSIVE_CRATE) || (blockageModel == STATIC_CRATE_TNT) || (blockageModel == STATIC_BEAKER_RED) ||
		    (blockageModel == STATIC_BEAKER_GREEN))
		{
			crateObj->boolPauseCooldown = 1;
		}
	}
	return 0;
}

int RB_CrateWeapon_LInC(struct Instance *crateInst, struct Thread *collidingTh, struct ScratchpadStruct *sps)
{
	struct Thread *crateThread;
	s32 result;

	crateThread = crateInst->thread;
	if (crateThread == NULL)
	{
		crateInst->thread = RB_CrateAny_LInC_Birth("crate");
		crateThread = crateInst->thread;
		if (crateThread == NULL)
		{
			return 0;
		}
		RB_CrateAny_LInC_Init(crateInst, crateThread, (void *)RB_CrateWeapon_ThCollide);
	}

	if (crateThread->funcThCollide == NULL)
	{
		result = 0;
	}

	else
	{
		result = RB_CrateAny_Collide(crateInst, collidingTh, sps);
	}
	return result;
}

int RB_CrateFruit_ThCollide(struct Thread *crateThread, struct Thread *collidingTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	struct InstanceBirthParams birth;
	SVec4 posWorld;
	union RBPickupScreen playerScreen;
	union RBPickupScreen weaponScreen;
	struct Instance *crateInst;
	struct Instance *explosionInst;
	struct Crate *crateObj;
	s32 hitModelID;
	struct Driver *driver;
	s32 newWumpa;
	s32 random;
	s32 quotient;
	// NOTE(aalhendi): A local collision context preserves retail's allocation across the break-up calls.
	struct ScratchpadStruct *collision = sps;
	(void)funcThCollide;

	crateObj = crateThread->object;
	crateInst = crateThread->inst;
	if (Chaos_IsHardCrateForDriver(crateInst, collidingTh, sps))
	{
		return COLL_HITBOX_FORCE_SOLID;
	}

	if ((crateObj->cooldown == 0) && ((crateInst->scale.x == 0) || (crateInst->scale.x == 0x1000)))
	{
		crateObj->cooldown = 0x1e;
		if (crateInst->scale.x == 0x1000)
		{
			crateInst->scale.x = 0;
			crateInst->scale.y = 0;
			crateInst->scale.z = 0;
			birth.modelID = STATIC_CRATE_EXPLOSION;
			birth.name = rb_crateExplosionName;
			birth.poolType = SMALL;
			birth.bucket = OTHER;
			birth.funcThTick = RB_CrateAny_ThTick_Explode;
			birth.objSize = 0;
			birth.parent = NULL;
			explosionInst = INSTANCE_BirthWithThread_Stack(&birth);
			explosionInst->colorRGBA = 0xf2953a0;
			explosionInst->alphaScale = 0x1000;
			explosionInst->matrix = crateInst->matrix;
			PlaySound3D(HOWL_SFX_TIME_CRATE_BREAK, crateInst);

			// NOTE(aalhendi): Breaking the crate advances the RNG even for an unrecognized hitter.
			random = MixRNG_Scramble();
			quotient = random / 4;
			newWumpa = random - quotient * 4 + 5;
			hitModelID = collision->Input1.modelID;
			if (hitModelID == DYNAMIC_PLAYER)
			{
				driver = collidingTh->object;
				Chaos_ApplyCrateRules(driver, crateInst);

				driver->PickupWumpaHUD.cooldown = 5;
				driver->PickupWumpaHUD.numCollected = newWumpa;

				posWorld.x = (s16)driver->instSelf->matrix.t[0];
				posWorld.y = (s16)driver->instSelf->matrix.t[1];
				posWorld.z = (s16)driver->instSelf->matrix.t[2];
				RB_Pickup_SetCamera(driver);
				CTR_GteLoadPositionV0(&posWorld);
				gte_rtps();
				CTR_GteStorePositionXY(playerScreen.coords);

				driver->PickupWumpaHUD.startX = playerScreen.coords[0] + GAME_TRACKER->pushBuffer[driver->driverID].rect.x;
				driver->PickupWumpaHUD.startY = playerScreen.coords[1] + GAME_TRACKER->pushBuffer[driver->driverID].rect.y - 0x14;
			}
			else if ((hitModelID == DYNAMIC_BOMB) || (hitModelID == DYNAMIC_ROCKET) || (hitModelID == DYNAMIC_SHIELD) || (hitModelID == DYNAMIC_SHIELD_GREEN))
			{
				driver = ((struct TrackerWeapon *)collidingTh->object)->driverParent;

				driver->PickupWumpaHUD.cooldown = 5;
				driver->PickupWumpaHUD.numCollected = newWumpa;

				posWorld.x = (s16)driver->instSelf->matrix.t[0];
				posWorld.y = (s16)driver->instSelf->matrix.t[1];
				posWorld.z = (s16)driver->instSelf->matrix.t[2];
				RB_Pickup_SetCamera(driver);
				CTR_GteLoadPositionV0(&posWorld);
				gte_rtps();
				CTR_GteStorePositionXY(weaponScreen.coords);

				driver->PickupWumpaHUD.startX = weaponScreen.coords[0] + GAME_TRACKER->pushBuffer[driver->driverID].rect.x;
				driver->PickupWumpaHUD.startY = weaponScreen.coords[1] + GAME_TRACKER->pushBuffer[driver->driverID].rect.y - 0x14;
			}
			return 1;
		}
	}

	if ((collision->Input1.modelID & COLL_MODELID_BLOCKAGE_FLAG) != 0)
	{
		s32 blockageModel;
		collision->Input1.modelID &= COLL_MODELID_VALUE_MASK;
		blockageModel = collision->Input1.modelID;
		if ((blockageModel == PU_EXPLOSIVE_CRATE) || (blockageModel == STATIC_CRATE_TNT) || (blockageModel == STATIC_BEAKER_RED) ||
		    (blockageModel == STATIC_BEAKER_GREEN))
		{
			crateObj->boolPauseCooldown = 1;
		}
	}
	return 0;
}

int RB_CrateFruit_LInC(struct Instance *crateInst, struct Thread *collidingTh, struct ScratchpadStruct *sps)
{
	struct Thread *crateThread;
	s32 result;

	crateThread = crateInst->thread;
	if (crateThread == NULL)
	{
		crateInst->thread = RB_CrateAny_LInC_Birth("fruit_crate");
		crateThread = crateInst->thread;
		if (crateThread == NULL)
		{
			return 0;
		}
		RB_CrateAny_LInC_Init(crateInst, crateThread, (void *)RB_CrateFruit_ThCollide);
	}

	if (crateThread->funcThCollide == NULL)
	{
		result = 0;
	}

	else
	{
		result = RB_CrateAny_Collide(crateInst, collidingTh, sps);
	}
	return result;
}

int RB_CrateTime_ThCollide(struct Thread *crateThread, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	struct InstanceBirthParams birth;
	SVec3 rot;
	MATRIX matrix;
	SVec4 posWorld;
	union RBPickupScreen posScreen;
	struct Instance *crateInst;
	struct Instance *explosionInst;
	struct Crate *crateObj;
	struct Driver *driver;
	int modelID;
	(void)funcThCollide;

	crateObj = crateThread->object;
	crateInst = crateThread->inst;
	if (Chaos_IsHardCrateForDriver(crateInst, driverTh, sps))
	{
		return COLL_HITBOX_FORCE_SOLID;
	}

	if ((crateObj->cooldown == 0) && ((crateInst->scale.x == 0) || (crateInst->scale.x == 0x1000)))
	{
		crateObj->cooldown = 0x1e;

		if (crateInst->scale.x == 0x1000)
		{
			crateInst->scale.x = 0;
			crateInst->scale.y = 0;
			crateInst->scale.z = 0;
			birth.modelID = STATIC_CRATE_EXPLOSION;
			birth.name = rb_crateExplosionName;
			birth.poolType = SMALL;
			birth.bucket = OTHER;
			birth.funcThTick = RB_CrateAny_ThTick_Explode;
			birth.objSize = 0;
			birth.parent = NULL;
			explosionInst = INSTANCE_BirthWithThread_Stack(&birth);
			explosionInst->colorRGBA = 0x80ff000;
			explosionInst->alphaScale = 0x1000;
			rot.x = 0;
			rot.y = rand() % 0x1000;
			rot.z = 0;
			explosionInst->matrix.t[0] = crateInst->matrix.t[0];
			explosionInst->matrix.t[1] = crateInst->matrix.t[1];
			explosionInst->matrix.t[2] = crateInst->matrix.t[2];
			ConvertRotToMatrix(&matrix, &rot);
			MatrixRotate(&explosionInst->matrix, &crateInst->matrix, &matrix);
			PlaySound3D(HOWL_SFX_TIME_CRATE_BREAK, crateInst);

			if ((sps->Input1.modelID != DYNAMIC_BOMB) && (sps->Input1.modelID != DYNAMIC_ROCKET) && (sps->Input1.modelID != DYNAMIC_PLAYER) &&
			    (sps->Input1.modelID != DYNAMIC_SHIELD) && (sps->Input1.modelID != DYNAMIC_SHIELD_GREEN))
			{
				// Unknown hitters consume a time crate permanently, without awarding time.
				goto pauseCooldown;
			}
			if (sps->Input1.modelID != DYNAMIC_PLAYER)
			{
				driver = ((struct TrackerWeapon *)driverTh->object)->driverParent;
				if ((driver->actionsFlagSet & ACTION_BOT) != 0)
				{
					return 1;
				}
			}
			else
			{
				driver = driverTh->object;
				Chaos_ApplyCrateRules(driver, crateInst);
			}

			driver->numTimeCrates++;
			modelID = crateInst->model->id;

			if (modelID == STATIC_TIME_CRATE_01)
			{
				GAME_TRACKER->frozenTimeRemaining += 0x3C0;
				GAME_TRACKER->timeCrateTypeSmashed = 1;
			}

			else if (modelID == STATIC_TIME_CRATE_02)
			{
				GAME_TRACKER->frozenTimeRemaining += 0x780;
				GAME_TRACKER->timeCrateTypeSmashed = 2;
			}

			else
			{
				Voiceline_RequestPlayOwned(0x13, GAME_CHARACTER_IDS[driver->driverID], 0x10, driver);
				GAME_TRACKER->frozenTimeRemaining += 0xb40;
				GAME_TRACKER->timeCrateTypeSmashed = 3;
			}

			driver->PickupTimeboxHUD.cooldown = 10;

			posWorld.x = (s16)crateInst->matrix.t[0];
			posWorld.y = (s16)crateInst->matrix.t[1];
			posWorld.z = (s16)crateInst->matrix.t[2];
			RB_Pickup_SetCamera(driver);
			CTR_GteLoadPositionV0(&posWorld);
			gte_rtps();
			CTR_GteStorePositionXY(posScreen.coords);

			driver->PickupTimeboxHUD.startX = posScreen.coords[0] + GAME_TRACKER->pushBuffer[driver->driverID].rect.x;
			driver->PickupTimeboxHUD.startY = posScreen.coords[1] + GAME_TRACKER->pushBuffer[driver->driverID].rect.y;

		pauseCooldown:
			crateObj->boolPauseCooldown = 1;
			return 1;
		}
	}

	if ((sps->Input1.modelID & COLL_MODELID_BLOCKAGE_FLAG) != 0)
	{
		s32 blockageModel;
		sps->Input1.modelID &= COLL_MODELID_VALUE_MASK;
		blockageModel = sps->Input1.modelID;
		if ((blockageModel == PU_EXPLOSIVE_CRATE) || (blockageModel == STATIC_CRATE_TNT) || (blockageModel == STATIC_BEAKER_RED) ||
		    (blockageModel == STATIC_BEAKER_GREEN))
		{
			crateObj->boolPauseCooldown = 1;
		}
	}
	return 0;
}

int RB_CrateTime_LInC(struct Instance *crateInst, struct Thread *driverTh, struct ScratchpadStruct *sps)
{
	struct Thread *crateThread;
	s32 result;

	crateThread = crateInst->thread;
	if (crateThread == NULL)
	{
		crateInst->thread = RB_CrateAny_LInC_Birth("fruit_crate");
		crateThread = crateInst->thread;
		if (crateThread == NULL)
		{
			return 0;
		}
		RB_CrateAny_LInC_Init(crateInst, crateThread, (void *)RB_CrateTime_ThCollide);
	}

	if (crateThread->funcThCollide == NULL)
	{
		result = 0;
	}

	else
	{
		result = RB_CrateAny_Collide(crateInst, driverTh, sps);
	}
	return result;
}
