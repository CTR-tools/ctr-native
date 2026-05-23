#include <common.h>

// NOTE(aalhendi): Ghidra-assisted native port of retail 0x80042e50; not ASM-stamped yet.

static void PushBuffer_SetFrustumPlane_LoadAxisVector(int x, int y, int z)
{
	CTC2((u32)(s32)x, 0);
	CTC2((u32)(s32)y, 2);
	CTC2((u32)(s32)z, 4);
}

static void PushBuffer_SetFrustumPlane_LoadIRVector(int x, int y, int z)
{
	MTC2_S(x, 9);
	MTC2_S(y, 10);
	MTC2_S(z, 11);
}

static int PushBuffer_SetFrustumPlane_ReadLeadingZeroes(u32 value)
{
	MTC2(value, 30);
	return MFC2_S(31);
}

static s32 PushBuffer_SetFrustumPlane_Abs(s32 value)
{
	return (value < 0) ? -value : value;
}

int PushBuffer_SetFrustumPlane(s16 *frustumData, struct FrustumCornerOUT *fc1, s16 *camPos, struct FrustumCornerOUT *fc2)
{
	int leadingZeroBits;
	int temp;
	s32 normalX;
	s32 normalY;
	s32 normalZ;
	int cameraPosX = camPos[0];
	int cameraPosY = camPos[1];
	int cameraPosZ = camPos[2];

	PushBuffer_SetFrustumPlane_LoadAxisVector(fc2->pos[0] - cameraPosX, fc2->pos[1] - cameraPosY, fc2->pos[2] - cameraPosZ);
	PushBuffer_SetFrustumPlane_LoadIRVector(fc1->pos[0] - cameraPosX, fc1->pos[1] - cameraPosY, fc1->pos[2] - cameraPosZ);

	gte_op0();

	normalX = MFC2_S(25);
	normalY = MFC2_S(26);
	normalZ = MFC2_S(27);

	leadingZeroBits = PushBuffer_SetFrustumPlane_ReadLeadingZeroes((u32)PushBuffer_SetFrustumPlane_Abs(normalX));

	temp = PushBuffer_SetFrustumPlane_ReadLeadingZeroes((u32)PushBuffer_SetFrustumPlane_Abs(normalY));
	if (temp < leadingZeroBits)
		leadingZeroBits = temp;

	temp = PushBuffer_SetFrustumPlane_ReadLeadingZeroes((u32)PushBuffer_SetFrustumPlane_Abs(normalZ));
	if (temp < leadingZeroBits)
		leadingZeroBits = temp;

	if (leadingZeroBits < 0x12)
	{
		int vecBitShift = 0x12 - leadingZeroBits;
		normalX >>= vecBitShift & 0x1f;
		normalY >>= vecBitShift & 0x1f;
		normalZ >>= vecBitShift & 0x1f;
	}

	int length = SquareRoot0_stub(normalX * normalX + normalY * normalY + normalZ * normalZ);
	if (length != 0)
	{
		normalX = (normalX << 12) / length;
		normalY = (normalY << 12) / length;
		normalZ = (normalZ << 12) / length;
	}

	int planeD = (normalX * cameraPosX + normalY * cameraPosY + normalZ * cameraPosZ) >> 13;

	frustumData[0] = (s16)normalX;
	frustumData[1] = (s16)normalY;
	frustumData[2] = (s16)normalZ;
	frustumData[3] = (s16)planeD;

	u32 planeType = (u32)normalX >> 31;
	if (normalY < 0)
		planeType |= 2;
	if (normalZ < 0)
		planeType |= 4;

	return planeType;
}
