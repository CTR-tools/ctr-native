#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002f5f4-0x8002fc28
void EngineSound_Player(struct Driver *driver)
{
	u8 id = driver->driverID;
	u32 volume;
	u32 distortion;
	u32 lr;
	int engine = data.MetaDataCharacters[data.characterIDs[id]].engineID;

	if (driver->unk47B == 0)
	{
		driver->fill_3B6[0] = (driver->fill_3B6[0] * 0x177) >> 9;
		driver->fill_3B6[1] = (driver->fill_3B6[1] * 3000 + 0x22400) >> 0xc;

		volume = DECOMP_VehCalc_MapToRange(driver->fill_3B6[0], 0, driver->const_AccelSpeed_ClassStat, 0, 0xe6);
		distortion = DECOMP_VehCalc_MapToRange(driver->fill_3B6[1], 0, driver->const_AccelSpeed_ClassStat, 0x3c, 200);
	}
	else if (driver->unk47B == 1)
	{
		driver->fill_3B6[0] = (driver->fill_3B6[0] * 3000 + 0x322bc0) >> 0xc;
		driver->fill_3B6[1] = (driver->fill_3B6[1] * 3000 + 0x22400) >> 0xc;

		volume = DECOMP_VehCalc_MapToRange(driver->fill_3B6[0], 0, driver->const_AccelSpeed_ClassStat, 0x82, 0xe6);
		distortion = DECOMP_VehCalc_MapToRange(driver->fill_3B6[1], 0, driver->const_AccelSpeed_ClassStat, 0x3c, 200);
	}
	else
	{
		int targetPitch;
		int distortionValue;

		if (driver->kartState == KS_ENGINE_REVVING)
		{
			targetPitch = 0;
			if (0 < driver->fireSpeed)
				targetPitch = 0x3000;

			targetPitch = (driver->fill_3B6[1] * 0x40 + targetPitch * 0x30 + driver->unk36E * 0x90) >> 8;
			if (0 < driver->fireSpeed)
				targetPitch += 0x1000;
		}
		else
		{
			targetPitch = driver->fireSpeed;
			if (targetPitch < 0)
				targetPitch = -targetPitch;

			if (((driver->actionsFlagSetPrevFrame & 1) == 0) || (driver->kartState == KS_DRIFTING))
			{
				targetPitch += 0xf00;
			}
			else
			{
				int speed = driver->speedApprox;
				if (speed < 0)
					speed = -speed;
				targetPitch = (targetPitch + speed) >> 1;
			}
		}

		int pitchDelta = targetPitch - driver->fill_3B6[1];
		if (pitchDelta < 0)
			pitchDelta = -pitchDelta;

		if (pitchDelta < 0x601)
		{
			u16 cooldown = driver->fill_3B6[0] - 500;
			driver->fill_3B6[0] = cooldown;

			if (driver->kartState == KS_DRIFTING)
			{
				if (cooldown < 2000)
					driver->fill_3B6[0] = 2000;
			}
			else if ((s16)cooldown < 0)
			{
				driver->fill_3B6[0] = 0;
			}
		}
		else
		{
			s16 cooldown = driver->fill_3B6[0] + 2000;
			driver->fill_3B6[0] = cooldown;
			if (14000 < cooldown)
				driver->fill_3B6[0] = 14000;
		}

		int steer = driver->wheelRotation;
		driver->fill_3B6[1] = (s16)((targetPitch * 0x89 + driver->fill_3B6[1] * 0x177) >> 9);
		if (steer < 0)
			steer = -steer;

		u32 volMax = ((driver->actionsFlagSet & 0x100000) == 0) ? 0xe6 : 0xbe;
		volume = DECOMP_VehCalc_MapToRange(driver->fill_3B6[0], 0, driver->const_AccelSpeed_ClassStat, 0x82, volMax);

		if ((driver->kartState != KS_DRIFTING) && ((driver->actionsFlagSet & 8) == 0))
			volume += steer >> 3;

		u32 pitchMax = ((driver->actionsFlagSet & 0x100000) == 0) ? 200 : 0xbe;
		int enginePitch =
		    DECOMP_VehCalc_MapToRange(driver->fill_3B6[1], 0, driver->const_AccelSpeed_ClassStat + driver->const_SacredFireSpeed + 0xf00, 0x3c, pitchMax);

		if ((driver->actionsFlagSet & 0x100000) == 0)
		{
			if (driver->kartState == KS_DRIFTING)
			{
				if (driver->turbo_MeterRoomLeft == 0)
				{
					if (driver->sfxDistortOffset != 0)
						driver->sfxDistortOffset--;
				}
				else
				{
					driver->sfxDistortOffset = ((u8)driver->const_turboMaxRoom >> 1) - (driver->turbo_MeterRoomLeft >> 6);
				}

				int drift = (s32)((u32)driver->unk3D4[0] << 0x10) >> 0x13;
				if (drift < 0)
					drift = -drift;
				enginePitch -= drift;
				if (enginePitch < 0)
					enginePitch = 0;
			}

			distortionValue = enginePitch + driver->sfxDistortOffset;
		}
		else
		{
			int drift = (s32)((u32)driver->unk3D4[0] << 0x10) >> 0x13;
			if (drift < 0)
				drift = -drift;
			distortionValue = enginePitch - drift;
			if (distortionValue < 0)
				distortionValue = 0;
		}

		if (0xff < distortionValue)
			distortionValue = 0xff;
		distortion = (u32)distortionValue;
	}

	lr = 0x80 - ((s32)((u32)driver->wheelRotation << 0x10) >> 0x13);
	if (lr < 0x40)
		lr = 0x40;
	else if (0xc0 < lr)
		lr = 0xc0;

	volume = (volume & 0xff) << 0x10;
	distortion = (distortion & 0xff) << 8;
	lr &= 0xff;

	if ((driver->actionsFlagSet & 0x10000) != 0)
		volume |= distortion | 0x1000000;
	else
		volume |= distortion;

	EngineAudio_Recalculate(((engine * 4) + id) & 0xffff, volume | lr);
}
