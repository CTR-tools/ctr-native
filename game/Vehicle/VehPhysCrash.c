#include "VehCommon.h"

bool VehPhysCrash_TrySquashSmallerKart(struct Driver *first, struct Driver *second)
{
	float firstScale = VehPhysForce_GetKartScale(first);
	float secondScale = VehPhysForce_GetKartScale(second);
	if (firstScale == secondScale)
	{
		return false;
	}
	struct Driver *large = firstScale > secondScale ? first : second;
	struct Driver *small = firstScale > secondScale ? second : first;
	if (small->squishTimer == 0)
	{
		if (!VehPickState_TrySquish(small))
		{
			return false;
		}
		// The Turbo item's squash gives the attacker a high forced hop.
		large->forcedJumpType = FORCED_JUMP_HIGH;
	}
	return true;
}

enum
{
	VEH_PHYS_CRASH_FAST_SQRT_ITERATIONS = 0x10,
	VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT = 8,
	VEH_PHYS_CRASH_MATRIX_FRAC_SHIFT = 0xc,
	VEH_PHYS_CRASH_UNIT_VECTOR_SCALE = 0x1000,
	VEH_PHYS_CRASH_BOUNCE_Y_CLAMP = 0x3200,
	VEH_PHYS_CRASH_BOUNCE_DOT_DIVISOR = 0xc00,
	VEH_PHYS_CRASH_BOT_NAV_ROT_SHIFT = 4,
	VEH_PHYS_CRASH_BOT_SPEED_SHIFT = 8,

	VEH_PHYS_CRASH_DAMAGE_TYPE_MASK = 2,
	VEH_PHYS_CRASH_DAMAGE_TYPE_TURBO = 3,
	VEH_PHYS_CRASH_DAMAGE_REASON_SHIELD = 0,
	VEH_PHYS_CRASH_DAMAGE_REASON_TURBO = 5,
	VEH_PHYS_CRASH_DAMAGE_REASON_MASK = 6,
	VEH_PHYS_CRASH_TURBO_DAMAGE_IMPACT = 0xa00,

	VEH_PHYS_CRASH_BUBBLE_POP_FX = 0x4f,
	VEH_PHYS_CRASH_VOICELINE_CRASH = 1,
	VEH_PHYS_CRASH_VOICELINE_HARD_CRASH = 5,
	VEH_PHYS_CRASH_VOICELINE_PRIORITY = 0x10,

	VEH_PHYS_CRASH_FEEDBACK_MIN_IMPACT = 0x200,
	VEH_PHYS_CRASH_VOLUME_IMPACT_MAX = 0x1900,
	VEH_PHYS_CRASH_VOLUME_MIN = 0x3f,
	VEH_PHYS_CRASH_VOLUME_MAX = 0xff,
	VEH_PHYS_CRASH_HARD_CRASH_VOLUME = 0xdc,
	VEH_PHYS_CRASH_FEEDBACK_COOLDOWN_FRAMES = 3,

	VEH_PHYS_CRASH_RUMBLE_CHANNEL = 8,
	VEH_PHYS_CRASH_RUMBLE_FORCE = 0x7f,
	VEH_PHYS_CRASH_JOG_TURNING = 0x29,
	VEH_PHYS_CRASH_JOG_STRAIGHT = 0x19,
	VEH_PHYS_CRASH_JOG_DURATION = 0x60,
};


static inline u32 VehPhysCrash_LengthSq2(s32 x, s32 z)
{
	return (u32)CTR_MipsAddLo(CTR_MipsMulLo(x, x), CTR_MipsMulLo(z, z));
}

static inline u32 VehPhysCrash_LengthSq3(s32 x, s32 y, s32 z)
{
	return (u32)CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(x, x), CTR_MipsMulLo(y, y)), CTR_MipsMulLo(z, z));
}

static inline s32 VehPhysCrash_Dot3(s32 ax, s32 ay, s32 az, s32 bx, s32 by, s32 bz)
{
	return CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(ax, bx), CTR_MipsMulLo(ay, by)), CTR_MipsMulLo(az, bz));
}

void VehPhysCrash_ConvertVecToSpeed(struct Driver *d, Vec3 *vel)
{
	register int speed2D CTR_PSX_REGISTER("$19") = VehCalc_FastSqrt(VehPhysCrash_LengthSq2(vel->x, vel->z), VEH_PHYS_CRASH_FAST_SQRT_ITERATIONS);
	int speed;
	int jumpSpeed;
	int approximateSpeed;
	register int projectionDot CTR_PSX_REGISTER("$2");
	register int projOnMovingDirAxis CTR_PSX_REGISTER("$19");
	int projX;
	int projY;
	int projZ;

	speed = (u32)VehCalc_FastSqrt(VehPhysCrash_LengthSq3(vel->x, vel->y, vel->z), VEH_PHYS_CRASH_FAST_SQRT_ITERATIONS) >> VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT;
	d->speed = speed;
	d->axisRotationY = (s16)ratan2(CTR_MipsSll(vel->y, VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT), speed2D);
	d->axisRotationX = (s16)ratan2(vel->x, vel->z);

	projectionDot = VehPhysCrash_Dot3(vel->x, vel->y, vel->z, d->matrixMovingDir.m[0][1], d->matrixMovingDir.m[1][1], d->matrixMovingDir.m[2][1]);
	CTR_PSX_OBSERVE_VALUE(projectionDot);
	projOnMovingDirAxis = CTR_MipsSra(projectionDot, VEH_PHYS_CRASH_MATRIX_FRAC_SHIFT);

	projX = CTR_MipsSra(CTR_MipsMulLo(d->matrixMovingDir.m[0][1], projOnMovingDirAxis), VEH_PHYS_CRASH_MATRIX_FRAC_SHIFT);
	projY = CTR_MipsSra(CTR_MipsMulLo(d->matrixMovingDir.m[1][1], projOnMovingDirAxis), VEH_PHYS_CRASH_MATRIX_FRAC_SHIFT);
	projZ = CTR_MipsSra(CTR_MipsMulLo(d->matrixMovingDir.m[2][1], projOnMovingDirAxis), VEH_PHYS_CRASH_MATRIX_FRAC_SHIFT);

	jumpSpeed = (u32)VehCalc_FastSqrt(VehPhysCrash_LengthSq3(projX, projY, projZ), VEH_PHYS_CRASH_FAST_SQRT_ITERATIONS) >> VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT;

	d->jumpHeightCurr = jumpSpeed;
	if (projOnMovingDirAxis < 0)
	{
		d->jumpHeightCurr = (s16)CTR_MipsNegLo(jumpSpeed);
	}

	projX = CTR_MipsSubLo(vel->x, projX);
	projY = CTR_MipsSubLo(vel->y, projY);
	projZ = CTR_MipsSubLo(vel->z, projZ);

	approximateSpeed =
	    (u32)VehCalc_FastSqrt(VehPhysCrash_LengthSq3(projX, projY, projZ), VEH_PHYS_CRASH_FAST_SQRT_ITERATIONS) >> VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT;

	d->speedApprox = approximateSpeed;

	if (VehPhysCrash_Dot3(projX, projY, projZ, d->matrixMovingDir.m[0][2], d->matrixMovingDir.m[1][2], d->matrixMovingDir.m[2][2]) < 0)
	{
		d->speedApprox = (s16)CTR_MipsNegLo(approximateSpeed);
	}
}

static inline int VehPhysCrash_ScaleBounceDot(int value)
{
	return value / VEH_PHYS_CRASH_BOUNCE_DOT_DIVISOR;
}

int VehPhysCrash_BounceSelf(const SVec3 *normal, const Vec3 *origin, Vec3 *vel, b32 boolOtherDriver)
{
	int diffX = CTR_MipsSubLo(vel->x, origin->x);
	int diffY = CTR_MipsSubLo(vel->y, origin->y);
	int diffZ = CTR_MipsSubLo(vel->z, origin->z);
	int dot = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(diffX, normal->x), CTR_MipsMulLo(diffY, normal->y)), CTR_MipsMulLo(diffZ, normal->z)),
	                      VEH_PHYS_CRASH_MATRIX_FRAC_SHIFT);
	int impactStrength;
	int absDot;
	int oldY;

	if (boolOtherDriver != 0)
	{
		if (dot <= 0)
		{
			return 0;
		}
	}
	else if (dot >= 0)
	{
		return 0;
	}

	impactStrength = sdata->vehicleCollisionImpactStrength;
	absDot = dot;
	if (dot < 0)
	{
		CTR_PSX_FORGET_VALUE(absDot);
		absDot = CTR_MipsNegLo(absDot);
	}

	if (impactStrength < absDot)
	{
		sdata->vehicleCollisionImpactStrength = absDot;
	}

	diffX = CTR_MipsSubLo(diffX, VehPhysCrash_ScaleBounceDot(CTR_MipsMulLo(dot, normal->x)));
	diffY = CTR_MipsSubLo(diffY, VehPhysCrash_ScaleBounceDot(CTR_MipsMulLo(dot, normal->y)));
	diffZ = CTR_MipsSubLo(diffZ, VehPhysCrash_ScaleBounceDot(CTR_MipsMulLo(dot, normal->z)));

	vel->x = CTR_MipsAddLo(diffX, origin->x);

	oldY = vel->y;
	diffY = CTR_MipsAddLo(diffY, origin->y);
	if ((oldY < diffY) && (diffY > VEH_PHYS_CRASH_BOUNCE_Y_CLAMP))
	{
		diffY = VEH_PHYS_CRASH_BOUNCE_Y_CLAMP;
	}
	vel->y = diffY;
	vel->z = CTR_MipsAddLo(diffZ, origin->z);

	return 0;
}

void VehPhysCrash_AI(struct Driver *bot, Vec3 *vel)
{
	register MATRIX *matrix CTR_PSX_REGISTER("$16") = &VEH_PHYS_CRASH_MATRIX;
	register Vec3 *forward CTR_PSX_REGISTER("$7");
	register SVec3 *rotationArgument CTR_PSX_REGISTER("$5");
	s16 matrixForwardZ;
	int botSpeed;
	int velocityZProduct;
	int velocityZ;

	sdata->botCrashNavRot.x = (s16)CTR_MipsSll(bot->botData.botNavFrame->rot[0], VEH_PHYS_CRASH_BOT_NAV_ROT_SHIFT);
	sdata->botCrashNavRot.y = (s16)CTR_MipsSll(bot->botData.botNavFrame->rot[1], VEH_PHYS_CRASH_BOT_NAV_ROT_SHIFT);
	CTR_PSX_OBSERVE_VALUE(matrix);
	rotationArgument = &VEH_BOT_CRASH_NAV_ROT_ARG;
	sdata->botCrashNavRot.z = (s16)CTR_MipsSll(bot->botData.botNavFrame->rot[2], VEH_PHYS_CRASH_BOT_NAV_ROT_SHIFT);
	CTR_PSX_CLOBBER("$4");
	ConvertRotToMatrix(matrix, rotationArgument);
	forward = &VEH_PHYS_CRASH_FORWARD;

	forward->x = CTR_MipsSra(matrix->m[0][2], VEH_PHYS_CRASH_BOT_NAV_ROT_SHIFT);
	matrixForwardZ = matrix->m[2][2];
	forward->y = CTR_MipsSra(matrix->m[1][2], VEH_PHYS_CRASH_BOT_NAV_ROT_SHIFT);
	forward->z = CTR_MipsSra(matrixForwardZ, VEH_PHYS_CRASH_BOT_NAV_ROT_SHIFT);

	botSpeed =
	    CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(forward->x, vel->x), CTR_MipsMulLo(forward->y, vel->y)), CTR_MipsMulLo(forward->z, vel->z)),
	                VEH_PHYS_CRASH_BOT_SPEED_SHIFT);

	bot->botData.aiPhysics.speedLinear = botSpeed;
	bot->botData.aiPhysics.accel.x = CTR_MipsSubLo(vel->x, CTR_MipsSra(CTR_MipsMulLo(forward->x, botSpeed), VEH_PHYS_CRASH_BOT_SPEED_SHIFT));
	velocityZProduct = CTR_MipsMulLo(forward->z, botSpeed);
	velocityZ = vel->z;
	bot->botData.botFlags |= BOT_FLAG_FREE_PHYSICS;
	bot->botData.aiPhysics.accel.z = CTR_MipsSubLo(velocityZ, CTR_MipsSra(velocityZProduct, VEH_PHYS_CRASH_BOT_SPEED_SHIFT));
}

int VehPhysCrash_Attack(struct Driver *driver1, struct Driver *driver2, b32 canPlayFeedback, b32 boolPlayBubblePop)
{
	if ((driver1->actionsFlagSet & ACTION_MASK_WEAPON) == 0)
	{
		if ((driver2->actionsFlagSet & ACTION_MASK_WEAPON) != 0 && g_config.maskDamagesOthers)
		{
			driver1->pendingDamageType = VEH_PHYS_CRASH_DAMAGE_TYPE_MASK;
			driver1->pendingDamageReasonByte = VEH_PHYS_CRASH_DAMAGE_REASON_MASK;
			driver1->pendingDamageAttacker = driver2;

			if ((canPlayFeedback != 0) && (driver1->kartState != KS_BLASTED) && (driver1->invincibleTimer == 0))
			{
				OtherFX_DriverCrashing((driver1->actionsFlagSet & ACTION_ENGINE_ECHO) != 0, VEH_PHYS_CRASH_VOLUME_MAX);
				Voiceline_RequestPlayOwned(VEH_PHYS_CRASH_VOICELINE_CRASH, GAME_CHARACTER_IDS[driver1->driverID], VEH_PHYS_CRASH_VOICELINE_PRIORITY, driver1);
			}
		}

		if ((driver2->instBubbleHold != NULL) && (driver1->instBubbleHold == NULL))
		{
			struct Shield *bubble = driver2->instBubbleHold->thread->object;

			bubble->flags |= SHIELD_FLAG_CRASH_ATTACK;
			driver2->instBubbleHold = NULL;

			driver1->pendingDamageType = VEH_PHYS_CRASH_DAMAGE_TYPE_MASK;
			driver1->pendingDamageReasonByte = VEH_PHYS_CRASH_DAMAGE_REASON_SHIELD;
			driver1->pendingDamageAttacker = driver2;

			if ((canPlayFeedback != 0) && (driver1->kartState != KS_BLASTED) && (driver1->invincibleTimer == 0))
			{
				OtherFX_DriverCrashing((driver1->actionsFlagSet & ACTION_ENGINE_ECHO) != 0, VEH_PHYS_CRASH_VOLUME_MAX);

				if (boolPlayBubblePop != 0)
				{
					OtherFX_Play(VEH_PHYS_CRASH_BUBBLE_POP_FX, 1);
				}

				Voiceline_RequestPlayOwned(VEH_PHYS_CRASH_VOICELINE_CRASH, GAME_CHARACTER_IDS[driver1->driverID], VEH_PHYS_CRASH_VOICELINE_PRIORITY, driver1);
			}
		}

		if ((sdata->vehicleCollisionImpactStrength > VEH_PHYS_CRASH_TURBO_DAMAGE_IMPACT) && (driver2->reserves != 0) &&
		    ((driver2->actionsFlagSet & ACTION_TURBO_ITEM) != 0) && (driver1->reserves == 0))
		{
			driver2->forcedJumpType = FORCED_JUMP_HIGH;

			driver1->pendingDamageType = VEH_PHYS_CRASH_DAMAGE_TYPE_TURBO;
			driver1->pendingDamageReasonByte = VEH_PHYS_CRASH_DAMAGE_REASON_TURBO;
			driver1->pendingDamageAttacker = driver2;
		}
	}

	return canPlayFeedback;
}

static inline int VehPhysCrash_CurrentFrame(void)
{
	struct GameTracker *gameTracker;

	VEH_LOAD_GAME_TRACKER(gameTracker);
	return gameTracker->frameTimer_MainFrame_ResetDB;
}

static inline int VehPhysCrash_LastFeedbackFrame(void)
{
	register u32 page CTR_PSX_REGISTER("$2");
	register int frame CTR_PSX_REGISTER("$2");

	CTR_PSX_LOAD_SYMBOL_PAGE(page, VEH_LAST_FEEDBACK_FRAME_ASM_NAME);
	CTR_PSX_LOAD_WORD_FROM_PAGE(frame, page, VEH_LAST_FEEDBACK_FRAME_ASM_NAME, sdata->audioDefaults[8]);
	CTR_PSX_BIND_VALUE_CLOBBER(frame, "$18");
	return frame;
}

static inline void VehPhysCrash_RecordFeedbackFrame(void)
{
	sdata->audioDefaults[8] = VehPhysCrash_CurrentFrame();
}

void VehPhysCrash_AnyTwoCars(struct Thread *thread, struct DriverCollisionSearch *searchArg, Vec3 *selfVelArg)
{
	// NOTE(aalhendi): The collision paths intentionally keep their weighted
	// velocity scratch in separate scopes so GCC 2.8 reuses retail's registers.
	register struct DriverCollisionSearch *search CTR_PSX_REGISTER("$21") = searchArg;
	register Vec3 *selfVel CTR_PSX_REGISTER("$19") = selfVelArg;
	int distance;
	register struct Driver *otherDriver CTR_PSX_REGISTER("$16") = NULL;
	register struct Driver *selfDriver CTR_PSX_REGISTER("$22");
	register int hitStrength CTR_PSX_REGISTER("$18") = 0;
	register SVec3 *hitDir CTR_PSX_REGISTER("$20") = NULL;
	u32 canPlayFeedback;
	u32 selfActions;
	u32 botMask;
	int attackResult;
	Vec3 otherVel;
	Vec3 weightedVel;
	int hitDirX;
	int hitDirY;
	register int hitDirZ CTR_PSX_REGISTER("$2");

	CTR_PSX_KEEP_VALUE(search);
	CTR_PSX_KEEP_VALUE(selfVel);
	if (Chaos_IsTimeStopped())
	{
		return;
	}
	distance = MATH_FastSqrt(search->bucket.bestDistSq, 0);
	hitDirZ = VEH_PHYS_CRASH_UNIT_VECTOR_SCALE;

	if (distance != 0)
	{
		hitDirX = CTR_MipsDiv(CTR_MipsSll(search->bucket.dist.x, VEH_PHYS_CRASH_MATRIX_FRAC_SHIFT), distance);
		hitDirZ = CTR_MipsDiv(CTR_MipsSll(search->bucket.dist.z, VEH_PHYS_CRASH_MATRIX_FRAC_SHIFT), distance);
		hitDirY = CTR_MipsDiv(CTR_MipsSll(search->bucket.dist.y, VEH_PHYS_CRASH_MATRIX_FRAC_SHIFT), distance);
		search->hitDir.x = (s16)hitDirX;
		search->hitDir.y = (s16)hitDirY;
	}
	else
	{
		search->hitDir.x = 0;
		search->hitDir.y = 0;
	}
	search->hitDir.z = hitDirZ;
	CTR_PSX_MEMORY_BARRIER();

	otherDriver = search->bucket.th->object;
	selfDriver = thread->object;

	hitStrength = VehPhysForce_ScaleValue(selfDriver, thread->driverHitRadius) +
	              VehPhysForce_ScaleValue(otherDriver, otherDriver->instSelf->thread->driverHitRadius) - distance;
	if (hitStrength <= 0)
	{
		return;
	}
	if (VehPhysCrash_TrySquashSmallerKart(selfDriver, otherDriver))
	{
		return;
	}

	botMask = ACTION_BOT;
	selfActions = selfDriver->actionsFlagSet;
	sdata->vehicleCollisionImpactStrength = 0;
	CTR_PSX_MEMORY_BARRIER();

	if ((selfActions & botMask) != 0)
	{
		hitDir = &search->hitDir;

		if ((otherDriver->actionsFlagSet & ACTION_BOT) == 0)
		{
			VehPhysForce_ConvertSpeedToVec(otherDriver, &otherVel);
			{
				register int lhsWeight CTR_PSX_REGISTER("$3");

				lhsWeight = selfDriver->const_CollisionWeight;
				weightedVel.x = CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->x, lhsWeight), CTR_MipsMulLo(otherVel.x, otherDriver->const_CollisionWeight)),
				                            CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));
				lhsWeight = selfDriver->const_CollisionWeight;
				weightedVel.y = CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->y, lhsWeight), CTR_MipsMulLo(otherVel.y, otherDriver->const_CollisionWeight)),
				                            CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));
				lhsWeight = selfDriver->const_CollisionWeight;
				weightedVel.z = CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->z, lhsWeight), CTR_MipsMulLo(otherVel.z, otherDriver->const_CollisionWeight)),
				                            CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));
				hitDir = &search->hitDir;

				if (VehPhysCrash_BounceSelf(hitDir, &weightedVel, &otherVel, 1) < 0)
				{
					sdata->vehicleCollisionImpactStrength = 0;
				}
				if (VehPhysCrash_BounceSelf(hitDir, &weightedVel, selfVel, 0) > 0)
				{
					sdata->vehicleCollisionImpactStrength = 0;
				}
			}

			selfVel->x = CTR_MipsAddLo(selfVel->x, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.x, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			selfVel->y = CTR_MipsAddLo(selfVel->y, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.y, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			selfVel->z = CTR_MipsAddLo(selfVel->z, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.z, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			otherVel.x = CTR_MipsSubLo(otherVel.x, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.x, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			otherVel.y = CTR_MipsSubLo(otherVel.y, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.y, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			otherVel.z = CTR_MipsSubLo(otherVel.z, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.z, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			VehPhysCrash_AI(selfDriver, selfVel);
			VEH_CONVERT_VEC_TO_SPEED(otherDriver, &otherVel);
		}
		else
		{
			otherVel.x = CTR_MipsAddLo(otherDriver->xSpeed, otherDriver->botData.aiPhysics.accel.x);
			otherVel.y = CTR_MipsAddLo(otherDriver->ySpeed, otherDriver->botData.aiPhysics.accel.y);
			otherVel.z = CTR_MipsAddLo(otherDriver->zSpeed, otherDriver->botData.aiPhysics.accel.z);

			{
				register int lhsWeight CTR_PSX_REGISTER("$3");

				lhsWeight = selfDriver->const_CollisionWeight;
				weightedVel.x = CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->x, lhsWeight), CTR_MipsMulLo(otherVel.x, otherDriver->const_CollisionWeight)),
				                            CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));
				lhsWeight = selfDriver->const_CollisionWeight;
				weightedVel.y = CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->y, lhsWeight), CTR_MipsMulLo(otherVel.y, otherDriver->const_CollisionWeight)),
				                            CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));
				lhsWeight = selfDriver->const_CollisionWeight;
				weightedVel.z = CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->z, lhsWeight), CTR_MipsMulLo(otherVel.z, otherDriver->const_CollisionWeight)),
				                            CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));

				if (VehPhysCrash_BounceSelf(hitDir, &weightedVel, &otherVel, 1) < 0)
				{
					sdata->vehicleCollisionImpactStrength = 0;
				}
				if (VehPhysCrash_BounceSelf(hitDir, &weightedVel, selfVel, 0) > 0)
				{
					sdata->vehicleCollisionImpactStrength = 0;
				}
			}

			selfVel->x = CTR_MipsAddLo(selfVel->x, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.x, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			selfVel->y = CTR_MipsAddLo(selfVel->y, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.y, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			selfVel->z = CTR_MipsAddLo(selfVel->z, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.z, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			otherVel.x = CTR_MipsSubLo(otherVel.x, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.x, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			otherVel.y = CTR_MipsSubLo(otherVel.y, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.y, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			otherVel.z = CTR_MipsSubLo(otherVel.z, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.z, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
			VehPhysCrash_AI(otherDriver, &otherVel);
			VehPhysCrash_AI(selfDriver, selfVel);
			BOTS_CollideWithOtherAI(selfDriver, otherDriver);
		}

		return;
	}

	hitDir = &search->hitDir;

	if ((otherDriver->actionsFlagSet & ACTION_BOT) != 0)
	{
		otherVel.x = CTR_MipsAddLo(otherDriver->xSpeed, otherDriver->botData.aiPhysics.accel.x);
		otherVel.y = CTR_MipsAddLo(otherDriver->ySpeed, otherDriver->botData.aiPhysics.accel.y);
		otherVel.z = CTR_MipsAddLo(otherDriver->zSpeed, otherDriver->botData.aiPhysics.accel.z);

		{
			register int lhsWeight CTR_PSX_REGISTER("$3");

			lhsWeight = selfDriver->const_CollisionWeight;
			weightedVel.x = CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->x, lhsWeight), CTR_MipsMulLo(otherVel.x, otherDriver->const_CollisionWeight)),
			                            CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));
			lhsWeight = selfDriver->const_CollisionWeight;
			weightedVel.y = CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->y, lhsWeight), CTR_MipsMulLo(otherVel.y, otherDriver->const_CollisionWeight)),
			                            CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));
			lhsWeight = selfDriver->const_CollisionWeight;
			weightedVel.z = CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->z, lhsWeight), CTR_MipsMulLo(otherVel.z, otherDriver->const_CollisionWeight)),
			                            CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));

			if (VehPhysCrash_BounceSelf(hitDir, &weightedVel, &otherVel, 1) < 0)
			{
				sdata->vehicleCollisionImpactStrength = 0;
			}
			if (VehPhysCrash_BounceSelf(hitDir, &weightedVel, selfVel, 0) > 0)
			{
				sdata->vehicleCollisionImpactStrength = 0;
			}
		}

		selfVel->x = CTR_MipsAddLo(selfVel->x, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.x, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		selfVel->y = CTR_MipsAddLo(selfVel->y, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.y, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		selfVel->z = CTR_MipsAddLo(selfVel->z, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.z, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		otherVel.x = CTR_MipsSubLo(otherVel.x, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.x, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		otherVel.y = CTR_MipsSubLo(otherVel.y, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.y, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		otherVel.z = CTR_MipsSubLo(otherVel.z, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.z, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		VehPhysCrash_AI(otherDriver, &otherVel);
	}
	else
	{
		{
			register int lhsWeight CTR_PSX_REGISTER("$3");

			lhsWeight = selfDriver->const_CollisionWeight;
			weightedVel.x =
			    CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->x, lhsWeight), CTR_MipsMulLo(otherDriver->velocity.x, otherDriver->const_CollisionWeight)),
			                CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));
			lhsWeight = selfDriver->const_CollisionWeight;
			weightedVel.y =
			    CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->y, lhsWeight), CTR_MipsMulLo(otherDriver->velocity.y, otherDriver->const_CollisionWeight)),
			                CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));
			lhsWeight = selfDriver->const_CollisionWeight;
			weightedVel.z =
			    CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(selfVel->z, lhsWeight), CTR_MipsMulLo(otherDriver->velocity.z, otherDriver->const_CollisionWeight)),
			                CTR_MipsAddLo(lhsWeight, otherDriver->const_CollisionWeight));

			if (VehPhysCrash_BounceSelf(hitDir, &weightedVel, &otherDriver->velocity, 1) < 0)
			{
				sdata->vehicleCollisionImpactStrength = 0;
			}
			if (VehPhysCrash_BounceSelf(hitDir, &weightedVel, selfVel, 0) > 0)
			{
				sdata->vehicleCollisionImpactStrength = 0;
			}
		}

		selfVel->x = CTR_MipsAddLo(selfVel->x, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.x, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		selfVel->y = CTR_MipsAddLo(selfVel->y, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.y, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		selfVel->z = CTR_MipsAddLo(selfVel->z, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.z, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		otherDriver->velocity.x =
		    CTR_MipsSubLo(otherDriver->velocity.x, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.x, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		otherDriver->velocity.y =
		    CTR_MipsSubLo(otherDriver->velocity.y, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.y, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
		otherDriver->velocity.z =
		    CTR_MipsSubLo(otherDriver->velocity.z, CTR_MipsSra(CTR_MipsMulLo(search->hitDir.z, hitStrength), VEH_PHYS_CRASH_VECTOR_SPEED_SHIFT));
	}

	{
		register int impactStrength CTR_PSX_REGISTER("$4");
		register int currentFrame CTR_PSX_REGISTER("$3");
		register int lastFeedbackFrame CTR_PSX_REGISTER("$2");
		register int frameDelta CTR_PSX_REGISTER("$18");
		register u32 isCoolingDown CTR_PSX_REGISTER("$2");

		currentFrame = VehPhysCrash_CurrentFrame();
		lastFeedbackFrame = VehPhysCrash_LastFeedbackFrame();
		impactStrength = sdata->vehicleCollisionImpactStrength;
		CTR_PSX_KEEP_VALUE(impactStrength);
		frameDelta = CTR_MipsSubLo(currentFrame, lastFeedbackFrame);
		isCoolingDown = ((u32)frameDelta < VEH_PHYS_CRASH_FEEDBACK_COOLDOWN_FRAMES);
		canPlayFeedback = isCoolingDown ^ 1;

		if (impactStrength > VEH_PHYS_CRASH_FEEDBACK_MIN_IMPACT)
		{
			int volume;
			if ((((thread->modelIndex == DYNAMIC_PLAYER) || (search->bucket.th->modelIndex == DYNAMIC_PLAYER)) &&
			     (volume = VehCalc_MapToRange(impactStrength, 0, VEH_PHYS_CRASH_VOLUME_IMPACT_MAX, VEH_PHYS_CRASH_VOLUME_MIN, VEH_PHYS_CRASH_VOLUME_MAX),
			      canPlayFeedback != 0)) &&
			    (selfDriver->kartState != KS_BLASTED) && (selfDriver->invincibleTimer == 0) && (otherDriver->kartState != KS_BLASTED) &&
			    (otherDriver->invincibleTimer == 0))
			{
				OtherFX_DriverCrashing((selfDriver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0, volume);
				VehPhysCrash_RecordFeedbackFrame();

				if ((u32)volume > VEH_PHYS_CRASH_HARD_CRASH_VOLUME)
				{
					Voiceline_RequestPlayOwned(VEH_PHYS_CRASH_VOICELINE_HARD_CRASH, GAME_CHARACTER_IDS[selfDriver->driverID], VEH_PHYS_CRASH_VOICELINE_PRIORITY, selfDriver);
				}
			}

			GAMEPAD_ShockFreq(otherDriver, VEH_PHYS_CRASH_RUMBLE_CHANNEL, 0);
			GAMEPAD_ShockForce1(otherDriver, VEH_PHYS_CRASH_RUMBLE_CHANNEL, VEH_PHYS_CRASH_RUMBLE_FORCE);
			// NOTE(aalhendi): Keeping the calls in both branches lets GCC 2.8
			// perform retail's common-tail merge without changing native behavior.
			if (otherDriver->simpTurnState > 0)
			{
				GAMEPAD_JogCon1(otherDriver, VEH_PHYS_CRASH_JOG_TURNING, VEH_PHYS_CRASH_JOG_DURATION);
			}
			else
			{
				GAMEPAD_JogCon1(otherDriver, VEH_PHYS_CRASH_JOG_STRAIGHT, VEH_PHYS_CRASH_JOG_DURATION);
			}

			GAMEPAD_ShockFreq(selfDriver, VEH_PHYS_CRASH_RUMBLE_CHANNEL, 0);
			GAMEPAD_ShockForce1(selfDriver, VEH_PHYS_CRASH_RUMBLE_CHANNEL, VEH_PHYS_CRASH_RUMBLE_FORCE);
			if (selfDriver->simpTurnState > 0)
			{
				GAMEPAD_JogCon1(selfDriver, VEH_PHYS_CRASH_JOG_TURNING, VEH_PHYS_CRASH_JOG_DURATION);
			}
			else
			{
				GAMEPAD_JogCon1(selfDriver, VEH_PHYS_CRASH_JOG_STRAIGHT, VEH_PHYS_CRASH_JOG_DURATION);
			}

			selfDriver->actionsFlagSet |= ACTION_HUMAN_HUMAN_COLLISION;
			otherDriver->actionsFlagSet |= ACTION_HUMAN_HUMAN_COLLISION;
		}

		attackResult = VehPhysCrash_Attack(selfDriver, otherDriver, canPlayFeedback, 0);
		VehPhysCrash_Attack(otherDriver, selfDriver, attackResult, 1);
	}
}
