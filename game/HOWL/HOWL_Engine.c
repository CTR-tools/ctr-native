#include <common.h>

// Initialize car engine audio system for one driver
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80028880-0x800289b0
char EngineAudio_InitOnce(u32 soundID, u32 flags)

{
	struct EngineFX *ptrEngineFX;
	struct ChannelStats *channel;
	u32 distortion = (flags >> 8) & 0xff;
	u32 volume = (flags >> 0x10) & 0xff;
	u16 echo = (flags >> 0x18) & 0xff;
	u16 LR = flags & 0xff;
	struct ChannelAttr channelAttr;

	if (sdata->boolAudioEnabled == 0)
	{
		return 0;
	}

	// check out of bounds
	soundID = soundID & 0xffff;
	if ((int)soundID >= sdata->ptrHowlHeader->numEngineFX)
	{
		return 0;
	}

	// check sound is loaded
	ptrEngineFX = &sdata->howl_metaEngineFX[soundID];
	if (sdata->howl_spuAddrs[ptrEngineFX->spuIndex].spuAddr == 0)
	{
		return 0;
	}

	// make ChannelAttr from howl pointer somehow
	howl_InitChannelAttr_EngineFX(ptrEngineFX, &channelAttr, volume, LR, distortion);
	channelAttr.reverb = echo;

	Smart_EnterCriticalSection();

	channel = Channel_AllocSlot(0x7c, &channelAttr);

	// if channel was found
	if (channel != 0)
	{
		// type engineFX
		channel->type = 0;
		channel->unk2 = 0;
		channel->echo = echo;
		channel->vol = volume;
		channel->distort = distortion;
		channel->LR = LR;
		channel->timeLeft = 0;
		channel->soundID = soundID;
		channel->flags |= 4;
	}

	Smart_ExitCriticalSection();

	return (channel != 0);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800289b0-0x80028b54
s16 EngineAudio_Recalculate(u32 soundID, u32 sfx)
{
	int iVar1;
	u32 distortion = (sfx >> 8) & 0xff;
	u32 volume = (sfx >> 0x10) & 0xff;
	u16 echo = (sfx >> 0x18) & 0xff;
	u16 LR = sfx & 0xff;

	struct EngineFX *ptrEngineFX;
	struct ChannelStats *channel;
	struct ChannelAttr channelAttr;
	struct GameTracker *gGT;

	if (sdata->boolAudioEnabled == 0)
	{
		return 0;
	}

	soundID = soundID & 0xffff;
	if (sdata->ptrHowlHeader->numEngineFX <= (int)soundID)
	{
		return 0;
	}

	gGT = sdata->gGT;

	ptrEngineFX = &sdata->howl_metaEngineFX[soundID];

	if (gGT->numPlyrCurrGame > 1)
	{
		// 3P/4P game
		iVar1 = volume * 0x2d;

		// 2P game
		if (gGT->numPlyrCurrGame == 2)
		{
			iVar1 = volume * 0x37;
		}

		volume = (iVar1 << 2) >> 8;
	}

	// no distortion
	if (distortion == 0x80)
	{
		channelAttr.pitch = ptrEngineFX->pitch;
	}

	// distortion
	else
	{
		channelAttr.pitch = ptrEngineFX->pitch * data.distortConst_Engine[distortion] >> 0x10;
	}

	Channel_SetVolume(&channelAttr, sdata->vol_FX * ptrEngineFX->volume * volume >> 10, LR);
	channelAttr.reverb = echo;

	Smart_EnterCriticalSection();

	// 0 - engineFX
	// soundID & 0xffff, dont search for specific instance
	channel = Channel_SearchFX_EditAttr(0, soundID, 0x70, &channelAttr);

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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002f5f4-0x8002fc28
void EngineSound_Player(struct Driver *driver)
{
	u8 id = driver->driverID;
	u32 volume;
	u32 distortion;
	u32 lr;
	int engine = data.MetaDataCharacters[data.characterIDs[id]].engineID;

	if (driver->engineSoundMode == ENGINE_SOUND_FADE_OUT)
	{
		driver->engineSoundVolumeState = (driver->engineSoundVolumeState * 0x177) >> 9;
		driver->engineSoundPitchState = (driver->engineSoundPitchState * 3000 + 0x22400) >> 0xc;

		volume = VehCalc_MapToRange(driver->engineSoundVolumeState, 0, driver->const_AccelSpeed_ClassStat, 0, 0xe6);
		distortion = VehCalc_MapToRange(driver->engineSoundPitchState, 0, driver->const_AccelSpeed_ClassStat, 0x3c, 200);
	}
	else if (driver->engineSoundMode == ENGINE_SOUND_FADE_IN)
	{
		driver->engineSoundVolumeState = (driver->engineSoundVolumeState * 3000 + 0x322bc0) >> 0xc;
		driver->engineSoundPitchState = (driver->engineSoundPitchState * 3000 + 0x22400) >> 0xc;

		volume = VehCalc_MapToRange(driver->engineSoundVolumeState, 0, driver->const_AccelSpeed_ClassStat, 0x82, 0xe6);
		distortion = VehCalc_MapToRange(driver->engineSoundPitchState, 0, driver->const_AccelSpeed_ClassStat, 0x3c, 200);
	}
	else
	{
		int targetPitch;
		int distortionValue;

		if (driver->kartState == KS_ENGINE_REVVING)
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
			targetPitch = driver->fireSpeed;
			if (targetPitch < 0)
			{
				targetPitch = -targetPitch;
			}

			if (((driver->actionsFlagSetPrevFrame & 1) == 0) || (driver->kartState == KS_DRIFTING))
			{
				targetPitch += 0xf00;
			}
			else
			{
				int speed = driver->speedApprox;
				if (speed < 0)
				{
					speed = -speed;
				}
				targetPitch = (targetPitch + speed) >> 1;
			}
		}

		int pitchDelta = targetPitch - driver->engineSoundPitchState;
		if (pitchDelta < 0)
		{
			pitchDelta = -pitchDelta;
		}

		if (pitchDelta < 0x601)
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
		else
		{
			s16 cooldown = driver->engineSoundVolumeState + 2000;
			driver->engineSoundVolumeState = cooldown;
			if (14000 < cooldown)
			{
				driver->engineSoundVolumeState = 14000;
			}
		}

		int steer = driver->wheelRotation;
		driver->engineSoundPitchState = (s16)((targetPitch * 0x89 + driver->engineSoundPitchState * 0x177) >> 9);
		if (steer < 0)
		{
			steer = -steer;
		}

		u32 volMax = ((driver->actionsFlagSet & ACTION_BOT) == 0) ? 0xe6 : 0xbe;
		volume = VehCalc_MapToRange(driver->engineSoundVolumeState, 0, driver->const_AccelSpeed_ClassStat, 0x82, volMax);

		if ((driver->kartState != KS_DRIFTING) && ((driver->actionsFlagSet & 8) == 0))
		{
			volume += steer >> 3;
		}

		u32 pitchMax = ((driver->actionsFlagSet & ACTION_BOT) == 0) ? 200 : 0xbe;
		int enginePitch =
		    VehCalc_MapToRange(driver->engineSoundPitchState, 0, driver->const_AccelSpeed_ClassStat + driver->const_SacredFireSpeed + 0xf00, 0x3c, pitchMax);

		if ((driver->actionsFlagSet & ACTION_BOT) == 0)
		{
			if (driver->kartState == KS_DRIFTING)
			{
				if (driver->turbo_MeterRoomLeft == 0)
				{
					if (driver->sfxDistortOffset != 0)
					{
						driver->sfxDistortOffset--;
					}
				}
				else
				{
					driver->sfxDistortOffset = ((u8)driver->const_turboMaxRoom >> 1) - (driver->turbo_MeterRoomLeft >> 6);
				}

				int drift = (s32)((u32)driver->turnWobbleAngle << 0x10) >> 0x13;
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
		else
		{
			int drift = (s32)((u32)driver->turnWobbleAngle << 0x10) >> 0x13;
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

		if (0xff < distortionValue)
		{
			distortionValue = 0xff;
		}
		distortion = (u32)distortionValue;
	}

	lr = 0x80 - ((s32)((u32)driver->wheelRotation << 0x10) >> 0x13);
	if (lr < 0x40)
	{
		lr = 0x40;
	}
	else if (0xc0 < lr)
	{
		lr = 0xc0;
	}

	volume = (volume & 0xff) << 0x10;
	distortion = (distortion & 0xff) << 8;
	lr &= 0xff;

	if ((driver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
	{
		volume |= distortion | 0x1000000;
	}
	else
	{
		volume |= distortion;
	}

	EngineAudio_Recalculate(((engine * 4) + id) & 0xffff, volume | lr);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002fc28-0x8002fc64
int EngineSound_VolumeAdjust(int desired, int current, int step)
{
	int delta = desired - current;
	int absDelta = delta;

	if (absDelta < 0)
	{
		absDelta = -absDelta;
	}

	if (step < absDelta)
	{
		if (delta < 1)
		{
			return current - step;
		}

		return current + step;
	}

	return desired;
}

static int EngineSound_AI_GetTargetPitch(struct Driver *ai)
{
	int target = ai->const_AccelSpeed_ClassStat;

	if (target < 0)
	{
		target = -target;
	}

	if (((ai->actionsFlagSetPrevFrame & 1) == 0) || (ai->kartState == KS_DRIFTING))
	{
		target += 0xf00;
	}
	else
	{
		int speed = ai->speedApprox;

		if (speed < 0)
		{
			speed = -speed;
		}

		target = (target + speed) >> 1;
	}

	return target;
}

static void EngineSound_AI_UpdateSmoothing(struct Driver *ai, int targetPitch)
{
	int delta = targetPitch - ai->engineSoundPitchState;

	if (delta < 0)
	{
		delta = -delta;
	}

	if (delta < 0x601)
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
	else
	{
		s16 cooldown = ai->engineSoundVolumeState + 2000;
		ai->engineSoundVolumeState = cooldown;

		if (14000 < cooldown)
		{
			ai->engineSoundVolumeState = 14000;
		}
	}

	ai->engineSoundPitchState = (s16)((targetPitch * 0x89 + ai->engineSoundPitchState * 0x177) >> 9);
}

static u32 EngineSound_AI_CalculateVolume(struct Driver *ai, int slotIndex, int distance)
{
	u32 volume = VehCalc_MapToRange(ai->engineSoundVolumeState, 0, ai->const_AccelSpeed_ClassStat, 0x82, 0xe6);

	if (distance < 2000)
	{
		if (200 < distance)
		{
			volume = VehCalc_MapToRange(distance, 200, 2000, volume, 0);
		}
	}
	else
	{
		volume = 0;
	}

	volume = EngineSound_VolumeAdjust(volume, sdata->audioDefaults[slotIndex], 10);
	sdata->audioDefaults[slotIndex] = volume;

	return volume;
}

static u32 EngineSound_AI_CalculateDistortion(struct Driver *ai, int distanceDelta)
{
	int distortion;
	int pitch = VehCalc_MapToRange(ai->engineSoundPitchState, 0, ai->const_AccelSpeed_ClassStat, 0x3c, 0xaa);

	distanceDelta >>= 3;

	if (distanceDelta < -0x14)
	{
		distanceDelta = -0x14;
	}
	else if (0x14 < distanceDelta)
	{
		distanceDelta = 0x14;
	}

	distortion = pitch - distanceDelta;
	if (distortion < 0)
	{
		return 0;
	}

	if (0xff < distortion)
	{
		return 0xff;
	}

	return distortion;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002fc64-0x8002ff28
void EngineSound_AI(struct Driver *ai, struct Driver *cameraDriver, int slotIndex, int distance, int distanceDelta, u32 lr)
{
	u32 volume;
	u32 distortion;
	int targetPitch = EngineSound_AI_GetTargetPitch(ai);

	EngineSound_AI_UpdateSmoothing(ai, targetPitch);

	volume = EngineSound_AI_CalculateVolume(ai, slotIndex, distance);
	distortion = EngineSound_AI_CalculateDistortion(ai, distanceDelta);

	if ((int)lr < 0)
	{
		lr = 0;
	}
	else if (0xff < (int)lr)
	{
		lr = 0xff;
	}

	distortion = (distortion & 0xff) << 8;
	if ((cameraDriver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
	{
		distortion |= 0x1000000;
	}

	EngineAudio_Recalculate((slotIndex + 0x10) & 0xffff, ((volume & 0xff) << 0x10) | distortion | (lr & 0xff));
}

static int EngineSound_NearestAIs_GetDistance(struct Driver *ai, int pushBufferIndex)
{
	struct PushBuffer *pb = &sdata->gGT->pushBuffer[pushBufferIndex];
	int dx = CTR_MipsSubLo(pb->pos.x, CTR_MipsSra(ai->posCurr.x, 8));
	int dy = CTR_MipsSubLo(pb->pos.y, CTR_MipsSra(ai->posCurr.y, 8));
	int dz = CTR_MipsSubLo(pb->pos.z, CTR_MipsSra(ai->posCurr.z, 8));

	return SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dy, dy)), CTR_MipsMulLo(dz, dz)));
}

static void EngineSound_NearestAIs_InsertClosest(struct Driver *ai, int playerIndex, int distance, struct Driver **closestDrivers, int *closestDistances,
                                                 s16 *closestPlayers)
{
	if (distance < closestDistances[0])
	{
		closestPlayers[1] = closestPlayers[0];
		closestDrivers[1] = closestDrivers[0];
		closestDistances[1] = closestDistances[0];

		closestPlayers[0] = playerIndex;
		closestDrivers[0] = ai;
		closestDistances[0] = distance;
	}
	else if (distance < closestDistances[1])
	{
		closestPlayers[1] = playerIndex;
		closestDrivers[1] = ai;
		closestDistances[1] = distance;
	}
}

static int EngineSound_NearestAIs_CalculateLR(s32 *dir)
{
	int angle = CTR_MipsAddLo(ratan2(dir[0], CTR_MipsNegLo(dir[2])), 0x800);
	int lr = CTR_MipsSra(CTR_MipsNegLo(CTR_MipsSll(angle, 20)), 23);

	if (lr < 0x81)
	{
		if (lr < -0x80)
		{
			lr = -0x100 - lr;
		}
	}
	else
	{
		lr = 0x100 - lr;
	}

	return lr + 0x80;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ff28-0x80030208
void EngineSound_NearestAIs(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *closestDrivers[2];
	int closestDistances[2];
	s16 closestPlayers[2];

	if (gGT->numBotsCurrGame == 0)
	{
		return;
	}

	closestDrivers[0] = NULL;
	closestDrivers[1] = NULL;
	closestDistances[0] = 0x7fffffff;
	closestDistances[1] = 0x7fffffff;

	for (struct Thread *thread = gGT->threadBuckets[ROBOT].thread; thread != NULL; thread = thread->siblingThread)
	{
		struct Driver *ai = thread->object;

		for (int i = 0; i < (u8)gGT->numPlyrCurrGame; i++)
		{
			EngineSound_NearestAIs_InsertClosest(ai, i, EngineSound_NearestAIs_GetDistance(ai, i), closestDrivers, closestDistances, closestPlayers);
		}
	}

	for (int i = 0; i < 2; i++)
	{
		struct Driver *ai = closestDrivers[i];
		if (ai != NULL)
		{
			s32 dir[3];
			s16 playerIndex = closestPlayers[i];
			struct Driver *cameraDriver = gGT->cameraDC[playerIndex].driverToFollow;
			u32 lr;

			GTE_AudioLR_Driver(&gGT->pushBuffer[playerIndex].matrix_Camera, ai, dir);

			lr = EngineSound_VolumeAdjust(EngineSound_NearestAIs_CalculateLR(dir), sdata->audioDefaults[4 + i], 10);
			sdata->audioDefaults[4 + i] = lr;

			EngineSound_AI(ai, cameraDriver, i, closestDistances[i], closestDistances[i] - sdata->audioDefaults[2 + i], lr);
			sdata->audioDefaults[2 + i] = closestDistances[i];
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80028b54-0x80028bbc
void EngineAudio_Stop(u32 soundID)
{
	if (sdata->boolAudioEnabled == 0)
	{
		return;
	}

	soundID = soundID & 0xffff;
	if (sdata->ptrHowlHeader->numEngineFX <= (int)soundID)
	{
		return;
	}

	// 0 - engineFX
	Smart_EnterCriticalSection();
	Channel_SearchFX_Destroy(0, soundID, 0xffffffff);
	Smart_ExitCriticalSection();

	return;
}
