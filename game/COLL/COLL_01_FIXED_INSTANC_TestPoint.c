#include <common.h>

u32 COLL_FIXED_INSTANC_TestPoint(struct ScratchpadStruct *sps, struct BSP *node)
{
	u8 *spsBytes = (u8 *)sps;
	u8 *nodeBytes = (u8 *)node;
	s32 diffX;
	s32 diffY;
	s32 diffZ;
	s32 centerDiffX;
	s32 centerDiffY;
	s32 centerDiffZ;
	s32 dotSegment;
	s32 dotCenter;
	s32 shift;
	s32 divisor;
	s32 factor;
	s32 projX;
	s32 projY;
	s32 projZ;
	s32 relX;
	s32 relY;
	s32 relZ;
	s32 radius;
	s32 radiusSquared;
	s32 distSquared;
	s32 remaining;
	s32 hitX;
	s32 hitY;
	s32 hitZ;
	s32 normalX;
	s32 normalY;
	s32 normalZ;
	s32 len;
	s32 invLen;
	s32 scaledX;
	s32 scaledY;
	s32 scaledZ;

	if (nodeBytes[1] == 4)
	{
		*(struct BSP **)(spsBytes + 0x48) = node;
		CollFixed_WriteS16(spsBytes, 0x42, CollFixed_ReadU16(spsBytes, 0x42) + 1);
	}

	if ((CollFixed_ReadU16(spsBytes, 0x22) & 0x40) != 0)
	{
		CollFixed_WriteS16(spsBytes, 0x72, 0x1000);
		CollFixed_WriteS16(spsBytes, 0x70, 0);
		CollFixed_WriteS16(spsBytes, 0x74, 0);
		spsBytes[0x7e] = 6;
		CollFixed_WriteS32(spsBytes, 0x84, 0);
		*(struct BSP **)(spsBytes + 0x48) = node;
		CollFixed_WriteS16(spsBytes, 0x1c, CollFixed_ReadU16(spsBytes, 0x10));
		CollFixed_WriteS16(spsBytes, 0x1e, CollFixed_ReadU16(spsBytes, 0x12));
		CollFixed_WriteS16(spsBytes, 0x20, CollFixed_ReadU16(spsBytes, 0x14));
		CollFixed_WriteS16(spsBytes, 0x42, CollFixed_ReadU16(spsBytes, 0x42) + 1);
		return 6;
	}

	CollFixed_WriteS32(spsBytes, 0x1c8, 0);

	diffX = CollFixed_ReadS16(spsBytes, 0) - CollFixed_ReadS16(spsBytes, 0x10);
	diffY = 0;
	diffZ = CollFixed_ReadS16(spsBytes, 4) - CollFixed_ReadS16(spsBytes, 0x14);
	CollFixed_WriteS32(spsBytes, 0x1c4, diffX);
	CollFixed_WriteS32(spsBytes, 0x1cc, diffZ);

	centerDiffX = CollFixed_ReadS16(nodeBytes, 0x10) - CollFixed_ReadS16(spsBytes, 0x10);
	centerDiffY = 0;
	centerDiffZ = CollFixed_ReadS16(nodeBytes, 0x14) - CollFixed_ReadS16(spsBytes, 0x14);
	CollFixed_WriteS32(spsBytes, 0x1d4, 0);
	CollFixed_WriteS32(spsBytes, 0x1d0, centerDiffX);
	CollFixed_WriteS32(spsBytes, 0x1d8, centerDiffZ);

	if ((nodeBytes[0] & 0x40) != 0)
	{
		diffY = CollFixed_ReadS16(spsBytes, 2) - CollFixed_ReadS16(spsBytes, 0x12);
		CollFixed_WriteS32(spsBytes, 0x1c8, diffY);
		centerDiffY = CollFixed_ReadS16(nodeBytes, 0x12) - CollFixed_ReadS16(spsBytes, 0x12);
		CollFixed_WriteS32(spsBytes, 0x1d4, centerDiffY);
	}

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(diffX, diffY));
	CollFixed_GteLoadR13R21(CollFixed_PackS16Pair(diffZ, centerDiffX));
	CollFixed_GteLoadR22R23(CollFixed_PackS16Pair(centerDiffY, centerDiffZ));
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(diffX, diffY));
	CollFixed_GteLoadVZ0(diffZ);
	CollFixed_GteMVMVA();

	dotSegment = CollFixed_GteReadMAC1();
	dotCenter = CollFixed_GteReadMAC2();
	CollFixed_WriteS32(spsBytes, 0x1ac, dotSegment);
	CollFixed_WriteS32(spsBytes, 0x1b0, dotCenter);

	if (dotCenter <= 0)
		return 0;

	CollFixed_GteLoadLZCS(dotCenter);
	shift = CollFixed_GteReadLZCR() - 2;
	if (shift < 0)
	{
		shift = 0;
	}
	else if (shift > 12)
	{
		shift = 12;
	}

	divisor = dotSegment >> (12 - shift);
	dotCenter = CollFixed_Sll32(dotCenter, shift);

	if (divisor < 0)
		return 0;

	factor = 0;
	if (divisor != 0)
	{
		factor = dotCenter / divisor;
	}
	CollFixed_WriteS32(spsBytes, 0x1b4, factor);

	projX = CollFixed_MulLo(factor, diffX) >> 12;
	projY = 0;
	if ((nodeBytes[0] & 0x40) != 0)
	{
		projY = CollFixed_MulLo(factor, diffY) >> 12;
	}
	projZ = CollFixed_MulLo(factor, diffZ) >> 12;

	relX = projX - centerDiffX;
	relY = projY - centerDiffY;
	relZ = projZ - centerDiffZ;
	CollFixed_WriteS32(spsBytes, 0x1dc, projX);
	CollFixed_WriteS32(spsBytes, 0x1e0, projY);
	CollFixed_WriteS32(spsBytes, 0x1e4, projZ);

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(relX, relY));
	CollFixed_GteLoadR13R21(relZ);
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(relX, relY));
	CollFixed_GteLoadVZ0(relZ);
	CollFixed_GteMVMVA();

	radius = CollFixed_ReadS16(nodeBytes, 0x16);
	radiusSquared = CollFixed_ReadS16(spsBytes, 6) + radius;
	radiusSquared *= radiusSquared;
	distSquared = CollFixed_GteReadMAC1();
	CollFixed_WriteS32(spsBytes, 0x1b8, radiusSquared);
	CollFixed_WriteS32(spsBytes, 0x1bc, distSquared);

	remaining = radiusSquared - distSquared;
	if (remaining < 0)
		return 0;

	if (remaining != 0)
	{
		if (dotSegment != 0)
		{
			factor -= CollFixed_Sll32(remaining, 12) / dotSegment;
		}
		CollFixed_WriteS32(spsBytes, 0x1c0, factor);
	}

	if (CollFixed_ReadS32(spsBytes, 0x84) < factor)
		return 0;

	hitX = 0;
	hitY = 0;
	hitZ = 0;
	if (factor > 0)
	{
		hitX = CollFixed_MulLo(diffX, factor) >> 12;
		hitY = CollFixed_MulLo(diffY, factor) >> 12;
		hitZ = CollFixed_MulLo(diffZ, factor) >> 12;
	}

	if (((nodeBytes[0] & 0x20) != 0) && (hitY < CollFixed_ReadS16(nodeBytes, 0x12)) &&
	    ((CollFixed_ReadS16(nodeBytes, 0x12) + CollFixed_ReadS16(nodeBytes, 2)) < hitY))
	{
		return 0;
	}

	*(struct BSP **)(spsBytes + 0x48) = node;
	CollFixed_WriteS32(spsBytes, 0x84, factor);
	CollFixed_WriteS16(spsBytes, 0x42, CollFixed_ReadU16(spsBytes, 0x42) + 1);
	CollFixed_WriteS32(spsBytes, 0x1e8, hitX);
	CollFixed_WriteS32(spsBytes, 0x1ec, hitY);
	CollFixed_WriteS32(spsBytes, 0x1f0, hitZ);

	normalX = hitX - centerDiffX;
	normalY = 0;
	if ((nodeBytes[0] & 0x40) != 0)
	{
		normalY = hitY - centerDiffY;
	}
	normalZ = hitZ - centerDiffZ;

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(normalX, normalY));
	CollFixed_GteLoadR13R21(normalZ);
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(normalX, normalY));
	CollFixed_GteLoadVZ0(normalZ);
	CollFixed_GteMVMVA();

	len = SquareRoot0(CollFixed_GteReadMAC1());
	invLen = 0x1000000 / len;

	normalX = CollFixed_MulLo(normalX, invLen) >> 12;
	normalY = CollFixed_MulLo(normalY, invLen) >> 12;
	normalZ = CollFixed_MulLo(normalZ, invLen) >> 12;
	CollFixed_WriteS32(spsBytes, 0x1f4, normalX);
	CollFixed_WriteS32(spsBytes, 0x1f8, normalY);
	CollFixed_WriteS32(spsBytes, 0x1fc, normalZ);

	CollFixed_WriteS16(spsBytes, 0x1c, CollFixed_ReadU16(spsBytes, 0x10) + hitX);
	CollFixed_WriteS16(spsBytes, 0x70, normalX);
	CollFixed_WriteS16(spsBytes, 0x72, normalY);
	CollFixed_WriteS16(spsBytes, 0x74, normalZ);
	CollFixed_WriteS16(spsBytes, 0x20, CollFixed_ReadU16(spsBytes, 0x14) + hitZ);
	CollFixed_WriteS16(spsBytes, 0x1e, CollFixed_ReadU16(spsBytes, 0x12) + hitY);
	spsBytes[0x7e] = 6;

	scaledX = CollFixed_MulLo(normalX, radius) >> 12;
	scaledY = CollFixed_MulLo(normalY, radius) >> 12;
	scaledZ = CollFixed_MulLo(normalZ, radius) >> 12;
	CollFixed_WriteS32(spsBytes, 0x200, scaledX);
	CollFixed_WriteS32(spsBytes, 0x204, scaledY);
	CollFixed_WriteS32(spsBytes, 0x208, scaledZ);

	CollFixed_WriteS16(spsBytes, 0x78, CollFixed_ReadU16(nodeBytes, 0x10) + scaledX);
	CollFixed_WriteS16(spsBytes, 0x68, CollFixed_ReadU16(nodeBytes, 0x10) + scaledX);
	CollFixed_WriteS16(spsBytes, 0x7a, CollFixed_ReadU16(nodeBytes, 0x12) + scaledY);
	CollFixed_WriteS16(spsBytes, 0x6a, CollFixed_ReadU16(nodeBytes, 0x12) + scaledY);
	CollFixed_WriteS16(spsBytes, 0x7c, CollFixed_ReadU16(nodeBytes, 0x14) + scaledZ);
	CollFixed_WriteS16(spsBytes, 0x6c, CollFixed_ReadU16(nodeBytes, 0x14) + scaledZ);

	return 0;
}
