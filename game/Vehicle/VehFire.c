#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005ab24-0x8005abfc.
void VehFire_Audio(struct Driver *driver, int speed_cap)
{
	u32 distortion;
	u32 volume;
	u32 echo;

	// if turbo audio cooldown is not done
	if (driver->VehFire_AudioCooldown != 0)
	{
		return;
	}

	if (speed_cap >= 0x80)
	{
		// max volume
		volume = 0xff;

		// distort
		distortion = 0x6c;

		Voiceline_RequestPlay(0x10, data.characterIDs[driver->driverID], 0x10);

		goto Skip;
	}

	if (speed_cap >= 0x40)
	{
		// 3/4 volume
		volume = 0xc0;

		// no distort
		distortion = HOWL_SFX_DISTORTION_NONE;

		goto Skip;
	}

	// half volume
	volume = 0x80;

	// distort
	distortion = 0x94;

Skip:

	echo = 0;

	// if echo is required
	if ((driver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
	{
		echo = 1;
	}

	// 0xD = Turbo Boost Sound
	// 0x80 = balance L/R
	OtherFX_Play_LowLevel(0xd, 1, HowlSfx_Pack(HOWL_SFX_LR_CENTER, distortion, volume, echo));

	// turbo audio cooldown 0.24s
	driver->VehFire_AudioCooldown = 0xf0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005abfc-0x8005b0c4.

// param1 - driver
// param2 - reserves to add
// param3 - add type
// param4 - fire level
void VehFire_Increment(struct Driver *driver, int reserves, u32 type, int fireLevel)
{
	u8 kartState;
	s8 count;

	int newFireSpeedCap;
	int newFireSize;
	int oldOTT;

	u32 addFlags;
	struct Turbo *turboObj;
	struct Thread *turboThread;
	struct Instance *turboInst1;
	struct Instance *turboInst2;

	struct GameTracker *gGT = sdata->gGT;
	if (
	    // if this is a turbo pad
	    ((type & 4) != 0) &&

	    // racer is in accel prevention (holding square)
	    ((driver->actionsFlagSet & ACTION_ACCEL_PREVENTION) != 0))
	{
		// do nothing
		return;
	}

	if (
	    // Player / AI structure + 0x4a shows driver index (0-7)

	    // If this is the first driver (P1) and
	    (driver->driverID == '\0') &&

	    // player of any kind
	    (driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER))
	{
		// Add Reserves to ghost buffer
		GhostTape_WriteBoosts(reserves, (u8)type, fireLevel);
	}

	kartState = driver->kartState;

	if (kartState == KS_SPINNING)
	{
		return;
	}
	if (kartState == KS_MASK_GRABBED)
	{
		return;
	}
	if (kartState == KS_BLASTED)
	{
		return;
	}

	// Clear the turbo input latch and mark an outside turbo.
	driver->actionsFlagSet = (driver->actionsFlagSet & ~ACTION_TURBO_INPUT_LATCH) | ACTION_NEW_BOOST;

	// turbo thread bucket
	turboThread = gGT->threadBuckets[TURBO].thread;

	// check all turbo threads
	while (turboThread != 0)
	{
		// if this turbo thread is owned by this driver
		if (((struct Turbo *)turboThread->object)->driver == driver)
		{
			// quit, turboThread is now this driver's turbo thread
			break;
		}

		// next turbo thread in bucket
		turboThread = turboThread->siblingThread;
	}

	// if no turbo exists, create one
	if (turboThread == 0)
	{
#if BUILD < JpnRetail

		driver->numTurbos = 1;

#else

		// Japan retail gates this through the extra turbo state byte.
		if (driver->japanTurboUnknown == 0)
		{
			driver->numTurbos = 1;
			if ((driver->numTurbosHighScore < 1) && ((gGT->gameMode1 & END_OF_RACE) == 0))
			{
				driver->numTurbosHighScore = 1;
			}
		}
		else
		{
			driver->numTurbos = (s16)CTR_MipsAddLo((u16)driver->numTurbos, 1);
			if ((driver->numTurbosHighScore < driver->numTurbos) && ((gGT->gameMode1 & END_OF_RACE) == 0))
			{
				driver->numTurbosHighScore = driver->numTurbos;
			}
		}

#endif

#if defined(CTR_NATIVE)
		turboInst1 = INSTANCE_BirthWithThread(0x2c, 0, SMALL, TURBO, VehTurbo_ThTick, sizeof(struct Turbo), 0);

		turboObj = 0;

		if (turboInst1 != 0)
		{
			// get thread, ignore all collisions
			turboThread = turboInst1->thread;
			turboThread->flags |= THREAD_FLAG_DISABLE_COLLISION;

			// get object, set essentials
			turboObj = turboThread->object;
			turboObj->driver = driver;
			turboObj->fireVisibilityCooldown = 0;

			// make flame disappear after
			// 	- powerslide: two frames (quick death)
			//	- all others: -1 frames (255 = 'no' death)
			if (type & 2)
			{
				count = 2;
			}
			else
			{
				count = -1;
			}
			turboObj->fireDisappearCountdown = count;

			// player of any kind
			if (driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
			{
				turboObj->fireAudioDistort = 0;

				if (driver->kartState != KS_CRASHING)
				{
					VehFire_Audio(driver, fireLevel);
				}
			}

			turboThread->funcThDestroy = VehTurbo_ThDestroy;

			// turbo #2
			turboInst2 = INSTANCE_Birth3D(gGT->modelPtr[STATIC_TURBO_EFFECT], // model
			                              &sdata->s_turbo2[0],                // name
			                              turboThread                         // parent thread
			);

			// 2P 3P 4P flags
			addFlags = 0;

			turboObj->inst = turboInst2;
			turboObj->fireAnimIndex = 0;

			// 1P flags
			if (gGT->numPlyrCurrGame == 1)
			{
				addFlags = VISIBLE_DURING_GAMEPLAY;
			}

			// Make turbos invisible and transparent.
			turboInst1->flags = turboInst1->flags | addFlags | 0x1040080;
			turboInst2->flags = turboInst2->flags | addFlags | 0x1040080;
		}
#else
		turboObj = 0;
		turboInst1 = 0;
#endif
	}

	// if turbo exists, modify it
	else
	{
		// get the turbo's object
		turboObj = turboThread->object;

		// get the turbo's instances
		turboInst1 = turboThread->inst;
		turboInst2 = turboObj->inst;

		// remove "dead thread" flag
		turboThread->flags &= ~THREAD_FLAG_DEAD;

		// turbo pad
		if ((type & 4) != 0)
		{
			// only increase counter on the first frame of turbo pad

			if ((driver->actionsFlagSetPrevFrame & ACTION_NEW_BOOST) == 0)
			{
				driver->numTurbos = (s16)CTR_MipsAddLo((u16)driver->numTurbos, 1);

#if BUILD == JpnRetail
				// the japanese version of the game keeps track of your highest turbo chain in a race
				if (driver->numTurbosHighScore < driver->numTurbos && (gGT->gameMode1 & END_OF_RACE) == 0)
					driver->numTurbosHighScore = driver->numTurbos;
#endif
			}
		}

		// all other boosts
		else
		{
			// make fire invisible for the sake of the visibility cooldown as explained in common.h
			turboInst1->flags |= DEPTH_FADE | HIDE_MODEL;
			turboInst2->flags |= DEPTH_FADE | HIDE_MODEL;

			turboObj->fireVisibilityCooldown = 0x60;
			driver->numTurbos = (s16)CTR_MipsAddLo((u16)driver->numTurbos, 1);
#if BUILD == JpnRetail
			// the japanese version of the game keeps track of your highest turbo chain in a race
			if (driver->numTurbosHighScore < driver->numTurbos && (gGT->gameMode1 & END_OF_RACE) == 0)
				driver->numTurbosHighScore = driver->numTurbos;
#endif
		}

		turboObj->fireDisappearCountdown = -1;
		turboInst1->alphaScale = 0;
		turboInst2->alphaScale = 0;

		// player of any kind
		if (driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
		{
			if (
			    // if racer is not getting an Outside turbo (turbo pad or powerup),
			    // this prevents audio-spam from multiple boosts
			    ((driver->actionsFlagSet & ACTION_NEW_BOOST) == 0) || ((driver->actionsFlagSetPrevFrame & ACTION_NEW_BOOST) == 0))

			{
				turboObj->fireAudioDistort = 0;
				VehFire_Audio(driver, fireLevel);
			}
		}
	}

	newFireSpeedCap = CTR_MipsAddLo(CTR_MipsSra(CTR_MipsMulLo(fireLevel, CTR_MipsSubLo(driver->const_SacredFireSpeed, driver->const_SingleTurboSpeed)), 8),
	                                driver->const_SingleTurboSpeed);

	if (
	    // any gain in boost,
	    // resize to gain boost
	    (
	        // Reserves are equal to zero
	        // OR
	        // speed cap has been raised
	        (driver->reserves == 0) || (driver->fireSpeedCap < newFireSpeedCap)) ||

	    // OR

	    // you have USF, and boosted on a non-STP,
	    // resize fire to lose size
	    (
	        // Current speed cap is greater than 0x1000
	        // AND
	        // You are not on a super turbo pad
	        (int)driver->const_SacredFireSpeed < (int)driver->fireSpeedCap && ((driver->stepFlagSet & COLL_STEP_TRIGGER_SUPER_TURBO_PAD) == 0)))

	{
		driver->fireSpeedCap = (s16)newFireSpeedCap;

		if (turboObj != 0)
		{
			// modify, cap, and save the size of the fire
			newFireSize = CTR_MipsAddLo(CTR_MipsSra(fireLevel, 6), 5);
			if (newFireSize > 8)
			{
				newFireSize = 8;
			}
			turboObj->fireSize = (s16)newFireSize;
		}
	}

	// turbo item boost
	if ((type & TURBO_ITEM) != 0)
	{
		driver->actionsFlagSet |= ACTION_TURBO_ITEM;
	}

	// turbo pad, boost powerup
	if (type & 1)
	{
		// this adds reserves on the first frame you touch the turbo pad,
		// then prevent reserves from decreasing until the first frame
		// you leave the turbo pad

		oldOTT = driver->turbo_outsideTimer;

		if (oldOTT < reserves)
		{
			int reserveDelta = CTR_MipsSubLo(reserves, (u16)driver->turbo_outsideTimer);
			driver->reserves = (s16)CTR_MipsAddLo((u16)driver->reserves, reserveDelta);
			driver->turbo_outsideTimer = (s16)reserves;
		}
	}

	// startline, hang time, powerslide
	else if ((type & 0x10) == 0)
	{
		// increase reserves BY param2
		driver->reserves = (s16)CTR_MipsAddLo((u16)driver->reserves, reserves);
	}

	// super engine
	else
	{
		// set reserves to reserves parameter instead of incrementing
		if (driver->reserves < reserves)
		{
			driver->reserves = (s16)reserves;
		}
	}

	// player of any kind
	if (driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
	{
		// CameraDC flag
		gGT->cameraDC[driver->driverID].flags |= 0x80;

		// gamepad vibration
		GAMEPAD_ShockForce1(driver, 8, 0x7f);
	}
	// #endif
}
