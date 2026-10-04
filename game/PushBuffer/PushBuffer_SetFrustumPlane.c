#include <common.h>
#include <ctr_gte_transfer.h>


static inline s32 PushBuffer_SetFrustumPlane_Abs(s32 value)
{
	if (value < 0)
	{
		CTR_PSX_KEEP_VALUE_RELAXED(value);
		value = -value;
	}
	return value;
}

s32 PushBuffer_SetFrustumPlane(struct PushBufferFrustumPlane *frustumPlane, struct FrustumCornerOUT *fc1, const SVec3 *camPos, struct FrustumCornerOUT *fc2)
{
	s32 leadingZeroBits;
	s32 temp;
	s32 normalX;
	s32 normalY;
	s32 normalZ;
	s32 length;
	s32 planeD;
	u32 planeType;
	register s32 result CTR_PSX_REGISTER("$2");
	s32 cameraPosX = camPos->x;
	s32 cameraPosY = camPos->y;
	s32 cameraPosZ = camPos->z;

	CTC2_S(fc2->pos.x - cameraPosX, 0);
	CTC2_S(fc2->pos.y - cameraPosY, 2);
	CTC2_S(fc2->pos.z - cameraPosZ, 4);
	MTC2_S(fc1->pos.x - cameraPosX, 9);
	MTC2_S(fc1->pos.y - cameraPosY, 10);
	MTC2_S(fc1->pos.z - cameraPosZ, 11);
	CTR_PSX_GTE_PIPELINE_DELAY();

	gte_op0();

	normalX = MFC2_S(25);
	normalY = MFC2_S(26);
	normalZ = MFC2_S(27);

	MTC2((u32)PushBuffer_SetFrustumPlane_Abs(normalX), 30);
	CTR_PSX_GTE_PIPELINE_DELAY();
	leadingZeroBits = MFC2_S(31);

	MTC2((u32)PushBuffer_SetFrustumPlane_Abs(normalY), 30);
	CTR_PSX_GTE_PIPELINE_DELAY();
	CTR_GteReadDataDelayed(temp, 31);
	if (temp < leadingZeroBits)
	{
		leadingZeroBits = temp;
	}

	MTC2((u32)PushBuffer_SetFrustumPlane_Abs(normalZ), 30);
	CTR_PSX_GTE_PIPELINE_DELAY();
	CTR_GteReadDataDelayed(temp, 31);
	if (temp < leadingZeroBits)
	{
		leadingZeroBits = temp;
	}

	if (leadingZeroBits < 0x12)
	{
		leadingZeroBits = 0x12 - leadingZeroBits;
		normalX >>= leadingZeroBits;
		normalY >>= leadingZeroBits;
		normalZ >>= leadingZeroBits;
	}

	length = SquareRoot0_stub(normalX * normalX + normalY * normalY + normalZ * normalZ);
	if (length != 0)
	{
		// NOTE(aalhendi): CLZ reduction bounds components to [-16384, 16383]
		// and their squared sum fits s32. Multiplication keeps negative scaling
		// defined on native C while retaining retail's three shifts.
		normalX = (normalX * 4096) / length;
		normalY = (normalY * 4096) / length;
		normalZ = (normalZ * 4096) / length;
	}

	planeD = (normalX * cameraPosX + normalY * cameraPosY + normalZ * cameraPosZ) >> 13;

	frustumPlane->normal.x = (s16)normalX;
	frustumPlane->normal.y = (s16)normalY;
	frustumPlane->normal.z = (s16)normalZ;
	frustumPlane->halfDistance = (s16)planeD;

	planeType = (u32)normalX >> 31;
	if (normalY < 0)
	{
		planeType |= 2;
	}
	if (normalZ < 0)
	{
		// NOTE(aalhendi): Retail forms the final flag in the ordinary ABI return
		// register; this binding adds no incoming or caller-preserved register.
		result = planeType;
		CTR_PSX_KEEP_VALUE_RELAXED(result);
		result |= 4;
		return result;
	}

	return planeType;
}
