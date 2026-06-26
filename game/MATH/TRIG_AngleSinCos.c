#include <common.h>

static void TRIG_AngleSinCos_Common(u32 angle, s32 *sine, s32 *cosine)
{
	u32 trig = *(u32 *)&data.trigApprox[angle & 0x3ff];

	if ((angle & 0x400) == 0)
	{
		*sine = (s32)(trig << 0x10) >> 0x10;
		*cosine = (s32)trig >> 0x10;

		if ((angle & 0x800) != 0)
		{
			*sine = -*sine;
			*cosine = -*cosine;
		}
	}
	else
	{
		*sine = (s32)trig >> 0x10;
		*cosine = (s32)(trig << 0x10) >> 0x10;

		if ((angle & 0x800) == 0)
		{
			*cosine = -*cosine;
		}
		else
		{
			*sine = -*sine;
		}
	}
}

void TRIG_AngleSinCos_r16r17r18_duplicate(u32 angle, u32 *sine, u32 *cosine)
{
	u32 trig = *(u32 *)&data.trigApprox[angle & 0x3ff];

	// NOTE(aalhendi): Retail input is s0 and outputs are zero-extended s1/s2.
	if ((angle & 0x400) == 0)
	{
		*cosine = trig >> 0x10;
		*sine = trig & 0xffff;

		if ((angle & 0x800) != 0)
		{
			*cosine = (0u - *cosine) & 0xffff;
			*sine = (0u - *sine) & 0xffff;
		}
	}
	else
	{
		*cosine = trig & 0xffff;
		*sine = trig >> 0x10;

		if ((angle & 0x800) == 0)
		{
			*cosine = (0u - *cosine) & 0xffff;
		}
		else
		{
			*sine = (0u - *sine) & 0xffff;
		}
	}
}

void TRIG_AngleSinCos_r19r17r18(u32 angle, s32 *sine, s32 *cosine)
{
	// NOTE(aalhendi): Retail input is s3 and outputs are s1/s2.
	TRIG_AngleSinCos_Common(angle, sine, cosine);
}

void TRIG_AngleSinCos_r15r16r17(u32 angle, s32 *sine, s32 *cosine)
{
	// NOTE(aalhendi): Retail input is t7 and outputs are s0/s1.
	TRIG_AngleSinCos_Common(angle, sine, cosine);
}

void TRIG_AngleSinCos_r16r17r18(u32 angle, s32 *sine, s32 *cosine)
{
	// NOTE(aalhendi): Retail input is s0 and outputs are s1/s2.
	TRIG_AngleSinCos_Common(angle, sine, cosine);
}

void TRIG_AngleSinCos_r9r8r10(u32 angle, s32 *sine, s32 *cosine)
{
	// NOTE(aalhendi): Retail input is t1 and outputs are t0/t2.
	TRIG_AngleSinCos_Common(angle, sine, cosine);
}
