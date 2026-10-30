#ifndef CTR_COLL_FIXED_TRIANGLE_SHARED_H
#define CTR_COLL_FIXED_TRIANGLE_SHARED_H

#include <common.h>

internal s32 Coll_MipsAbsS32(s32 value)
{
	return (value < 0) ? CTR_MipsNegLo(value) : value;
}

struct CollTriangleProjection
{
	s32 firstA;
	s32 firstB;
	s32 firstHit;
	s32 secondA;
	s32 secondB;
	s32 secondHit;
};

internal void CollFixed_GteLoadR11R12(u32 value)
{
	CTC2(value, 0);
}

internal void CollFixed_GteLoadR13R21(u32 value)
{
	CTC2(value, 1);
}

internal void CollFixed_GteLoadR22R23(u32 value)
{
	CTC2(value, 2);
}

internal void CollFixed_GteLoadVXY0(u32 value)
{
	MTC2(value, 0);
}

internal void CollFixed_GteLoadVZ0(s32 value)
{
	MTC2_S(value, 1);
}

internal void CollFixed_GteLoadIR0(s32 value)
{
	MTC2_S(value, 8);
}

internal void CollFixed_GteLoadIR(s32 x, s32 y, s32 z)
{
	MTC2_S(x, 9);
	MTC2_S(y, 10);
	MTC2_S(z, 11);
}

internal void CollFixed_GteLoadMAC(s32 x, s32 y, s32 z)
{
	MTC2_S(x, 25);
	MTC2_S(y, 26);
	MTC2_S(z, 27);
}

internal s32 CollFixed_GteReadMAC1(void)
{
	return MFC2_S(25);
}

internal s32 CollFixed_GteReadMAC2(void)
{
	return MFC2_S(26);
}

internal s32 CollFixed_GteReadMAC3(void)
{
	return MFC2_S(27);
}

internal void CollFixed_GteMVMVA(void)
{
	doCOP2(0x0406012);
}

internal void CollFixed_GteGPL12(void)
{
	doCOP2(0x01a8003e);
}

#endif
