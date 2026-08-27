#include <common.h>

static inline void VehPickupItem_CopyMatrix(MATRIX *dst, const MATRIX *src)
{
	dst->m[0][0] = src->m[0][0];
	dst->m[0][1] = src->m[0][1];
	dst->m[0][2] = src->m[0][2];
	dst->m[1][0] = src->m[1][0];
	dst->m[1][1] = src->m[1][1];
	dst->m[1][2] = src->m[1][2];
	dst->m[2][0] = src->m[2][0];
	dst->m[2][1] = src->m[2][1];
	dst->m[2][2] = src->m[2][2];
	dst->t[0] = src->t[0];
	dst->t[1] = src->t[1];
	dst->t[2] = src->t[2];
}

static inline void VehPickupItem_ClearMineMotion(struct MineWeapon *mine)
{
	mine->velocity.x = 0;
	mine->velocity.y = 0;
	mine->velocity.z = 0;
	mine->stopFallAtY = 0;
}

enum
{
	MASK_GOOD_GUY_CHARACTER_BITS = 0x20c9,
	MASK_MODEL_COUNT = 2,
	MASK_SOUND_ID_OFFSET_FROM_MODEL = 0x1a,
	MASK_BEAM_MODEL_STRIDE = 2,
	MASK_INITIAL_ROT_X = 0x40,
};

enum
{
	POTION_THROW_RANDOM = 0x1,
	POTION_THROW_BACKWARD = 0x2,
	POTION_THROW_FORWARD = 0x4,
	POTION_THROW_RANDOM_MASK = 0x1f,
	POTION_THROW_RANDOM_BIAS = 0x10,
	POTION_THROW_SPEED = 0x78,
	POTION_THROW_Y_VELOCITY = 0x30,
	POTION_THROW_MATRIX_SHIFT = 12,
};

enum
{
	MISSILE_TARGET_DRIVER_COUNT = 8,
	MISSILE_TARGET_DISTANCE_SENTINEL = 0x7fffffff,
	MISSILE_TARGET_POS_SHIFT = 8,
	MISSILE_TARGET_GTE_RTPS_OVERFLOW = 0x40000,
	MISSILE_TARGET_SCREEN_LEFT = 0x1f,
	MISSILE_TARGET_SCREEN_RIGHT_MARGIN = 0x1e,
	MISSILE_TARGET_SCREEN_TOP = 0x15,
	MISSILE_TARGET_SCREEN_BOTTOM_MARGIN = 0x14,
};

enum
{
	WEAPON_ID_TURBO = 0,
	WEAPON_ID_BOMB_MISSILE = 2,
	WEAPON_ID_MINE = 3,
	WEAPON_ID_BEAKER = 4,
	WEAPON_ID_SHIELD = 6,
	WEAPON_ID_MASK = 7,
	WEAPON_ID_CLOCK = 8,
	WEAPON_ID_WARPBALL = 9,
	WEAPON_ID_INVISIBILITY = 0xc,
	WEAPON_ID_SUPER_ENGINE = 0xd,
	SHOOT_NOW_NO_FLAGS = 0,
	SHOOT_NOW_BACKWARD = 0x2,
	TURBO_ITEM_BOOST_NORMAL = 0x80,
	TURBO_ITEM_BOOST_JUICED = 0x100,
	TURBO_ITEM_RESERVES = 0x960,
	TURBO_ITEM_FIRE_FLAGS = FREEZE_RESERVES_ON_TURBO_PAD | TURBO_ITEM,
	WEAPON_GAMEPAD_RUMBLE_FRAMES = 8,
	WEAPON_GAMEPAD_RUMBLE_FORCE = 0x7f,
	ACTIVE_MISSILE_LIMIT = 12,
	MISSILE_RACE_FALLBACK_EVENT_MASK = 1,
	MISSILE_TRACKER_VELOCITY_NUMERATOR = 5,
	MISSILE_TRACKER_VELOCITY_SHIFT = 8,
	TRACKER_LAUNCH_VELOCITY_NUMERATOR = 3,
	TRACKER_LAUNCH_VELOCITY_SHIFT = 7,
	TRACKER_PARENT_SAFETY_FRAMES = 60,
	MINE_PARENT_SAFETY_FRAMES = 10,
	MINE_COLL_PROBE_TOP_Y_OFFSET = -400,
	MINE_COLL_PROBE_BOTTOM_Y_OFFSET = 64,
	MINE_COLL_SEARCH_PLAYER_THRESHOLD = 3,
	MINE_COLL_CALLBACK_FLAGS = 0x40,
	MINE_COLL_SCRATCH_OFFSET = 0x108,
	MINE_HITBOX_FRUIT_MODEL = PU_FRUIT_CRATE,
	MINE_HITBOX_RANDOM_MODEL = PU_RANDOM_CRATE,
	BEAKER_MODEL_HEADER_CAMERA_FLAG = 0x2,
	SHIELD_DARK_MODEL = DYNAMIC_SHIELD_DARK,
	SHIELD_SCALE = 0x700,
	SHIELD_HIGHLIGHT_ROT_Y = 0xc00,
	SHIELD_DURATION_NORMAL = 0x2d00,
	SHIELD_ALPHA_SCALE = 0x400,
	CLOCK_HURT_DURATION_NORMAL = 0x1e00,
	CLOCK_HURT_DURATION_JUICED = 0x2d00,
	CLOCK_FLASH_FRAMES = 4,
	CLOCK_SELF_SEND_FRAMES = 0x1e,
	CLOCK_DRIVER_COUNT = 8,
	CLOCK_HURT_REASON = 1,
	WARPBALL_MODEL = DYNAMIC_WARPBALL,
	WARPBALL_MATRIX_IDENTITY_SCALE = FP(1.0),
	WARPBALL_POS_SHIFT = 8,
	WARPBALL_PARENT_SAFETY_FRAMES = 10,
	WARPBALL_VELOCITY_NUMERATOR = 7,
	WARPBALL_VELOCITY_SHIFT = 8,
	WARPBALL_PARTICLE_ICON_GROUP = 0,
	WARPBALL_PARTICLE_OT_OFFSET = -6,
	INVISIBILITY_CLEAR_DRAW_FLAGS = ~(DRAW_TRANSPARENT | GHOST_DRAW_TRANSPARENT),
	INVISIBILITY_DURATION_NORMAL = 0x1e00,
	INVISIBILITY_DURATION_JUICED = 0x2d00,
	SUPER_ENGINE_DURATION_NORMAL = 0x1e00,
	SUPER_ENGINE_DURATION_JUICED = 0x2d00,
	SOUND_BOMB_LAUNCH = 0x47,
	SOUND_MISSILE_LAUNCH = 0x4a,
	SOUND_MINE_DROP = 0x52,
	SOUND_SHIELD = 0x57,
	SOUND_CLOCK = 0x44,
	SOUND_WARPBALL = 0x4d,
	SOUND_INVISIBILITY = 0x61,
	VOICELINE_BOMB_LAUNCH = 10,
	VOICELINE_MISSILE_LAUNCH = 11,
	VOICELINE_MINE_DROP = 0xf,
	VOICELINE_CLOCK = 0xe,
	VOICELINE_WARPBALL = 0xc,
	VOICELINE_WEAPON_PRIORITY = 0x10,
};

CTR_STATIC_ASSERT((s32)STATIC_UKAUKA - 1 == (s32)STATIC_AKUAKU);
CTR_STATIC_ASSERT((s32)STATIC_AKUAKU + MASK_SOUND_ID_OFFSET_FROM_MODEL == 0x53);
CTR_STATIC_ASSERT((s32)STATIC_UKAUKA + MASK_SOUND_ID_OFFSET_FROM_MODEL == 0x54);
CTR_STATIC_ASSERT(MISSILE_TARGET_DRIVER_COUNT == len(((struct GameTracker *)0)->drivers));
CTR_STATIC_ASSERT((s32)SHOOT_NOW_BACKWARD == (s32)POTION_THROW_BACKWARD);
CTR_STATIC_ASSERT((s32)MINE_HITBOX_FRUIT_MODEL == (s32)PU_FRUIT_CRATE);
CTR_STATIC_ASSERT((s32)MINE_HITBOX_RANDOM_MODEL == (s32)PU_RANDOM_CRATE);
CTR_STATIC_ASSERT((s32)SHIELD_DARK_MODEL == (s32)DYNAMIC_SHIELD_DARK);
CTR_STATIC_ASSERT(CLOCK_DRIVER_COUNT == len(((struct GameTracker *)0)->drivers));
CTR_STATIC_ASSERT((s32)WARPBALL_MODEL == (s32)DYNAMIC_WARPBALL);
CTR_STATIC_ASSERT((u32)INVISIBILITY_CLEAR_DRAW_FLAGS == 0xfff8ffffu);

b32 VehPickupItem_MaskBoolGoodGuy(struct Driver *d)
{
	s32 charID = data.characterIDs[d->driverID];

	// Crash, Coco, Pura, Polar, Penta
	u32 maskBits = MASK_GOOD_GUY_CHARACTER_BITS;

	return (maskBits >> charID) & 1;
}

// boolPlaySound only gates sound when refreshing an existing mask object.
struct MaskHeadWeapon *VehPickupItem_MaskUseWeapon(struct Driver *driver, b32 boolPlaySound)

{
	struct Thread *currThread;
	struct MaskHeadWeapon *maskObj;
	struct Model *modelPtr;
	struct Thread *t;
	struct GameTracker *gGT;
	struct Instance *instance;
	int soundID;

	gGT = sdata->gGT;

	if (!LOAD_IsOpen_RacingOrBattle() || ((gGT->gameMode1 & ADVENTURE_ARENA) != 0))
	{
		// no mask object in adv arena
		maskObj = NULL;
		return maskObj;
	}

	t = driver->instSelf->thread;

	// check for existing mask
	for (currThread = t->childThread; currThread != 0; currThread = currThread->siblingThread)
	{
		// if thread->modelIndex is NOT Aku or Uka
		if ((u32)(currThread->modelIndex - STATIC_AKUAKU) >= MASK_MODEL_COUNT)
		{
			continue;
		}

		currThread->funcThTick = RB_MaskWeapon_ThTick;

		maskObj = currThread->object;
		maskObj->duration = (driver->numWumpas < DRIVER_WUMPA_JUICED_COUNT) ? MASK_HEAD_DURATION_NORMAL : MASK_HEAD_DURATION_JUICED;

		if (
		    // If this is human and not AI
		    ((driver->actionsFlagSet & ACTION_BOT) == 0) &&

		    (boolPlaySound != 0))
		{
			soundID = currThread->modelIndex + MASK_SOUND_ID_OFFSET_FROM_MODEL;
			OtherFX_Play_Echo(soundID, 1, driver->actionsFlagSet & ACTION_ENGINE_ECHO);
		}

		// un-kill thread
		currThread->flags &= ~THREAD_FLAG_DEAD;

		// return object attached to thread
		return (struct MaskHeadWeapon *)currThread->object;
	}

	b32 boolGoodGuy = VehPickupItem_MaskBoolGoodGuy(driver);

	s32 modelID = STATIC_UKAUKA - boolGoodGuy;

	instance = INSTANCE_BirthWithThread(modelID, sdata->s_doctor1, SMALL, OTHER, RB_MaskWeapon_ThTick, sizeof(struct MaskHeadWeapon), t);

	soundID = modelID + MASK_SOUND_ID_OFFSET_FROM_MODEL;

	if (
	    // If this is human and not AI
	    ((driver->actionsFlagSet & ACTION_BOT) == 0) &&

	    (OtherFX_Play_Echo(soundID, 1, driver->actionsFlagSet & ACTION_ENGINE_ECHO),

	     (driver->kartState != KS_ENGINE_REVVING) && (driver->kartState != KS_MASK_GRABBED)))
	{
		if (boolGoodGuy == 0)
		{
			gGT->gameMode1 &= ~(AKU_SONG);
			gGT->gameMode1 |= UKA_SONG;
		}

		else
		{
			gGT->gameMode1 &= ~(UKA_SONG);
			gGT->gameMode1 |= AKU_SONG;
		}
	}

	modelPtr = gGT->modelPtr[STATIC_AKUBEAM + ((modelID - STATIC_AKUAKU) * MASK_BEAM_MODEL_STRIDE)];

	t = instance->thread;

	maskObj = (struct MaskHeadWeapon *)t->object;

// NOTE(aalhendi): Native keeps this model lookup string host-side; PS1 uses
// the retail RDATA symbol.
#ifdef CTR_NATIVE
	maskObj->maskBeamInst = INSTANCE_Birth3D(modelPtr, "akubeam1", t);
#else
	maskObj->maskBeamInst = INSTANCE_Birth3D(modelPtr, rdata.s_akubeam1, t);
#endif

	t->funcThDestroy = PROC_DestroyInstance;

	t->flags |= THREAD_FLAG_DISABLE_COLLISION;
	instance->flags |= HIDE_MODEL;
	maskObj->maskBeamInst->flags |= HIDE_MODEL;
	maskObj->duration = (driver->numWumpas < DRIVER_WUMPA_JUICED_COUNT) ? MASK_HEAD_DURATION_NORMAL : MASK_HEAD_DURATION_JUICED;
	maskObj->rot.x = MASK_INITIAL_ROT_X;
	maskObj->rot.y = 0;
	maskObj->rot.z = 0;
	maskObj->scale = MASK_HEAD_SCALE_NORMAL;

	return maskObj;
}

static struct PushBuffer *VehPickupItem_GetDriverPushBuffer(struct GameTracker *gGT, u8 driverID)
{
	return &gGT->pushBuffer[driverID];
}

static void VehPickupItem_MissileLoadPlayerView(struct GameTracker *gGT, struct Driver *driver)
{
	struct PushBuffer *pb = VehPickupItem_GetDriverPushBuffer(gGT, driver->driverID);

	SetRotMatrix(&pb->matrix_ViewProj);
	SetTransMatrix(&pb->matrix_ViewProj);
}

static void VehPickupItem_MissileLoadAiView(struct Driver *driver)
{
	SVec3 rot = {.x = driver->rotCurr.x, .y = driver->rotCurr.y, .z = driver->rotCurr.z};
	MATRIX matrix = {0};
	MATRIX unusedInverse;

	ConvertRotToMatrix(&matrix, &rot);
	matrix.t[0] = CTR_MipsSra(driver->posCurr.x, 8);
	matrix.t[1] = CTR_MipsSra(driver->posCurr.y, 8);
	matrix.t[2] = CTR_MipsSra(driver->posCurr.z, 8);

	MATH_HitboxMatrix(&unusedInverse, &matrix);

	SetRotMatrix(&matrix);
	SetTransMatrix(&matrix);
}

static b32 VehPickupItem_MissileCandidateVisible(struct PushBuffer *pb, struct Driver *candidate)
{
	struct Instance *inst = candidate->instSelf;
	u32 sxy;
	s32 gteFlag;
	s16 screenX;
	s16 screenY;

	MTC2(((u32)(u16)inst->matrix.t[0]) | ((u32)(u16)inst->matrix.t[1] << 16), 0);
	MTC2((s32)(s16)inst->matrix.t[2], 1);
	gte_rtps();

	sxy = MFC2(14);
	gteFlag = CFC2(31);
	if ((gteFlag & MISSILE_TARGET_GTE_RTPS_OVERFLOW) != 0)
	{
		return 0;
	}

	screenX = (s16)sxy;
	if (screenX < MISSILE_TARGET_SCREEN_LEFT)
	{
		return 0;
	}
	if (screenX >= pb->rect.w - MISSILE_TARGET_SCREEN_RIGHT_MARGIN)
	{
		return 0;
	}

	screenY = (s16)(sxy >> 16);
	if (screenY < MISSILE_TARGET_SCREEN_TOP)
	{
		return 0;
	}
	if (screenY >= pb->rect.h - MISSILE_TARGET_SCREEN_BOTTOM_MARGIN)
	{
		return 0;
	}

	return 1;
}

struct Driver *VehPickupItem_MissileGetTargetDriver(struct Driver *driver)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *target = NULL;
	s32 closestDistance = MISSILE_TARGET_DISTANCE_SENTINEL;
	struct PushBuffer *pb = VehPickupItem_GetDriverPushBuffer(gGT, driver->driverID);

	if (driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
	{
		VehPickupItem_MissileLoadPlayerView(gGT, driver);
	}
	else
	{
		VehPickupItem_MissileLoadAiView(driver);
	}

	for (s32 i = 0; i < MISSILE_TARGET_DRIVER_COUNT; i++)
	{
		struct Driver *candidate = gGT->drivers[i];

		if (candidate == NULL)
		{
			continue;
		}
		if (candidate == driver)
		{
			continue;
		}
		if (candidate->kartState == KS_MASK_GRABBED)
		{
			continue;
		}

		if (((gGT->gameMode1 & BATTLE_MODE) != 0) && (candidate->BattleHUD.teamID == driver->BattleHUD.teamID))
		{
			continue;
		}

		if (candidate->invisibleTimer != 0)
		{
			continue;
		}

		if (!VehPickupItem_MissileCandidateVisible(pb, candidate))
		{
			continue;
		}

		s32 dx = CTR_MipsSra(CTR_MipsSubLo(candidate->posCurr.x, driver->posCurr.x), MISSILE_TARGET_POS_SHIFT);
		s32 dz = CTR_MipsSra(CTR_MipsSubLo(candidate->posCurr.z, driver->posCurr.z), MISSILE_TARGET_POS_SHIFT);
		s32 distance = CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dz, dz));
		if (distance < closestDistance)
		{
			closestDistance = distance;
			target = candidate;
		}
	}

	return target;
}

b32 VehPickupItem_PotionThrow(struct MineWeapon *mine, struct Instance *inst, u32 flags)
{
	s32 throwVelocity;

	if ((flags & POTION_THROW_FORWARD) == 0)
	{
		if ((flags & POTION_THROW_BACKWARD) == 0)
		{
			if ((flags & POTION_THROW_RANDOM) == 0)
			{
				return 0;
			}

			throwVelocity = (MixRNG_Scramble() & POTION_THROW_RANDOM_MASK) - POTION_THROW_RANDOM_BIAS;
		}
		else
		{
			throwVelocity = -POTION_THROW_SPEED;
		}
	}
	else
	{
		throwVelocity = POTION_THROW_SPEED;
	}

	mine->velocity.x = (inst->matrix.m[0][2] * throwVelocity) >> POTION_THROW_MATRIX_SHIFT;
	mine->velocity.y = POTION_THROW_Y_VELOCITY;
	mine->velocity.z = (inst->matrix.m[2][2] * throwVelocity) >> POTION_THROW_MATRIX_SHIFT;
	mine->crateInst = NULL;
	mine->flags |= MINE_WEAPON_FLAG_THROWN;

	return 1;
}

void VehPickupItem_ShootNow(struct Driver *d, s32 weaponID, s32 flags)
{
	struct Instance *dInst;
	struct Thread *weaponTh;
	struct Instance *weaponInst;
	struct MineWeapon *mw;
	struct TrackerWeapon *tw;
	struct GameTracker *gGT = sdata->gGT;
	int modelID;
	int mineHitModel = 0;
	int mineShouldInitFollower = 0;

	switch (weaponID)
	{
	// Turbo
	case WEAPON_ID_TURBO:
	{
		int boost = TURBO_ITEM_BOOST_NORMAL;
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			boost = TURBO_ITEM_BOOST_JUICED;
		}

		VehFire_Increment(d, TURBO_ITEM_RESERVES, TURBO_ITEM_FIRE_FLAGS, boost);
	}
	break;

	// Shared code for Bomb and Missile
	case WEAPON_ID_BOMB_MISSILE:
		if (gGT->numMissiles >= ACTIVE_MISSILE_LIMIT)
		{
			return;
		}

		gGT->numMissiles++;
		d->numTimesMissileLaunched++;

		GAMEPAD_ShockFreq(d, WEAPON_GAMEPAD_RUMBLE_FRAMES, 0);
		GAMEPAD_ShockForce1(d, WEAPON_GAMEPAD_RUMBLE_FRAMES, WEAPON_GAMEPAD_RUMBLE_FORCE);

		struct Driver *victim = VehPickupItem_MissileGetTargetDriver(d);

		// if driver not found
		if (victim == 0)
		{
			// if not battle mode
			if ((gGT->gameMode1 & BATTLE_MODE) == 0)
			{
				if ((gGT->elapsedEventTime & MISSILE_RACE_FALLBACK_EVENT_MASK) != 0)
				{
					// if not DYNAMIC_PLAYER
					if (d->instSelf->thread->modelIndex != DYNAMIC_PLAYER)
					{
						int rank = d->driverRank;
						if (rank != 0)
						{
							victim = gGT->driversInRaceOrder[rank - 1];
						}
					}
				}
			}

			else
			{
				int closest = MISSILE_TARGET_DISTANCE_SENTINEL;

				for (int i = 0; i < MISSILE_TARGET_DRIVER_COUNT; i++)
				{
					struct Driver *tempD = gGT->drivers[i];

					if (tempD == 0)
					{
						continue;
					}
					if (tempD == d)
					{
						continue;
					}
					if (tempD->invisibleTimer != 0)
					{
						continue;
					}
					if (tempD->kartState == KS_MASK_GRABBED)
					{
						continue;
					}
					if (tempD->BattleHUD.teamID == d->BattleHUD.teamID)
					{
						continue;
					}

					int distX = CTR_MipsSra(CTR_MipsSubLo(tempD->posCurr.x, d->posCurr.x), MISSILE_TARGET_POS_SHIFT);
					int distZ = CTR_MipsSra(CTR_MipsSubLo(tempD->posCurr.z, d->posCurr.z), MISSILE_TARGET_POS_SHIFT);

					int dist = CTR_MipsAddLo(CTR_MipsMulLo(distX, distX), CTR_MipsMulLo(distZ, distZ));
					if (dist < closest)
					{
						closest = dist;
						victim = tempD;
					}
				}
			}
		}

		dInst = d->instSelf;

		// set up missile
		modelID = DYNAMIC_ROCKET;
		int bucket = TRACKING;
		struct Thread *parentTh = 0;
		char *weaponName = rdata.s_bombtracker1;

		// bomb
		if ((d->heldItemID == HELD_ITEM_BOMB_1X) || (d->heldItemID == HELD_ITEM_BOMB_3X))
		{
			modelID = DYNAMIC_BOMB;
			bucket = OTHER;
			parentTh = dInst->thread;
			weaponName = sdata->s_bomb1;
		}

		// medium stack pool
		weaponInst = INSTANCE_BirthWithThread(modelID, weaponName, MEDIUM, bucket, RB_MovingExplosive_ThTick, sizeof(struct TrackerWeapon), parentTh);

		// NOTE(aalhendi): Native low-RAM audit candidate only. Retail
		// dereferences weapon birth results before later checks in several
		// branches of this function; keep unpatched until memory pressure or
		// gameplay repro proves the semantic fallback.

		VehPickupItem_CopyMatrix(&weaponInst->matrix, &dInst->matrix);

		VehPhysForce_RotAxisAngle(&weaponInst->matrix, CTR_VECTOR_DATA(&(d->AxisAngle1_normalVec)), d->rotCurr.y);

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyTracker;
		weaponTh->funcThCollide = (void *)RB_Hazard_ThCollide_Missile;

		tw = weaponTh->object;
		tw->flags = 0;
		tw->framesSeekTargetTnt = 0;
		tw->soundIDCount = 0;
		tw->timeAlive = 0;
		tw->driverParent = d;
		tw->driverTarget = victim;
		tw->instParent = dInst;

		int talk;

		// bomb
		if (modelID == DYNAMIC_BOMB)
		{
			talk = VOICELINE_BOMB_LAUNCH;
			d->instBombThrow = weaponInst;

			SVECTOR rot;
			CTR_MatrixToRot(&rot, &weaponInst->matrix, 0x11);

			// not a typo, required like this
			tw->dir.x = rot.vy;
			tw->dir.y = rot.vx;
			tw->dir.z = rot.vz;

			PlaySound3D(SOUND_BOMB_LAUNCH, weaponInst);
		}

		// missile
		else
		{
			talk = VOICELINE_MISSILE_LAUNCH;

			if (victim != 0)
			{
				if (victim->thTrackingMe == 0)
				{
					victim->thTrackingMe = RB_GetThread_ClosestTracker(victim);
				}
			}

			PlaySound3D(SOUND_MISSILE_LAUNCH, weaponInst);
		}

		// if human and not AI
		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(talk, data.characterIDs[d->driverID], VOICELINE_WEAPON_PRIORITY);
		}

		tw->rotY = d->rotCurr.y;

		// do NOT patch for 60fps,
		// velocity uses elapsedTime
		tw->vel.y = 0;
		tw->vel.x = (weaponInst->matrix.m[0][2] * TRACKER_LAUNCH_VELOCITY_NUMERATOR) >> TRACKER_LAUNCH_VELOCITY_SHIFT;
		tw->vel.z = (weaponInst->matrix.m[2][2] * TRACKER_LAUNCH_VELOCITY_NUMERATOR) >> TRACKER_LAUNCH_VELOCITY_SHIFT;

		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			tw->flags |= TRACKER_FLAG_POWERED_UP;
		}

		// bomb
		if (modelID == DYNAMIC_BOMB)
		{
			struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

			if (
			    // hold d-pad DOWN
			    ((gb->buttonsHeldCurrFrame & BTN_DOWN) != 0) ||

			    // pinstripe
			    ((flags & SHOOT_NOW_BACKWARD) != 0))
			{
				tw->flags |= TRACKER_FLAG_BOMB_BACKWARD;

				tw->vel.x = -(((tw->vel.x >> 1) * 3) / 5);
				tw->vel.z = -(((tw->vel.z >> 1) * 3) / 5);
			}
		}

		// missile
		else
		{
			if (d->numWumpas < DRIVER_WUMPA_JUICED_COUNT)
			{
				// do NOT patch for 60fps,
				// velocity uses elapsedTime
				tw->vel.x = (weaponInst->matrix.m[0][2] * MISSILE_TRACKER_VELOCITY_NUMERATOR) >> MISSILE_TRACKER_VELOCITY_SHIFT;
				tw->vel.z = (weaponInst->matrix.m[2][2] * MISSILE_TRACKER_VELOCITY_NUMERATOR) >> MISSILE_TRACKER_VELOCITY_SHIFT;
			}
		}

		tw->parentSafetyFrames = TRACKER_PARENT_SAFETY_FRAMES;
		tw->blindFrames = 0;
		break;

	// TNT/Nitro
	case WEAPON_ID_MINE:

		// tnt or nitro
		modelID = STATIC_CRATE_TNT;
		char *mineName = sdata->s_tnt1;
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			modelID = PU_EXPLOSIVE_CRATE;
			mineName = sdata->s_nitro1;
		}

		weaponInst = INSTANCE_BirthWithThread(modelID, mineName, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);

		dInst = d->instSelf;

		VehPickupItem_CopyMatrix(&weaponInst->matrix, &dInst->matrix);

		weaponInst->scale.x = 0;
		weaponInst->scale.y = 0;
		weaponInst->scale.z = 0;

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyInstance;
		weaponTh->funcThCollide = (void *)RB_Hazard_ThCollide_Generic;

		PlaySound3D(SOUND_MINE_DROP, weaponInst);

		// if human and not AI
		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(VOICELINE_MINE_DROP, data.characterIDs[d->driverID], VOICELINE_WEAPON_PRIORITY);
		}

		mw = weaponTh->object;
		mw->driverTarget = 0;
		mw->instParent = dInst;
		mw->crateInst = 0;
		VehPickupItem_ClearMineMotion(mw);
		mw->boolDestroyed = 0;
		mw->parentSafetyFrames = MINE_PARENT_SAFETY_FRAMES;
		mw->tntSpinY = 0;
		mw->flags = 0;

		RB_MinePool_Add(mw);
		VehPickupItem_PotionThrow(mw, weaponInst, flags);
		mineHitModel = weaponInst->model->id | COLL_MODELID_BLOCKAGE_FLAG;
		mineShouldInitFollower = (flags == 0);

	RunMineCOLL:;

		SVec3 probeTop;
		SVec3 probeBottom;

		probeTop.x = (s16)(u16)weaponInst->matrix.t[0];
		probeTop.y = (s16)CTR_MipsAddLo((u16)weaponInst->matrix.t[1], MINE_COLL_PROBE_TOP_Y_OFFSET);
		probeTop.z = (s16)(u16)weaponInst->matrix.t[2];

		probeBottom.x = (s16)(u16)weaponInst->matrix.t[0];
		probeBottom.y = (s16)CTR_MipsAddLo((u16)weaponInst->matrix.t[1], MINE_COLL_PROBE_BOTTOM_Y_OFFSET);
		probeBottom.z = (s16)(u16)weaponInst->matrix.t[2];

		struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, MINE_COLL_SCRATCH_OFFSET);

		sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
		sps->Union.QuadBlockColl.quadFlagsIgnored = 0;

		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES;
		if (gGT->numPlyrCurrGame < MINE_COLL_SEARCH_PLAYER_THRESHOLD)
		{
			sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD;
		}

		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;

		COLL_SearchBSP_CallbackQUADBLK(&probeTop, &probeBottom, sps, MINE_COLL_CALLBACK_FLAGS);

		if (sps->boolDidTouchHitbox != 0)
		{
			sps->Input1.modelID = mineHitModel;

			RB_Hazard_CollLevInst(sps, weaponTh);

			struct InstDef *instDef = sps->bspHitbox->data.hitbox.instDef;

			s16 modelTouched = instDef->modelID;
			if ((modelTouched == MINE_HITBOX_FRUIT_MODEL) || (modelTouched == MINE_HITBOX_RANDOM_MODEL))
			{
				mw->crateInst = instDef->ptrInstance;
			}

			else
			{
				RB_GenericMine_ThDestroy(weaponTh, weaponInst, mw);
			}

			sps->Union.QuadBlockColl.searchFlags = 0;
			COLL_SearchBSP_CallbackQUADBLK(&probeTop, &probeBottom, sps, 0);
		}

		RB_MakeInstanceReflective(sps, weaponInst);

		SVec3 fallbackNormal;
		s16 *rotationNormal;

		if (sps->boolDidTouchQuadblock == 0)
		{
			fallbackNormal = (SVec3){.x = 0, .y = COLL_FRACTION_ONE, .z = 0};
			rotationNormal = CTR_VECTOR_DATA(&(fallbackNormal));

			mw->stopFallAtY = weaponInst->matrix.t[1];
		}

		else
		{
			mw->stopFallAtY = sps->Union.QuadBlockColl.hitPos.y;
			rotationNormal = CTR_VECTOR_DATA(&(sps->hit.plane.normal));
		}

		VehPhysForce_RotAxisAngle(&weaponInst->matrix, rotationNormal, d->angle);

		if (weaponID == WEAPON_ID_MINE)
		{
			d->instTntSend = weaponInst;
		}

		// dropped a mine
		d->actionsFlagSet |= ACTION_DROPPING_MINE;

		if (mineShouldInitFollower != 0)
		{
			RB_Follower_Init(d, weaponTh);
		}
		break;

	// Beaker
	case WEAPON_ID_BEAKER:

		if (d->numWumpas < DRIVER_WUMPA_JUICED_COUNT)
		{
			modelID = STATIC_BEAKER_GREEN;

			weaponInst = INSTANCE_BirthWithThread(modelID, sdata->s_beaker1, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);
			if (weaponInst == 0)
			{
				return;
			}
		}
		else
		{
			modelID = STATIC_BEAKER_RED;

			weaponInst = INSTANCE_BirthWithThread(modelID, sdata->s_beaker1, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);
		}

		dInst = d->instSelf;

		VehPickupItem_CopyMatrix(&weaponInst->matrix, &dInst->matrix);

		// potion always faces camera
		weaponInst->model->headers[0].flags |= BEAKER_MODEL_HEADER_CAMERA_FLAG;

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyInstance;
		weaponTh->funcThCollide = (void *)RB_Hazard_ThCollide_Generic;

		PlaySound3D(SOUND_MINE_DROP, weaponInst);

		// if human and not AI
		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(VOICELINE_MINE_DROP, data.characterIDs[d->driverID], VOICELINE_WEAPON_PRIORITY);
		}

		mw = weaponTh->object;
		mw->driverTarget = 0;
		mw->instParent = dInst;
		mw->crateInst = 0;
		mw->boolDestroyed = 0;
		mw->parentSafetyFrames = MINE_PARENT_SAFETY_FRAMES;
		mw->flags = 0;
		if (modelID == STATIC_BEAKER_RED)
		{
			mw->flags = MINE_WEAPON_FLAG_RED_BEAKER;
		}

		struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

		// throw potion forward
		if ((gb->buttonsHeldCurrFrame & BTN_UP) != 0)
		{
			flags |= POTION_THROW_FORWARD;
		}

		RB_MinePool_Add(mw);
		b32 didThrowPotion = VehPickupItem_PotionThrow(mw, weaponInst, flags);

		if (didThrowPotion == 0)
		{
			weaponInst->scale.x = 0;
			weaponInst->scale.y = 0;
			weaponInst->scale.z = 0;

			VehPickupItem_ClearMineMotion(mw);

			mineHitModel = weaponInst->model->id;
			mineShouldInitFollower = 1;
			goto RunMineCOLL;
		}
		break;

	// Shield Bubble
	case WEAPON_ID_SHIELD:;

		char *shieldDarkName = rdata.s_shielddark;
		char *highlightName = rdata.s_highlight;

		weaponInst =
		    INSTANCE_BirthWithThread(SHIELD_DARK_MODEL, shieldDarkName, MEDIUM, OTHER, RB_ShieldDark_ThTick_Grow, sizeof(struct Shield), d->instSelf->thread);

		weaponTh = weaponInst->thread;
		weaponInst->scale.x = SHIELD_SCALE;
		weaponInst->scale.y = SHIELD_SCALE;
		weaponInst->scale.z = SHIELD_SCALE;
		weaponTh->funcThDestroy = PROC_DestroyInstance;
		OtherFX_Play(SOUND_SHIELD, 1);

		modelID = DYNAMIC_SHIELD_GREEN;
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			modelID = DYNAMIC_SHIELD;
		}

		struct Instance *instColor = INSTANCE_Birth3D(gGT->modelPtr[modelID], sdata->s_shield, weaponTh);

		struct Instance *instHighlight = INSTANCE_Birth3D(gGT->modelPtr[DYNAMIC_HIGHLIGHT], highlightName, weaponTh);

		instColor->scale.x = SHIELD_SCALE;
		instColor->scale.y = SHIELD_SCALE;
		instColor->scale.z = SHIELD_SCALE;

		instHighlight->scale.x = SHIELD_SCALE;
		instHighlight->scale.y = SHIELD_SCALE;
		instHighlight->scale.z = SHIELD_SCALE;

		struct Shield *shieldObj = weaponTh->object;
		shieldObj->animFrame = 0;
		shieldObj->flags = 0;
		shieldObj->instColor = instColor;
		shieldObj->instHighlight = instHighlight;
		shieldObj->highlightRot.x = 0;
		shieldObj->highlightRot.y = SHIELD_HIGHLIGHT_ROT_Y;
		shieldObj->highlightRot.z = 0;
		shieldObj->highlightTimer = 0;

		if (d->numWumpas < DRIVER_WUMPA_JUICED_COUNT)
		{
			shieldObj->duration = SHIELD_DURATION_NORMAL;
		}
		else
		{
			shieldObj->flags = SHIELD_FLAG_BLUE;
		}

		weaponInst->alphaScale = SHIELD_ALPHA_SCALE;
		d->instBubbleHold = weaponInst;
		break;

	// Mask
	case WEAPON_ID_MASK:
		VehPickupItem_MaskUseWeapon(d, true);
		break;

	// Clock
	case WEAPON_ID_CLOCK:

		d->numTimesClockWeaponUsed++;

		OtherFX_Play(SOUND_CLOCK, 1);

		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(VOICELINE_CLOCK, data.characterIDs[d->driverID], VOICELINE_WEAPON_PRIORITY);
		}

		int hurtVal = CLOCK_HURT_DURATION_NORMAL;
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			hurtVal = CLOCK_HURT_DURATION_JUICED;
		}

		struct Driver **dptr;

		for (dptr = &gGT->drivers[0]; dptr < &gGT->drivers[CLOCK_DRIVER_COUNT]; dptr++)
		{
			struct Driver *victim = *dptr;

			if (victim == 0)
			{
				continue;
			}

			victim->clockFlash = CLOCK_FLASH_FRAMES;

			if (victim == d)
			{
				d->clockSend = CLOCK_SELF_SEND_FRAMES;
				continue;
			}

			// if spin out driver
			if (RB_Hazard_HurtDriver(victim, CLOCK_HURT_REASON, 0, 0) != 0)
			{
				victim->clockReceive = hurtVal;
			}
		}
		break;

	// Warpball
	case WEAPON_ID_WARPBALL:

		dInst = d->instSelf;
		GAMEPAD_ShockFreq(d, WEAPON_GAMEPAD_RUMBLE_FRAMES, 0);
		GAMEPAD_ShockForce1(d, WEAPON_GAMEPAD_RUMBLE_FRAMES, WEAPON_GAMEPAD_RUMBLE_FORCE);

		// MEDIUM
		char *warpballName = rdata.s_warpball;

		weaponInst = INSTANCE_BirthWithThread(WARPBALL_MODEL, warpballName, MEDIUM, TRACKING, RB_Warpball_ThTick, sizeof(struct TrackerWeapon), 0);

		weaponInst->matrix.m[0][0] = WARPBALL_MATRIX_IDENTITY_SCALE;
		weaponInst->matrix.m[0][1] = 0;
		weaponInst->matrix.m[0][2] = 0;
		weaponInst->matrix.m[1][0] = 0;
		weaponInst->matrix.m[1][1] = WARPBALL_MATRIX_IDENTITY_SCALE;
		weaponInst->matrix.m[1][2] = 0;
		weaponInst->matrix.m[2][0] = 0;
		weaponInst->matrix.m[2][1] = 0;
		weaponInst->matrix.m[2][2] = WARPBALL_MATRIX_IDENTITY_SCALE;

		weaponInst->matrix.t[0] = CTR_MipsSra(d->posCurr.x, WARPBALL_POS_SHIFT);
		weaponInst->matrix.t[1] = CTR_MipsSra(d->posCurr.y, WARPBALL_POS_SHIFT);
		weaponInst->matrix.t[2] = CTR_MipsSra(d->posCurr.z, WARPBALL_POS_SHIFT);

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyInstance;

		PlaySound3D(SOUND_WARPBALL, weaponInst);

		Voiceline_RequestPlay(VOICELINE_WARPBALL, data.characterIDs[d->driverID], VOICELINE_WEAPON_PRIORITY);

		// used by RB_Warpball_SeekDriver
		victim = 0;
		int rank = d->driverRank;
		if (rank == 0)
		{
			victim = gGT->driversInRaceOrder[rank + 1];
		}
		else
		{
			victim = gGT->driversInRaceOrder[rank - 1];
		}

		tw = weaponTh->object;
		tw->flags = TRACKER_FLAG_WARPBALL_FALLBACK_PATH;
		tw->soundIDCount = 0;
		tw->ptrNodeNext = 0;
		tw->pathProgress = 0;
		tw->turnAroundFrames = 0;
		tw->driverParent = d;
		tw->driverTarget = victim;
		tw->instParent = dInst;

		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			tw->flags |= TRACKER_FLAG_POWERED_UP;
		}

		// sets nodeCurrIndex
		RB_Warpball_SeekDriver(tw, d->checkpoint.currentIndex, d);

		struct CheckpointNode *cn = gGT->level1->ptr_restart_points;
		tw->nodeNextIndex = tw->nodeCurrIndex;
		tw->ptrNodeCurr = &cn[tw->nodeCurrIndex];

		if ((tw->flags & TRACKER_FLAG_WARPBALL_TARGET_PATH) == 0)
		{
			RB_Warpball_Start(tw);
		}
		else
		{
			tw->flags &= ~TRACKER_FLAG_WARPBALL_FALLBACK_PATH;
		}

		tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, victim);

		tw->vel.y = 0;
		tw->rotY = d->angle;
		tw->parentSafetyFrames = WARPBALL_PARENT_SAFETY_FRAMES;

		// do NOT patch for 60fps,
		// velocity uses elapsedTime
		tw->vel.x = (dInst->matrix.m[0][2] * WARPBALL_VELOCITY_NUMERATOR) >> WARPBALL_VELOCITY_SHIFT;
		tw->vel.z = (dInst->matrix.m[2][2] * WARPBALL_VELOCITY_NUMERATOR) >> WARPBALL_VELOCITY_SHIFT;

		struct Particle *p = Particle_Init(0, gGT->iconGroup[WARPBALL_PARTICLE_ICON_GROUP], &data.emSet_Warpball[0]);

		tw->ptrParticle = p;

		if (p != 0)
		{
			p->otIndexOffset = (s8)WARPBALL_PARTICLE_OT_OFFSET;
		}

		break;

	// invisibility
	case WEAPON_ID_INVISIBILITY:

		if (d->invisibleTimer == 0)
		{
			d->instFlagsBackup = d->instSelf->flags;

			d->instSelf->flags = (d->instSelf->flags & INVISIBILITY_CLEAR_DRAW_FLAGS) | GHOST_DRAW_TRANSPARENT;

			OtherFX_Play(SOUND_INVISIBILITY, 1);
		}

		int time = INVISIBILITY_DURATION_NORMAL;
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			time = INVISIBILITY_DURATION_JUICED;
		}

		d->invisibleTimer = time;
		break;


	// Super Engine
	case WEAPON_ID_SUPER_ENGINE:
	{
		int engine = SUPER_ENGINE_DURATION_NORMAL;
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			engine = SUPER_ENGINE_DURATION_JUICED;
		}

		d->superEngineTimer = engine;
	}
	break;
	}
}

void VehPickupItem_ShootOnCirclePress(struct Driver *d)
{
	u8 weapon;

	if (d->pendingDamageType != 0)
	{
		VehPickState_NewState(d, d->pendingDamageType, d->pendingDamageAttacker, d->pendingDamageReasonByte);
	}

	// If you want to fire a weapon
	if ((d->actionsFlagSet & ACTION_WEAPON_FIRE_REQUEST) == 0)
	{
		return;
	}

	// Remove the request to fire a weapon, since we will fire it now
	d->actionsFlagSet &= ~ACTION_WEAPON_FIRE_REQUEST;

	weapon = d->heldItemID;

	// Missiles and Bombs share code,
	// Change Bomb1x, Bomb3x, Missile3x, to Missile1x
	if ((weapon == HELD_ITEM_BOMB_1X) || (weapon == HELD_ITEM_BOMB_3X) || (weapon == HELD_ITEM_MISSILE_3X))
	{
		weapon = HELD_ITEM_BOMB_MISSILE_SHARED;
	}

	VehPickupItem_ShootNow(d, (s32)weapon, SHOOT_NOW_NO_FLAGS);
}
