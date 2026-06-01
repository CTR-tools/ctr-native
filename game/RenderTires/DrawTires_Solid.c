#include <stddef.h>
#include <stdint.h>

struct DrawTiresSolidScratch
{
	u32 savedRegs[12];
	int numPlyr;
	int playerCounter;
	struct Icon **wheelSprites;
	u32 tireColor;
	int otRangeNormal;
	int otRangeSecondary;
	u16 wheelSize;
	u16 pad4a;
	s16 vertSplit;
	u16 pad4e;
	s16 splitCameraY;
	u16 pad52;
	int lodThreshold;
	int wheelLocalPairA[4];
	int wheelLocalPairB[4];
	int wheelLocalPairC[4];
	int wheelLocalPairD[4];
	int viewNormalVectors[8];
	int transformedRimVectors[8];
	int tireAxisA[8];
	int tireAxisB[8];
	int projectedSxy[4];
	int cornerDepthBias[2];
	u32 jumpTable[8];
	u32 pad150[4];
	int instFlags;
	int pad164[2];
	s16 depthOffsetStartBytes;
	s16 depthOffsetEndBytes;
	int otRangeStart;
	int otRangeEnd;
};

_Static_assert(offsetof(struct DrawTiresSolidScratch, numPlyr) == 0x30);
_Static_assert(offsetof(struct DrawTiresSolidScratch, playerCounter) == 0x34);
_Static_assert(offsetof(struct DrawTiresSolidScratch, wheelSprites) == 0x38);
_Static_assert(offsetof(struct DrawTiresSolidScratch, tireColor) == 0x3c);
_Static_assert(offsetof(struct DrawTiresSolidScratch, otRangeNormal) == 0x40);
_Static_assert(offsetof(struct DrawTiresSolidScratch, otRangeSecondary) == 0x44);
_Static_assert(offsetof(struct DrawTiresSolidScratch, wheelSize) == 0x48);
_Static_assert(offsetof(struct DrawTiresSolidScratch, vertSplit) == 0x4c);
_Static_assert(offsetof(struct DrawTiresSolidScratch, splitCameraY) == 0x50);
_Static_assert(offsetof(struct DrawTiresSolidScratch, lodThreshold) == 0x54);
_Static_assert(offsetof(struct DrawTiresSolidScratch, wheelLocalPairA) == 0x58);
_Static_assert(offsetof(struct DrawTiresSolidScratch, wheelLocalPairB) == 0x68);
_Static_assert(offsetof(struct DrawTiresSolidScratch, wheelLocalPairC) == 0x78);
_Static_assert(offsetof(struct DrawTiresSolidScratch, wheelLocalPairD) == 0x88);
_Static_assert(offsetof(struct DrawTiresSolidScratch, viewNormalVectors) == 0x98);
_Static_assert(offsetof(struct DrawTiresSolidScratch, transformedRimVectors) == 0xb8);
_Static_assert(offsetof(struct DrawTiresSolidScratch, tireAxisA) == 0xd8);
_Static_assert(offsetof(struct DrawTiresSolidScratch, tireAxisB) == 0xf8);
_Static_assert(offsetof(struct DrawTiresSolidScratch, projectedSxy) == 0x118);
_Static_assert(offsetof(struct DrawTiresSolidScratch, cornerDepthBias) == 0x128);
_Static_assert(offsetof(struct DrawTiresSolidScratch, jumpTable) == 0x130);
_Static_assert(offsetof(struct DrawTiresSolidScratch, instFlags) == 0x160);
_Static_assert(offsetof(struct DrawTiresSolidScratch, depthOffsetStartBytes) == 0x16c);
_Static_assert(offsetof(struct DrawTiresSolidScratch, depthOffsetEndBytes) == 0x16e);
_Static_assert(offsetof(struct DrawTiresSolidScratch, otRangeStart) == 0x170);
_Static_assert(offsetof(struct DrawTiresSolidScratch, otRangeEnd) == 0x174);
_Static_assert(sizeof(POLY_FT4) == 0x28);
_Static_assert(offsetof(POLY_FT4, r0) == 0x4);
_Static_assert(offsetof(POLY_FT4, x0) == 0x8);
_Static_assert(offsetof(POLY_FT4, u0) == 0xc);
_Static_assert(offsetof(POLY_FT4, x3) == 0x20);
_Static_assert(sizeof(struct Instance) == 0x74);
_Static_assert(offsetof(struct Instance, scale) == 0x1c);
_Static_assert(offsetof(struct Instance, matrix) == 0x30);
_Static_assert(offsetof(struct Instance, vertSplit) == 0x56);
_Static_assert(sizeof(struct InstDrawPerPlayer) == 0x88);
_Static_assert(offsetof(struct InstDrawPerPlayer, pushBuffer) == 0x0);
_Static_assert(offsetof(struct InstDrawPerPlayer, m3x3) == 0x24);
_Static_assert(offsetof(struct InstDrawPerPlayer, instFlags) == 0x44);
_Static_assert(offsetof(struct InstDrawPerPlayer, lodIndex) == 0x64);
_Static_assert(offsetof(struct InstDrawPerPlayer, depthOffset) == 0x68);
_Static_assert(offsetof(struct InstDrawPerPlayer, unkE4) == 0x70);
_Static_assert(offsetof(struct InstDrawPerPlayer, unkE8) == 0x74);

static const u32 sDrawTiresSolidJumpTable[8] = {
    0x8006ed7c, 0x8006ed98, 0x8006edb4, 0x8006edcc, 0x8006ede4, 0x8006ee00, 0x8006ee1c, 0x8006ee3c,
};

static const u8 sDrawTiresSpriteIndexTable[0x81] = {
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x10, 0x10,
};

struct DrawTiresSolidTrigPair
{
	int sin;
	int cos;
};

struct DrawTiresSolidProjectedWheel
{
	struct Icon *wheelSprite;
	int selectedOT;
	int selectedOTSlot;
	int jumpIndex;
};

static s16 DrawTiresSolid_ReadS16(struct DrawTiresSolidScratch *scratch, int offset)
{
	return *(s16 *)((char *)scratch + offset);
}

static int DrawTiresSolid_ReadS32(struct DrawTiresSolidScratch *scratch, int offset)
{
	return *(int *)((char *)scratch + offset);
}

static void DrawTiresSolid_WriteS16(struct DrawTiresSolidScratch *scratch, int offset, int value)
{
	*(s16 *)((char *)scratch + offset) = value;
}

static void DrawTiresSolid_WriteS32(struct DrawTiresSolidScratch *scratch, int offset, int value)
{
	*(int *)((char *)scratch + offset) = value;
}

static int DrawTiresSolid_CountLeadingSignBits(int value)
{
	u32 bits = value;
	u32 sign = bits >> 31;
	int count = 0;

	while (count < 32 && (((bits >> (31 - count)) & 1) == sign))
	{
		count++;
	}

	return count;
}

static int DrawTiresSolid_ReadMatrixWord(MATRIX *matrix, int offset)
{
	return *(int *)((char *)matrix + offset);
}

static u32 DrawTiresSolid_PackXY(int x, int y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

static struct DrawTiresSolidTrigPair DrawTiresSolid_TrigAngleSinCos(int angle)
{
	struct DrawTiresSolidTrigPair pair;

	TRIG_AngleSinCos_r9r8r10(angle, &pair.sin, &pair.cos);

	return pair;
}

static int DrawTiresSolid_GetLodThreshold(char numPlyr)
{
	return (((int)numPlyr - 2) > 0) ? 0 : 2;
}

static struct InstDrawPerPlayer *DrawTiresSolid_GetIdpp(struct Instance *inst, int playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)INST_GETIDPP(inst) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static void DrawTiresSolid_InitScratch(struct DrawTiresSolidScratch *scratch, char numPlyr)
{
	scratch->numPlyr = numPlyr;
	scratch->lodThreshold = DrawTiresSolid_GetLodThreshold(numPlyr);

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Solid copies the
	// eight jump addresses at 0x8008a344 to scratchpad 0x1f800130. Native keeps
	// the executable-backed values as data and dispatches them through the
	// equivalent corner-order switch.
	for (int i = 0; i < 8; i++)
	{
		scratch->jumpTable[i] = sDrawTiresSolidJumpTable[i];
	}
}

static void DrawTiresSolid_AddHazardOffset(struct DrawTiresSolidScratch *scratch, int angle, int shift, int offsetY, int offsetZ)
{
	struct DrawTiresSolidTrigPair spin = DrawTiresSolid_TrigAngleSinCos(angle);

	DrawTiresSolid_WriteS16(scratch, offsetY, DrawTiresSolid_ReadS16(scratch, offsetY) + (spin.cos >> shift));
	DrawTiresSolid_WriteS16(scratch, offsetZ, DrawTiresSolid_ReadS16(scratch, offsetZ) + (spin.sin >> shift));
}

s32 Unknown_8006ef98(s32 radicand)
{
	int root = 0;
	int remainder = 0;
	int bitCount;
	u32 work;

	// NOTE(aalhendi): Retail input is s5 and output is s6.
	if (radicand == 0)
		return 0;

	bitCount = DrawTiresSolid_CountLeadingSignBits(radicand) & 0x1e;
	work = ((u32)radicand) << bitCount;

	for (bitCount ^= 0x1e; bitCount >= 0; bitCount -= 2)
	{
		int trial;

		remainder |= work >> 30;
		trial = remainder - ((root << 2) + 1);
		root <<= 1;
		work <<= 2;

		if (trial >= 0)
		{
			root++;
			remainder = trial << 2;
		}
		else
		{
			remainder <<= 2;
		}
	}

	return root;
}

static void DrawTiresSolid_BuildWheelLocalPairs(struct DrawTiresSolidScratch *scratch, struct Driver *driver, struct Instance *inst,
                                                struct InstDrawPerPlayer *idpp)
{
	int wheelX = (inst->scale[0] * 0x90) >> 12;
	int negWheelX = -wheelX;
	int wheelY;
	int wheelFrontZ;
	int wheelRearZ;
	int hazardAngle;
	int hazardShift = 9;
	struct DrawTiresSolidTrigPair steering;

	DrawTiresSolid_WriteS16(scratch, 0x58, wheelX);
	DrawTiresSolid_WriteS16(scratch, 0x78, wheelX);
	DrawTiresSolid_WriteS16(scratch, 0x68, negWheelX);
	DrawTiresSolid_WriteS16(scratch, 0x88, negWheelX);
	DrawTiresSolid_WriteS16(scratch, 0x60, wheelX - 0x1000);
	DrawTiresSolid_WriteS16(scratch, 0x80, wheelX - 0x1000);
	DrawTiresSolid_WriteS16(scratch, 0x70, negWheelX + 0x1000);
	DrawTiresSolid_WriteS16(scratch, 0x90, negWheelX + 0x1000);

	wheelY = (inst->scale[1] * 0x40) >> 12;
	DrawTiresSolid_WriteS16(scratch, 0x5a, wheelY);
	DrawTiresSolid_WriteS16(scratch, 0x62, 0);
	DrawTiresSolid_WriteS16(scratch, 0x6a, wheelY);
	DrawTiresSolid_WriteS16(scratch, 0x72, 0);
	DrawTiresSolid_WriteS16(scratch, 0x7a, wheelY);
	DrawTiresSolid_WriteS16(scratch, 0x82, 0);
	DrawTiresSolid_WriteS16(scratch, 0x8a, wheelY);
	DrawTiresSolid_WriteS16(scratch, 0x92, 0);

	wheelFrontZ = (inst->scale[2] * 0xc7) >> 12;
	DrawTiresSolid_WriteS32(scratch, 0x5c, wheelFrontZ);
	DrawTiresSolid_WriteS32(scratch, 0x6c, wheelFrontZ);

	wheelRearZ = (inst->scale[2] * -0x60) >> 12;
	DrawTiresSolid_WriteS32(scratch, 0x7c, wheelRearZ);
	DrawTiresSolid_WriteS32(scratch, 0x84, wheelRearZ);
	DrawTiresSolid_WriteS32(scratch, 0x8c, wheelRearZ);
	DrawTiresSolid_WriteS32(scratch, 0x94, wheelRearZ);

	DrawTiresSolid_WriteS32(scratch, 0x40, idpp->unkE4);
	DrawTiresSolid_WriteS32(scratch, 0x44, idpp->unkE8);
	DrawTiresSolid_WriteS32(scratch, 0x48, (s16)driver->wheelSize);

	steering = DrawTiresSolid_TrigAngleSinCos(driver->wheelRotation << 2);
	DrawTiresSolid_WriteS16(scratch, 0x60, DrawTiresSolid_ReadS16(scratch, 0x58) - steering.cos);
	DrawTiresSolid_WriteS32(scratch, 0x64, DrawTiresSolid_ReadS32(scratch, 0x5c) + steering.sin);
	DrawTiresSolid_WriteS16(scratch, 0x70, DrawTiresSolid_ReadS16(scratch, 0x68) + steering.cos);
	DrawTiresSolid_WriteS32(scratch, 0x74, DrawTiresSolid_ReadS32(scratch, 0x6c) - steering.sin);

	hazardAngle = driver->hazardTimer << 5;
	if ((driver->hazardTimer & 1) != 0)
	{
		hazardShift = 6;
		hazardAngle <<= 1;
	}

	DrawTiresSolid_AddHazardOffset(scratch, hazardAngle, hazardShift, 0x62, 0x64);
	DrawTiresSolid_AddHazardOffset(scratch, hazardAngle + 0x400, hazardShift, 0x82, 0x84);
	DrawTiresSolid_AddHazardOffset(scratch, hazardAngle + 0x800, hazardShift, 0x72, 0x74);
	DrawTiresSolid_AddHazardOffset(scratch, hazardAngle + 0xc00, hazardShift, 0x92, 0x94);
}

static void DrawTiresSolid_SetupGteState(struct DrawTiresSolidScratch *scratch, struct Instance *inst, struct InstDrawPerPlayer *idpp, struct PushBuffer *pb)
{
	int relX = (inst->matrix.t[0] - pb->pos[0]) << 2;
	int relY = (inst->matrix.t[1] - pb->pos[1]) << 2;
	int relZ = (inst->matrix.t[2] - pb->pos[2]) << 2;
	int splitCameraY = (DrawTiresSolid_ReadS16(scratch, 0x4c) - pb->pos[1]) << 2;

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Solid writes
	// instance matrix words to GTE control regs 16-20, IDPP m3x3 words to regs
	// 8-12, and camera-relative translation to regs 5-7 at
	// 0x8006e848-0x8006e8e0. Native uses explicit C parameters instead of the
	// retail a2/a3/s8/scratchpad register contract.
	CTC2(DrawTiresSolid_ReadMatrixWord(&inst->matrix, 0x0), 16);
	CTC2(DrawTiresSolid_ReadMatrixWord(&inst->matrix, 0x4), 17);
	CTC2(DrawTiresSolid_ReadMatrixWord(&inst->matrix, 0x8), 18);
	CTC2(DrawTiresSolid_ReadMatrixWord(&inst->matrix, 0xc), 19);
	CTC2(DrawTiresSolid_ReadMatrixWord(&inst->matrix, 0x10), 20);

	CTC2(DrawTiresSolid_ReadMatrixWord(&idpp->m3x3, 0x0), 8);
	CTC2(DrawTiresSolid_ReadMatrixWord(&idpp->m3x3, 0x4), 9);
	CTC2(DrawTiresSolid_ReadMatrixWord(&idpp->m3x3, 0x8), 10);
	CTC2(DrawTiresSolid_ReadMatrixWord(&idpp->m3x3, 0xc), 11);
	CTC2(DrawTiresSolid_ReadMatrixWord(&idpp->m3x3, 0x10), 12);

	CTC2(relX, 5);
	CTC2(relY, 6);
	CTC2(relZ, 7);

	DrawTiresSolid_WriteS16(scratch, 0x50, splitCameraY);
}

static void DrawTiresSolid_BuildWheelAxes(struct DrawTiresSolidScratch *scratch)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail uses s7/t8 as scratchpad
	// cursors initialized at 0x8006e8e4-0x8006e8e8, then walks four wheel records
	// backward through scratchpad. Native uses explicit offsets while preserving
	// the same GTE operations and scratch writes.
	for (int inputOffset = 0x88, outputBase = 0x18; inputOffset >= 0x58; inputOffset -= 0x10, outputBase -= 8)
	{
		int centerX;
		int centerY;
		int centerZ;
		int len;
		int invLen;
		int rimX;
		int rimY;
		int rimZ;
		int normalX;
		int normalY;
		int normalZ;
		int axisX;
		int axisY;
		int axisZ;
		int wheelSize;

		MTC2(DrawTiresSolid_ReadS32(scratch, inputOffset + 0), 0);
		MTC2(DrawTiresSolid_ReadS32(scratch, inputOffset + 4), 1);
		MTC2(DrawTiresSolid_ReadS32(scratch, inputOffset + 8), 2);
		MTC2(DrawTiresSolid_ReadS32(scratch, inputOffset + 12), 3);

		gte_lcv0tr_b();
		centerX = MFC2_S(9);
		centerY = MFC2_S(10);
		centerZ = MFC2_S(11);

		gte_sqr0_b();
		DrawTiresSolid_WriteS16(scratch, inputOffset + 0, centerX);
		DrawTiresSolid_WriteS16(scratch, inputOffset + 2, centerY);
		len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));

		gte_lcv1_b();
		DrawTiresSolid_WriteS32(scratch, inputOffset + 4, centerZ);
		invLen = -(0x10000 / len);

		rimX = MFC2_S(9);
		rimY = MFC2_S(10);
		rimZ = MFC2_S(11);

		DrawTiresSolid_WriteS16(scratch, outputBase + 0xb8, rimX);
		normalX = (centerX * invLen) >> 4;
		DrawTiresSolid_WriteS16(scratch, outputBase + 0x98, normalX);

		normalY = (centerY * invLen) >> 4;
		DrawTiresSolid_WriteS16(scratch, outputBase + 0xba, rimY);
		DrawTiresSolid_WriteS16(scratch, outputBase + 0x9a, normalY);

		normalZ = (centerZ * invLen) >> 4;
		DrawTiresSolid_WriteS32(scratch, outputBase + 0xbc, rimZ);
		DrawTiresSolid_WriteS32(scratch, outputBase + 0x9c, normalZ);

		CTC2(normalX & 0xffff, 0);
		CTC2(normalY & 0xffff, 2);
		CTC2(normalZ, 4);

		gte_op12_b();
		axisX = MFC2_S(25);
		axisY = MFC2_S(26);
		axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));
		invLen = 0x10000 / len;

		axisX = (axisX * invLen) >> 4;
		MTC2(axisX, 9);
		axisY = (axisY * invLen) >> 4;
		MTC2(axisY, 10);
		axisZ = (axisZ * invLen) >> 4;
		MTC2(axisZ, 11);

		wheelSize = DrawTiresSolid_ReadS32(scratch, 0x48);
		gte_op12_b();

		DrawTiresSolid_WriteS16(scratch, outputBase + 0xd8, (axisX * wheelSize) >> 0x12);
		DrawTiresSolid_WriteS16(scratch, outputBase + 0xda, (axisY * wheelSize) >> 0x12);
		DrawTiresSolid_WriteS16(scratch, outputBase + 0xdc, (axisZ * wheelSize) >> 0x12);

		axisX = MFC2_S(25);
		axisY = MFC2_S(26);
		axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));
		invLen = (wheelSize * -(0x10000 / len)) >> 12;

		DrawTiresSolid_WriteS16(scratch, outputBase + 0xf8, (axisX * invLen) >> 10);
		DrawTiresSolid_WriteS16(scratch, outputBase + 0xfa, (axisY * invLen) >> 10);
		DrawTiresSolid_WriteS16(scratch, outputBase + 0xfc, (axisZ * invLen) >> 10);
	}
}

static void DrawTiresSolid_SetupProjectionState(struct PushBuffer *pb)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Solid loads the
	// PushBuffer projection matrix, clears translation, and writes OFX/OFY/H at
	// 0x8006ead4-0x8006eb24 through a2-relative loads. Native uses an explicit
	// PushBuffer pointer until the retail register contract is restored.
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0x0), 0);
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0x4), 1);
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0x8), 2);
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0xc), 3);
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0x10), 4);

	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	CTC2(pb->rect.w << 0xf, 24);
	CTC2(pb->rect.h << 0xf, 25);
	CTC2(pb->distanceToScreen_PREV, 26);
}

static void DrawTiresSolid_LoadCorner(struct DrawTiresSolidScratch *scratch, int vectorIndex, int centerOffset, int outputBase, int axisSign, int rimSign)
{
	int x = DrawTiresSolid_ReadS16(scratch, centerOffset + 0) + (axisSign * DrawTiresSolid_ReadS16(scratch, outputBase + 0xd8)) +
	        (rimSign * DrawTiresSolid_ReadS16(scratch, outputBase + 0xf8));
	int y = DrawTiresSolid_ReadS16(scratch, centerOffset + 2) + (axisSign * DrawTiresSolid_ReadS16(scratch, outputBase + 0xda)) +
	        (rimSign * DrawTiresSolid_ReadS16(scratch, outputBase + 0xfa));
	int z = DrawTiresSolid_ReadS16(scratch, centerOffset + 4) + (axisSign * DrawTiresSolid_ReadS16(scratch, outputBase + 0xdc)) +
	        (rimSign * DrawTiresSolid_ReadS16(scratch, outputBase + 0xfc));

	MTC2(DrawTiresSolid_PackXY(x, y), vectorIndex * 2);
	MTC2(z, (vectorIndex * 2) + 1);
}

static int DrawTiresSolid_SelectSpriteIndex(int angleValue)
{
	int tableIndex = angleValue >> 5;

	if (angleValue < 0)
		tableIndex = -tableIndex;

	if ((tableIndex - 0x80) > 0)
		tableIndex = 0x80;

	return sDrawTiresSpriteIndexTable[tableIndex];
}

static struct DrawTiresSolidProjectedWheel DrawTiresSolid_SelectProjectedWheel(struct DrawTiresSolidScratch *scratch, int centerOffset, int outputBase,
                                                                               int wheelIndex)
{
	struct DrawTiresSolidProjectedWheel selected;
	int selectedOT = DrawTiresSolid_ReadS32(scratch, 0x40);
	int splitDelta = DrawTiresSolid_ReadS16(scratch, 0x50) - DrawTiresSolid_ReadS16(scratch, centerOffset + 2);
	u32 depthValue;
	int angleValue;
	int spriteIndex;

	selected.wheelSprite = 0;
	selected.selectedOT = selectedOT;
	selected.selectedOTSlot = 0;
	selected.jumpIndex = wheelIndex;

	gte_avsz4_b();

	if (splitDelta >= 0)
		selectedOT = DrawTiresSolid_ReadS32(scratch, 0x44);

	MTC2(DrawTiresSolid_ReadS32(scratch, outputBase + 0x98), 0);
	MTC2(DrawTiresSolid_ReadS32(scratch, outputBase + 0x9c), 1);
	CTC2(DrawTiresSolid_ReadS32(scratch, outputBase + 0xb8), 8);
	CTC2(DrawTiresSolid_ReadS32(scratch, outputBase + 0xbc), 9);

	DrawTiresSolid_WriteS32(scratch, 0x170, DrawTiresSolid_ReadS16(scratch, 0x16c) + selectedOT);
	DrawTiresSolid_WriteS32(scratch, 0x174, DrawTiresSolid_ReadS16(scratch, 0x16e) + selectedOT);

	gte_llv0_b();
	depthValue = MFC2(24);
	selected.selectedOT = selectedOT;
	selected.selectedOTSlot = selectedOT + (int)((depthValue >> 0x11) << 2);
	angleValue = MFC2_S(9);
	spriteIndex = DrawTiresSolid_SelectSpriteIndex(angleValue);
	if (angleValue < 0)
		selected.jumpIndex += 4;

	selected.wheelSprite = scratch->wheelSprites[spriteIndex];

	return selected;
}

static void DrawTiresSolid_CopyIconUV(POLY_FT4 *p, struct Icon *icon)
{
	u32 uv23 = *(u32 *)&icon->texLayout.u2;

	*(u32 *)&p->u0 = *(u32 *)&icon->texLayout.u0;
	*(u32 *)&p->u1 = *(u32 *)&icon->texLayout.u1;
	*(u32 *)&p->u2 = uv23;
	*(u32 *)&p->u3 = uv23 >> 16;
}

static int DrawTiresSolid_ApplyCornerOrder(struct DrawTiresSolidScratch *scratch, int jumpIndex, int *selectedOTSlot, int sxy[4])
{
	switch (jumpIndex)
	{
	case 0:
		sxy[0] = DrawTiresSolid_ReadS32(scratch, 0x124);
		sxy[1] = DrawTiresSolid_ReadS32(scratch, 0x11c);
		sxy[2] = DrawTiresSolid_ReadS32(scratch, 0x120);
		sxy[3] = DrawTiresSolid_ReadS32(scratch, 0x118);
		*selectedOTSlot -= DrawTiresSolid_ReadS32(scratch, 0x128);
		break;
	case 1:
		sxy[0] = DrawTiresSolid_ReadS32(scratch, 0x120);
		sxy[1] = DrawTiresSolid_ReadS32(scratch, 0x118);
		sxy[2] = DrawTiresSolid_ReadS32(scratch, 0x124);
		sxy[3] = DrawTiresSolid_ReadS32(scratch, 0x11c);
		*selectedOTSlot -= DrawTiresSolid_ReadS32(scratch, 0x12c);
		break;
	case 2:
		sxy[0] = DrawTiresSolid_ReadS32(scratch, 0x124);
		sxy[1] = DrawTiresSolid_ReadS32(scratch, 0x11c);
		sxy[2] = DrawTiresSolid_ReadS32(scratch, 0x120);
		sxy[3] = DrawTiresSolid_ReadS32(scratch, 0x118);
		DrawTiresSolid_WriteS32(scratch, 0x128, 0);
		break;
	case 3:
		sxy[0] = DrawTiresSolid_ReadS32(scratch, 0x120);
		sxy[1] = DrawTiresSolid_ReadS32(scratch, 0x118);
		sxy[2] = DrawTiresSolid_ReadS32(scratch, 0x124);
		sxy[3] = DrawTiresSolid_ReadS32(scratch, 0x11c);
		DrawTiresSolid_WriteS32(scratch, 0x12c, 0);
		break;
	case 4:
		sxy[0] = DrawTiresSolid_ReadS32(scratch, 0x11c);
		sxy[1] = DrawTiresSolid_ReadS32(scratch, 0x124);
		sxy[2] = DrawTiresSolid_ReadS32(scratch, 0x118);
		sxy[3] = DrawTiresSolid_ReadS32(scratch, 0x120);
		*selectedOTSlot -= DrawTiresSolid_ReadS32(scratch, 0x128);
		break;
	case 5:
		sxy[0] = DrawTiresSolid_ReadS32(scratch, 0x118);
		sxy[1] = DrawTiresSolid_ReadS32(scratch, 0x120);
		sxy[2] = DrawTiresSolid_ReadS32(scratch, 0x11c);
		sxy[3] = DrawTiresSolid_ReadS32(scratch, 0x124);
		*selectedOTSlot -= DrawTiresSolid_ReadS32(scratch, 0x12c);
		break;
	case 6:
		sxy[0] = DrawTiresSolid_ReadS32(scratch, 0x11c);
		sxy[1] = DrawTiresSolid_ReadS32(scratch, 0x124);
		sxy[2] = DrawTiresSolid_ReadS32(scratch, 0x118);
		sxy[3] = DrawTiresSolid_ReadS32(scratch, 0x120);
		*selectedOTSlot -= 8;
		DrawTiresSolid_WriteS32(scratch, 0x128, 8);
		break;
	case 7:
		sxy[0] = DrawTiresSolid_ReadS32(scratch, 0x118);
		sxy[1] = DrawTiresSolid_ReadS32(scratch, 0x120);
		sxy[2] = DrawTiresSolid_ReadS32(scratch, 0x11c);
		sxy[3] = DrawTiresSolid_ReadS32(scratch, 0x124);
		*selectedOTSlot -= 8;
		DrawTiresSolid_WriteS32(scratch, 0x12c, 8);
		break;
	default:
		return 0;
	}

	return 1;
}

static void DrawTiresSolid_WritePrimitiveCorners(POLY_FT4 *p, int sxy[4])
{
	*(u32 *)&p->x0 = sxy[0];
	*(u32 *)&p->x1 = sxy[1];
	*(u32 *)&p->x2 = sxy[2];
	*(u32 *)&p->x3 = sxy[3];
}

static void DrawTiresSolid_LinkPrimitive(struct DrawTiresSolidScratch *scratch, POLY_FT4 *p, int selectedOTSlot)
{
	u_long *otSlot;
	int otRangeStart = DrawTiresSolid_ReadS32(scratch, 0x170);
	int otRangeEnd = DrawTiresSolid_ReadS32(scratch, 0x174);

	if ((otRangeStart - selectedOTSlot) > 0)
		selectedOTSlot = otRangeStart;

	if ((otRangeEnd - selectedOTSlot) < 0)
		selectedOTSlot = otRangeEnd;

	otSlot = (u_long *)(uintptr_t)selectedOTSlot;
	p->tag = *otSlot | 0x09000000;
#ifdef CTR_NATIVE
	// NOTE(aalhendi): PSX-backfeed blocker: retail stores a 24-bit primitive
	// address in the OT tag. ctr-native's PC renderer uses full 32-bit host
	// pointers like addPolyFT4/AddPrim, so only the native write is widened.
	*otSlot = (u_long)(uintptr_t)p;
#else
	*otSlot = (u_long)CtrGpu_PrimToOTLink24(p);
#endif
}

static int DrawTiresSolid_EmitProjectedWheel(struct DrawTiresSolidScratch *scratch, struct DrawTiresSolidProjectedWheel *selected, struct PrimMem *primMem,
                                             int *primCount)
{
	POLY_FT4 *p = (POLY_FT4 *)primMem->curr;
	int selectedOTSlot = selected->selectedOTSlot;
	int sxy[4];

	*(u32 *)&p->r0 = scratch->tireColor;

	if (selected->wheelSprite == 0)
		return 1;

	DrawTiresSolid_CopyIconUV(p, selected->wheelSprite);

#ifdef CTR_INTERNAL
	if (CtrTireDebug_ShouldLog(CTR_TIREDBG_SOLID_PRIM) != 0)
	{
		fprintf(stderr, "[TIREDBG][solid-prim] color=%08x code=%02x rgb=%02x,%02x,%02x tpage=%04x blend=%d clut=%04x jump=%d ot=%08x flags=%08x\n",
		        scratch->tireColor, p->code, p->r0, p->g0, p->b0, p->tpage, (p->tpage >> 5) & 3, p->clut, selected->jumpIndex, selected->selectedOTSlot,
		        scratch->instFlags);
	}
#endif

	if (DrawTiresSolid_ApplyCornerOrder(scratch, selected->jumpIndex, &selectedOTSlot, sxy) == 0)
		return 1;

	if (DrawTiresSolid_ReadS32(scratch, 0x44) == selected->selectedOT && (scratch->instFlags & 0x4000) != 0)
		return 0;

	DrawTiresSolid_WritePrimitiveCorners(p, sxy);
	DrawTiresSolid_LinkPrimitive(scratch, p, selectedOTSlot);
	primMem->curr = (char *)primMem->curr + sizeof(POLY_FT4);
	(*primCount)++;

	return 1;
}

static int DrawTiresSolid_ProjectWheelQuads(struct DrawTiresSolidScratch *scratch, struct PrimMem *primMem, int *primCount)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Solid walks the
	// projection loop with s7/t8/t9 scratchpad cursors from 0x8006eb34 onward.
	// Native uses explicit scratch offsets while preserving corner ordering,
	// depth-bias scratch updates, UV copy, and OT linking.
	for (int centerOffset = 0x88, outputBase = 0x18, wheelIndex = 3; centerOffset >= 0x58; centerOffset -= 0x10, outputBase -= 8, wheelIndex--)
	{
		struct DrawTiresSolidProjectedWheel projectedWheel;

		DrawTiresSolid_LoadCorner(scratch, 0, centerOffset, outputBase, -1, -1);
		DrawTiresSolid_LoadCorner(scratch, 1, centerOffset, outputBase, 1, -1);
		DrawTiresSolid_LoadCorner(scratch, 2, centerOffset, outputBase, -1, 1);

		gte_rtpt_b();
		DrawTiresSolid_WriteS32(scratch, 0x118, MFC2(12));
		DrawTiresSolid_WriteS32(scratch, 0x11c, MFC2(13));
		DrawTiresSolid_WriteS32(scratch, 0x120, MFC2(14));

		DrawTiresSolid_LoadCorner(scratch, 0, centerOffset, outputBase, 1, 1);
		gte_rtps_b();
		DrawTiresSolid_WriteS32(scratch, 0x124, MFC2(14));

		projectedWheel = DrawTiresSolid_SelectProjectedWheel(scratch, centerOffset, outputBase, wheelIndex);

		if (DrawTiresSolid_EmitProjectedWheel(scratch, &projectedWheel, primMem, primCount) == 0)
			return 0;
	}

	return 1;
}

static int DrawTiresSolid_StagePlayer(struct DrawTiresSolidScratch *scratch, struct Driver *driver, struct Instance *inst, int playerIndex,
                                      struct PrimMem *primMem, int *primCount)
{
	struct InstDrawPerPlayer *idpp = DrawTiresSolid_GetIdpp(inst, playerIndex);
	struct PushBuffer *pb = idpp->pushBuffer;
	int flags = idpp->instFlags;

	scratch->playerCounter = scratch->numPlyr - playerIndex;
	scratch->vertSplit = inst->vertSplit;
	scratch->depthOffsetStartBytes = idpp->depthOffset[0] << 2;
	scratch->depthOffsetEndBytes = idpp->depthOffset[1] << 2;
	scratch->instFlags = flags;

	if ((flags & DRAW_SUCCESSFUL) == 0)
		return 0;

	if ((flags & HIDE_MODEL) != 0)
		return 0;

	if ((idpp->lodIndex - scratch->lodThreshold) > 0)
		return 0;

	scratch->wheelSprites = driver->wheelSprites;
	scratch->tireColor = ((flags & PUSHBUFFER_EXISTS) != 0) ? 0x2e808080 : driver->tireColor;

	if (pb == 0)
		return 0;

#ifdef CTR_INTERNAL
	if (CtrTireDebug_ShouldLog(CTR_TIREDBG_SOLID_STAGE) != 0)
	{
		struct GameTracker *gGT = sdata->gGT;
		fprintf(stderr,
		        "[TIREDBG][solid-stage] frame=%d level=%d mode=%08x player=%d inst=%p driver=%p flags=%08x push=%d refl=%d "
		        "lod=%d/%d tire=%08x driverTire=%08x wheelSize=%d pb=%p pbSize=%dx%d\n",
		        gGT != 0 ? gGT->framesInThisLEV : -1, gGT != 0 ? gGT->levelID : -1, gGT != 0 ? gGT->gameMode1 : 0, playerIndex, (void *)inst, (void *)driver,
		        flags, (flags & PUSHBUFFER_EXISTS) != 0, (flags & REFLECTIVE) != 0, idpp->lodIndex, scratch->lodThreshold, scratch->tireColor,
		        driver->tireColor, driver->wheelSize, (void *)pb, pb->rect.w, pb->rect.h);
	}
#endif

	scratch->wheelSize = driver->wheelSize;
	scratch->otRangeNormal = idpp->unkE4;
	scratch->otRangeSecondary = idpp->unkE8;
	scratch->otRangeStart = scratch->depthOffsetStartBytes + idpp->unkE4;
	scratch->otRangeEnd = scratch->depthOffsetEndBytes + idpp->unkE4;

	DrawTiresSolid_BuildWheelLocalPairs(scratch, driver, inst, idpp);
	DrawTiresSolid_SetupGteState(scratch, inst, idpp, pb);
	DrawTiresSolid_BuildWheelAxes(scratch);
	DrawTiresSolid_SetupProjectionState(pb);
	DrawTiresSolid_ProjectWheelQuads(scratch, primMem, primCount);

	return 1;
}

void DrawTires_Solid(struct Thread *thread, struct PrimMem *primMem, char numPlyr)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006e588-0x8006ef30;
	// native uses the accepted explicit DrawTiresSolidScratch stack/scratch ABI.
	struct DrawTiresSolidScratch scratch = {0};
	int primCount;

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Solid owns scratchpad
	// 0x1f800000 and live s7/t8/t9/v0/s0 register cursors. Native uses
	// offset-checked stack state and explicit helpers; PSX backfeed must restore
	// the retail scratchpad/register entry protocol.
	if (primMem == 0)
		return;

	primCount = primMem->unk1;
	DrawTiresSolid_InitScratch(&scratch, numPlyr);

	for (struct Thread *currThread = thread; currThread != 0; currThread = currThread->siblingThread)
	{
		struct Driver *driver = (struct Driver *)currThread->object;
		struct Instance *inst = currThread->inst;

		if (driver == 0 || inst == 0)
			continue;

		for (int playerIndex = 0; playerIndex < (int)(u8)numPlyr; playerIndex++)
		{
			if (DrawTiresSolid_StagePlayer(&scratch, driver, inst, playerIndex, primMem, &primCount) == 0)
				continue;
		}
	}

	primMem->unk1 = primCount;
}
