#include <common.h>

extern struct RacingWheelData rwd_default;

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006163c-0x800616b0.
int VehPhysJoystick_ReturnToRest(int stickVal, int half, struct RacingWheelData *rwd)
{
	if (rwd == 0)
	{
		rwd = &rwd_default;
	}

	stickVal = CTR_MipsSubLo(stickVal, rwd->gamepadCenter);

	if (stickVal < 0)
	{
		return CTR_MipsNegLo(VehCalc_MapToRange(CTR_MipsNegLo(stickVal), rwd->deadZone, rwd->range, 0, half));
	}

	return VehCalc_MapToRange(stickVal, rwd->deadZone, rwd->range, 0, half);
}

struct RacingWheelData rwd_default = {.gamepadCenter = 0x80,

                                      .deadZone = 0x30,

                                      .range = 0x7f};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800616b0-0x800617cc.
int VehPhysJoystick_GetStrength(int val, int max, struct RacingWheelData *rwd)
{
	int dead;
	int range;
	int dist;

	if (rwd == 0)
	{
		dead = 0x30;
		range = 0x7f;
		dist = 0x5e;
	}

	else
	{
		dead = rwd->deadZone;
		range = rwd->range;
		dist = CTR_MipsSubLo(range, dead);
	}

	if (val < dead)
	{
		return 0;
	}

	dead = CTR_MipsSubLo(val, dead);

	if (range <= val)
	{
		return max;
	}

	int halfDist = CTR_MipsSra(CTR_MipsAddLo(dist, (u32)dist >> 31), 1);
	int maxFifth = max / 5;

	if (halfDist <= dead)
	{
		dead = CTR_MipsSubLo(dead, halfDist);
		dead = CTR_MipsMulLo(dead, CTR_MipsSubLo(max, maxFifth));
		dead = CTR_MipsSll(dead, 1);
		return CTR_MipsAddLo(CTR_MipsDiv(dead, dist), maxFifth);
	}

	dead = CTR_MipsMulLo(dead, CTR_MipsSll(maxFifth, 1));
	return CTR_MipsDiv(dead, dist);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800617cc-0x8006181c.
int VehPhysJoystick_GetStrengthAbsolute(int stickVal, int maxSteer, struct RacingWheelData *rwd)
{
	int center = 0x80;
	if (rwd != NULL)
	{
		center = rwd->gamepadCenter;
	}

	int distFromCenter = CTR_MipsSubLo(stickVal, center);

	// if steering right
	if (distFromCenter < 0)
	{
		return CTR_MipsNegLo(VehPhysJoystick_GetStrength(CTR_MipsNegLo(distFromCenter), maxSteer, rwd));
	}

	// steer left
	return VehPhysJoystick_GetStrength(distFromCenter, maxSteer, rwd);
}
