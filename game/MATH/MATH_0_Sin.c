#include <common.h>

int MATH_Sin(u32 angle)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003d184-0x8003d1c0.
	u32 trig = *(u32 *)&data.trigApprox[angle & 0x3ff];
	if ((angle & 0x400) == 0)
	{
		trig <<= 0x10;
	}

	s32 sine = (s32)trig >> 0x10;
	if ((angle & 0x800) != 0)
	{
		sine = -sine;
	}

	return sine;
}
