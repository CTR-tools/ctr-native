#include <common.h>

static int EngineSound_AI_GetTargetPitch(struct Driver *ai)
{
	int target = ai->const_AccelSpeed_ClassStat;

	if (target < 0)
		target = -target;

	if (((ai->actionsFlagSetPrevFrame & 1) == 0) || (ai->kartState == KS_DRIFTING))
	{
		target += 0xf00;
	}
	else
	{
		int speed = ai->speedApprox;

		if (speed < 0)
			speed = -speed;

		target = (target + speed) >> 1;
	}

	return target;
}

static void EngineSound_AI_UpdateSmoothing(struct Driver *ai, int targetPitch)
{
	int delta = targetPitch - ai->fill_3B6[1];

	if (delta < 0)
		delta = -delta;

	if (delta < 0x601)
	{
		u16 cooldown = ai->fill_3B6[0] - 500;
		ai->fill_3B6[0] = cooldown;

		if (ai->kartState == KS_DRIFTING)
		{
			if ((s16)cooldown < 2000)
				ai->fill_3B6[0] = 2000;
		}
		else if ((s16)cooldown < 0)
		{
			ai->fill_3B6[0] = 0;
		}
	}
	else
	{
		s16 cooldown = ai->fill_3B6[0] + 2000;
		ai->fill_3B6[0] = cooldown;

		if (14000 < cooldown)
			ai->fill_3B6[0] = 14000;
	}

	ai->fill_3B6[1] = (s16)((targetPitch * 0x89 + ai->fill_3B6[1] * 0x177) >> 9);
}

static u32 EngineSound_AI_CalculateVolume(struct Driver *ai, int slotIndex, int distance)
{
	u32 volume = DECOMP_VehCalc_MapToRange(ai->fill_3B6[0], 0, ai->const_AccelSpeed_ClassStat, 0x82, 0xe6);

	if (distance < 2000)
	{
		if (200 < distance)
			volume = DECOMP_VehCalc_MapToRange(distance, 200, 2000, volume, 0);
	}
	else
	{
		volume = 0;
	}

	volume = DECOMP_EngineSound_VolumeAdjust(volume, sdata->audioDefaults[slotIndex], 10);
	sdata->audioDefaults[slotIndex] = volume;

	return volume;
}

static u32 EngineSound_AI_CalculateDistortion(struct Driver *ai, int distanceDelta)
{
	int distortion;
	int pitch = DECOMP_VehCalc_MapToRange(ai->fill_3B6[1], 0, ai->const_AccelSpeed_ClassStat, 0x3c, 0xaa);

	distanceDelta >>= 3;

	if (distanceDelta < -0x14)
		distanceDelta = -0x14;
	else if (0x14 < distanceDelta)
		distanceDelta = 0x14;

	distortion = pitch - distanceDelta;
	if (distortion < 0)
		return 0;

	if (0xff < distortion)
		return 0xff;

	return distortion;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002fc64-0x8002ff28
void DECOMP_EngineSound_AI(struct Driver *ai, struct Driver *cameraDriver, int slotIndex, int distance, int distanceDelta, u32 lr)
{
	u32 volume;
	u32 distortion;
	int targetPitch = EngineSound_AI_GetTargetPitch(ai);

	EngineSound_AI_UpdateSmoothing(ai, targetPitch);

	volume = EngineSound_AI_CalculateVolume(ai, slotIndex, distance);
	distortion = EngineSound_AI_CalculateDistortion(ai, distanceDelta);

	if ((int)lr < 0)
		lr = 0;
	else if (0xff < (int)lr)
		lr = 0xff;

	distortion = (distortion & 0xff) << 8;
	if ((cameraDriver->actionsFlagSet & 0x10000) != 0)
		distortion |= 0x1000000;

	EngineAudio_Recalculate((slotIndex + 0x10) & 0xffff, ((volume & 0xff) << 0x10) | distortion | (lr & 0xff));
}

void EngineSound_AI(struct Driver *ai, struct Driver *cameraDriver, int slotIndex, int distance, int distanceDelta, u32 lr)
{
	DECOMP_EngineSound_AI(ai, cameraDriver, slotIndex, distance, distanceDelta, lr);
}
