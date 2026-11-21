#include "VehCommon.h"
#include "VehPhysProc_Input.c"
#include "VehPhysProc_Items.c"

// budget: 4624
// curr: 4380

enum
{
	VEH_PHYS_PROC_ACTION_CARRY_MASK = ACTION_TOUCH_GROUND | ACTION_JUMP_BUTTON_HELD | ACTION_STEER_LEFT | ACTION_HIGH_JUMP | ACTION_TURBO_INPUT_LATCH |
	                                  ACTION_DRIVING_WRONG_WAY | ACTION_TURBO_ITEM | ACTION_WEAPON_FIRE_REQUEST | ACTION_ENGINE_ECHO | ACTION_REVERSING_ENGINE |
	                                  ACTION_RACE_TIMER_FROZEN | ACTION_AIRBORNE | ACTION_BOT | ACTION_BEHIND_START_LINE | ACTION_RACE_FINISHED |
	                                  ACTION_TRACKER_TARGETED | ACTION_CHECKPOINT_BRANCH_PENDING | ACTION_HUMAN_HUMAN_COLLISION | ACTION_REVERSE_STEER_LEFT |
	                                  ACTION_REVERSE_STEER_RIGHT,
	VEH_PHYS_PROC_ITEM_ROLL_FAST_STOP_FRAMES = 70,
	VEH_PHYS_PROC_WEAPON_COOLDOWN_EMPTY_FRAMES = 0x1e,
	VEH_PHYS_PROC_WEAPON_COOLDOWN_STACK_FRAMES = 5,
	VEH_PHYS_PROC_ASSUMED_CROSS_BUTTON = BTN_CROSS_one,
	VEH_PHYS_PROC_INVISIBLE_REAPPEAR_FX = 0x62,
	VEH_PHYS_PROC_DISTANCE_SPEED_SHIFT = 8,
	VEH_PHYS_PROC_TEN_WUMPA_COUNT = 10,
	VEH_PHYS_PROC_HAZARD_MOVING_SPEED_MIN = 0x100,
	VEH_PHYS_PROC_HAZARD_LOW_SPEED_THRESHOLD = 0x101,
	VEH_PHYS_PROC_HAZARD_TIMER_EVEN_MASK = -2,
	VEH_PHYS_PROC_CLOCK_WADDLE_TIMER_SHIFT = 6,
	VEH_PHYS_PROC_CLOCK_WADDLE_TIMER_MAX = 0x40,
	VEH_PHYS_PROC_CLOCK_WADDLE_TRIG_SHIFT = 4,
	VEH_PHYS_PROC_CLOCK_WADDLE_SPEED_SHIFT = 8,
	VEH_PHYS_PROC_CLOCK_WADDLE_SPEED_MAX = 0x20,
	VEH_PHYS_PROC_CLOCK_WADDLE_TRIG_FORCE_SHIFT = 5,
	VEH_PHYS_PROC_CLOCK_WADDLE_RUMBLE_CHANNEL = 4,
	VEH_PHYS_PROC_CLOCK_WADDLE_FORCE_BASE = 0x18,
	VEH_PHYS_PROC_REVERSE_STICK_THRESHOLD = 100,
	VEH_PHYS_PROC_BRAKE_HIGH_SPEED_THRESHOLD = 0x300,
	VEH_PHYS_PROC_REVERSE_SPEED_COMPARE = 0x301,
	VEH_PHYS_PROC_REVERSE_SPEED_SCALE_SHIFT = 7,
	VEH_PHYS_PROC_REVERSE_SPEED_ROUNDING_BIAS = 0x7f,
	VEH_PHYS_PROC_BRAKE_SPEED_SCALE_SHIFT = 8,
	VEH_PHYS_PROC_BRAKE_SPEED_ROUNDING_BIAS = 0xff,
	VEH_PHYS_PROC_BRAKE_REVERSE_MULTIPLIER = -3,
	VEH_PHYS_PROC_BRAKE_REVERSE_SCALE_SHIFT = 2,
	VEH_PHYS_PROC_BRAKE_REVERSE_ROUNDING_BIAS = 3,
	VEH_PHYS_PROC_HALF_SPEED_SHIFT = 1,
	VEH_PHYS_PROC_SUPER_ENGINE_BASE_FIRE = 0x80,
	VEH_PHYS_PROC_SUPER_ENGINE_JUICED_FIRE = 0x100,
	VEH_PHYS_PROC_SUPER_ENGINE_RESERVES = 120,
	VEH_PHYS_PROC_TERRAIN_SPEED_NEUTRAL = 0x100,
	VEH_PHYS_PROC_TERRAIN_SPEED_SHIFT = 8,
	VEH_PHYS_PROC_STEER_TURN_CONST_SHIFT = 1,
	VEH_PHYS_PROC_STEER_TURN_CONST_DIVISOR = 5,
	VEH_PHYS_PROC_STEER_ACCEL_TAP_SPEED_MAX = 0x2600,
	VEH_PHYS_PROC_STEER_ACCEL_TAP_STRENGTH = 0x5a,
	VEH_PHYS_PROC_STEER_WALL_RUB_STRENGTH = 0x30,
	VEH_PHYS_PROC_STEER_BRAKE_STRENGTH = 0x40,
	VEH_PHYS_PROC_STEER_RESET_FRAMES = 10000,
	VEH_PHYS_PROC_WHEEL_ROTATION_STRENGTH = 0x40,
	VEH_PHYS_PROC_WHEEL_ROTATION_INTERP_STEP = 0x18,
	VEH_PHYS_PROC_TIRE_COLOR_SPEED_AVERAGE_SHIFT = 1,
	VEH_PHYS_PROC_TIRE_COLOR_STEP_BLEND_SHIFT = 3,
	VEH_PHYS_PROC_TIRE_COLOR_STEP_RESULT_SHIFT = 0xc,
	VEH_PHYS_PROC_VSHIFT_MAX_COUNT = 5,
	VEH_PHYS_PROC_VSHIFT_START_GUARD_TIMER = 0x60,
	VEH_PHYS_PROC_VSHIFT_WINDOW_TIMER = 0x280,
	VEH_PHYS_PROC_BATTLE_BLASTED_INVINCIBLE_TIMER = 0xb40,
	VEH_PHYS_PROC_DRIFT_ANGLE_LERP_SHIFT = 3,
	VEH_PHYS_PROC_DRIFT_AXIS_STEP_MS_SHIFT = 6,
	VEH_PHYS_PROC_DRIFT_MS_SCALE_SHIFT = 5,
	VEH_PHYS_PROC_DRIFT_TURN_CONST_SHIFT = 2,
	VEH_PHYS_PROC_FRAME_TIME_SHIFT = 5,
	VEH_PHYS_PROC_DRIFT_SPINOUT_THRESHOLD_SHIFT = 1,
	VEH_PHYS_PROC_TURN_WOBBLE_START_ANGLE_MAX = 10,
	VEH_PHYS_PROC_TURN_WOBBLE_START_TIMER = 8,
	VEH_PHYS_PROC_TURN_WOBBLE_START_VELOCITY = 0x14,
	VEH_PHYS_PROC_TURN_WOBBLE_ANGLE_MAX = 0x32,
	VEH_PHYS_PROC_TURN_WOBBLE_RETURN_VELOCITY = 10,
	VEH_PHYS_PROC_ANGLE_INTEGRATION_SHIFT = 0xd,
	VEH_PHYS_PROC_DRIFT_MAX_BOOSTS = 3,
	VEH_PHYS_PROC_DRIFT_METER_FULL_FX = 0xf,
	VEH_PHYS_PROC_DRIFT_FIRE_LEVEL_SHIFT = 6,
	VEH_PHYS_PROC_DRIFT_FAILED_BOOST_EXHAUST_TIMER = 8,
	VEH_PHYS_PROC_DRIFT_SPINOUT_NO_INPUT_TIMER = 0x3c0,
	VEH_PHYS_PROC_DRIFT_REVERSE_SPINOUT_NO_INPUT_TIMER = 0x780,
	VEH_PHYS_PROC_CRASH_SCALE_XY = 0xccc,
	VEH_PHYS_PROC_SPIN_SLOW_SPEED_THRESHOLD = 0x2ff,
	VEH_PHYS_PROC_SPIN_INITIAL_RATE = 300,
	VEH_PHYS_PROC_SPIN_VOICELINE_ID = 3,
	VEH_PHYS_PROC_SPIN_VOICELINE_PRIORITY = 0x10,
	VEH_PHYS_PROC_SPIN_LEFT_FEEDBACK = 0x19,
	VEH_PHYS_PROC_SPIN_RIGHT_FEEDBACK = 0x29,
	VEH_PHYS_PROC_SPIN_FEEDBACK_TIMER = 0x60,
	VEH_PHYS_PROC_SPIN_STOP_ANGLE_THRESHOLD = 16,
	VEH_PHYS_PROC_SPIN_RECENTER_ANGLE_THRESHOLD = 400,
	VEH_PHYS_PROC_SPIN_RECENTER_RATE_NUM_SHIFT = 2,
	VEH_PHYS_PROC_SPIN_RECENTER_RATE_DEN_SHIFT = 3,
	VEH_PHYS_PROC_SPIN_MIN_RECENTER_RATE = 0x20,
	VEH_PHYS_PROC_SPIN_STOP_ANIM_SWEEP_STEP = 5,
	VEH_PHYS_PROC_SPIN_STOP_ANIM_RETURN_STEP = 2,
};

CTR_STATIC_ASSERT((ANG_TWO_PI - 1) == 0xfff);

static inline void VehPhysProc_Driving_DecrementTimer(s16 *timer, int elapsed)
{
	if (*timer > 0)
	{
		int value = CTR_MipsSubLo(*timer, elapsed);
		if (value < 0)
		{
			value = 0;
		}

		*timer = (s16)value;
	}
}

void VehPhysProc_Driving_PhysLinear(struct Thread *thread, struct Driver *driverArg)
{
	struct Driver *driver;
	struct Thread *driverThread;
	int kartState;
	register u8 heldItemID CTR_PSX_REGISTER("$3");
	u16 driverTimerNegativeFinal;
	int approxTrig;
	int approximateSpeed;
	register int approximateSpeedRaw CTR_PSX_REGISTER("$24");
	int tireColorStep;
	int tireColorWeight;
	int tireSpeedWeight;
	register int tireSpeedRaw CTR_PSX_REGISTER("$3");
	register int tireApproximateSpeed CTR_PSX_REGISTER("$24");
	int steerStrength;
	int steeringInput;
	register u32 buttonsTapped CTR_PSX_REGISTER("$4");
	int timerHazard;
	int airborneTimer;
	register int trigForce CTR_PSX_REGISTER("$2");
	s32 clockReceiveRaw;
	register int clockWaddleTimer CTR_PSX_REGISTER("$6");
	register u32 actionsFlagSetCopy CTR_PSX_REGISTER("$18");
	register int targetBaseSpeed CTR_PSX_REGISTER("$23");
	struct GamepadBuffer *ptrgamepad;
	u32 cross;
	u32 square;

	int msPerFrameNeg;
	int msPerFrame;
	register int reservesTimer CTR_PSX_REGISTER("$3");
	register int turboTimer CTR_PSX_REGISTER("$4");
	register int audioTimer CTR_PSX_REGISTER("$5");
	register int wallTimer CTR_PSX_REGISTER("$6");
	register int forcedJumpTimer CTR_PSX_REGISTER("$7");
	register int jumpCooldownTimer CTR_PSX_REGISTER("$8");
	register int highJumpTimer CTR_PSX_REGISTER("$9");
	register int burnTimer CTR_PSX_REGISTER("$10");
	register int squishTimer CTR_PSX_REGISTER("$11");
	register int clockTimer CTR_PSX_REGISTER("$15");
	register int accelTapTimer CTR_PSX_REGISTER("$16");
	register int vShiftGuardTimer CTR_PSX_REGISTER("$12");
	register int vShiftWindowTimer CTR_PSX_REGISTER("$13");
	register int superEngineTimer CTR_PSX_REGISTER("$14");
	register int jumpTenBuffer CTR_PSX_REGISTER("$18");
	register int rainCloudEffect CTR_PSX_REGISTER("$20");
	register u32 actionsFlagSetNext CTR_PSX_REGISTER("$18");
	register u32 buttonsHeld CTR_PSX_REGISTER("$5");
	int stickRY;
	int gasStickLY;
	int scratchValue;
	struct Shield *shield;
	struct TrackerWeapon *bomb;
	s16 *normalDestination;
	u32 superEngineFireLevel;
	int centeredStick;
	int driverSpeedCopy;
	register int normalVecIndex CTR_PSX_REGISTER("$16");
	register int nextHeldItemCount CTR_PSX_REGISTER("$2");

	driverThread = thread;
	driver = driverArg;

	// If race timer is not supposed to stop for this racer
	if ((driver->actionsFlagSet & ACTION_RACE_TIMER_FROZEN) == 0)
	{
		// set racer's timer to the time on the clock
		driver->timeElapsedInRace = GAME_TRACKER->elapsedEventTime;
	}
	CTR_PSX_KEEP_VALUE(driver);

	// === Count Timers ===


	// elapsed milliseconds per frame, ~32
	msPerFrame = GAME_TRACKER->elapsedTimeMS;
	msPerFrameNeg = CTR_MipsNegLo(msPerFrame);

	if ((GAME_TRACKER->elapsedEventTime < 10 * MINUTE) && ((driver->actionsFlagSet & ACTION_RACE_TIMER_FROZEN) == 0))
	{
		driver->distanceDriven =
		    CTR_MipsAddLo(driver->distanceDriven, CTR_MipsSra(CTR_MipsMulLo(driver->speedApprox, msPerFrame), VEH_PHYS_PROC_DISTANCE_SPEED_SHIFT));
	}

	reservesTimer = driver->reserves;
	CTR_PSX_KEEP_VALUE(reservesTimer);
	turboTimer = driver->turbo_outsideTimer;
	audioTimer = driver->VehFire_AudioCooldown;
	wallTimer = driver->wallRubTimer;
	forcedJumpTimer = driver->jump_ForcedMS;
	jumpCooldownTimer = driver->jump_CooldownMS;
	highJumpTimer = driver->jump_HighJumpTimerMS;
	burnTimer = driver->burnTimer;
	squishTimer = driver->squishTimer;
	clockTimer = driver->clockReceive;
	accelTapTimer = driver->accelTapWindowTimer;
	vShiftGuardTimer = driver->vShiftStartGuardTimer;
	vShiftWindowTimer = driver->vShiftWindowTimer;
	superEngineTimer = driver->superEngineTimer;
	jumpTenBuffer = driver->jump_TenBuffer;

	if (reservesTimer > 0)
	{
		reservesTimer = CTR_MipsAddLo(reservesTimer, msPerFrameNeg);
		if (reservesTimer < 0)
		{
			reservesTimer = 0;
		}
		VehFire_SetReserves(driver, reservesTimer);
		driver->timeSpentUsingReserves = CTR_MipsSubLo(driver->timeSpentUsingReserves, msPerFrameNeg);
	}
	if (turboTimer > 0)
	{
		turboTimer = CTR_MipsAddLo(turboTimer, msPerFrameNeg);
		if (turboTimer < 0)
		{
			turboTimer = 0;
		}
		driver->turbo_outsideTimer = (s16)turboTimer;
	}
	if (audioTimer > 0)
	{
		audioTimer = CTR_MipsAddLo(audioTimer, msPerFrameNeg);
		if (audioTimer < 0)
		{
			audioTimer = 0;
		}
		driver->VehFire_AudioCooldown = (s16)audioTimer;
	}
	if (wallTimer > 0)
	{
		wallTimer = CTR_MipsAddLo(wallTimer, msPerFrameNeg);
		if (wallTimer < 0)
		{
			wallTimer = 0;
		}
		driver->wallRubTimer = (s16)wallTimer;
		driver->timeSpentAgainstWall = CTR_MipsSubLo(driver->timeSpentAgainstWall, msPerFrameNeg);
	}
	if (forcedJumpTimer > 0)
	{
		forcedJumpTimer = CTR_MipsAddLo(forcedJumpTimer, msPerFrameNeg);
		if (forcedJumpTimer < 0)
		{
			forcedJumpTimer = 0;
		}
		driver->jump_ForcedMS = (s16)forcedJumpTimer;
	}
	if (jumpCooldownTimer > 0)
	{
		jumpCooldownTimer = CTR_MipsAddLo(jumpCooldownTimer, msPerFrameNeg);
		if (jumpCooldownTimer < 0)
		{
			jumpCooldownTimer = 0;
		}
		driver->jump_CooldownMS = (s16)jumpCooldownTimer;
	}
	if (highJumpTimer > 0)
	{
		highJumpTimer = CTR_MipsAddLo(highJumpTimer, msPerFrameNeg);
		if (highJumpTimer < 0)
		{
			highJumpTimer = 0;
		}
		driver->jump_HighJumpTimerMS = (s16)highJumpTimer;
	}
	if (burnTimer > 0)
	{
		burnTimer = CTR_MipsAddLo(burnTimer, msPerFrameNeg);
		if (burnTimer < 0)
		{
			burnTimer = 0;
		}
		driver->burnTimer = (s16)burnTimer;
		driver->timeSpentBurnt = CTR_MipsSubLo(driver->timeSpentBurnt, msPerFrameNeg);
	}
	if (squishTimer > 0)
	{
		squishTimer = CTR_MipsAddLo(squishTimer, msPerFrameNeg);
		if (squishTimer < 0)
		{
			squishTimer = 0;
		}
		driver->squishTimer = (s16)squishTimer;
		driver->timeSpentSquished = CTR_MipsSubLo(driver->timeSpentSquished, msPerFrameNeg);
	}
	if (vShiftGuardTimer > 0)
	{
		vShiftGuardTimer = CTR_MipsAddLo(vShiftGuardTimer, msPerFrameNeg);
		if (vShiftGuardTimer < 0)
		{
			vShiftGuardTimer = 0;
		}
		driver->vShiftStartGuardTimer = (s16)vShiftGuardTimer;
	}
	if (vShiftWindowTimer > 0)
	{
		vShiftWindowTimer = CTR_MipsAddLo(vShiftWindowTimer, msPerFrameNeg);
		if (vShiftWindowTimer < 0)
		{
			vShiftWindowTimer = 0;
		}
		driver->vShiftWindowTimer = (s16)vShiftWindowTimer;
	}

	// If Super Engine Cheat is not enabled
	if ((GAME_TRACKER->gameMode2 & CHEAT_ENGINE) == 0)
	{
		if (superEngineTimer > 0 && !Chaos_IsTimeStopped())
		{
			superEngineTimer = CTR_MipsAddLo(superEngineTimer, msPerFrameNeg);
			if (superEngineTimer < 0)
			{
				superEngineTimer = 0;
			}
			driver->superEngineTimer = (s16)superEngineTimer;
		}
	}

	if (clockTimer > 0 && !Chaos_IsTimeStopped())
	{
		clockTimer = CTR_MipsAddLo(clockTimer, msPerFrameNeg);
		if (clockTimer < 0)
		{
			clockTimer = 0;
		}
		driver->clockReceive = clockTimer;
	}
	if (accelTapTimer > 0)
	{
		accelTapTimer = CTR_MipsAddLo(accelTapTimer, msPerFrameNeg);
		if (accelTapTimer < 0)
		{
			accelTapTimer = 0;
		}
		driver->accelTapWindowTimer = (s16)accelTapTimer;
	}

	if (jumpTenBuffer > 0)
	{
		driver->jump_TenBuffer = (s16)CTR_MipsSubLo(jumpTenBuffer, 1);
		CTR_PSX_KEEP_VALUE(jumpTenBuffer);
	}
	if (driver->numWumpas >= VEH_PHYS_PROC_TEN_WUMPA_COUNT)
	{
		driver->timeSpentInTenWumpa = CTR_MipsSubLo(driver->timeSpentInTenWumpa, msPerFrameNeg);
	}
	if (driver->currentTerrain == TERRAIN_MUD)
	{
		driver->timeSpentInMud = CTR_MipsSubLo(driver->timeSpentInMud, msPerFrameNeg);
	}


	// === Check Last Place ===

	// Last Place, and time is unfrozen
	if ((((driver->driverRank == 7) && (GAME_TRACKER->numPlyrCurrGame == 1)) || ((driver->driverRank == 5) && (GAME_TRACKER->numPlyrCurrGame == 2)) ||
	     ((driver->driverRank == 3) && (GAME_TRACKER->numPlyrCurrGame > 2))) &&
	    ((driver->actionsFlagSet & ACTION_RACE_TIMER_FROZEN) == 0))
	{
		driver->timeSpentInLastPlace = CTR_MipsSubLo(driver->timeSpentInLastPlace, msPerFrameNeg);
	}

	// === Determine Hazard ===

	rainCloudEffect = RAIN_CLOUD_EFFECT_NONE;

	// if you have a raincloud over your head from potion
	if (driver->thCloud != 0)
	{
		rainCloudEffect = ((struct RainCloud *)driver->thCloud->object)->effect;
	}

	// get approximate speed
	CTR_PSX_LOAD_SIGNED_HALF(approximateSpeedRaw, driver, offsetof(struct Driver, speedApprox), driver->speedApprox);
	CTR_PSX_KEEP_VALUE(approximateSpeedRaw);

	// Action flags (isRaceOver, isTimeFrozen, etc)
	actionsFlagSetCopy = driver->actionsFlagSet;

	// driver->clockReceive
	normalVecIndex = driver->clockReceive;

	driver->rainCloudEffect = rainCloudEffect;
	CTR_PSX_MEMORY_BARRIER();

	// absolute value of speed
	if (approximateSpeedRaw < 0)
	{
		approximateSpeedRaw = CTR_MipsNegLo(approximateSpeedRaw);
	}
	approximateSpeed = approximateSpeedRaw;

	if ((normalVecIndex == 0) && (normalVecIndex = driver->squishTimer, normalVecIndex == 0) &&
	    (rainCloudEffect != RAIN_CLOUD_EFFECT_SLOW || (normalVecIndex = GAME_TRACKER->elapsedEventTime, normalVecIndex == 0)))
	{
		goto applyNormalHazard;
	}

	// if you are not impacted by hazard (other than clock)
	{
		if ((actionsFlagSetCopy & ACTION_TOUCH_GROUND) == 0)
		{
			goto airborneHazard;
		}

		CTR_PSX_LOAD_STACK_WORD(approximateSpeedRaw, 32, approximateSpeed);
		trigForce = approximateSpeedRaw < VEH_PHYS_PROC_HAZARD_LOW_SPEED_THRESHOLD;
		CTR_PSX_KEEP_VALUE(trigForce);
		// Touching the ground at speed applies the clock-waddle feedback.
		if (trigForce != 0)
		{
			goto speedIsLow;
		}
		clockReceiveRaw = driver->clockReceive;
		scratchValue = normalVecIndex;
		if (normalVecIndex < 0)
		{
			scratchValue = CTR_MipsNegLo(scratchValue);
		}

		normalVecIndex = clockReceiveRaw;
		clockWaddleTimer = CTR_MipsSra(normalVecIndex, VEH_PHYS_PROC_CLOCK_WADDLE_TIMER_SHIFT);
		if (clockWaddleTimer > VEH_PHYS_PROC_CLOCK_WADDLE_TIMER_MAX)
		{
			clockWaddleTimer = VEH_PHYS_PROC_CLOCK_WADDLE_TIMER_MAX;
		}

		normalVecIndex = CTR_MipsSll(normalVecIndex, VEH_PHYS_PROC_CLOCK_WADDLE_TRIG_SHIFT);
		approxTrig = (s32)VEH_TRIG_APPROX(ANG_MODULO_HALF_PI(normalVecIndex));
		if ((normalVecIndex & ANG_QUADRANT_BIT) == 0)
		{
			approxTrig = CTR_MipsSll(approxTrig, 16);
		}
		approxTrig = CTR_MipsSra(approxTrig, 16);
		if ((normalVecIndex & ANG_SIGN_BIT) != 0)
		{
			approxTrig = CTR_MipsNegLo(approxTrig);
		}
		CTR_PSX_KEEP_VALUE(normalVecIndex);

		trigForce = CTR_MipsSra(approxTrig, VEH_PHYS_PROC_CLOCK_WADDLE_TRIG_FORCE_SHIFT);
		CTR_PSX_KEEP_VALUE(trigForce);
		clockWaddleTimer = CTR_MipsAddLo(clockWaddleTimer, trigForce);
		approximateSpeedRaw = approximateSpeed;
		normalVecIndex = CTR_MipsSra(approximateSpeedRaw, VEH_PHYS_PROC_CLOCK_WADDLE_SPEED_SHIFT);
		if (normalVecIndex > VEH_PHYS_PROC_CLOCK_WADDLE_SPEED_MAX)
		{
			normalVecIndex = VEH_PHYS_PROC_CLOCK_WADDLE_SPEED_MAX;
		}
		clockWaddleTimer = CTR_MipsAddLo(clockWaddleTimer, normalVecIndex);
		CTR_PSX_KEEP_VALUE(clockWaddleTimer);

		GAMEPAD_ShockForce1(driver, VEH_PHYS_PROC_CLOCK_WADDLE_RUMBLE_CHANNEL, CTR_MipsAddLo(clockWaddleTimer, VEH_PHYS_PROC_CLOCK_WADDLE_FORCE_BASE));

		driverTimerNegativeFinal = scratchValue | 1;
		goto writeHazardTimer;

	airborneHazard:
		CTR_PSX_LOAD_STACK_WORD(approximateSpeedRaw, 32, approximateSpeed);
		trigForce = approximateSpeedRaw < VEH_PHYS_PROC_HAZARD_LOW_SPEED_THRESHOLD;
		CTR_PSX_KEEP_VALUE(trigForce);
		if (trigForce != 0)
		{
			goto speedIsLow;
		}
		airborneTimer = normalVecIndex;
		if (normalVecIndex < 0)
		{
			airborneTimer = CTR_MipsNegLo(airborneTimer);
		}
		scratchValue = CTR_MipsNegLo(airborneTimer);
		goto finalizeHazardTimer;

	speedIsLow:
		scratchValue = driver->hazardTimer;
		if (scratchValue > 0)
		{
			scratchValue = CTR_MipsNegLo(scratchValue);
		}

	finalizeHazardTimer:
		driverTimerNegativeFinal = scratchValue | 1;

	writeHazardTimer:
		driver->hazardTimer = (s16)driverTimerNegativeFinal;
	}
	goto hazardDone;

applyNormalHazard:
	approximateSpeedRaw = approximateSpeed;
	timerHazard = driver->hazardTimer;
	trigForce = approximateSpeedRaw < (VEH_PHYS_PROC_HAZARD_MOVING_SPEED_MIN + 1);
	CTR_PSX_KEEP_VALUE(trigForce);
	// Hazard timer will not go down unless you keep moving.
	if (trigForce == 0)
	{
		timerHazard = CTR_MipsAddLo(timerHazard, msPerFrameNeg);
	}

	timerHazard = timerHazard & VEH_PHYS_PROC_HAZARD_TIMER_EVEN_MASK;
	if (timerHazard > -1)
	{
		timerHazard = -2;
	}

	driver->hazardTimer = timerHazard;

hazardDone:


	// === Item Roll ===


	// if Held Item = None (rolling)
	if (driver->heldItemID == HELD_ITEM_ROULETTE)
	{
		normalVecIndex = driver->itemRollTimer;
		CTR_PSX_KEEP_VALUE(normalVecIndex);

		if (normalVecIndex != 0)
		{
			driver->itemRollTimer = (s16)CTR_MipsSubLo(normalVecIndex, 1);
			goto itemRollDone;
		}

		// Select a random weapon for driver.
		VehPhysGeneral_SetHeldItem(driver);

		VehPhysProc_PlayItemAcquireSound(driver);
	}
itemRollDone:
	CTR_PSX_KEEP_VALUE(normalVecIndex);


	// === Item Used By Player ===


	normalVecIndex = driver->noItemTimer;
	CTR_PSX_KEEP_VALUE(normalVecIndex);
	if (normalVecIndex != 0)
	{
		// if Item is about to be gone and Number of Items = 0
		if ((normalVecIndex == 1) && (driver->numHeldItems == 0))
		{
			VehPhysProc_RemoveHeldItem(driver);
		}

		driver->noItemTimer = (s16)CTR_MipsSubLo(normalVecIndex, 1);
	}
	CTR_PSX_KEEP_VALUE(normalVecIndex);

	if (driver->invincibleTimer != 0)
	{
		driver->invincibleTimer = CTR_MipsSubLo(driver->invincibleTimer, GAME_TRACKER->elapsedTimeMS);
		if (driver->invincibleTimer < 0)
		{
			driver->invincibleTimer = 0;
		}
	}

	// If invisible, without Permanent Invisibility cheat,
	// dont remove invisibleTimer check, or an invalid
	// instFlagsBackup overwrites instFlags
	if ((driver->invisibleTimer != 0) && ((GAME_TRACKER->gameMode2 & CHEAT_INVISIBLE) == 0) && !Chaos_IsTimeStopped())
	{
		driver->invisibleTimer = CTR_MipsSubLo(driver->invisibleTimer, GAME_TRACKER->elapsedTimeMS);

		if (driver->invisibleTimer < 0)
		{
			driver->invisibleTimer = 0;
		}
		CTR_PSX_MEMORY_BARRIER();

		// if newly visible
		if (driver->invisibleTimer == 0)
		{
			driver->instSelf->flags = driver->instFlagsBackup;
			driver->instSelf->alphaScale = 0;
			OtherFX_Play(VEH_PHYS_PROC_INVISIBLE_REAPPEAR_FX, 1);
		}
	}


	// === Normal Vector ===


	// action flags
	driver->actionsFlagSetPrevFrame = actionsFlagSetCopy;

	// NOTE(aalhendi): Back up the independent fields in retail store order.
	*(CtrPackedU32 *)&driver->rotPrev.x = CTR_ReadU32AlignedLE(&driver->rotCurr.x);
	driver->posPrev.x = driver->posCurr.x;
	driver->posPrev.y = driver->posCurr.y;
	driver->posPrev.z = driver->posCurr.z;
	driver->rotPrev.z = driver->rotCurr.z;
	driver->jumpHeightPrev = driver->jumpHeightCurr;
	driver->turnAnglePrev = driver->turnAngleCurr;

	// Preserve the subset of action flags that feed driving physics.
	actionsFlagSetCopy &= VEH_PHYS_PROC_ACTION_CARRY_MASK;

	// disable input if opening adv hub door with key
	if ((GAME_TRACKER->gameMode2 & GAME_MODE2_VEH_FREEZE_MASK) != 0)
	{
		driver->actionsFlagSet = actionsFlagSetCopy;
		return;
	}

	trigForce = (u8)driver->normalVecID;
	CTR_PSX_OBSERVE_VALUE(trigForce);
	normalVecIndex = trigForce + 1;
	if (normalVecIndex > 0)
	{
		normalVecIndex = 0;
	}
	normalDestination = (s16 *)((u8 *)&driver->AxisAngle4_normalVec + CTR_MipsSll(normalVecIndex, 3));

	if ((actionsFlagSetCopy & ACTION_TOUCH_GROUND) != 0)
	{
		*(CtrPackedU32 *)normalDestination = CTR_ReadU32AlignedLE(&driver->AxisAngle1_normalVec);
		normalDestination[2] = driver->AxisAngle1_normalVec.z;
	}
	else
	{
		*(CtrPackedU32 *)normalDestination = CTR_ReadU32AlignedLE(&driver->AxisAngle2_normalVec);
		normalDestination[2] = driver->AxisAngle2_normalVec.z;
	}
	driver->normalVecID = (s8)normalVecIndex;
	CTR_PSX_MEMORY_BARRIER();


	// === Check Mask Weapon ===


	{
		register int maskModelAku CTR_PSX_REGISTER("$2");
		register struct Thread *maskThread CTR_PSX_REGISTER("$4");
		int maskModelUka;
		u32 maskAction;

		maskThread = driverThread->childThread;
		if (maskThread == 0)
		{
			goto maskCheckDone;
		}
		maskModelUka = STATIC_UKAUKA;
		maskModelAku = STATIC_AKUAKU;
		maskAction = ACTION_MASK_WEAPON;

	maskCheckThread:
		normalVecIndex = maskThread->modelIndex;
		if (normalVecIndex == maskModelUka)
		{
			goto driverHasMask;
		}
		if (normalVecIndex != maskModelAku)
		{
			goto maskCheckNextThread;
		}

	driverHasMask:
		actionsFlagSetCopy |= maskAction;
		goto maskCheckDone;

	maskCheckNextThread:
		CTR_PSX_KEEP_VALUE(maskAction);
		maskThread = maskThread->siblingThread;
		if (maskThread != 0)
		{
			goto maskCheckThread;
		}

	maskCheckDone:
		(void)maskModelUka;
		(void)maskModelAku;
		(void)maskAction;
	}


	// === Check Buttons ===


	// pointer to gamepad input of current player (driver)
	struct GamepadBuffer drivingPad = GAMEPADS->gamepad[(u32)driver->driverID];
	Chaos_ApplyInputRules(driver, &drivingPad);
	ptrgamepad = &drivingPad;

	buttonsHeld = 0;
	buttonsTapped = 0;
	stickRY = VEH_PHYS_PROC_STICK_CENTER;
	gasStickLY = VEH_PHYS_PROC_STICK_CENTER;
	if ((GAME_TRACKER->gameMode1 & END_OF_RACE) == 0)
	{
		buttonsHeld = ptrgamepad->buttonsHeldCurrFrame;
		buttonsTapped = ptrgamepad->buttonsTapped;
		stickRY = ptrgamepad->stickRY;
		gasStickLY = ptrgamepad->stickLY;
	}

	approximateSpeedRaw = buttonsHeld & BTN_CROSS_one;
	cross = (u32)approximateSpeedRaw;
	approximateSpeedRaw = buttonsHeld & BTN_SQUARE_one;
	square = (u32)approximateSpeedRaw;
	CTR_PSX_OBSERVE_MEMORY(cross);
	CTR_PSX_OBSERVE_MEMORY(square);
	CTR_PSX_OBSERVE_MEMORY(approximateSpeed);

	// state of kart
	approximateSpeedRaw = driver->kartState;
	CTR_PSX_KEEP_VALUE(approximateSpeedRaw);
	kartState = approximateSpeedRaw;


	// === Check Weapons ===


	if ((((buttonsTapped & BTN_CIRCLE) != 0) &&

	     ((kartState == KS_NORMAL) || (kartState == KS_DRIFTING) || (kartState == KS_ANTIVSHIFT))) &&

	    // if there is no tnt on your head
	    (driver->instTntRecv == 0))
	{
		// If there is a Bomb Pointer
		if (driver->instBombThrow != 0)
		{
			// Detonate the bomb
			bomb = (struct TrackerWeapon *)driver->instBombThrow->thread->object;
			bomb->flags |= 2;
			driver->instBombThrow = NULL;
			goto CheckJumpButtons;
		}

		// If there is a Bubble Pointer
		if (driver->instBubbleHold != 0)
		{
			// Shoot the bubble
			shield = (struct Shield *)driver->instBubbleHold->thread->object;
			shield->flags |= SHIELD_FLAG_SHOOT;
			driver->instBubbleHold = NULL;
			goto CheckJumpButtons;
		}

		// An active roulette and every invalid fire request share the same
		// fast-stop check in the retail control flow.
		if (driver->itemRollTimer != 0)
		{
			goto CheckItemRollTimer;
		}

		// === Item Roll finished before PhysLinear ===

		// If you dont have roulette or no weapon,
		// and if you did not have a weapon last frame (0x3c->0),
		// and if raincloud item roll is not blocking weapon fire,
		// and if you are not being effected by Clock Weapon
		heldItemID = driver->heldItemID;
		if (heldItemID == HELD_ITEM_NONE)
		{
			goto CheckItemRollTimer;
		}
		if (heldItemID == HELD_ITEM_ROULETTE)
		{
			goto CheckItemRollTimer;
		}
		if (driver->noItemTimer != 0)
		{
			goto CheckItemRollTimer;
		}
		if (rainCloudEffect == RAIN_CLOUD_EFFECT_ITEM_ROLL)
		{
			goto CheckItemRollTimer;
		}
	    if (driver->clockReceive != 0  && !g_config.allowWeaponsDuringClock)
		{
			goto CheckItemRollTimer;
		}

		// NOTE(aalhendi): Retail keeps the item count in s0 through the cooldown
		// branches before reusing s0 for the jump-button mask.
		CTR_PSX_LOAD_UNSIGNED_BYTE(normalVecIndex, driver, offsetof(struct Driver, numHeldItems), driver->numHeldItems);
		// This driver wants to fire a weapon.
		actionsFlagSetCopy |= ACTION_WEAPON_FIRE_REQUEST;
		if (normalVecIndex != 0)
		{
			if (heldItemID == HELD_ITEM_SPRING)
			{
				if ((driver->jump_CoyoteTimerMS != 0) && (driver->jump_CooldownMS == 0))
				{
					nextHeldItemCount = normalVecIndex - 1;
					driver->numHeldItems = (u8)nextHeldItemCount;
				}
			}
			else if ((GAME_TRACKER->gameMode2 & (CHEAT_BOMBS | CHEAT_TURBO | CHEAT_MASK)) == 0)
			{
				nextHeldItemCount = normalVecIndex - 1;
				driver->numHeldItems = (u8)nextHeldItemCount;
			}
			driver->noItemTimer = VEH_PHYS_PROC_WEAPON_COOLDOWN_STACK_FRAMES;
		}
		else
		{
			driver->noItemTimer = VEH_PHYS_PROC_WEAPON_COOLDOWN_EMPTY_FRAMES;
		}
		goto CheckJumpButtons;
	}
	goto CheckJumpButtons;

CheckItemRollTimer:
	normalVecIndex = buttonsTapped & VEH_PHYS_PROC_JUMP_BUTTON_MASK;
	if (driver->itemRollTimer >= VEH_PHYS_PROC_ITEM_ROLL_FAST_STOP_FRAMES)
	{
		goto CheckJumpButtonsReady;
	}
	driver->itemRollTimer = 0;

CheckJumpButtons:
	// Check for Tapping L1 and R1
	normalVecIndex = buttonsTapped & VEH_PHYS_PROC_JUMP_BUTTON_MASK;

CheckJumpButtonsReady:
	actionsFlagSetCopy = VehPhysProc_JumpInput(driver, (u32)normalVecIndex, buttonsHeld, rainCloudEffect, actionsFlagSetCopy);
	steeringInput = ptrgamepad->stickLX;
	// Apply after input resolution, before jump and steering consumers use the result.
	actionsFlagSetCopy = Chaos_ApplyDriver(driver, actionsFlagSetCopy);
	CTR_PSX_OBSERVE_VALUE(buttonsHeld);
	CTR_PSX_LOAD_WORD(approximateSpeedRaw, square);
	if (
	    // If you are holding Square
	    (approximateSpeedRaw != 0) &&

	    // if you're not on any turbo pad
	    ((driver->stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD_MASK) == 0))
	{
		// Set Reserves to zero
		VehFire_SetReserves(driver, 0);
	}

	if (
	    // If Reserves are not zero
	    (driver->reserves != 0) ||

	    (rainCloudEffect == RAIN_CLOUD_EFFECT_RESERVE_RELEASE))
	{
		CTR_PSX_LOAD_WORD(approximateSpeedRaw, cross);
		// If you are not holding Cross
		if (approximateSpeedRaw == 0)
		{
			register int gasStickArg CTR_PSX_REGISTER("$4");
			register int gasCenterArg CTR_PSX_REGISTER("$5");
			register struct RacingWheelData *gasWheelArg CTR_PSX_REGISTER("$6");

			gasStickArg = stickRY;
			gasCenterArg = VEH_PHYS_PROC_STICK_CENTER;
			gasWheelArg = NULL;
			trigForce = CTR_MipsNegLo(VehPhysJoystick_ReturnToRest(gasStickArg, gasCenterArg, gasWheelArg));
			CTR_PSX_KEEP_VALUE(trigForce);

			if (trigForce < 1)
			{
				actionsFlagSetCopy |= ACTION_ACCEL_RELEASED_WITH_RESERVES;
			}
		}

		CTR_PSX_LOAD_WORD(approximateSpeedRaw, square);
		// If holding Square while moving fast
		if (approximateSpeedRaw != 0)
		{
			CTR_PSX_LOAD_WORD(approximateSpeedRaw, approximateSpeed);
			trigForce = approximateSpeedRaw < (VEH_PHYS_PROC_BRAKE_HIGH_SPEED_THRESHOLD + 1);
			CTR_PSX_KEEP_VALUE(trigForce);
			if (trigForce == 0)
			{
				// back wheel skids
				actionsFlagSetCopy |= ACTION_BACK_SKID;
			}
		}

		trigForce = driver->stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD_MASK;
		// if you're on any turbo pad
		if (trigForce != 0)
		{
			// assume not holding square until boost is over
			square = 0;
		}
		approximateSpeedRaw = VEH_PHYS_PROC_ASSUMED_CROSS_BUTTON;
		CTR_PSX_KEEP_VALUE(approximateSpeedRaw);

		// Assume you're holding Cross, because
		// you have Reserves and you aren't slowing down
		cross = (u32)approximateSpeedRaw;
	}


	// === Gas/Brake section ===

	{
		register int gasTargetSpeed CTR_PSX_REGISTER("$19");
		register int gasScratchValue CTR_PSX_REGISTER("$16");

		if (((s8)driver->simpTurnState < 0) || (actionsFlagSetCopy &= ~ACTION_REVERSE_STEER_LEFT, (s8)driver->simpTurnState < 1))
		{
			actionsFlagSetCopy &= ~ACTION_REVERSE_STEER_RIGHT;
		}
		trigForce = driver->speedApprox;
		CTR_PSX_KEEP_VALUE(trigForce);
		if (trigForce < 0)
		{
			trigForce = CTR_MipsNegLo(trigForce);
		}
		if (trigForce < 0x300)
		{
			actionsFlagSetCopy &= ~(ACTION_REVERSE_STEER_LEFT | ACTION_REVERSE_STEER_RIGHT);
		}
		gasTargetSpeed = 0;
		CTR_PSX_KEEP_VALUE(gasTargetSpeed);

		// with zero wumpa, should be const_Speed_ClassStat (13140 for Crash Bandicoot)
		// this works for both decomp and original

		// with one wumpa, retail gives 13169

		trigForce = VehPhysGeneral_GetBaseSpeed(driver);

		CTR_PSX_LOAD_WORD(approximateSpeedRaw, square);
		targetBaseSpeed = trigForce;
		if (approximateSpeedRaw == 0)
		{
			goto gasNoBrake;
		}

		gasScratchValue = VehPhysJoystick_ReturnToRest(gasStickLY, VEH_PHYS_PROC_STICK_CENTER, NULL);
		if (gasScratchValue >= VEH_PHYS_PROC_REVERSE_STICK_THRESHOLD)
		{
			goto gasBrakeReverse;
		}
		if (gasScratchValue <= 0)
		{
			goto gasSquareCross;
		}
		if ((actionsFlagSetCopy & ACTION_REVERSING_ENGINE) == 0)
		{
			goto gasSquareCross;
		}

	gasBrakeReverse:
		trigForce = CTR_MipsNegLo(driver->const_BackwardSpeed);
		tireSpeedRaw = CTR_MipsSll(trigForce, 1);
		CTR_PSX_OBSERVE_VALUE(tireSpeedRaw);
		trigForce = CTR_MipsAddLo(tireSpeedRaw, trigForce);
		gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_BRAKE_REVERSE_SCALE_SHIFT);
		if (trigForce < 0)
		{
			trigForce = CTR_MipsAddLo(trigForce, VEH_PHYS_PROC_BRAKE_REVERSE_ROUNDING_BIAS);
			CTR_PSX_OBSERVE_VALUE(trigForce);
			gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_BRAKE_REVERSE_SCALE_SHIFT);
		}
		goto gasReverseWithBrake;

	gasSquareCross:
		CTR_PSX_LOAD_WORD(approximateSpeedRaw, cross);
		if (approximateSpeedRaw == 0)
		{
			goto gasSquareNoCross;
		}
		trigForce = (u32)targetBaseSpeed >> 31;
		trigForce = CTR_MipsAddLo(targetBaseSpeed, trigForce);
		CTR_PSX_OBSERVE_VALUE(trigForce);
		gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_HALF_SPEED_SHIFT);
		actionsFlagSetCopy |= ACTION_BRAKE_WITH_ACCEL;
		goto gasClearReversing;

	gasSquareNoCross:
		CTR_PSX_FORGET_VALUE(approximateSpeedRaw);
		trigForce = VehPhysJoystick_ReturnToRest(stickRY, VEH_PHYS_PROC_STICK_CENTER, 0);
		CTR_PSX_NEGATE(gasScratchValue, trigForce);
		if (gasScratchValue <= 0)
		{
			goto gasSquareNonPositive;
		}
		trigForce = CTR_MipsMulLo(targetBaseSpeed, gasScratchValue);
		gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_BRAKE_SPEED_SCALE_SHIFT);
		if (trigForce < 0)
		{
			trigForce = CTR_MipsAddLo(trigForce, VEH_PHYS_PROC_BRAKE_SPEED_ROUNDING_BIAS);
			CTR_PSX_OBSERVE_VALUE(trigForce);
			gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_BRAKE_SPEED_SCALE_SHIFT);
		}
		actionsFlagSetCopy |= ACTION_BRAKE_WITH_ACCEL;
		goto gasClearSteerFlags;

	gasSquareNonPositive:
		if (gasScratchValue >= 0)
		{
			goto gasBrakeOnly;
		}
		trigForce = CTR_MipsMulLo(driver->const_BackwardSpeed, gasScratchValue);
		gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_BRAKE_SPEED_SCALE_SHIFT);
		if (trigForce < 0)
		{
			trigForce = CTR_MipsAddLo(trigForce, VEH_PHYS_PROC_BRAKE_SPEED_ROUNDING_BIAS);
			CTR_PSX_OBSERVE_VALUE(trigForce);
			gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_BRAKE_SPEED_SCALE_SHIFT);
		}

	gasReverseWithBrake:
		actionsFlagSetCopy |= ACTION_REVERSING_ENGINE | ACTION_BRAKE_WITH_ACCEL;
		goto gasClearSteerFlags;

	gasBrakeOnly:
		actionsFlagSetCopy |= ACTION_ACCEL_PREVENTION;
		goto gasClearReversing;

	gasNoBrake:
		CTR_PSX_LOAD_WORD(approximateSpeedRaw, cross);
		buttonsTapped = (u32)stickRY;
		if (approximateSpeedRaw == 0)
		{
			goto gasNoBrakeNoCross;
		}
		CTR_PSX_FORGET_VALUE(targetBaseSpeed);
		gasTargetSpeed = targetBaseSpeed;
		goto gasClearReversing;

	gasNoBrakeNoCross:
		CTR_PSX_FORGET_VALUE(approximateSpeedRaw);
		trigForce = VehPhysJoystick_ReturnToRest((int)buttonsTapped, VEH_PHYS_PROC_STICK_CENTER, 0);
		CTR_PSX_NEGATE(gasScratchValue, trigForce);
		if (gasScratchValue < 0)
		{
			goto gasPositiveStick;
		}
		if (gasScratchValue != 0)
		{
			goto gasScaleBaseSpeed;
		}

		CTR_PSX_FORGET_VALUE(gasScratchValue);
		centeredStick = VehPhysJoystick_ReturnToRest(gasStickLY, VEH_PHYS_PROC_STICK_CENTER, 0);
		if (centeredStick >= VEH_PHYS_PROC_REVERSE_STICK_THRESHOLD)
		{
			goto gasReverseFromNeutral;
		}
		CTR_PSX_CLOBBER("$2");
		trigForce = ACTION_REVERSING_ENGINE;
		if (centeredStick <= 0)
		{
			goto gasScaleBaseSpeed;
		}
		if ((actionsFlagSetCopy & (u32)trigForce) == 0)
		{
			goto gasScaleBaseSpeed;
		}

	gasReverseFromNeutral:
		tireSpeedRaw = driver->const_BackwardSpeed;
		CTR_PSX_OBSERVE_VALUE(tireSpeedRaw);
		trigForce = ACTION_REVERSING_ENGINE;
		actionsFlagSetCopy |= (u32)trigForce;
		gasTargetSpeed = CTR_MipsNegLo(tireSpeedRaw);
		goto gasClearSteerFlags;

	gasScaleBaseSpeed:
		trigForce = CTR_MipsMulLo(targetBaseSpeed, gasScratchValue);
		gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_REVERSE_SPEED_SCALE_SHIFT);
		if (trigForce < 0)
		{
			trigForce = CTR_MipsAddLo(trigForce, VEH_PHYS_PROC_REVERSE_SPEED_ROUNDING_BIAS);
			CTR_PSX_OBSERVE_VALUE(trigForce);
			gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_REVERSE_SPEED_SCALE_SHIFT);
		}

	gasClearReversing:
		actionsFlagSetCopy &= ~ACTION_REVERSING_ENGINE;

	gasClearSteerFlags:
		actionsFlagSetNext = actionsFlagSetCopy & ~(ACTION_REVERSE_STEER_LEFT | ACTION_REVERSE_STEER_RIGHT);
		goto gasDone;

	gasPositiveStick:
		if (driver->speedApprox >= VEH_PHYS_PROC_REVERSE_SPEED_COMPARE)
		{
			goto gasReverseSteering;
		}
		if ((actionsFlagSetCopy & (ACTION_REVERSE_STEER_LEFT | ACTION_REVERSE_STEER_RIGHT)) == 0)
		{
			goto gasPositiveReverseSpeed;
		}

	gasReverseSteering:
		actionsFlagSetCopy |= ACTION_ACCEL_PREVENTION;
		centeredStick = (s8)driver->simpTurnState;
		if (centeredStick > 0)
		{
			actionsFlagSetCopy |= ACTION_REVERSE_STEER_RIGHT;
		}
		if (centeredStick < 0)
		{
			trigForce = ACTION_REVERSE_STEER_LEFT;
			goto gasCombineAction;
		}
		goto gasDone;

	gasPositiveReverseSpeed:
		trigForce = CTR_MipsMulLo(driver->const_BackwardSpeed, gasScratchValue);
		gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_REVERSE_SPEED_SCALE_SHIFT);
		if (trigForce < 0)
		{
			trigForce = CTR_MipsAddLo(trigForce, VEH_PHYS_PROC_REVERSE_SPEED_ROUNDING_BIAS);
			CTR_PSX_OBSERVE_VALUE(trigForce);
			gasTargetSpeed = CTR_MipsSra(trigForce, VEH_PHYS_PROC_REVERSE_SPEED_SCALE_SHIFT);
		}
		trigForce = ACTION_REVERSING_ENGINE;

	gasCombineAction:
		actionsFlagSetCopy |= (u32)trigForce;

	gasDone:
		actionsFlagSetNext = actionsFlagSetCopy;

		// driving backwards
		if ((actionsFlagSetNext & ACTION_REVERSING_ENGINE) != 0)
		{
			driver->timeSpentReversing = CTR_MipsSubLo(driver->timeSpentReversing, msPerFrameNeg);
		}

		// not driving backwards
		else
		{
			if (driver->superEngineTimer != 0)
			{
				// if Racer is moving
				if (0 < gasTargetSpeed)
				{
					// not holding brake
					if ((actionsFlagSetNext & (ACTION_ACCEL_RELEASED_WITH_RESERVES | ACTION_BRAKE_WITH_ACCEL)) == 0)
					{
						driver->actionsFlagSet = actionsFlagSetNext;

						// fire level, depending on numWumpa
						superEngineFireLevel = VEH_PHYS_PROC_SUPER_ENGINE_BASE_FIRE;
						if (driver->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
						{
							superEngineFireLevel = VEH_PHYS_PROC_SUPER_ENGINE_JUICED_FIRE;
						}

						// add 0.12s reserves
						VehFire_Increment(driver, VEH_PHYS_PROC_SUPER_ENGINE_RESERVES, (TURBO_PAD | SUPER_ENGINE), superEngineFireLevel);

						actionsFlagSetNext = driver->actionsFlagSet;
					}
				}
			}
		}

		// if accel prevention (hold square)
		trigForce = (int)(actionsFlagSetNext & ACTION_ACCEL_PREVENTION);
		if (trigForce != 0)
		{
			CTR_PSX_LOAD_SIGNED_HALF(trigForce, driver, offsetof(struct Driver, speedApprox), driver->speedApprox);
			if (trigForce < 0)
			{
				CTR_PSX_NEGATE_IN_PLACE(trigForce);
			}
			if (trigForce > VEH_PHYS_PROC_BRAKE_HIGH_SPEED_THRESHOLD)
			{
				// record amount of time with high speed
				driver->timeSpentWithHighSpeed = CTR_MipsSubLo(driver->timeSpentWithHighSpeed, msPerFrameNeg);
			}
		}

		if ((driver->accelTapWindowTimer == 0) ||

		    ((driver->kartState != KS_NORMAL) && (driver->kartState != KS_ANTIVSHIFT)))
		{
			driver->accelTapCount = 0;
		}

		if (driver->fireSpeed <= 0)
		{
			goto fireSpeedNonPositive;
		}
		if (gasTargetSpeed <= 0)
		{
			goto fireSpeedDirectionChanged;
		}
		driver->fireSpeed = (s16)gasTargetSpeed;
		goto fireSpeedDone;

	fireSpeedNonPositive:
		if (gasTargetSpeed <= 0)
		{
			goto storeFireSpeed;
		}

	fireSpeedDirectionChanged:
		if (driver->accelTapWindowTimer != 0)
		{
			driver->accelTapCount = (s16)CTR_MipsAddLo((u16)driver->accelTapCount, 1);
		}
		driver->accelTapWindowTimer = DRIVER_ACCEL_TAP_WINDOW_MS;

	storeFireSpeed:
		driver->fireSpeed = (s16)gasTargetSpeed;

	fireSpeedDone:

		// brakes
		if ((actionsFlagSetNext & (ACTION_MASK_WEAPON | ACTION_BRAKE_WITH_ACCEL)) == 0)
		{
			gasScratchValue = driver->terrainMeta2->speedMultiplier;

			if (gasScratchValue != VEH_PHYS_PROC_TERRAIN_SPEED_NEUTRAL)
			{
				// Base Speed = 0xB4 (at Cove water) * Base Speed >> 8
				tireSpeedRaw = CTR_MipsMulLo(gasScratchValue, gasTargetSpeed);
				CTR_PSX_OBSERVE_VALUE(tireSpeedRaw);
				gasTargetSpeed = CTR_MipsSra(tireSpeedRaw, VEH_PHYS_PROC_TERRAIN_SPEED_SHIFT);
				approxTrig = CTR_MipsMulLo(gasScratchValue, targetBaseSpeed);
				targetBaseSpeed = CTR_MipsSra(approxTrig, VEH_PHYS_PROC_TERRAIN_SPEED_SHIFT);
			}
		}
		driver->terrainScaledBaseSpeed = (s16)targetBaseSpeed;
		driver->baseSpeed = (s16)gasTargetSpeed;


		// === Steering Section ===


		// assume neutral steer (drive straight)
		gasScratchValue = VEH_PHYS_PROC_STICK_CENTER;

		// If you're not in End-Of-Race menu
		if ((GAME_TRACKER->gameMode1 & END_OF_RACE) == 0)
		{
			// gamepadBuffer -> stickLX
			gasScratchValue = steeringInput;
		}

		// default steer strength from class stats
		steerStrength = CTR_MipsAddLo(driver->const_TurnRate,
		                              CTR_MipsSll((s8)driver->turnConst, VEH_PHYS_PROC_STEER_TURN_CONST_SHIFT) / VEH_PHYS_PROC_STEER_TURN_CONST_DIVISOR);

		// if mashing X button
		if (driver->accelTapCount < DRIVER_ACCEL_TAP_STEER_COUNT)
		{
			goto checkWallSteering;
		}
		CTR_PSX_LOAD_WORD(approximateSpeedRaw, approximateSpeed);
		trigForce = approximateSpeedRaw < VEH_PHYS_PROC_STEER_ACCEL_TAP_SPEED_MAX;
		CTR_PSX_OBSERVE_VALUE(trigForce);
		if (trigForce != 0)
		{
			steerStrength = CTR_MipsNegLo(VehPhysJoystick_GetStrengthAbsolute(gasScratchValue, VEH_PHYS_PROC_STEER_ACCEL_TAP_STRENGTH, ptrgamepad->rwd));
			goto steerStrengthReady;
		}

	checkWallSteering:
		// rubbing on wall now, or recently
		if (driver->wallRubTimer != 0)
		{
			steerStrength = CTR_MipsNegLo(VehPhysJoystick_GetStrengthAbsolute(gasScratchValue, VEH_PHYS_PROC_STEER_WALL_RUB_STRENGTH, ptrgamepad->rwd));
			goto steerStrengthReady;
		}

		// === not rubbing on wall now, or recently ===

		// if not holding Square (& 0x8)
		// or not using brakes (& 0x20)
		if ((actionsFlagSetNext & (ACTION_ACCEL_PREVENTION | ACTION_BRAKE_WITH_ACCEL)) == 0)
		{
			goto useDefaultTurnRate;
		}

		// if only holding Square
		CTR_PSX_LOAD_WORD(approximateSpeedRaw, cross);
		if (approximateSpeedRaw == 0)
		{
			goto useBrakeTurnRate;
		}

		// === if holding Square + Cross ===
		{
			register int mapInputMax CTR_PSX_REGISTER("$5");
			register int mapClassSpeed CTR_PSX_REGISTER("$6");
			register int mapOutputMin CTR_PSX_REGISTER("$7");
			mapInputMax = VEH_PHYS_PROC_BRAKE_HIGH_SPEED_THRESHOLD;
			CTR_PSX_OBSERVE_VALUE(mapInputMax);
			mapClassSpeed = (u16)driver->const_Speed_ClassStat;

			// absolute value driver speed
			driverSpeedCopy = driver->speed;
			mapOutputMin = VEH_PHYS_PROC_STEER_BRAKE_STRENGTH;
			if (driverSpeedCopy < 0)
			{
				driverSpeedCopy = CTR_MipsNegLo(driverSpeedCopy);
			}

// As speed increases, turn rate decreases
			steerStrength = VEH_MAP_TO_RANGE_STAGED_FIFTH(driverSpeedCopy, mapInputMax, CTR_MipsSra(CTR_MipsSll(mapClassSpeed, 16), 17), mapOutputMin, steerStrength);
			steerStrength = CTR_MipsNegLo(VehPhysJoystick_GetStrengthAbsolute(gasScratchValue, steerStrength, ptrgamepad->rwd));
		}
		goto steerStrengthReady;

	useBrakeTurnRate:
		steerStrength = CTR_MipsNegLo(VehPhysJoystick_GetStrengthAbsolute(gasScratchValue, VEH_PHYS_PROC_STEER_BRAKE_STRENGTH, ptrgamepad->rwd));
		goto steerStrengthReady;

	useDefaultTurnRate:
		steerStrength = CTR_MipsNegLo(VehPhysJoystick_GetStrengthAbsolute(gasScratchValue, steerStrength, ptrgamepad->rwd));

	steerStrengthReady:

		// no desired steer
		if (steerStrength == 0)
		{
			driver->numFramesSpentSteering = VEH_PHYS_PROC_STEER_RESET_FRAMES;
			goto storeSteerState;
		}

		if (steerStrength >= 0)
		{
			goto steerPositive;
		}
		if ((s8)driver->simpTurnState < 0)
		{
			goto steerPositive;
		}
		actionsFlagSetNext &= ~ACTION_STEER_LEFT;
		goto resetSteeringTimer;

	steerPositive:
		if (steerStrength <= 0)
		{
			goto storeSteerState;
		}
		if ((s8)driver->simpTurnState > 0)
		{
			goto storeSteerState;
		}
		actionsFlagSetNext |= ACTION_STEER_LEFT;

	resetSteeringTimer:
		driver->numFramesSpentSteering = 0;

	storeSteerState:
		driver->simpTurnState = (s8)steerStrength;

		// Change wheel rotation based on StickLX
		gasScratchValue = VehPhysJoystick_GetStrengthAbsolute(gasScratchValue, VEH_PHYS_PROC_WHEEL_ROTATION_STRENGTH, ptrgamepad->rwd);
		driver->wheelRotation =
		    (s16)VehCalc_InterpBySpeed((int)driver->wheelRotation, VEH_PHYS_PROC_WHEEL_ROTATION_INTERP_STEP, CTR_MipsNegLo(gasScratchValue));

		CTR_PSX_LOAD_SIGNED_HALF(tireSpeedRaw, driver, offsetof(struct Driver, fireSpeed), driver->fireSpeed);
		trigForce = (int)driver->actionsFlagSetPrevFrame;
		gasScratchValue = tireSpeedRaw;
		if (tireSpeedRaw < 0)
		{
			CTR_PSX_NEGATE_IN_PLACE(gasScratchValue);
		}

		if ((trigForce & ACTION_TOUCH_GROUND) == 0)
		{
			goto addTireColorSpeedBonus;
		}

		approximateSpeedRaw = kartState;
		if (approximateSpeedRaw == KS_DRIFTING)
		{
			goto addTireColorSpeedBonus;
		}

		tireApproximateSpeed = approximateSpeed;
		CTR_PSX_KEEP_VALUE(tireApproximateSpeed);
		trigForce = CTR_MipsAddLo(gasScratchValue, tireApproximateSpeed);
		CTR_PSX_OBSERVE_VALUE(trigForce);
		gasScratchValue = CTR_MipsSra(trigForce, VEH_PHYS_PROC_TIRE_COLOR_SPEED_AVERAGE_SHIFT);
		goto blendTireColorSpeed;

	addTireColorSpeedBonus:
		gasScratchValue = CTR_MipsAddLo(gasScratchValue, DRIVER_TIRE_COLOR_SPEED_AIRBORNE_BONUS);

	blendTireColorSpeed:
		tireColorWeight = driver->tireColorCycleStep * DRIVER_TIRE_COLOR_STEP_WEIGHT;
		tireSpeedWeight = gasScratchValue * DRIVER_TIRE_COLOR_SPEED_WEIGHT;
		tireColorStep = ((tireSpeedWeight + tireColorWeight) * (1 << VEH_PHYS_PROC_TIRE_COLOR_STEP_BLEND_SHIFT)) >> VEH_PHYS_PROC_TIRE_COLOR_STEP_RESULT_SHIFT;
		driver->tireColorCycleStep = tireColorStep;

		if ((driver->actionsFlagSetPrevFrame & ACTION_ACCEL_PREVENTION) == 0)
		{
			trigForce = gasTargetSpeed;

			// prevent Basic Speed from being negative
			if (gasTargetSpeed < 0)
			{
				CTR_PSX_NEGATE_IN_PLACE(trigForce);
			}

			if (trigForce > DRIVER_TIRE_COLOR_LOW_SPEED_THRESHOLD)
			{
				goto decrementTireColorTimer;
			}

			approximateSpeedRaw = approximateSpeed;
			trigForce = approximateSpeedRaw <= DRIVER_TIRE_COLOR_LOW_SPEED_THRESHOLD;
			if (trigForce != 0)
			{
				goto tireColorTimerDone;
			}

		decrementTireColorTimer:
			driver->tireColorCycleTimer = (s16)CTR_MipsSubLo((u16)driver->tireColorCycleTimer, tireColorStep);
		}

	tireColorTimerDone:
		// alternate tire colors each frame,
		// if 2e808080 is detected (&1==0),
		// if not RevEngine, and if tire color timer expired
		if ((driver->tireColorCycleTimer < 1) && ((driver->tireColor & 1) == 0))
		{
			CTR_PSX_LOAD_WORD_VOLATILE(approximateSpeedRaw, kartState);
			if (approximateSpeedRaw != KS_ENGINE_REVVING)
			{
				goto setDarkTireColor;
			}
		}

		// default tire color
		driver->tireColor = DRIVER_TIRE_COLOR_DEFAULT;
		goto setTireColorDone;

	setDarkTireColor:
		driver->tireColorCycleTimer = DRIVER_TIRE_COLOR_TIMER_RESET;
		driver->tireColor = DRIVER_TIRE_COLOR_DARK;

	setTireColorDone:

		driver->actionsFlagSet = actionsFlagSetNext;
	}
	return;
}


void VehPhysProc_Driving_Audio(struct Thread *t, struct Driver *d)
{
	(void)t;
	EngineSound_Player(d);
}


void VehPhysProc_Driving_Update(struct Thread *t, struct Driver *d)
{
	u8 turnRate;
	struct GamepadBuffer drivingPad = GAMEPADS->gamepad[d->driverID];
	Chaos_ApplyInputRules(d, &drivingPad);

	// if racer touched the ground in this frame
	if ((d->actionsFlagSet & ACTION_STARTED_TOUCH_GROUND) != 0)
	{
		int signedTurnConst;
		int turnConstTerm;
		register int simpTurnState CTR_PSX_REGISTER("$3");

		signedTurnConst = (s8)d->turnConst;
		turnConstTerm = (signedTurnConst * (1 << VEH_PHYS_PROC_STEER_TURN_CONST_SHIFT)) / VEH_PHYS_PROC_STEER_TURN_CONST_DIVISOR;
		simpTurnState = (s8)d->simpTurnState;
		turnRate = d->const_TurnRate;

		// set Simplified Turning state to its own absolute value
		if (simpTurnState < 0)
		{
			simpTurnState = -simpTurnState;
		}

		// if steering hard enough to start a drift
		if (((((turnRate + turnConstTerm) >> VEH_PHYS_PROC_HALF_SPEED_SHIFT) < simpTurnState) &&

		     // player has jump buttons held
		     (drivingPad.buttonsHeldCurrFrame & d->buttonUsedToStartDrift) != 0) &&

		    // player is not in accel prevention or braking and
		    ((d->actionsFlagSet & ACTION_ACCEL_PREVENTION) == 0) && ((d->const_Speed_ClassStat >> 1) <= d->speedApprox))
		{
			VehPhysProc_PowerSlide_Init(t, d);

			// exit the function
			return;
		}
	}

	// At this point, assume driver is not touching ground.
	// Wait until the V-shift startup guard has elapsed.
	if ((d->vShiftStartGuardTimer == 0) &&

	    // if V_Shift happened too many times,
	    // meaning you jitter between two quadblocks
	    // in a "V" shape
	    (d->vShiftCount >= VEH_PHYS_PROC_VSHIFT_MAX_COUNT))
	{
		// Stop driving, until you press X, prevents jitters
		VehPhysProc_FreezeVShift_Init(t, d);
	}

	else
	{
		// If the V-shift window expires, restart the count.
		if (d->vShiftWindowTimer == 0)
		{
			// wipe
			d->vShiftCount = 0;
		}
	}
}


#include "VehPhysProc_TakeControl.c"

void VehPhysProc_Driving_Init(struct Thread *t, struct Driver *d)
{
	if (Chaos_IsWaitingForStart(d))
	{
		VehStuckProc_RevEngine_Init(t, d);
		return;
	}

	if (((u32)(GAME_TRACKER->levelID - GEM_STONE_VALLEY) >= 5) || LOAD_IsOpen_AdvHub())
	{
		if (((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0) && (d->kartState == KS_BLASTED))
		{
			d->invincibleTimer = VEH_PHYS_PROC_BATTLE_BLASTED_INVINCIBLE_TIMER;
		}

		d->vShiftStartGuardTimer = VEH_PHYS_PROC_VSHIFT_START_GUARD_TIMER;
		d->vShiftWindowTimer = VEH_PHYS_PROC_VSHIFT_WINDOW_TIMER;

		d->funcPtrs[DRIVER_FUNC_UPDATE] = VehPhysProc_Driving_Update;
		d->funcPtrs[DRIVER_FUNC_PHYS_LINEAR] = VehPhysProc_Driving_PhysLinear;
		d->funcPtrs[DRIVER_FUNC_AUDIO] = VehPhysProc_Driving_Audio;
		d->funcPtrs[DRIVER_FUNC_PHYS_ANGULAR] = VehPhysGeneral_PhysAngular;
		d->funcPtrs[DRIVER_FUNC_APPLY_FORCES] = VehPhysForce_OnApplyForces;
		d->funcPtrs[DRIVER_FUNC_COLL_MOVED] = COLL_MOVED_PlayerSearch;
		d->funcPtrs[DRIVER_FUNC_COLLIDE_DRIVERS] = VehPhysForce_CollideDrivers;
		d->funcPtrs[DRIVER_FUNC_COLL_FIXED] = COLL_FIXED_PlayerSearch;
		d->funcPtrs[DRIVER_FUNC_JUMP_FRICTION] = VehPhysGeneral_JumpAndFriction;
		d->funcPtrs[DRIVER_FUNC_TRANSLATE_MATRIX] = VehPhysForce_TranslateMatrix;
		d->funcPtrs[DRIVER_FUNC_ANIMATE] = VehFrameProc_Driving;

		// must put this HERE, so that
		// the above IF-statement works
		d->kartState = KS_NORMAL;

		// Turbo meter = full
		d->turbo_MeterRoomLeft = 0;
		d->vShiftCount = 0;
		d->funcPtrs[DRIVER_FUNC_INIT] = NULL;
		d->funcPtrs[DRIVER_FUNC_PARTICLES] = VehEmitter_DriverMain;
	}
}


void VehPhysProc_FreezeEndEvent_PhysLinear(struct Thread *t, struct Driver *d)
{
	s32 jumpTenBuffer;
	s32 actionFlagsClearMask;

	VehPhysProc_Driving_PhysLinear(t, d);
	actionFlagsClearMask = ~ACTION_JUMP_BUTTON_HELD;
	jumpTenBuffer = d->jump_TenBuffer;

	// reset two speed variables
	d->baseSpeed = 0;
	d->fireSpeed = 0;
	d->ampTurnState = 0;
	d->simpTurnState = 0;
	d->wheelRotation = 0;

	// edit flags
	d->actionsFlagSet |= ACTION_ACCEL_PREVENTION;
	CTR_PSX_OBSERVE_MEMORY(d->actionsFlagSet);
	d->actionsFlagSet &= actionFlagsClearMask;

	if (jumpTenBuffer > 0)
	{
		d->jump_TenBuffer = 0;
	}
}


void VehPhysProc_FreezeEndEvent_Init(struct Thread *t, struct Driver *d)
{
	(void)t;
	if (d->kartState == KS_FREEZE)
	{
		return;
	}

	d->funcPtrs[DRIVER_FUNC_PHYS_LINEAR] = VehPhysProc_FreezeEndEvent_PhysLinear;
	d->funcPtrs[DRIVER_FUNC_AUDIO] = VehPhysProc_Driving_Audio;
	d->funcPtrs[DRIVER_FUNC_PHYS_ANGULAR] = VehPhysGeneral_PhysAngular;
	d->funcPtrs[DRIVER_FUNC_APPLY_FORCES] = VehPhysForce_OnApplyForces;
	d->funcPtrs[DRIVER_FUNC_COLL_MOVED] = COLL_MOVED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_COLLIDE_DRIVERS] = VehPhysForce_CollideDrivers;
	d->funcPtrs[DRIVER_FUNC_COLL_FIXED] = COLL_FIXED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_JUMP_FRICTION] = VehPhysGeneral_JumpAndFriction;
	d->funcPtrs[DRIVER_FUNC_TRANSLATE_MATRIX] = VehPhysForce_TranslateMatrix;
	d->funcPtrs[DRIVER_FUNC_ANIMATE] = VehFrameProc_Driving;

	d->kartState = KS_FREEZE;
	d->speed = 0;
	d->speedApprox = 0;
	d->funcPtrs[DRIVER_FUNC_INIT] = NULL;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = NULL;
	d->funcPtrs[DRIVER_FUNC_PARTICLES] = VehEmitter_DriverMain;
}


void VehPhysProc_FreezeVShift_Update(struct Thread *t, struct Driver *d)
{
	// if firespeed = 0 and
	// player is not in accel prevention (4),
	// not pressing Square (4, 6),
	if ((d->fireSpeed != 0) ||
	    // in player-on-player collision, accel prevention, or braking
	    ((d->actionsFlagSet & (ACTION_HUMAN_HUMAN_COLLISION | ACTION_ACCEL_PREVENTION | ACTION_BRAKE_WITH_ACCEL)) != 0))
	{
		VehPhysProc_Driving_Init(t, d);
	}
	else
	{
		// stop kart
		d->speed = 0;
		d->speedApprox = 0;
	}
}


void VehPhysProc_FreezeVShift_ReverseOneFrame(struct Thread *t, struct Driver *d)
{
	int actionFlagSet;

	VehPhysGeneral_JumpAndFriction(t, d);

	actionFlagSet = d->actionsFlagSet;

	// if player started jumping this frame
	if ((actionFlagSet & ACTION_JUMP_STARTED) != 0)
	{
		VehPhysProc_Driving_Init(t, d);
	}
	// if there are not two humans colliding
	else if ((actionFlagSet & ACTION_HUMAN_HUMAN_COLLISION) == 0)
	{
		d->xSpeed = 0;
		d->ySpeed = 0;
		d->zSpeed = 0;

		d->speed = 0;
		d->speedApprox = 0;

		// set position to previous position
		d->posCurr.x = d->posPrev.x;
		d->posCurr.y = d->posPrev.y;
		d->posCurr.z = d->posPrev.z;
	}
}


void VehPhysProc_FreezeVShift_Init(struct Thread *t, struct Driver *d)
{
	(void)t;
	d->kartState = KS_ANTIVSHIFT;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = VehPhysProc_FreezeVShift_Update;
	d->funcPtrs[DRIVER_FUNC_PHYS_LINEAR] = VehPhysProc_Driving_PhysLinear;
	d->funcPtrs[DRIVER_FUNC_AUDIO] = VehPhysProc_Driving_Audio;
	d->funcPtrs[DRIVER_FUNC_PHYS_ANGULAR] = VehPhysGeneral_PhysAngular;
	d->funcPtrs[DRIVER_FUNC_APPLY_FORCES] = VehPhysForce_OnApplyForces;
	d->funcPtrs[DRIVER_FUNC_COLL_MOVED] = COLL_MOVED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_COLLIDE_DRIVERS] = VehPhysForce_CollideDrivers;
	d->funcPtrs[DRIVER_FUNC_COLL_FIXED] = COLL_FIXED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_JUMP_FRICTION] = VehPhysProc_FreezeVShift_ReverseOneFrame;
	d->funcPtrs[DRIVER_FUNC_TRANSLATE_MATRIX] = VehPhysForce_TranslateMatrix;
	d->funcPtrs[DRIVER_FUNC_ANIMATE] = VehFrameProc_Driving;

	// Turbo meter = full
	d->turbo_MeterRoomLeft = 0;
	d->funcPtrs[DRIVER_FUNC_INIT] = NULL;
	d->funcPtrs[DRIVER_FUNC_PARTICLES] = VehEmitter_DriverMain;

	// turn off 29th flag of actions flag set (means players dont collide anymore)
	d->actionsFlagSet &= ~ACTION_HUMAN_HUMAN_COLLISION;
}


void VehPhysProc_PowerSlide_PhysAngular(struct Thread *th, struct Driver *driver)
{
	register int axisAngleDelta CTR_PSX_REGISTER("$16");
	register int cameraSpinRate CTR_PSX_REGISTER("$18");
	int currentSpinRate;
	register int driftTurnInput CTR_PSX_REGISTER("$19");
	register b32 spinRateNegated CTR_PSX_REGISTER("$20");
	register int steerInput CTR_PSX_REGISTER("$4");
	int steerInputRange;
	register int desiredSpinRate CTR_PSX_REGISTER("$18");
	register b32 clampSpinRate CTR_PSX_REGISTER("$2");
	int signedSpinRate;
	int driftTurnAngleBase;
	int driftTurnInputAbs;
	register int driftTurnScaled CTR_PSX_REGISTER("$3");
	register int driftTurnMagic CTR_PSX_REGISTER("$2");
	register int driftTurnQuotient CTR_PSX_REGISTER("$6");
	register int driftMapZero CTR_PSX_REGISTER("$5");
	register int driftMapEnd CTR_PSX_REGISTER("$7");
	register int driftTurnBase CTR_PSX_REGISTER("$8");
	register int driftAngleScale CTR_PSX_REGISTER("$2");
	s64 driftTurnProduct;
	int numFramesDriftingAbs;
	int turnWobbleAngleAbs;
	int turnWobbleAngleCurrent;
	int turnWobbleAngleNext;
	int turnWobbleTimerSigned;
	int turnAngleCurrent;
	u8 spinoutFrames;
	register int ampTurnState CTR_PSX_REGISTER("$3");
	struct GameTracker *angleTracker;
	int driftBoostTimeSigned;
	u16 driftBoostTimeUnsigned;
	u16 elapsedTimeUnsigned;
	u16 angle;
	register int wrappedAxisAngle CTR_PSX_REGISTER("$2");
	(void)th;
	wrappedAxisAngle = ((driver->axisRotationX - driver->angle) + ANG_PI) & (ANG_TWO_PI - 1);
	CTR_PSX_OBSERVE_VALUE(wrappedAxisAngle);
	axisAngleDelta = wrappedAxisAngle - ANG_PI;
	CTR_PSX_KEEP_VALUE(axisAngleDelta);
	CTR_PSX_MEMORY_BARRIER();
	angle = (u16)driver->angle;
	if (axisAngleDelta != 0)
	{
		// decrease by 1/8
		// val = val * 7/8
		register int axisAngleStepLimit CTR_PSX_REGISTER("$4");
		register int scaledElapsedTime CTR_PSX_REGISTER("$2");

		axisAngleDelta = CTR_MipsSra(axisAngleDelta, VEH_PHYS_PROC_DRIFT_ANGLE_LERP_SHIFT);
		if (axisAngleDelta == 0)
		{
			axisAngleDelta = 1;
		}

		scaledElapsedTime = GAME_TRACKER->elapsedTimeMS << VEH_PHYS_PROC_DRIFT_AXIS_STEP_MS_SHIFT;
		CTR_PSX_KEEP_VALUE(scaledElapsedTime);
		axisAngleStepLimit = scaledElapsedTime >> VEH_PHYS_PROC_DRIFT_MS_SCALE_SHIFT;
		CTR_PSX_KEEP_VALUE(axisAngleStepLimit);

		if (axisAngleDelta > axisAngleStepLimit)
		{
			axisAngleDelta = axisAngleStepLimit;
		}

		axisAngleStepLimit = CTR_MipsNegLo(axisAngleStepLimit);
		if (axisAngleDelta < axisAngleStepLimit)
		{
			axisAngleDelta = axisAngleStepLimit;
		}

		// change player rotation
		driver->angle = (s16)CTR_MipsAddLo(angle, axisAngleDelta);

		driver->axisRotationX = (s16)ANG_MODULO_TWO_PI(CTR_MipsSubLo((u16)driver->axisRotationX, axisAngleDelta));
	}

	if (driver->multDrift < 0)
	{
		register int cameraSpinRateMagnitude CTR_PSX_REGISTER("$2");

		cameraSpinRateMagnitude = (int)driver->const_Drifting_CameraSpinRate;
		CTR_PSX_KEEP_VALUE(cameraSpinRateMagnitude);

		// negative cam spin rate
		cameraSpinRate = -cameraSpinRateMagnitude;
	}
	else
	{
		// positive cam spin rate
		cameraSpinRate = (int)driver->const_Drifting_CameraSpinRate;
	}

	{
		int remainingRotation = CTR_MipsSubLo(driver->rotCurr.w, cameraSpinRate);
		register int rotationStep CTR_PSX_REGISTER("$16");
		int maxRotationStep;
		register int previousRotation CTR_PSX_REGISTER("$4");
		register int rotationProduct CTR_PSX_REGISTER("$3");
		register int currentRotation CTR_PSX_REGISTER("$4");
		register int targetRotation CTR_PSX_REGISTER("$6");

		if (remainingRotation < 0)
		{
			remainingRotation = CTR_MipsNegLo(remainingRotation);
		}
		CTR_PSX_KEEP_VALUE(remainingRotation);

		rotationStep = CTR_MipsSra(remainingRotation, VEH_PHYS_PROC_DRIFT_ANGLE_LERP_SHIFT);
		CTR_PSX_KEEP_VALUE(rotationStep);
		if (rotationStep == 0)
		{
			rotationStep = 1;
		}

		maxRotationStep = (u8)driver->const_DriftCameraLerpStep;
		previousRotation = driver->rotPrev.w;
		CTR_PSX_KEEP_VALUE(previousRotation);
		if (rotationStep < maxRotationStep)
		{
			maxRotationStep = rotationStep;
		}

		driver->rotPrev.w = VehCalc_InterpBySpeed(previousRotation, 8, maxRotationStep);
		rotationProduct = CTR_MipsMulLo(driver->rotPrev.w, GAME_TRACKER->elapsedTimeMS);
		targetRotation = cameraSpinRate;
		currentRotation = driver->rotCurr.w;
		CTR_PSX_KEEP_VALUE(targetRotation);
		CTR_PSX_KEEP_VALUE(currentRotation);
		CTR_PSX_KEEP_VALUE(rotationProduct);
		driver->rotCurr.w = VehCalc_InterpBySpeed(currentRotation, rotationProduct >> VEH_PHYS_PROC_DRIFT_MS_SCALE_SHIFT, targetRotation);
	}

	steerInputRange = CTR_MipsAddLo(driver->const_TurnRate,
	                                CTR_MipsSll((s8)driver->turnConst, VEH_PHYS_PROC_STEER_TURN_CONST_SHIFT) / VEH_PHYS_PROC_STEER_TURN_CONST_DIVISOR);

	// turning rate
	currentSpinRate = driver->rotationSpinRate;

	// drift direction
	driftTurnInput = (int)driver->multDrift;

	spinRateNegated = false;

	steerInput = (s8)driver->simpTurnState;
	desiredSpinRate = steerInput << FRACTIONAL_BITS_8;
	CTR_PSX_KEEP_VALUE_RELAXED(desiredSpinRate);

	if (driftTurnInput < 0)
	{
		// if steering to the right
		if (desiredSpinRate < 1)
		{
			desiredSpinRate = VehCalc_MapToRange(CTR_MipsNegLo(desiredSpinRate), 0, CTR_MipsSll(steerInputRange, FRACTIONAL_BITS_8), 0,
			                                     CTR_MipsSll(CTR_MipsNegLo((s8)driver->const_SteerVel_DriftStandard), FRACTIONAL_BITS_8));
		}

		// if steering to the left
		else
		{
			desiredSpinRate = VehCalc_MapToRange(desiredSpinRate, 0, CTR_MipsSll(steerInputRange, FRACTIONAL_BITS_8), 0,
			                                     CTR_MipsSll(CTR_MipsNegLo((s8)driver->const_SteerVel_DriftSwitchWay), FRACTIONAL_BITS_8));
		}
	}

	// if drifting to the left
	else
	{
		// if steering to the left
		if (desiredSpinRate >= 0)
		{
			desiredSpinRate = VehCalc_MapToRange(desiredSpinRate, 0, CTR_MipsSll(steerInputRange, FRACTIONAL_BITS_8), 0,
			                                     CTR_MipsSll((s8)driver->const_SteerVel_DriftStandard, FRACTIONAL_BITS_8));
		}

		// if steering to the right
		else
		{
			desiredSpinRate = VehCalc_MapToRange(CTR_MipsNegLo(desiredSpinRate), 0, CTR_MipsSll(steerInputRange, FRACTIONAL_BITS_8), 0,
			                                     CTR_MipsSll((s8)driver->const_SteerVel_DriftSwitchWay, FRACTIONAL_BITS_8));
		}
	}

	CTR_PSX_FORGET_VALUE(desiredSpinRate);

	if ((desiredSpinRate < 0) || ((clampSpinRate = desiredSpinRate < currentSpinRate), (desiredSpinRate == 0 && currentSpinRate < 0)))
	{
		spinRateNegated = true;
		desiredSpinRate = CTR_MipsNegLo(desiredSpinRate);
		currentSpinRate = CTR_MipsNegLo(currentSpinRate);
		driftTurnInput = CTR_MipsNegLo(driftTurnInput);
		clampSpinRate = desiredSpinRate < currentSpinRate;
	}

	// 0x464 and 0x466 impact turning somehow

	if (clampSpinRate)
	{
		currentSpinRate = CTR_MipsSubLo(
		    currentSpinRate, CTR_MipsSra(CTR_MipsMulLo(driver->const_DriftSpinRateDecel, GAME_TRACKER->elapsedTimeMS), VEH_PHYS_PROC_DRIFT_MS_SCALE_SHIFT));
		clampSpinRate = currentSpinRate < desiredSpinRate;
	}
	else
	{
		currentSpinRate = CTR_MipsAddLo(
		    currentSpinRate, CTR_MipsSra(CTR_MipsMulLo(driver->const_DriftSpinRateAccel, GAME_TRACKER->elapsedTimeMS), VEH_PHYS_PROC_DRIFT_MS_SCALE_SHIFT));
		clampSpinRate = desiredSpinRate < currentSpinRate;
	}

	if (clampSpinRate)
	{
		currentSpinRate = desiredSpinRate;
	}

	if (desiredSpinRate == 0)
	{
		goto neutralDrift;
	}

	// if drifting left
	if (driftTurnInput > 0)
	{
		driver->KartStates.Drifting.numFramesDrifting = (s16)CTR_MipsAddLo((u16)driver->KartStates.Drifting.numFramesDrifting, 1);

		if (driver->KartStates.Drifting.numFramesDrifting < 0)
		{
			driver->KartStates.Drifting.numFramesDrifting = 0;
		}
		goto driftFrameDone;
	}

	// if drifting right
	if (driftTurnInput < 0)
	{
		driver->KartStates.Drifting.numFramesDrifting = (s16)CTR_MipsSubLo((u16)driver->KartStates.Drifting.numFramesDrifting, 1);

		if (driver->KartStates.Drifting.numFramesDrifting > 0)
		{
			driver->KartStates.Drifting.numFramesDrifting = 0;
		}
		goto driftFrameDone;
	}

neutralDrift:
	// Interpolate by 1 unit, until zero.
	driver->KartStates.Drifting.numFramesDrifting = VehCalc_InterpBySpeed((int)driver->KartStates.Drifting.numFramesDrifting, 1, 0);

driftFrameDone:
	if (spinRateNegated)
	{
		currentSpinRate = CTR_MipsNegLo(currentSpinRate);
		driftTurnInput = CTR_MipsNegLo(driftTurnInput);
	}

	// Map value from [oldMin, oldMax] to [newMin, newMax]
	// inverting newMin and newMax will give an inverse range mapping
	{
		register int driftElapsedTime CTR_PSX_REGISTER("$4") = (int)driver->KartStates.Drifting.driftTotalTimeMS;
		register int driftRampFrames CTR_PSX_REGISTER("$6") = (u8)driver->const_DriftTurnRampFrames;

		driftTurnInput =
		    VehCalc_MapToRange(driftElapsedTime, 0, CTR_MipsSll(driftRampFrames, VEH_PHYS_PROC_FRAME_TIME_SHIFT),
		                       CTR_MipsSra(CTR_MipsMulLo((s8)driver->const_DriftTurnStartupScale, driver->multDrift), FRACTIONAL_BITS_8), driftTurnInput);
	}
	CTR_PSX_KEEP_VALUE(driftTurnInput);

	do
	{
		if (driftTurnInput >= 0)
		{
			if (currentSpinRate < CTR_MipsNegLo(driftTurnInput))
			{
				currentSpinRate = CTR_MipsNegLo(driftTurnInput);
			}
			if (driftTurnInput > 0)
			{
				break;
			}
		}

		if (CTR_MipsNegLo(driftTurnInput) < currentSpinRate)
		{
			currentSpinRate = CTR_MipsNegLo(driftTurnInput);
		}
	} while (0);

	driftTurnScaled = CTR_MipsSll((s8)driver->turnConst, VEH_PHYS_PROC_DRIFT_TURN_CONST_SHIFT);
	CTR_PSX_KEEP_VALUE_RELAXED(driftTurnScaled);
	driftTurnMagic = 0x66666667;
	driftTurnProduct = (s64)driftTurnScaled * driftTurnMagic;
	driftTurnInputAbs = driftTurnInput;
	if (driftTurnInput < 0)
	{
		driftTurnInputAbs = CTR_MipsNegLo(driftTurnInputAbs);
	}
	driftMapZero = 0;
	driftTurnBase = (s8)driver->const_DriftTurnBase;
	driftMapEnd = driftMapZero;
	driver->rotationSpinRate = (s16)currentSpinRate;
	CTR_PSX_MEMORY_BARRIER();
	signedSpinRate = (s16)currentSpinRate;
	driftAngleScale = (int)driver->const_DriftTurnAngleScale;
	CTR_PSX_KEEP_VALUE_RELAXED(driftAngleScale);
	driftTurnScaled = CTR_MipsSra(driftTurnScaled, 31);
	driftTurnQuotient = CTR_MipsSra((s32)(driftTurnProduct >> 32), 1);
	driftTurnQuotient = CTR_MipsSubLo(driftTurnQuotient, driftTurnScaled);
	driftTurnAngleBase = VEH_MAP_TO_RANGE_STAGED_FIFTH(driftTurnInputAbs, driftMapZero, CTR_MipsAddLo(driftTurnBase, driftTurnQuotient), driftMapEnd, driftAngleScale);
	if (driftTurnInput < 0)
	{
		driftTurnAngleBase = CTR_MipsNegLo(driftTurnAngleBase);

		if (signedSpinRate >= 0)
		{
			driftTurnAngleBase = CTR_MipsAddLo(driftTurnAngleBase,
			                                   VehCalc_MapToRange(signedSpinRate, 0, CTR_MipsSll((s8)driver->const_SteerVel_DriftSwitchWay, FRACTIONAL_BITS_8),
			                                                      0, CTR_MipsNegLo((int)driver->const_DriftTurnOppositeDirectionAngle)));
		}
		else
		{
			int mapSpinRate;
			CTR_PSX_FORGET_VALUE(signedSpinRate);
			mapSpinRate = signedSpinRate;
			if (signedSpinRate < 0)
			{
				CTR_PSX_NEGATE_IN_PLACE(mapSpinRate);
			}
			driftTurnAngleBase =
			    CTR_MipsAddLo(driftTurnAngleBase, VehCalc_MapToRange(mapSpinRate, 0, CTR_MipsSll((s8)driver->const_SteerVel_DriftStandard, FRACTIONAL_BITS_8),
			                                                         0, CTR_MipsNegLo((int)driver->const_DriftTurnSameDirectionAngle)));
		}
	}
	else
	{
		if (signedSpinRate < 0)
		{
			int mapSpinRate;
			CTR_PSX_FORGET_VALUE(signedSpinRate);
			mapSpinRate = signedSpinRate;
			if (signedSpinRate < 0)
			{
				CTR_PSX_NEGATE_IN_PLACE(mapSpinRate);
			}
			driftTurnAngleBase =
			    CTR_MipsAddLo(driftTurnAngleBase, VehCalc_MapToRange(mapSpinRate, 0, CTR_MipsSll((s8)driver->const_SteerVel_DriftSwitchWay, FRACTIONAL_BITS_8),
			                                                         0, (int)driver->const_DriftTurnOppositeDirectionAngle));
		}
		else
		{
			driftTurnAngleBase = CTR_MipsAddLo(driftTurnAngleBase,
			                                   VehCalc_MapToRange(signedSpinRate, 0, CTR_MipsSll((s8)driver->const_SteerVel_DriftStandard, FRACTIONAL_BITS_8),
			                                                      0, (int)driver->const_DriftTurnSameDirectionAngle));
		}
	}

	driftTurnAngleBase = CTR_MipsSubLo(driftTurnAngleBase, driver->turnAngleCurr);
	turnAngleCurrent = (u16)driver->turnAngleCurr;
	if (driftTurnAngleBase != 0)
	{
		driftTurnAngleBase = CTR_MipsSra(driftTurnAngleBase, VEH_PHYS_PROC_DRIFT_ANGLE_LERP_SHIFT);
		if (driftTurnAngleBase == 0)
		{
			driftTurnAngleBase = 1;
		}
		CTR_PSX_MEMORY_BARRIER();
		{
			register int turnAngleNext CTR_PSX_REGISTER("$2");

			turnAngleNext = CTR_MipsAddLo(turnAngleCurrent, driftTurnAngleBase);
			CTR_PSX_KEEP_VALUE(turnAngleNext);
			driver->turnAngleCurr = (s16)turnAngleNext;
		}
	}

	numFramesDriftingAbs = driver->KartStates.Drifting.numFramesDrifting;
	spinoutFrames = (u8)driver->const_Drifting_FramesTillSpinout;

	if (numFramesDriftingAbs < 0)
	{
		numFramesDriftingAbs = CTR_MipsNegLo(numFramesDriftingAbs);
	}

	// get half of spin-out constant,
	// this determines when to start making tire sound effects,
	// after the turbo meter finishes filling past it's max capacity

	// if you drift beyond the limit of the turbo meter
	if ((spinoutFrames >> VEH_PHYS_PROC_DRIFT_SPINOUT_THRESHOLD_SHIFT) < numFramesDriftingAbs)
	{
		// Play the SFX of near-spinout

		int turnWobbleAngleAbs = driver->turnWobbleAngle;
		if (turnWobbleAngleAbs < 0)
		{
			turnWobbleAngleAbs = CTR_MipsNegLo(turnWobbleAngleAbs);
		}

		// if low distortion
		if (turnWobbleAngleAbs < VEH_PHYS_PROC_TURN_WOBBLE_START_ANGLE_MAX)
		{
			// count up for 8 frames
			driver->turnWobbleTimer = VEH_PHYS_PROC_TURN_WOBBLE_START_TIMER;

			// distortion, rate of change
			driver->turnWobbleVelocity = VEH_PHYS_PROC_TURN_WOBBLE_START_VELOCITY;

			if (driftTurnInput < 0)
			{
				driver->turnWobbleVelocity = (s16)CTR_MipsNegLo(driver->turnWobbleVelocity);
			}
		}
	}

	// if not near-spinout
	else
	{
		// stop increasing distortion,
		// go back down
		driver->turnWobbleTimer = 0;
	}

	turnWobbleAngleAbs = driver->turnWobbleAngle;
	if (turnWobbleAngleAbs < 0)
	{
		turnWobbleAngleAbs = CTR_MipsNegLo(turnWobbleAngleAbs);
	}

	// if distortion is too high
	if (turnWobbleAngleAbs > VEH_PHYS_PROC_TURN_WOBBLE_ANGLE_MAX)
	{
		// stop increasing distortion,
		// go back down
		driver->turnWobbleTimer = 0;
	}

	turnWobbleTimerSigned = driver->turnWobbleTimer;
	{
		register int turnWobbleTimerUnsigned CTR_PSX_REGISTER("$3");

		turnWobbleTimerUnsigned = (u16)driver->turnWobbleTimer;

		// frames counting down
		if (turnWobbleTimerSigned != 0)
		{
			register int turnWobbleAngleUnsigned CTR_PSX_REGISTER("$2");
			register int turnWobbleVelocityUnsigned CTR_PSX_REGISTER("$4");

			turnWobbleTimerUnsigned--;
			CTR_PSX_KEEP_VALUE(turnWobbleTimerUnsigned);
			turnWobbleAngleUnsigned = (u16)driver->turnWobbleAngle;
			turnWobbleVelocityUnsigned = (u16)driver->turnWobbleVelocity;
			driver->turnWobbleTimer = (s16)turnWobbleTimerUnsigned;
			CTR_PSX_MEMORY_BARRIER();

			// move up each frame
			turnWobbleAngleNext = turnWobbleAngleUnsigned + turnWobbleVelocityUnsigned;
		}

		// frame countdown over
		else
		{
			int turnWobbleVelocityAbs;

			// nearing spinout sfx
			driver->turnWobbleVelocity = VEH_PHYS_PROC_TURN_WOBBLE_RETURN_VELOCITY;

			if (0 < driver->turnWobbleAngle)
			{
				driver->turnWobbleVelocity = (s16)CTR_MipsNegLo(driver->turnWobbleVelocity);
			}

			turnWobbleVelocityAbs = driver->turnWobbleVelocity;
			turnWobbleAngleCurrent = driver->turnWobbleAngle;
			if (turnWobbleVelocityAbs < 0)
			{
				turnWobbleVelocityAbs = CTR_MipsNegLo(turnWobbleVelocityAbs);
			}

			// move down until zero
			turnWobbleAngleNext = VehCalc_InterpBySpeed(turnWobbleAngleCurrent, turnWobbleVelocityAbs, 0);
		}
	}

	// near-spinout distortion SFX
	driver->turnWobbleAngle = turnWobbleAngleNext;
	CTR_PSX_MEMORY_BARRIER();

	angleTracker = GAME_TRACKER;
	CTR_PSX_KEEP_VALUE(angleTracker);
	ampTurnState = CTR_MipsAddLo(signedSpinRate, driftTurnInput);
	CTR_PSX_KEEP_VALUE(ampTurnState);
	driver->ampTurnState = (s16)ampTurnState;

	driver->angle = (s16)ANG_MODULO_TWO_PI(
	    CTR_MipsAddLo((u16)driver->angle, CTR_MipsSra(CTR_MipsMulLo(ampTurnState, angleTracker->elapsedTimeMS), VEH_PHYS_PROC_ANGLE_INTEGRATION_SHIFT)));

	driftBoostTimeSigned = driver->KartStates.Drifting.driftBoostTimeMS;
	driftBoostTimeUnsigned = (u16)driver->KartStates.Drifting.driftBoostTimeMS;
	if (driftBoostTimeSigned != 0)
	{
		register int axisKick CTR_PSX_REGISTER("$16");
		register int axisKickRate CTR_PSX_REGISTER("$3");
		register int axisKickProduct CTR_PSX_REGISTER("$9");
		register int turnAngleForKick CTR_PSX_REGISTER("$2");

		// decrease by elpased time
		elapsedTimeUnsigned = (u16)angleTracker->elapsedTimeMS;
		driver->KartStates.Drifting.driftBoostTimeMS = (s16)CTR_MipsSubLo(driftBoostTimeUnsigned, elapsedTimeUnsigned);

		if (driver->KartStates.Drifting.driftBoostTimeMS < 0)
		{
			driver->KartStates.Drifting.driftBoostTimeMS = 0;
		}

		axisKickRate = (u8)driver->const_DriftBoostAxisKickRate;
		axisKickProduct = CTR_MipsMulLo(axisKickRate, GAME_TRACKER->elapsedTimeMS);
		turnAngleForKick = driver->turnAngleCurr;
		axisKick = CTR_MipsSra(axisKickProduct, VEH_PHYS_PROC_DRIFT_MS_SCALE_SHIFT);
		if (turnAngleForKick < 0)
		{
			axisKick = CTR_MipsNegLo(axisKick);
		}
		CTR_PSX_KEEP_VALUE(axisKick);

		driver->axisRotationX = (s16)ANG_MODULO_TWO_PI(CTR_MipsAddLo((u16)driver->axisRotationX, axisKick));
	}

	driver->rotCurr.y = (s16)CTR_MipsAddLo((u16)driver->turnWobbleAngle, CTR_MipsAddLo((u16)driver->angle, (u16)driver->turnAngleCurr));

	// increment this by milliseconds
	driver->KartStates.Drifting.driftTotalTimeMS = (s16)CTR_MipsAddLo((u16)driver->KartStates.Drifting.driftTotalTimeMS, (u16)GAME_TRACKER->elapsedTimeMS);

	if (driver->KartStates.Drifting.driftTotalTimeMS > (int)((u32)(u8)driver->const_DriftTurnRampFrames << VEH_PHYS_PROC_FRAME_TIME_SHIFT))
	{
		driver->KartStates.Drifting.driftTotalTimeMS = (s16)((u16)(u8)driver->const_DriftTurnRampFrames << VEH_PHYS_PROC_FRAME_TIME_SHIFT);
	}

	VehPhysForce_RotAxisAngle(&driver->matrixMovingDir, CTR_VECTOR_DATA(&driver->AxisAngle1_normalVec), driver->angle);
	{
		register const CtrPackedU32 *matrixWords CTR_PSX_REGISTER("$16") = (const CtrPackedU32 *)&driver->matrixMovingDir;
		CTR_PSX_KEEP_VALUE(matrixWords);
		{
			register u32 matrixWord0 CTR_PSX_REGISTER("$12") = matrixWords[0];
			register u32 matrixWord1 CTR_PSX_REGISTER("$13") = matrixWords[1];

			CTC2(matrixWord0, 0);
			CTC2(matrixWord1, 1);
		}
		{
			register u32 matrixWord2 CTR_PSX_REGISTER("$12") = matrixWords[2];
			register u32 matrixWord3 CTR_PSX_REGISTER("$13") = matrixWords[3];
			register u32 matrixWord4 CTR_PSX_REGISTER("$14") = matrixWords[4];

			CTC2(matrixWord2, 2);
			CTC2(matrixWord3, 3);
			CTC2(matrixWord4, 4);
		}
	}
	VehPhysForce_CounterSteer(driver);
}

void VehPhysProc_PowerSlide_Finalize(struct Driver *d)
{
	d->previousFrameMultDrift = d->multDrift;
	d->timeUntilDriftSpinout = (s16)CTR_MipsSll((u8)d->const_DriftReleaseTurnAssistFrames, VEH_PHYS_PROC_FRAME_TIME_SHIFT);
}


void VehPhysProc_PowerSlide_Update(struct Thread *t, struct Driver *d)
{
	struct GamepadBuffer drivingPad = GAMEPADS->gamepad[d->driverID];
	Chaos_ApplyInputRules(d, &drivingPad);
	// If you press L1 or R1, attempt to convert the drift meter to reserves.
	if ((drivingPad.buttonsTapped & VEH_PHYS_PROC_JUMP_BUTTON_MASK) != 0)
	{
		int meterLeft = d->turbo_MeterRoomLeft;

		d->KartStates.Drifting.numFramesDrifting = 0;

		if (meterLeft != 0)
		{
			int highMeter = CTR_MipsSll((u8)d->const_turboLowRoomWarning, VEH_PHYS_PROC_FRAME_TIME_SHIFT);

			if (meterLeft < highMeter)
			{
				int incrementReserves =
				    VehCalc_MapToRange(meterLeft, 0, highMeter, CTR_MipsSll((u8)d->const_turboFullBarReserveGain, VEH_PHYS_PROC_FRAME_TIME_SHIFT), 0);

				VehFire_Increment(d, incrementReserves, POWER_SLIDE_HANG_TIME,
				                  CTR_MipsSll(d->KartStates.Drifting.numBoostsSuccess, VEH_PHYS_PROC_DRIFT_FIRE_LEVEL_SHIFT));

				d->KartStates.Drifting.numBoostsSuccess = (s8)CTR_MipsAddLo((u8)d->KartStates.Drifting.numBoostsSuccess, 1);

				if (d->KartStates.Drifting.numBoostsSuccess < VEH_PHYS_PROC_DRIFT_MAX_BOOSTS)
				{
					d->actionsFlagSet |= ACTION_TURBO_INPUT_LATCH;
				}

				d->KartStates.Drifting.driftBoostTimeMS = (s16)CTR_MipsSll((u8)d->const_DriftBoostDurationFrames, VEH_PHYS_PROC_FRAME_TIME_SHIFT);
			}
			else
			{
				d->failedBoostExhaustTimer = VEH_PHYS_PROC_DRIFT_FAILED_BOOST_EXHAUST_TIMER;
			}

			d->turbo_MeterRoomLeft = 0;
			d->KartStates.Drifting.numBoostsAttempted = (s8)CTR_MipsAddLo((u8)d->KartStates.Drifting.numBoostsAttempted, 1);
		}
	}
	else
	{
		int meterLeft = (u16)d->turbo_MeterRoomLeft;

		if (d->turbo_MeterRoomLeft != 0)
		{
			meterLeft = CTR_MipsSubLo(meterLeft, (u16)GAME_TRACKER->elapsedTimeMS);
			d->turbo_MeterRoomLeft = (s16)meterLeft;

			if (CTR_MipsSll(meterLeft, 16) < 0)
			{
				d->turbo_MeterRoomLeft = 0;
			}

			if (d->turbo_MeterRoomLeft == 0)
			{
				OtherFX_Play_Echo(VEH_PHYS_PROC_DRIFT_METER_FULL_FX, 1, (u16)(d->actionsFlagSet >> 16) & 1);

				d->KartStates.Drifting.numBoostsAttempted = (s8)CTR_MipsAddLo((u8)d->KartStates.Drifting.numBoostsAttempted, VEH_PHYS_PROC_DRIFT_MAX_BOOSTS);
			}
		}
		else
		{
			if (d->KartStates.Drifting.numBoostsAttempted < VEH_PHYS_PROC_DRIFT_MAX_BOOSTS)
			{
				d->turbo_MeterRoomLeft = (s16)CTR_MipsSll((u8)d->const_turboMaxRoom, VEH_PHYS_PROC_FRAME_TIME_SHIFT);
			}
		}
	}

	// Drift for too long, or reverse without a turbo pad, and the kart spins out.
	if (d->KartStates.Drifting.numFramesDrifting > (u8)d->const_Drifting_FramesTillSpinout)
	{
		d->NoInputTimer = VEH_PHYS_PROC_DRIFT_SPINOUT_NO_INPUT_TIMER;
		VehPhysProc_SpinFirst_Init(t, d);
	}
	else if ((d->speedApprox < 0) && ((d->stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD_MASK) == 0))
	{
		d->NoInputTimer = VEH_PHYS_PROC_DRIFT_REVERSE_SPINOUT_NO_INPUT_TIMER;
		VehPhysProc_SpinFirst_Init(t, d);
	}
	else
	{
		// Switchway drift counts frames negative; spin out after the negative limit.
		if ((d->KartStates.Drifting.numFramesDrifting < CTR_MipsNegLo((u8)d->const_Drifting_FramesTillSpinout)) ||

		    // speed is less than half the driver's speed classStat
		    (((d->speed < CTR_MipsSra(d->const_Speed_ClassStat, VEH_PHYS_PROC_HALF_SPEED_SHIFT) ||

		       ((d->actionsFlagSet & (ACTION_DRIVING_AGAINST_WALL | ACTION_ACCEL_PREVENTION | ACTION_BRAKE_WITH_ACCEL)) != 0)) ||

		      // If the gamepad input is...
		      ((drivingPad.buttonsHeldCurrFrame &

		        // does not include the jump button that you used to start drifting
		        d->buttonUsedToStartDrift) == 0))))
		{
			// Stop drifting, just drive
			VehPhysProc_PowerSlide_Finalize(d);
			VehPhysProc_Driving_Init(t, d);
		}
	}
}


void VehPhysProc_PowerSlide_PhysLinear(struct Thread *thread, struct Driver *driver)
{
	VehPhysProc_Driving_PhysLinear(thread, driver);
	driver->actionsFlagSet |= ACTION_BACK_SKID | ACTION_FRONT_SKID;
	driver->timeSpentDrifting = CTR_MipsAddLo(driver->timeSpentDrifting, GAME_TRACKER->elapsedTimeMS);
}


void VehPhysProc_PowerSlide_InitSetUpdate(struct Thread *t, struct Driver *d)
{
	(void)t;
	d->funcPtrs[DRIVER_FUNC_INIT] = 0;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = VehPhysProc_PowerSlide_Update;
}


void VehPhysProc_PowerSlide_Init(struct Thread *t, struct Driver *d)
{
	int drift;

	(void)t;
	d->kartState = KS_DRIFTING;

	// Character's Drift stat + ((Turning multiplier? << 2) / 5) * 100
	drift = CTR_MipsSll(
	    CTR_MipsAddLo((s8)d->const_DriftTurnBase, CTR_MipsSll((s8)d->turnConst, VEH_PHYS_PROC_DRIFT_TURN_CONST_SHIFT) / VEH_PHYS_PROC_STEER_TURN_CONST_DIVISOR),
	    FRACTIONAL_BITS_8);

	d->multDrift = (s16)drift;

	// if simplified turning state is negative (means you're turning right)
	if ((s8)d->simpTurnState < 0)
	{
		// also make Multiplied drift negative
		d->multDrift = (s16)CTR_MipsNegLo(drift);
	}

	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_PowerSlide_InitSetUpdate;
	d->funcPtrs[DRIVER_FUNC_PHYS_LINEAR] = VehPhysProc_PowerSlide_PhysLinear;
	d->funcPtrs[DRIVER_FUNC_AUDIO] = VehPhysProc_Driving_Audio;
	d->funcPtrs[DRIVER_FUNC_PHYS_ANGULAR] = VehPhysProc_PowerSlide_PhysAngular;
	d->funcPtrs[DRIVER_FUNC_APPLY_FORCES] = VehPhysForce_OnApplyForces;
	d->funcPtrs[DRIVER_FUNC_COLL_MOVED] = COLL_MOVED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_COLLIDE_DRIVERS] = VehPhysForce_CollideDrivers;
	d->funcPtrs[DRIVER_FUNC_COLL_FIXED] = COLL_FIXED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_JUMP_FRICTION] = VehPhysGeneral_JumpAndFriction;
	d->funcPtrs[DRIVER_FUNC_TRANSLATE_MATRIX] = VehPhysForce_TranslateMatrix;
	d->funcPtrs[DRIVER_FUNC_ANIMATE] = VehFrameProc_Driving;

	// erase union in driver struct
	d->KartStates.Drifting.numFramesDrifting = 0;
	d->KartStates.Drifting.driftTotalTimeMS = 0;
	d->KartStates.Drifting.driftBoostTimeMS = 0;
	d->KartStates.Drifting.numBoostsAttempted = 0;
	d->KartStates.Drifting.numBoostsSuccess = 0;

	d->rotationSpinRate = 0;
	d->turnAngleLerpVel = 0;
	d->timeUntilDriftSpinout = 0;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = NULL;
	d->funcPtrs[DRIVER_FUNC_PARTICLES] = VehEmitter_DriverMain;

	// Turbo meter space left to fill = Length of Turbo meter << 5
	d->turbo_MeterRoomLeft = (s16)CTR_MipsSll((u8)d->const_turboMaxRoom, VEH_PHYS_PROC_FRAME_TIME_SHIFT);
}


void VehPhysProc_SlamWall_PhysAngular(struct Thread *t, struct Driver *d)
{
	register u16 angle CTR_PSX_REGISTER("$2");

	(void)t;

	angle = (u16)(CTR_MipsAddLo((u16)d->angle,
	                            CTR_MipsSra(CTR_MipsMulLo(d->ampTurnState, GAME_TRACKER->elapsedTimeMS), VEH_PHYS_PROC_ANGLE_INTEGRATION_SHIFT)) &
	              (ANG_TWO_PI - 1));
	d->angle = (s16)angle;

	d->rotCurr.y = (s16)CTR_MipsAddLo((u16)d->turnWobbleAngle, CTR_MipsAddLo(angle, (u16)d->turnAngleCurr));

	d->rotCurr.w = VehCalc_InterpBySpeed(d->rotCurr.w, CTR_MipsSra(CTR_MipsSll(GAME_TRACKER->elapsedTimeMS, 5), 5), 0);

	d->turnAngleCurr = VehCalc_InterpBySpeed(d->turnAngleCurr, CTR_MipsSra(CTR_MipsSll(GAME_TRACKER->elapsedTimeMS, 7), 5), 0);

	VehPhysForce_RotAxisAngle(&d->matrixMovingDir, CTR_VECTOR_DATA(&(d->AxisAngle1_normalVec)), d->angle);
}


void VehPhysProc_SlamWall_Update(struct Thread *t, struct Driver *d)
{
	(void)t;
	(void)d;
}


void VehPhysProc_SlamWall_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;
}


void VehPhysProc_SlamWall_Animate(struct Thread *t, struct Driver *d)
{
	struct Instance *inst = t->inst;
	int animFrame;
	int numFrames;

	inst->animFrame = (s16)CTR_MipsAddLo((u16)inst->animFrame, 1);

	d->matrixIndex = (u8)CTR_MipsAddLo(d->matrixIndex, 1);

	animFrame = inst->animFrame;
	numFrames = VehFrameInst_GetNumAnimFrames(inst, inst->animIndex);

	if (animFrame < (numFrames - 1))
	{
		return;
	}

	numFrames = VehFrameInst_GetNumAnimFrames(inst, 0);
	if (numFrames > 0)
	{
		inst->animIndex = 0;
		inst->animFrame = VehFrameInst_GetStartFrame(0, numFrames);
		d->matrixArray = BAKED_GTE_MATRIX_NONE;
		d->matrixIndex = 0;
	}

	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_Driving_Init;
}


void VehPhysProc_SlamWall_Init(struct Thread *t, struct Driver *d)
{
	struct Instance *inst = t->inst;

	d->kartState = KS_CRASHING;
	d->numFramesSpentSteering = VEH_PHYS_PROC_STEER_RESET_FRAMES;
	d->turbo_MeterRoomLeft = 0;
	d->rotationSpinRate = 0;
	d->turnAngleLerpVel = 0;
	d->turnWobbleAngle = 0;
	d->turnWobbleVelocity = 0;
	d->turnWobbleTimer = 0;
	d->ampTurnState = 0;
	d->speed = 0;
	d->speedApprox = 0;

	d->xSpeed = 0;
	d->ySpeed = 0;
	d->zSpeed = 0;
	d->velocity.x = 0;
	d->velocity.y = 0;
	d->velocity.z = 0;
	d->speedometerNeedleValue = 0;
	VehFire_SetReserves(d, 0);
	d->turbo_outsideTimer = 0;
	d->VehFire_AudioCooldown = 0;
	d->Screen_OffsetY = 0;
	d->distanceFromGround = 0;
	d->reserved_0x40e = 0;
	d->jumpSquishStretch2 = 0;
	d->jumpSquishStretch = 0;
	d->baseSpeed = 0;
	d->fireSpeed = 0;

	// NOTE(aalhendi): Retail only writes X/Y scale here.
	inst->scale.y = (s16)VehPhysForce_ScaleValue(d, VEH_PHYS_PROC_CRASH_SCALE_XY);
	inst->scale.x = (s16)VehPhysForce_ScaleValue(d, VEH_PHYS_PROC_CRASH_SCALE_XY);

	d->funcPtrs[DRIVER_FUNC_UPDATE] = VehPhysProc_SlamWall_Update;
	d->funcPtrs[DRIVER_FUNC_INIT] = NULL;
	d->funcPtrs[DRIVER_FUNC_PHYS_LINEAR] = VehPhysProc_SlamWall_PhysLinear;
	d->funcPtrs[DRIVER_FUNC_AUDIO] = VehPhysProc_Driving_Audio;
	d->funcPtrs[DRIVER_FUNC_PHYS_ANGULAR] = VehPhysProc_SlamWall_PhysAngular;
	d->funcPtrs[DRIVER_FUNC_APPLY_FORCES] = VehPhysForce_OnApplyForces;
	d->funcPtrs[DRIVER_FUNC_COLL_MOVED] = COLL_MOVED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_COLLIDE_DRIVERS] = VehPhysForce_CollideDrivers;
	d->funcPtrs[DRIVER_FUNC_COLL_FIXED] = COLL_FIXED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_JUMP_FRICTION] = VehPhysGeneral_JumpAndFriction;
	d->funcPtrs[DRIVER_FUNC_TRANSLATE_MATRIX] = VehPhysForce_TranslateMatrix;
	d->funcPtrs[DRIVER_FUNC_ANIMATE] = VehPhysProc_SlamWall_Animate;
	d->funcPtrs[DRIVER_FUNC_PARTICLES] = VehEmitter_DriverMain;
}


void VehPhysProc_SpinFirst_Update(struct Thread *t, struct Driver *d)
{
	int speedApprox;

	// cooldown after the spinning started
	if (d->NoInputTimer != 0)
	{
		// absolute value
		speedApprox = d->speedApprox;
		if (speedApprox < 0)
		{
			speedApprox = CTR_MipsNegLo(speedApprox);
		}

		// quit if moving quickly
		if (speedApprox > VEH_PHYS_PROC_SPIN_SLOW_SPEED_THRESHOLD)
		{
			return;
		}
	}

	// if speed has slown to near-halt,
	// or if NoInputTimer ran out
	VehPhysProc_SpinLast_Init(t, d);
}


void VehPhysProc_SpinFirst_PhysLinear(struct Thread *t, struct Driver *d)
{
	d->NoInputTimer = (s16)CTR_MipsSubLo((u16)d->NoInputTimer, (u16)GAME_TRACKER->elapsedTimeMS);
	if (d->NoInputTimer < 0)
	{
		d->NoInputTimer = 0;
	}

	VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;

	d->actionsFlagSet |= ACTION_WARP | ACTION_FRONT_SKID | ACTION_BACK_SKID | ACTION_ACCEL_PREVENTION;

	d->timeSpentSpinningOut = CTR_MipsAddLo(d->timeSpentSpinningOut, GAME_TRACKER->elapsedTimeMS);
}


void VehPhysProc_SpinFirst_PhysAngular(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT;
	register s32 rotationSpinRate CTR_PSX_REGISTER("$4");
	register s32 storedRotationSpinRate CTR_PSX_REGISTER("$3");

	(void)t;
	rotationSpinRate = (u16)d->rotationSpinRate;

	d->numFramesSpentSteering = VEH_PHYS_PROC_STEER_RESET_FRAMES;

	storedRotationSpinRate = (s16)rotationSpinRate >> 3;
	rotationSpinRate -= storedRotationSpinRate;

	d->turnAngleCurr += d->KartStates.Spinning.driftSpinRate;
	d->turnAngleCurr += ANG_PI;
	d->turnAngleCurr &= ANG_TWO_PI - 1;
	d->turnAngleCurr -= ANG_PI;

	d->rotationSpinRate = (s16)rotationSpinRate;
	rotationSpinRate = (u16)d->turnWobbleAngle;
	storedRotationSpinRate = (u16)d->rotationSpinRate;
	gGT = GAME_TRACKER;
	d->ampTurnState = (s16)storedRotationSpinRate;
	rotationSpinRate -= (s16)rotationSpinRate >> 3;
	d->turnWobbleAngle = (s16)rotationSpinRate;

	storedRotationSpinRate = (s16)storedRotationSpinRate;
	d->angle += (s16)((storedRotationSpinRate * gGT->elapsedTimeMS) >> VEH_PHYS_PROC_ANGLE_INTEGRATION_SHIFT);
	d->angle &= ANG_TWO_PI - 1;

	d->rotCurr.y = d->angle + d->turnAngleCurr + d->turnWobbleAngle;

	d->rotCurr.w = VehCalc_InterpBySpeed(d->rotCurr.w, (gGT->elapsedTimeMS << 5) >> 5, 0);

	VehPhysForce_RotAxisAngle(&d->matrixMovingDir, CTR_VECTOR_DATA(&(d->AxisAngle1_normalVec)), d->angle);
}


void VehPhysProc_SpinFirst_InitSetUpdate(struct Thread *t, struct Driver *d)
{
	(void)t;
	d->funcPtrs[DRIVER_FUNC_INIT] = 0;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = VehPhysProc_SpinFirst_Update;
}


void VehPhysProc_SpinFirst_Init(struct Thread *t, struct Driver *d)
{
	(void)t;

	d->kartState = KS_SPINNING;

	d->turnAngleLerpVel = 0;
	d->turbo_MeterRoomLeft = 0;

	if (LOAD_IsOpen_RacingOrBattle() && ((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) == 0))
	{
		RB_Player_ModifyWumpa(d, -1);
	}

	Voiceline_RequestPlayOwned(VEH_PHYS_PROC_SPIN_VOICELINE_ID, GAME_CHARACTER_IDS[d->driverID], VEH_PHYS_PROC_SPIN_VOICELINE_PRIORITY, d);

	// if spinning left
	d->KartStates.Spinning.spinDir = 1;
	d->KartStates.Spinning.driftSpinRate = VEH_PHYS_PROC_SPIN_INITIAL_RATE;

	if (d->ampTurnState < 0)
	{
		// if spinning right
		d->KartStates.Spinning.driftSpinRate = -VEH_PHYS_PROC_SPIN_INITIAL_RATE;
		d->KartStates.Spinning.spinDir = -1;
	}

	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_SpinFirst_InitSetUpdate;
	d->funcPtrs[DRIVER_FUNC_PHYS_LINEAR] = VehPhysProc_SpinFirst_PhysLinear;
	d->funcPtrs[DRIVER_FUNC_AUDIO] = VehPhysProc_Driving_Audio;
	d->funcPtrs[DRIVER_FUNC_PHYS_ANGULAR] = VehPhysProc_SpinFirst_PhysAngular;
	d->funcPtrs[DRIVER_FUNC_APPLY_FORCES] = VehPhysForce_OnApplyForces;
	d->funcPtrs[DRIVER_FUNC_COLL_MOVED] = COLL_MOVED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_COLLIDE_DRIVERS] = VehPhysForce_CollideDrivers;
	d->funcPtrs[DRIVER_FUNC_COLL_FIXED] = COLL_FIXED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_JUMP_FRICTION] = VehPhysGeneral_JumpAndFriction;
	d->funcPtrs[DRIVER_FUNC_TRANSLATE_MATRIX] = VehPhysForce_TranslateMatrix;
	d->funcPtrs[DRIVER_FUNC_ANIMATE] = VehFrameProc_Spinning;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = NULL;
	d->funcPtrs[DRIVER_FUNC_PARTICLES] = VehEmitter_DriverMain;

	if ((s8)d->simpTurnState > 0)
	{
		GAMEPAD_JogCon1(d, VEH_PHYS_PROC_SPIN_RIGHT_FEEDBACK, VEH_PHYS_PROC_SPIN_FEEDBACK_TIMER);
	}

	else
	{
		GAMEPAD_JogCon1(d, VEH_PHYS_PROC_SPIN_LEFT_FEEDBACK, VEH_PHYS_PROC_SPIN_FEEDBACK_TIMER);
	}
}


void VehPhysProc_SpinLast_Update(struct Thread *t, struct Driver *d)
{
	int driftAngle = d->turnAngleCurr;
	if (driftAngle < 0)
	{
		driftAngle = CTR_MipsNegLo(driftAngle);
	}

	// if almost facing forward
	if (driftAngle < VEH_PHYS_PROC_SPIN_STOP_ANGLE_THRESHOLD)
	{
		// stop spin
		VehPhysProc_SpinStop_Init(t, d);
	}
}


void VehPhysProc_SpinLast_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;

	d->actionsFlagSet |= ACTION_WARP | ACTION_ACCEL_PREVENTION;
}


void VehPhysProc_SpinLast_PhysAngular(struct Thread *t, struct Driver *d)
{
	register struct GameTracker *gGT CTR_PSX_REGISTER("$5");
	register u16 angle CTR_PSX_REGISTER("$2");
	register s32 turnAngleCurr CTR_PSX_REGISTER("$4");
	int driftAngleCurr;

	(void)t;
	driftAngleCurr = d->turnAngleCurr;

	d->numFramesSpentSteering = VEH_PHYS_PROC_STEER_RESET_FRAMES;

	d->rotationSpinRate = (s16)CTR_MipsSubLo((u16)d->rotationSpinRate, CTR_MipsSra(d->rotationSpinRate, 3));
	d->turnWobbleAngle = (s16)CTR_MipsSubLo((u16)d->turnWobbleAngle, CTR_MipsSra(d->turnWobbleAngle, 3));

	d->ampTurnState = d->rotationSpinRate;

	if (driftAngleCurr > 0)
	{
		if ((d->KartStates.Spinning.driftSpinRate < 0) && (driftAngleCurr < VEH_PHYS_PROC_SPIN_RECENTER_ANGLE_THRESHOLD))
		{
			d->KartStates.Spinning.driftSpinRate = (s16)CTR_MipsSra(CTR_MipsNegLo(CTR_MipsSll(driftAngleCurr, VEH_PHYS_PROC_SPIN_RECENTER_RATE_NUM_SHIFT)),
			                                                        VEH_PHYS_PROC_SPIN_RECENTER_RATE_DEN_SHIFT);

			if (d->KartStates.Spinning.driftSpinRate > -VEH_PHYS_PROC_SPIN_MIN_RECENTER_RATE)
			{
				d->KartStates.Spinning.driftSpinRate = -VEH_PHYS_PROC_SPIN_MIN_RECENTER_RATE;
			}
		}

		d->turnAngleCurr = (s16)CTR_MipsSubLo(
		    CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnAngleCurr, (u16)d->KartStates.Spinning.driftSpinRate), ANG_PI) & (ANG_TWO_PI - 1), ANG_PI);

		if ((d->KartStates.Spinning.driftSpinRate < 0) && (d->turnAngleCurr < 0))
		{
			d->turnAngleCurr = 0;
		}
	}

	else if (driftAngleCurr < 0)
	{
		if ((d->KartStates.Spinning.driftSpinRate > 0) && (driftAngleCurr > -VEH_PHYS_PROC_SPIN_RECENTER_ANGLE_THRESHOLD))
		{
			d->KartStates.Spinning.driftSpinRate = (s16)CTR_MipsSra(CTR_MipsNegLo(CTR_MipsSll(driftAngleCurr, VEH_PHYS_PROC_SPIN_RECENTER_RATE_NUM_SHIFT)),
			                                                        VEH_PHYS_PROC_SPIN_RECENTER_RATE_DEN_SHIFT);

			if (d->KartStates.Spinning.driftSpinRate < VEH_PHYS_PROC_SPIN_MIN_RECENTER_RATE)
			{
				d->KartStates.Spinning.driftSpinRate = VEH_PHYS_PROC_SPIN_MIN_RECENTER_RATE;
			}
		}

		d->turnAngleCurr = (s16)CTR_MipsSubLo(
		    CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnAngleCurr, (u16)d->KartStates.Spinning.driftSpinRate), ANG_PI) & (ANG_TWO_PI - 1), ANG_PI);

		if ((d->KartStates.Spinning.driftSpinRate > 0) && (d->turnAngleCurr > 0))
		{
			d->turnAngleCurr = 0;
		}
	}

	gGT = GAME_TRACKER;
	turnAngleCurr = (u16)d->turnAngleCurr;
	angle = (u16)(CTR_MipsAddLo((u16)d->angle, CTR_MipsSra(CTR_MipsMulLo(d->ampTurnState, gGT->elapsedTimeMS), VEH_PHYS_PROC_ANGLE_INTEGRATION_SHIFT)) &
	              (ANG_TWO_PI - 1));
	d->angle = (s16)angle;

	d->rotCurr.y = (s16)CTR_MipsAddLo((u16)d->turnWobbleAngle, CTR_MipsAddLo(angle, turnAngleCurr));

	d->rotCurr.w = VehCalc_InterpBySpeed(d->rotCurr.w, CTR_MipsSra(CTR_MipsSll(gGT->elapsedTimeMS, 5), 5), 0);

	VehPhysForce_RotAxisAngle(&d->matrixMovingDir, CTR_VECTOR_DATA(&(d->AxisAngle1_normalVec)), d->angle);
}


void VehPhysProc_SpinLast_Init(struct Thread *t, struct Driver *d)
{
	(void)t;

	d->funcPtrs[DRIVER_FUNC_INIT] = NULL;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = VehPhysProc_SpinLast_Update;
	d->funcPtrs[DRIVER_FUNC_PHYS_LINEAR] = VehPhysProc_SpinLast_PhysLinear;
	d->funcPtrs[DRIVER_FUNC_AUDIO] = VehPhysProc_Driving_Audio;
	d->funcPtrs[DRIVER_FUNC_PHYS_ANGULAR] = VehPhysProc_SpinLast_PhysAngular;
	d->funcPtrs[DRIVER_FUNC_APPLY_FORCES] = VehPhysForce_OnApplyForces;
	d->funcPtrs[DRIVER_FUNC_COLL_MOVED] = COLL_MOVED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_COLLIDE_DRIVERS] = VehPhysForce_CollideDrivers;
	d->funcPtrs[DRIVER_FUNC_COLL_FIXED] = COLL_FIXED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_JUMP_FRICTION] = VehPhysGeneral_JumpAndFriction;
	d->funcPtrs[DRIVER_FUNC_TRANSLATE_MATRIX] = VehPhysForce_TranslateMatrix;
	d->funcPtrs[DRIVER_FUNC_ANIMATE] = VehFrameProc_LastSpin;
	d->funcPtrs[DRIVER_FUNC_PARTICLES] = VehEmitter_DriverMain;
}


void VehPhysProc_SpinStop_Update(struct Thread *t, struct Driver *d)
{
	(void)t;
	(void)d;
}


void VehPhysProc_SpinStop_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;
}


void VehPhysProc_SpinStop_PhysAngular(struct Thread *t, struct Driver *d)
{
	register u16 angle CTR_PSX_REGISTER("$2");

	(void)t;

	angle = (u16)(CTR_MipsAddLo((u16)d->angle,
	                            CTR_MipsSra(CTR_MipsMulLo(d->ampTurnState, GAME_TRACKER->elapsedTimeMS), VEH_PHYS_PROC_ANGLE_INTEGRATION_SHIFT)) &
	              (ANG_TWO_PI - 1));
	d->angle = (s16)angle;
	d->rotCurr.y = (s16)CTR_MipsAddLo((u16)d->turnWobbleAngle, CTR_MipsAddLo(angle, (u16)d->turnAngleCurr));

	d->rotCurr.w = VehCalc_InterpBySpeed(d->rotCurr.w, CTR_MipsSra(CTR_MipsSll(GAME_TRACKER->elapsedTimeMS, 5), 5), 0);
	d->turnAngleCurr = VehCalc_InterpBySpeed(d->turnAngleCurr, CTR_MipsSra(CTR_MipsSll(GAME_TRACKER->elapsedTimeMS, 7), 5), 0);

	VehPhysForce_RotAxisAngle(&d->matrixMovingDir, CTR_VECTOR_DATA(&(d->AxisAngle1_normalVec)), d->angle);
}


// only Animate is needed, see StopSpin_Init for details

void VehPhysProc_SpinStop_Animate(struct Thread *t, struct Driver *d)
{
	register struct Instance *inst CTR_PSX_REGISTER("$16") = t->inst;
	register int numFrames CTR_PSX_REGISTER("$17");
	int targetFrame;

	numFrames = VehFrameInst_GetNumAnimFrames(inst, inst->animIndex);
	if (numFrames <= 0)
	{
		goto RESUME_DRIVING;
	}

	if (d->KartStates.Spinning.spinDir == -1)
	{
		goto SWEEP_LEFT_TO_RIGHT;
	}
	if (d->KartStates.Spinning.spinDir != 1)
	{
		goto RETURN_TO_CENTER;
	}

	// steer from right to left, to exaggerate the force when steering stops abruptly
	inst->animFrame = (s16)CTR_MipsSubLo((u16)inst->animFrame, VEH_PHYS_PROC_SPIN_STOP_ANIM_SWEEP_STEP);
	if (inst->animFrame >= 0)
	{
		goto DONE;
	}
	inst->animFrame = 0;
	d->KartStates.Spinning.spinDir = 0;
	goto DONE;

SWEEP_LEFT_TO_RIGHT:
	// steer from left to right, to exaggerate the force when steering stops abruptly
	inst->animFrame = (s16)CTR_MipsAddLo((u16)inst->animFrame, VEH_PHYS_PROC_SPIN_STOP_ANIM_SWEEP_STEP);
	if (inst->animFrame < numFrames)
	{
		goto DONE;
	}
	inst->animFrame = numFrames - 1;
	CTR_PSX_KEEP_VALUE(numFrames);
	d->KartStates.Spinning.spinDir = 0;
	goto DONE;

RETURN_TO_CENTER:
	targetFrame = VehFrameInst_GetStartFrame(0, numFrames);
	CTR_PSX_KEEP_VALUE(targetFrame);
	inst->animFrame = VehCalc_InterpBySpeed(inst->animFrame, VEH_PHYS_PROC_SPIN_STOP_ANIM_RETURN_STEP, targetFrame);
	if (inst->animFrame != targetFrame)
	{
		goto DONE;
	}

RESUME_DRIVING:
	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_Driving_Init;

DONE:
	return;
}


void VehPhysProc_SpinStop_Init(struct Thread *t, struct Driver *d)
{
	(void)t;
	d->funcPtrs[DRIVER_FUNC_INIT] = NULL;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = VehPhysProc_SpinStop_Update;
	d->funcPtrs[DRIVER_FUNC_PHYS_LINEAR] = VehPhysProc_SpinStop_PhysLinear;
	d->funcPtrs[DRIVER_FUNC_AUDIO] = VehPhysProc_Driving_Audio;
	d->funcPtrs[DRIVER_FUNC_PHYS_ANGULAR] = VehPhysProc_SpinStop_PhysAngular;
	d->funcPtrs[DRIVER_FUNC_APPLY_FORCES] = VehPhysForce_OnApplyForces;

	d->funcPtrs[DRIVER_FUNC_COLL_MOVED] = COLL_MOVED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_COLLIDE_DRIVERS] = VehPhysForce_CollideDrivers;

	d->funcPtrs[DRIVER_FUNC_COLL_FIXED] = COLL_FIXED_PlayerSearch;
	d->funcPtrs[DRIVER_FUNC_JUMP_FRICTION] = VehPhysGeneral_JumpAndFriction;
	d->funcPtrs[DRIVER_FUNC_TRANSLATE_MATRIX] = VehPhysForce_TranslateMatrix;
	d->funcPtrs[DRIVER_FUNC_ANIMATE] = VehPhysProc_SpinStop_Animate;
	d->funcPtrs[DRIVER_FUNC_PARTICLES] = VehEmitter_DriverMain;
}
