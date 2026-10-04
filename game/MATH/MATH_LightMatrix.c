#include <ctr_types.h>

// NOTE(aalhendi): Retail passes and returns packed matrix words in t3-t7.
// These declarations must precede common.h's inline definitions and stay
// confined to this leaf's retail translation unit.

#ifndef CTR_NATIVE
register u32 lightT3 CTR_PSX_REGISTER("$11");
register u32 lightT4 CTR_PSX_REGISTER("$12");
register u32 lightT5 CTR_PSX_REGISTER("$13");
register u32 lightT6 CTR_PSX_REGISTER("$14");
register u32 lightT7 CTR_PSX_REGISTER("$15");
#endif

#include <common.h>

#ifdef CTR_NATIVE
#define MATH_LIGHT_ARGUMENTS u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4
#else
#define MATH_LIGHT_ARGUMENTS void
#endif

void Unknown_8006c558(MATH_LIGHT_ARGUMENTS)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Snapshot every input before the ordered, possibly aliased stores.
	u32 lightT3 = *r0;
	u32 lightT4 = *r1;
	u32 lightT5 = *r2;
	u32 lightT6 = *r3;
	u32 lightT7 = *r4;
#endif
	register u32 packed CTR_PSX_REGISTER("$3");
	register u32 work CTR_PSX_REGISTER("$8");
	register u32 highMask CTR_PSX_REGISTER("$9");

	// NOTE(aalhendi): Retail materializes the shared mask before starting column zero.
	highMask = 0xffff0000u;
	CTR_PSX_OBSERVE_VALUE(highMask);
	// Stage all three columns through LIGHT; leave ROT and LIGHT controls unchanged.
	packed = lightT3 & 0xffffu;
	work = lightT4 & highMask;
	packed |= work;
	MTC2(packed, 0);
	MTC2(lightT6, 1);
	packed = lightT3 >> 16;
	work = lightT5 << 16;
	gte_llv0_b();
	packed |= work;
	work = lightT6 >> 16;
	MTC2(packed, 2);
	MTC2(work, 3);
	packed = lightT4 & 0xffffu;
	work = lightT5 & highMask;
	packed |= work;
	lightT3 = MFC2(9);
	lightT4 = MFC2(10);
	lightT6 = MFC2(11);
	gte_llv1_b();
	MTC2(packed, 4);
	MTC2(lightT7, 5);
	lightT3 &= 0xffffu;
	lightT4 <<= 16;
	lightT6 &= 0xffffu;
	packed = MFC2(9);
	lightT5 = MFC2(10);
	work = MFC2(11);
	gte_llv2_b();
	packed <<= 16;
	lightT3 |= packed;
	lightT5 &= 0xffffu;
	work <<= 16;
	lightT6 |= work;
	packed = MFC2(9);
	work = MFC2(10);
	lightT7 = MFC2(11);
	packed &= 0xffffu;
	lightT4 |= packed;
	work <<= 16;
	lightT5 |= work;

#ifdef CTR_NATIVE
	*r0 = lightT3;
	*r1 = lightT4;
	*r2 = lightT5;
	*r3 = lightT6;
	*r4 = lightT7;
#endif
}

#undef MATH_LIGHT_ARGUMENTS
