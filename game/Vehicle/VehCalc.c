#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80058f54-0x80058f9c.
int VehCalc_InterpBySpeed(int val, int speed, int desired)
{
	if (val > desired)
	{
		val = CTR_MipsSubLo(val, speed);

		if (val < desired)
		{
			return desired;
		}
	}

	else
	{
		val = CTR_MipsAddLo(val, speed);

		if (val > desired)
		{
			return desired;
		}
	}

	return val;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80058f9c-0x8005900c.
int VehCalc_MapToRange(int val, int oldMin, int oldMax, int newMin, int newMax)
{
	if (val <= oldMin)
	{
		return newMin;
	}

	if (val >= oldMax)
	{
		return newMax;
	}

	int distFromBottom = CTR_MipsSubLo(val, oldMin);
	int newRange = CTR_MipsSubLo(newMax, newMin);
	int oldRange = CTR_MipsSubLo(oldMax, oldMin);
	int scaledDistance = CTR_MipsDiv(CTR_MipsMulLo(distFromBottom, newRange), oldRange);

	return CTR_MipsAddLo(newMin, scaledDistance);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005900c-0x80059070.
int VehCalc_SteerAccel(int steeringFrameCount, int stage2FirstFrame, int stage2FrameLength, int stage4FirstFrame, int stage1MinSteer, int stage1MaxSteer)
{
	int steerAccel = stage1MaxSteer;

	// Steering Stage 1,
	// if first 4 frames of steering
	// increase steer acceleration as time passes
	if (steeringFrameCount < stage2FirstFrame)
	{
		// map "frame" from [0,4] -> [0x800,0xC00]

		steerAccel = VehCalc_MapToRange(steeringFrameCount, 0, stage2FirstFrame, stage1MinSteer, stage1MaxSteer);
	}

	else
	{
		int stage3Start = CTR_MipsAddLo(stage2FirstFrame, stage2FrameLength);

		// Steering Stage 3
		// frames 12+
		// decrease steer acceleration as time passes
		if (stage3Start < steeringFrameCount)
		{
			// map "frame" from [12,64] -> [0xC00,0]

			steerAccel = VehCalc_MapToRange(steeringFrameCount, stage3Start, stage4FirstFrame, stage1MaxSteer, 0);
		}
	}

	// Steering Stage 2,
	// next 0x8 frames (frame 4 to 12)
	// max steer accel of 0xC00
	return steerAccel;

	// Steering Stage 4,
	// part of Stage 3's mapping,
	// for all steering after frame 64,
	// steer acceleration is zero, so
	// angular velocity is constant
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80059070-0x80059100.
u32 VehCalc_FastSqrt(u32 n, u32 shift)
{
	u32 result = 0;
	u32 rootBitIndex = 1;
	u32 lastApproximation = 0;
	u32 testBit = (u32)1 << (shift & 0x1f);

	while ((testBit < n) && ((testBit << 2) != 0))
	{
		rootBitIndex++;
		testBit <<= 2;
	}

	u32 addBit = (u32)1 << ((rootBitIndex + CTR_MipsSubLo(shift, 1)) & 0x1f);

	while (testBit != 0)
	{
		u32 shiftedResult = result << (rootBitIndex & 0x1f);

		if ((s32)rootBitIndex < 0)
		{
			shiftedResult = result >> (CTR_MipsNegLo(rootBitIndex) & 0x1f);
		}

		u32 approximation = shiftedResult + lastApproximation + testBit;

		if (approximation <= n)
		{
			result += addBit;
			lastApproximation = approximation;
		}

		addBit >>= 1;
		testBit >>= 2;
		rootBitIndex--;
	}

	return result;
}
