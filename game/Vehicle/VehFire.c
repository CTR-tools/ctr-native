#include "VehCommon.h"

enum
{
	VEH_FIRE_AUDIO_HIGH_THRESHOLD = 0x80,
	VEH_FIRE_AUDIO_MEDIUM_THRESHOLD = 0x40,
	VEH_FIRE_AUDIO_VOLUME_LOW = 0x80,
	VEH_FIRE_AUDIO_VOLUME_MEDIUM = 0xc0,
	VEH_FIRE_AUDIO_VOLUME_HIGH = 0xff,
	VEH_FIRE_AUDIO_DISTORT_LOW = 0x94,
	VEH_FIRE_AUDIO_DISTORT_HIGH = 0x6c,
	VEH_FIRE_VOICELINE_HIGH_BOOST_ID = 0x10,
	VEH_FIRE_VOICELINE_PRIORITY = 0x10,
	VEH_FIRE_AUDIO_SFX = 0xd,
	VEH_FIRE_AUDIO_COOLDOWN = 0xf0,

	VEH_FIRE_POWER_SLIDE_DISAPPEAR_FRAMES = 2,
	VEH_FIRE_NO_DISAPPEAR = -1,
	VEH_FIRE_VISIBILITY_COOLDOWN = 0x60,
	VEH_FIRE_SPEED_CAP_SHIFT = 8,
	VEH_FIRE_SIZE_SHIFT = 6,
	VEH_FIRE_SIZE_BASE = 5,
	VEH_FIRE_SIZE_MAX = 8,
	VEH_FIRE_CAMERA_SHAKE_FLAG = 0x80,
	VEH_FIRE_RUMBLE_CHANNEL = 8,
	VEH_FIRE_RUMBLE_FORCE = 0x7f,
};

enum
{
	VEH_FIRE_INITIAL_INSTANCE_FLAGS = DEPTH_FADE | DRAW_BILLBOARD | HIDE_MODEL,
};


void VehFire_Audio(struct Driver *driver, int speed_cap)
{
	register struct Driver *audioDriver CTR_PSX_REGISTER("$18") = driver;
	register u32 volume CTR_PSX_REGISTER("$17");
	register u32 distortion CTR_PSX_REGISTER("$16");
	register u32 volumeFlags CTR_PSX_REGISTER("$5");
	register u32 distortionFlags CTR_PSX_REGISTER("$4");
	register u32 packedAudio CTR_PSX_REGISTER("$2");
	register u32 audioFlags CTR_PSX_REGISTER("$6");

	// if turbo audio cooldown is not done
	if (audioDriver->VehFire_AudioCooldown != 0)
	{
		return;
	}

	volume = VEH_FIRE_AUDIO_VOLUME_LOW;
	if (speed_cap < VEH_FIRE_AUDIO_MEDIUM_THRESHOLD)
		goto LowSpeed;

	volume = VEH_FIRE_AUDIO_VOLUME_MEDIUM;
	if (speed_cap < VEH_FIRE_AUDIO_HIGH_THRESHOLD)
		goto MediumSpeed;

	volume = VEH_FIRE_AUDIO_VOLUME_HIGH;
	distortion = VEH_FIRE_AUDIO_DISTORT_HIGH;
	Voiceline_RequestPlayOwned(VEH_FIRE_VOICELINE_HIGH_BOOST_ID, GAME_CHARACTER_IDS[audioDriver->driverID], VEH_FIRE_VOICELINE_PRIORITY, audioDriver);
	CTR_PSX_KEEP_VALUE(volume);
	volumeFlags = volume << HOWL_SFX_VOLUME_SHIFT;
	goto AudioFlags;

MediumSpeed:
	distortion = HOWL_SFX_DISTORTION_NONE;
	goto PackVolume;

LowSpeed:
	distortion = VEH_FIRE_AUDIO_DISTORT_LOW;

PackVolume:
	volumeFlags = volume << HOWL_SFX_VOLUME_SHIFT;

AudioFlags:
	distortionFlags = distortion << HOWL_SFX_DISTORTION_SHIFT;
	if ((audioDriver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
	{
		packedAudio = distortionFlags | HOWL_SFX_ECHO_FLAG;
		packedAudio = volumeFlags | packedAudio;
		goto PlayAudio;
	}
	packedAudio = volumeFlags | distortionFlags;

PlayAudio:
	audioFlags = packedAudio | HOWL_SFX_LR_CENTER;
	CTR_PSX_KEEP_VALUE(audioFlags);
	OtherFX_Play_LowLevel(VEH_FIRE_AUDIO_SFX, 1, audioFlags);

	// turbo audio cooldown 0.24s
	audioDriver->VehFire_AudioCooldown = VEH_FIRE_AUDIO_COOLDOWN;
}


void VehFire_SetReserves(struct Driver *driver, int reserves)
{
	driver->reserves = (s16)Chaos_ReservesForDriver(driver, reserves);
}

void VehFire_SetLevel(struct Driver *driver, struct Turbo *turbo, int fireLevel)
{
	const int speedCap = CTR_MipsAddLo(
	    CTR_MipsSra(CTR_MipsMulLo(fireLevel, CTR_MipsSubLo(driver->const_SacredFireSpeed, driver->const_SingleTurboSpeed)), VEH_FIRE_SPEED_CAP_SHIFT),
	    driver->const_SingleTurboSpeed);
	if (driver->reserves == 0 || driver->fireSpeedCap < speedCap ||
	    (driver->fireSpeedCap > driver->const_SacredFireSpeed &&
	     (driver->stepFlagSet & COLL_STEP_TRIGGER_SUPER_TURBO_PAD) == 0))
	{
		driver->fireSpeedCap = (s16)speedCap;
		if (turbo != NULL)
		{
			turbo->fireSize = (s16)CTR_MipsAddLo(CTR_MipsSra(fireLevel, VEH_FIRE_SIZE_SHIFT), VEH_FIRE_SIZE_BASE);
			if (turbo->fireSize > VEH_FIRE_SIZE_MAX)
			{
				turbo->fireSize = VEH_FIRE_SIZE_MAX;
			}
		}
	}
}

// param1 - driver
// param2 - reserves to add
// param3 - add type
// param4 - fire level
void VehFire_Increment(struct Driver *driverArg, int reservesArg, u32 typeArg, int fireLevelArg)
{
	register struct Driver *driver CTR_PSX_REGISTER("$18") = driverArg;
	register int reserves CTR_PSX_REGISTER("$20") = reservesArg;
	register u32 type CTR_PSX_REGISTER("$19") = typeArg;
	register int fireLevel CTR_PSX_REGISTER("$21") = Chaos_FireLevelForDriver(driverArg, fireLevelArg);
	u8 kartState;
	s8 count;

	int oldOTT;
	int reserveDelta;

	u32 addFlags;
	register struct Turbo *turboObj CTR_PSX_REGISTER("$16");
	struct Thread *turboThread;
	register struct Instance *turboInst1 CTR_PSX_REGISTER("$17");

	if (
	    // if this is a turbo pad
	    ((type & TURBO_PAD) != 0) &&

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
		GhostTape_WriteBoosts(reserves, type, fireLevel);
	}

	kartState = driver->kartState;

	if (kartState == KS_SPINNING)
	{
		return;
	}
	if (kartState == KS_BLASTED)
	{
		return;
	}
	if (kartState == KS_MASK_GRABBED)
	{
		return;
	}

	// Clear the turbo input latch and mark an outside turbo.
	driver->actionsFlagSet = (driver->actionsFlagSet & ~ACTION_TURBO_INPUT_LATCH) | ACTION_NEW_BOOST;

	// turbo thread bucket
	turboThread = GAME_TRACKER->threadBuckets[TURBO].thread;

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

	// if turbo exists, modify it
	if (turboThread != 0)
	{
		// get the turbo's object
		turboObj = turboThread->object;

		// get the turbo's instances
		turboInst1 = turboThread->inst;

		// remove "dead thread" flag
		turboThread->flags &= ~THREAD_FLAG_DEAD;

		// all other boosts
		if ((type & TURBO_PAD) == 0)
		{
			// make fire invisible for the sake of the visibility cooldown as explained in common.h
			turboInst1->flags |= DEPTH_FADE | HIDE_MODEL;
			turboObj->inst->flags |= DEPTH_FADE | HIDE_MODEL;

			turboObj->fireVisibilityCooldown = VEH_FIRE_VISIBILITY_COOLDOWN;
			driver->numTurbos = (s16)CTR_MipsAddLo((u16)driver->numTurbos, 1);
		}

		// turbo pad
		else
		{
			// only increase counter on the first frame of turbo pad

			if ((driver->actionsFlagSetPrevFrame & ACTION_NEW_BOOST) == 0)
			{
				driver->numTurbos = (s16)CTR_MipsAddLo((u16)driver->numTurbos, 1);
			}
		}

		turboObj->fireDisappearCountdown = VEH_FIRE_NO_DISAPPEAR;
		turboInst1->alphaScale = 0;
		turboObj->inst->alphaScale = 0;

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

	// if no turbo exists, create one
	else
	{
		driver->numTurbos = 1;

		turboInst1 = INSTANCE_BirthWithThread(STATIC_TURBO_EFFECT, VEH_TURBO1_NAME, SMALL, TURBO, VehTurbo_ThTick, sizeof(struct Turbo), 0);

		turboObj = 0;

		if (turboInst1 != 0)
		{
			// get object, set essentials
			turboObj = turboInst1->thread->object;
			turboObj->driver = driver;
			turboObj->fireVisibilityCooldown = 0;

			// Ignore collisions between the turbo thread and the world.
			turboInst1->thread->flags |= THREAD_FLAG_DISABLE_COLLISION;

			// player of any kind
			if (driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
			{
				turboObj->fireAudioDistort = 0;

				if (driver->kartState != KS_CRASHING)
				{
					VehFire_Audio(driver, fireLevel);
				}
			}

			turboInst1->thread->funcThDestroy = VehTurbo_ThDestroy;

			// turbo #2
			turboObj->inst = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_TURBO_EFFECT], // model
			                                  VEH_TURBO2_NAME,                                 // name
			                                  turboInst1->thread                               // parent thread
			);

			// 2P 3P 4P flags
			addFlags = DEPTH_FADE | DRAW_BILLBOARD;

			// 1P flags
			if (GAME_TRACKER->numPlyrCurrGame == 1)
			{
				addFlags |= VISIBLE_DURING_GAMEPLAY;
			}

			// Initial fire instances are billboarded but hidden until the turbo tick reveals them.
			addFlags |= HIDE_MODEL;
			turboInst1->flags |= addFlags;
			turboObj->inst->flags |= addFlags;

			// Powerslide flames disappear quickly; other flames remain until their thread is reused.
			turboObj->fireAnimIndex = 0;
			if ((type & POWER_SLIDE_HANG_TIME) != 0)
			{
				CTR_PSX_MEMORY_BARRIER();
				count = VEH_FIRE_POWER_SLIDE_DISAPPEAR_FRAMES;
			}
			else
			{
				CTR_PSX_MEMORY_BARRIER();
				count = VEH_FIRE_NO_DISAPPEAR;
			}
			turboObj->fireDisappearCountdown = count;
		}
	}

	VehFire_SetLevel(driver, turboObj, fireLevel);

	// turbo item boost
	if ((type & TURBO_ITEM) != 0)
	{
		driver->actionsFlagSet |= ACTION_TURBO_ITEM;
	}

 	// turbo pad, boost powerup
	if ((type & FREEZE_RESERVES_ON_TURBO_PAD) != 0)
	{
		// this adds reserves on the first frame you touch the turbo pad,
		// then prevent reserves from decreasing until the first frame
		// you leave the turbo pad

		oldOTT = driver->turbo_outsideTimer;
		CTR_PSX_OBSERVE_VALUE(oldOTT);
		reserveDelta = CTR_MipsSubLo(reserves, (u16)driver->turbo_outsideTimer);

		if (oldOTT < reserves)
		{
			driver->reserves = (s16)CTR_MipsAddLo((u16)driver->reserves, reserveDelta);
			driver->turbo_outsideTimer = (s16)reserves;
		}
	}

	// startline, hang time, powerslide
	else if ((type & SUPER_ENGINE) == 0)
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
		GAME_TRACKER->cameraDC[driver->driverID].flags |= VEH_FIRE_CAMERA_SHAKE_FLAG;

		// gamepad vibration
		GAMEPAD_ShockForce1(driver, VEH_FIRE_RUMBLE_CHANNEL, VEH_FIRE_RUMBLE_FORCE);
	}

	CTR_PSX_OBSERVE_VALUE(reserves);
	CTR_PSX_OBSERVE_VALUE(type);
	CTR_PSX_OBSERVE_VALUE(fireLevel);
	// #endif
}
