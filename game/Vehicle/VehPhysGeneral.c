#include <common.h>

// byte budget: 1996
// current size: 1824

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005fc8c-0x80060458.
void VehPhysGeneral_PhysAngular(struct Thread *thread, struct Driver *driver)
{
	int speedApprox;
	int elapsedTimeMS;
	int classSpeed_original;
	int driverSpeed;
	u32 destinedRot;
	int classSpeed_halved;
	struct Terrain *terrain;
	int rotCurrW_original;
	int rotCurrWAbs;
	u16 angle;
	int turnResistMinBitshift;
	int turnResistMaxBitshift;
	int driftAngleCurr_Final;
	int turnResistMax;
	int turnResistMin;
	u32 actionsFlagSet;
	b32 interpLessThanOriginal;
	b32 wInterpLessThanZero;
	s16 forwardDir;
	int rotCurrW_interp;
	s8 simpTurnState;
	s16 driftAngleCurr_og;

	PhysLerpRot(driver, 0);

	elapsedTimeMS = sdata->gGT->elapsedTimeMS;
	actionsFlagSet = driver->actionsFlagSet;
	forwardDir = driver->forwardDir;
	simpTurnState = driver->simpTurnState;
	speedApprox = (int)driver->speedApprox;
	rotCurrW_interp = CTR_MipsSll(simpTurnState, 8);
	if (speedApprox < 1)
	{
		if (driver->baseSpeed < 0)
		{
			forwardDir = -1;
			driver->forwardDir = -1;
		}
		if (-1 < speedApprox)
		{
			goto LAB_8005fd74;
		}
	}
	else
	{
	LAB_8005fd74:
		if (-1 < driver->baseSpeed)
		{
			forwardDir = 1;
			driver->forwardDir = 1;
		}
	}
	if (forwardDir < 0)
	{
		rotCurrW_interp = CTR_MipsNegLo(CTR_MipsSll(simpTurnState, 8));
		actionsFlagSet ^= ACTION_STEER_LEFT;
	}
	if (speedApprox < 0)
	{
		speedApprox = CTR_MipsNegLo(speedApprox);
	}
	if (((actionsFlagSet & ACTION_TOUCH_GROUND) != 0) && ((driver->stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD_MASK) == 0))
	{
		rotCurrW_interp = VehCalc_MapToRange(speedApprox, 0x10, 0x300, 0, rotCurrW_interp);
	}
	terrain = driver->terrainMeta1;
	rotCurrW_original = (int)driver->rotationSpinRate;
	if (rotCurrW_interp == 0)
	{
		int rate =
		    CTR_MipsSra(CTR_MipsMulLo(CTR_MipsAddLo(driver->const_TurnInputDelay, CTR_MipsMulLo((s8)driver->turnConst, 0x32)), terrain->turnResponseScale), 8);

		rotCurrW_interp = VehCalc_InterpBySpeed(rotCurrW_original, rate, 0);

		forwardDir = (s16)rotCurrW_interp;
	}
	else
	{
		wInterpLessThanZero = rotCurrW_interp < 0;
		if (wInterpLessThanZero)
		{
			rotCurrW_interp = CTR_MipsNegLo(rotCurrW_interp);
			rotCurrW_original = CTR_MipsNegLo(rotCurrW_original);
		}
		if (rotCurrW_original < rotCurrW_interp)
		{
			int rate = CTR_MipsSra(
			    CTR_MipsMulLo(CTR_MipsAddLo(driver->const_TurnInputDelay, CTR_MipsMulLo((s8)driver->turnConst, 100)), terrain->turnResponseScale), 8);
			rotCurrW_original = CTR_MipsAddLo(rotCurrW_original, rate);

			interpLessThanOriginal = rotCurrW_interp < rotCurrW_original;
		LAB_8005fee4:
			if (interpLessThanOriginal)
			{
				rotCurrW_original = rotCurrW_interp;
			}
		}
		else if (rotCurrW_interp < rotCurrW_original)
		{
			int rate = CTR_MipsSra(
			    CTR_MipsMulLo(CTR_MipsAddLo(driver->const_TurnInputDelay, CTR_MipsMulLo((s8)driver->turnConst, 50)), terrain->turnResponseScale), 8);
			rotCurrW_original = CTR_MipsSubLo(rotCurrW_original, rate);

			interpLessThanOriginal = rotCurrW_original < rotCurrW_interp;
			goto LAB_8005fee4;
		}
		forwardDir = (s16)rotCurrW_original;
		if (wInterpLessThanZero)
		{
			forwardDir = (s16)CTR_MipsNegLo(forwardDir);
		}
	}

	rotCurrW_original = (int)forwardDir;
	driver->rotationSpinRate = forwardDir;

	rotCurrW_interp = (int)driver->timeUntilDriftSpinout;
	if (rotCurrW_interp != 0)
	{
		classSpeed_halved = CTR_MipsSubLo(rotCurrW_interp, elapsedTimeMS);
		rotCurrW_interp = VehCalc_MapToRange(rotCurrW_interp, 0, 0x140, 0, (int)driver->previousFrameMultDrift);
		rotCurrW_original = CTR_MipsAddLo(rotCurrW_original, rotCurrW_interp);
		if (classSpeed_halved < 0)
		{
			classSpeed_halved = 0;
		}
		driver->timeUntilDriftSpinout = (s16)classSpeed_halved;
	}

	classSpeed_halved = CTR_MipsSll((u16)driver->const_Speed_ClassStat, 0x10);
	classSpeed_original = CTR_MipsSra(classSpeed_halved, 0x10);
	turnResistMax = CTR_MipsMulLo((u8)driver->const_turnResistMax, classSpeed_original);
	turnResistMin = CTR_MipsMulLo((u8)driver->const_turnResistMin, classSpeed_original);
	forwardDir = driver->turnAngleLerpVel;
	rotCurrW_interp = (int)driver->const_modelRotVelMax;
	turnResistMaxBitshift = CTR_MipsSra(turnResistMax, 8);
	turnResistMinBitshift = CTR_MipsSra(turnResistMin, 8);

	// gas and brake together
	if ((actionsFlagSet & ACTION_BRAKE_WITH_ACCEL) != 0)
	{
		turnResistMaxBitshift = CTR_MipsSra(turnResistMax, 9);
		if (0x300 < speedApprox)
		{
			// driver is leaving skids
			driver->actionsFlagSet |= ACTION_BACK_SKID;
		}
		turnResistMinBitshift = CTR_MipsSra(turnResistMin, 9);
		if (driver->baseSpeed == 0)
		{
			rotCurrW_interp = (int)driver->const_modelRotVelMin;
		}
		else
		{
			turnResistMax = (int)driver->speed;
			if (turnResistMax < 0)
			{
				turnResistMax = CTR_MipsNegLo(turnResistMax);
			}
			// Rotating the model to exaggerate the steering animation
			// only do this if driver speed is more than 0x300
			rotCurrW_interp =
			    VehCalc_MapToRange(turnResistMax, 0x300, CTR_MipsSra(classSpeed_halved, 0x11), (int)driver->const_modelRotVelMin, rotCurrW_interp);
		}
	}
	driverSpeed = (int)driver->speed;
	if (driverSpeed < 0)
	{
		driverSpeed = CTR_MipsNegLo(driverSpeed);
	}

	// this prevents you from steering sharp at low speeds
	turnResistMin = CTR_MipsSll(CTR_MipsAddLo((u8)driver->const_TurnRate, CTR_MipsSll((s8)driver->turnConst, 1) / 5), 8);
	turnResistMax = VehCalc_MapToRange(driverSpeed, turnResistMinBitshift, turnResistMaxBitshift, turnResistMin, 0);

	classSpeed_halved = 0;
	if (turnResistMinBitshift <= speedApprox)
	{
		rotCurrWAbs = rotCurrW_original;
		if (rotCurrW_original < 0)
		{
			rotCurrWAbs = CTR_MipsNegLo(rotCurrW_original);
		}
		if (turnResistMax < rotCurrWAbs)
		{
			classSpeed_halved = (int)driver->fireSpeed;
			if (classSpeed_halved < 0)
			{
				classSpeed_halved = CTR_MipsNegLo(classSpeed_halved);
			}
			classSpeed_halved = VehCalc_MapToRange(classSpeed_halved, turnResistMinBitshift, turnResistMaxBitshift, 0, rotCurrW_interp);
			classSpeed_halved = VehCalc_MapToRange(rotCurrWAbs, turnResistMax, turnResistMin, 0, classSpeed_halved);
			if (rotCurrW_original < 0)
			{
				classSpeed_halved = CTR_MipsNegLo(classSpeed_halved);
			}
		}
	}

	driftAngleCurr_og = driver->turnAngleCurr;

	// spins camera from side of driver, to back of driver,
	// when the drifting ends. "LerpToForwards"
	driver->turnAngleLerpVel = VehPhysGeneral_LerpToForwards(driver, (int)driftAngleCurr_og, (int)forwardDir, classSpeed_halved);

	classSpeed_halved = (int)(s16)driver->turnAngleLerpVel;

	if (terrain->turnAngleScale != 0x100)
	{
		classSpeed_halved = CTR_MipsSra(CTR_MipsMulLo(terrain->turnAngleScale, classSpeed_halved), 8);
	}
	driftAngleCurr_Final = CTR_MipsAddLo(driftAngleCurr_og, CTR_MipsSra(CTR_MipsMulLo(classSpeed_halved, elapsedTimeMS), 5));
	driver->turnAngleCurr = (s16)driftAngleCurr_Final;
	turnResistMinBitshift = rotCurrW_original;
	if ((0x2ff < speedApprox) && ((actionsFlagSet & ACTION_TOUCH_GROUND) != 0))
	{
		turnResistMaxBitshift = VehCalc_SteerAccel(driver->numFramesSpentSteering, (int)driver->const_SteerAccel_Stage2_FirstFrame,
		                                           (int)driver->const_SteerAccel_Stage2_FrameLength, (int)driver->const_SteerAccel_Stage4_FirstFrame,
		                                           (int)driver->const_SteerAccel_Stage1_MinSteer, (int)driver->const_SteerAccel_Stage1_MaxSteer);
		if (rotCurrW_original < 0)
		{
			turnResistMinBitshift = CTR_MipsNegLo(rotCurrW_original);
		}

		turnResistMinBitshift = CTR_MipsSra(CTR_MipsMulLo(driver->const_SteerAccelTurnVelScale, turnResistMinBitshift), 8);

		driver->numFramesSpentSteering = (s16)CTR_MipsAddLo((u16)driver->numFramesSpentSteering, 1);

		// the higher the value of turnResistMaxBitshift the more steering is "locked up"
		// try setting mov r3, xxxx at 80060170 for proof
		if (turnResistMinBitshift < turnResistMaxBitshift)
		{
			turnResistMaxBitshift = turnResistMinBitshift;
		}

		// steering left or right
		if ((actionsFlagSet & ACTION_STEER_LEFT) != 0)
		{
			turnResistMaxBitshift = CTR_MipsNegLo(turnResistMaxBitshift);
		}

		turnResistMax = (int)driver->const_SteerAccelTurnVelLimit;

		if ((rotCurrW_original < 1) ||
		    (turnResistMinBitshift = CTR_MipsNegLo(turnResistMax), turnResistMinBitshift <= CTR_MipsAddLo(rotCurrW_original, turnResistMaxBitshift)))
		{
			if (rotCurrW_original < 0)
			{
				turnResistMinBitshift = CTR_MipsAddLo(rotCurrW_original, turnResistMaxBitshift);
				if (turnResistMax < CTR_MipsAddLo(rotCurrW_original, turnResistMaxBitshift))
				{
					turnResistMinBitshift = turnResistMax;
				}
			}
			else
			{
				turnResistMinBitshift = CTR_MipsAddLo(rotCurrW_original, turnResistMaxBitshift);
			}
		}
	}
	turnResistMax = (int)driver->turnWobbleAngle;
	turnResistMaxBitshift = (int)driver->turnWobbleTimer;
	rotCurrW_original = (int)driver->turnWobbleVelocity;
	if (((terrain->flags & TERRAIN_FLAG_SKIP_TURN_ASSIST) == 0) && ((actionsFlagSet & ACTION_TOUCH_GROUND) != 0))
	{
		turnResistMin = driftAngleCurr_Final;
		if (driftAngleCurr_Final < 0)
		{
			turnResistMin = CTR_MipsNegLo(driftAngleCurr_Final);
		}
		if (CTR_MipsSra(CTR_MipsAddLo(CTR_MipsSll(rotCurrW_interp, 1), rotCurrW_interp), 2) < turnResistMin)
		{
			rotCurrW_interp = classSpeed_halved;
			if (classSpeed_halved < 0)
			{
				rotCurrW_interp = CTR_MipsNegLo(classSpeed_halved);
			}
			if (rotCurrW_interp < 3)
			{
				rotCurrW_interp = turnResistMax;
				if (turnResistMax < 0)
				{
					rotCurrW_interp = CTR_MipsNegLo(turnResistMax);
				}
				if (rotCurrW_interp < 10)
				{
					turnResistMaxBitshift = 8;
					rotCurrW_original = 0x14;
					if (driftAngleCurr_Final < 0)
					{
						rotCurrW_original = -0x14;
					}
				}
			}
			goto LAB_80060284;
		}
	}
	turnResistMaxBitshift = 0;
LAB_80060284:
	rotCurrW_interp = turnResistMax;
	if (turnResistMax < 0)
	{
		rotCurrW_interp = CTR_MipsNegLo(turnResistMax);
	}
	if (0x32 < rotCurrW_interp)
	{
		turnResistMaxBitshift = 0;
	}
	if (turnResistMaxBitshift == 0)
	{
		rotCurrW_original = 10;
		if (0 < turnResistMax)
		{
			rotCurrW_original = CTR_MipsNegLo(10);
		}
		rotCurrW_interp = rotCurrW_original;
		if (rotCurrW_original < 0)
		{
			rotCurrW_interp = CTR_MipsNegLo(rotCurrW_original);
		}
		rotCurrW_interp = VehCalc_InterpBySpeed(turnResistMax, rotCurrW_interp, 0);
		forwardDir = (s16)rotCurrW_interp;
	}
	else
	{
		turnResistMaxBitshift = CTR_MipsSubLo(turnResistMaxBitshift, 1);
		forwardDir = (s16)CTR_MipsAddLo(driver->turnWobbleAngle, rotCurrW_original);
	}
	angle = driver->angle;
	driver->turnWobbleTimer = (s16)turnResistMaxBitshift;
	driver->turnWobbleAngle = forwardDir;
	driver->turnWobbleVelocity = (s16)rotCurrW_original;
	rotCurrW_interp = VehCalc_MapToRange(speedApprox, 0, 0x600, classSpeed_halved, 0);
	rotCurrW_original = CTR_MipsSra(CTR_MipsMulLo(rotCurrW_interp, elapsedTimeMS), 5);
	rotCurrW_interp = rotCurrW_original;
	if (rotCurrW_original < 0)
	{
		rotCurrW_interp = CTR_MipsNegLo(rotCurrW_original);
	}
	if (1 < rotCurrW_interp)
	{
		angle = (u16)(CTR_MipsSubLo(angle, rotCurrW_original) & 0xfff);
	}
	driver->ampTurnState = (s16)turnResistMinBitshift;

	angle = (u16)(CTR_MipsAddLo(angle, CTR_MipsSra(CTR_MipsMulLo(turnResistMinBitshift, elapsedTimeMS), 0xd)) & 0xfff);
	driver->angle = angle;

	(driver->rotCurr).y = (s16)CTR_MipsAddLo(CTR_MipsAddLo(angle, (s16)driftAngleCurr_Final), forwardDir);

	if (((actionsFlagSet & ACTION_ACCEL_PREVENTION) == 0) && (driver->accelTapCount < DRIVER_ACCEL_TAP_STEER_COUNT))
	{
		if (terrain->turnLeanScale != 0x100)
		{
			turnResistMinBitshift = CTR_MipsSra(CTR_MipsMulLo(turnResistMinBitshift, terrain->turnLeanScale), 8);
		}
	}
	else
	{
		turnResistMinBitshift = CTR_MipsSra(CTR_MipsMulLo(turnResistMinBitshift, 10), 8);
	}

	driver->axisRotationX = (s16)(CTR_MipsAddLo((u16)driver->axisRotationX, CTR_MipsSra(CTR_MipsMulLo(turnResistMinBitshift, elapsedTimeMS), 0xd)) & 0xfff);

	PhysTerrainSlope(driver);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80060458-0x80060488.
int VehPhysGeneral_LerpQuarterStrength(int current, int desired)
{
	if (desired != 0)
	{
		desired = CTR_MipsSra(desired, 2);

		if (desired == 0)
		{
			desired = 1;
		}
	}

	if (desired <= current)
	{
		current = desired;
	}

	return current;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80060488-0x800605a0.
int VehPhysGeneral_LerpToForwards(struct Driver *d, int currentAngle, int currentVelocity, int targetAngle)
{
	b32 mirrored = false;
	int desiredVelocity = 0;

	d->turnAngleLerpTarget = 0;
	if ((targetAngle < 0) || ((targetAngle == 0 && (currentAngle < 0))))
	{
		mirrored = true;
		currentAngle = CTR_MipsNegLo(currentAngle);
		currentVelocity = CTR_MipsNegLo(currentVelocity);
		targetAngle = CTR_MipsNegLo(targetAngle);
	}

	if (d->wallRubTimer != 0xf0)
	{
		if (targetAngle < currentAngle)
		{
			u32 lerpStrength;

			if (d->const_modelRotVelMax < currentAngle)
			{
				lerpStrength = CTR_MipsSubLo(CTR_MipsSll((u8)d->const_ModelTurnReturnStrength, 4), (u8)d->const_ModelTurnReturnStrength);
			}
			else
			{
				lerpStrength = (u8)d->const_ModelTurnReturnStrength;
			}
			desiredVelocity = VehPhysGeneral_LerpQuarterStrength(lerpStrength, CTR_MipsSubLo(currentAngle, targetAngle));
			desiredVelocity = CTR_MipsNegLo(desiredVelocity);
		}
		else
		{
			if (currentAngle < targetAngle)
			{
				if (currentAngle < 0)
				{
					desiredVelocity =
					    VehPhysGeneral_LerpQuarterStrength((u8)d->const_ModelTurnNegativeReturnStrength, CTR_MipsSubLo(targetAngle, currentAngle));
				}
				else
				{
					desiredVelocity = VehPhysGeneral_LerpQuarterStrength((u8)d->const_ModelTurnCounterSteerStrength, CTR_MipsSubLo(targetAngle, currentAngle));
					d->turnAngleLerpTarget = (s16)targetAngle;
				}
			}
		}
	}

	// Interpolate rotation by speed
	desiredVelocity = VehCalc_InterpBySpeed(currentVelocity, d->const_ModelTurnVelocityLerp, desiredVelocity);
	if (mirrored)
	{
		desiredVelocity = CTR_MipsNegLo(desiredVelocity);
	}
	return desiredVelocity;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800605a0-0x80060630.
int VehPhysGeneral_JumpGetVelY(s16 *normalVec, Vec3 *speedXYZ)
{
	int normalY = normalVec[1];
	int absNormalY = normalY;

	if (absNormalY < 0)
	{
		absNormalY = CTR_MipsNegLo(absNormalY);
	}

	if (absNormalY < 0x15)
	{
		return 0;
	}

	int dot = CTR_MipsAddLo(CTR_MipsMulLo(speedXYZ->x, normalVec[0]), CTR_MipsMulLo(speedXYZ->z, normalVec[2]));

	return CTR_MipsDiv(dot, normalY);
}

static int VehPhysGeneral_Jump_Abs(int value)
{
	return value < 0 ? CTR_MipsNegLo(value) : value;
}

static int VehPhysGeneral_Jump_Div2TowardZero(int value)
{
	return CTR_MipsSra(CTR_MipsAddLo(value, (u32)value >> 31), 1);
}

static int VehPhysGeneral_Jump_Div4TowardZero(int value)
{
	if (value < 0)
	{
		value = CTR_MipsAddLo(value, 3);
	}

	return CTR_MipsSra(value, 2);
}

static Vec3 VehPhysGeneral_Jump_RotateLoadedVector(s16 vx, s16 vy, s16 vz)
{
	Vec3 out;

	MTC2(CTR_PackS16Pair(vx, vy), 0);
	MTC2((u32)(u16)vz, 1);
	gte_mvmva(1, 0, 0, 3, 0);
	out.x = MFC2_S(25);
	out.y = MFC2_S(26);
	out.z = MFC2_S(27);

	return out;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80060630-0x80060f0c
void VehPhysGeneral_JumpAndFriction(struct Thread *t, struct Driver *d)
{
	(void)t;

	// Retail loads matrixMovingDir once, then reuses the same CP2 rotation regs
	// for every jump/friction impulse in this function.
	gte_SetRotMatrix(&d->matrixMovingDir);

	if ((d->kartState != KS_DRIFTING) && ((d->actionsFlagSet & ACTION_MASK_WEAPON) == 0) && (d->reserves == 0))
	{
		int ampTurn = VehPhysGeneral_Jump_Abs(CTR_MipsSra((s16)d->ampTurnState, 8));

		int turnDecrease = VehCalc_MapToRange(ampTurn, 0, (u8)d->const_BackwardTurnRate, 0, (int)d->const_TurnDecreaseRate);
		int baseSpeed = d->baseSpeed;
		int absBaseSpeed = VehPhysGeneral_Jump_Abs(baseSpeed);

		if (absBaseSpeed < turnDecrease)
		{
			turnDecrease = absBaseSpeed;
		}

		if (baseSpeed < 0)
		{
			d->baseSpeed = (s16)CTR_MipsAddLo((u16)d->baseSpeed, turnDecrease);
		}
		else
		{
			d->baseSpeed = (s16)CTR_MipsSubLo((u16)d->baseSpeed, turnDecrease);
		}
	}

	if (d->wallRubTimer != 0)
	{
		if (d->wallRubSpeedLimit < d->baseSpeed)
		{
			d->baseSpeed = d->wallRubSpeedLimit;
		}

		if (d->baseSpeed < CTR_MipsNegLo(d->wallRubSpeedLimit))
		{
			d->baseSpeed = (s16)CTR_MipsNegLo(d->wallRubSpeedLimit);
		}
	}

	Vec3 movement = d->velocity;
	int speedLoss = 0;

	if ((d->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		goto CHECK_FOR_ANY_JUMP;
	}

	int acceleration = 0;

	if (((d->stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD_MASK) != 0) && (d->baseSpeed > 0))
	{
		acceleration = 8000;
	}
	else if (d->baseSpeed != 0)
	{
		if (((d->terrainMeta1->flags & TERRAIN_FLAG_ACCEL_WHILE_REVERSE_SLIDING) == 0) || (d->baseSpeed < 1) || (d->speedApprox >= 0))
		{
			int speedApprox = d->speedApprox;
			int absSpeedApprox = VehPhysGeneral_Jump_Abs(speedApprox);

			if ((absSpeedApprox > 0x2ff) && ((d->baseSpeed < 1) || (speedApprox < 1)) && ((d->baseSpeed >= 0) || (speedApprox >= 0)))
			{
				goto PROCESS_ACCEL;
			}
		}

		acceleration = CTR_MipsAddLo(d->const_Accel_ClassStat, CTR_MipsSll((s8)d->accelConst, 5) / 5);

		if ((d->stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD_MASK) == 0)
		{
			if ((d->reserves != 0) && (d->baseSpeed > 0))
			{
				acceleration = d->const_Accel_Reserves;
			}

			int slowUntilSpeed = d->terrainMeta1->slowUntilSpeed;
			if ((slowUntilSpeed != 0x100) && ((d->actionsFlagSet & ACTION_MASK_WEAPON) == 0))
			{
				acceleration = CTR_MipsSra(CTR_MipsMulLo(slowUntilSpeed, acceleration), 8);
			}
		}
		else if (d->baseSpeed > 0)
		{
			acceleration = 8000;
		}
	}

PROCESS_ACCEL:
{
	int forwardImpulse = CTR_MipsSra(CTR_MipsMulLo(acceleration, sdata->gGT->elapsedTimeMS), 5);
	Vec3 rotated = VehPhysGeneral_Jump_RotateLoadedVector(0, 0, (s16)forwardImpulse);

	if (d->baseSpeed < 0)
	{
		d->forwardAccelImpulse = (s16)CTR_MipsNegLo(forwardImpulse);

		movement.x = CTR_MipsSubLo(movement.x, rotated.x);
		movement.y = CTR_MipsSubLo(movement.y, rotated.y);
		movement.z = CTR_MipsSubLo(movement.z, rotated.z);

		d->forwardAccelVector.x = (s16)CTR_MipsNegLo(rotated.x);
		d->forwardAccelVector.y = (s16)CTR_MipsNegLo(rotated.y);
		d->forwardAccelVector.z = (s16)CTR_MipsNegLo(rotated.z);
	}
	else
	{
		d->forwardAccelImpulse = (s16)forwardImpulse;

		movement.x = CTR_MipsAddLo(movement.x, rotated.x);
		movement.y = CTR_MipsAddLo(movement.y, rotated.y);
		movement.z = CTR_MipsAddLo(movement.z, rotated.z);

		d->forwardAccelVector.x = (s16)rotated.x;
		d->forwardAccelVector.y = (s16)rotated.y;
		d->forwardAccelVector.z = (s16)rotated.z;
	}

	u32 movementLengthSq =
	    (u32)CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(movement.x, movement.x), CTR_MipsMulLo(movement.y, movement.y)), CTR_MipsMulLo(movement.z, movement.z));
	speedLoss = CTR_MipsSubLo((s32)(VehCalc_FastSqrt(movementLengthSq, 0x10) >> 8), VehPhysGeneral_Jump_Abs(d->baseSpeed));

	b32 clampToForwardImpulse = forwardImpulse < speedLoss;
	if (speedLoss < 0)
	{
		speedLoss = 0;
		clampToForwardImpulse = forwardImpulse < 0;
	}
	if (clampToForwardImpulse)
	{
		speedLoss = forwardImpulse;
	}

	if (((d->actionsFlagSet & ACTION_TOUCH_GROUND) == 0) || (d->jump_ForcedMS == 0))
	{
		goto CHECK_FOR_ANY_JUMP;
	}

	if (d->jump_HighJumpTimerMS != 0)
	{
		d->jump_HighJumpTimerMS = 0x180;
	}

	if (d->kartState == KS_BLASTED)
	{
		GAMEPAD_ShockFreq(d, 8, 0);
		GAMEPAD_ShockForce1(d, 8, 0x7f);
	}
}

	goto PROCESS_JUMP;

CHECK_FOR_ANY_JUMP:
	if (((d->actionsFlagSet & ACTION_WEAPON_FIRE_REQUEST) != 0) && (d->heldItemID == 5))
	{
		d->actionsFlagSet &= ~ACTION_WEAPON_FIRE_REQUEST;

		if ((d->jump_CoyoteTimerMS != 0) && (d->jump_CooldownMS == 0))
		{
			d->jump_ForcedMS = 0xa0;

			int jumpForce = CTR_MipsAddLo(CTR_MipsSll(d->const_JumpForce, 3), d->const_JumpForce);
			d->jump_InitialVelY = (s16)VehPhysGeneral_Jump_Div4TowardZero(jumpForce);

			OtherFX_Play_Echo(9, 1, (d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);

			d->jump_HighJumpTimerMS = 0x180;
			goto PROCESS_JUMP;
		}

		d->noItemTimer = 0;
	}

	if (d->forcedJumpType == FORCED_JUMP_NONE)
	{
		if ((d->jump_CoyoteTimerMS == 0) || (d->jump_TenBuffer == 0) || (d->jump_CooldownMS != 0))
		{
			if ((d->actionsFlagSet & ACTION_TOUCH_GROUND) != 0)
			{
				if ((d->underDriver != NULL) && (d->underDriver->mulNormVecY != 0))
				{
					int speedApprox = d->speedApprox;
					if (speedApprox < 0)
					{
						speedApprox = VehPhysGeneral_Jump_Abs(speedApprox);
					}

					s16 antiGravVelY = (s16)CTR_MipsSra(CTR_MipsMulLo(d->underDriver->mulNormVecY, speedApprox), 8);
					Vec3 rotated = VehPhysGeneral_Jump_RotateLoadedVector(0, antiGravVelY, 0);

					movement.x = CTR_MipsAddLo(movement.x, rotated.x);
					movement.y = CTR_MipsAddLo(movement.y, rotated.y);
					movement.z = CTR_MipsAddLo(movement.z, rotated.z);
				}
			}

			goto NOT_JUMPING;
		}

		d->jump_ForcedMS = 0xa0;
		d->numberOfJumps = (s16)CTR_MipsAddLo((u16)d->numberOfJumps, 1);
		d->jump_InitialVelY = d->const_JumpForce;

		OtherFX_Play_Echo(8, 1, (d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
	}
	else
	{
		if ((d->jump_ForcedMS == 0) || (d->jump_InitialVelY == d->const_JumpForce))
		{
			OtherFX_Play(0x7e, 1);
		}

		d->jump_ForcedMS = 0xa0;

		int jumpForce = CTR_MipsAddLo(CTR_MipsSll(d->const_JumpForce, 1), d->const_JumpForce);
		if (d->forcedJumpType == FORCED_JUMP_HIGH)
		{
			d->jump_HighJumpTimerMS = 0x180;
			d->jump_InitialVelY = (s16)jumpForce;
		}
		else
		{
			d->jump_InitialVelY = (s16)VehPhysGeneral_Jump_Div2TowardZero(jumpForce);
		}

		d->forcedJumpType = FORCED_JUMP_NONE;
	}

PROCESS_JUMP:
	d->jump_CooldownMS = 0x180;
	d->jump_TenBuffer = 0;
	d->actionsFlagSet |= ACTION_JUMP_STARTED | ACTION_TURBO_INPUT_LATCH;

	int bestJumpVelY = 0;
	int jumpVelY = VehPhysGeneral_JumpGetVelY(d->AxisAngle4_normalVec.v, &movement);
	if (VehPhysGeneral_Jump_Abs(bestJumpVelY) < VehPhysGeneral_Jump_Abs(jumpVelY))
	{
		bestJumpVelY = jumpVelY;
	}

	s16 *normalVec = d->AxisAngle1_normalVec.v;
	if ((d->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		normalVec = d->AxisAngle2_normalVec.v;
	}

	jumpVelY = VehPhysGeneral_JumpGetVelY(normalVec, &movement);

	int jumpVelYSquared = CTR_MipsMulLo(bestJumpVelY, bestJumpVelY);
	if (VehPhysGeneral_Jump_Abs(bestJumpVelY) < VehPhysGeneral_Jump_Abs(jumpVelY))
	{
		jumpVelYSquared = CTR_MipsMulLo(jumpVelY, jumpVelY);
		bestJumpVelY = jumpVelY;
	}

	int verticalSpeed = VehCalc_FastSqrt((u32)CTR_MipsSra(CTR_MipsAddLo(jumpVelYSquared, CTR_MipsMulLo(d->jump_InitialVelY, d->jump_InitialVelY)), 8), 8);

	int maxVerticalSpeed = sdata->gGT->level1->jumpVerticalSpeedCap << 8;
	if (maxVerticalSpeed == 0)
	{
		maxVerticalSpeed = 0x3700;
	}
	else if (maxVerticalSpeed > 0x5000)
	{
		maxVerticalSpeed = 0x5000;
	}

	verticalSpeed = CTR_MipsSubLo(verticalSpeed, bestJumpVelY);
	if (maxVerticalSpeed < verticalSpeed)
	{
		verticalSpeed = maxVerticalSpeed;
	}

	if (movement.y < verticalSpeed)
	{
		movement.y = verticalSpeed;
	}

NOT_JUMPING:
	VehPhysCrash_ConvertVecToSpeed(d, &movement);

	int speed = CTR_MipsSubLo((u16)d->speed, speedLoss);
	d->speed = (s16)speed;
	if (d->speed < 0)
	{
		d->speed = 0;
	}

	int speedApprox = d->speedApprox;
	if (speedApprox < 0)
	{
		speedApprox = VehPhysGeneral_Jump_Abs(speedApprox);

		if (speedApprox < 0x100)
		{
			d->speedometerNeedleValue = (s16)CTR_MipsSubLo((u16)d->speedometerNeedleValue, CTR_MipsSra(d->speedometerNeedleValue, 3));
		}
		else
		{
			d->speedometerNeedleValue =
			    (s16)((u32)CTR_MipsAddLo(CTR_MipsMulLo(d->speedometerNeedleValue, 0xd), CTR_MipsMulLo(sdata->gGT->timer & 7, 0x300)) >> 4);
		}
	}
	else
	{
		d->speedometerNeedleValue = (s16)CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo(d->speedometerNeedleValue, 0xd), CTR_MipsMulLo(speedApprox, 3)), 4);
	}
}

enum ItemSet
{
	ITEMSET_Race1 = 0,
	ITEMSET_Race2,
	ITEMSET_Race3,
	ITEMSET_Race4,
	ITEMSET_BattleDefault,
	ITEMSET_BattleCustom,
	ITEMSET_CrystalChallenge,
	ITEMSET_BossRace
};

// all except CrystalChallenge
extern u8 *charPtr[8];
extern u8 numWeapons[8];

// Itemset infographic (outdated):
// https://discord.com/channels/330945093416779787/550106151887568906/734368526294450267
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80060f0c-0x80061488.
void VehPhysGeneral_SetHeldItem(struct Driver *driver)
{
	s32 rng;
	int itemSet;
	u8 item;
	s8 bossFails;
	struct GameTracker *gGT;

	gGT = sdata->gGT;

	itemSet = -1;

	if ((gGT->gameMode1 & BATTLE_MODE) != 0)
	{
		// 6th Itemset (Battle Mode Custom Itemset)
		itemSet = ITEMSET_BattleCustom;

		// 5th Itemset (Battle Mode Default Itemset, 0x34de)
		if (gGT->battleSetup.enabledWeapons == 0x34de)
		{
			itemSet = ITEMSET_BattleDefault;
		}
	}

	// Not in Battle Mode
	else
	{
		if ((gGT->gameMode1 & CRYSTAL_CHALLENGE) != 0)
		{
			// 7th Itemset (Crystal Challenge)
			itemSet = ITEMSET_CrystalChallenge;
		}
		else
		{
			// Choose Itemset based on number of Drivers
			int mode = gGT->numPlyrCurrGame + gGT->numBotsNextGame;

			switch (mode)
			{
			// if boss race
			case 2:

				// boss race, last place
				itemSet = ITEMSET_BossRace;

				// if in first place
				if (driver->driverRank == 0)
				{
				Itemset1:
					// 1st Itemset
					itemSet = ITEMSET_Race1;
				}
				break;

			// 3P VS race
			case 3:

				// if first place
				if (driver->driverRank == 0)
				{
					goto Itemset1;
				}

				// default (2nd or 3rd place)
				itemSet = ITEMSET_Race4;

				// 50/50 chance of an upgrade,
				// while in 2nd place

				if (driver->driverRank == 1)
				{
					itemSet = ITEMSET_Race3;
					rng = MixRNG_Scramble();
					if (rng & 1)
					{
						goto Itemset2;
					}
				}

				break;
			case 4:
				itemSet = driver->driverRank;
				break;
			case 5:
				itemSet = driver->driverRank;
				// 5th rank is 4th Itemset
				if (itemSet == 4)
				{
					itemSet = 3;
				}
				break;

			// 2P Arcade
			case 6:

				// careful, dont get confused by names
				itemSet = driver->driverRank;

				// if 1st place, ItemSet1
				if (itemSet == 0)
				{
					goto Itemset1;
				}

				// if 6th place, ItemSet4
				if (itemSet == 5)
				{
					itemSet = ITEMSET_Race4;
				}

				// 2nd, 3rd place, gets 2nd Itemset
				// 4th, 5th place, gets 3rd Itemset
				else
				{
					itemSet = (itemSet - 1) / 2 + 1;
				}

				break;

			// 1P Arcade
			case 8:

				// 0,1 = 0 (itemset1)
				// 2,3 = 1 (itemset2)
				// 4,5 = 2 (itemset3)
				// 6,7 = 3 (itemset4)
				itemSet = CTR_MipsSra(CTR_MipsAddLo(driver->driverRank, (u32)driver->driverRank >> 31), 1);

				// if in 2nd place, get itemSet2
				if (itemSet == 1)
				{
				Itemset2:
					itemSet = ITEMSET_Race2;
				}
			}
		}

		// if you have 4th-place itemset on first lap,
		// then override to 3rd place
		if (itemSet == ITEMSET_Race4 && driver->lapIndex == 0)
		{
			itemSet = ITEMSET_Race3;
		}
	}

	// Decide item for Driver
	rng = CTR_MipsSra(MixRNG_Scramble(), 0x3);
	rng = CTR_MipsSubLo(rng, CTR_MipsMulLo(CTR_MipsDiv(rng, 0xc8), 0xc8));

	switch (itemSet)
	{
	case ITEMSET_Race1:
	case ITEMSET_Race2:
	case ITEMSET_Race3:
	case ITEMSET_Race4:
	case ITEMSET_BattleDefault:
	case ITEMSET_BossRace:
		driver->heldItemID = charPtr[itemSet][(rng * numWeapons[itemSet]) / 0xc8];
		break;

	// uses int array instead of char,
	// should fix that later, requires 230 rewrite
	case ITEMSET_BattleCustom:
		driver->heldItemID = gGT->battleSetup.RNG_itemSetCustom[(rng * gGT->battleSetup.numWeapons) / 0xc8];
		break;

	case ITEMSET_CrystalChallenge:
		// Item is bomb at Rocky Road, Nitro Court
		// Item is turbo at Skull Rock and Rampage Ruins
		item = 0x1;
		if (gGT->levelID != SKULL_ROCK && gGT->levelID != RAMPAGE_RUINS)
		{
			goto SetItem;
		}
		driver->heldItemID = 0x0;
		break;

	// "-1st place": Undecided rank
	default:
		rng = MixRNG_Scramble();
		item = (u8)CTR_MipsSubLo(rng, CTR_MipsMulLo(CTR_MipsDiv(rng, 0xc), 0xc));
	SetItem:
		driver->heldItemID = item;
	}

	// In Boss race
	if (gGT->gameMode1 & ADVENTURE_BOSS)
	{
		bossFails = sdata->advProgress.timesLostBossRace[gGT->bossID];

		if (bossFails < 0x3)
		{
			// Replace Clock, Mask,  with 3 Missiles
			if ((u32)driver->heldItemID - 0x7 < 0x3)
			{
				driver->heldItemID = 0xb;
			}
		}

		else if (bossFails < 0x4)
		{
			// Replace Clock, Mask with 3 Missiles
			if ((u32)driver->heldItemID - 0x7 < 0x2)
			{
				driver->heldItemID = 0xb;
			}
		}

		else if (bossFails < 0x5 && driver->heldItemID == 0x8)
		{
			// Replace Clock with 3 Missiles
			driver->heldItemID = 0xb;
		}

		// Replace 3 Missiles with 1 Missile if racing Komodo Joe
		if (gGT->levelID == DRAGON_MINES && driver->heldItemID == 0xb)
		{
			driver->heldItemID = 0x2;
		}
	}

	// Replace unused Spring item with Turbo
	if (driver->heldItemID == 0x5)
	{
		driver->heldItemID = 0x0;
	}

	// Make sure only 1 Warpball is instanced at once
	if (driver->heldItemID == 0x9)
	{
		// if nobody has warpball, then set flag that somebody has it
		if ((gGT->gameMode1 & WARPBALL_HELD) == 0)
		{
			gGT->gameMode1 |= WARPBALL_HELD;
		}

		// if somebody has warpball already, then give 3 missiles
		else
		{
			driver->heldItemID = 0xb;
		}
	}

	if (
	    // if you got 3 missiles
	    driver->heldItemID == 0xb &&

	    // if more than 2 players
	    gGT->numPlyrCurrGame > 2 &&

	    // if not in battle mode
	    ((gGT->gameMode1 & BATTLE_MODE) == 0))
	{
		// if less than 2 drivers have 3 missiles, then increase number of drivers that have it
		if (gGT->numPlayersWith3Missiles < 2)
		{
			gGT->numPlayersWith3Missiles++;
		}

		// if 2 drivers already have 3 missiles, now you have 1 missile
		else
		{
			driver->heldItemID = 0x2;
		}
	}

	// Set number of held items
	if ((u32)driver->heldItemID - 0xA < 0x2)
	{
		driver->numHeldItems = 0x3;
	}

	return;
}

u8 *charPtr[8] = {(u8 *)&data.RNG_itemSetRace1[0],
                  (u8 *)&data.RNG_itemSetRace2[0],
                  (u8 *)&data.RNG_itemSetRace3[0],
                  (u8 *)&data.RNG_itemSetRace4[0],
                  (u8 *)&data.RNG_itemSetBattleDefault[0],
                  (u8 *)&sdata_static.gameTracker.battleSetup.RNG_itemSetCustom[0],
                  NULL,
                  (u8 *)&data.RNG_itemSetBossrace[0]};

u8 numWeapons[8] = {0x14, 0x34, 0x14, 0x13, 0x14, 0, 0, 0x14};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80061488-0x8006163c.
int VehPhysGeneral_GetBaseSpeed(struct Driver *driver)
{
	int netSpeed;
	int statAdditional;
	int speedAdditional;
	statAdditional = (int)driver->const_Speed_ClassStat;

	int netWumpaFruitCount = (int)driver->numWumpas;
	if (netWumpaFruitCount > 9)
	{
		netWumpaFruitCount = 9;
	}

	int turboMultiplier = (int)driver->turboConst;
	if (turboMultiplier > 5)
	{
		turboMultiplier = 5;
	}

	int netSpeedStat = CTR_MipsSubLo(CTR_MipsDiv(CTR_MipsSll(CTR_MipsSubLo(driver->const_AccelSpeed_ClassStat, driver->const_Speed_ClassStat), 0xc), 5), 1);

	speedAdditional =
	    CTR_MipsSra(CTR_MipsAddLo(CTR_MipsDiv(CTR_MipsMulLo(netWumpaFruitCount, netSpeedStat), 10), CTR_MipsMulLo(turboMultiplier, netSpeedStat)), 0xc);

	if ((driver->actionsFlagSet & ACTION_MASK_WEAPON) != 0)
	{
		speedAdditional = CTR_MipsAddLo(speedAdditional, driver->const_MaskSpeed);
	}

	if (driver->reserves != 0)
	{
		statAdditional = CTR_MipsAddLo(statAdditional, driver->fireSpeedCap);

		int netSpeedCap = CTR_MipsSubLo(
		    CTR_MipsAddLo(driver->const_SingleTurboSpeed, CTR_MipsSll(CTR_MipsSubLo(driver->const_SacredFireSpeed, driver->const_SingleTurboSpeed), 1)),
		    driver->fireSpeedCap);
		if (netSpeedCap < 0)
		{
			netSpeedCap = 0;
		}

		if (netSpeedCap < speedAdditional)
		{
			speedAdditional = netSpeedCap;
		}
	}

	int subtract = 0;

	if (driver->instTntRecv != 0)
	{
		subtract = CTR_MipsSra(driver->const_DamagedSpeed, 1);
	}

	if (
	    // burn, squish, or raincloud
	    (driver->burnTimer != 0) || (driver->squishTimer != 0) || (driver->rainCloudEffect == RAIN_CLOUD_EFFECT_SLOW))
	{
		subtract = driver->const_DamagedSpeed;
	}

	if (driver->clockReceive != 0)
	{
		// NOTE(aalhendi) Retail scales clock damage by rank: stronger near the front, still nonzero near the back.
		int clockEffect = CTR_MipsSra(CTR_MipsMulLo(driver->const_DamagedSpeed, CTR_MipsSubLo(0x14, driver->driverRank)), 4);

		if (subtract < clockEffect)
		{
			subtract = clockEffect;
		}
	}

	netSpeed = CTR_MipsSubLo(CTR_MipsAddLo(statAdditional, speedAdditional), subtract);

	if (0x6400 < netSpeed)
	{
		netSpeed = 0x6400;
	}

	return netSpeed;
}
