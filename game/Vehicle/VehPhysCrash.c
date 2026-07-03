#include <common.h>

static u32 VehPhysCrash_LengthSq2(s32 x, s32 z)
{
	return (u32)CTR_MipsAddLo(CTR_MipsMulLo(x, x), CTR_MipsMulLo(z, z));
}

static u32 VehPhysCrash_LengthSq3(s32 x, s32 y, s32 z)
{
	return (u32)CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(x, x), CTR_MipsMulLo(y, y)), CTR_MipsMulLo(z, z));
}

static s32 VehPhysCrash_Dot3(s32 ax, s32 ay, s32 az, s32 bx, s32 by, s32 bz)
{
	return CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(ax, bx), CTR_MipsMulLo(ay, by)), CTR_MipsMulLo(az, bz));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005cd1c-0x8005cf64.
void VehPhysCrash_ConvertVecToSpeed(struct Driver *d, Vec3 *vel)
{
	int speed2D = VehCalc_FastSqrt(VehPhysCrash_LengthSq2(vel->x, vel->z), 0x10);
	s16 speed3D = (s16)(VehCalc_FastSqrt(VehPhysCrash_LengthSq3(vel->x, vel->y, vel->z), 0x10) >> 8);

	d->speed = speed3D;
	d->axisRotationY = (s16)ratan2(CTR_MipsSll(vel->y, 8), speed2D);
	d->axisRotationX = (s16)ratan2(vel->x, vel->z);

	int projOnMovingDirAxis =
	    CTR_MipsSra(VehPhysCrash_Dot3(vel->x, vel->y, vel->z, d->matrixMovingDir.m[0][1], d->matrixMovingDir.m[1][1], d->matrixMovingDir.m[2][1]), 0xc);

	int projX = CTR_MipsSra(CTR_MipsMulLo(d->matrixMovingDir.m[0][1], projOnMovingDirAxis), 0xc);
	int projY = CTR_MipsSra(CTR_MipsMulLo(d->matrixMovingDir.m[1][1], projOnMovingDirAxis), 0xc);
	int projZ = CTR_MipsSra(CTR_MipsMulLo(d->matrixMovingDir.m[2][1], projOnMovingDirAxis), 0xc);

	speed3D = (s16)(VehCalc_FastSqrt(VehPhysCrash_LengthSq3(projX, projY, projZ), 0x10) >> 8);

	d->jumpHeightCurr = speed3D;
	if (projOnMovingDirAxis < 0)
	{
		d->jumpHeightCurr = (s16)CTR_MipsNegLo(speed3D);
	}

	projX = CTR_MipsSubLo(vel->x, projX);
	projY = CTR_MipsSubLo(vel->y, projY);
	projZ = CTR_MipsSubLo(vel->z, projZ);

	speed3D = (s16)(VehCalc_FastSqrt(VehPhysCrash_LengthSq3(projX, projY, projZ), 0x10) >> 8);

	d->speedApprox = speed3D;

	if (VehPhysCrash_Dot3(projX, projY, projZ, d->matrixMovingDir.m[0][2], d->matrixMovingDir.m[1][2], d->matrixMovingDir.m[2][2]) < 0)
	{
		d->speedApprox = (s16)CTR_MipsNegLo(speed3D);
	}
}

static int VehPhysCrash_BounceSelf_Div6Shift9(int value)
{
	s64 product = (s64)value * 0x2aaaaaab;
	int high = (s32)((u64)product >> 32);

	return CTR_MipsSubLo(CTR_MipsSra(high, 9), CTR_MipsSra(value, 31));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005cf64-0x8005d0d0.
int VehPhysCrash_BounceSelf(const SVec3 *normal, const Vec3 *origin, Vec3 *vel, b32 boolOtherDriver)
{
	int diffX = CTR_MipsSubLo(vel->x, origin->x);
	int diffY = CTR_MipsSubLo(vel->y, origin->y);
	int diffZ = CTR_MipsSubLo(vel->z, origin->z);
	int dot = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(diffX, normal->x), CTR_MipsMulLo(diffY, normal->y)), CTR_MipsMulLo(diffZ, normal->z)), 0xc);

	if (boolOtherDriver == 0)
	{
		if (dot >= 0)
		{
			return 0;
		}
	}
	else if (dot <= 0)
	{
		return 0;
	}

	int absDot = dot;
	if (absDot < 0)
	{
		absDot = CTR_MipsNegLo(absDot);
	}

	if (sdata->vehicleCollisionImpactStrength < absDot)
	{
		sdata->vehicleCollisionImpactStrength = absDot;
	}

	diffX = CTR_MipsSubLo(diffX, VehPhysCrash_BounceSelf_Div6Shift9(CTR_MipsMulLo(dot, normal->x)));
	diffY = CTR_MipsSubLo(diffY, VehPhysCrash_BounceSelf_Div6Shift9(CTR_MipsMulLo(dot, normal->y)));
	diffZ = CTR_MipsSubLo(diffZ, VehPhysCrash_BounceSelf_Div6Shift9(CTR_MipsMulLo(dot, normal->z)));

	vel->x = CTR_MipsAddLo(diffX, origin->x);

	int oldY = vel->y;
	int newY = CTR_MipsAddLo(diffY, origin->y);
	if ((oldY < newY) && (newY > 0x3200))
	{
		newY = 0x3200;
	}
	vel->y = newY;
	vel->z = CTR_MipsAddLo(diffZ, origin->z);

	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005d0d0-0x8005d218.
void VehPhysCrash_AI(struct Driver *bot, Vec3 *vel)
{
	sdata->botCrashNavRot.x = (s16)CTR_MipsSll(bot->botData.botNavFrame->rot[0], 4);
	sdata->botCrashNavRot.y = (s16)CTR_MipsSll(bot->botData.botNavFrame->rot[1], 4);
	sdata->botCrashNavRot.z = (s16)CTR_MipsSll(bot->botData.botNavFrame->rot[2], 4);

	struct VehPhysCrashAiScratch *scratch = (struct VehPhysCrashAiScratch *)(void *)&sdata->dataLibFiller[0];
	MATRIX *matrix = &scratch->matrix;

	ConvertRotToMatrix(matrix, &sdata->botCrashNavRot);

	scratch->forward.x = CTR_MipsSra(matrix->m[0][2], 4);
	scratch->forward.y = CTR_MipsSra(matrix->m[1][2], 4);
	scratch->forward.z = CTR_MipsSra(matrix->m[2][2], 4);

	int botSpeed = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(scratch->forward.x, vel->x), CTR_MipsMulLo(scratch->forward.y, vel->y)),
	                                         CTR_MipsMulLo(scratch->forward.z, vel->z)),
	                           8);

	bot->botData.aiPhysics.speedLinear = botSpeed;
	bot->botData.aiPhysics.accel.x = CTR_MipsSubLo(vel->x, CTR_MipsSra(CTR_MipsMulLo(scratch->forward.x, botSpeed), 8));
	bot->botData.botFlags |= BOT_FLAG_FREE_PHYSICS;
	bot->botData.aiPhysics.accel.z = CTR_MipsSubLo(vel->z, CTR_MipsSra(CTR_MipsMulLo(scratch->forward.z, botSpeed), 8));
}

static void VehPhysCrash_Attack_SetReason(struct Driver *driver, u8 reason)
{
	driver->pendingDamageReasonByte = reason;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005d218-0x8005d404.
int VehPhysCrash_Attack(struct Driver *driver1, struct Driver *driver2, b32 canPlayFeedback, b32 boolPlayBubblePop)
{
	if ((driver1->actionsFlagSet & ACTION_MASK_WEAPON) == 0)
	{
		if ((driver2->actionsFlagSet & ACTION_MASK_WEAPON) != 0)
		{
			driver1->pendingDamageType = 2;
			VehPhysCrash_Attack_SetReason(driver1, 6);
			driver1->pendingDamageAttacker = driver2;

			if ((canPlayFeedback != 0) && (driver1->kartState != KS_BLASTED) && (driver1->invincibleTimer == 0))
			{
				OtherFX_DriverCrashing((driver1->actionsFlagSet & ACTION_ENGINE_ECHO) != 0, 0xff);
				Voiceline_RequestPlay(1, data.characterIDs[driver1->driverID], 0x10);
			}
		}

		if ((driver2->instBubbleHold != NULL) && (driver1->instBubbleHold == NULL))
		{
			struct Shield *bubble = driver2->instBubbleHold->thread->object;

			bubble->flags |= SHIELD_FLAG_CRASH_ATTACK;
			driver2->instBubbleHold = NULL;

			driver1->pendingDamageType = 2;
			VehPhysCrash_Attack_SetReason(driver1, 0);
			driver1->pendingDamageAttacker = driver2;

			if ((canPlayFeedback != 0) && (driver1->kartState != KS_BLASTED) && (driver1->invincibleTimer == 0))
			{
				OtherFX_DriverCrashing((driver1->actionsFlagSet & ACTION_ENGINE_ECHO) != 0, 0xff);

				if (boolPlayBubblePop != 0)
				{
					OtherFX_Play(0x4f, 1);
				}

				Voiceline_RequestPlay(1, data.characterIDs[driver1->driverID], 0x10);
			}
		}

		if ((sdata->vehicleCollisionImpactStrength > 0xa00) && (driver2->reserves != 0) && ((driver2->actionsFlagSet & ACTION_TURBO_ITEM) != 0) &&
		    (driver1->reserves == 0))
		{
			driver2->forcedJumpType = FORCED_JUMP_HIGH;

			driver1->pendingDamageType = 3;
			VehPhysCrash_Attack_SetReason(driver1, 5);
			driver1->pendingDamageAttacker = driver2;
		}
	}

	return canPlayFeedback;
}

// NOTE(aalhendi): These static helpers factor repeated retail blocks; they
// are not separate retail symbols.
static s32 VehPhysCrash_WeightedAverage(s32 lhs, s16 lhsWeight, s32 rhs, s16 rhsWeight)
{
	return CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(lhs, lhsWeight), CTR_MipsMulLo(rhs, rhsWeight)), CTR_MipsAddLo(lhsWeight, rhsWeight));
}

static void VehPhysCrash_WeightedVelocity(Vec3 *out, Vec3 *lhs, struct Driver *lhsDriver, Vec3 *rhs, struct Driver *rhsDriver)
{
	out->x = VehPhysCrash_WeightedAverage(lhs->x, lhsDriver->const_CollisionWeight, rhs->x, rhsDriver->const_CollisionWeight);
	out->y = VehPhysCrash_WeightedAverage(lhs->y, lhsDriver->const_CollisionWeight, rhs->y, rhsDriver->const_CollisionWeight);
	out->z = VehPhysCrash_WeightedAverage(lhs->z, lhsDriver->const_CollisionWeight, rhs->z, rhsDriver->const_CollisionWeight);
}

static void VehPhysCrash_AddImpulse(Vec3 *vel, const SVec3 *hitDir, s32 strength)
{
	vel->x = CTR_MipsAddLo(vel->x, CTR_MipsSra(CTR_MipsMulLo(hitDir->x, strength), 8));
	vel->y = CTR_MipsAddLo(vel->y, CTR_MipsSra(CTR_MipsMulLo(hitDir->y, strength), 8));
	vel->z = CTR_MipsAddLo(vel->z, CTR_MipsSra(CTR_MipsMulLo(hitDir->z, strength), 8));
}

static void VehPhysCrash_SubImpulse(Vec3 *vel, const SVec3 *hitDir, s32 strength)
{
	vel->x = CTR_MipsSubLo(vel->x, CTR_MipsSra(CTR_MipsMulLo(hitDir->x, strength), 8));
	vel->y = CTR_MipsSubLo(vel->y, CTR_MipsSra(CTR_MipsMulLo(hitDir->y, strength), 8));
	vel->z = CTR_MipsSubLo(vel->z, CTR_MipsSra(CTR_MipsMulLo(hitDir->z, strength), 8));
}

static void VehPhysCrash_BouncePair(const SVec3 *hitDir, const Vec3 *weightedVel, Vec3 *otherVel, Vec3 *selfVel)
{
	if (VehPhysCrash_BounceSelf(hitDir, weightedVel, otherVel, 1) < 0)
	{
		sdata->vehicleCollisionImpactStrength = 0;
	}

	if (VehPhysCrash_BounceSelf(hitDir, weightedVel, selfVel, 0) > 0)
	{
		sdata->vehicleCollisionImpactStrength = 0;
	}
}

static void VehPhysCrash_PlayHumanFeedback(struct Thread *selfThread, struct Thread *otherThread, struct Driver *selfDriver, struct Driver *otherDriver,
                                           u32 canPlayFeedback)
{
	if (sdata->vehicleCollisionImpactStrength <= 0x200)
	{
		return;
	}

	if ((selfThread->modelIndex == DYNAMIC_PLAYER) || (otherThread->modelIndex == DYNAMIC_PLAYER))
	{
		int volume = VehCalc_MapToRange(sdata->vehicleCollisionImpactStrength, 0, 0x1900, 0x3f, 0xff);

		if ((canPlayFeedback != 0) && (selfDriver->kartState != KS_BLASTED) && (selfDriver->invincibleTimer == 0) && (otherDriver->kartState != KS_BLASTED) &&
		    (otherDriver->invincibleTimer == 0))
		{
			OtherFX_DriverCrashing((selfDriver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0, volume);

			// NOTE(aalhendi): Retail uses DAT_8008d838. This field currently
			// names the same USA address as the last audioDefaults slot.
			sdata->audioDefaults[8] = sdata->gGT->frameTimer_MainFrame_ResetDB;

			if ((u32)volume > 0xdc)
			{
				Voiceline_RequestPlay(5, data.characterIDs[selfDriver->driverID], 0x10);
			}
		}
	}

	GAMEPAD_ShockFreq(otherDriver, 8, 0);
	GAMEPAD_ShockForce1(otherDriver, 8, 0x7f);
	GAMEPAD_JogCon1(otherDriver, (otherDriver->simpTurnState > 0) ? 0x29 : 0x19, 0x60);

	GAMEPAD_ShockFreq(selfDriver, 8, 0);
	GAMEPAD_ShockForce1(selfDriver, 8, 0x7f);
	GAMEPAD_JogCon1(selfDriver, (selfDriver->simpTurnState > 0) ? 0x29 : 0x19, 0x60);

	selfDriver->actionsFlagSet |= ACTION_HUMAN_HUMAN_COLLISION;
	otherDriver->actionsFlagSet |= ACTION_HUMAN_HUMAN_COLLISION;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005d404-0x8005e104
void VehPhysCrash_AnyTwoCars(struct Thread *thread, struct DriverCollisionSearch *search, Vec3 *selfVel)
{
	int distance = VehCalc_FastSqrt(search->bucket.bestDistSq, 0);
	const SVec3 *dist = &search->bucket.dist;
	SVec3 *hitDir = &search->hitDir;

	if (distance == 0)
	{
		CTR_SET_VEC3(hitDir->v, 0, 0, 0x1000);
	}
	else
	{
		CTR_SET_VEC3(hitDir->v, (s16)CTR_MipsDiv(CTR_MipsSll(dist->x, 0xc), distance), (s16)CTR_MipsDiv(CTR_MipsSll(dist->y, 0xc), distance),
		             (s16)CTR_MipsDiv(CTR_MipsSll(dist->z, 0xc), distance));
	}

	struct Thread *otherThread = search->bucket.th;
	struct Driver *otherDriver = otherThread->object;
	struct Driver *selfDriver = thread->object;

	int hitStrength = CTR_MipsSubLo(CTR_MipsAddLo(thread->driverHitRadius, otherThread->driverHitRadius), distance);
	if (hitStrength <= 0)
	{
		return;
	}

	sdata->vehicleCollisionImpactStrength = 0;

	if ((selfDriver->actionsFlagSet & ACTION_BOT) != 0)
	{
		Vec3 otherVel;
		Vec3 weightedVel;

		if ((otherDriver->actionsFlagSet & ACTION_BOT) == 0)
		{
			VehPhysForce_ConvertSpeedToVecOut(otherDriver, &otherVel);
			VehPhysCrash_WeightedVelocity(&weightedVel, selfVel, selfDriver, &otherVel, otherDriver);
			VehPhysCrash_BouncePair(hitDir, &weightedVel, &otherVel, selfVel);
			VehPhysCrash_AddImpulse(selfVel, hitDir, hitStrength);
			VehPhysCrash_SubImpulse(&otherVel, hitDir, hitStrength);
			VehPhysCrash_AI(selfDriver, selfVel);
			VehPhysCrash_ConvertVecToSpeed(otherDriver, &otherVel);
		}
		else
		{
			otherVel.x = CTR_MipsAddLo(otherDriver->xSpeed, otherDriver->botData.aiPhysics.accel.x);
			otherVel.y = CTR_MipsAddLo(otherDriver->ySpeed, otherDriver->botData.aiPhysics.accel.y);
			otherVel.z = CTR_MipsAddLo(otherDriver->zSpeed, otherDriver->botData.aiPhysics.accel.z);

			VehPhysCrash_WeightedVelocity(&weightedVel, selfVel, selfDriver, &otherVel, otherDriver);
			VehPhysCrash_BouncePair(hitDir, &weightedVel, &otherVel, selfVel);
			VehPhysCrash_AddImpulse(selfVel, hitDir, hitStrength);
			VehPhysCrash_SubImpulse(&otherVel, hitDir, hitStrength);
			VehPhysCrash_AI(otherDriver, &otherVel);
			VehPhysCrash_AI(selfDriver, selfVel);
			BOTS_CollideWithOtherAI(selfDriver, otherDriver);
		}

		return;
	}

	if ((otherDriver->actionsFlagSet & ACTION_BOT) != 0)
	{
		Vec3 otherVel;
		Vec3 weightedVel;

		otherVel.x = CTR_MipsAddLo(otherDriver->xSpeed, otherDriver->botData.aiPhysics.accel.x);
		otherVel.y = CTR_MipsAddLo(otherDriver->ySpeed, otherDriver->botData.aiPhysics.accel.y);
		otherVel.z = CTR_MipsAddLo(otherDriver->zSpeed, otherDriver->botData.aiPhysics.accel.z);

		VehPhysCrash_WeightedVelocity(&weightedVel, selfVel, selfDriver, &otherVel, otherDriver);
		VehPhysCrash_BouncePair(hitDir, &weightedVel, &otherVel, selfVel);
		VehPhysCrash_AddImpulse(selfVel, hitDir, hitStrength);
		VehPhysCrash_SubImpulse(&otherVel, hitDir, hitStrength);
		VehPhysCrash_AI(otherDriver, &otherVel);
	}
	else
	{
		Vec3 weightedVel;
		Vec3 *otherVel = &otherDriver->velocity;

		VehPhysCrash_WeightedVelocity(&weightedVel, selfVel, selfDriver, otherVel, otherDriver);
		VehPhysCrash_BouncePair(hitDir, &weightedVel, otherVel, selfVel);
		VehPhysCrash_AddImpulse(selfVel, hitDir, hitStrength);
		VehPhysCrash_SubImpulse(otherVel, hitDir, hitStrength);
	}

	u32 canPlayFeedback = ((u32)CTR_MipsSubLo(sdata->gGT->frameTimer_MainFrame_ResetDB, sdata->audioDefaults[8]) >= 3);

	VehPhysCrash_PlayHumanFeedback(thread, otherThread, selfDriver, otherDriver, canPlayFeedback);

	int attackResult = VehPhysCrash_Attack(selfDriver, otherDriver, canPlayFeedback, 0);
	VehPhysCrash_Attack(otherDriver, selfDriver, attackResult, 1);
}
