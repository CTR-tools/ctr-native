#include "VehCommon.h"

void VehPhysForce_ConvertSpeedToVec(struct Driver *driver, Vec3 *velocity)
{
	u32 angle;
	s32 sine;
	s32 cosine;
	s32 yComponent;

	angle = (s16)driver->axisRotationY;
	{
		register u32 yQuadrant CTR_PSX_REGISTER("$2");
		register const VecElement *yTrigPtr CTR_PSX_REGISTER("$3");

		yTrigPtr = (const VecElement *)&data.trigApprox[ANG_MODULO_HALF_PI(angle)];
		sine = *yTrigPtr;
		yQuadrant = angle & ANG_QUADRANT_BIT;
		if (yQuadrant != 0)
		{
			cosine = (s16)sine;
			sine = CTR_MipsSra(sine, 16);
			if ((angle & ANG_SIGN_BIT) != 0)
			{
				sine = CTR_MipsNegLo(sine);
			}
			else
			{
				cosine = CTR_MipsNegLo(cosine);
			}
		}
		else
		{
			cosine = CTR_MipsSra(sine, 16);
			sine = CTR_MipsSll(sine, 16);
			sine = CTR_MipsSra(sine, 16);
			if ((angle & ANG_SIGN_BIT) != 0)
			{
				cosine = CTR_MipsNegLo(cosine);
				sine = CTR_MipsNegLo(sine);
			}
		}
	}

	velocity->y = CTR_MipsSra(CTR_MipsMulLo(driver->speed, sine), FRACTIONAL_BITS);
	yComponent = CTR_MipsSra(CTR_MipsMulLo(driver->speed, cosine), FRACTIONAL_BITS);

	{
		u32 xQuadrant;
		register u32 xTrigIndex CTR_PSX_REGISTER("$2");
		const VecElement *xTrigPtr;

		xTrigPtr = (const VecElement *)&data.trigApprox[0];
		angle = (s16)driver->axisRotationX;
		xTrigIndex = ANG_MODULO_HALF_PI(angle);
		sine = xTrigPtr[xTrigIndex];
		xQuadrant = angle & ANG_QUADRANT_BIT;
		if (xQuadrant != 0)
		{
			cosine = (s16)sine;
			sine = CTR_MipsSra(sine, 16);
			if ((angle & ANG_SIGN_BIT) != 0)
			{
				sine = CTR_MipsNegLo(sine);
			}
			else
			{
				cosine = CTR_MipsNegLo(cosine);
			}
		}
		else
		{
			cosine = CTR_MipsSra(sine, 16);
			sine = CTR_MipsSll(sine, 16);
			sine = CTR_MipsSra(sine, 16);
			if ((angle & ANG_SIGN_BIT) != 0)
			{
				cosine = CTR_MipsNegLo(cosine);
				sine = CTR_MipsNegLo(sine);
			}
		}
	}

	{
		s32 xVelocity;

		xVelocity = yComponent * sine;
		xVelocity = xVelocity >> FRACTIONAL_BITS;
		velocity->x = xVelocity;
	}
	velocity->z = CTR_MipsSra(CTR_MipsMulLo(yComponent, cosine), FRACTIONAL_BITS);
}

enum
{
	VEH_PHYS_FORCE_QUAD_LOW_GRAVITY = 0x2,
	VEH_PHYS_FORCE_LOW_GRAVITY_DIVISOR = 100,
	VEH_PHYS_FORCE_MUD_TERMINAL_SPEED = 0x100,
	VEH_PHYS_FORCE_SKID_SPEED_THRESHOLD = 0x300,
	VEH_PHYS_FORCE_TERRAIN_SCALE_NEUTRAL = 0x100,
	VEH_PHYS_FORCE_TERRAIN_SIDE_LOCK_TIMER = -0x140,
	VEH_PHYS_FORCE_TERRAIN_RUMBLE_FRAMES = 4,
	VEH_PHYS_FORCE_TERRAIN_RUMBLE_FORCE = 0x7f,
	VEH_PHYS_FORCE_ROLLBACK_WINDOW_TIMER = 0x280,
	VEH_PHYS_FORCE_MATRIX_BLEND_FULL = 0x100,
	VEH_PHYS_FORCE_KART_SCALE_NORMAL = 1,
	VEH_PHYS_FORCE_KART_SCALE_BASE = 0xccc,
	VEH_PHYS_FORCE_MASK_GRAB_SCALE_XZ_FACTOR = 0x28,
	VEH_PHYS_FORCE_MIN_SQUASH_XZ_SCALE = 0x400,
	VEH_PHYS_FORCE_TARGET_SQUISH_DEFAULT = -800,
	VEH_PHYS_FORCE_SQUISH_DEADBAND = 0x960,
	VEH_PHYS_FORCE_AIR_SQUISH_MIN = -800,
	VEH_PHYS_FORCE_LANDING_SQUISH_MIN = -0x640,
	VEH_PHYS_FORCE_SQUISH_MAX = 800,
	VEH_PHYS_FORCE_HAZARD_BLINK_MASK = 0x80,
	VEH_PHYS_FORCE_FALL_STRETCH_HEIGHT_MAX = 0xa00,
	VEH_PHYS_FORCE_FALL_STRETCH_MIN = 0x280,
	VEH_PHYS_FORCE_FALL_STRETCH_MAX = 0x320,
	VEH_PHYS_FORCE_TNT_SCALE_Y_THRESHOLD = 2500,
	VEH_PHYS_FORCE_TNT_SCALE_Y_BASE = 0x800,
	VEH_PHYS_FORCE_SQUISH_INTERP_SPEED = 300,
	VEH_PHYS_FORCE_SQUISH_SCALE_INTERP_SPEED = 0xa0,
	VEH_PHYS_FORCE_SQUISH_SCALE_XZ_FACTOR = 0xa0,
	VEH_PHYS_FORCE_SQUISH_RESTORE_SFX = 0x5b,
	VEH_PHYS_FORCE_SQUISH_OFFSET_NORMAL_SCALE = 0x13,
	VEH_PHYS_FORCE_WAKE_PARTICLE_ICON_GROUP = 9,
	VEH_PHYS_FORCE_WAKE_WATERLINE_Y = 0,
	VEH_PHYS_FORCE_WAKE_VISIBLE_MIN_Y = -0x4f,
	VEH_PHYS_FORCE_WAKE_INITIAL_SCALE = 0x1000,
	VEH_PHYS_FORCE_WAKE_PARTICLE_SPEED_MIN = 0xc00,
	VEH_PHYS_FORCE_WAKE_PARTICLE_PREV_Y_MIN = -0x200,
	VEH_PHYS_FORCE_WAKE_BURST_PARTICLE_COUNT = 10,
	VEH_PHYS_FORCE_TURBO_PAD_RESERVES = 0x3c0,
	VEH_PHYS_FORCE_SUPER_TURBO_PAD_RESERVES = 0x78,
	VEH_PHYS_FORCE_TURBO_PAD_FIRE_LEVEL = 0x100,
	VEH_PHYS_FORCE_SUPER_TURBO_PAD_FIRE_LEVEL = 0x800,
	VEH_PHYS_FORCE_COLLISION_BEST_DIST_INIT = 0x7fffffff,
	VEH_PHYS_FORCE_SURFACE_PUSHBACK_Y_BIAS = 4,
	VEH_PHYS_FORCE_SURFACE_PUSHBACK_SHIFT = 6,
};

static inline u16 VehPhysForce_QuadFlags(const struct QuadBlock *quad)
{
	// NOTE(aalhendi): Native collision searches can leave a missing quad while
	// retail reaches this helper only with a valid fixed-address quad pointer.
	return quad != NULL ? quad->quadFlags : 0;
}


static int VehPhysForce_ClampTerminalVelocity(int velocity, int original, int limit)
{
	// Overspeed must be allowed to move back toward the limit under gravity.
	if (velocity > limit && velocity > original)
	{
		return original < limit ? limit : original;
	}
	if (velocity < -limit && velocity < original)
	{
		return original > -limit ? -limit : original;
	}
	return velocity;
}

void VehPhysForce_OnGravity(struct Driver *driver, Vec3 *velocity)
{
	int originalLocalZ;
	int elapsedTimeMS;
	register int gravityElapsedTimeMS CTR_PSX_REGISTER("$10");
	register int gravityProduct CTR_PSX_REGISTER("$11");
	register int gravityScaled CTR_PSX_REGISTER("$2");
	register int originalLocalX CTR_PSX_REGISTER("$8");
	register int originalLocalY CTR_PSX_REGISTER("$7");
	register int gravityConstant CTR_PSX_REGISTER("$2");
	register int gravityY CTR_PSX_REGISTER("$9");
	register struct GameTracker *gameTracker CTR_PSX_REGISTER("$3");
	register struct QuadBlock *underDriver CTR_PSX_REGISTER("$2");
	register u32 quadFlags CTR_PSX_REGISTER("$2");
	register int gravityX CTR_PSX_REGISTER("$4");
	register int gravityLocalY CTR_PSX_REGISTER("$5");
	register int gravityZ CTR_PSX_REGISTER("$3");
	register int forwardAccelImpulse CTR_PSX_REGISTER("$9");
	u32 actionsFlagSet;
	int speedApprox;
	int baseSpeed;
	register int fireSpeed CTR_PSX_REGISTER("$5");
	register int slopeForwardSpeedBonus CTR_PSX_REGISTER("$4");
	register int halfSlopeForwardSpeedBonus CTR_PSX_REGISTER("$2");
	register int localX CTR_PSX_REGISTER("$21");
	int localY;
	register int localZ CTR_PSX_REGISTER("$19");
	register int speedLimit CTR_PSX_REGISTER("$9");
	TerrainFlags terrainFlags;
	register int kartState CTR_PSX_REGISTER("$5");

	CTR_PSX_OBSERVE_MEMORY(velocity);

	{
		register u32 matrix0 CTR_PSX_REGISTER("$5");
		register u32 matrix1 CTR_PSX_REGISTER("$4");
		register u32 matrix2 CTR_PSX_REGISTER("$7");
		register u32 matrix3 CTR_PSX_REGISTER("$6");
		register s32 matrix4 CTR_PSX_REGISTER("$8");
		register u32 highMask CTR_PSX_REGISTER("$9");

		matrix0 = CTR_ReadU32AlignedLE((const char *)&driver->matrixMovingDir);
		matrix1 = CTR_ReadU32AlignedLE((const char *)&driver->matrixMovingDir + 4);
		matrix2 = CTR_ReadU32AlignedLE((const char *)&driver->matrixMovingDir + 8);
		CTR_PSX_KEEP_VALUE(matrix2);
		matrix3 = CTR_ReadU32AlignedLE((const char *)&driver->matrixMovingDir + 12);
		matrix4 = *(const s16 *)((const char *)&driver->matrixMovingDir + 16);

		CTC2(matrix0, 0);
		CTC2(matrix1, 1);
		CTC2(matrix2, 2);
		CTC2(matrix3, 3);
		CTC2(matrix4, 4);
		highMask = 0xffff0000u;

		// NOTE(aalhendi): Retail transposes matrixMovingDir into the light
		// matrix registers before projecting velocity and gravity locally.
		CTC2((matrix0 & 0xffffu) | (matrix1 & highMask), 8);
		CTC2((matrix3 & 0xffffu) | (matrix0 & highMask), 9);
		CTC2((matrix2 & 0xffffu) | (matrix3 & highMask), 10);
		CTC2((matrix1 & 0xffffu) | (matrix2 & highMask), 11);
		CTC2(matrix4, 12);
	}

	{
		register Vec3 *inputVelocity CTR_PSX_REGISTER("$10") = velocity;
		register u32 inputZ CTR_PSX_REGISTER("$11");

		CTR_PSX_KEEP_VALUE(inputVelocity);
		MTC2(((u32)inputVelocity->x & 0xffffu) | ((u32)inputVelocity->y << 16), 0);
		inputZ = (u32)inputVelocity->z;
		MTC2(inputZ, 1);
		CTR_PSX_GTE_PIPELINE_DELAY();
		gte_mvmva(1, 1, 0, 3, 0);
		localX = MFC2_S(25);
		localY = MFC2_S(26);
		localZ = MFC2_S(27);
	}
	gameTracker = GAME_TRACKER;
	gravityConstant = Chaos_GravityForDriver(driver);
	CTR_PSX_OBSERVE_VALUE(gravityConstant);
	gravityY = CTR_MipsNegLo(gravityConstant);
	CTR_PSX_KEEP_VALUE(gravityY);
	underDriver = driver->underDriver;
	originalLocalY = localY;
	originalLocalZ = localZ;
	CTR_PSX_OBSERVE_MEMORY(originalLocalZ);
	{
		register int elapsedTimeSource CTR_PSX_REGISTER("$3");

		elapsedTimeSource = gameTracker->elapsedTimeMS;
		CTR_PSX_OBSERVE_VALUE(elapsedTimeSource);
		quadFlags = VehPhysForce_QuadFlags(underDriver);
		elapsedTimeMS = elapsedTimeSource;
	}
	originalLocalX = localX;

	// NOTE(aalhendi): Retail does not branch before reading this flag. Native
	// can reach this frame after TeleportSelf clears underDriver and before
	// fixed collision repopulates it; PS1 low-RAM behavior is no low-gravity
	// flag, while host C would crash on a null dereference.
	if ((quadFlags & VEH_PHYS_FORCE_QUAD_LOW_GRAVITY) != 0)
	{
		int scaledGravity = CTR_MipsAddLo(CTR_MipsSll(gravityY, 2), gravityY);
		gravityY = CTR_MipsAddLo(CTR_MipsSll(scaledGravity, 3), gravityY) / VEH_PHYS_FORCE_LOW_GRAVITY_DIVISOR;
	}

	CTR_PSX_OBSERVE_MEMORY(elapsedTimeMS);
	CTR_PSX_RELOAD(elapsedTimeMS);
	gravityElapsedTimeMS = elapsedTimeMS;
	CTR_PSX_KEEP_VALUE(gravityElapsedTimeMS);
	gravityProduct = CTR_MipsMulLo(gravityY, gravityElapsedTimeMS);
	CTR_PSX_OBSERVE_VALUE(gravityProduct);
	gravityScaled = CTR_MipsSra(gravityProduct, 5);
	CTR_PSX_OBSERVE_VALUE(gravityScaled);
	MTC2((u32)CTR_MipsSll(gravityScaled, 16), 0);
	gravityElapsedTimeMS = 0;
	CTR_PSX_KEEP_VALUE(gravityElapsedTimeMS);
	MTC2((u32)gravityElapsedTimeMS, 1);
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_mvmva(1, 1, 0, 3, 0);
	gravityX = MFC2_S(25);
	gravityLocalY = MFC2_S(26);
	gravityZ = MFC2_S(27);
	forwardAccelImpulse = driver->forwardAccelImpulse;
	CTR_PSX_OBSERVE_VALUE(forwardAccelImpulse);

	if (((gravityZ < 0) && (forwardAccelImpulse > 0)) || ((gravityZ > 0) && (forwardAccelImpulse < 0)))
	{
		gravityZ = 0;
	}

	actionsFlagSet = driver->actionsFlagSet;
	speedApprox = driver->speedApprox;
	baseSpeed = driver->baseSpeed;
	if ((actionsFlagSet & ACTION_ACCEL_PREVENTION) || ((baseSpeed > 0) && (speedApprox < 0)) || ((driver->baseSpeed < 0) && (driver->speedApprox > 0)))
	{
		gravityX = 0;
		gravityZ = 0;
	}

	localX = CTR_MipsAddLo(localX, gravityX);
	localY = CTR_MipsAddLo(localY, gravityLocalY);

	fireSpeed = driver->fireSpeed;
	slopeForwardSpeedBonus = driver->const_SlopeForwardSpeedBonus;
	CTR_PSX_KEEP_VALUE(fireSpeed);
	CTR_PSX_KEEP_VALUE(slopeForwardSpeedBonus);
	localZ = CTR_MipsAddLo(localZ, gravityZ);
	speedLimit = CTR_MipsAddLo(fireSpeed, slopeForwardSpeedBonus);
	CTR_PSX_KEEP_VALUE(speedLimit);
	if (localZ > speedLimit)
	{
		localZ = originalLocalZ;
		if (originalLocalZ < speedLimit)
		{
			localZ = speedLimit;
		}
	}

	halfSlopeForwardSpeedBonus = CTR_MipsSra(slopeForwardSpeedBonus, 1);
	CTR_PSX_KEEP_VALUE(halfSlopeForwardSpeedBonus);
	speedLimit = CTR_MipsSubLo(fireSpeed, halfSlopeForwardSpeedBonus);
	CTR_PSX_KEEP_VALUE(speedLimit);
	if (localZ < speedLimit)
	{
		localZ = originalLocalZ;
		if (originalLocalZ > speedLimit)
		{
			localZ = speedLimit;
		}
	}

	{
		register TerrainFlags terrainFlagsValue CTR_PSX_REGISTER("$2");
		register int sideTestOrNegativeLimit CTR_PSX_REGISTER("$3");

		speedLimit = driver->const_SideSpeedClamp;
		terrainFlagsValue = driver->terrainMeta1->flags;
		CTR_PSX_KEEP_VALUE(terrainFlagsValue);
		CTR_PSX_KEEP_VALUE(speedLimit);
		sideTestOrNegativeLimit = speedLimit < localX;
		terrainFlags = terrainFlagsValue;
		if (sideTestOrNegativeLimit)
		{
			localX = originalLocalX;
			CTR_PSX_CLOBBER("$3");
			if (localX < speedLimit)
			{
				localX = speedLimit;
			}
		}

		sideTestOrNegativeLimit = CTR_MipsNegLo(speedLimit);
		CTR_PSX_KEEP_VALUE(sideTestOrNegativeLimit);

		if (localX < sideTestOrNegativeLimit)
		{
			localX = originalLocalX;
			if (originalLocalX > sideTestOrNegativeLimit)
			{
				localX = sideTestOrNegativeLimit;
			}
		}
	}

	CTR_PSX_OBSERVE_MEMORY(terrainFlags);
	speedLimit = driver->const_TerminalVelocity;
	if (localY < 0)
	{
		register TerrainFlags terminalTerrainFlags CTR_PSX_REGISTER("$11");
		register int mudPhysicsTest CTR_PSX_REGISTER("$2");

		terminalTerrainFlags = terrainFlags;
		CTR_PSX_KEEP_VALUE(terminalTerrainFlags);
		mudPhysicsTest = terminalTerrainFlags & TERRAIN_FLAG_MUD_PHYSICS;
		CTR_PSX_KEEP_VALUE(mudPhysicsTest);
		if (mudPhysicsTest != 0)
		{
			speedLimit = VEH_PHYS_FORCE_MUD_TERMINAL_SPEED;
			if (originalLocalY < -VEH_PHYS_FORCE_MUD_TERMINAL_SPEED)
			{
				originalLocalY = -VEH_PHYS_FORCE_MUD_TERMINAL_SPEED;
			}
		}
	}

	localY = VehPhysForce_ClampTerminalVelocity(localY, originalLocalY, speedLimit);

	kartState = driver->kartState;
	CTR_PSX_KEEP_VALUE(kartState);
	if (kartState == KS_MASK_GRABBED)
	{
		localX = 0;
		localZ = 0;
	}
	else if (((driver->actionsFlagSetPrevFrame & ACTION_TOUCH_GROUND) != 0) || (kartState == KS_BLASTED) ||
	         ((driver->speedApprox > driver->terrainScaledBaseSpeed) && (driver->terrainMeta2->speedMultiplier < VEH_PHYS_FORCE_TERRAIN_SCALE_NEUTRAL)))
	{
		int perpendicularFriction;
		int forwardFriction;
		register int terrainFrictionScale CTR_PSX_REGISTER("$9");
		register int terrainNeutralScale;
		register int terrainTimer CTR_PSX_REGISTER("$17");
		register int frictionElapsedTimeMS CTR_PSX_REGISTER("$10");
		register int perpendicularProduct CTR_PSX_REGISTER("$7");
		register int forwardProduct CTR_PSX_REGISTER("$3");

		if ((actionsFlagSet & ACTION_ACCEL_PREVENTION) != 0)
		{
			u32 rawBrakeFriction;
			int absSpeedApprox = speedApprox;

			if (absSpeedApprox < 0)
			{
				CTR_PSX_NEGATE_IN_PLACE(absSpeedApprox);
			}
			if (absSpeedApprox > VEH_PHYS_FORCE_SKID_SPEED_THRESHOLD)
			{
				actionsFlagSet |= ACTION_BACK_SKID;
			}

			rawBrakeFriction = CTR_MipsSll((u16)driver->const_BrakeFriction, 16);
			perpendicularFriction = CTR_MipsSra((s32)rawBrakeFriction, 16);
			if (driver->rainCloudEffect == RAIN_CLOUD_EFFECT_HEAVY_FRICTION)
			{
				perpendicularFriction = CTR_MipsSll(perpendicularFriction, 4);
			}
			else if (kartState == KS_BLASTED)
			{
				perpendicularFriction = CTR_MipsSra(perpendicularFriction * 3, 2);
			}
			else if (kartState == KS_SPINNING)
			{
				perpendicularFriction = CTR_MipsSra((s32)rawBrakeFriction, 17);
			}
			CTR_PSX_OBSERVE_VALUE(perpendicularFriction);
			forwardFriction = perpendicularFriction;
		}
		else
		{
			if (baseSpeed != 0)
			{
				int absSpeedApprox = speedApprox;

				if (absSpeedApprox < 0)
				{
					CTR_PSX_NEGATE_IN_PLACE(absSpeedApprox);
				}
				if ((absSpeedApprox > VEH_PHYS_FORCE_SKID_SPEED_THRESHOLD) &&
				    (((baseSpeed > 0) && (speedApprox < 0)) || ((baseSpeed < 0) && (speedApprox > 0))))
				{
					int brakeAbsSpeed = speedApprox;

					if (brakeAbsSpeed < 0)
					{
						CTR_PSX_NEGATE_IN_PLACE(brakeAbsSpeed);
					}

					perpendicularFriction = driver->const_PedalFriction_Perpendicular;
					forwardFriction = driver->const_BrakeFriction;

					if (brakeAbsSpeed > VEH_PHYS_FORCE_SKID_SPEED_THRESHOLD)
					{
						actionsFlagSet |= ACTION_BACK_SKID;
					}
				}
				else
				{
					if (kartState == KS_DRIFTING)
					{
						perpendicularFriction = driver->const_DriftCurve;
						forwardFriction = driver->const_DriftFriction;
					}
					else
					{
						int pedalAbsSpeed;

						if (speedApprox < 0)
						{
							pedalAbsSpeed = speedApprox;
							CTR_PSX_KEEP_VALUE(pedalAbsSpeed);
							pedalAbsSpeed = CTR_MipsNegLo(pedalAbsSpeed);
						}
						else
						{
							pedalAbsSpeed = speedApprox;
						}
						perpendicularFriction = driver->const_PedalFriction_Perpendicular;
						forwardFriction = driver->const_PedalFriction_Forward;
						if (pedalAbsSpeed > VEH_PHYS_FORCE_SKID_SPEED_THRESHOLD)
						{
							int absBaseSpeed = baseSpeed;

							if (absBaseSpeed < 0)
							{
								CTR_PSX_NEGATE_IN_PLACE(absBaseSpeed);
							}
							if (pedalAbsSpeed < CTR_MipsSra(absBaseSpeed, 1))
							{
								actionsFlagSet |= ACTION_BACK_SKID;
							}
						}
					}
				}
			}
			else
			{
				perpendicularFriction = driver->const_NoPedalFriction_Perpendicular;
				forwardFriction = driver->const_NoPedalFriction_Forward;

				if (driver->rainCloudEffect == RAIN_CLOUD_EFFECT_HEAVY_FRICTION)
				{
					perpendicularFriction = CTR_MipsSll(driver->const_BrakeFriction, 4);
					forwardFriction = perpendicularFriction;
				}
			}
		}

		// NOTE(aalhendi): These lifetimes preserve retail's interleaved multiply
		// pipelines and their delayed MFLO destinations under GCC 2.8.1.
		frictionElapsedTimeMS = elapsedTimeMS;
		CTR_PSX_OBSERVE_VALUE(frictionElapsedTimeMS);
		perpendicularProduct = CTR_MipsMulLo(perpendicularFriction, frictionElapsedTimeMS);
		forwardProduct = CTR_MipsMulLo(forwardFriction, frictionElapsedTimeMS);
		terrainFrictionScale = driver->terrainMeta1->groundFrictionScale;
		terrainNeutralScale = VEH_PHYS_FORCE_TERRAIN_SCALE_NEUTRAL;
		perpendicularFriction = CTR_MipsSra(perpendicularProduct, 5);
		CTR_PSX_KEEP_VALUE(forwardProduct);
		CTR_PSX_OBSERVE_VALUE(perpendicularProduct);
		forwardFriction = CTR_MipsSra(forwardProduct, 5);
		if (terrainFrictionScale != terrainNeutralScale)
		{
			register int terrainPerpendicularProduct CTR_PSX_REGISTER("$3");

			terrainPerpendicularProduct = CTR_MipsMulLo(terrainFrictionScale, perpendicularFriction);
			CTR_PSX_KEEP_VALUE(terrainPerpendicularProduct);
			{
				register int terrainForwardProduct CTR_PSX_REGISTER("$2");

				terrainForwardProduct = CTR_MipsMulLo(terrainFrictionScale, forwardFriction);
				perpendicularFriction = CTR_MipsSra(terrainPerpendicularProduct, 8);
				CTR_PSX_KEEP_VALUE(terrainForwardProduct);
				forwardFriction = CTR_MipsSra(terrainForwardProduct, 8);
			}
		}

		terrainTimer = driver->terrainFrictionTimer;
		CTR_PSX_KEEP_VALUE(terrainTimer);
		if (terrainTimer < 0)
		{
			int absLocalX;
			if (terrainTimer == VEH_PHYS_FORCE_TERRAIN_SIDE_LOCK_TIMER)
			{
				if (localX < 0)
				{
					absLocalX = localX;
					CTR_PSX_KEEP_VALUE(absLocalX);
					absLocalX = CTR_MipsNegLo(absLocalX);
				}
				else
				{
					absLocalX = localX;
				}
				perpendicularFriction = CTR_MipsSra(absLocalX, 1);
			}
			else
			{
				{
					register int terrainBoostValue CTR_PSX_REGISTER("$2");
					register int terrainBoostProduct CTR_PSX_REGISTER("$10");
					int terrainBoostScaled;

					terrainBoostValue = driver->const_TerrainFrictionBoost;
					terrainBoostProduct = CTR_MipsMulLo(perpendicularFriction, terrainBoostValue);
					CTR_PSX_KEEP_VALUE(terrainBoostValue);
					terrainBoostScaled = CTR_MipsSra(terrainBoostProduct, 8);
					CTR_PSX_KEEP_VALUE(terrainBoostProduct);
					perpendicularFriction = CTR_MipsAddLo(perpendicularFriction, terrainBoostScaled);
				}
				if (perpendicularFriction < 0)
				{
					perpendicularFriction = 0;
				}

				if (localX < 0)
				{
					absLocalX = localX;
					CTR_PSX_KEEP_VALUE(absLocalX);
					absLocalX = CTR_MipsNegLo(absLocalX);
				}
				else
				{
					absLocalX = localX;
				}
			}

			if (absLocalX > 0)
			{
				actionsFlagSet |= ACTION_BACK_SKID | ACTION_FRONT_SKID;
				GAMEPAD_ShockForce1(driver, VEH_PHYS_FORCE_TERRAIN_RUMBLE_FRAMES, VEH_PHYS_FORCE_TERRAIN_RUMBLE_FORCE);
				GAMEPAD_ShockFreq(driver, VEH_PHYS_FORCE_TERRAIN_RUMBLE_FRAMES, 0);
			}

			{
				register int negativeElapsedTimeMS CTR_PSX_REGISTER("$11");

				CTR_PSX_RELOAD(elapsedTimeMS);
				negativeElapsedTimeMS = elapsedTimeMS;
				CTR_PSX_KEEP_VALUE(negativeElapsedTimeMS);
				terrainTimer = CTR_MipsAddLo(terrainTimer, negativeElapsedTimeMS);
			}
			if (terrainTimer > 0)
			{
				terrainTimer = 0;
			}
			driver->terrainFrictionTimer = (s16)terrainTimer;
		}
		else if (terrainTimer > 0)
		{
			CTR_PSX_RELOAD(elapsedTimeMS);
			frictionElapsedTimeMS = elapsedTimeMS;
			CTR_PSX_KEEP_VALUE(frictionElapsedTimeMS);
			terrainTimer = CTR_MipsSubLo(terrainTimer, frictionElapsedTimeMS);
			if (terrainTimer < 0)
			{
				terrainTimer = 0;
			}

			{
				register int terrainBoostValue CTR_PSX_REGISTER("$2");
				register int terrainBoostProduct CTR_PSX_REGISTER("$10");
				int terrainBoostScaled;

				terrainBoostValue = driver->const_TerrainFrictionBoost;
				terrainBoostProduct = CTR_MipsMulLo(perpendicularFriction, terrainBoostValue);
				CTR_PSX_KEEP_VALUE(terrainBoostValue);
				terrainBoostScaled = CTR_MipsSra(terrainBoostProduct, 8);
				CTR_PSX_KEEP_VALUE(terrainBoostProduct);
				perpendicularFriction = CTR_MipsAddLo(perpendicularFriction, terrainBoostScaled);
			}
			driver->terrainFrictionTimer = (s16)terrainTimer;
			if (perpendicularFriction < 0)
			{
				perpendicularFriction = 0;
			}
		}

		// NOTE(aalhendi): Retail reloads the spilled terrain flags separately
		// from each mask result, so both values must remain live for the test.
		if ((actionsFlagSet & ACTION_MASK_WEAPON) == 0)
		{
			register TerrainFlags mudTerrainFlags CTR_PSX_REGISTER("$11");
			int mudPhysicsSet;

			CTR_PSX_RELOAD(terrainFlags);
			mudTerrainFlags = terrainFlags;
			mudPhysicsSet = mudTerrainFlags & TERRAIN_FLAG_MUD_PHYSICS;
			CTR_PSX_OBSERVE_VALUE(mudTerrainFlags);
			if (mudPhysicsSet != 0)
			{
				int absSideSpeed;
				int minForwardFriction;
				int sideSpeed;

				sideSpeed = CTR_MipsSra(localX, 3);
				if (sideSpeed < 0)
				{
					absSideSpeed = sideSpeed;
					CTR_PSX_KEEP_VALUE(absSideSpeed);
					absSideSpeed = CTR_MipsNegLo(absSideSpeed);
				}
				else
				{
					absSideSpeed = sideSpeed;
				}

				if (perpendicularFriction < absSideSpeed)
				{
					perpendicularFriction = absSideSpeed;
				}

				minForwardFriction = 0;
				if (localZ == 0)
				{
					goto MUD_COMPARE_ORDER;
				}
				if (baseSpeed == 0)
				{
					goto MUD_COMPARE_ORDER;
				}
				if ((localZ ^ baseSpeed) >= 0)
				{
					goto MUD_COMPARE_ORDER;
				}

				// NOTE(aalhendi): Keeping the shifted value separate lets retail move
				// the absolute result into the branch delay slot before comparing it.
				{
					register int halfLocalZ CTR_PSX_REGISTER("$2");
					int absoluteHalfLocalZ;

					halfLocalZ = CTR_MipsSra(localZ, 1);
					absoluteHalfLocalZ = halfLocalZ;
					if (halfLocalZ < 0)
					{
						absoluteHalfLocalZ = CTR_MipsNegLo(absoluteHalfLocalZ);
					}
					CTR_PSX_KEEP_VALUE(absoluteHalfLocalZ);
					minForwardFriction = absoluteHalfLocalZ;
				}
				goto MUD_COMPARE_FRICTION;

			MUD_COMPARE_ORDER:
				if (!(localZ < baseSpeed))
				{
					goto MUD_CHECK_REVERSE_ORDER;
				}
				if (localZ <= 0)
				{
					goto MUD_COMPUTE_DIFFERENCE;
				}

			MUD_CHECK_REVERSE_ORDER:
				if (!(baseSpeed < localZ))
				{
					goto APPLY_TERRAIN_FRICTION;
				}
				if (localZ < 0)
				{
					goto APPLY_TERRAIN_FRICTION;
				}

			MUD_COMPUTE_DIFFERENCE:
			{
				int speedDifference;

				speedDifference = CTR_MipsSubLo(localZ, baseSpeed);
				if (speedDifference < 0)
				{
					speedDifference = CTR_MipsNegLo(speedDifference);
				}
				CTR_PSX_KEEP_VALUE(speedDifference);
				minForwardFriction = CTR_MipsSra(speedDifference, 1);
			}

			MUD_COMPARE_FRICTION:
				if (forwardFriction < minForwardFriction)
				{
					forwardFriction = minForwardFriction;
				}
			}
		}

	APPLY_TERRAIN_FRICTION:
	{
		register TerrainFlags sideslipTerrainFlags CTR_PSX_REGISTER("$10");
		int sideslipFrictionSet;

		CTR_PSX_RELOAD(terrainFlags);
		sideslipTerrainFlags = terrainFlags;
		sideslipFrictionSet = sideslipTerrainFlags & TERRAIN_FLAG_SIDESLIP_FRICTION;
		CTR_PSX_OBSERVE_VALUE(sideslipTerrainFlags);
		if (sideslipFrictionSet != 0)
		{
			perpendicularFriction = CTR_MipsSra(perpendicularFriction * 3, 2);
			if (perpendicularFriction < forwardFriction)
			{
				perpendicularFriction = forwardFriction;
			}
		}

		localX = VehCalc_InterpBySpeed(localX, perpendicularFriction, 0);
		localZ = VehCalc_InterpBySpeed(localZ, forwardFriction, 0);
	}
	}

	{
		register u32 packedLocalXY CTR_PSX_REGISTER("$2");

		packedLocalXY = (u16)localX;
		CTR_PSX_KEEP_VALUE(packedLocalXY);
		packedLocalXY |= (u32)(u16)localY << 16;
		CTR_PSX_KEEP_VALUE(packedLocalXY);
		MTC2(packedLocalXY, 0);
	}
	MTC2((u32)localZ, 1);
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_mvmva(1, 0, 0, 3, 0);
	{
		register Vec3 *outputVelocity CTR_PSX_REGISTER("$11") = velocity;

		CTR_PSX_KEEP_VALUE(outputVelocity);
		CTR_PSX_STORE_COP2_VEC3(&outputVelocity->x, 25, 26, 27);
	}
	driver->actionsFlagSet = actionsFlagSet;

	if ((actionsFlagSet & ACTION_AIRBORNE) == 0)
	{
		if ((driver->boolFirstFrameSinceRevEngine != 0) && (localZ != 0))
		{
			driver->forwardDir = (localZ < 0) ? -1 : 1;
			driver->boolFirstFrameSinceRevEngine = 0;
			goto CHECK_ROLLBACK_FROM_PREVIOUS_DIRECTION;
		}

		{
			register int previousLocalZ CTR_PSX_REGISTER("$10");

			previousLocalZ = originalLocalZ;
			CTR_PSX_KEEP_VALUE(previousLocalZ);
			if (previousLocalZ < 0)
			{
				goto SET_FORWARD_DIRECTION_IF_NONNEGATIVE;
			}
		}

		if (localZ < 0)
		{
			driver->forwardDir = -1;
		}

		{
			register int previousLocalZ CTR_PSX_REGISTER("$11");

			previousLocalZ = originalLocalZ;
			CTR_PSX_KEEP_VALUE(previousLocalZ);
			if (previousLocalZ > 0)
			{
				goto CHECK_ROLLBACK_FROM_FORWARD;
			}
		}

	SET_FORWARD_DIRECTION_IF_NONNEGATIVE:
		if (localZ >= 0)
		{
			driver->forwardDir = 1;
		}
	}

CHECK_ROLLBACK_FROM_PREVIOUS_DIRECTION:
{
	register int previousLocalZ CTR_PSX_REGISTER("$10");

	previousLocalZ = originalLocalZ;
	CTR_PSX_KEEP_VALUE(previousLocalZ);
	if (previousLocalZ >= 0)
	{
		goto CHECK_ROLLBACK_FROM_FORWARD;
	}
}

	if (localZ <= 0)
	{
		return;
	}
	goto START_ROLLBACK;

CHECK_ROLLBACK_FROM_FORWARD:
	if (localZ < 0)
	{
		goto START_ROLLBACK;
	}
	{
		register int previousLocalZ CTR_PSX_REGISTER("$11");

		previousLocalZ = originalLocalZ;
		CTR_PSX_KEEP_VALUE(previousLocalZ);
		if (previousLocalZ > 0)
		{
			return;
		}
	}
	if (localZ <= 0)
	{
		return;
	}

START_ROLLBACK:
	if (driver->vShiftWindowTimer != 0)
	{
		driver->vShiftCount = (s16)CTR_MipsAddLo((u16)driver->vShiftCount, 1);
	}
	driver->vShiftWindowTimer = VEH_PHYS_FORCE_ROLLBACK_WINDOW_TIMER;
}

void VehPhysForce_OnApplyForces(struct Thread *thread, struct Driver *driver)
{
	const int maxMudSinkYLevel = FP(-1);
	const int maxSpeed = FP8(100);
	register s32 velocityX CTR_PSX_REGISTER("$2");
	register s32 velocityZ CTR_PSX_REGISTER("$3");
	register s32 accelX CTR_PSX_REGISTER("$4");
	register s32 accelZ CTR_PSX_REGISTER("$5");

	(void)thread;
	if (driver->speed > maxSpeed)
	{
		driver->speed = maxSpeed;
	}

	/* origin of driver model is center-bottom of kart,
	use orientation matrix, and half-radius {0, 25, 0},
	to find the "true" center of the 3D model */
	VehGteSetColorMatrix(&driver->matrixFacingDir);
	MTC2(0x190000, 0);
	MTC2(0, 1);
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_mvmva(1, 2, 0, 3, 0);
	CTR_PSX_STORE_COP2_WORD(&driver->originToCenter.x, 25);
	CTR_PSX_STORE_COP2_WORD(&driver->originToCenter.y, 26);
	CTR_PSX_STORE_COP2_WORD(&driver->originToCenter.z, 27);

	VehPhysForce_ConvertSpeedToVec(driver, &driver->velocity);

	if ((driver->underDriver) && (driver->underDriver->terrain_type == TERRAIN_MUD))
	{
		if (driver->posCurr.y > maxMudSinkYLevel)
		{
			// sink slower as you approach the mud's bottom
			int sinkSpeed = CTR_MipsSubLo(maxMudSinkYLevel, driver->posCurr.y);
			if (driver->velocity.y < sinkSpeed)
			{
				driver->velocity.y = sinkSpeed;
			}
		}
	}

	VehPhysForce_OnGravity(driver, &driver->velocity);

	accelX = driver->accel.x;
	CTR_PSX_KEEP_VALUE(accelX);
	velocityX = driver->velocity.x;
	accelZ = driver->accel.z;
	CTR_PSX_KEEP_VALUE(accelZ);
	*(VecElement *)&driver->normalVecUP.x = 0x10000000;
	*(VecElement *)&driver->AxisAngle1_normalVec.x = 0x10000000;
	driver->currBlockTouching = nullptr;
	driver->normalVecUP.z = 0;
	driver->AxisAngle1_normalVec.z = 0;
	driver->collisionFlags = 0;

	velocityX = CTR_MipsAddLo(velocityX, accelX);
	velocityZ = CTR_MipsAddLo(driver->velocity.z, accelZ);
	driver->velocity.x = velocityX;
	driver->velocity.z = velocityZ;
	driver->velocity.y = CTR_MipsAddLo(driver->velocity.y, driver->accel.y);
}

void VehPhysForce_CollideDrivers(struct Thread *thread, struct Driver *driver)
{
	CollStepFlags stepFlagSet = driver->stepFlagSet;

	driver->velocity.x = CTR_MipsSubLo(driver->velocity.x, driver->accel.x);
	driver->velocity.y = CTR_MipsSubLo(driver->velocity.y, driver->accel.y);
	driver->velocity.z = CTR_MipsSubLo(driver->velocity.z, driver->accel.z);

	if ((stepFlagSet & COLL_STEP_FLAG_KILL_PLANE) != 0 && (GAME_TRACKER->gameMode1 & BATTLE_MODE) == 0)
	{
		driver->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
	}

	{
		register struct Driver *fireDriver CTR_PSX_REGISTER("$4");
		int reserves;
		int turboFlags;
		int fireLevel;

		fireDriver = driver;

		if ((stepFlagSet & COLL_STEP_TRIGGER_SUPER_TURBO_PAD) != 0)
		{
			goto ApplySuperTurboPad;
		}
		if ((stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD) == 0)
		{
			goto TurboPadDone;
		}
		reserves = VEH_PHYS_FORCE_TURBO_PAD_RESERVES;
		if ((GAME_TRACKER->gameMode2 & CHEAT_TURBOPAD) == 0)
		{
			goto ApplyNormalTurboPad;
		}

	ApplySuperTurboPad:
		reserves = VEH_PHYS_FORCE_SUPER_TURBO_PAD_RESERVES;
		turboFlags = TURBO_PAD | FREEZE_RESERVES_ON_TURBO_PAD;
		fireLevel = VEH_PHYS_FORCE_SUPER_TURBO_PAD_FIRE_LEVEL;
		goto ApplyTurboPad;

	ApplyNormalTurboPad:
		turboFlags = TURBO_PAD | FREEZE_RESERVES_ON_TURBO_PAD;
		fireLevel = VEH_PHYS_FORCE_TURBO_PAD_FIRE_LEVEL;

	ApplyTurboPad:
		VehFire_Increment(fireDriver, reserves, turboFlags, fireLevel);

	TurboPadDone:;
	}
	Chaos_OnTurboPadContact(driver, (stepFlagSet & (COLL_STEP_TRIGGER_TURBO_PAD | COLL_STEP_TRIGGER_SUPER_TURBO_PAD)) != 0);

	if ((stepFlagSet & COLL_STEP_FLAG_WATER_BSP) != 0)
	{
		struct Instance *inst = thread->inst;

		inst->vertSplit = 0;
		inst->flags |= SPLIT_LINE;
	}
	else
	{
		thread->inst->flags &= ~SPLIT_LINE;
	}

	if ((thread->flags & THREAD_FLAG_DISABLE_COLLISION) == 0 && Chaos_IsTimeStopped())
	{
		Chaos_RecordFrozenMaskContacts(driver);
	}
	if ((thread->flags & THREAD_FLAG_DISABLE_COLLISION) == 0 && !Chaos_IsTimeStopped())
	{
		struct DriverCollisionSearch search;
		int bucketPosY;
		int bucketPosZ;

		search.bucket.pos.x = (s16)CTR_MipsSra(driver->posCurr.x, FRACTIONAL_BITS_8);
		bucketPosY = driver->posCurr.y;
		search.bucket.pos.y = (s16)CTR_MipsSra(bucketPosY, FRACTIONAL_BITS_8);
		bucketPosZ = driver->posCurr.z;
		search.bucket.pos.z = (s16)CTR_MipsSra(bucketPosZ, FRACTIONAL_BITS_8);
		search.bucket.bestDistSq = VEH_PHYS_FORCE_COLLISION_BEST_DIST_INIT;
		search.bucket.th = NULL;

		PROC_CollidePointWithBucket(thread->siblingThread, &search.bucket);
		PROC_CollidePointWithBucket(GAME_TRACKER->threadBuckets[ROBOT].thread, &search.bucket);

		if (search.bucket.th != NULL)
		{
			struct Driver *other = search.bucket.th->object;
			int radiusSum = VehPhysForce_ScaleValue(driver, thread->driverHitRadius) +
			                VehPhysForce_ScaleValue(other, search.bucket.th->driverHitRadius);

			CTR_PSX_CLOBBER("$9");
			if (search.bucket.bestDistSq < CTR_MipsMulLo(radiusSum, radiusSum))
			{
				VehPhysCrash_AnyTwoCars(thread, &search, &driver->velocity);
			}
		}
	}

	if ((driver->collisionFlags & DRIVER_COLL_FLAG_SURFACE_PUSHBACK) != 0)
	{
		int retailStackPad0;
		int retailStackPad1;
		int retailStackPad2;
		int retailStackPad3;
		register int diffX CTR_PSX_REGISTER("$6");
		register int diffZ CTR_PSX_REGISTER("$5");
		register int floorDiffY CTR_PSX_REGISTER("$4");
		register int productX CTR_PSX_REGISTER("$9");
		register int productY CTR_PSX_REGISTER("$7");
		register int productZ CTR_PSX_REGISTER("$3");
		register int dotProduct CTR_PSX_REGISTER("$2");
		register int hitY CTR_PSX_REGISTER("$8");
		register int posZ CTR_PSX_REGISTER("$3");
		register int velocityX CTR_PSX_REGISTER("$2");

		// NOTE(aalhendi): GCC 2.8.1 otherwise shortens and reorders these
		// multiply pipelines. The PSX path retains their retail HI/LO schedule.
		diffX = CTR_MipsSubLo(CTR_MipsSra(driver->posCurr.x, FRACTIONAL_BITS_8), driver->spsHitPos.x);
		productX = CTR_MipsMulLo(driver->spsNormalVec.x, diffX);
		posZ = driver->quadBlockHeight;
		floorDiffY = CTR_MipsAddLo(CTR_MipsSubLo(CTR_MipsSra(posZ, FRACTIONAL_BITS_8), driver->spsHitPos.y), VEH_PHYS_FORCE_SURFACE_PUSHBACK_Y_BIAS);
		productY = CTR_MipsMulLo(driver->spsNormalVec.y, floorDiffY);
		posZ = driver->posCurr.z;
		diffZ = CTR_MipsSubLo(CTR_MipsSra(posZ, FRACTIONAL_BITS_8), driver->spsHitPos.z);
		productZ = CTR_MipsMulLo(driver->spsNormalVec.z, diffZ);
		dotProduct = CTR_MipsAddLo(CTR_MipsAddLo(productX, productY), productZ);
		hitY = driver->spsHitPos.y;

		CTR_PSX_OBSERVE_MEMORY(retailStackPad0);
		CTR_PSX_OBSERVE_MEMORY(retailStackPad1);
		CTR_PSX_OBSERVE_MEMORY(retailStackPad2);
		CTR_PSX_OBSERVE_MEMORY(retailStackPad3);

		if (dotProduct < 0)
		{
			int diffY;
			register int shiftedDiffY CTR_PSX_REGISTER("$4");
			register int velocityY CTR_PSX_REGISTER("$2");
			register int velocityZ CTR_PSX_REGISTER("$3");

			diffX = CTR_MipsSll(diffX, VEH_PHYS_FORCE_SURFACE_PUSHBACK_SHIFT);
			CTR_PSX_CLOBBER("$11");
			CTR_PSX_SHIFT_LEFT_IN_PLACE(diffZ, VEH_PHYS_FORCE_SURFACE_PUSHBACK_SHIFT);
			posZ = driver->posCurr.y;
			velocityX = driver->velocity.x;

			diffY = CTR_MipsSubLo(CTR_MipsSra(posZ, FRACTIONAL_BITS_8), hitY);
			shiftedDiffY = CTR_MipsSll(diffY, VEH_PHYS_FORCE_SURFACE_PUSHBACK_SHIFT);
			driver->velocity.x = CTR_MipsAddLo(velocityX, diffX);
			CTR_PSX_MEMORY_BARRIER();
			velocityY = driver->velocity.y;
			velocityZ = driver->velocity.z;
			driver->velocity.y = CTR_MipsAddLo(velocityY, shiftedDiffY);
			driver->velocity.z = CTR_MipsAddLo(velocityZ, diffZ);
		}
	}
}

static inline int VehPhysForce_TranslateMatrix_Abs(int value)
{
	return value < 0 ? CTR_MipsNegLo(value) : value;
}

static inline int VehPhysForce_TranslateMatrix_Div256TowardZero(int value)
{
	if (value < 0)
	{
		value = CTR_MipsAddLo(value, 0xff);
	}

	return CTR_MipsSra(value, 8);
}

static inline struct MatrixND *VehPhysForce_TranslateMatrix_GetBakedEntry(u8 matrixArray, u8 matrixIndex)
{
	return &((struct MatrixND *)VEH_BAKED_GTE_PHYS_ENTRY(matrixArray))[matrixIndex];
}

static inline u8 VehPhysForce_TranslateMatrix_RemapIndex(u8 matrixIndex, int fromArray, int toArray)
{
	int scaledIndex = CTR_MipsSll(matrixIndex, 8);
	int fromLast = CTR_MipsSubLo(VEH_BAKED_GTE_NUM_ENTRIES(fromArray), 1);
	register int quotient CTR_PSX_REGISTER("$3");
	register int blend CTR_PSX_REGISTER("$16");
	register int fullBlend CTR_PSX_REGISTER("$2");

	quotient = CTR_MipsDiv(scaledIndex, fromLast);
	CTR_PSX_OBSERVE_VALUE(quotient);

	CTR_PSX_LOAD_IMMEDIATE(fullBlend, VEH_PHYS_FORCE_MATRIX_BLEND_FULL);
	blend = CTR_MipsSubLo(fullBlend, quotient);
	CTR_PSX_OBSERVE_VALUE(fullBlend);

	if (blend < 0)
	{
		blend = 0;
	}
	if (blend > VEH_PHYS_FORCE_MATRIX_BLEND_FULL)
	{
		blend = VEH_PHYS_FORCE_MATRIX_BLEND_FULL;
	}

	blend = CTR_MipsSra(CTR_MipsMulLo(blend, CTR_MipsSubLo(VEH_BAKED_GTE_NUM_ENTRIES(toArray), 1)), 8);
	return (u8)blend;
}

static inline void VehPhysForce_TranslateMatrix_UpdateMatrixAnimation(struct Driver *d)
{
	int matrixArray;
	register int matrixArraySource CTR_PSX_REGISTER("$2");

	if ((d->reserves != 0) && (d->fireSpeed >= d->const_Speed_ClassStat) && ((d->actionsFlagSet & ACTION_TURBO_INPUT_LATCH) == 0))
	{
		matrixArray = d->matrixArray;
		if (matrixArray == BAKED_GTE_MATRIX_WHEELIE_START)
		{
			goto ADVANCE_WHEELIE_START;
		}

		if (matrixArray < BAKED_GTE_MATRIX_WHEELIE_HOLD)
		{
			if (matrixArray == BAKED_GTE_MATRIX_NONE)
			{
				goto BEGIN_WHEELIE_START;
			}
			goto UPDATE_SQUISH_RECOVER;
		}

		if (matrixArray == BAKED_GTE_MATRIX_WHEELIE_HOLD)
		{
			goto UPDATE_SQUISH_RECOVER;
		}
		if (matrixArray == BAKED_GTE_MATRIX_WHEELIE_RECOVER)
		{
			goto REMAP_WHEELIE_RECOVER_TO_START;
		}
		goto UPDATE_SQUISH_RECOVER;

	BEGIN_WHEELIE_START:
		d->matrixArray = BAKED_GTE_MATRIX_WHEELIE_START;
		goto RESET_MATRIX_INDEX;

	ADVANCE_WHEELIE_START:
		d->matrixIndex++;
		if (d->matrixIndex >= VEH_BAKED_GTE_NUM_ENTRIES(BAKED_GTE_MATRIX_WHEELIE_START))
		{
			d->matrixArray = BAKED_GTE_MATRIX_WHEELIE_HOLD;
			goto RESET_MATRIX_INDEX;
		}
		goto UPDATE_SQUISH_RECOVER;

	REMAP_WHEELIE_RECOVER_TO_START:
		d->matrixIndex = VehPhysForce_TranslateMatrix_RemapIndex(d->matrixIndex, BAKED_GTE_MATRIX_WHEELIE_RECOVER, BAKED_GTE_MATRIX_WHEELIE_START);
		d->matrixArray = BAKED_GTE_MATRIX_WHEELIE_START;
		goto UPDATE_SQUISH_RECOVER;
	}

	matrixArraySource = d->matrixArray;
	CTR_PSX_OBSERVE_VALUE(matrixArraySource);
	matrixArray = matrixArraySource;
	if (matrixArraySource == BAKED_GTE_MATRIX_NONE)
	{
		goto UPDATE_SQUISH_RECOVER;
	}

	if (matrixArray == BAKED_GTE_MATRIX_WHEELIE_HOLD)
	{
		goto BEGIN_WHEELIE_RECOVER;
	}

	if (matrixArray < BAKED_GTE_MATRIX_WHEELIE_RECOVER)
	{
		if (matrixArray == BAKED_GTE_MATRIX_WHEELIE_START)
		{
			goto REMAP_WHEELIE_START_TO_RECOVER;
		}
		goto UPDATE_SQUISH_RECOVER;
	}

	if (matrixArray == BAKED_GTE_MATRIX_WHEELIE_RECOVER)
	{
		goto ADVANCE_WHEELIE_RECOVER;
	}
	goto UPDATE_SQUISH_RECOVER;

REMAP_WHEELIE_START_TO_RECOVER:
	d->matrixIndex = VehPhysForce_TranslateMatrix_RemapIndex(d->matrixIndex, BAKED_GTE_MATRIX_WHEELIE_START, BAKED_GTE_MATRIX_WHEELIE_RECOVER);
	d->matrixArray = BAKED_GTE_MATRIX_WHEELIE_RECOVER;
	goto UPDATE_SQUISH_RECOVER;

BEGIN_WHEELIE_RECOVER:
	d->matrixArray = BAKED_GTE_MATRIX_WHEELIE_RECOVER;
	goto RESET_MATRIX_INDEX;

ADVANCE_WHEELIE_RECOVER:
	d->matrixIndex++;
	if (d->matrixIndex >= VEH_BAKED_GTE_NUM_ENTRIES(BAKED_GTE_MATRIX_WHEELIE_RECOVER))
	{
		d->matrixArray = BAKED_GTE_MATRIX_NONE;
		goto RESET_MATRIX_INDEX;
	}
	goto UPDATE_SQUISH_RECOVER;

RESET_MATRIX_INDEX:
	d->matrixIndex = 0;

UPDATE_SQUISH_RECOVER:
	if (d->matrixArray == BAKED_GTE_MATRIX_SQUISH_RECOVER)
	{
		d->matrixIndex++;
		if (d->matrixIndex >= VEH_BAKED_GTE_NUM_ENTRIES(BAKED_GTE_MATRIX_SQUISH_RECOVER))
		{
			d->matrixArray = BAKED_GTE_MATRIX_NONE;
			d->matrixIndex = 0;
		}
	}
}

typedef struct CTR_MAY_ALIAS VehPhysForceRotationWords
{
	CtrPackedU32 word0;
	CtrPackedU32 word1;
	CtrPackedU32 word2;
	CtrPackedU32 word3;
	u16 word4;
} VehPhysForceRotationWords;

static inline void VehPhysForce_TranslateMatrix_CopyFacingMatrix(struct Instance *inst, struct Driver *d)
{
	((VehPhysForceRotationWords *)&inst->matrix)->word0 = ((const VehPhysForceRotationWords *)&d->matrixFacingDir)->word0;
	((VehPhysForceRotationWords *)&inst->matrix)->word1 = ((const VehPhysForceRotationWords *)&d->matrixFacingDir)->word1;
	((VehPhysForceRotationWords *)&inst->matrix)->word2 = ((const VehPhysForceRotationWords *)&d->matrixFacingDir)->word2;
	((VehPhysForceRotationWords *)&inst->matrix)->word3 = ((const VehPhysForceRotationWords *)&d->matrixFacingDir)->word3;
	((VehPhysForceRotationWords *)&inst->matrix)->word4 = ((const VehPhysForceRotationWords *)&d->matrixFacingDir)->word4;
}

static inline void VehPhysForce_TranslateMatrix_UpdateInstanceMatrix(struct Instance *inst, struct Driver *d)
{
	register int matrixArray CTR_PSX_REGISTER("$2");
	register int matrixArrayIndex CTR_PSX_REGISTER("$3");

	matrixArray = d->matrixArray;
	CTR_PSX_KEEP_VALUE(matrixArray);
	if (matrixArray != BAKED_GTE_MATRIX_NONE)
	{
		struct MatrixND *entry;
		s16 *entryVec;
		int rotatedX;
		register int rotatedY CTR_PSX_REGISTER("$17");
		int rotatedZ;
		register u32 entryZ CTR_PSX_REGISTER("$16");
		MatrixNDOverlapMatrix *matrix;
		register MATRIX *instMatrix CTR_PSX_REGISTER("$4");
		register const CtrPackedU32 *facingWords CTR_PSX_REGISTER("$17");
		register MATRIX *facingArgument CTR_PSX_REGISTER("$5");

		instMatrix = &inst->matrix;
		facingWords = (const CtrPackedU32 *)&d->matrixFacingDir;
		// NOTE(aalhendi): Preserve the retail array-index and MatrixRotate argument lifetimes.
		matrixArrayIndex = matrixArray;
		facingArgument = (MATRIX *)facingWords;
		entry = VehPhysForce_TranslateMatrix_GetBakedEntry(matrixArrayIndex, d->matrixIndex);
		entryVec = (s16 *)entry;
		matrix = MatrixND_GetOverlapMatrix(entry);
		MatrixRotate(instMatrix, facingArgument, (MATRIX *)matrix);

		{
			register u32 matrixWord0 CTR_PSX_REGISTER("$12");
			register u32 matrixWord1 CTR_PSX_REGISTER("$13");
			register u32 matrixWord4 CTR_PSX_REGISTER("$14");

			CTR_PSX_KEEP_VALUE(facingWords);
			matrixWord0 = facingWords[0];
			matrixWord1 = facingWords[1];
			CTC2(matrixWord0, 0);
			CTC2(matrixWord1, 1);
			matrixWord0 = facingWords[2];
			matrixWord1 = facingWords[3];
			matrixWord4 = facingWords[4];
			CTC2(matrixWord0, 2);
			CTC2(matrixWord1, 3);
			CTC2(matrixWord4, 4);
		}

		MTC2((u32)(u16)entryVec[0] | ((u32)entryVec[1] << 16), 0);
		entryZ = (u16)entryVec[2];
		CTR_PSX_KEEP_VALUE(entryZ);
		MTC2(entryZ, 1);
		CTR_PSX_GTE_PIPELINE_DELAY();
		gte_mvmva(1, 0, 0, 3, 0);
		rotatedX = MFC2_S(25);
		rotatedY = MFC2_S(26);
		CTR_PSX_KEEP_VALUE(rotatedY);
		rotatedZ = MFC2_S(27);

		inst->matrix.t[0] = CTR_MipsSra(CTR_MipsAddLo(d->posCurr.x, rotatedX), 8);
		inst->matrix.t[1] = CTR_MipsAddLo(CTR_MipsSra(CTR_MipsAddLo(d->posCurr.y, rotatedY), 8), ((s8)d->Screen_OffsetY * 3) >> 3);
		inst->matrix.t[2] = CTR_MipsSra(CTR_MipsAddLo(d->posCurr.z, rotatedZ), 8);
	}
	else
	{
		VehPhysForce_TranslateMatrix_CopyFacingMatrix(inst, d);

		inst->matrix.t[0] = CTR_MipsSra(d->posCurr.x, 8);
		inst->matrix.t[1] = CTR_MipsAddLo(CTR_MipsSra(d->posCurr.y, 8), ((s8)d->Screen_OffsetY * 3) >> 3);
		inst->matrix.t[2] = CTR_MipsSra(d->posCurr.z, 8);
	}

	if (d->squishTimer != 0)
	{
		inst->matrix.t[0] = CTR_MipsAddLo(inst->matrix.t[0], CTR_MipsSra(d->AxisAngle2_normalVec.x * VEH_PHYS_FORCE_SQUISH_OFFSET_NORMAL_SCALE, 12));
		inst->matrix.t[1] = CTR_MipsAddLo(inst->matrix.t[1], CTR_MipsSra(d->AxisAngle2_normalVec.y * VEH_PHYS_FORCE_SQUISH_OFFSET_NORMAL_SCALE, 12));
		inst->matrix.t[2] = CTR_MipsAddLo(inst->matrix.t[2], CTR_MipsSra(d->AxisAngle2_normalVec.z * VEH_PHYS_FORCE_SQUISH_OFFSET_NORMAL_SCALE, 12));
	}
}

static inline void VehPhysForce_TranslateMatrix_SpawnWakeParticle(struct Driver *d)
{
	struct Particle *p = Particle_Init(0, GAME_TRACKER->iconGroup[VEH_PHYS_FORCE_WAKE_PARTICLE_ICON_GROUP], &data.emSet_Falling[0]);

	if (p != NULL)
	{
		VehPhysForce_ScaleParticle(d, p);
		p->otIndexOffset = d->instSelf->depthBiasNormal;
		p->owner.driverInst = d->instSelf;
		p->driverID = d->driverID;
	}
}

static inline void VehPhysForce_TranslateMatrix_SetWakeRotation(struct Instance *wake, struct Driver *d)
{
	register s32 angle CTR_PSX_REGISTER("$16");
	s32 trigOrSin;
	s32 matrixCos;

	angle = d->angle;
	trigOrSin = (s32)VEH_TRIG_APPROX(angle & 0x3ff);
	CTR_PSX_OBSERVE_VALUE(angle);

	if ((angle & 0x400) == 0)
	{
		goto FirstOrThirdQuadrant;
	}

	matrixCos = (s16)trigOrSin;
	trigOrSin = (s16)((u32)trigOrSin >> 16);
	if ((angle & 0x800) != 0)
	{
		goto NegateMatrixSin;
	}
	matrixCos = CTR_MipsNegLo(matrixCos);
	goto RotationReady;

FirstOrThirdQuadrant:
	matrixCos = (s16)((u32)trigOrSin >> 16);
	trigOrSin = CTR_MipsSll(trigOrSin, 16);
	trigOrSin = CTR_MipsSra(trigOrSin, 16);
	if ((angle & 0x800) == 0)
	{
		goto RotationReady;
	}
	matrixCos = CTR_MipsNegLo(matrixCos);

NegateMatrixSin:
	trigOrSin = CTR_MipsNegLo(trigOrSin);

RotationReady:

	*(CtrPackedU32 *)&wake->matrix.m[0][0] = (u32)matrixCos;
	*(CtrPackedU32 *)&wake->matrix.m[0][2] = (u32)trigOrSin;
	*(CtrPackedU32 *)&wake->matrix.m[1][1] = VEH_PHYS_FORCE_WAKE_INITIAL_SCALE;
	*(CtrPackedU32 *)&wake->matrix.m[2][0] = (u32)CTR_MipsNegLo(trigOrSin);
	wake->matrix.m[2][2] = (s16)matrixCos;
}

static inline void VehPhysForce_TranslateMatrix_UpdateWake(struct Instance *inst, struct Driver *d)
{
	struct Instance *wake;
	int wakeY = inst->matrix.t[1];
	u16 visibleWakeScale;

	if (wakeY >= VEH_PHYS_FORCE_WAKE_WATERLINE_Y)
	{
		goto HIDE_WAKE;
	}

	if (wakeY < VEH_PHYS_FORCE_WAKE_VISIBLE_MIN_Y)
	{
		goto RELOAD_AND_HIDE_WAKE;
	}
	if ((inst->flags & SPLIT_LINE) == 0)
	{
		goto RELOAD_AND_HIDE_WAKE;
	}

	wake = d->wakeInst;
	if (wake == NULL)
	{
		return;
	}

	wake->flags &= ~HIDE_MODEL;
	wake->depthBiasNormal = (u8)CTR_MipsAddLo(inst->depthBiasNormal, 1);
	wake->depthBiasSecondary = (u8)CTR_MipsSubLo(inst->depthBiasSecondary, 1);

	wake->matrix.t[0] = inst->matrix.t[0];
	wake->matrix.t[1] = VEH_PHYS_FORCE_WAKE_WATERLINE_Y;
	wake->matrix.t[2] = inst->matrix.t[2];

	VehPhysForce_TranslateMatrix_SetWakeRotation(wake, d);

	if (d->wakeScale == 0)
	{
		d->wakeScale = VEH_PHYS_FORCE_WAKE_INITIAL_SCALE;

		if (GAME_TRACKER->numPlyrCurrGame < 2)
		{
			if ((VehPhysForce_TranslateMatrix_Abs(d->speed) > VEH_PHYS_FORCE_WAKE_PARTICLE_SPEED_MIN) &&
			    (d->posPrev.y > VEH_PHYS_FORCE_WAKE_PARTICLE_PREV_Y_MIN))
			{
				int i;

				i = VEH_PHYS_FORCE_WAKE_BURST_PARTICLE_COUNT;
				do
				{
					VehPhysForce_TranslateMatrix_SpawnWakeParticle(d);
					i--;
				} while (i != 0);
			}
		}
	}
	else if (GAME_TRACKER->numPlyrCurrGame < 2)
	{
		if (VehPhysForce_TranslateMatrix_Abs(d->speed) > VEH_PHYS_FORCE_WAKE_PARTICLE_SPEED_MIN)
		{
			VehPhysForce_TranslateMatrix_SpawnWakeParticle(d);
		}
	}

	wake->scale.x = (s16)VehPhysForce_ScaleValue(d, d->wakeScale);
	visibleWakeScale = (u16)d->wakeScale;
	CTR_PSX_OBSERVE_VALUE(visibleWakeScale);
	wake->scale.z = (s16)VehPhysForce_ScaleValue(d, visibleWakeScale);
	return;

RELOAD_AND_HIDE_WAKE:
	wakeY = inst->matrix.t[1];

HIDE_WAKE:
	if (wakeY > VEH_PHYS_FORCE_WAKE_WATERLINE_Y)
	{
		inst->flags &= ~SPLIT_LINE;
	}

	{
		struct Instance *hiddenWake = d->wakeInst;

		if (hiddenWake != NULL)
		{
			hiddenWake->flags |= HIDE_MODEL;
			d->wakeScale = 0;
			hiddenWake->scale.x = 0;
			hiddenWake->scale.z = d->wakeScale;
		}
	}
}

float VehPhysForce_GetKartScale(const struct Driver *driver)
{
	if ((driver->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
	{
		return VEH_PHYS_FORCE_KART_SCALE_NORMAL;
	}
	return driver->kartScale != 0 ? driver->kartScale : VEH_PHYS_FORCE_KART_SCALE_NORMAL;
}

int VehPhysForce_ScaleValue(const struct Driver *driver, int value)
{
	return (int)(value * VehPhysForce_GetKartScale(driver));
}

void VehPhysForce_ScaleMatrix(const struct Driver *driver, MATRIX *matrix)
{
	for (int row = 0; row < 3; row++)
	{
		for (int column = 0; column < 3; column++)
		{
			matrix->m[row][column] = (s16)VehPhysForce_ScaleValue(driver, matrix->m[row][column]);
		}
	}
}

void VehPhysForce_ScaleParticle(const struct Driver *driver, struct Particle *particle)
{
	const bool line = (particle->flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE) != 0;
	struct ParticleOscillator *oscillator = particle->oscillator;
	for (int i = 0; i < (int)len(particle->axis); i++)
	{
		const bool spatial = i <= PARTICLE_AXIS_POS_Z ||
		    i == PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE || i == PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y ||
		    (line && (i == PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X || i == PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z));
		if (spatial)
		{
			struct ParticleAxis *axis = &particle->axis[i];
			axis->startVal = VehPhysForce_ScaleValue(driver, axis->startVal);
			axis->velocity = (s16)VehPhysForce_ScaleValue(driver, axis->velocity);
			axis->accel = (s16)VehPhysForce_ScaleValue(driver, axis->accel);
		}
		if ((particle->flagsOscillatorAxis & (1 << i)) != 0 && oscillator != NULL)
		{
			if (spatial)
			{
				oscillator->previousValue = (s16)VehPhysForce_ScaleValue(driver, oscillator->previousValue);
				oscillator->scale = (u16)VehPhysForce_ScaleValue(driver, oscillator->scale);
				oscillator->offset = (s16)VehPhysForce_ScaleValue(driver, oscillator->offset);
				oscillator->min = (s16)VehPhysForce_ScaleValue(driver, oscillator->min);
				oscillator->max = (s16)VehPhysForce_ScaleValue(driver, oscillator->max);
			}
			oscillator = oscillator->next;
		}
	}
}

void VehPhysForce_TranslateMatrix(struct Thread *thread, struct Driver *driver)
{
	struct Instance *inst = thread->inst;

	{
		int jumpHeightCurr;
		int targetSquish;

		if (driver->kartState == KS_WARP_PAD)
		{
			goto SQUASH_STRETCH_DONE;
		}

		if ((driver->kartState == KS_MASK_GRABBED) && ((driver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0))
		{
			inst->scale.y = (s16)VehPhysForce_ScaleValue(
			    driver, CTR_MipsAddLo(driver->jumpSquishStretch, VEH_PHYS_FORCE_KART_SCALE_BASE));

			targetSquish = CTR_MipsSubLo(VEH_PHYS_FORCE_KART_SCALE_BASE,
			                             VehPhysForce_TranslateMatrix_Div256TowardZero(driver->jumpSquishStretch * VEH_PHYS_FORCE_MASK_GRAB_SCALE_XZ_FACTOR));
			if (targetSquish < VEH_PHYS_FORCE_MIN_SQUASH_XZ_SCALE)
			{
				targetSquish = VEH_PHYS_FORCE_MIN_SQUASH_XZ_SCALE;
			}

			inst->scale.x = (s16)VehPhysForce_ScaleValue(driver, targetSquish);
			inst->scale.z = inst->scale.x;
			goto SQUASH_STRETCH_DONE;
		}

		jumpHeightCurr = driver->jumpHeightCurr;
		targetSquish = VEH_PHYS_FORCE_TARGET_SQUISH_DEFAULT;

		if ((driver->actionsFlagSet & ACTION_JUMP_STARTED) == 0)
		{
			register int currentSquish CTR_PSX_REGISTER("$4");
			register int computedTargetSquish CTR_PSX_REGISTER("$16");
			register int weightedJumpHeight CTR_PSX_REGISTER("$3");

			currentSquish = driver->jumpSquishStretch2;
			CTR_PSX_OBSERVE_VALUE(currentSquish);
			weightedJumpHeight = jumpHeightCurr * 7;
			computedTargetSquish = CTR_MipsSubLo(currentSquish, CTR_MipsSra(CTR_MipsAddLo(currentSquish * 9, weightedJumpHeight), 4));
			CTR_PSX_OBSERVE_VALUE(computedTargetSquish);
			targetSquish = CTR_MipsSll(computedTargetSquish, 2);

			{
				int targetSquishMagnitude = targetSquish;

				if (targetSquishMagnitude < 0)
				{
					CTR_PSX_FORGET_VALUE(targetSquishMagnitude);
					targetSquishMagnitude = CTR_MipsNegLo(targetSquishMagnitude);
				}
				if (targetSquishMagnitude < VEH_PHYS_FORCE_SQUISH_DEADBAND)
				{
					targetSquish = 0;
				}
			}

			if (((driver->actionsFlagSet | driver->actionsFlagSetPrevFrame) & ACTION_STARTED_TOUCH_GROUND) != 0)
			{
				if (targetSquish < VEH_PHYS_FORCE_LANDING_SQUISH_MIN)
				{
					targetSquish = VEH_PHYS_FORCE_LANDING_SQUISH_MIN;
				}
			}
			else if (targetSquish < VEH_PHYS_FORCE_AIR_SQUISH_MIN)
			{
				targetSquish = VEH_PHYS_FORCE_AIR_SQUISH_MIN;
			}

			if (targetSquish > VEH_PHYS_FORCE_SQUISH_MAX)
			{
				targetSquish = VEH_PHYS_FORCE_SQUISH_MAX;
			}
		}

		if ((driver->hazardTimer > 0) && ((driver->hazardTimer & VEH_PHYS_FORCE_HAZARD_BLINK_MASK) == 0) &&
		    (targetSquish > VEH_PHYS_FORCE_TARGET_SQUISH_DEFAULT))
		{
			targetSquish = VEH_PHYS_FORCE_TARGET_SQUISH_DEFAULT;
		}

		if (((driver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0) && (jumpHeightCurr < 0))
		{
			int mapped = VehCalc_MapToRange(CTR_MipsNegLo(jumpHeightCurr), 0, VEH_PHYS_FORCE_FALL_STRETCH_HEIGHT_MAX, VEH_PHYS_FORCE_FALL_STRETCH_MIN,
			                                VEH_PHYS_FORCE_FALL_STRETCH_MAX);

			if (targetSquish < mapped)
			{
				targetSquish = mapped;
			}

			driver->jumpSquishStretch2 = jumpHeightCurr;
		}

		if ((driver->instTntRecv != NULL) && (driver->instTntRecv->scale.y < VEH_PHYS_FORCE_TNT_SCALE_Y_THRESHOLD))
		{
			targetSquish = CTR_MipsAddLo(targetSquish, CTR_MipsSll(CTR_MipsSubLo(driver->instTntRecv->scale.y, VEH_PHYS_FORCE_TNT_SCALE_Y_BASE), 1));
		}

		{
			int currentSquish = driver->jumpSquishStretch;
			int absTargetSquish = targetSquish;

			if (absTargetSquish < 0)
			{
				CTR_PSX_FORGET_VALUE(absTargetSquish);
				absTargetSquish = CTR_MipsNegLo(absTargetSquish);
			}
			if (currentSquish < 0)
			{
				currentSquish = CTR_MipsNegLo(currentSquish);
			}
			if (currentSquish < absTargetSquish)
			{
				driver->jumpSquishStretch = targetSquish;
			}
		}

		driver->jumpSquishStretch = VehCalc_InterpBySpeed(driver->jumpSquishStretch, VEH_PHYS_FORCE_SQUISH_INTERP_SPEED, 0);
		driver->jumpSquishStretch2 = (s16)CTR_MipsSra(CTR_MipsAddLo(driver->jumpSquishStretch2 * 9, jumpHeightCurr * 7), 4);

		if (driver->squishTimer != 0)
		{
			inst->scale.y = 0;
		}
		else if (inst->scale.y == 0)
		{
			if (driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
			{
				OtherFX_Play_Echo(VEH_PHYS_FORCE_SQUISH_RESTORE_SFX, 1, (driver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
			}

			inst->scale.y = (s16)VehPhysForce_ScaleValue(
			    driver, CTR_MipsAddLo(driver->jumpSquishStretch, VEH_PHYS_FORCE_KART_SCALE_BASE));
			driver->matrixArray = BAKED_GTE_MATRIX_SQUISH_RECOVER;
			driver->matrixIndex = 0;
		}
		else
		{
			int targetScaleY = CTR_MipsAddLo(driver->jumpSquishStretch, VEH_PHYS_FORCE_KART_SCALE_BASE);
			inst->scale.y = VehCalc_InterpBySpeed(inst->scale.y, VEH_PHYS_FORCE_SQUISH_SCALE_INTERP_SPEED,
			                                      VehPhysForce_ScaleValue(driver, targetScaleY));
		}

		{
			int targetScaleXZ = CTR_MipsSubLo(VEH_PHYS_FORCE_KART_SCALE_BASE,
			    VehPhysForce_TranslateMatrix_Div256TowardZero(driver->jumpSquishStretch * VEH_PHYS_FORCE_SQUISH_SCALE_XZ_FACTOR));
			targetScaleXZ = VehPhysForce_ScaleValue(driver, targetScaleXZ);
			inst->scale.x = VehCalc_InterpBySpeed(inst->scale.x, VEH_PHYS_FORCE_SQUISH_SCALE_INTERP_SPEED, targetScaleXZ);
			inst->scale.z = VehCalc_InterpBySpeed(inst->scale.z, VEH_PHYS_FORCE_SQUISH_SCALE_INTERP_SPEED, targetScaleXZ);
		}
	}

SQUASH_STRETCH_DONE:
	VehPhysForce_RotAxisAngle(&driver->matrixFacingDir, CTR_VECTOR_DATA(&(driver->AxisAngle2_normalVec)), driver->rotCurr.y);
	VehPhysForce_TranslateMatrix_UpdateMatrixAnimation(driver);
	VehPhysForce_TranslateMatrix_UpdateInstanceMatrix(inst, driver);
	VehPhysForce_TranslateMatrix_UpdateWake(inst, driver);
}

// NOTE(aalhendi): Native helpers preserve R3000 wraparound semantics without C
// overflow UB. The direct operators keep GCC 2.8.1's retail signed-MULT shape.
#define VEH_ROT_MUL_LO(lhs, rhs)       CTR_MipsMulLo((lhs), (rhs))
#define VEH_ROT_ADD_LO(lhs, rhs)       CTR_MipsAddLo((lhs), (rhs))
#define VEH_ROT_SUB_LO(lhs, rhs)       CTR_MipsSubLo((lhs), (rhs))
#define VEH_ROT_NEG_LO(value)          CTR_MipsNegLo(value)
#define VEH_ROT_SRA(value, shift)      CTR_MipsSra((value), (shift))
#define VEH_ROT_DIV(dividend, divisor) CTR_MipsDiv((dividend), (divisor))

void VehPhysForce_RotAxisAngle(MATRIX *m, s16 *normVec, s32 angle)
{
	register s16 *normalVector CTR_PSX_REGISTER("$24");
	register MATRIX *matrix CTR_PSX_REGISTER("$15");
	register u32 normalXBits CTR_PSX_REGISTER("$2");
	register u32 normalYBits CTR_PSX_REGISTER("$3");
	register u32 normalZBits CTR_PSX_REGISTER("$4");
	register s32 normalX CTR_PSX_REGISTER("$11");
	register s32 normalY CTR_PSX_REGISTER("$5");
	register s32 normalZ CTR_PSX_REGISTER("$10");
	register u32 packedTrig CTR_PSX_REGISTER("$7");
	register s32 trigCos CTR_PSX_REGISTER("$8");
	register const CtrPackedU32 *trigBase CTR_PSX_REGISTER("$3");
	s32 trigLowTemp;

	normalVector = normVec;
	normalXBits = (u16)normalVector[0];
	CTR_PSX_KEEP_VALUE(normalVector);
	matrix = m;
	matrix->m[0][1] = (s16)normalXBits;
	normalX = (s16)normalXBits;
	normalYBits = (u16)normalVector[1];
	matrix->m[1][1] = (s16)normalYBits;
	CTR_PSX_OBSERVE_MEMORY(matrix->m[1][1]);
	normalY = (s16)normalYBits;
	CTR_PSX_CLOBBER("$3");
	// NOTE(aalhendi): Retail overlaps the trig-table page setup with the normal-Z
	// load and sign extension. Keep that scheduling detail out of native builds.
	normalZBits = (u16)normalVector[2];
	CTR_PSX_ADD_SYMBOL_LOW(trigBase, trigBase, VEH_TRIG_APPROX_ASM_NAME, (const CtrPackedU32 *)data.trigApprox);
	normalZ = CTR_MipsSra(CTR_MipsSll((s32)normalZBits, 16), 16);
	matrix->m[2][1] = (s16)normalZBits;
	packedTrig = trigBase[ANG_MODULO_HALF_PI(angle)];

	if (IS_ANG_FIRST_OR_THIRD_QUADRANT(angle))
	{
		trigCos = CTR_MipsSra((s32)packedTrig, 16);
		CTR_PSX_KEEP_VALUE(trigCos);
		packedTrig = (u32)CTR_MipsSra(CTR_MipsSll((s32)packedTrig, 16), 16);
		if (!IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
		{
			goto TrigReady;
		}
		trigCos = CTR_MipsNegLo(trigCos);
	}
	else
	{
		trigLowTemp = CTR_MipsSll((s32)packedTrig, 16);
		CTR_PSX_KEEP_VALUE(trigLowTemp);
		CTR_PSX_SHIFT_RIGHT_ARITHMETIC(trigCos, trigLowTemp, 16);
		CTR_PSX_KEEP_VALUE(trigCos);
		packedTrig = (u32)CTR_MipsSra((s32)packedTrig, 16);
		if (!IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
		{
			trigCos = CTR_MipsNegLo(trigCos);
			goto TrigReady;
		}
	}
	packedTrig = (u32)CTR_MipsNegLo((s32)packedTrig);

TrigReady:
{
	s32 normalXSq = VEH_ROT_MUL_LO(normalX, normalX);
	register s32 normalZSq CTR_PSX_REGISTER("$4") = VEH_ROT_MUL_LO(normalZ, normalZ);
	s32 crossXZ = VEH_ROT_MUL_LO(normalX, VEH_ROT_NEG_LO(normalZ));
	s32 denom = VEH_ROT_ADD_LO(normalXSq, normalZSq);
	s32 scaledSinY = VEH_ROT_SRA(VEH_ROT_MUL_LO((s32)packedTrig, normalY), 12);
	s32 scaledCosY = VEH_ROT_SRA(VEH_ROT_MUL_LO(trigCos, normalY), 12);
	register s32 outX CTR_PSX_REGISTER("$11");
	register s32 outZ CTR_PSX_REGISTER("$10");
	s32 outY;
	register int shift CTR_PSX_REGISTER("$3");
	s16 generatedColumn[3];

	MTC2((u32)denom, 30);
	CTR_PSX_GTE_PIPELINE_DELAY();
	shift = (s32)MFC2(31);

	if (denom == 0)
	{
		s32 dot;

		dot = VEH_ROT_ADD_LO(VEH_ROT_MUL_LO((s32)packedTrig, normalX), VEH_ROT_MUL_LO(trigCos, normalZ));

		if (normalVector[1] < 0)
		{
			scaledSinY = VEH_ROT_NEG_LO(scaledSinY);
		}

		outY = VEH_ROT_SRA(VEH_ROT_NEG_LO(dot), 12);
		outX = scaledSinY;
		outZ = scaledCosY;
	}
	else
	{
		s32 sinRemainder;
		s32 cosRemainder;
		s32 divX;
		s32 divZ;
		s32 dot;

		shift = VEH_ROT_SUB_LO(0x14, shift);
		if (shift > 0)
		{
			normalXSq = VEH_ROT_SRA(normalXSq, shift);
			normalZSq = VEH_ROT_SRA(normalZSq, shift);
			crossXZ = VEH_ROT_SRA(crossXZ, shift);
			denom = VEH_ROT_SRA(denom, shift);
		}

		sinRemainder = VEH_ROT_SUB_LO((s32)packedTrig, scaledSinY);
		cosRemainder = VEH_ROT_SUB_LO(trigCos, scaledCosY);
		divX = VEH_ROT_DIV(VEH_ROT_ADD_LO(VEH_ROT_MUL_LO(sinRemainder, normalZSq), VEH_ROT_MUL_LO(cosRemainder, crossXZ)), denom);
		divZ = VEH_ROT_DIV(VEH_ROT_ADD_LO(VEH_ROT_MUL_LO(sinRemainder, crossXZ), VEH_ROT_MUL_LO(cosRemainder, normalXSq)), denom);
		CTR_PSX_CLOBBER("$12");
		CTR_PSX_CLOBBER("$9");
		dot = VEH_ROT_ADD_LO(VEH_ROT_MUL_LO((s32)packedTrig, normalX), VEH_ROT_MUL_LO(trigCos, normalZ));

		outX = VEH_ROT_ADD_LO(scaledSinY, divX);
		outY = VEH_ROT_SRA(VEH_ROT_NEG_LO(dot), 12);
		outZ = VEH_ROT_ADD_LO(scaledCosY, divZ);
	}

	generatedColumn[0] = (s16)outX;
	matrix->m[0][2] = generatedColumn[0];
	generatedColumn[1] = (s16)outY;
	matrix->m[1][2] = generatedColumn[1];
	generatedColumn[2] = (s16)outZ;
	matrix->m[2][2] = generatedColumn[2];

	// NOTE(aalhendi): Retail uses GTE OP(sf=12) with the normal and generated
	// columns to derive the first matrix column.
	{
		register s32 gteX CTR_PSX_REGISTER("$12");
		register s32 gteY CTR_PSX_REGISTER("$13");
		register s32 gteZ CTR_PSX_REGISTER("$14");

		gteX = normalVector[0];
		gteY = normalVector[1];
		CTC2((u32)gteX, 0);
		gteZ = normalVector[2];
		CTC2((u32)gteY, 2);
		CTC2((u32)gteZ, 4);

		CTR_PSX_LOAD_SIGNED_HALF(gteX, generatedColumn, 0, generatedColumn[0]);
		CTR_PSX_LOAD_SIGNED_HALF(gteY, generatedColumn, 2, generatedColumn[1]);
		CTR_PSX_LOAD_SIGNED_HALF(gteZ, generatedColumn, 4, generatedColumn[2]);
		MTC2((u32)gteX, 9);
		MTC2((u32)gteY, 10);
		MTC2((u32)gteZ, 11);
		CTR_PSX_GTE_PIPELINE_DELAY();
		gte_op12();

		gteX = MFC2_S(25);
		gteY = MFC2_S(26);
		gteZ = MFC2_S(27);
		matrix->m[0][0] = (s16)gteX;
		matrix->m[1][0] = (s16)gteY;
		matrix->m[2][0] = (s16)gteZ;
	}
}
}

#undef VEH_ROT_DIV
#undef VEH_ROT_SRA
#undef VEH_ROT_NEG_LO
#undef VEH_ROT_SUB_LO
#undef VEH_ROT_ADD_LO
#undef VEH_ROT_MUL_LO

void VehPhysForce_CounterSteer(struct Driver *driver)
{
	int speedApprox;
	register u32 counterSteerRatio CTR_PSX_REGISTER("$2");
	u32 counterSteerValue;
	int angleLimit;
	int angle;
	int sine;

	speedApprox = driver->speedApprox;
	driver->accel.x = 0;
	driver->accel.y = 0;
	CTR_PSX_OBSERVE_MEMORY(driver->accel.y);
	if (speedApprox < 0)
	{
		speedApprox = CTR_MipsNegLo(speedApprox);
	}
	driver->accel.z = 0;

	if (speedApprox <= FP8(3))
	{
		return;
	}
	if (driver->kartState == KS_CRASHING)
	{
		return;
	}

	if ((driver->actionsFlagSet & ACTION_WARP) != 0)
	{
		return;
	}
	if (driver->wallRubTimer != 0)
	{
		return;
	}
	if ((driver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		return;
	}

	counterSteerRatio = driver->terrainMeta1->counterSteerRatio;
	if (counterSteerRatio == 0)
	{
		return;
	}
	counterSteerValue = (u32)-8000;
	counterSteerValue = CTR_MipsSra(CTR_MipsMulLo(counterSteerRatio, counterSteerValue), 8);

	angleLimit = (u8)driver->const_ModelTurnCounterSteerStrength;
	angle = CTR_MipsSubLo(driver->turnAngleCurr, driver->turnAnglePrev);
	if (angle > angleLimit)
	{
		angle = angleLimit;
	}
	if (angle < CTR_MipsNegLo(angleLimit))
	{
		angle = CTR_MipsNegLo(angleLimit);
	}

	sine = (s32)VEH_TRIG_APPROX(ANG_MODULO_HALF_PI(angle));
	if ((angle & ANG_QUADRANT_BIT) == 0)
	{
		sine = CTR_MipsSll(sine, 16);
	}
	sine = CTR_MipsSra(sine, 16);
	if ((angle & ANG_SIGN_BIT) != 0)
	{
		sine = CTR_MipsNegLo(sine);
	}

	// NOTE(aalhendi): The slope setup leaves matrixMovingDir in the GTE
	// rotation registers; retail transforms the counter-steer impulse in place.
	counterSteerValue = CTR_MipsSra(CTR_MipsMulLo(counterSteerValue, sine), 12);
	MTC2((u32)(u16)counterSteerValue, 0);
	MTC2(0, 1);
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_mvmva(1, 0, 0, 3, 0);
	CTR_PSX_STORE_COP2_HALF_DEAD_SCRATCH(driver, offsetof(struct Driver, accel.x), 25, "$7");
	CTR_PSX_STORE_COP2_HALF_DEAD_SCRATCH(driver, offsetof(struct Driver, accel.y), 26, "$7");
	CTR_PSX_STORE_COP2_HALF_DEAD_SCRATCH(driver, offsetof(struct Driver, accel.z), 27, "$7");
}
