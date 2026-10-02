#include <common.h>

#ifndef GAME_ENGINE_FX_METADATA
#define GAME_ENGINE_FX_METADATA       (sdata->howl_metaEngineFX)
#define GAME_ENGINE_VOLUME_FX         (sdata->vol_FX)
#define GAME_ENGINE_DISTORTIONS       (data.distortConst_Engine)
#define GAME_ENGINE_AUDIO_DEFAULTS    (sdata->audioDefaults)
#define GAME_ENGINE_DISTANCE_DEFAULTS (&sdata->audioDefaults[2])
#define GAME_ENGINE_PAN_DEFAULTS      (&sdata->audioDefaults[4])
#endif

// Initialize car engine audio system for one driver
b32 EngineAudio_InitOnce(u32 soundID, u32 flags)
{
	struct EngineFX *ptrEngineFX;
	struct ChannelStats *channel;
	u32 id;
	u32 distortion;
	u32 volume;
	u32 echo;
	u32 LR;
	struct ChannelAttr channelAttr;
	if (GAME_AUDIO_ENABLED == 0)
	{
		return 0;
	}
	id = soundID & 0xffff;
	if (id >= (u32)GAME_HOWL_HEADER->numEngineFX)
	{
		return 0;
	}
	ptrEngineFX = &GAME_ENGINE_FX_METADATA[id];
	if (GAME_HOWL_SPU_ADDRS[ptrEngineFX->spuIndex].spuAddr == 0)
	{
		return 0;
	}
	distortion = HowlSfx_Distortion(flags);
	echo = HowlSfx_Echo(flags);
	volume = HowlSfx_Volume(flags);
	LR = HowlSfx_LR(flags);
	howl_InitChannelAttr_EngineFX(ptrEngineFX, &channelAttr, volume, LR, distortion);
	channelAttr.reverb = echo;
	Smart_EnterCriticalSection();
	channel = Channel_AllocSlot(HOWL_CHANNEL_UPDATE_ALL_ATTRS, &channelAttr);
	if (channel != 0)
	{
		channel->type = HOWL_CHANNEL_TYPE_ENGINE_FX;
		channel->unk2 = 0;
		channel->echo = echo;
		channel->vol = volume;
		channel->distort = distortion;
		channel->LR = LR;
		channel->timeLeft = 0;
		channel->soundID = id;
		channel->flags |= 4;
	}
	Smart_ExitCriticalSection();
	return channel != 0;
}

s16 EngineAudio_Recalculate(u32 soundID, u32 sfx)
{
	u32 volume;
	u32 echo;
	u32 distortion;
	u32 LR;
	u32 id;
	struct EngineFX *ptrEngineFX;
	struct ChannelStats *channel;
	struct ChannelAttr channelAttr;
	struct GameTracker *gGT;
	if (GAME_AUDIO_ENABLED == 0)
	{
		return 0;
	}
	id = soundID & 0xffff;
	if ((u32)GAME_HOWL_HEADER->numEngineFX <= id)
	{
		return 0;
	}
	volume = HowlSfx_Volume(sfx);
	ptrEngineFX = &GAME_ENGINE_FX_METADATA[id];
	echo = HowlSfx_Echo(sfx);
	distortion = HowlSfx_Distortion(sfx);
	gGT = GAME_TRACKER;
	LR = HowlSfx_LR(sfx);
	if (gGT->numPlyrCurrGame == 2)
	{
		volume = (volume * 0x37 << 2) >> 8;
	}
	else if (gGT->numPlyrCurrGame >= 3)
	{
		volume = (volume * 0x2d << 2) >> 8;
	}
	Channel_SetVolume(&channelAttr, GAME_ENGINE_VOLUME_FX * ptrEngineFX->volume * volume >> 10, LR);
	if (distortion != HOWL_SFX_DISTORTION_NONE)
	{
		channelAttr.pitch = (u32)ptrEngineFX->pitch * (u32)GAME_ENGINE_DISTORTIONS[distortion] >> 0x10;
	}
	else
	{
		channelAttr.pitch = ptrEngineFX->pitch;
	}
	channelAttr.reverb = echo;
	Smart_EnterCriticalSection();
	channel = Channel_SearchFX_EditAttr(HOWL_CHANNEL_TYPE_ENGINE_FX, soundID & 0xffff, HOWL_CHANNEL_UPDATE_DYNAMIC_ATTRS, &channelAttr);
	if (channel != 0)
	{
		channel->echo = echo;
		channel->vol = volume;
		channel->distort = distortion;
		channel->LR = LR;
	}
	Smart_ExitCriticalSection();
	return 1;
}

void EngineSound_Player(struct Driver *driver)
{
	u32 soundID = (u32)GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[driver->driverID]].engineID * 4u + driver->driverID;
	u32 volume;
	// NOTE(aalhendi): Retail keeps mapped distortion in a1 while forming signed pan.
	register u32 distortion CTR_PSX_REGISTER("$5");
	s32 lr;
	// NOTE(aalhendi): Retail combines packed fields in v0 before the shared call.
	register u32 flags CTR_PSX_REGISTER("$2");
	// NOTE(aalhendi): The packing scopes keep shifted distortion in a2.
	if (driver->engineSoundMode == ENGINE_SOUND_FADE_OUT)
	{
		driver->engineSoundVolumeState = (driver->engineSoundVolumeState * 0x177) >> 9;
		driver->engineSoundPitchState = (driver->engineSoundPitchState * 3000 + 0x22400) >> 0xc;
		volume = VehCalc_MapToRange(driver->engineSoundVolumeState, 0, driver->const_AccelSpeed_ClassStat, 0, 0xe6);
		distortion = VehCalc_MapToRange(driver->engineSoundPitchState, 0, driver->const_AccelSpeed_ClassStat, 0x3c, 200);

		{
			u32 wheel;
			wheel = (u16)driver->wheelRotation;
			lr = 0x80;
			lr -= (s32)(wheel << 0x10) >> 0x13;
		}
		if (lr < 0x40)
		{
			lr = 0x40;
		}
		else if (lr > 0xc0)
		{
			lr = 0xc0;
		}
		{
			u32 mask = volume & 0xff;
			u32 volumeBits;
			register u32 distortionBits CTR_PSX_REGISTER("$6");
			u32 panBits;
			u32 idArg;
			volumeBits = mask << 16;
			mask = distortion & 0xff;
			distortionBits = mask << 8;
			panBits = (u32)lr & 0xff;
			idArg = soundID & 0xffff;
			if ((driver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
			{
				flags = distortionBits | 0x1000000;
				// NOTE(aalhendi): Keep this fade branch's pan and packing path before the shared call.
				CTR_PSX_KEEP_VALUE_RELAXED(flags);
				flags = volumeBits | flags;
			}
			else
			{
				flags = volumeBits | distortionBits;
			}
			EngineAudio_Recalculate(idArg, flags | panBits);
		}
		return;
	}
	if (driver->engineSoundMode == ENGINE_SOUND_FADE_IN)
	{
		driver->engineSoundVolumeState = (driver->engineSoundVolumeState * 3000 + 0x322bc0) >> 0xc;
		driver->engineSoundPitchState = (driver->engineSoundPitchState * 3000 + 0x22400) >> 0xc;
		volume = VehCalc_MapToRange(driver->engineSoundVolumeState, 0, driver->const_AccelSpeed_ClassStat, 0x82, 0xe6);
		distortion = VehCalc_MapToRange(driver->engineSoundPitchState, 0, driver->const_AccelSpeed_ClassStat, 0x3c, 200);

		{
			u32 wheel;
			wheel = (u16)driver->wheelRotation;
			lr = 0x80;
			lr -= (s32)(wheel << 0x10) >> 0x13;
		}
		if (lr < 0x40)
		{
			lr = 0x40;
		}
		else if (lr > 0xc0)
		{
			lr = 0xc0;
		}
		{
			u32 mask = volume & 0xff;
			u32 volumeBits;
			register u32 distortionBits CTR_PSX_REGISTER("$6");
			u32 panBits;
			u32 idArg;
			volumeBits = mask << 16;
			mask = distortion & 0xff;
			distortionBits = mask << 8;
			panBits = (u32)lr & 0xff;
			idArg = soundID & 0xffff;
			if ((driver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
			{
				flags = distortionBits | 0x1000000;
				// NOTE(aalhendi): Keep this fade branch's pan and packing path before the shared call.
				CTR_PSX_KEEP_VALUE_RELAXED(flags);
				flags = volumeBits | flags;
			}
			else
			{
				flags = volumeBits | distortionBits;
			}
			EngineAudio_Recalculate(idArg, flags | panBits);
		}
		return;
	}
	{
		int targetPitch;
		u32 kartState = driver->kartState;
		int distortionValue;
		int pitchDelta;
		int steer;
		int enginePitch;
		// NOTE(aalhendi): The fresh flags load follows the pitch store in v0.
		register u32 currentActions CTR_PSX_REGISTER("$2");

		if (kartState == KS_ENGINE_REVVING)
		{
			targetPitch = 0;
			if (0 < driver->fireSpeed)
			{
				targetPitch = 0x3000;
			}

			targetPitch = (driver->engineSoundPitchState * 0x40 + targetPitch * 0x30 + driver->speedometerNeedleValue * 0x90) >> 8;
			if (0 < driver->fireSpeed)
			{
				targetPitch += 0x1000;
			}
		}
		else
		{
			// NOTE(aalhendi): Preserve the signed source in v1 while copying its magnitude.
			register int fire CTR_PSX_REGISTER("$3") = driver->fireSpeed;
			u32 previousActions = driver->actionsFlagSetPrevFrame;
			targetPitch = fire;
			if (fire < 0)
			{
				targetPitch = -targetPitch;
			}

			if (((previousActions & ACTION_TOUCH_GROUND) != 0) && (kartState != KS_DRIFTING))
			{
				int speed = driver->speedApprox;
				if (speed < 0)
				{
					speed = -speed;
				}
				targetPitch = (targetPitch + speed) >> 1;
			}
			else
			{
				targetPitch += 0xf00;
			}
		}

		pitchDelta = targetPitch - driver->engineSoundPitchState;
		if (pitchDelta < 0)
		{
			pitchDelta = -pitchDelta;
		}

		if (pitchDelta >= 0x601)
		{
			s16 cooldown = driver->engineSoundVolumeState + 2000;
			driver->engineSoundVolumeState = cooldown;
			if (14000 < cooldown)
			{
				driver->engineSoundVolumeState = 14000;
			}
		}
		else
		{
			u16 cooldown = driver->engineSoundVolumeState - 500;
			driver->engineSoundVolumeState = cooldown;

			if (driver->kartState == KS_DRIFTING)
			{
				if ((s16)cooldown < 2000)
				{
					driver->engineSoundVolumeState = 2000;
				}
			}
			else if ((s16)cooldown < 0)
			{
				driver->engineSoundVolumeState = 0;
			}
		}

		{
			int oldPitchProduct = driver->engineSoundPitchState * 0x177;
			u32 weightedPitch;
			int rawSteer;
			weightedPitch = (u32)(targetPitch * 0x89 + oldPitchProduct);
			// NOTE(aalhendi): Retail reads steering after the weighted sum and before narrowing pitch.
			CTR_PSX_DEPEND_MEMORY(&driver->wheelRotation, weightedPitch);
			rawSteer = driver->wheelRotation;
			driver->engineSoundPitchState = (s16)(weightedPitch >> 9);
			// NOTE(aalhendi): Store the narrowed pitch before loading current action flags.
			CTR_PSX_MEMORY_BARRIER();
			currentActions = driver->actionsFlagSet;
			if (rawSteer < 0)
			{
				rawSteer = -rawSteer;
			}
			// NOTE(aalhendi): Retail finishes the steering magnitude before its shift.
			CTR_PSX_KEEP_VALUE_RELAXED(rawSteer);
			steer = rawSteer >> 3;
		}

		if ((currentActions & ACTION_BOT) != 0)
		{
			volume = VehCalc_MapToRange(driver->engineSoundVolumeState, 0, driver->const_AccelSpeed_ClassStat, 0x82, 0xbe);
		}
		else
		{
			volume = VehCalc_MapToRange(driver->engineSoundVolumeState, 0, driver->const_AccelSpeed_ClassStat, 0x82, 0xe6);
		}

		if ((driver->kartState != KS_DRIFTING) && ((driver->actionsFlagSet & ACTION_ACCEL_PREVENTION) == 0))
		{
			volume += steer;
		}

		if ((driver->actionsFlagSet & ACTION_BOT) != 0)
		{
			enginePitch =
			    VehCalc_MapToRange(driver->engineSoundPitchState, 0, driver->const_AccelSpeed_ClassStat + driver->const_SacredFireSpeed + 0xf00, 0x3c, 0xbe);
		}
		else
		{
			enginePitch =
			    VehCalc_MapToRange(driver->engineSoundPitchState, 0, driver->const_AccelSpeed_ClassStat + driver->const_SacredFireSpeed + 0xf00, 0x3c, 200);
		}

		if ((driver->actionsFlagSet & ACTION_BOT) != 0)
		{
			int drift = (s32)((u32)(u16)driver->turnWobbleAngle << 0x10) >> 0x13;
			if (drift < 0)
			{
				drift = -drift;
			}
			distortionValue = enginePitch - drift;
			if (distortionValue < 0)
			{
				distortionValue = 0;
			}
		}
		else
		{
			if (driver->kartState == KS_DRIFTING)
			{
				int drift;
				if (driver->turbo_MeterRoomLeft != 0)
				{
					driver->sfxDistortOffset = ((u8)driver->const_turboMaxRoom >> 1) - (driver->turbo_MeterRoomLeft >> 6);
				}
				else if (driver->sfxDistortOffset != 0)
				{
					driver->sfxDistortOffset--;
				}

				drift = (s32)((u32)(u16)driver->turnWobbleAngle << 0x10) >> 0x13;
				if (drift < 0)
				{
					drift = -drift;
				}
				enginePitch -= drift;
				if (enginePitch < 0)
				{
					enginePitch = 0;
				}
			}

			distortionValue = enginePitch + driver->sfxDistortOffset;
		}

		if (0xff < distortionValue)
		{
			distortionValue = 0xff;
		}
		distortion = (u32)distortionValue;
	}
	{
		u32 wheel;
		wheel = (u16)driver->wheelRotation;
		lr = 0x80;
		lr -= (s32)(wheel << 0x10) >> 0x13;
	}
	if (lr < 0x40)
	{
		lr = 0x40;
	}
	else if (lr > 0xc0)
	{
		lr = 0xc0;
	}
	{
		u32 mask = volume & 0xff;
		u32 volumeBits;
		register u32 distortionBits CTR_PSX_REGISTER("$6");
		u32 panBits;
		u32 idArg;
		volumeBits = mask << 16;
		mask = distortion & 0xff;
		distortionBits = mask << 8;
		panBits = (u32)lr & 0xff;
		idArg = soundID & 0xffff;
		if ((driver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
		{
			flags = distortionBits | 0x1000000;
			flags = volumeBits | flags;
		}
		else
		{
			flags = volumeBits | distortionBits;
		}
		EngineAudio_Recalculate(idArg, flags | panBits);
	}
}

int EngineSound_VolumeAdjust(int desired, int current, int step)
{
	int delta = desired - current;
	int absDelta = delta;

	if (delta < 0)
	{
		// NOTE(aalhendi): Retail negates the copied magnitude in v0, leaving delta in v1 for the direction check.
		CTR_PSX_NEGATE_IN_PLACE(absDelta);
	}

	if (step < absDelta)
	{
		if (delta > 0)
		{
			return current + step;
		}

		return current - step;
	}

	return desired;
}

void EngineSound_AI(struct Driver *ai, struct Driver *cameraDriver, int slotIndex, int distanceArg, int distanceDelta, u32 lr)
{
	u32 volume;
	s32 distortion;
	// NOTE(aalhendi): Retail combines the packed sound flags in v0.
	register u32 packedFlags CTR_PSX_REGISTER("$2");
	// NOTE(aalhendi): Keep the first map result in a3 through the distance checks.
	register u32 baseVolume CTR_PSX_REGISTER("$7");
	s32 pan = (s32)lr;
	s32 doppler;
	u32 soundID;
	u32 volumeBits;
	u32 distortionBits;
	s32 distance;
	// NOTE(aalhendi): Keep the signed acceleration input separate from its absolute target.
	register s32 originalPitch CTR_PSX_REGISTER("$3");
	s32 targetPitch;
	s32 pitchDelta;
	u32 previousActions;
	// NOTE(aalhendi): Retain the entry LR read before the driver saves.
	CTR_PSX_KEEP_VALUE_RELAXED(pan);
	// NOTE(aalhendi): Order the AI save after LR.
	CTR_PSX_DEPEND_VALUE(ai, pan);
	// NOTE(aalhendi): Order the camera save after the AI save.
	CTR_PSX_DEPEND_VALUE(cameraDriver, ai);
	// NOTE(aalhendi): Reserve s0 for distance before the remaining register saves.
	CTR_PSX_BIND_VALUE_CLOBBER(slotIndex, "$16");
	originalPitch = ai->const_AccelSpeed_ClassStat;
	previousActions = ai->actionsFlagSetPrevFrame;
	targetPitch = originalPitch;

	if (originalPitch < 0)
	{
		targetPitch = -targetPitch;
	}
	distance = distanceArg;
	if (((previousActions & ACTION_TOUCH_GROUND) != 0) && (ai->kartState != KS_DRIFTING))
	{
		s32 speed = ai->speedApprox;
		if (speed < 0)
		{
			speed = -speed;
		}
		targetPitch = (targetPitch + speed) >> 1;
	}
	else
	{
		targetPitch += 0xf00;
	}
	pitchDelta = targetPitch - ai->engineSoundPitchState;
	if (pitchDelta < 0)
	{
		pitchDelta = -pitchDelta;
	}
	if (pitchDelta >= 0x601)
	{
		s16 cooldown = ai->engineSoundVolumeState + 2000;
		ai->engineSoundVolumeState = cooldown;
		if (cooldown > 14000)
		{
			ai->engineSoundVolumeState = 14000;
		}
	}
	else
	{
		u16 cooldown = ai->engineSoundVolumeState - 500;
		ai->engineSoundVolumeState = cooldown;
		if (ai->kartState == KS_DRIFTING)
		{
			if ((s16)cooldown < 2000)
			{
				ai->engineSoundVolumeState = 2000;
			}
		}
		else if ((s16)cooldown < 0)
		{
			ai->engineSoundVolumeState = 0;
		}
	}
	{
		// NOTE(aalhendi): Prepare the map volume in a0 before smoothing the pitch.
		register s32 mapVolume CTR_PSX_REGISTER("$4") = ai->engineSoundVolumeState;
		// NOTE(aalhendi): The first weighted term reads the previous pitch from v0.
		register s32 previousPitch CTR_PSX_REGISTER("$2") = ai->engineSoundPitchState;
		// NOTE(aalhendi): Prepare the map limit in a2 before the weighted terms.
		register s32 mapAccel CTR_PSX_REGISTER("$6") = ai->const_AccelSpeed_ClassStat;
		u32 previousWeighted;
		// NOTE(aalhendi): Reuse v0 for the second term and the logical shift before narrowing.
		register u32 targetWeighted CTR_PSX_REGISTER("$2");
		previousWeighted = (u32)previousPitch * 0x177;
		targetWeighted = (u32)targetPitch * 0x89;
		targetWeighted = (targetWeighted + previousWeighted) >> 9;
		ai->engineSoundPitchState = (s16)targetWeighted;
		baseVolume = VehCalc_MapToRange(mapVolume, 0, mapAccel, 0x82, 0xe6);
	}
	if (distance < 2000)
	{
		volume = baseVolume;
		if (distance > 200)
		{
			volume = VehCalc_MapToRange(distance, 200, 2000, baseVolume, 0);
		}
	}
	else
	{
		volume = 0;
	}
	{
		s32 *slot;
		slot = &GAME_ENGINE_AUDIO_DEFAULTS[slotIndex];
		volume = EngineSound_VolumeAdjust(volume, *slot, 10);
		*slot = volume;
	}
	distortion = VehCalc_MapToRange(ai->engineSoundPitchState, 0, ai->const_AccelSpeed_ClassStat, 0x3c, 0xaa);
	doppler = distanceDelta >> 3;
	if (doppler < -20)
	{
		doppler = -20;
	}
	else if (doppler > 20)
	{
		doppler = 20;
	}
	distortion = CTR_MipsSubLo(distortion, doppler);
	if (distortion < 0)
	{
		distortion = 0;
	}
	else if (distortion > 255)
	{
		distortion = 255;
	}
	if (pan < 0)
	{
		pan = 0;
	}
	else if (pan > 255)
	{
		pan = 255;
	}
	{
		// NOTE(aalhendi): Add the slot in v0 before masking the sound ID to a halfword.
		register u32 soundNumber CTR_PSX_REGISTER("$2") = (u32)slotIndex + 0x10;
		soundID = soundNumber & 0xffff;
	}
	{
		// NOTE(aalhendi): Retail reuses v0 for each byte mask before shifting its channel.
		register u32 masked CTR_PSX_REGISTER("$2");
		masked = volume & 0xff;
		volumeBits = masked << 16;
		masked = (u32)distortion & 0xff;
		distortionBits = masked << 8;
	}
	{
		// NOTE(aalhendi): Keep the LR byte in a1 for the final packed argument.
		register u32 panByte CTR_PSX_REGISTER("$5") = (u32)pan & 0xff;
		if ((cameraDriver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
		{
			packedFlags = distortionBits | 0x1000000;
			packedFlags = volumeBits | packedFlags;
		}
		else
		{
			packedFlags = volumeBits | distortionBits;
		}
		EngineAudio_Recalculate(soundID, packedFlags | panByte);
	}
}

void EngineSound_NearestAIs(void)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct Driver *closestDrivers[2];
	s32 closestDistances[2];
	s16 closestPlayers[2];
	s32 dir[3];
	struct Thread *thread;
	s16 playerIndex;
	s16 slotIndex;

	if (gGT->numBotsNextGame == 0)
	{
		return;
	}
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail copies an unused player halfword while its driver is NULL.
	// Initialize the native C object before that copy.
	closestPlayers[0] = 0;
	closestPlayers[1] = 0;
#endif
	closestDrivers[0] = NULL;
	closestDrivers[1] = NULL;
	closestDistances[0] = 0x7fffffff;
	closestDistances[1] = 0x7fffffff;
	for (thread = gGT->threadBuckets[ROBOT].thread; thread != NULL; thread = thread->siblingThread)
	{
		struct Driver *ai = thread->object;
		for (playerIndex = 0; playerIndex < GAME_TRACKER->numPlyrCurrGame; playerIndex++)
		{
			s32 dx = CTR_MipsSubLo(GAME_TRACKER->pushBuffer[playerIndex].pos.x, ai->posCurr.x >> 8);
			s32 dy = CTR_MipsSubLo(GAME_TRACKER->pushBuffer[playerIndex].pos.y, ai->posCurr.y >> 8);
			s32 dz = CTR_MipsSubLo(GAME_TRACKER->pushBuffer[playerIndex].pos.z, ai->posCurr.z >> 8);
			s32 distance = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dy, dy)), CTR_MipsMulLo(dz, dz)));
			if (distance < closestDistances[0])
			{
				s16 savedPlayer = closestPlayers[0];
				struct Driver *savedDriver = closestDrivers[0];
				s32 savedDistance = closestDistances[0];
				closestPlayers[0] = playerIndex;
				closestDistances[1] = savedDistance;
				closestDistances[0] = distance;
				closestDrivers[0] = ai;
				closestPlayers[1] = savedPlayer;
				closestDrivers[1] = savedDriver;
			}
			else if (distance < closestDistances[1])
			{
				closestPlayers[1] = playerIndex;
				closestDistances[1] = distance;
				closestDrivers[1] = ai;
			}
		}
	}
	for (slotIndex = 0; slotIndex < 2; slotIndex++)
	{
		// NOTE(aalhendi): Pass the selected driver to GTE in a0.
		register struct Driver *candidate CTR_PSX_REGISTER("$4") = closestDrivers[slotIndex];
		if (candidate != NULL)
		{
			struct Driver *ai;
			s16 cameraIndex = closestPlayers[slotIndex];
			struct Driver *cameraDriver = GAME_TRACKER->cameraDC[cameraIndex].driverToFollow;
			// NOTE(aalhendi): Keep the angle in a2 while forming the signed LR value.
			register s32 angle CTR_PSX_REGISTER("$6");
			s32 lr;
			// NOTE(aalhendi): Reuse v0 for the wrapped unsigned shift and negation.
			register u32 shifted CTR_PSX_REGISTER("$2");
			ai = candidate;
			GTE_AudioLR_Driver(&GAME_TRACKER->pushBuffer[cameraIndex].matrix_Camera, ai, dir);
			angle = CTR_MipsAddLo(ratan2(dir[0], CTR_MipsNegLo(dir[2])), 0x800);
			shifted = (u32)angle << 20;
			lr = (s32)(0u - shifted) >> 23;
			if (lr >= 0x81)
			{
				lr = 0x100 - lr;
			}
			else if (lr < -0x80)
			{
				lr = -0x100 - lr;
			}
			lr += 0x80;
			lr = EngineSound_VolumeAdjust(lr, GAME_ENGINE_PAN_DEFAULTS[slotIndex], 10);
			GAME_ENGINE_PAN_DEFAULTS[slotIndex] = lr;
			EngineSound_AI(ai, cameraDriver, slotIndex, closestDistances[slotIndex],
			               CTR_MipsSubLo(closestDistances[slotIndex], GAME_ENGINE_DISTANCE_DEFAULTS[slotIndex]), lr);
			GAME_ENGINE_DISTANCE_DEFAULTS[slotIndex] = closestDistances[slotIndex];
		}
	}
}

void EngineAudio_Stop(u32 soundID)
{
	u32 id;

	if (GAME_AUDIO_ENABLED == 0)
	{
		return;
	}

	id = soundID & 0xffff;
	if ((u32)GAME_HOWL_HEADER->numEngineFX <= id)
	{
		return;
	}

	Smart_EnterCriticalSection();
	Channel_SearchFX_Destroy(HOWL_CHANNEL_TYPE_ENGINE_FX, id, 0xffffffff);
	Smart_ExitCriticalSection();

	return;
}
