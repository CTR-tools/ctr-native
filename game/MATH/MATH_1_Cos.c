#include <common.h>

int MATH_Cos(u32 angle)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003d1c0-0x8003d214.
	u32 trig = *(u32 *)&data.trigApprox[angle & 0x3ff];
	s32 cosine;

	if ((angle & 0x400) != 0)
	{
		cosine = (s32)(trig << 0x10) >> 0x10;
		if ((angle & 0x800) == 0)
		{
			cosine = -cosine;
		}
	}
	else
	{
		cosine = (s32)trig >> 0x10;
		if ((angle & 0x800) != 0)
		{
			cosine = -cosine;
		}
	}

	return cosine;
}
