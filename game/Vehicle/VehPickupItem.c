#include "VehCommon.h"

typedef s16 VehPickupItemSignedHalfword CTR_MAY_ALIAS;

// NOTE(aalhendi): Retail copies matrices as eight words. Native keeps the
// typed assignment so host compilers retain their normal aliasing guarantees.
#ifdef CTR_NATIVE
static inline void VehPickupItem_CopyInstanceMatrix(struct Instance *dst, const struct Instance *src)
{
	dst->matrix = src->matrix;
}
#define VehPickupItem_CopyInstanceMatrixForPhysics(dst, src, matrix) ((void)(matrix), VehPickupItem_CopyInstanceMatrix((dst), (src)))
#else
#define VEH_PICKUP_COPY_MATRIX_INSTRUCTIONS \
	"lw $9,48(%1)\n\t"                      \
	"lw $10,52(%1)\n\t"                     \
	"lw $11,56(%1)\n\t"                     \
	"lw $12,60(%1)\n\t"                     \
	"sw $9,48(%0)\n\t"                      \
	"sw $10,52(%0)\n\t"                     \
	"sw $11,56(%0)\n\t"                     \
	"sw $12,60(%0)\n\t"                     \
	"lw $9,64(%1)\n\t"                      \
	"lw $10,68(%1)\n\t"                     \
	"lw $11,72(%1)\n\t"                     \
	"lw $12,76(%1)\n\t"                     \
	"sw $9,64(%0)\n\t"                      \
	"sw $10,68(%0)\n\t"                     \
	"sw $11,72(%0)\n\t"                     \
	"sw $12,76(%0)"
#define VehPickupItem_CopyInstanceMatrix(dst, src) \
	__asm__ volatile(VEH_PICKUP_COPY_MATRIX_INSTRUCTIONS : : "r"(dst), "r"(src) : "$9", "$10", "$11", "$12", "memory")
#define VehPickupItem_CopyInstanceMatrixForPhysics(dst, src, matrix) \
	__asm__ volatile(VEH_PICKUP_COPY_MATRIX_INSTRUCTIONS : : "r"(dst), "r"(src), "r"(matrix) : "$9", "$10", "$11", "$12", "memory")
#endif

// NOTE(aalhendi): Retail addresses the scratchpad normal with an OR; native
// uses the typed member that occupies the same offset.
#ifdef CTR_NATIVE
#define VehPickupItem_SetWarpballMatrixPair(matrix, index, first, second) \
	do                                                                    \
	{                                                                     \
		((s16 *)(matrix)->m)[(index)] = (first);                          \
		((s16 *)(matrix)->m)[(index) + 1] = (second);                     \
	} while (0)
#else
typedef u32 VehPickupItemMatrixWord CTR_MAY_ALIAS;
#define VehPickupItem_SetWarpballMatrixPair(matrix, index, first, second) \
	(*(VehPickupItemMatrixWord *)&((s16 *)(matrix)->m)[(index)] = ((u32)(u16)(first) | ((u32)(u16)(second) << 16)))
#endif

#ifdef CTR_NATIVE
#define VehPickupItem_GetCollisionNormal(sps) CTR_VECTOR_DATA(&(sps)->hit.plane.normal)
#else
#define VehPickupItem_GetCollisionNormal(sps) ((s16 *)((u32)(sps) | 0x70))
#endif

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
	s32 charID = GAME_CHARACTER_IDS[d->driverID];
	b32 isGoodGuy = false;

	// Crash, Coco, Pura, Polar, Penta
	if ((charID == CRASH_BANDICOOT) || (charID == COCO_BANDICOOT) || (charID == POLAR) || (charID == PURA) || (charID == PENTA_PENGUIN))
	{
		isGoodGuy = true;
	}

	return isGoodGuy;
}

// boolPlaySound only gates sound when refreshing an existing mask object.
struct MaskHeadWeapon *VehPickupItem_MaskUseWeapon(struct Driver *driver, b32 boolPlaySound)
{
	struct Thread *currThread;
	struct MaskHeadWeapon *maskObj;
	struct Model *modelPtr;
	struct Thread *parentThread;
	struct Thread *maskThread;
	struct Instance *instance;
	int soundID;
	b32 boolGoodGuy;
	u32 actionsFlagSet;
	register char *beamName CTR_PSX_REGISTER("$5");
	register struct Thread *beamThread CTR_PSX_REGISTER("$6");

	if (!LOAD_IsOpen_RacingOrBattle() || ((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) != 0))
	{
		return NULL;
	}

	parentThread = driver->instSelf->thread;

	// check for existing mask
	for (currThread = parentThread->childThread; currThread != 0; currThread = currThread->siblingThread)
	{
		// if thread->modelIndex is NOT Aku or Uka
		if ((u32)((u16)currThread->modelIndex - STATIC_AKUAKU) >= MASK_MODEL_COUNT)
		{
			continue;
		}

		currThread->funcThTick = RB_MaskWeapon_ThTick;
		{
			register s32 existingDuration CTR_PSX_REGISTER("$2");
			register struct MaskHeadWeapon *existingMask CTR_PSX_REGISTER("$3");

			existingDuration = driver->numWumpas;
			existingDuration = existingDuration < DRIVER_WUMPA_JUICED_COUNT;
			if (existingDuration == 0)
			{
				existingMask = currThread->object;
				existingDuration = MASK_HEAD_DURATION_JUICED;
			}
			else
			{
				existingDuration = MASK_HEAD_DURATION_NORMAL;
				existingMask = currThread->object;
			}
			CTR_PSX_MEMORY_BARRIER();
			existingMask->duration = (s16)existingDuration;
		}

		actionsFlagSet = driver->actionsFlagSet;

		if (
		    // If this is human and not AI
		    ((actionsFlagSet & ACTION_BOT) == 0) &&

		    (boolPlaySound != 0))
		{
			if (currThread->modelIndex == STATIC_UKAUKA)
			{
				soundID = STATIC_UKAUKA + MASK_SOUND_ID_OFFSET_FROM_MODEL;
			}
			else if (currThread->modelIndex == STATIC_AKUAKU)
			{
				soundID = STATIC_AKUAKU + MASK_SOUND_ID_OFFSET_FROM_MODEL;
			}
			else
			{
				goto MaskAlreadyActive;
			}

			OtherFX_Play_Echo(soundID, 1, (actionsFlagSet >> 16) & 1);
		}

	MaskAlreadyActive:
		// un-kill thread
		currThread->flags &= ~THREAD_FLAG_DEAD;

		// return object attached to thread
		return (struct MaskHeadWeapon *)currThread->object;
	}

	boolGoodGuy = VehPickupItem_MaskBoolGoodGuy(driver);

	if ((boolGoodGuy << 16) != 0)
	{
		instance =
		    INSTANCE_BirthWithThread(STATIC_AKUAKU, VEH_PICKUP_DOCTOR_NAME, SMALL, OTHER, RB_MaskWeapon_ThTick, sizeof(struct MaskHeadWeapon), parentThread);

		actionsFlagSet = driver->actionsFlagSet;
		if (((actionsFlagSet & ACTION_BOT) == 0) && (OtherFX_Play_Echo(STATIC_AKUAKU + MASK_SOUND_ID_OFFSET_FROM_MODEL, 1, (actionsFlagSet >> 16) & 1),
		                                             (u32)(driver->kartState - KS_ENGINE_REVVING) > 1))
		{
			GAME_TRACKER->gameMode1 = (GAME_TRACKER->gameMode1 | AKU_SONG) & ~UKA_SONG;
		}

#ifdef CTR_NATIVE
		beamName = "akubeam1";
#else
		beamName = rdata.s_akubeam1;
#endif
		maskThread = instance->thread;
		beamThread = maskThread;
		modelPtr = GAME_TRACKER->modelPtr[STATIC_AKUBEAM];
	}
	else
	{
		instance =
		    INSTANCE_BirthWithThread(STATIC_UKAUKA, VEH_PICKUP_DOCTOR_NAME, SMALL, OTHER, RB_MaskWeapon_ThTick, sizeof(struct MaskHeadWeapon), parentThread);

		actionsFlagSet = driver->actionsFlagSet;
		if (((actionsFlagSet & ACTION_BOT) == 0) && (OtherFX_Play_Echo(STATIC_UKAUKA + MASK_SOUND_ID_OFFSET_FROM_MODEL, 1, (actionsFlagSet >> 16) & 1),
		                                             (u32)(driver->kartState - KS_ENGINE_REVVING) > 1))
		{
			GAME_TRACKER->gameMode1 = (GAME_TRACKER->gameMode1 | UKA_SONG) & ~AKU_SONG;
		}

#ifdef CTR_NATIVE
		beamName = "akubeam1";
#else
		beamName = rdata.s_akubeam1;
#endif
		maskThread = instance->thread;
		beamThread = maskThread;
		modelPtr = GAME_TRACKER->modelPtr[STATIC_UKABEAM];
	}

	maskObj = (struct MaskHeadWeapon *)maskThread->object;

	maskObj->maskBeamInst = INSTANCE_Birth3D(modelPtr, beamName, beamThread);

	maskThread->funcThDestroy = PROC_DestroyInstance;

	maskThread->flags |= THREAD_FLAG_DISABLE_COLLISION;
	instance->flags |= HIDE_MODEL;
	maskObj->maskBeamInst->flags |= HIDE_MODEL;
	{
		register s32 finalDuration CTR_PSX_REGISTER("$2");

		finalDuration = driver->numWumpas;
		finalDuration = finalDuration < DRIVER_WUMPA_JUICED_COUNT;
		if (finalDuration != 0)
		{
			finalDuration = MASK_HEAD_DURATION_NORMAL;
		}
		else
		{
			finalDuration = MASK_HEAD_DURATION_JUICED;
		}
		maskObj->duration = (s16)finalDuration;
	}
	maskObj->rot.x = MASK_INITIAL_ROT_X;
	maskObj->rot.y = 0;
	maskObj->scale = MASK_HEAD_SCALE_NORMAL;
	maskObj->rot.z = 0;

	return maskObj;
}

struct Driver *VehPickupItem_MissileGetTargetDriver(struct Driver *driver)
{
	SVec3 rotation;
	MATRIX matrix;
	MATRIX inverseMatrix;
	SVec3 candidatePosition;
	struct MissileProjection
	{
		s32 screenPosition;
		volatile s32 gteFlag;
	} projection;
	s32 *screenPositionPtr;
	struct Driver *target;
	s32 closestDistance;
	s32 i;

	target = NULL;
	closestDistance = MISSILE_TARGET_DISTANCE_SENTINEL;

	if (driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
	{
		struct PushBuffer *pushBuffer = &GAME_TRACKER->pushBuffer[driver->driverID];

		CTR_GteSetRotMatrix(&pushBuffer->matrix_ViewProj);
		CTR_GteSetTransMatrix(&pushBuffer->matrix_ViewProj);
	}
	else
	{
		rotation.x = (u16)driver->rotCurr.x;
		rotation.y = (u16)driver->rotCurr.y;
		rotation.z = (u16)driver->rotCurr.z;

		ConvertRotToMatrix(&matrix, &rotation);
		matrix.t[0] = CTR_MipsSra(driver->posCurr.x, MISSILE_TARGET_POS_SHIFT);
		matrix.t[1] = CTR_MipsSra(driver->posCurr.y, MISSILE_TARGET_POS_SHIFT);
		matrix.t[2] = CTR_MipsSra(driver->posCurr.z, MISSILE_TARGET_POS_SHIFT);

		MATH_HitboxMatrix(&inverseMatrix, &matrix);

		CTR_GteSetRotMatrix(&matrix);
		CTR_GteSetTransMatrix(&matrix);
	}

	i = 0;
	screenPositionPtr = &projection.screenPosition;
	do
	{
		struct Driver *candidate = GAME_TRACKER->drivers[i];
		s32 dx;
		s32 dz;
		s32 distance;
		s16 screenX;
		s16 screenY;

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

		if (((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0) && (candidate->BattleHUD.teamID == driver->BattleHUD.teamID))
		{
			continue;
		}

		if (candidate->invisibleTimer != 0)
		{
			continue;
		}

		candidatePosition.x = (u16)candidate->instSelf->matrix.t[0];
		candidatePosition.y = (u16)candidate->instSelf->matrix.t[1];
		candidatePosition.z = (u16)candidate->instSelf->matrix.t[2];

#ifdef CTR_NATIVE
		MTC2(CTR_PackS16Pair(candidatePosition.x, candidatePosition.y), 0);
		MTC2((s32)candidatePosition.z, 1);
#else
		__asm__ volatile("lwc2 $0,0(%0)\n\t"
		                 "lwc2 $1,4(%0)"
		                 :
		                 : "r"(&candidatePosition)
		                 : "memory");
#endif
		CTR_PSX_GTE_PIPELINE_DELAY();
		gte_rtps();
		CTR_PSX_STORE_COP2_WORD(screenPositionPtr, 14);

#ifdef CTR_NATIVE
		{
			register u32 flagValue CTR_PSX_REGISTER("$12");

			flagValue = CFC2(31);
			CTR_PSX_GTE_READ_DELAY();
			projection.gteFlag = flagValue;
		}
#else
		{
			register volatile s32 *flagPtr CTR_PSX_REGISTER("$2");

			flagPtr = &projection.gteFlag;
			__asm__ volatile("cfc2 $12,$31\n\t"
			                 "nop\n\t"
			                 "sw $12,0(%0)"
			                 :
			                 : "r"(flagPtr)
			                 : "$12", "memory");
		}
#endif

		if ((projection.gteFlag & MISSILE_TARGET_GTE_RTPS_OVERFLOW) != 0)
		{
			continue;
		}

		screenX = (s16)projection.screenPosition;
		if (screenX < MISSILE_TARGET_SCREEN_LEFT)
		{
			continue;
		}
		if (screenX >= GAME_TRACKER->pushBuffer[driver->driverID].rect.w - MISSILE_TARGET_SCREEN_RIGHT_MARGIN)
		{
			continue;
		}

		screenY = (s16)CTR_ReadU16LE((const u8 *)screenPositionPtr + sizeof(u16));
		if (screenY < MISSILE_TARGET_SCREEN_TOP)
		{
			continue;
		}
		if (screenY >= GAME_TRACKER->pushBuffer[driver->driverID].rect.h - MISSILE_TARGET_SCREEN_BOTTOM_MARGIN)
		{
			continue;
		}

		dx = CTR_MipsSra(CTR_MipsSubLo(candidate->posCurr.x, driver->posCurr.x), MISSILE_TARGET_POS_SHIFT);
		dz = CTR_MipsSra(CTR_MipsSubLo(candidate->posCurr.z, driver->posCurr.z), MISSILE_TARGET_POS_SHIFT);
		distance = CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dz, dz));
		if (distance < closestDistance)
		{
			closestDistance = distance;
			target = candidate;
		}
	} while (++i < MISSILE_TARGET_DRIVER_COUNT);

	return target;
}

b32 VehPickupItem_PotionThrow(struct MineWeapon *mine, struct Instance *inst, u32 flags)
{
	s32 throwVelocity;
	s32 matrixX;
	s32 matrixZ;
	s32 randomVelocityZ;
	register s32 tailValue CTR_PSX_REGISTER("$3");
	register b32 result CTR_PSX_REGISTER("$2");

	if ((flags & POTION_THROW_FORWARD) != 0)
	{
		matrixX = inst->matrix.m[0][2];
		CTR_PSX_KEEP_VALUE(matrixX);
		result = 1;
		CTR_PSX_KEEP_VALUE(result);
		mine->velocity.x = (s16)((matrixX * 15) >> 9);
		CTR_PSX_OBSERVE_MEMORY(mine->velocity.x);
		matrixZ = inst->matrix.m[2][2];
		mine->velocity.y = POTION_THROW_Y_VELOCITY;
		mine->crateInst = NULL;
		tailValue = (u16)mine->flags;
		tailValue |= MINE_WEAPON_FLAG_THROWN;
		mine->flags = (u16)tailValue;
		CTR_PSX_OBSERVE_MEMORY(mine->flags);
		tailValue = (matrixZ * 15) >> 9;
	}
	else if ((flags & POTION_THROW_BACKWARD) != 0)
	{
		result = 1;
		CTR_PSX_KEEP_VALUE(result);
		matrixX = inst->matrix.m[0][2];
		mine->velocity.x = (s16)((matrixX * -POTION_THROW_SPEED) >> POTION_THROW_MATRIX_SHIFT);
		CTR_PSX_OBSERVE_MEMORY(mine->velocity.x);
		matrixZ = inst->matrix.m[2][2];
		mine->velocity.y = POTION_THROW_Y_VELOCITY;
		mine->crateInst = NULL;
		tailValue = (u16)mine->flags;
		tailValue |= MINE_WEAPON_FLAG_THROWN;
		mine->flags = (u16)tailValue;
		CTR_PSX_OBSERVE_MEMORY(mine->flags);
		tailValue = (matrixZ * -POTION_THROW_SPEED) >> POTION_THROW_MATRIX_SHIFT;
	}
	else
	{
		if ((flags & POTION_THROW_RANDOM) == 0)
		{
			return 0;
		}

		throwVelocity = (RngDeadCoed(&VEH_ADV_RNG) & POTION_THROW_RANDOM_MASK) - POTION_THROW_RANDOM_BIAS;
		tailValue = inst->matrix.m[0][2];
		mine->velocity.x = (s16)((tailValue * throwVelocity) >> POTION_THROW_MATRIX_SHIFT);
		CTR_PSX_OBSERVE_MEMORY(mine->velocity.x);
		tailValue = inst->matrix.m[2][2];
		randomVelocityZ = (tailValue * throwVelocity) >> POTION_THROW_MATRIX_SHIFT;
		mine->velocity.y = POTION_THROW_Y_VELOCITY;
		mine->crateInst = NULL;
		tailValue = (u16)mine->flags;
		result = 1;
		tailValue |= MINE_WEAPON_FLAG_THROWN;
		mine->flags = (u16)tailValue;
		CTR_PSX_OBSERVE_MEMORY(mine->flags);
		tailValue = randomVelocityZ;
	}

	mine->velocity.z = (s16)tailValue;
	return result;
}

// NOTE(aalhendi): Most retail weapon paths assume allocation succeeds. Native
// skips failed births and releases any resources reserved before the failure.
void VehPickupItem_ShootNow(struct Driver *d, s32 weaponID, s32 flags)
{
	union
	{
		SVECTOR missileRotation;
		SVec3 mineProbeTop;
	} shootScratch;
	SVec3 mineProbeBottom;
	SVec3 beakerProbeTop;
	SVec3 beakerProbeBottom;
#ifndef CTR_NATIVE
	// NOTE(aalhendi): Taking each label's address keeps GCC's case blocks alive,
	// while dispatch still reads the original resident jump table.
	static void *const shootNowCaseLabels[] = {
	    &&ShootNowTurbo, &&ShootNowDone,  &&ShootNowBombMissile, &&ShootNowMine, &&ShootNowBeaker, &&ShootNowDone,         &&ShootNowShield,
	    &&ShootNowMask,  &&ShootNowClock, &&ShootNowWarpball,    &&ShootNowDone, &&ShootNowDone,   &&ShootNowInvisibility, &&ShootNowSuperEngine,
	};

	if ((u32)weaponID >= 14)
	{
		goto ShootNowDone;
	}
	VEH_PICKUP_SHOOT_NOW_DISPATCH(weaponID);
#endif
	switch (weaponID)
	{
	// Turbo
	case WEAPON_ID_TURBO:
#ifndef CTR_NATIVE
	ShootNowTurbo:
#endif
	{
		int boost = TURBO_ITEM_BOOST_NORMAL;
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			boost = TURBO_ITEM_BOOST_JUICED;
		}

		VehFire_Increment(d, TURBO_ITEM_RESERVES, TURBO_ITEM_FIRE_FLAGS, boost);
	}
	break;

	// Mask
	case WEAPON_ID_MASK:
#ifndef CTR_NATIVE
	ShootNowMask:
#endif
		VehPickupItem_MaskUseWeapon(d, true);
		break;

	// Shared code for Bomb and Missile
	case WEAPON_ID_BOMB_MISSILE:
#ifndef CTR_NATIVE
	ShootNowBombMissile:
#endif
	{
		register struct Instance *weaponInst CTR_PSX_REGISTER("$18");
		struct TrackerWeapon *tw;
		register struct Driver *victim CTR_PSX_REGISTER("$17");
		register MATRIX *weaponMatrix CTR_PSX_REGISTER("$20");
		int closest;

		if ((s32)GAME_TRACKER->numMissiles >= ACTIVE_MISSILE_LIMIT)
		{
			return;
		}

		closest = MISSILE_TARGET_DISTANCE_SENTINEL;
		d->numTimesMissileLaunched++;
		GAME_TRACKER->numMissiles++;

		GAMEPAD_ShockFreq(d, WEAPON_GAMEPAD_RUMBLE_FRAMES, 0);
		GAMEPAD_ShockForce1(d, WEAPON_GAMEPAD_RUMBLE_FRAMES, WEAPON_GAMEPAD_RUMBLE_FORCE);

		victim = VehPickupItem_MissileGetTargetDriver(d);
		if (victim == 0)
		{
			CTR_PSX_FORGET_VALUE(victim);
			if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0)
			{
				int i;

				for (i = 0; i < MISSILE_TARGET_DRIVER_COUNT; i++)
				{
					struct Driver *candidate = GAME_TRACKER->drivers[i];
#ifdef CTR_NATIVE
					int distX;
					int distZ;
#endif
					register int dist CTR_PSX_REGISTER("$3");

					if (candidate == 0)
					{
						continue;
					}
					if (candidate == d)
					{
						continue;
					}
					if (candidate->kartState == KS_MASK_GRABBED)
					{
						continue;
					}
					if (candidate->BattleHUD.teamID == d->BattleHUD.teamID)
					{
						continue;
					}
					if (candidate->invisibleTimer != 0)
					{
						continue;
					}

#ifdef CTR_NATIVE
					distX = CTR_MipsSra(CTR_MipsSubLo(candidate->posCurr.x, d->posCurr.x), MISSILE_TARGET_POS_SHIFT);
					distZ = CTR_MipsSra(CTR_MipsSubLo(candidate->posCurr.z, d->posCurr.z), MISSILE_TARGET_POS_SHIFT);
					dist = CTR_MipsAddLo(CTR_MipsMulLo(distX, distX), CTR_MipsMulLo(distZ, distZ));
#else
					// NOTE(aalhendi): GCC otherwise assigns the second MFLO to t6.
					// Keep retail's v1 reuse without exposing this schedule to native;
					// a broad memory clobber also destroys the required allocation.
					__asm__ volatile("lw $2,724(%1)\n\t"
					                 "lw $3,724(%2)\n\t"
					                 "nop\n\t"
					                 "subu $2,$2,$3\n\t"
					                 "sra $2,$2,8\n\t"
					                 "mult $2,$2\n\t"
					                 "lw $2,732(%1)\n\t"
					                 "lw $3,732(%2)\n\t"
					                 "mflo $5\n\t"
					                 "subu $2,$2,$3\n\t"
					                 "sra $2,$2,8\n\t"
					                 "mult $2,$2\n\t"
					                 "mflo %0\n\t"
					                 "addu %0,$5,%0"
					                 : "=r"(dist)
					                 : "r"(candidate), "r"(d)
					                 : "$2", "$5");
#endif
					if (dist < closest)
					{
						closest = dist;
						victim = candidate;
					}
				}
			}
			else if (d->instSelf->thread->modelIndex != DYNAMIC_PLAYER)
			{
				int rank = d->driverRank;

				if ((rank != 0) && ((GAME_TRACKER->elapsedEventTime & MISSILE_RACE_FALLBACK_EVENT_MASK) != 0))
				{
					victim = GAME_TRACKER->driversInRaceOrder[rank - 1];
				}
			}
		}

		if ((d->heldItemID == HELD_ITEM_MISSILE_1X) || (d->heldItemID == HELD_ITEM_MISSILE_3X))
		{
			weaponInst =
			    INSTANCE_BirthWithThread(DYNAMIC_ROCKET, rdata.s_bombtracker1, MEDIUM, TRACKING, RB_MovingExplosive_ThTick, sizeof(struct TrackerWeapon), 0);
		}
		else
		{
			weaponInst = INSTANCE_BirthWithThread(DYNAMIC_BOMB, VEH_PICKUP_BOMB_NAME, MEDIUM, OTHER, RB_MovingExplosive_ThTick, sizeof(struct TrackerWeapon),
			                                      d->instSelf->thread);
		}

#ifdef CTR_NATIVE
		if (weaponInst == NULL)
		{
			d->numTimesMissileLaunched--;
			GAME_TRACKER->numMissiles--;
			return;
		}
#endif
		weaponMatrix = &weaponInst->matrix;
		{
			register MATRIX *matrixArgument CTR_PSX_REGISTER("$4");
			register const struct Instance *copySource CTR_PSX_REGISTER("$2");

			copySource = d->instSelf;
			CTR_PSX_KEEP_VALUE(copySource);
			matrixArgument = weaponMatrix;
			CTR_PSX_KEEP_VALUE(matrixArgument);
			VehPickupItem_CopyInstanceMatrixForPhysics(weaponInst, copySource, matrixArgument);
			VehPhysForce_RotAxisAngle(matrixArgument, CTR_VECTOR_DATA(&d->AxisAngle1_normalVec), d->rotCurr.y);
		}

		weaponInst->thread->funcThDestroy = PROC_DestroyTracker;
		weaponInst->thread->funcThCollide = (void *)RB_Hazard_ThCollide_Missile;

		tw = weaponInst->thread->object;
		tw->flags = 0;
		tw->driverParent = d;
		tw->timeAlive = 0;
		tw->framesSeekTargetTnt = 0;
		tw->soundIDCount = 0;

		if ((d->heldItemID == HELD_ITEM_BOMB_1X) || (d->heldItemID == HELD_ITEM_BOMB_3X))
		{
			CTR_MatrixToRot(&shootScratch.missileRotation, &weaponInst->matrix, 0x11);
			tw->dir.x = shootScratch.missileRotation.vy;
			tw->dir.y = shootScratch.missileRotation.vx;
			tw->dir.z = shootScratch.missileRotation.vz;
			tw->driverTarget = victim;
			d->instBombThrow = weaponInst;

			PlaySound3D(SOUND_BOMB_LAUNCH, weaponInst);
			if ((d->actionsFlagSet & ACTION_BOT) == 0)
			{
				Voiceline_RequestPlay(VOICELINE_BOMB_LAUNCH, GAME_CHARACTER_IDS[d->driverID], VOICELINE_WEAPON_PRIORITY);
			}
		}
		else
		{
			weaponInst->thread->funcThCollide = (void *)RB_Hazard_ThCollide_Missile;
			if (victim != 0)
			{
				tw->driverTarget = victim;
				if (victim->thTrackingMe == 0)
				{
					victim->thTrackingMe = RB_GetThread_ClosestTracker(victim);
				}
			}
			else
			{
				CTR_PSX_CLOBBER("$17");
				tw->driverTarget = 0;
			}
			PlaySound3D(SOUND_MISSILE_LAUNCH, weaponInst);
			if ((d->actionsFlagSet & ACTION_BOT) == 0)
			{
				Voiceline_RequestPlay(VOICELINE_MISSILE_LAUNCH, GAME_CHARACTER_IDS[d->driverID], VOICELINE_WEAPON_PRIORITY);
			}
		}

		tw->rotY = d->rotCurr.y;
		tw->vel.y = 0;

		if ((d->heldItemID == HELD_ITEM_BOMB_1X) || (d->heldItemID == HELD_ITEM_BOMB_3X))
		{
			struct GamepadBuffer *gamepad;

			tw->vel.x = (weaponInst->matrix.m[0][2] * TRACKER_LAUNCH_VELOCITY_NUMERATOR) >> TRACKER_LAUNCH_VELOCITY_SHIFT;
			tw->vel.z = (weaponInst->matrix.m[2][2] * TRACKER_LAUNCH_VELOCITY_NUMERATOR) >> TRACKER_LAUNCH_VELOCITY_SHIFT;
			if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
			{
				tw->flags |= TRACKER_FLAG_POWERED_UP;
			}

			gamepad = &GAMEPADS->gamepad[d->driverID];
			if (((gamepad->buttonsHeldCurrFrame & BTN_DOWN) != 0) || ((flags & SHOOT_NOW_BACKWARD) != 0))
			{
				tw->flags |= TRACKER_FLAG_BOMB_BACKWARD;
				tw->vel.x = -(((tw->vel.x >> 1) * 3) / 5);
				tw->vel.z = -(((tw->vel.z >> 1) * 3) / 5);
			}
		}
		else if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			tw->vel.x = (weaponInst->matrix.m[0][2] * TRACKER_LAUNCH_VELOCITY_NUMERATOR) >> TRACKER_LAUNCH_VELOCITY_SHIFT;
			tw->vel.z = (weaponInst->matrix.m[2][2] * TRACKER_LAUNCH_VELOCITY_NUMERATOR) >> TRACKER_LAUNCH_VELOCITY_SHIFT;
			tw->flags |= TRACKER_FLAG_POWERED_UP;
		}
		else
		{
			tw->vel.x = (weaponInst->matrix.m[0][2] * MISSILE_TRACKER_VELOCITY_NUMERATOR) >> MISSILE_TRACKER_VELOCITY_SHIFT;
			tw->vel.z = (weaponInst->matrix.m[2][2] * MISSILE_TRACKER_VELOCITY_NUMERATOR) >> MISSILE_TRACKER_VELOCITY_SHIFT;
		}

		tw->parentSafetyFrames = TRACKER_PARENT_SAFETY_FRAMES;
		tw->blindFrames = 0;
		tw->instParent = d->instSelf;
		break;
	}

	// Clock
	case WEAPON_ID_CLOCK:
#ifndef CTR_NATIVE
	ShootNowClock:
#endif
	{
		int i;

		d->numTimesClockWeaponUsed++;
		OtherFX_Play(SOUND_CLOCK, 1);

		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(VOICELINE_CLOCK, GAME_CHARACTER_IDS[d->driverID], VOICELINE_WEAPON_PRIORITY);
		}

		for (i = 0; i < CLOCK_DRIVER_COUNT; i++)
		{
			struct Driver *victim;

#ifdef CTR_NATIVE
			// NOTE(aalhendi): Retail writes before its null check; native must skip empty driver slots.
			if (GAME_TRACKER->drivers[i] == 0)
			{
				continue;
			}
#endif
			GAME_TRACKER->drivers[i]->clockFlash = CLOCK_FLASH_FRAMES;
			victim = GAME_TRACKER->drivers[i];
			if (victim == 0)
			{
				continue;
			}
			if (victim != d)
			{
				if (RB_Hazard_HurtDriver(victim, CLOCK_HURT_REASON, 0, 0) != 0)
				{
					if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
					{
						GAME_TRACKER->drivers[i]->clockReceive = CLOCK_HURT_DURATION_JUICED;
					}
					else
					{
						GAME_TRACKER->drivers[i]->clockReceive = CLOCK_HURT_DURATION_NORMAL;
					}
				}
			}
			else
			{
				d->clockSend = CLOCK_SELF_SEND_FRAMES;
			}
		}
		break;
	}

	// Shield Bubble
	case WEAPON_ID_SHIELD:
#ifndef CTR_NATIVE
	ShootNowShield:
#endif
	{
		register struct Instance *weaponInst CTR_PSX_REGISTER("$18");
		register s32 scale CTR_PSX_REGISTER("$16");
		register struct Shield *shieldObj CTR_PSX_REGISTER("$17");

		weaponInst = INSTANCE_BirthWithThread(SHIELD_DARK_MODEL, VEH_PICKUP_SHIELD_DARK_NAME, MEDIUM, OTHER, RB_ShieldDark_ThTick_Grow, sizeof(struct Shield),
		                                      d->instSelf->thread);

#ifdef CTR_NATIVE
		if (weaponInst == NULL)
		{
			return;
		}
#endif
		weaponInst->scale.x = SHIELD_SCALE;
		weaponInst->scale.y = SHIELD_SCALE;
		weaponInst->scale.z = SHIELD_SCALE;

		weaponInst->thread->funcThDestroy = PROC_DestroyInstance;
		OtherFX_Play(SOUND_SHIELD, 1);

		shieldObj = weaponInst->thread->object;
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			shieldObj->instColor = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[DYNAMIC_SHIELD], VEH_PICKUP_SHIELD_NAME, weaponInst->thread);
		}
		else
		{
			shieldObj->instColor = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[DYNAMIC_SHIELD_GREEN], VEH_PICKUP_SHIELD_NAME, weaponInst->thread);
		}

#ifdef CTR_NATIVE
		if (shieldObj->instColor == NULL)
		{
			goto ShieldBirthFailed;
		}
#endif
		scale = SHIELD_SCALE;
		shieldObj->instColor->scale.x = scale;
		shieldObj->instColor->scale.y = scale;
		shieldObj->instColor->scale.z = scale;

		shieldObj->instHighlight = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[DYNAMIC_HIGHLIGHT], VEH_PICKUP_HIGHLIGHT_NAME, weaponInst->thread);

#ifdef CTR_NATIVE
		if (shieldObj->instHighlight == NULL)
		{
			INSTANCE_Death(shieldObj->instColor);
			goto ShieldBirthFailed;
		}
#endif
		shieldObj->instHighlight->scale.x = scale;
		shieldObj->instHighlight->scale.y = scale;
		shieldObj->instHighlight->scale.z = scale;

		shieldObj->flags = 0;
		shieldObj->highlightRot.x = 0;
		shieldObj->highlightRot.y = SHIELD_HIGHLIGHT_ROT_Y;
		shieldObj->highlightRot.z = 0;
		shieldObj->highlightTimer = 0;

		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			shieldObj->flags |= SHIELD_FLAG_BLUE;
		}
		else
		{
			shieldObj->duration = SHIELD_DURATION_NORMAL;
		}

		weaponInst->alphaScale = SHIELD_ALPHA_SCALE;
		shieldObj->animFrame = 0;
		d->instBubbleHold = weaponInst;
		break;

#ifdef CTR_NATIVE
	ShieldBirthFailed:
		// NOTE(aalhendi): Birth inserted this thread at the front of the driver's
		// children. Unlink it before recycling the unfinished shield and object.
		d->instSelf->thread->childThread = weaponInst->thread->siblingThread;
		PROC_DestroySelf(weaponInst->thread);
		return;
#endif
	}

	// Warpball
	case WEAPON_ID_WARPBALL:
#ifndef CTR_NATIVE
	ShootNowWarpball:
#endif
	{
		register struct Instance *weaponInst CTR_PSX_REGISTER("$18");
		register struct TrackerWeapon *tw CTR_PSX_REGISTER("$16");
		struct Driver *victim;
		struct CheckpointNode *checkpoints;
		struct Particle *p;

		GAMEPAD_ShockFreq(d, WEAPON_GAMEPAD_RUMBLE_FRAMES, 0);
		GAMEPAD_ShockForce1(d, WEAPON_GAMEPAD_RUMBLE_FRAMES, WEAPON_GAMEPAD_RUMBLE_FORCE);

		weaponInst = INSTANCE_BirthWithThread(WARPBALL_MODEL, rdata.s_warpball, MEDIUM, TRACKING, RB_Warpball_ThTick, sizeof(struct TrackerWeapon), 0);

#ifdef CTR_NATIVE
		if (weaponInst == NULL)
		{
			return;
		}
#endif
		weaponInst->matrix.t[0] = CTR_MipsSra(d->posCurr.x, WARPBALL_POS_SHIFT);
		weaponInst->matrix.t[1] = CTR_MipsSra(d->posCurr.y, WARPBALL_POS_SHIFT);
		weaponInst->matrix.t[2] = CTR_MipsSra(d->posCurr.z, WARPBALL_POS_SHIFT);

		VehPickupItem_SetWarpballMatrixPair(&weaponInst->matrix, 0, WARPBALL_MATRIX_IDENTITY_SCALE, 0);
		VehPickupItem_SetWarpballMatrixPair(&weaponInst->matrix, 2, 0, 0);
		VehPickupItem_SetWarpballMatrixPair(&weaponInst->matrix, 4, WARPBALL_MATRIX_IDENTITY_SCALE, 0);
		VehPickupItem_SetWarpballMatrixPair(&weaponInst->matrix, 6, 0, 0);
		weaponInst->matrix.m[2][2] = WARPBALL_MATRIX_IDENTITY_SCALE;

		weaponInst->thread->funcThDestroy = PROC_DestroyInstance;

		PlaySound3D(SOUND_WARPBALL, weaponInst);

		// if human and not AI (AIs can not use Warpball)
		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(VOICELINE_WARPBALL, GAME_CHARACTER_IDS[d->driverID], VOICELINE_WEAPON_PRIORITY);
		}

		tw = weaponInst->thread->object;
		tw->driverParent = d;
		tw->turnAroundFrames = 0;
		tw->ptrNodeNext = 0;
		victim = 0;
		if (d->driverRank != 0)
		{
			victim = GAME_TRACKER->driversInRaceOrder[d->driverRank - 1];
		}
		tw->driverTarget = victim;

		RB_Warpball_SeekDriver(tw, d->checkpoint.currentIndex, d);

		checkpoints = GAME_TRACKER->level1->ptr_restart_points;
		tw->nodeNextIndex = tw->nodeCurrIndex;
		tw->ptrNodeCurr = &checkpoints[tw->nodeCurrIndex];
		tw->flags = 0;
		tw->pathProgress = 0;

		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			tw->flags |= TRACKER_FLAG_POWERED_UP;
		}

		tw->flags |= TRACKER_FLAG_WARPBALL_FALLBACK_PATH;

		// make this driver invincible
		tw->driversHit = 1 << d->driverID;

		if (d->driverRank == 0)
		{
			tw->driverTarget = 0;
		}
		else
		{
			tw->driverTarget = RB_Warpball_GetDriverTarget(tw, weaponInst);
		}
		if (d->driverRank == 0)
		{
			tw->driverTarget = 0;
		}

		if (tw->driverTarget != 0)
		{
			RB_Warpball_SetTargetDriver(tw);
		}

		if ((tw->flags & TRACKER_FLAG_WARPBALL_TARGET_PATH) != 0)
		{
			tw->flags &= ~TRACKER_FLAG_WARPBALL_FALLBACK_PATH;
		}
		else
		{
			RB_Warpball_Start(tw);
		}

		tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, tw->driverTarget);

		tw->soundIDCount = 0;

		// do NOT patch for 60fps,
		// velocity uses elapsedTime
		tw->vel.x = (d->instSelf->matrix.m[0][2] * WARPBALL_VELOCITY_NUMERATOR) >> WARPBALL_VELOCITY_SHIFT;
		tw->vel.y = 0;
		tw->vel.z = (d->instSelf->matrix.m[2][2] * WARPBALL_VELOCITY_NUMERATOR) >> WARPBALL_VELOCITY_SHIFT;
		tw->parentSafetyFrames = WARPBALL_PARENT_SAFETY_FRAMES;
		tw->dir.y = d->angle;
		tw->instParent = d->instSelf;

		p = Particle_Init(0, GAME_TRACKER->iconGroup[WARPBALL_PARTICLE_ICON_GROUP], &data.emSet_Warpball[0]);

		tw->ptrParticle = p;

		if (p != 0)
		{
			p->otIndexOffset = (s8)WARPBALL_PARTICLE_OT_OFFSET;
		}

		break;
	}

	// TNT/Nitro
	case WEAPON_ID_MINE:
#ifndef CTR_NATIVE
	ShootNowMine:
#endif
	{
		register struct Instance *weaponInst CTR_PSX_REGISTER("$18");
		register struct MineWeapon *mw CTR_PSX_REGISTER("$16");
		register struct ScratchpadStruct *sps CTR_PSX_REGISTER("$17");
		register s32 soundID CTR_PSX_REGISTER("$4");
		register struct Instance *soundInstance CTR_PSX_REGISTER("$5");
		register struct InstDef *instDef CTR_PSX_REGISTER("$3");
		register s32 modelTouched CTR_PSX_REGISTER("$4");
		register MATRIX *matrixArgument CTR_PSX_REGISTER("$4");
		register s16 *rotationNormal CTR_PSX_REGISTER("$5");
		register s32 rotationAngle CTR_PSX_REGISTER("$6");

		sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, MINE_COLL_SCRATCH_OFFSET);

		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			weaponInst = INSTANCE_BirthWithThread(PU_EXPLOSIVE_CRATE, VEH_PICKUP_NITRO_NAME, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);
		}
		else
		{
			weaponInst = INSTANCE_BirthWithThread(STATIC_CRATE_TNT, VEH_PICKUP_TNT_NAME, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);
		}

#ifdef CTR_NATIVE
		if (weaponInst == NULL)
		{
			return;
		}
#endif
		{
			register const struct Instance *copySource CTR_PSX_REGISTER("$2");

			soundID = SOUND_MINE_DROP;
			CTR_PSX_KEEP_VALUE(soundID);
			copySource = d->instSelf;
			CTR_PSX_KEEP_VALUE(copySource);
			soundInstance = weaponInst;
			CTR_PSX_KEEP_VALUE(soundInstance);
			VehPickupItem_CopyInstanceMatrix(weaponInst, copySource);
		}

		weaponInst->scale.x = 0;
		weaponInst->scale.y = 0;
		weaponInst->scale.z = 0;

		weaponInst->thread->funcThDestroy = PROC_DestroyInstance;
		weaponInst->thread->funcThCollide = (void *)RB_Hazard_ThCollide_Generic;

		PlaySound3D(soundID, soundInstance);

		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(VOICELINE_MINE_DROP, GAME_CHARACTER_IDS[d->driverID], VOICELINE_WEAPON_PRIORITY);
		}

		mw = weaponInst->thread->object;
		mw->instParent = d->instSelf;
		mw->velocity.x = 0;
		mw->velocity.y = 0;
		mw->velocity.z = 0;
		mw->parentSafetyFrames = MINE_PARENT_SAFETY_FRAMES;
		mw->boolDestroyed = 0;
		mw->tntSpinY = 0;
		mw->driverTarget = 0;
		mw->crateInst = 0;
		mw->flags = 0;

		RB_MinePool_Add(mw);
		VehPickupItem_PotionThrow(mw, weaponInst, flags);

		shootScratch.mineProbeTop.x = (s16)(u16)weaponInst->matrix.t[0];
		shootScratch.mineProbeTop.y = (s16)CTR_MipsAddLo((u16)weaponInst->matrix.t[1], MINE_COLL_PROBE_TOP_Y_OFFSET);
		shootScratch.mineProbeTop.z = (s16)(u16)weaponInst->matrix.t[2];

		mineProbeBottom.x = (s16)(u16)weaponInst->matrix.t[0];
		mineProbeBottom.y = (s16)CTR_MipsAddLo((u16)weaponInst->matrix.t[1], MINE_COLL_PROBE_BOTTOM_Y_OFFSET);
		{
			register struct GameTracker *gameTracker CTR_PSX_REGISTER("$3");
			register s32 probeBottomZ CTR_PSX_REGISTER("$4");

			gameTracker = GAME_TRACKER;
			CTR_PSX_KEEP_VALUE(gameTracker);
			probeBottomZ = (u16)weaponInst->matrix.t[2];
			CTR_PSX_KEEP_VALUE(probeBottomZ);
			sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
			sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
			sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES;
			mineProbeBottom.z = (s16)probeBottomZ;
			if (gameTracker->numPlyrCurrGame < MINE_COLL_SEARCH_PLAYER_THRESHOLD)
			{
				sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD;
			}
		}
		sps->ptr_mesh_info = GAME_TRACKER->level1->ptr_mesh_info;
		COLL_SearchBSP_CallbackQUADBLK(&shootScratch.mineProbeTop, &mineProbeBottom, sps, MINE_COLL_CALLBACK_FLAGS);

		if ((u16)sps->boolDidTouchHitbox != 0)
		{
			sps->Input1.modelID = weaponInst->model->id | COLL_MODELID_BLOCKAGE_FLAG;
			RB_Hazard_CollLevInst(sps, weaponInst->thread);

			instDef = sps->bspHitbox->data.hitbox.instDef;
			CTR_PSX_LOAD_SIGNED_HALF_VOLATILE(modelTouched, instDef, offsetof(struct InstDef, modelID), *(VehPickupItemSignedHalfword *)&instDef->modelID);
			if (modelTouched == MINE_HITBOX_FRUIT_MODEL)
			{
				goto MineHitCrate;
			}
			if (modelTouched != MINE_HITBOX_RANDOM_MODEL)
			{
				goto MineDestroy;
			}

		MineHitCrate:
			mw->crateInst = instDef->ptrInstance;
			goto MineHitboxDone;

		MineDestroy:
			RB_GenericMine_ThDestroy(weaponInst->thread, weaponInst, mw);
		MineHitboxDone:

			sps->Union.QuadBlockColl.searchFlags = 0;
			COLL_SearchBSP_CallbackQUADBLK(&shootScratch.mineProbeTop, &mineProbeBottom, sps, 0);
		}
		else
		{
			mw->crateInst = 0;
		}

		RB_MakeInstanceReflective(sps, weaponInst);

		matrixArgument = &weaponInst->matrix;
		if (sps->boolDidTouchQuadblock != 0)
		{
			mw->stopFallAtY = sps->Union.QuadBlockColl.hitPos.y;
			rotationAngle = d->angle;
			CTR_PSX_KEEP_VALUE(rotationAngle);
			rotationNormal = VehPickupItem_GetCollisionNormal(sps);
		}
		else
		{
			mw->stopFallAtY = weaponInst->matrix.t[1];
			shootScratch.mineProbeTop.x = 0;
			shootScratch.mineProbeTop.y = COLL_FRACTION_ONE;
			shootScratch.mineProbeTop.z = 0;
			rotationAngle = d->angle;
			CTR_PSX_KEEP_VALUE(rotationAngle);
			rotationNormal = CTR_VECTOR_DATA(&shootScratch.mineProbeTop);
		}

		VehPhysForce_RotAxisAngle(matrixArgument, rotationNormal, rotationAngle);

		d->instTntSend = weaponInst;
		d->actionsFlagSet |= ACTION_DROPPING_MINE;
		if (flags == 0)
		{
			RB_Follower_Init(d, weaponInst->thread);
		}
		break;
	}

	// Beaker
	case WEAPON_ID_BEAKER:
#ifndef CTR_NATIVE
	ShootNowBeaker:
#endif
	{
		register struct Instance *weaponInst CTR_PSX_REGISTER("$18");
		register struct MineWeapon *mw CTR_PSX_REGISTER("$16");
		register struct ScratchpadStruct *sps CTR_PSX_REGISTER("$17");
		register struct InstDef *instDef CTR_PSX_REGISTER("$3");
		register s32 soundID CTR_PSX_REGISTER("$4");
		register struct Instance *soundInstance CTR_PSX_REGISTER("$5");
		register s32 modelTouched CTR_PSX_REGISTER("$4");
		register MATRIX *matrixArgument CTR_PSX_REGISTER("$4");
		register s16 *rotationNormal CTR_PSX_REGISTER("$5");
		register s32 rotationAngle CTR_PSX_REGISTER("$6");
		struct GamepadBuffer *gamepad;
		struct ModelHeader *modelHeader;
		u16 modelHeaderFlags;
		s32 potionFlags;

		sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, MINE_COLL_SCRATCH_OFFSET);

		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			weaponInst = INSTANCE_BirthWithThread(STATIC_BEAKER_RED, VEH_PICKUP_BEAKER_NAME, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);
#ifdef CTR_NATIVE
			if (weaponInst == NULL)
			{
				return;
			}
#endif
			mw = weaponInst->thread->object;
			mw->flags = MINE_WEAPON_FLAG_RED_BEAKER;
		}
		else
		{
			weaponInst =
			    INSTANCE_BirthWithThread(STATIC_BEAKER_GREEN, VEH_PICKUP_BEAKER_NAME, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);
			if (weaponInst == 0)
			{
				return;
			}
			mw = weaponInst->thread->object;
			mw->flags = 0;
		}

		{
			register const struct Instance *copySource CTR_PSX_REGISTER("$2");

			copySource = d->instSelf;
			CTR_PSX_KEEP_VALUE(copySource);
			VehPickupItem_CopyInstanceMatrix(weaponInst, copySource);
		}

		modelHeader = weaponInst->model->headers;
		CTR_PSX_KEEP_VALUE(modelHeader);
		soundID = SOUND_MINE_DROP;
		CTR_PSX_KEEP_VALUE(soundID);
		modelHeaderFlags = modelHeader->flags;
		CTR_PSX_KEEP_VALUE(modelHeaderFlags);
		soundInstance = weaponInst;
		CTR_PSX_KEEP_VALUE(soundInstance);
		modelHeader->flags = modelHeaderFlags | BEAKER_MODEL_HEADER_CAMERA_FLAG;

		weaponInst->thread->funcThDestroy = PROC_DestroyInstance;
		weaponInst->thread->funcThCollide = (void *)RB_Hazard_ThCollide_Generic;

		{
			register struct Instance *parentInstance CTR_PSX_REGISTER("$3");

			parentInstance = d->instSelf;
			CTR_PSX_KEEP_VALUE(parentInstance);
			mw->parentSafetyFrames = MINE_PARENT_SAFETY_FRAMES;
			mw->boolDestroyed = 0;
			mw->crateInst = 0;
			mw->instParent = parentInstance;

			PlaySound3D(soundID, soundInstance);
		}

		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(VOICELINE_MINE_DROP, GAME_CHARACTER_IDS[d->driverID], VOICELINE_WEAPON_PRIORITY);
		}

		RB_MinePool_Add(mw);

		gamepad = &GAMEPADS->gamepad[d->driverID];
		potionFlags = flags;
		if ((gamepad->buttonsHeldCurrFrame & BTN_UP) != 0)
		{
			potionFlags |= POTION_THROW_FORWARD;
		}

		if ((s16)VehPickupItem_PotionThrow(mw, weaponInst, potionFlags) != 0)
		{
			break;
		}

		weaponInst->scale.x = 0;
		weaponInst->scale.y = 0;
		weaponInst->scale.z = 0;
		mw->velocity.x = 0;
		mw->velocity.y = 0;
		mw->velocity.z = 0;

		beakerProbeTop.x = (s16)(u16)weaponInst->matrix.t[0];
		beakerProbeTop.y = (s16)CTR_MipsAddLo((u16)weaponInst->matrix.t[1], MINE_COLL_PROBE_TOP_Y_OFFSET);
		beakerProbeTop.z = (s16)(u16)weaponInst->matrix.t[2];

		beakerProbeBottom.x = (s16)(u16)weaponInst->matrix.t[0];
		beakerProbeBottom.y = (s16)CTR_MipsAddLo((u16)weaponInst->matrix.t[1], MINE_COLL_PROBE_BOTTOM_Y_OFFSET);
		{
			register struct GameTracker *gameTracker CTR_PSX_REGISTER("$3");
			register s32 probeBottomZ CTR_PSX_REGISTER("$4");

			gameTracker = GAME_TRACKER;
			CTR_PSX_KEEP_VALUE(gameTracker);
			probeBottomZ = (u16)weaponInst->matrix.t[2];
			CTR_PSX_KEEP_VALUE(probeBottomZ);
			sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
			sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
			sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES;
			beakerProbeBottom.z = (s16)probeBottomZ;
			if (gameTracker->numPlyrCurrGame < MINE_COLL_SEARCH_PLAYER_THRESHOLD)
			{
				sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD;
			}
		}
		sps->ptr_mesh_info = GAME_TRACKER->level1->ptr_mesh_info;
		COLL_SearchBSP_CallbackQUADBLK(&beakerProbeTop, &beakerProbeBottom, sps, MINE_COLL_CALLBACK_FLAGS);

		if ((u16)sps->boolDidTouchHitbox != 0)
		{
			sps->Input1.modelID = weaponInst->model->id;
			RB_Hazard_CollLevInst(sps, weaponInst->thread);

			instDef = sps->bspHitbox->data.hitbox.instDef;
			CTR_PSX_LOAD_SIGNED_HALF_VOLATILE(modelTouched, instDef, offsetof(struct InstDef, modelID), *(VehPickupItemSignedHalfword *)&instDef->modelID);
			if (modelTouched == MINE_HITBOX_FRUIT_MODEL)
			{
				goto BeakerHitCrate;
			}
			if (modelTouched != MINE_HITBOX_RANDOM_MODEL)
			{
				goto BeakerDestroy;
			}

		BeakerHitCrate:
			mw->crateInst = instDef->ptrInstance;
			goto BeakerHitboxDone;

		BeakerDestroy:
			RB_GenericMine_ThDestroy(weaponInst->thread, weaponInst, mw);
		BeakerHitboxDone:
			sps->Union.QuadBlockColl.searchFlags = 0;
			COLL_SearchBSP_CallbackQUADBLK(&beakerProbeTop, &beakerProbeBottom, sps, 0);
		}
		else
		{
			mw->crateInst = 0;
		}

		RB_MakeInstanceReflective(sps, weaponInst);

		matrixArgument = &weaponInst->matrix;
		if (sps->boolDidTouchQuadblock != 0)
		{
			mw->stopFallAtY = sps->Union.QuadBlockColl.hitPos.y;
			rotationAngle = d->angle;
			CTR_PSX_KEEP_VALUE(rotationAngle);
			rotationNormal = VehPickupItem_GetCollisionNormal(sps);
		}
		else
		{
			mw->stopFallAtY = weaponInst->matrix.t[1];
			beakerProbeTop.x = 0;
			beakerProbeTop.y = COLL_FRACTION_ONE;
			beakerProbeTop.z = 0;
			rotationAngle = d->angle;
			CTR_PSX_KEEP_VALUE(rotationAngle);
			rotationNormal = CTR_VECTOR_DATA(&beakerProbeTop);
		}

		VehPhysForce_RotAxisAngle(matrixArgument, rotationNormal, rotationAngle);
		RB_Follower_Init(d, weaponInst->thread);
		d->actionsFlagSet |= ACTION_DROPPING_MINE;
		break;
	}


	// Super Engine
	case WEAPON_ID_SUPER_ENGINE:
#ifndef CTR_NATIVE
	ShootNowSuperEngine:
#endif
	{
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			d->superEngineTimer = SUPER_ENGINE_DURATION_JUICED;
			break;
		}

		d->superEngineTimer = SUPER_ENGINE_DURATION_NORMAL;
	}
	break;

	// invisibility
	case WEAPON_ID_INVISIBILITY:
#ifndef CTR_NATIVE
	ShootNowInvisibility:
#endif
	{
		register s32 time CTR_PSX_REGISTER("$2");

		if (d->invisibleTimer == 0)
		{
			d->instFlagsBackup = d->instSelf->flags;

			d->instSelf->flags &= INVISIBILITY_CLEAR_DRAW_FLAGS;
			d->instSelf->flags |= GHOST_DRAW_TRANSPARENT;

			OtherFX_Play(SOUND_INVISIBILITY, 1);
		}

		time = d->numWumpas;
		time = time < DRIVER_WUMPA_JUICED_COUNT;
		if (time == 0)
		{
			time = INVISIBILITY_DURATION_JUICED;
		}
		else
		{
			time = INVISIBILITY_DURATION_NORMAL;
		}

		d->invisibleTimer = time;
		break;
	}
	}

#ifndef CTR_NATIVE
ShootNowDone:
#endif
    ;
}

void VehPickupItem_ShootOnCirclePress(struct Driver *d)
{
	s32 weapon;

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

	// Missiles and Bombs share code,
	// Change Bomb1x, Bomb3x, Missile3x, to Missile1x
	weapon = HELD_ITEM_BOMB_1X;
	if ((d->heldItemID != HELD_ITEM_BOMB_3X) && ((weapon = HELD_ITEM_BOMB_MISSILE_SHARED), d->heldItemID != HELD_ITEM_MISSILE_3X))
	{
		CTR_PSX_RELOAD(d->heldItemID);
		weapon = d->heldItemID;
	}
	if (weapon == HELD_ITEM_BOMB_1X)
	{
		weapon = HELD_ITEM_BOMB_MISSILE_SHARED;
	}

	VehPickupItem_ShootNow(d, (s32)weapon, SHOOT_NOW_NO_FLAGS);
}
