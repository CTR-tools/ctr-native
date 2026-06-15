#include <common.h>

// budget: 4624
// curr: 4380

enum
{
	VEH_PHYS_PROC_ACTION_CARRY_MASK = ACTION_TOUCH_GROUND | ACTION_JUMP_BUTTON_HELD | ACTION_STEER_LEFT | ACTION_HIGH_JUMP | ACTION_TURBO_INPUT_LATCH |
	                                  ACTION_DRIVING_WRONG_WAY | ACTION_TURBO_ITEM | ACTION_WEAPON_FIRE_REQUEST | ACTION_ENGINE_ECHO | ACTION_REVERSING_ENGINE |
	                                  ACTION_RACE_TIMER_FROZEN | ACTION_AIRBORNE | ACTION_BOT | ACTION_BEHIND_START_LINE | ACTION_RACE_FINISHED |
	                                  ACTION_TRACKER_TARGETED | ACTION_CHECKPOINT_BRANCH_PENDING | ACTION_HUMAN_HUMAN_COLLISION | ACTION_REVERSE_STEER_LEFT |
	                                  ACTION_REVERSE_STEER_RIGHT,
};

_Static_assert(VEH_PHYS_PROC_ACTION_CARRY_MASK == 0x7f1f83d5);

static void VehPhysProc_Driving_DecrementTimer(s16 *timer, int elapsed)
{
	if (*timer > 0)
	{
		int value = CTR_MipsSubLo(*timer, elapsed);
		if (value < 0)
			value = 0;

		*timer = (s16)value;
	}
}

static void VehPhysProc_Driving_DecrementTimerCounter(s16 *timer, int elapsed, int *counter)
{
	if (*timer > 0)
	{
		VehPhysProc_Driving_DecrementTimer(timer, elapsed);
		*counter = CTR_MipsAddLo(*counter, elapsed);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006181c-0x80062a2c
void VehPhysProc_Driving_PhysLinear(struct Thread *thread, struct Driver *driver)
{
	struct GameTracker *gGT;
	int gameMode2;

	u8 kartState;
	u8 heldItemID;
	s16 noItemTimer;
	u8 isNumWumpas10;
	u16 driverTimerNegativeFinal;
	u16 driverBaseSpeedUshort;
	int approxTrig;
	int driverBaseSpeed;
	int approximateSpeed;
	int sVar13;
	int iVar14;
	u32 buttonsTapped;
	u16 driverTimerNegativePrelim;
	int driverTimer;
	int timerHazard;
	int approximateSpeed2;
	u32 actionsFlagSetCopy;
	int driverSpeedSmth2;
	struct GamepadBuffer *ptrgamepad;
	u32 cross;
	u32 square;

	int msPerFrame;
	s16 driverRankItemValue;
	u32 itemSound;
	u32 uVar20;
	int stickLY;
	int stickRY;
	int driverSpeedOrSmth = 0;
	struct Thread *driverItemThread;
	struct Shield *shield;
	struct TrackerWeapon *bomb;
	u32 superEngineFireLevel;
	int unk0x80;
	int driverSpeedCopy;

	gGT = sdata->gGT;
	gameMode2 = gGT->gameMode2;

	// If race timer is not supposed to stop for this racer
	if ((driver->actionsFlagSet & ACTION_RACE_TIMER_FROZEN) == 0)
	{
		// set racer's timer to the time on the clock
		driver->timeElapsedInRace = gGT->elapsedEventTime;
	}


	// === Count Timers ===


	// elapsed milliseconds per frame, ~32
	msPerFrame = gGT->elapsedTimeMS;

	if (
	    // time on the clock
	    (gGT->elapsedEventTime < 10 * MINUTE) &&

	    // race timer is not frozen for this player
	    ((driver->actionsFlagSet & ACTION_RACE_TIMER_FROZEN) == 0))
	{
		// increment timer by (speed * time)
		driver->distanceDriven = CTR_MipsAddLo(driver->distanceDriven, CTR_MipsSra(CTR_MipsMulLo(driver->speedApprox, msPerFrame), 8));
	}

	VehPhysProc_Driving_DecrementTimerCounter(&driver->reserves, msPerFrame, &driver->timeSpentUsingReserves);
	VehPhysProc_Driving_DecrementTimer(&driver->turbo_outsideTimer, msPerFrame);
	VehPhysProc_Driving_DecrementTimer(&driver->VehFire_AudioCooldown, msPerFrame);
	VehPhysProc_Driving_DecrementTimerCounter(&driver->wallRubTimer, msPerFrame, &driver->timeSpentAgainstWall);
	VehPhysProc_Driving_DecrementTimer(&driver->jump_ForcedMS, msPerFrame);
	VehPhysProc_Driving_DecrementTimer(&driver->jump_CooldownMS, msPerFrame);
	VehPhysProc_Driving_DecrementTimer(&driver->jump_HighJumpTimerMS, msPerFrame);
	VehPhysProc_Driving_DecrementTimerCounter(&driver->burnTimer, msPerFrame, &driver->timeSpentBurnt);
	VehPhysProc_Driving_DecrementTimerCounter(&driver->squishTimer, msPerFrame, &driver->timeSpentSquished);
	VehPhysProc_Driving_DecrementTimer(&driver->vShiftStartGuardTimer, msPerFrame);
	VehPhysProc_Driving_DecrementTimer(&driver->vShiftWindowTimer, msPerFrame);

	// If Super Engine Cheat is not enabled
	if (!(gameMode2 & CHEAT_ENGINE))
	{
		VehPhysProc_Driving_DecrementTimer(&driver->superEngineTimer, msPerFrame);
	}

	VehPhysProc_Driving_DecrementTimer(&driver->clockReceive, msPerFrame);
	VehPhysProc_Driving_DecrementTimer(&driver->mashingXMakesItBig, msPerFrame);

	// If invisible, without Permanent Invisibility cheat,
	// dont remove invisibleTimer check, or an invalid
	// instFlagsBackup overwrites instFlags
	if ((driver->invisibleTimer != 0) && ((gameMode2 & CHEAT_INVISIBLE) == 0))
	{
		driver->invisibleTimer = CTR_MipsSubLo(driver->invisibleTimer, msPerFrame);

		// if newly visible
		if (driver->invisibleTimer <= 0)
		{
			driver->invisibleTimer = 0;
			driver->instSelf->flags = driver->instFlagsBackup;
			driver->instSelf->alphaScale = 0;
			OtherFX_Play(0x62, 1);
		}
	}

	if (0 < driver->jump_TenBuffer)
		driver->jump_TenBuffer = (s16)CTR_MipsSubLo(driver->jump_TenBuffer, 1);
	if (9 < driver->numWumpas)
		driver->timeSpentInTenWumpa = CTR_MipsAddLo(driver->timeSpentInTenWumpa, msPerFrame);
	if (driver->currentTerrain == 0xE)
		driver->timeSpentInMud = CTR_MipsAddLo(driver->timeSpentInMud, msPerFrame);


	// === Check Last Place ===

	// Last Place, and time is unfrozen
	if ((((driver->driverRank == 7) && (gGT->numPlyrCurrGame == 1)) || ((driver->driverRank == 5) && (gGT->numPlyrCurrGame == 2)) ||
	     ((driver->driverRank == 3) && ((u8)gGT->numPlyrCurrGame > 2))) &&
	    ((driver->actionsFlagSet & ACTION_RACE_TIMER_FROZEN) == 0))
	{
		driver->timeSpentInLastPlace = CTR_MipsAddLo(driver->timeSpentInLastPlace, msPerFrame);
	}

	// === Determine Hazard ===

	driverRankItemValue = 4;

	// if you have a raincloud over your head from potion
	if (driver->thCloud != 0)
		driverRankItemValue = ((struct RainCloud *)driver->thCloud->object)->boolScrollItem;

	// get approximate speed
	approximateSpeed = driver->speedApprox;

	// Action flags (isRaceOver, isTimeFrozen, etc)
	actionsFlagSetCopy = driver->actionsFlagSet;

	// driver->clockReceive
	driverTimer = driver->clockReceive;

	driver->driverRankItemValue = driverRankItemValue;

	// absolute value of speed
	if (approximateSpeed < 0)
		approximateSpeed = CTR_MipsNegLo(approximateSpeed);

	if ((
	        // if you are not impacted by clock weapon
	        (driverTimer == 0) && (
	                                  // get squished timer
	                                  driverTimer = driver->squishTimer,

	                                  // if you are not squished
	                                  driverTimer == 0)) &&
	    ((driverRankItemValue != 0 || (
	                                      // if time on the clock is zero
	                                      driverTimer = gGT->elapsedEventTime, driverTimer == 0))))
	{
		timerHazard = driver->hazardTimer;

		// Hazard timer will not go down unless you keep moving.
		if (approximateSpeed > 0x100)
		{
			// decrease hazard by elapsed time
			timerHazard = CTR_MipsSubLo(timerHazard, msPerFrame);
		}

		timerHazard = timerHazard & 0xfffe;
		if (timerHazard > -1)
			timerHazard = -2;

		driver->hazardTimer = timerHazard;
	}

	// if you are not impacted by hazard (other than clock)
	else
	{
		driverTimerNegativePrelim = driverTimer;

		// if you are not touching the ground
		if ((actionsFlagSetCopy & ACTION_TOUCH_GROUND) == 0)
		{
			// if speed is low
			if (approximateSpeed < 0x101)
				goto speedIsLow;

			// if speed is high...

			// absolute value of clock hazard
			if (driverTimer < 0)
			{
				driverTimerNegativePrelim = (u16)CTR_MipsNegLo(driverTimerNegativePrelim);
			}

		turndriverTimerNegative:
			driverTimerNegativeFinal = CTR_MipsNegLo(driverTimerNegativePrelim) | 1;
		}

		// if you are touching the ground
		else
		{
			// if speed is low
			if (approximateSpeed < 0x101)
			{
			speedIsLow:
				driverTimerNegativePrelim = (u16)driver->hazardTimer;
				driverTimerNegativeFinal = driverTimerNegativePrelim | 1;
				if ((s16)driverTimerNegativePrelim > 0)
					goto turndriverTimerNegative;
			}

			// if speed is high
			else
			{
				// absolute value of clock hazard
				if (driverTimer < 0)
				{
					driverTimerNegativePrelim = (u16)CTR_MipsNegLo(driverTimerNegativePrelim);
				}

				// Use trigonometry with speed and
				// clock timer to make the car waddle

				driverTimer = CTR_MipsSra(driver->clockReceive, 6);
				if (driverTimer > 0x40)
					driverTimer = 0x40;

				timerHazard = CTR_MipsSll(driver->clockReceive, 4);

				// approximate trigonometry
				approxTrig = MATH_Sin(timerHazard);
				approximateSpeed2 = CTR_MipsSra(approximateSpeed, 8);

				if (approximateSpeed2 > 0x20)
					approximateSpeed2 = 0x20;

				// gamepad vibration
				GAMEPAD_ShockForce1(driver, 4, CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsAddLo(driverTimer, CTR_MipsSra(approxTrig, 5)), approximateSpeed2), 0x18));

				driverTimerNegativeFinal = driverTimerNegativePrelim | 1;
			}
		}
		driver->hazardTimer = driverTimerNegativeFinal;
	}


	// === Item Roll ===


	// if Held Item = None (rolling)
	if (driver->heldItemID == 0x10)
	{
		// if Item roll is done
		if (driver->itemRollTimer == 0)
		{
			// Select a random weapon for driver
			VehPhysGeneral_SetHeldItem(driver);

			// if 9 < number of wumpa
			// if wumpa is 10
			isNumWumpas10 = driver->numWumpas > 9;

			// if you have less than 10 wumpa

			// "ding" sound
			itemSound = 0x5e;

			// if you have 10 wumpa
			if (isNumWumpas10)
			{
				// "ka-ching" sound
				itemSound = 0x41;
			}

			// sound of getting weapon
			OtherFX_Play(itemSound, isNumWumpas10);
		}

		// if Item roll is not done
		else
			driver->itemRollTimer = (s16)CTR_MipsSubLo(driver->itemRollTimer, 1);
	}


	// === Item Used By Player ===


	noItemTimer = driver->noItemTimer;
	if (noItemTimer != 0)
	{
		// if Item is about to be gone and Number of Items = 0
		if ((noItemTimer == 1) && (driver->numHeldItems == 0))
		{
			if (
			    // multiplayer game, not battle, weapon was 3 missiles
			    (2 < (u8)gGT->numPlyrCurrGame) && ((gGT->gameMode1 & BATTLE_MODE) == 0) && (driver->heldItemID == 0xB) && (gGT->numPlayersWith3Missiles > 0))
			{
				// keep count
				gGT->numPlayersWith3Missiles--;
			}

			// take away weapon
			driver->heldItemID = 0xf;
		}

		driver->noItemTimer = (s16)CTR_MipsSubLo(noItemTimer, 1);
	}

	if (driver->invincibleTimer != 0)
	{
		driver->invincibleTimer = CTR_MipsSubLo(driver->invincibleTimer, msPerFrame);
		if (driver->invincibleTimer < 0)
			driver->invincibleTimer = 0;
	}


	// === Normal Vector ===


	// action flags
	driver->actionsFlagSetPrevFrame = actionsFlagSetCopy;

	// backup rotation
	driver->rotPrev.x = driver->rotCurr.x;
	driver->rotPrev.y = driver->rotCurr.y;
	driver->rotPrev.z = driver->rotCurr.z;

	// backup position
	driver->posPrev.x = driver->posCurr.x;
	driver->posPrev.y = driver->posCurr.y;
	driver->posPrev.z = driver->posCurr.z;

	driver->jumpHeightPrev = driver->jumpHeightCurr;
	driver->turnAnglePrev = driver->turnAngleCurr;

	// Preserve the subset of action flags that feed driving physics.
	uVar20 = actionsFlagSetCopy & VEH_PHYS_PROC_ACTION_CARRY_MASK;

	// disable input if opening adv hub door with key
	if ((gameMode2 & 0x4004) != 0)
	{
		driver->actionsFlagSet = uVar20;
		return;
	}

	driver->normalVecID = 0;

	if ((actionsFlagSetCopy & ACTION_TOUCH_GROUND) != 0)
		driver->AxisAngle4_normalVec = driver->AxisAngle1_normalVec;
	else
		driver->AxisAngle4_normalVec = driver->AxisAngle2_normalVec;


	// === Check Mask Weapon ===


	actionsFlagSetCopy = uVar20;
	driverItemThread = thread->childThread;
	while (driverItemThread != 0)
	{
		// If thread->modelIndex is Aku or Uka
		if ((driverItemThread->modelIndex == STATIC_UKAUKA) || (driverItemThread->modelIndex == STATIC_AKUAKU))
		{
			// driver is using mask weapon
			actionsFlagSetCopy = uVar20 | ACTION_MASK_WEAPON;
			break;
		}

		// check next player in linked list
		driverItemThread = driverItemThread->siblingThread;
	}


	// === Check Buttons ===


	// pointer to gamepad input of current player (driver)
	ptrgamepad = &sdata->gGamepads->gamepad[(u32)driver->driverID];

	// no hold, no tap
	uVar20 = 0;
	buttonsTapped = 0;

	// If you're not in End-Of-Race menu
	if ((gGT->gameMode1 & END_OF_RACE) == 0)
	{
		uVar20 = ptrgamepad->buttonsHeldCurrFrame;
		buttonsTapped = ptrgamepad->buttonsTapped;
	}

	cross = uVar20 & BTN_CROSS;
	square = uVar20 & BTN_SQUARE;

	// state of kart
	kartState = driver->kartState;


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
			shield->flags |= 2;
			driver->instBubbleHold = NULL;
			goto CheckJumpButtons;
		}

		// item is rolling
		if (driver->itemRollTimer != 0)
		{
			// circle button ends timer, if
			// less than 70 frames (2.3s) remain
			if (driver->itemRollTimer < 70)
				driver->itemRollTimer = 0;

			// skip weapon firing check
			goto CheckJumpButtons;
		}

		// === Item Roll finished before PhysLinear ===

		// If you dont have "roulette" weapon (0x10), and if you dont have "no weapon" (0xf)
		// and if you did not have a weapon last frame (0x3c->0),
		// and if (unknown driverRankItemValue related to 0x4a0),
		// and if you are not being effected by Clock Weapon
		heldItemID = driver->heldItemID;
		if ((heldItemID != 0xF) && (heldItemID != 0x10) && (driver->noItemTimer == 0) && (driverRankItemValue != 1) && (driver->clockReceive == 0))
		{
			// This driver wants to fire a weapon
			actionsFlagSetCopy |= ACTION_WEAPON_FIRE_REQUEST;

			// if numHeldItems == 0
			// wait a full second before next weapon
			driver->noItemTimer = 0x1e;

			// If "held item quantity" is zero
			if (driver->numHeldItems != 0)
			{
				// if numHeldItems > 0,
				// wait 5 frames before next weapon use
				driver->noItemTimer = 5;

				// If you have the Spring weapon
				if (heldItemID == 5)
				{
					if ((driver->jump_CoyoteTimerMS != 0) && (driver->jump_CooldownMS == 0))
					{
						driver->numHeldItems--;
					}
				}

				else
				{
					// only reduce numHeldItem if not using item cheats
					if ((gameMode2 & (CHEAT_BOMBS | CHEAT_TURBO | CHEAT_MASK)) == 0)
						driver->numHeldItems--;
				}
			}
		}
	}


	// === Drift Section ===


CheckJumpButtons:

	// Check for Tapping L1 and R1
	buttonsTapped = buttonsTapped & 0xc00;

	if (
	    // if you're not pressing L1 or R1
	    (buttonsTapped == 0) ||
	    // or you are sliding
	    (driver->kartState == KS_DRIFTING))
	{
		if (
		    // If you are holding L1 or R1 and
		    ((uVar20 & 0xc00) != 0) && (driverRankItemValue != 3))
		{
			if ((actionsFlagSetCopy & ACTION_JUMP_BUTTON_HELD) == 0)
			{
				// 10 frame jump buffer
				driver->jump_TenBuffer = 10;
			}
			goto LAB_8006222c;
		}
		actionsFlagSetCopy &= ~ACTION_JUMP_BUTTON_HELD;
		if (driver->jump_TenBuffer > 0)
			driver->jump_TenBuffer = 0;
	}

	// if you're pressing jump buttons and not sliding
	else
	{
		// if L1 and R1 were being tapped at once
		if (buttonsTapped == 0xc00)
		{
			// set Last Jump button pressed to R1
			driver->buttonUsedToStartDrift = 0x400;
		}

		// if you're not tapping L1 and R1 at once
		else
		{
			// Last Jump button pressed = buttonsTapped
			driver->buttonUsedToStartDrift = buttonsTapped;
		}

		if (driverRankItemValue != 3)
		{
			driver->jump_TenBuffer = 10;
		LAB_8006222c:
			actionsFlagSetCopy |= ACTION_JUMP_BUTTON_HELD;
		}
	}
	if (
	    // If you are holding Square
	    (square != 0) &&

	    // if you're not on any turbo pad
	    ((driver->stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD_MASK) == 0))
	{
		// Set Reserves to zero
		driver->reserves = 0;
	}

	// assume normal gas pedal
	stickRY = 0x80;

	// If you're not in End-Of-Race menu
	if ((gGT->gameMode1 & END_OF_RACE) == 0)
	{
		// gamepadBuffer -> stickRY (for gas or reverse)
		stickRY = (int)ptrgamepad->stickRY;
	}

	if (
	    // If Reserves are not zero
	    (driver->reserves != 0) ||

	    (driverRankItemValue == 6))
	{
		// If you are not holding Cross
		if (cross == 0)
		{
			unk0x80 = VehPhysJoystick_ReturnToRest(stickRY, 0x80, 0);

			if (unk0x80 > -1)
			{
				actionsFlagSetCopy |= ACTION_ACCEL_RELEASED_WITH_RESERVES;
			}
		}

		// If holding Square while moving fast
		if ((square != 0) && (approximateSpeed > 0x300))
		{
			// back wheel skids
			actionsFlagSetCopy |= ACTION_BACK_SKID;
		}

		// if you're on any turbo pad
		if ((driver->stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD_MASK) != 0)
		{
			// assume not holding square until boost is over
			square = 0;
		}

		// Assume you're holding Cross, because
		// you have Reserves and you aren't slowing down
		cross = 0x10;
	}


	// === Gas/Brake section ===


	stickLY = 0x80;

	// If you're not in End-Of-Race menu
	if ((gGT->gameMode1 & END_OF_RACE) == 0)
	{
		stickLY = ptrgamepad->stickLY;
	}

	if (((s8)driver->simpTurnState < 0) || (actionsFlagSetCopy &= ~ACTION_REVERSE_STEER_LEFT, (s8)driver->simpTurnState < 1))
	{
		actionsFlagSetCopy &= ~ACTION_REVERSE_STEER_RIGHT;
	}
	approximateSpeed2 = driver->speedApprox;
	if (approximateSpeed2 < 0)
	{
		approximateSpeed2 = CTR_MipsNegLo(approximateSpeed2);
	}
	if (approximateSpeed2 < 0x300)
	{
		actionsFlagSetCopy &= ~(ACTION_REVERSE_STEER_LEFT | ACTION_REVERSE_STEER_RIGHT);
	}
	approximateSpeed2 = 0;

	// with zero wumpa, should be const_Speed_ClassStat (13140 for Crash Bandicoot)
	// this works for both decomp and original

	// with one wumpa, retail gives 13169

	driverBaseSpeed = VehPhysGeneral_GetBaseSpeed(driver);

	driverBaseSpeedUshort = driverBaseSpeed;

	// If you are not holding Square
	if (square == 0)
	{
		// driverSpeedSmth2 = Racer's Base Speed
		driverSpeedSmth2 = driverBaseSpeed;

		// If you are holding Cross, or if you have Reserves
		if (cross != 0)
		{
		LAB_8006253c:
			actionsFlagSetCopy &= ~ACTION_REVERSING_ENGINE;
			goto LAB_80062548;
		}

		// if you are not holding cross, or have no Reserves...
		// driverSpeedSmth2 is replaced

		driverSpeedSmth2 = VehPhysJoystick_ReturnToRest(stickRY, 0x80, 0);

		driverSpeedOrSmth = CTR_MipsNegLo(driverSpeedSmth2);
		if (driverSpeedSmth2 < 1)
		{
			if ((driverSpeedOrSmth == 0) && ((unk0x80 = VehPhysJoystick_ReturnToRest(stickLY, 0x80, 0),

			                                  (unk0x80 > 99) ||

			                                      ((unk0x80 > 0) && ((actionsFlagSetCopy & ACTION_REVERSING_ENGINE) != 0)))))
			{
				actionsFlagSetCopy |= ACTION_REVERSING_ENGINE;

				driverSpeedSmth2 = CTR_MipsNegLo(driver->const_BackwardSpeed);
				goto LAB_80062548;
			}

			driverSpeedOrSmth = CTR_MipsMulLo(driverBaseSpeed, driverSpeedOrSmth);
			driverSpeedSmth2 = CTR_MipsSra(driverSpeedOrSmth, 7);
			if (driverSpeedOrSmth < 0)
				driverSpeedSmth2 = CTR_MipsSra(CTR_MipsAddLo(driverSpeedOrSmth, 0x7f), 7);

			// remove flag for reversing
			goto LAB_8006253c;
		}
		if ((driver->speedApprox < 0x301) && ((actionsFlagSetCopy & (ACTION_REVERSE_STEER_LEFT | ACTION_REVERSE_STEER_RIGHT)) == 0))
		{
			driverSpeedOrSmth = CTR_MipsMulLo(driver->const_BackwardSpeed, driverSpeedOrSmth);
			if (driverSpeedOrSmth < 0)
				driverSpeedOrSmth = CTR_MipsAddLo(driverSpeedOrSmth, 0x7f);
			approximateSpeed2 = CTR_MipsSra(driverSpeedOrSmth, 7);
			buttonsTapped = ACTION_REVERSING_ENGINE;
		LAB_800625c4:
			uVar20 = actionsFlagSetCopy | buttonsTapped;
		}
		else
		{
			uVar20 = actionsFlagSetCopy | ACTION_ACCEL_PREVENTION;
			if (0 < (s8)driver->simpTurnState)
				uVar20 = actionsFlagSetCopy | ACTION_REVERSE_STEER_RIGHT | ACTION_ACCEL_PREVENTION;
			if ((s8)driver->simpTurnState < 0)
			{
				buttonsTapped = ACTION_REVERSE_STEER_LEFT;
				actionsFlagSetCopy = uVar20;
				goto LAB_800625c4;
			}
		}
	}
	// If you are holding Square
	else
	{
		unk0x80 = VehPhysJoystick_ReturnToRest(stickLY, 0x80, 0);

		if ((unk0x80 < 100) && ((unk0x80 < 1 || ((actionsFlagSetCopy & ACTION_REVERSING_ENGINE) == 0))))
		{
			// if you are not holding cross, and you have no Reserves
			if (cross == 0)
			{
				driverSpeedOrSmth = VehPhysJoystick_ReturnToRest(stickRY, 0x80, 0);

				if (driverSpeedOrSmth < 0)
				{
					driverSpeedOrSmth = CTR_MipsMulLo(driverBaseSpeed, CTR_MipsNegLo(driverSpeedOrSmth));
					if (driverSpeedOrSmth < 0)
						driverSpeedOrSmth = CTR_MipsAddLo(driverSpeedOrSmth, 0xff);
					driverSpeedSmth2 = CTR_MipsSra(driverSpeedOrSmth, 8);

					// gas and brake together
					actionsFlagSetCopy |= ACTION_BRAKE_WITH_ACCEL;

					goto LAB_80062548;
				}

				if (0 < driverSpeedOrSmth)
				{
					driverSpeedOrSmth = CTR_MipsMulLo(driver->const_BackwardSpeed, CTR_MipsNegLo(driverSpeedOrSmth));
					if (driverSpeedOrSmth < 0)
						driverSpeedOrSmth = CTR_MipsAddLo(driverSpeedOrSmth, 0xff);
					driverSpeedSmth2 = CTR_MipsSra(driverSpeedOrSmth, 8);

					// reversing, and gas+brake
					goto LAB_8006248c;
				}

				// driverSpeedOrSmth == 0,
				// no gas, only brake

				// using the brake
				actionsFlagSetCopy |= ACTION_ACCEL_PREVENTION;

				driverSpeedSmth2 = approximateSpeed2;
			}
			// If you are holding cross, or you have Reserves
			else
			{
				// gas and brake together
				actionsFlagSetCopy |= ACTION_BRAKE_WITH_ACCEL;

				driverSpeedSmth2 = CTR_MipsSra(CTR_MipsAddLo(driverBaseSpeed, (u32)driverBaseSpeed >> 31), 1);
			}
			goto LAB_8006253c;
		}
		driverSpeedOrSmth = CTR_MipsMulLo(driver->const_BackwardSpeed, -3);
		driverSpeedSmth2 = CTR_MipsSra(driverSpeedOrSmth, 2);
		if (driverSpeedOrSmth < 0)
			driverSpeedSmth2 = CTR_MipsSra(CTR_MipsAddLo(driverSpeedOrSmth, 3), 2);

	LAB_8006248c:
		// reversing engine, and brakes
		actionsFlagSetCopy |= ACTION_REVERSING_ENGINE | ACTION_BRAKE_WITH_ACCEL;

	LAB_80062548:
		uVar20 = actionsFlagSetCopy & ~(ACTION_REVERSE_STEER_LEFT | ACTION_REVERSE_STEER_RIGHT);
		approximateSpeed2 = driverSpeedSmth2;
	}

	// driving backwards
	if ((uVar20 & ACTION_REVERSING_ENGINE) != 0)
	{
		driver->timeSpentReversing = CTR_MipsAddLo(driver->timeSpentReversing, gGT->elapsedTimeMS);
	}

	// not driving backwards
	else
	{
		if (driver->superEngineTimer != 0)
		{
			// if Racer is moving
			if (0 < approximateSpeed2)
			{
				// not holding brake
				if ((uVar20 & (ACTION_ACCEL_RELEASED_WITH_RESERVES | ACTION_BRAKE_WITH_ACCEL)) == 0)
				{
					driver->actionsFlagSet = uVar20;

					// fire level, depending on numWumpa
					superEngineFireLevel = 0x80;
					if (driver->numWumpas > 9)
						superEngineFireLevel = 0x100;

					// add 0.12s reserves
					VehFire_Increment(driver, 120, (TURBO_PAD | SUPER_ENGINE), superEngineFireLevel);

					uVar20 = driver->actionsFlagSet;
				}
			}
		}
	}

	// if accel prevention (hold square)
	actionsFlagSetCopy = uVar20 & ACTION_ACCEL_PREVENTION;
	if (actionsFlagSetCopy != 0)
	{
		// high speed
		if ((driver->speedApprox > 0x300) || (driver->speedApprox < -0x300))
		{
			// record amount of time with high speed
			driver->timeSpentWithHighSpeed = CTR_MipsAddLo(driver->timeSpentWithHighSpeed, msPerFrame);
		}
	}

	if ((driver->mashingXMakesItBig == 0) ||

	    ((driver->kartState != KS_NORMAL) && (driver->kartState != KS_ANTIVSHIFT)))
	{
		driver->mashXUnknown = 0;
	}

	if (driver->fireSpeed < 1)
	{
		// if Racer is moving, skip next 4 lines of code
		if (approximateSpeed2 > 0)
			goto LAB_800626d4;
	LAB_800626fc:
		// Racer struct + 0x39E = Racer's Base Speed
		driver->fireSpeed = (s16)approximateSpeed2;
	}
	else
	{
		// if Racer is not moving
		if (approximateSpeed2 < 1)
		{
		LAB_800626d4:
			if (driver->mashingXMakesItBig != 0)
				driver->mashXUnknown = (s16)CTR_MipsAddLo((u16)driver->mashXUnknown, 1);
			driver->mashingXMakesItBig = 0x100;
			goto LAB_800626fc;
		}
		// Racer struct + 0x39E = Racer's Base Speed
		driver->fireSpeed = (s16)approximateSpeed2;
	}

	// brakes
	if ((uVar20 & (ACTION_MASK_WEAPON | ACTION_BRAKE_WITH_ACCEL)) == 0)
	{
		driverSpeedOrSmth = driver->terrainMeta2->speedMultiplier;

		if (driverSpeedOrSmth != 0x100)
		{
			// Base Speed = 0xB4 (at Cove water) * Base Speed >> 8
			approximateSpeed2 = CTR_MipsSra(CTR_MipsMulLo(driverSpeedOrSmth, approximateSpeed2), 8);
			driverBaseSpeedUshort = (u16)CTR_MipsSra(CTR_MipsMulLo(driverSpeedOrSmth, driverBaseSpeed), 8);
		}
	}
	driver->terrainScaledBaseSpeed = (s16)driverBaseSpeedUshort;
	driver->baseSpeed = (s16)approximateSpeed2;


	// === Steering Section ===


	// assume neutral steer (drive straight)
	driverSpeedOrSmth = 0x80;

	// If you're not in End-Of-Race menu
	if ((gGT->gameMode1 & END_OF_RACE) == 0)
	{
		// gamepadBuffer -> stickLX
		driverSpeedOrSmth = (int)ptrgamepad->stickLX;
	}

	// default steer strength from class stats
	iVar14 = CTR_MipsAddLo(driver->const_TurnRate, CTR_MipsSll((s8)driver->turnConst, 1) / 5);

	// if mashing X button
	if ((driver->mashXUnknown > 6) && (approximateSpeed < 0x2600))
	{
		// sharp turn
		iVar14 = 0x5a;
		goto UseTurnRate;
	}

	// rubbing on wall now, or recently
	if (driver->wallRubTimer != 0)
	{
		// restrict turn
		iVar14 = 0x30;
		goto UseTurnRate;
	}

	// === not rubbing on wall now, or recently ===

	// if not holding Square (& 0x8)
	// or not using brakes (& 0x20)
	if ((uVar20 & (ACTION_ACCEL_PREVENTION | ACTION_BRAKE_WITH_ACCEL)) == 0)
	{
		// use const_TurnRate + turnConst<<1/5
		goto UseTurnRate;
	}

	// if only holding Square
	if (cross == 0)
	{
		// turn rate
		iVar14 = 0x40;
		goto UseTurnRate;
	}

	// === if holding Square + Cross ===

	// absolute value driver speed
	driverSpeedCopy = driver->speed;
	if (driverSpeedCopy < 0)
		driverSpeedCopy = CTR_MipsNegLo(driverSpeedCopy);

	// As speed increases, turn rate decreases
	iVar14 = VehCalc_MapToRange(driverSpeedCopy, 0x300, CTR_MipsSra(driver->const_Speed_ClassStat, 1), 0x40, iVar14);

UseTurnRate:

	// Steer, based on strength, and LeftStickX
	iVar14 = VehPhysJoystick_GetStrengthAbsolute(driverSpeedOrSmth, iVar14, ptrgamepad->rwd);

	// no desired steer
	if (CTR_MipsNegLo(iVar14) == 0)
	{
		driver->numFramesSpentSteering = 10000;
	}

	// desired steer
	else
	{
		// desired steer left, or active steer left
		if ((iVar14 < 1) || ((s8)driver->simpTurnState < 0))
		{
			// desired steer right, or active steer right
			if ((-1 < iVar14) || (0 < (s8)driver->simpTurnState))
			{
				// active steer has not changed
				goto SkipSetSteer;
			}

			// active steer left
			uVar20 |= ACTION_STEER_LEFT;
		}

		else
		{
			// active steer right
			uVar20 &= ~ACTION_STEER_LEFT;
		}
		driver->numFramesSpentSteering = 0;
	}

SkipSetSteer:

	driver->simpTurnState = (s8)CTR_MipsNegLo(iVar14);

	// Change wheel rotation based on StickLX
	driverSpeedOrSmth = VehPhysJoystick_GetStrengthAbsolute(driverSpeedOrSmth, 0x40, ptrgamepad->rwd);
	driverBaseSpeedUshort = VehCalc_InterpBySpeed((int)driver->wheelRotation, 0x18, CTR_MipsNegLo(driverSpeedOrSmth));
	driver->wheelRotation = (s16)driverBaseSpeedUshort;

	driverSpeedOrSmth = (int)driver->fireSpeed;
	if (driverSpeedOrSmth < 0)
		driverSpeedOrSmth = CTR_MipsNegLo(driverSpeedOrSmth);

	if (((driver->actionsFlagSetPrevFrame & ACTION_TOUCH_GROUND) == 0) || (kartState == KS_DRIFTING))
	{
		driverSpeedOrSmth = CTR_MipsAddLo(driverSpeedOrSmth, 0xf00);
	}
	else
	{
		driverSpeedOrSmth = CTR_MipsSra(CTR_MipsAddLo(driverSpeedOrSmth, approximateSpeed), 1);
	}

	sVar13 = CTR_MipsSra(CTR_MipsSll(CTR_MipsAddLo(CTR_MipsMulLo(driverSpeedOrSmth, 0x89), CTR_MipsMulLo(driver->unkSpeedValue2, 0x177)), 3), 0xc);
	driver->unkSpeedValue2 = sVar13;

	if ((driver->actionsFlagSetPrevFrame & ACTION_ACCEL_PREVENTION) == 0)
	{
		// prevent Basic Speed from being negative
		if (approximateSpeed2 < 0)
			approximateSpeed2 = CTR_MipsNegLo(approximateSpeed2);

		// If base or approximate speed is above the low-speed threshold.
		if ((approximateSpeed2 > 0x200) || (approximateSpeed > 0x200))
		{
			driver->unkSpeedValue1 = (s16)CTR_MipsSubLo(driver->unkSpeedValue1, sVar13);
		}
	}

	// alternate tire colors each frame,
	// if 2e808080 is detected (&1==0),
	// if not RevEngine, and if unkSpeedVal
	if ((driver->unkSpeedValue1 < 1) && ((driver->tireColor & 1) == 0) && (kartState != KS_ENGINE_REVVING))
	{
		// reset 0x3BC
		driver->unkSpeedValue1 = 0x1e00;

		driver->tireColor = 0x2e606061;
	}

	// default tire color
	else
	{
		driver->tireColor = 0x2e808080;
	}

	driver->actionsFlagSet = uVar20;
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80062a2c-0x80062a4c.
void VehPhysProc_Driving_Audio(struct Thread *t, struct Driver *d)
{
	EngineSound_Player(d);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80062a4c-0x80062b74.
void VehPhysProc_Driving_Update(struct Thread *t, struct Driver *d)
{
	// if racer touched the ground in this frame
	if ((d->actionsFlagSet & ACTION_STARTED_TOUCH_GROUND) != 0)
	{
		int simpTurnState = (s8)d->simpTurnState;

		// set Simplified Turning state to its own absolute value
		if (simpTurnState < 0)
		{
			simpTurnState = CTR_MipsNegLo(simpTurnState);
		}

		// if steering hard enough to start a drift
		if (((CTR_MipsSra(CTR_MipsAddLo(d->const_TurnRate, CTR_MipsSll((s8)d->turnConst, 1) / 5), 1) < simpTurnState) &&

		     // player has jump buttons held
		     ((sdata->gGamepads->gamepad[d->driverID].buttonsHeldCurrFrame) & d->buttonUsedToStartDrift) != 0) &&

		    // player is not in accel prevention or braking and
		    ((d->actionsFlagSet & ACTION_ACCEL_PREVENTION) == 0) && (CTR_MipsSra(d->const_Speed_ClassStat, 1) <= d->speedApprox))
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
	    (4 < d->vShiftCount))
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


extern DriverFunc PlayerDrivingFuncTable[DRIVER_FUNC_COUNT];

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80062b74-0x80062ca8.
void VehPhysProc_Driving_Init(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;

	if (((u32)(gGT->levelID - GEM_STONE_VALLEY) >= 5) || (LOAD_IsOpen_AdvHub() != 0))
	{
		// Turbo meter = full
		d->turbo_MeterRoomLeft = 0;

		d->vShiftStartGuardTimer = 0x60;
		d->vShiftWindowTimer = 0x280;

		d->vShiftCount = 0;

		for (int i = 0; i < DRIVER_FUNC_COUNT; i++)
		{
			d->funcPtrs[i] = PlayerDrivingFuncTable[i];
		}

		if (((gGT->gameMode1 & BATTLE_MODE) != 0) && (d->kartState == KS_BLASTED))
		{
			d->invincibleTimer = 0xb40;
		}

		// must put this HERE, so that
		// the above IF-statement works
		d->kartState = KS_NORMAL;
	}
}

DriverFunc PlayerDrivingFuncTable[DRIVER_FUNC_COUNT] = {
    NULL,
    VehPhysProc_Driving_Update,
    VehPhysProc_Driving_PhysLinear,
    VehPhysProc_Driving_Audio,
    VehPhysGeneral_PhysAngular,
    VehPhysForce_OnApplyForces,
    COLL_MOVED_PlayerSearch,
    VehPhysForce_CollideDrivers,
    COLL_FIXED_PlayerSearch,
    VehPhysGeneral_JumpAndFriction,
    VehPhysForce_TranslateMatrix,
    VehFrameProc_Driving,
    VehEmitter_DriverMain,
};


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80062ca8-0x80062d04.
void VehPhysProc_FreezeEndEvent_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	d->simpTurnState = 0;
	d->wheelRotation = 0;
	d->ampTurnState = 0;

	// reset two speed variables
	d->baseSpeed = 0;
	d->fireSpeed = 0;

	// edit flags
	d->actionsFlagSet |= ACTION_ACCEL_PREVENTION;
	d->actionsFlagSet &= ~ACTION_JUMP_BUTTON_HELD;

	if (d->jump_TenBuffer > 0)
		d->jump_TenBuffer = 0;
}


extern DriverFunc PlayerFreezeFuncTable[DRIVER_FUNC_COUNT];

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80062d04-0x80062db0.
void VehPhysProc_FreezeEndEvent_Init(struct Thread *t, struct Driver *d)
{
	if (d->kartState == KS_FREEZE)
		return;

	d->kartState = KS_FREEZE;
	d->speed = 0;
	d->speedApprox = 0;

	for (int i = 0; i < DRIVER_FUNC_COUNT; i++)
	{
		d->funcPtrs[i] = PlayerFreezeFuncTable[i];
	}
}

DriverFunc PlayerFreezeFuncTable[DRIVER_FUNC_COUNT] = {NULL,
                                                       NULL,
                                                       VehPhysProc_FreezeEndEvent_PhysLinear,
                                                       VehPhysProc_Driving_Audio,
                                                       VehPhysGeneral_PhysAngular,
                                                       VehPhysForce_OnApplyForces,
                                                       COLL_MOVED_PlayerSearch,
                                                       VehPhysForce_CollideDrivers,
                                                       COLL_FIXED_PlayerSearch,
                                                       VehPhysGeneral_JumpAndFriction,
                                                       VehPhysForce_TranslateMatrix,
                                                       VehFrameProc_Driving,
                                                       VehEmitter_DriverMain};


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80062db0-0x80062e04.
void VehPhysProc_FreezeVShift_Update(struct Thread *t, struct Driver *d)
{
	// if firespeed = 0 and
	// player is not in accel prevention (4),
	// not pressing Square (4, 6),
	if ((d->fireSpeed == 0) &&
	    // not in player-on-player collision
	    ((d->actionsFlagSet & (ACTION_HUMAN_HUMAN_COLLISION | ACTION_ACCEL_PREVENTION | ACTION_BRAKE_WITH_ACCEL)) == 0))
	{
		// stop kart
		d->speed = 0;
		d->speedApprox = 0;

		return;
	}

	VehPhysProc_Driving_Init(t, d);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80062e04-0x80062e94.
void VehPhysProc_FreezeVShift_ReverseOneFrame(struct Thread *t, struct Driver *d)
{
	VehPhysGeneral_JumpAndFriction(t, d);

	int actionFlagSet = d->actionsFlagSet;

	// if player did not start jumping this frame
	if ((actionFlagSet & ACTION_JUMP_STARTED) == 0)
	{
		// if there are not two humans colliding
		if ((actionFlagSet & ACTION_HUMAN_HUMAN_COLLISION) == 0)
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

		return;
	}

	VehPhysProc_Driving_Init(t, d);
}


extern DriverFunc PlayerAntiVShiftFuncTable[DRIVER_FUNC_COUNT];

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80062e94-0x80062f4c.
void VehPhysProc_FreezeVShift_Init(struct Thread *t, struct Driver *d)
{
	d->kartState = KS_ANTIVSHIFT;

	// Turbo meter = full
	d->turbo_MeterRoomLeft = 0;

	// turn off 29th flag of actions flag set (means players dont collide anymore)
	d->actionsFlagSet &= ~ACTION_HUMAN_HUMAN_COLLISION;

	for (int i = 0; i < DRIVER_FUNC_COUNT; i++)
	{
		d->funcPtrs[i] = PlayerAntiVShiftFuncTable[i];
	}
}


DriverFunc PlayerAntiVShiftFuncTable[DRIVER_FUNC_COUNT] = {NULL,
                                                           VehPhysProc_FreezeVShift_Update,
                                                           VehPhysProc_Driving_PhysLinear,
                                                           VehPhysProc_Driving_Audio,
                                                           VehPhysGeneral_PhysAngular,
                                                           VehPhysForce_OnApplyForces,
                                                           COLL_MOVED_PlayerSearch,
                                                           VehPhysForce_CollideDrivers,
                                                           COLL_FIXED_PlayerSearch,
                                                           VehPhysProc_FreezeVShift_ReverseOneFrame,
                                                           VehPhysForce_TranslateMatrix,
                                                           VehFrameProc_Driving,
                                                           VehEmitter_DriverMain};


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80062f4c-0x80063634.
void VehPhysProc_PowerSlide_PhysAngular(struct Thread *th, struct Driver *driver)
{
	int cVar1;
	char bVar2;
	char bVar3;
	struct GameTracker *gGT;
	int sVar5;
	u16 uVar6;
	u16 uVar7;
	int iVar8;
	int iVar9;
	u32 uVar10;
	int iVar11;
	int iVar13;
	u32 uVar14;
	int iVar15;

	int iVar12_A;
	int elapsedTimeDouble;
	int iVar12_C;
	int iVar12_D;

	int absVal_NumFrameDrift;
	int absVal_DistortCurr;
	int absVal_DistortVel;

	gGT = sdata->gGT;

	iVar12_A = CTR_MipsSubLo(CTR_MipsAddLo(CTR_MipsSubLo(driver->axisRotationX, driver->angle), 0x800) & 0xfff, 0x800);
	if (iVar12_A != 0)
	{
		// decrease by 1/8
		// val = val * 7/8
		iVar13 = CTR_MipsSra(iVar12_A, 3);

		if (iVar13 == 0)
		{
			iVar13 = 1;
		}

		elapsedTimeDouble = CTR_MipsSra(CTR_MipsSll(gGT->elapsedTimeMS, 6), 5);

		if (iVar13 > elapsedTimeDouble)
			iVar13 = elapsedTimeDouble;

		int minElapsedDelta = CTR_MipsNegLo(elapsedTimeDouble);
		if (iVar13 < minElapsedDelta)
			iVar13 = minElapsedDelta;

		// change player rotation
		driver->angle = (s16)CTR_MipsAddLo((u16)driver->angle, iVar13);

		driver->axisRotationX = (s16)(CTR_MipsSubLo((u16)driver->axisRotationX, iVar13) & 0xfff);
	}

	// positive cam spin rate
	iVar13 = (int)driver->const_Drifting_CameraSpinRate;

	if (driver->multDrift < 0)
	{
		// negative cam spin rate
		iVar13 = CTR_MipsNegLo(iVar13);
	}

	PhysLerpRot(driver, iVar13);

	// turning rate
	iVar12_D = driver->rotationSpinRate;

	// drift direction
	iVar15 = (int)driver->multDrift;

	bVar3 = false;

	iVar9 = (s8)driver->simpTurnState;

	iVar13 = CTR_MipsSll(iVar9, 8);

	// if drifting to the right
	if (iVar15 < 0)
	{
		// if steering to the right
		if (iVar13 < 1)
		{
			iVar13 = CTR_MipsNegLo(CTR_MipsSll(iVar9, 8));

			// const_SteerVel_DriftStandard
			iVar9 = CTR_MipsNegLo((s8)driver->const_SteerVel_DriftStandard);
		}

		// if steering to the left
		else
		{
			// const_SteerVel_DriftSwitchWay
			iVar9 = CTR_MipsNegLo((s8)driver->const_SteerVel_DriftSwitchWay);
		}
	}

	// if drifting to the left
	else
	{
		// if steering to the right
		if (iVar13 < 0)
		{
			iVar13 = CTR_MipsNegLo(CTR_MipsSll(iVar9, 8));

			// const_SteerVel_DriftSwitchWay
			cVar1 = (s8)driver->const_SteerVel_DriftSwitchWay;
		}

		// if steering to the left
		else
		{
			// const_SteerVel_DriftStandard
			cVar1 = (s8)driver->const_SteerVel_DriftStandard;
		}

		iVar9 = (int)cVar1;
	}

	// Map "simpTurnState" from [0, const_TurnRate] to [0, driftDirection]
	iVar13 = VehCalc_MapToRange(iVar13, 0, CTR_MipsSll(CTR_MipsAddLo(driver->const_TurnRate, CTR_MipsSll((s8)driver->turnConst, 1) / 5), 8), 0,
	                            CTR_MipsSll(iVar9, 8));

	if ((iVar13 < 0) || ((
	                        // compare two turning rates
	                        bVar2 = iVar13 < iVar12_D,

	                        iVar13 == 0 && (iVar12_D < 0))))
	{
		bVar3 = true;
		iVar13 = CTR_MipsNegLo(iVar13);
		iVar12_D = CTR_MipsNegLo(iVar12_D);
		iVar15 = CTR_MipsNegLo(iVar15);
		bVar2 = iVar13 < iVar12_D;
	}

	// 0x464 and 0x466 impact turning somehow

	if (bVar2)
	{
		iVar12_D = CTR_MipsSubLo(iVar12_D, CTR_MipsSra(CTR_MipsMulLo(driver->const_DriftSpinRateDecel, gGT->elapsedTimeMS), 5));
		bVar2 = iVar12_D < iVar13;
	}
	else
	{
		iVar12_D = CTR_MipsAddLo(iVar12_D, CTR_MipsSra(CTR_MipsMulLo(driver->const_DriftSpinRateAccel, gGT->elapsedTimeMS), 5));
		bVar2 = iVar13 < iVar12_D;
	}

	if (bVar2)
	{
		iVar12_D = iVar13;
	}

	// if not holding a drift direction,
	// interpolate to "neutral" drift
	if ((iVar13 == 0) || (iVar15 == 0))
	{
	LAB_80063244:

		// Interpolate by 1 unit, until zero
		driver->KartStates.Drifting.numFramesDrifting = VehCalc_InterpBySpeed((int)driver->KartStates.Drifting.numFramesDrifting, 1, 0);
	}

	// if holding a drift
	else
	{
		// if drifting right
		if (iVar15 < 1)
		{
			driver->KartStates.Drifting.numFramesDrifting = (s16)CTR_MipsSubLo((u16)driver->KartStates.Drifting.numFramesDrifting, 1);

			if (driver->KartStates.Drifting.numFramesDrifting > 0)
				driver->KartStates.Drifting.numFramesDrifting = 0;
		}

		// if drifting left
		else
		{
			driver->KartStates.Drifting.numFramesDrifting = (s16)CTR_MipsAddLo((u16)driver->KartStates.Drifting.numFramesDrifting, 1);

			if (driver->KartStates.Drifting.numFramesDrifting < 0)
				driver->KartStates.Drifting.numFramesDrifting = 0;
		}
	}
	if (bVar3)
	{
		iVar12_D = CTR_MipsNegLo(iVar12_D);
		iVar15 = CTR_MipsNegLo(iVar15);
	}

	// Map value from [oldMin, oldMax] to [newMin, newMax]
	// inverting newMin and newMax will give an inverse range mapping
	iVar13 = VehCalc_MapToRange((int)driver->KartStates.Drifting.driftTotalTimeMS, 0, CTR_MipsSll((u8)driver->const_DriftTurnRampFrames, 5),
	                            CTR_MipsSra(CTR_MipsMulLo((s8)driver->const_DriftTurnStartupScale, driver->multDrift), 8), iVar15);
	if (-1 < iVar13)
	{
		if (iVar12_D < CTR_MipsNegLo(iVar13))
		{
			iVar12_D = CTR_MipsNegLo(iVar13);
		}
		sVar5 = (s16)iVar12_D;
		if (0 < iVar13)
			goto LAB_800632cc;
	}
	sVar5 = (s16)iVar12_D;
	if (CTR_MipsNegLo(iVar13) < iVar12_D)
	{
		sVar5 = (s16)CTR_MipsNegLo(iVar13);
	}
LAB_800632cc:
	iVar12_D = iVar13;
	if (iVar13 < 0)
	{
		iVar12_D = CTR_MipsNegLo(iVar13);
	}
	driver->rotationSpinRate = sVar5;
	iVar9 = (int)sVar5;

	// Map value from [oldMin, oldMax] to [newMin, newMax]
	// inverting newMin and newMax will give an inverse range mapping
	iVar12_D = VehCalc_MapToRange(iVar12_D, 0, CTR_MipsAddLo((s8)driver->const_DriftTurnBase, CTR_MipsSll((s8)driver->turnConst, 2) / 5), 0,
	                              (int)driver->const_DriftTurnAngleScale);

	iVar15 = iVar9;

	if (iVar9 < 0)
		iVar15 = CTR_MipsNegLo(iVar9);

	// iVar13 and iVar9 have different signs
	iVar8 = (int)driver->const_DriftTurnOppositeDirectionAngle;
	iVar11 = (s8)driver->const_SteerVel_DriftSwitchWay;

	// if both numbers have same sign,
	// either both < 0, or both >= 0
	if ((iVar13 ^ iVar9) >= 0)
	{
		iVar8 = (int)driver->const_DriftTurnSameDirectionAngle;
		iVar11 = (s8)driver->const_SteerVel_DriftStandard;
	}

	if (iVar13 < 0)
	{
		iVar12_D = CTR_MipsNegLo(iVar12_D);
		iVar8 = CTR_MipsNegLo(iVar8);
	}

	// Map value from [oldMin, oldMax] to [newMin, newMax]
	// inverting newMin and newMax will give an inverse range mapping
	iVar15 = VehCalc_MapToRange(iVar15, 0, CTR_MipsSll(iVar11, 8), 0, iVar8);

	iVar12_D = CTR_MipsSubLo(CTR_MipsAddLo(iVar12_D, iVar15), driver->turnAngleCurr);

	iVar15 = CTR_MipsSra(iVar12_D, 3);

	sVar5 = (s16)iVar15;
	if (iVar12_D != 0)
	{
		if (iVar15 == 0)
		{
			sVar5 = 1;
		}
		driver->turnAngleCurr = (s16)CTR_MipsAddLo((u16)driver->turnAngleCurr, sVar5);
	}

	absVal_NumFrameDrift = driver->KartStates.Drifting.numFramesDrifting;

	if (absVal_NumFrameDrift < 0)
		absVal_NumFrameDrift = CTR_MipsNegLo(absVal_NumFrameDrift);

	// get half of spin-out constant,
	// this determines when to start making tire sound effects,
	// after the turbo meter finishes filling past it's max capacity

	// if you drift beyond the limit of the turbo meter
	if (((u8)driver->const_Drifting_FramesTillSpinout >> 1) < absVal_NumFrameDrift)
	{
		// Play the SFX of near-spinout

		absVal_DistortCurr = driver->turnWobbleAngle;
		if (absVal_DistortCurr < 0)
			absVal_DistortCurr = CTR_MipsNegLo(absVal_DistortCurr);

		// if low distortion
		if (absVal_DistortCurr < 10)
		{
			// count up for 8 frames
			driver->turnWobbleTimer = 8;

			// distortion, rate of change
			driver->turnWobbleVelocity = 0x14;

			if (iVar13 < 0)
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

	absVal_DistortCurr = driver->turnWobbleAngle;
	if (absVal_DistortCurr < 0)
		absVal_DistortCurr = CTR_MipsNegLo(absVal_DistortCurr);

	// if distortion is too high
	if (0x32 < absVal_DistortCurr)
	{
		// stop increasing distortion,
		// go back down
		driver->turnWobbleTimer = 0;
	}

	// frame countdown over
	if (driver->turnWobbleTimer == 0)
	{
		// nearing spinout sfx
		driver->turnWobbleVelocity = 10;

		if (0 < driver->turnWobbleAngle)
			driver->turnWobbleVelocity = (s16)CTR_MipsNegLo(driver->turnWobbleVelocity);

		absVal_DistortVel = driver->turnWobbleVelocity;
		if (absVal_DistortVel < 0)
			absVal_DistortVel = CTR_MipsNegLo(absVal_DistortVel);

		// move down until zero
		sVar5 = VehCalc_InterpBySpeed(driver->turnWobbleAngle, absVal_DistortVel, 0);
	}

	// frames counting down
	else
	{
		driver->turnWobbleTimer = (s16)CTR_MipsSubLo((u16)driver->turnWobbleTimer, 1);

		// move up each frame
		sVar5 = CTR_MipsAddLo((u16)driver->turnWobbleAngle, (u16)driver->turnWobbleVelocity);
	}

	// near-spinout distortion SFX
	driver->turnWobbleAngle = sVar5;

	driver->ampTurnState = (s16)CTR_MipsAddLo(iVar9, iVar13);

	driver->angle = (s16)(CTR_MipsAddLo((u16)driver->angle, CTR_MipsSra(CTR_MipsMulLo(driver->ampTurnState, gGT->elapsedTimeMS), 0xd)) & 0xfff);

	if (driver->KartStates.Drifting.driftBoostTimeMS != 0)
	{
		// decrease by elpased time
		driver->KartStates.Drifting.driftBoostTimeMS = (s16)CTR_MipsSubLo((u16)driver->KartStates.Drifting.driftBoostTimeMS, (u16)gGT->elapsedTimeMS);

		if (driver->KartStates.Drifting.driftBoostTimeMS < 0)
			driver->KartStates.Drifting.driftBoostTimeMS = 0;

		sVar5 = CTR_MipsSra(CTR_MipsMulLo((u8)driver->const_DriftBoostAxisKickRate, gGT->elapsedTimeMS), 5);

		if (driver->turnAngleCurr < 0)
		{
			sVar5 = CTR_MipsNegLo(sVar5);
		}

		driver->axisRotationX = (s16)(CTR_MipsAddLo((u16)driver->axisRotationX, sVar5) & 0xfff);
	}

	driver->rotCurr.y = (s16)CTR_MipsAddLo(CTR_MipsAddLo((u16)driver->turnWobbleAngle, (u16)driver->angle), (u16)driver->turnAngleCurr);

	// increment this by milliseconds
	driver->KartStates.Drifting.driftTotalTimeMS = (s16)CTR_MipsAddLo((u16)driver->KartStates.Drifting.driftTotalTimeMS, (u16)gGT->elapsedTimeMS);

	if (driver->KartStates.Drifting.driftTotalTimeMS > CTR_MipsSll((u8)driver->const_DriftTurnRampFrames, 5))
		driver->KartStates.Drifting.driftTotalTimeMS = (s16)CTR_MipsSll((u8)driver->const_DriftTurnRampFrames, 5);

	PhysTerrainSlope(driver);
}

void PhysLerpRot(struct Driver *driver, int targetRotW)
{
	int remainingRot = CTR_MipsSubLo(driver->rotCurr.w, targetRotW);
	if (remainingRot < 0)
		remainingRot = CTR_MipsNegLo(remainingRot);

	int lerpStep = CTR_MipsSra(remainingRot, 3);

	if (lerpStep == 0)
	{
		lerpStep = 1;
	}

	int maxLerpStep = (u8)driver->const_DriftCameraLerpStep;
	if (lerpStep < (u8)driver->const_DriftCameraLerpStep)
	{
		maxLerpStep = lerpStep;
	}

	// Interpolate rotation by speed
	driver->rotPrev.w = VehCalc_InterpBySpeed((int)driver->rotPrev.w, 8, maxLerpStep);

	// Interpolate rotation by speed
	driver->rotCurr.w = VehCalc_InterpBySpeed((int)driver->rotCurr.w, CTR_MipsSra(CTR_MipsMulLo(driver->rotPrev.w, sdata->gGT->elapsedTimeMS), 5), targetRotW);
}

void PhysTerrainSlope(struct Driver *driver)
{
	VehPhysForce_RotAxisAngle(&driver->matrixMovingDir, driver->AxisAngle1_normalVec.v, (int)driver->angle);
	gte_SetRotMatrix(&driver->matrixMovingDir);
	VehPhysForce_CounterSteer(driver);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063634-0x8006364c.
void VehPhysProc_PowerSlide_Finalize(struct Driver *d)
{
	d->timeUntilDriftSpinout = (s16)CTR_MipsSll((u8)d->const_DriftReleaseTurnAssistFrames, 5);
	d->previousFrameMultDrift = d->multDrift;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006364c-0x800638d4.
void VehPhysProc_PowerSlide_Update(struct Thread *t, struct Driver *d)
{
	s16 noInputTime;
	int incrementReserves;
	int meterLeft;
	int highMeter;
	struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[d->driverID];

	// This is the distance remaining that can be filled
	meterLeft = d->turbo_MeterRoomLeft;

	if ((pad->buttonsTapped & (BTN_R1 | BTN_L1)) == 0)
	{
		// If there is no room in the turbo meter left to fill
		if (meterLeft == 0)
		{
			// If you have not attempted to boost 3 times in a row
			if (d->KartStates.Drifting.numBoostsAttempted < 3)
			{
				// set turbo meter to empty
				meterLeft = CTR_MipsSll((u8)d->const_turboMaxRoom, 5);
			}
		}

		// If the turbo meter is not full
		else
		{
			// decreaes the amoutn of room remaining, by elapsed milliseconds per frame, ~32
			meterLeft = (s16)CTR_MipsSubLo((u16)meterLeft, (u16)sdata->gGT->elapsedTimeMS);

			// if the bar goes beyond full
			if (meterLeft < 0)
			{
				// set bar to full
				meterLeft = 0;
			}

			// If bar is full
			if (meterLeft == 0)
			{
				OtherFX_Play_Echo(0xf, 1, d->actionsFlagSet & ACTION_ENGINE_ECHO);


				// Add to your number of boost attempts, this makes it
				// so you can't attempt to boost again until you release L1 + R1
				d->KartStates.Drifting.numBoostsAttempted = (s8)CTR_MipsAddLo((u8)d->KartStates.Drifting.numBoostsAttempted, 3);
			}
		}
	}

	// If you do press L1 or R1
	else
	{
		// Set drift timer to zero, start the drift
		d->KartStates.Drifting.numFramesDrifting = 0;

		// If turbo meter is not empty
		if (meterLeft != 0)
		{
			// const_turboLowRoomWarning
			// get length where turbo turns from green to red
			highMeter = CTR_MipsSll((u8)d->const_turboLowRoomWarning, 5);

			// If distance remaining to be filled in turbo bar, is less than,
			// the distance remaining from the red/green "turning point" to the end,

			// If meter is in the red
			if (meterLeft < highMeter)
			{
				// reserves_gain = map from old range to new range,
				// the more room remaining to fill, the less boost you get
				// old minMax: [zero -> const_turboLowRoomWarning]
				// new minMax: [const_turboFullBarReserveGain, -> zero]
				incrementReserves = VehCalc_MapToRange(meterLeft, 0, highMeter, CTR_MipsSll((u8)d->const_turboFullBarReserveGain, 5), 0);

				VehFire_Increment(

				    // driver
				    d,

				    // amount of reserves
				    incrementReserves,

				    2,

				    // fire level, bigger boost for attempt number (1,2, or 3)
				    CTR_MipsSll(d->KartStates.Drifting.numBoostsSuccess, 6));

				// increase the counter for number of times you've boosted in a row (0-3)
				d->KartStates.Drifting.numBoostsSuccess = (s8)CTR_MipsAddLo((u8)d->KartStates.Drifting.numBoostsSuccess, 1);

				// if you've boosted less than 3 times in a row
				if (d->KartStates.Drifting.numBoostsSuccess < 3)
				{
					// give a chance to boost again
					d->actionsFlagSet |= ACTION_TURBO_INPUT_LATCH;
				}

				// drift boost meter = constant
				d->KartStates.Drifting.driftBoostTimeMS = (s16)CTR_MipsSll((u8)d->const_DriftBoostDurationFrames, 5);
			}

			// If meter is in the green
			else
			{
				// force exhaust feedback for the failed boost
				d->failedBoostExhaustTimer = 8;
			}

			meterLeft = 0;

			// increase number of boost attempts (both success and failure)
			d->KartStates.Drifting.numBoostsAttempted = (s8)CTR_MipsAddLo((u8)d->KartStates.Drifting.numBoostsAttempted, 1);
		}
	}

	d->turbo_MeterRoomLeft = meterLeft;

	// 1.0 seconds
	noInputTime = 0x3c0;

	// If the "spin-out" constant is less than your drift counter
	if (((u8)d->const_Drifting_FramesTillSpinout < d->KartStates.Drifting.numFramesDrifting) ||

	    ((d->speedApprox < 0 && (
	                                // 2.0 seconds
	                                noInputTime = 0x780,

	                                // if you're not on any turbo pad
	                                (d->stepFlagSet & COLL_STEP_TRIGGER_TURBO_PAD_MASK) == 0))))
	{
		// Make the character spin out from too much drifting

		// Set amount of NoInput
		d->NoInputTimer = noInputTime;

		VehPhysProc_SpinFirst_Init(t, d);
	}

	// if you aren't spinning out
	else
	{
		// Switchway drift counts frames negative; spin out after the negative limit.
		if ((d->KartStates.Drifting.numFramesDrifting < CTR_MipsNegLo((u8)d->const_Drifting_FramesTillSpinout)) ||

		    // speed is less than half the driver's speed classStat
		    (((d->speed < CTR_MipsSra(d->const_Speed_ClassStat, 1) ||

		       ((d->actionsFlagSet & (ACTION_DRIVING_AGAINST_WALL | ACTION_ACCEL_PREVENTION | ACTION_BRAKE_WITH_ACCEL)) != 0)) ||

		      // If the gamepad input is...
		      ((pad->buttonsHeldCurrFrame &

		        // does not include the jump button that you used to start drifting
		        d->buttonUsedToStartDrift) == 0))))
		{
			// Stop drifting, just drive
			VehPhysProc_PowerSlide_Finalize(d);
			VehPhysProc_Driving_Init(t, d);
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800638d4-0x80063920.
void VehPhysProc_PowerSlide_PhysLinear(struct Thread *thread, struct Driver *driver)
{
	VehPhysProc_Driving_PhysLinear(thread, driver);
	driver->actionsFlagSet |= ACTION_BACK_SKID | ACTION_FRONT_SKID;
	driver->timeSpentDrifting = CTR_MipsAddLo(driver->timeSpentDrifting, sdata->gGT->elapsedTimeMS);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063920-0x80063934.
void VehPhysProc_PowerSlide_InitSetUpdate(struct Thread *t, struct Driver *d)
{
	d->funcPtrs[DRIVER_FUNC_INIT] = 0;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = VehPhysProc_PowerSlide_Update;
}


extern DriverFunc PlayerDriftingFuncTable[DRIVER_FUNC_COUNT];

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063934-0x80063a44.
void VehPhysProc_PowerSlide_Init(struct Thread *t, struct Driver *d)
{
	d->kartState = KS_DRIFTING;

	// Character's Drift stat + ((Turning multiplier? << 2) / 5) * 100
	int drift = CTR_MipsSll(CTR_MipsAddLo((s8)d->const_DriftTurnBase, CTR_MipsSll((s8)d->turnConst, 2) / 5), 8);

	// if simplified turning state is negative (means you're turning right)
	if ((s8)d->simpTurnState < 0)
	{
		// also make Multiplied drift negative
		drift = CTR_MipsNegLo(drift);
	}

	d->multDrift = (s16)drift;

	d->rotationSpinRate = 0;
	d->turnAngleLerpVel = 0;
	d->timeUntilDriftSpinout = 0;

	// Turbo meter space left to fill = Length of Turbo meter << 5
	d->turbo_MeterRoomLeft = (s16)CTR_MipsSll((u8)d->const_turboMaxRoom, 5);

	// erase union in driver struct
	d->KartStates.Drifting.numFramesDrifting = 0;
	d->KartStates.Drifting.driftBoostTimeMS = 0;
	d->KartStates.Drifting.driftTotalTimeMS = 0;
	d->KartStates.Drifting.numBoostsAttempted = 0;
	d->KartStates.Drifting.numBoostsSuccess = 0;

	for (int i = 0; i < DRIVER_FUNC_COUNT; i++)
	{
		d->funcPtrs[i] = PlayerDriftingFuncTable[i];
	}
}

DriverFunc PlayerDriftingFuncTable[DRIVER_FUNC_COUNT] = {
    VehPhysProc_PowerSlide_InitSetUpdate,
    NULL,
    VehPhysProc_PowerSlide_PhysLinear,
    VehPhysProc_Driving_Audio,
    VehPhysProc_PowerSlide_PhysAngular,
    VehPhysForce_OnApplyForces,
    COLL_MOVED_PlayerSearch,
    VehPhysForce_CollideDrivers,
    COLL_FIXED_PlayerSearch,
    VehPhysGeneral_JumpAndFriction,
    VehPhysForce_TranslateMatrix,
    VehFrameProc_Driving,
    VehEmitter_DriverMain,
};


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063a44-0x80063af8.
void VehPhysProc_SlamWall_PhysAngular(struct Thread *t, struct Driver *d)
{
	int elapsedTimeMS = sdata->gGT->elapsedTimeMS;

	d->angle = (s16)(CTR_MipsAddLo((u16)d->angle, CTR_MipsSra(CTR_MipsMulLo(d->ampTurnState, elapsedTimeMS), 0xd)) & 0xfff);

	d->rotCurr.y = (s16)CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnWobbleAngle, (u16)d->angle), (u16)d->turnAngleCurr);

	d->rotCurr.w = VehCalc_InterpBySpeed(d->rotCurr.w, CTR_MipsSra(CTR_MipsSll(elapsedTimeMS, 5), 5), 0);

	d->turnAngleCurr = VehCalc_InterpBySpeed(d->turnAngleCurr, CTR_MipsSra(CTR_MipsSll(elapsedTimeMS, 7), 5), 0);

	VehPhysForce_RotAxisAngle(&d->matrixMovingDir, d->AxisAngle1_normalVec.v, d->angle);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063af8-0x80063b00.
void VehPhysProc_SlamWall_Update(struct Thread *t, struct Driver *d)
{
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063b00-0x80063b2c.
void VehPhysProc_SlamWall_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063b2c-0x80063bd4.
void VehPhysProc_SlamWall_Animate(struct Thread *t, struct Driver *d)
{
	struct Instance *inst = t->inst;

	inst->animFrame = (s16)CTR_MipsAddLo((u16)inst->animFrame, 1);

	d->matrixIndex = (u8)CTR_MipsAddLo(d->matrixIndex, 1);

	int numFrames = VehFrameInst_GetNumAnimFrames(inst, inst->animIndex);

	if (inst->animFrame < (numFrames - 1))
	{
		return;
	}

	numFrames = VehFrameInst_GetNumAnimFrames(inst, 0);
	if (numFrames > 0)
	{
		inst->animIndex = 0;
		inst->animFrame = VehFrameInst_GetStartFrame(0, numFrames);
		d->matrixArray = 0;
		d->matrixIndex = 0;
	}

	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_Driving_Init;
}


DriverFunc PlayerCrashingFuncTable[DRIVER_FUNC_COUNT] = {
    0,
    VehPhysProc_SlamWall_Update,
    VehPhysProc_SlamWall_PhysLinear,
    VehPhysProc_Driving_Audio,
    VehPhysProc_SlamWall_PhysAngular,
    VehPhysForce_OnApplyForces,
    COLL_MOVED_PlayerSearch,
    VehPhysForce_CollideDrivers,
    COLL_FIXED_PlayerSearch,
    VehPhysGeneral_JumpAndFriction,
    VehPhysForce_TranslateMatrix,
    VehPhysProc_SlamWall_Animate,
    VehEmitter_DriverMain,
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063bd4-0x80063cf4.
void VehPhysProc_SlamWall_Init(struct Thread *t, struct Driver *d)
{
	int i;
	struct Instance *inst;
	inst = d->instSelf;

	// NOTE(aalhendi): Retail only writes X/Y scale here.
	inst->scale[0] = 0xccc;
	inst->scale[1] = 0xccc;

	d->kartState = KS_CRASHING;

	d->numFramesSpentSteering = 10000;

	d->Screen_OffsetY = 0;
	d->ampTurnState = 0;
	d->speedometerNeedleValue = 0;
	d->speed = 0;
	d->speedApprox = 0;
	d->baseSpeed = 0;
	d->fireSpeed = 0;
	d->rotationSpinRate = 0;
	d->turnAngleLerpVel = 0;
	d->turnWobbleAngle = 0;
	d->turnWobbleVelocity = 0;
	d->turnWobbleTimer = 0;
	d->turbo_MeterRoomLeft = 0;
	d->turbo_outsideTimer = 0;
	d->VehFire_AudioCooldown = 0;
	d->reserves = 0;
	d->distanceFromGround = 0;
	d->jumpSquishStretch = 0;
	d->unk40E = 0;
	d->jumpSquishStretch2 = 0;

	// all ints set to zero
	d->xSpeed = 0;
	d->ySpeed = 0;
	d->zSpeed = 0;
	d->velocity.x = 0;
	d->velocity.y = 0;
	d->velocity.z = 0;

	for (i = 0; i < DRIVER_FUNC_COUNT; i++)
	{
		d->funcPtrs[i] = PlayerCrashingFuncTable[i];
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063cf4-0x80063d44.
void VehPhysProc_SpinFirst_Update(struct Thread *t, struct Driver *d)
{
	int speedApprox;

	// cooldown after the spinning started
	if (d->NoInputTimer != 0)
	{
		// absolute value
		speedApprox = d->speedApprox;
		if (speedApprox < 0)
			speedApprox = CTR_MipsNegLo(speedApprox);

		// quit if moving quickly
		if (speedApprox > 0x2ff)
			return;
	}

	// if speed has slown to near-halt,
	// or if NoInputTimer ran out
	VehPhysProc_SpinLast_Init(t, d);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063d44-0x80063dc8.
void VehPhysProc_SpinFirst_PhysLinear(struct Thread *t, struct Driver *d)
{
	int elapsedTimeMS = sdata->gGT->elapsedTimeMS;

	d->NoInputTimer = (s16)CTR_MipsSubLo((u16)d->NoInputTimer, (u16)elapsedTimeMS);
	if (d->NoInputTimer < 0)
		d->NoInputTimer = 0;

	VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;

	d->actionsFlagSet |= ACTION_WARP | ACTION_FRONT_SKID | ACTION_BACK_SKID | ACTION_ACCEL_PREVENTION;

	d->timeSpentSpinningOut = CTR_MipsAddLo(d->timeSpentSpinningOut, elapsedTimeMS);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063dc8-0x80063eac.
void VehPhysProc_SpinFirst_PhysAngular(struct Thread *t, struct Driver *d)
{
	int elapsedTimeMS = sdata->gGT->elapsedTimeMS;

	d->numFramesSpentSteering = 10000;

	d->rotationSpinRate = (s16)CTR_MipsSubLo((u16)d->rotationSpinRate, CTR_MipsSra(d->rotationSpinRate, 3));
	d->turnWobbleAngle = (s16)CTR_MipsSubLo((u16)d->turnWobbleAngle, CTR_MipsSra(d->turnWobbleAngle, 3));

	d->turnAngleCurr = (s16)CTR_MipsSubLo(CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnAngleCurr, (u16)d->KartStates.Spinning.driftSpinRate), 0x800) & 0xfff, 0x800);

	d->ampTurnState = d->rotationSpinRate;

	d->angle = (s16)(CTR_MipsAddLo((u16)d->angle, CTR_MipsSra(CTR_MipsMulLo(d->rotationSpinRate, elapsedTimeMS), 0xd)) & 0xfff);

	d->rotCurr.y = (s16)CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnWobbleAngle, (u16)d->angle), (u16)d->turnAngleCurr);

	d->rotCurr.w = VehCalc_InterpBySpeed(d->rotCurr.w, CTR_MipsSra(CTR_MipsSll(elapsedTimeMS, 5), 5), 0);

	VehPhysForce_RotAxisAngle(&d->matrixMovingDir, d->AxisAngle1_normalVec.v, d->angle);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063eac-0x80063ec0.
void VehPhysProc_SpinFirst_InitSetUpdate(struct Thread *t, struct Driver *d)
{
	d->funcPtrs[DRIVER_FUNC_INIT] = 0;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = VehPhysProc_SpinFirst_Update;
}


DriverFunc PlayerSpinningFuncTable[DRIVER_FUNC_COUNT] = {VehPhysProc_SpinFirst_InitSetUpdate,
                                                         0,
                                                         VehPhysProc_SpinFirst_PhysLinear,
                                                         VehPhysProc_Driving_Audio,
                                                         VehPhysProc_SpinFirst_PhysAngular,
                                                         VehPhysForce_OnApplyForces,
                                                         COLL_MOVED_PlayerSearch,
                                                         VehPhysForce_CollideDrivers,
                                                         COLL_FIXED_PlayerSearch,
                                                         VehPhysGeneral_JumpAndFriction,
                                                         VehPhysForce_TranslateMatrix,
                                                         VehFrameProc_Spinning,
                                                         VehEmitter_DriverMain};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063ec0-0x8006402c.
void VehPhysProc_SpinFirst_Init(struct Thread *t, struct Driver *d)
{
	int i;
	int feedback;

	d->kartState = KS_SPINNING;

	d->turnAngleLerpVel = 0;
	d->turbo_MeterRoomLeft = 0;

	if (LOAD_IsOpen_RacingOrBattle() && ((sdata->gGT->gameMode1 & ADVENTURE_ARENA) == 0))
	{
		RB_Player_ModifyWumpa(d, -1);
	}

	Voiceline_RequestPlay(3, data.characterIDs[d->driverID], 0x10);

	// if spinning left
	d->KartStates.Spinning.spinDir = 1;
	d->KartStates.Spinning.driftSpinRate = 300;

	if (d->ampTurnState < 0)
	{
		// if spinning right
		d->KartStates.Spinning.spinDir = -1;
		d->KartStates.Spinning.driftSpinRate = -300;
	}

	if ((s8)d->simpTurnState < 1)
	{
		feedback = 0x19;
	}

	else
	{
		feedback = 0x29;
	}

	for (i = 0; i < DRIVER_FUNC_COUNT; i++)
	{
		d->funcPtrs[i] = PlayerSpinningFuncTable[i];
	}

	GAMEPAD_JogCon1(d, feedback, 0x60);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006402c-0x8006406c.
void VehPhysProc_SpinLast_Update(struct Thread *t, struct Driver *d)
{
	int driftAngle = d->turnAngleCurr;
	if (driftAngle < 0)
	{
		driftAngle = CTR_MipsNegLo(driftAngle);
	}

	// if almost facing forward
	if (driftAngle < 16)
	{
		// stop spin
		VehPhysProc_SpinStop_Init(t, d);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006406c-0x800640a4.
void VehPhysProc_SpinLast_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;

	d->actionsFlagSet |= ACTION_WARP | ACTION_ACCEL_PREVENTION;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800640a4-0x80064254.
void VehPhysProc_SpinLast_PhysAngular(struct Thread *t, struct Driver *d)
{
	int elapsedTimeMS = sdata->gGT->elapsedTimeMS;
	int driftAngleCurr;
	driftAngleCurr = d->turnAngleCurr;

	d->numFramesSpentSteering = 10000;

	d->rotationSpinRate = (s16)CTR_MipsSubLo((u16)d->rotationSpinRate, CTR_MipsSra(d->rotationSpinRate, 3));
	d->turnWobbleAngle = (s16)CTR_MipsSubLo((u16)d->turnWobbleAngle, CTR_MipsSra(d->turnWobbleAngle, 3));

	d->ampTurnState = d->rotationSpinRate;

	if (driftAngleCurr < 0)
	{
		if ((d->KartStates.Spinning.driftSpinRate > 0) && (driftAngleCurr > -400))
		{
			d->KartStates.Spinning.driftSpinRate = (s16)CTR_MipsSra(CTR_MipsNegLo(CTR_MipsSll(driftAngleCurr, 2)), 3);

			if (d->KartStates.Spinning.driftSpinRate < 0x20)
				d->KartStates.Spinning.driftSpinRate = 0x20;
		}

		d->turnAngleCurr =
		    (s16)CTR_MipsSubLo(CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnAngleCurr, (u16)d->KartStates.Spinning.driftSpinRate), 0x800) & 0xfff, 0x800);

		if ((d->KartStates.Spinning.driftSpinRate > 0) && (d->turnAngleCurr > 0))
		{
			d->turnAngleCurr = 0;
		}
	}

	if (driftAngleCurr > 0)
	{
		if ((d->KartStates.Spinning.driftSpinRate < 0) && (driftAngleCurr < 400))
		{
			d->KartStates.Spinning.driftSpinRate = (s16)CTR_MipsSra(CTR_MipsNegLo(CTR_MipsSll(driftAngleCurr, 2)), 3);

			if (d->KartStates.Spinning.driftSpinRate > -0x20)
				d->KartStates.Spinning.driftSpinRate = -0x20;
		}

		d->turnAngleCurr =
		    (s16)CTR_MipsSubLo(CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnAngleCurr, (u16)d->KartStates.Spinning.driftSpinRate), 0x800) & 0xfff, 0x800);

		if ((d->KartStates.Spinning.driftSpinRate < 0) && (d->turnAngleCurr < 0))
		{
			d->turnAngleCurr = 0;
		}
	}

	d->angle = (s16)(CTR_MipsAddLo((u16)d->angle, CTR_MipsSra(CTR_MipsMulLo(d->ampTurnState, elapsedTimeMS), 0xd)) & 0xfff);

	d->rotCurr.y = (s16)CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnWobbleAngle, (u16)d->angle), (u16)d->turnAngleCurr);

	d->rotCurr.w = VehCalc_InterpBySpeed(d->rotCurr.w, CTR_MipsSra(CTR_MipsSll(elapsedTimeMS, 5), 5), 0);

	VehPhysForce_RotAxisAngle(&d->matrixMovingDir, d->AxisAngle1_normalVec.v, d->angle);
}


DriverFunc PlayerLastSpinFuncTable[DRIVER_FUNC_COUNT] = {0,
                                                         VehPhysProc_SpinLast_Update,
                                                         VehPhysProc_SpinLast_PhysLinear,
                                                         VehPhysProc_Driving_Audio,
                                                         VehPhysProc_SpinLast_PhysAngular,
                                                         VehPhysForce_OnApplyForces,
                                                         COLL_MOVED_PlayerSearch,
                                                         VehPhysForce_CollideDrivers,
                                                         COLL_FIXED_PlayerSearch,
                                                         VehPhysGeneral_JumpAndFriction,
                                                         VehPhysForce_TranslateMatrix,
                                                         VehFrameProc_LastSpin,
                                                         VehEmitter_DriverMain};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80064254-0x800642ec.
void VehPhysProc_SpinLast_Init(struct Thread *t, struct Driver *d)
{
	int i;

	for (i = 0; i < DRIVER_FUNC_COUNT; i++)
	{
		d->funcPtrs[i] = PlayerLastSpinFuncTable[i];
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800642ec-0x800642f4.
void VehPhysProc_SpinStop_Update(struct Thread *t, struct Driver *d)
{
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800642f4-0x80064320.
void VehPhysProc_SpinStop_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80064320-0x800643d4.
void VehPhysProc_SpinStop_PhysAngular(struct Thread *t, struct Driver *d)
{
	int elapsedTimeMS = sdata->gGT->elapsedTimeMS;

	d->angle = (s16)(CTR_MipsAddLo((u16)d->angle, CTR_MipsSra(CTR_MipsMulLo(d->ampTurnState, elapsedTimeMS), 0xd)) & 0xfff);
	d->rotCurr.y = (s16)CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnWobbleAngle, (u16)d->angle), (u16)d->turnAngleCurr);

	d->rotCurr.w = VehCalc_InterpBySpeed(d->rotCurr.w, CTR_MipsSra(CTR_MipsSll(elapsedTimeMS, 5), 5), 0);
	d->turnAngleCurr = VehCalc_InterpBySpeed(d->turnAngleCurr, CTR_MipsSra(CTR_MipsSll(elapsedTimeMS, 7), 5), 0);

	VehPhysForce_RotAxisAngle(&d->matrixMovingDir, d->AxisAngle1_normalVec.v, d->angle);
}


// only Animate is needed, see StopSpin_Init for details

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800643d4-0x800644d0.
void VehPhysProc_SpinStop_Animate(struct Thread *t, struct Driver *d)
{
	struct Instance *inst = t->inst;

	int numFrames = VehFrameInst_GetNumAnimFrames(inst, inst->animIndex);

	if (numFrames > 0)
	{
		// steer from left to right, to exaggerate the force when steering stops abruptly
		if (d->KartStates.Spinning.spinDir == -1)
		{
			inst->animFrame = (s16)CTR_MipsAddLo((u16)inst->animFrame, 5);

			if (inst->animFrame < numFrames)
				return;

			inst->animFrame = numFrames - 1;
			d->KartStates.Spinning.spinDir = 0;
			return;
		}

		// steer from right to left, to exaggerate the force when steering stops abruptly
		if (d->KartStates.Spinning.spinDir == 1)
		{
			inst->animFrame = (s16)CTR_MipsSubLo((u16)inst->animFrame, 5);

			if (inst->animFrame >= 0)
				return;

			inst->animFrame = 0;
			d->KartStates.Spinning.spinDir = 0;
			return;
		}

		int targetFrame = VehFrameInst_GetStartFrame(0, numFrames);
		inst->animFrame = VehCalc_InterpBySpeed(inst->animFrame, 2, targetFrame);

		if (inst->animFrame != targetFrame)
			return;
	}

	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_Driving_Init;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800644d0-0x80064568.
void VehPhysProc_SpinStop_Init(struct Thread *t, struct Driver *d)
{
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
