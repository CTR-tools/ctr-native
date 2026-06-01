#include <stddef.h>
#include <stdint.h>

struct DrawTiresReflectionScratch
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

_Static_assert(offsetof(struct DrawTiresReflectionScratch, numPlyr) == 0x30);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, playerCounter) == 0x34);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, wheelSprites) == 0x38);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, tireColor) == 0x3c);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, otRangeNormal) == 0x40);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, otRangeSecondary) == 0x44);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, wheelSize) == 0x48);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, vertSplit) == 0x4c);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, splitCameraY) == 0x50);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, lodThreshold) == 0x54);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, wheelLocalPairA) == 0x58);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, wheelLocalPairB) == 0x68);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, wheelLocalPairC) == 0x78);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, wheelLocalPairD) == 0x88);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, viewNormalVectors) == 0x98);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, transformedRimVectors) == 0xb8);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, tireAxisA) == 0xd8);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, tireAxisB) == 0xf8);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, projectedSxy) == 0x118);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, cornerDepthBias) == 0x128);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, jumpTable) == 0x130);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, instFlags) == 0x160);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, depthOffsetStartBytes) == 0x16c);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, depthOffsetEndBytes) == 0x16e);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, otRangeStart) == 0x170);
_Static_assert(offsetof(struct DrawTiresReflectionScratch, otRangeEnd) == 0x174);
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

static const u32 sDrawTiresReflectionJumpTable[8] = {
    0x8006f7f8, 0x8006f814, 0x8006f830, 0x8006f848, 0x8006f860, 0x8006f87c, 0x8006f898, 0x8006f8b8,
};

struct DrawTiresReflectionProjectedWheel
{
	struct Icon *wheelSprite;
	int selectedOT;
	int selectedOTSlot;
	int jumpIndex;
};

static int DrawTiresReflection_GetLodThreshold(char numPlyr)
{
	return (((int)numPlyr - 2) > 0) ? 0 : 2;
}

static struct InstDrawPerPlayer *DrawTiresReflection_GetIdpp(struct Instance *inst, int playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)INST_GETIDPP(inst) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static s16 DrawTiresReflection_ReadS16(struct DrawTiresReflectionScratch *scratch, int offset)
{
	return *(s16 *)((char *)scratch + offset);
}

static int DrawTiresReflection_ReadS32(struct DrawTiresReflectionScratch *scratch, int offset)
{
	return *(int *)((char *)scratch + offset);
}

static void DrawTiresReflection_WriteS16(struct DrawTiresReflectionScratch *scratch, int offset, int value)
{
	*(s16 *)((char *)scratch + offset) = value;
}

static void DrawTiresReflection_WriteS32(struct DrawTiresReflectionScratch *scratch, int offset, int value)
{
	*(int *)((char *)scratch + offset) = value;
}

static int DrawTiresReflection_ReadMatrixWord(MATRIX *matrix, int offset)
{
	return *(int *)((char *)matrix + offset);
}

static u32 DrawTiresReflection_PackXY(int x, int y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

static void DrawTiresReflection_InitScratch(struct DrawTiresReflectionScratch *scratch, char numPlyr)
{
	scratch->numPlyr = numPlyr;
	scratch->lodThreshold = DrawTiresReflection_GetLodThreshold(numPlyr);

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Reflection copies
	// the eight jump addresses at 0x8008a364 to scratchpad 0x1f800130. Native
	// keeps the executable-backed values as data and dispatches them through the
	// equivalent reflected corner-order switch.
	for (int i = 0; i < 8; i++)
	{
		scratch->jumpTable[i] = sDrawTiresReflectionJumpTable[i];
	}
}

static void DrawTiresReflection_AddHazardOffset(struct DrawTiresReflectionScratch *scratch, int angle, int shift, int offsetY, int offsetZ)
{
	struct DrawTiresSolidTrigPair spin = DrawTiresSolid_TrigAngleSinCos(angle);

	DrawTiresReflection_WriteS16(scratch, offsetY, DrawTiresReflection_ReadS16(scratch, offsetY) + (spin.cos >> shift));
	DrawTiresReflection_WriteS16(scratch, offsetZ, DrawTiresReflection_ReadS16(scratch, offsetZ) + (spin.sin >> shift));
}

static void DrawTiresReflection_BuildWheelLocalPairs(struct DrawTiresReflectionScratch *scratch, struct Driver *driver, struct Instance *inst,
                                                     struct InstDrawPerPlayer *idpp)
{
	int wheelX;
	int negWheelX;
	int wheelY;
	int wheelFrontZ;
	int wheelRearZ;
	int hazardAngle;
	int hazardShift = 9;
	struct DrawTiresSolidTrigPair steering;

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Reflection loads
	// Instance.scale[0] at 0x8006f0e0, then the push-buffer branch delay slot at
	// 0x8006f0e8 overwrites v1 with instFlags & 0x100 before the multiply at
	// 0x8006f10c. Native makes that register lineage explicit until the exact
	// delay-slot/register ABI is restored for PSX backfeed.
	wheelX = ((scratch->instFlags & PUSHBUFFER_EXISTS) * 0x90) >> 12;
	negWheelX = -wheelX;

	DrawTiresReflection_WriteS16(scratch, 0x58, wheelX);
	DrawTiresReflection_WriteS16(scratch, 0x78, wheelX);
	DrawTiresReflection_WriteS16(scratch, 0x68, negWheelX);
	DrawTiresReflection_WriteS16(scratch, 0x88, negWheelX);
	DrawTiresReflection_WriteS16(scratch, 0x60, wheelX - 0x1000);
	DrawTiresReflection_WriteS16(scratch, 0x80, wheelX - 0x1000);
	DrawTiresReflection_WriteS16(scratch, 0x70, negWheelX + 0x1000);
	DrawTiresReflection_WriteS16(scratch, 0x90, negWheelX + 0x1000);

	wheelY = (inst->scale[1] * 0x40) >> 12;
	DrawTiresReflection_WriteS16(scratch, 0x5a, wheelY);
	DrawTiresReflection_WriteS16(scratch, 0x62, 0);
	DrawTiresReflection_WriteS16(scratch, 0x6a, wheelY);
	DrawTiresReflection_WriteS16(scratch, 0x72, 0);
	DrawTiresReflection_WriteS16(scratch, 0x7a, wheelY);
	DrawTiresReflection_WriteS16(scratch, 0x82, 0);
	DrawTiresReflection_WriteS16(scratch, 0x8a, wheelY);
	DrawTiresReflection_WriteS16(scratch, 0x92, 0);

	wheelFrontZ = (inst->scale[2] * 0xc7) >> 12;
	DrawTiresReflection_WriteS32(scratch, 0x5c, wheelFrontZ);
	DrawTiresReflection_WriteS32(scratch, 0x6c, wheelFrontZ);

	wheelRearZ = (inst->scale[2] * -0x60) >> 12;
	DrawTiresReflection_WriteS32(scratch, 0x7c, wheelRearZ);
	DrawTiresReflection_WriteS32(scratch, 0x84, wheelRearZ);
	DrawTiresReflection_WriteS32(scratch, 0x8c, wheelRearZ);
	DrawTiresReflection_WriteS32(scratch, 0x94, wheelRearZ);

	DrawTiresReflection_WriteS32(scratch, 0x40, idpp->unkE4);
	DrawTiresReflection_WriteS32(scratch, 0x44, idpp->unkE8);
	DrawTiresReflection_WriteS32(scratch, 0x48, (s16)driver->wheelSize);

	steering = DrawTiresSolid_TrigAngleSinCos(driver->wheelRotation << 2);
	DrawTiresReflection_WriteS16(scratch, 0x60, DrawTiresReflection_ReadS16(scratch, 0x58) - steering.cos);
	DrawTiresReflection_WriteS32(scratch, 0x64, DrawTiresReflection_ReadS32(scratch, 0x5c) + steering.sin);
	DrawTiresReflection_WriteS16(scratch, 0x70, DrawTiresReflection_ReadS16(scratch, 0x68) + steering.cos);
	DrawTiresReflection_WriteS32(scratch, 0x74, DrawTiresReflection_ReadS32(scratch, 0x6c) - steering.sin);

	hazardAngle = driver->hazardTimer << 5;
	if ((driver->hazardTimer & 1) != 0)
	{
		hazardShift = 6;
		hazardAngle <<= 1;
	}

	DrawTiresReflection_AddHazardOffset(scratch, hazardAngle, hazardShift, 0x62, 0x64);
	DrawTiresReflection_AddHazardOffset(scratch, hazardAngle + 0x400, hazardShift, 0x82, 0x84);
	DrawTiresReflection_AddHazardOffset(scratch, hazardAngle + 0x800, hazardShift, 0x72, 0x74);
	DrawTiresReflection_AddHazardOffset(scratch, hazardAngle + 0xc00, hazardShift, 0x92, 0x94);
}

static void DrawTiresReflection_SetupGteState(struct DrawTiresReflectionScratch *scratch, struct Instance *inst, struct InstDrawPerPlayer *idpp,
                                              struct PushBuffer *pb)
{
	int relX = (inst->matrix.t[0] - pb->pos[0]) << 2;
	int relY = (inst->matrix.t[1] - pb->pos[1]) << 2;
	int relZ = (inst->matrix.t[2] - pb->pos[2]) << 2;
	int splitCameraY = (DrawTiresReflection_ReadS16(scratch, 0x4c) - pb->pos[1]) << 2;

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Reflection writes
	// instance matrix words to GTE control regs 16-20, IDPP m3x3 words to regs
	// 8-12, and camera-relative translation to regs 5-7 at
	// 0x8006f2c4-0x8006f35c. Native uses explicit C parameters instead of the
	// retail a2/a3/s8/scratchpad register contract.
	CTC2(DrawTiresReflection_ReadMatrixWord(&inst->matrix, 0x0), 16);
	CTC2(DrawTiresReflection_ReadMatrixWord(&inst->matrix, 0x4), 17);
	CTC2(DrawTiresReflection_ReadMatrixWord(&inst->matrix, 0x8), 18);
	CTC2(DrawTiresReflection_ReadMatrixWord(&inst->matrix, 0xc), 19);
	CTC2(DrawTiresReflection_ReadMatrixWord(&inst->matrix, 0x10), 20);

	CTC2(DrawTiresReflection_ReadMatrixWord(&idpp->m3x3, 0x0), 8);
	CTC2(DrawTiresReflection_ReadMatrixWord(&idpp->m3x3, 0x4), 9);
	CTC2(DrawTiresReflection_ReadMatrixWord(&idpp->m3x3, 0x8), 10);
	CTC2(DrawTiresReflection_ReadMatrixWord(&idpp->m3x3, 0xc), 11);
	CTC2(DrawTiresReflection_ReadMatrixWord(&idpp->m3x3, 0x10), 12);

	CTC2(relX, 5);
	CTC2(relY, 6);
	CTC2(relZ, 7);

	DrawTiresReflection_WriteS16(scratch, 0x50, splitCameraY);
}

static void DrawTiresReflection_BuildWheelAxes(struct DrawTiresReflectionScratch *scratch)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail uses s7/t8 as scratchpad
	// cursors initialized at 0x8006f360-0x8006f364, then walks four wheel
	// records backward through scratchpad. Native uses explicit offsets while
	// preserving the reflected Y mirror and GTE operation sequence.
	for (int inputOffset = 0x88, outputBase = 0x18; inputOffset >= 0x58; inputOffset -= 0x10, outputBase -= 8)
	{
		int centerX;
		int originalCenterY;
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
		int splitCameraY = DrawTiresReflection_ReadS16(scratch, 0x50);

		MTC2(DrawTiresReflection_ReadS32(scratch, inputOffset + 0), 0);
		MTC2(DrawTiresReflection_ReadS32(scratch, inputOffset + 4), 1);
		MTC2(DrawTiresReflection_ReadS32(scratch, inputOffset + 8), 2);
		MTC2(DrawTiresReflection_ReadS32(scratch, inputOffset + 12), 3);

		gte_lcv0tr_b();
		centerX = MFC2_S(9);
		originalCenterY = MFC2_S(10);
		centerZ = MFC2_S(11);
		centerY = splitCameraY - (originalCenterY - splitCameraY);

		gte_sqr0_b();
		DrawTiresReflection_WriteS16(scratch, inputOffset + 0, centerX);
		DrawTiresReflection_WriteS16(scratch, inputOffset + 2, centerY);
		len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));

		gte_lcv1_b();
		DrawTiresReflection_WriteS32(scratch, inputOffset + 4, centerZ);
		invLen = -(0x10000 / len);

		rimX = MFC2_S(9);
		rimY = -MFC2_S(10);
		rimZ = MFC2_S(11);

		DrawTiresReflection_WriteS16(scratch, outputBase + 0xb8, rimX);
		normalX = (centerX * invLen) >> 4;
		DrawTiresReflection_WriteS16(scratch, outputBase + 0x98, normalX);

		normalY = (centerY * invLen) >> 4;
		DrawTiresReflection_WriteS16(scratch, outputBase + 0xba, rimY);
		DrawTiresReflection_WriteS16(scratch, outputBase + 0x9a, normalY);

		normalZ = (centerZ * invLen) >> 4;
		DrawTiresReflection_WriteS32(scratch, outputBase + 0xbc, rimZ);
		DrawTiresReflection_WriteS32(scratch, outputBase + 0x9c, normalZ);

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

		wheelSize = DrawTiresReflection_ReadS32(scratch, 0x48);
		gte_op12_b();

		DrawTiresReflection_WriteS16(scratch, outputBase + 0xd8, (axisX * wheelSize) >> 0x12);
		DrawTiresReflection_WriteS16(scratch, outputBase + 0xda, (axisY * wheelSize) >> 0x12);
		DrawTiresReflection_WriteS16(scratch, outputBase + 0xdc, (axisZ * wheelSize) >> 0x12);

		axisX = MFC2_S(25);
		axisY = MFC2_S(26);
		axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));
		invLen = (wheelSize * -(0x10000 / len)) >> 12;

		DrawTiresReflection_WriteS16(scratch, outputBase + 0xf8, (axisX * invLen) >> 10);
		DrawTiresReflection_WriteS16(scratch, outputBase + 0xfa, (axisY * invLen) >> 10);
		DrawTiresReflection_WriteS16(scratch, outputBase + 0xfc, (axisZ * invLen) >> 10);
	}
}

static void DrawTiresReflection_SetupProjectionState(struct PushBuffer *pb)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Reflection loads
	// the PushBuffer projection matrix, clears translation, and writes OFX/OFY/H
	// at 0x8006f560-0x8006f5b0 through a2-relative loads. Native uses an
	// explicit PushBuffer pointer until the retail register contract is restored.
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0x0), 0);
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0x4), 1);
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0x8), 2);
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0xc), 3);
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0x10), 4);

	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	CTC2(pb->rect.w << 0xf, 24);
	CTC2(pb->rect.h << 0xf, 25);
	CTC2(pb->distanceToScreen_PREV, 26);
}

static void DrawTiresReflection_LoadCorner(struct DrawTiresReflectionScratch *scratch, int vectorIndex, int centerOffset, int outputBase, int axisSign,
                                           int rimSign)
{
	int x = DrawTiresReflection_ReadS16(scratch, centerOffset + 0) + (axisSign * DrawTiresReflection_ReadS16(scratch, outputBase + 0xd8)) +
	        (rimSign * DrawTiresReflection_ReadS16(scratch, outputBase + 0xf8));
	int y = DrawTiresReflection_ReadS16(scratch, centerOffset + 2) + (axisSign * DrawTiresReflection_ReadS16(scratch, outputBase + 0xda)) +
	        (rimSign * DrawTiresReflection_ReadS16(scratch, outputBase + 0xfa));
	int z = DrawTiresReflection_ReadS16(scratch, centerOffset + 4) + (axisSign * DrawTiresReflection_ReadS16(scratch, outputBase + 0xdc)) +
	        (rimSign * DrawTiresReflection_ReadS16(scratch, outputBase + 0xfc));

	MTC2(DrawTiresReflection_PackXY(x, y), vectorIndex * 2);
	MTC2(z, (vectorIndex * 2) + 1);
}

static struct DrawTiresReflectionProjectedWheel DrawTiresReflection_SelectProjectedWheel(struct DrawTiresReflectionScratch *scratch, int outputBase,
                                                                                         int wheelIndex)
{
	struct DrawTiresReflectionProjectedWheel selected;
	int selectedOT = DrawTiresReflection_ReadS32(scratch, 0x44);
	u32 depthValue;
	int angleValue;
	int spriteIndex;

	selected.wheelSprite = 0;
	selected.selectedOT = selectedOT;
	selected.selectedOTSlot = 0;
	selected.jumpIndex = wheelIndex;

	MTC2(DrawTiresReflection_ReadS32(scratch, outputBase + 0x98), 0);
	MTC2(DrawTiresReflection_ReadS32(scratch, outputBase + 0x9c), 1);
	CTC2(DrawTiresReflection_ReadS32(scratch, outputBase + 0xb8), 8);
	CTC2(DrawTiresReflection_ReadS32(scratch, outputBase + 0xbc), 9);

	gte_avsz4_b();
	depthValue = MFC2(24);

	DrawTiresReflection_WriteS32(scratch, 0x170, DrawTiresReflection_ReadS16(scratch, 0x16c) + selectedOT);
	DrawTiresReflection_WriteS32(scratch, 0x174, DrawTiresReflection_ReadS16(scratch, 0x16e) + selectedOT);

	gte_llv0_b();
	selected.selectedOTSlot = selectedOT + (int)((depthValue >> 0x11) << 2);
	angleValue = MFC2_S(9);
	spriteIndex = DrawTiresSolid_SelectSpriteIndex(angleValue);
	if (angleValue < 0)
		selected.jumpIndex += 4;

	selected.wheelSprite = scratch->wheelSprites[spriteIndex];

	return selected;
}

static void DrawTiresReflection_CopyIconUV(POLY_FT4 *p, struct Icon *icon)
{
	u32 uv23 = *(u32 *)&icon->texLayout.u2;

	*(u32 *)&p->u0 = *(u32 *)&icon->texLayout.u0;
	*(u32 *)&p->u1 = *(u32 *)&icon->texLayout.u1;
	*(u32 *)&p->u2 = uv23;
	*(u32 *)&p->u3 = uv23 >> 16;
}

static int DrawTiresReflection_ApplyCornerOrder(struct DrawTiresReflectionScratch *scratch, int jumpIndex, int *selectedOTSlot, int sxy[4])
{
	switch (jumpIndex)
	{
	case 0:
		sxy[0] = DrawTiresReflection_ReadS32(scratch, 0x124);
		sxy[1] = DrawTiresReflection_ReadS32(scratch, 0x11c);
		sxy[2] = DrawTiresReflection_ReadS32(scratch, 0x120);
		sxy[3] = DrawTiresReflection_ReadS32(scratch, 0x118);
		*selectedOTSlot -= DrawTiresReflection_ReadS32(scratch, 0x128);
		break;
	case 1:
		sxy[0] = DrawTiresReflection_ReadS32(scratch, 0x120);
		sxy[1] = DrawTiresReflection_ReadS32(scratch, 0x118);
		sxy[2] = DrawTiresReflection_ReadS32(scratch, 0x124);
		sxy[3] = DrawTiresReflection_ReadS32(scratch, 0x11c);
		*selectedOTSlot -= DrawTiresReflection_ReadS32(scratch, 0x12c);
		break;
	case 2:
		sxy[0] = DrawTiresReflection_ReadS32(scratch, 0x124);
		sxy[1] = DrawTiresReflection_ReadS32(scratch, 0x11c);
		sxy[2] = DrawTiresReflection_ReadS32(scratch, 0x120);
		sxy[3] = DrawTiresReflection_ReadS32(scratch, 0x118);
		DrawTiresReflection_WriteS32(scratch, 0x128, 0);
		break;
	case 3:
		sxy[0] = DrawTiresReflection_ReadS32(scratch, 0x120);
		sxy[1] = DrawTiresReflection_ReadS32(scratch, 0x118);
		sxy[2] = DrawTiresReflection_ReadS32(scratch, 0x124);
		sxy[3] = DrawTiresReflection_ReadS32(scratch, 0x11c);
		DrawTiresReflection_WriteS32(scratch, 0x12c, 0);
		break;
	case 4:
		sxy[0] = DrawTiresReflection_ReadS32(scratch, 0x11c);
		sxy[1] = DrawTiresReflection_ReadS32(scratch, 0x124);
		sxy[2] = DrawTiresReflection_ReadS32(scratch, 0x118);
		sxy[3] = DrawTiresReflection_ReadS32(scratch, 0x120);
		*selectedOTSlot -= DrawTiresReflection_ReadS32(scratch, 0x128);
		break;
	case 5:
		sxy[0] = DrawTiresReflection_ReadS32(scratch, 0x118);
		sxy[1] = DrawTiresReflection_ReadS32(scratch, 0x120);
		sxy[2] = DrawTiresReflection_ReadS32(scratch, 0x11c);
		sxy[3] = DrawTiresReflection_ReadS32(scratch, 0x124);
		*selectedOTSlot -= DrawTiresReflection_ReadS32(scratch, 0x12c);
		break;
	case 6:
		sxy[0] = DrawTiresReflection_ReadS32(scratch, 0x11c);
		sxy[1] = DrawTiresReflection_ReadS32(scratch, 0x124);
		sxy[2] = DrawTiresReflection_ReadS32(scratch, 0x118);
		sxy[3] = DrawTiresReflection_ReadS32(scratch, 0x120);
		*selectedOTSlot -= 8;
		DrawTiresReflection_WriteS32(scratch, 0x128, 8);
		break;
	case 7:
		sxy[0] = DrawTiresReflection_ReadS32(scratch, 0x118);
		sxy[1] = DrawTiresReflection_ReadS32(scratch, 0x120);
		sxy[2] = DrawTiresReflection_ReadS32(scratch, 0x11c);
		sxy[3] = DrawTiresReflection_ReadS32(scratch, 0x124);
		*selectedOTSlot -= 8;
		DrawTiresReflection_WriteS32(scratch, 0x12c, 8);
		break;
	default:
		return 0;
	}

	return 1;
}

static void DrawTiresReflection_WritePrimitiveCorners(POLY_FT4 *p, int sxy[4])
{
	*(u32 *)&p->x0 = sxy[0];
	*(u32 *)&p->x1 = sxy[1];
	*(u32 *)&p->x2 = sxy[2];
	*(u32 *)&p->x3 = sxy[3];
}

static void DrawTiresReflection_LinkPrimitive(struct DrawTiresReflectionScratch *scratch, POLY_FT4 *p, int selectedOTSlot)
{
	u_long *otSlot;
	int otRangeStart = DrawTiresReflection_ReadS32(scratch, 0x170);
	int otRangeEnd = DrawTiresReflection_ReadS32(scratch, 0x174);

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

static void DrawTiresReflection_EmitProjectedWheel(struct DrawTiresReflectionScratch *scratch, struct DrawTiresReflectionProjectedWheel *selected,
                                                   struct PrimMem *primMem, int *primCount, int centerOffset)
{
	POLY_FT4 *p = (POLY_FT4 *)primMem->curr;
	int selectedOTSlot = selected->selectedOTSlot;
	int sxy[4];
	int splitDelta;

	*(u32 *)&p->r0 = scratch->tireColor;

	if (selected->wheelSprite == 0)
		return;

	DrawTiresReflection_CopyIconUV(p, selected->wheelSprite);

#ifdef CTR_INTERNAL
	if (CtrTireDebug_ShouldLog(CTR_TIREDBG_REFLECT_PRIM) != 0)
	{
		fprintf(stderr, "[TIREDBG][reflect-prim] color=%08x code=%02x rgb=%02x,%02x,%02x tpage=%04x blend=%d clut=%04x jump=%d ot=%08x flags=%08x\n",
		        scratch->tireColor, p->code, p->r0, p->g0, p->b0, p->tpage, (p->tpage >> 5) & 3, p->clut, selected->jumpIndex, selected->selectedOTSlot,
		        scratch->instFlags);
	}
#endif

	if (DrawTiresReflection_ApplyCornerOrder(scratch, selected->jumpIndex, &selectedOTSlot, sxy) == 0)
		return;

	splitDelta = DrawTiresReflection_ReadS16(scratch, 0x50) - DrawTiresReflection_ReadS16(scratch, centerOffset + 2);
	if (splitDelta < 0)
		return;

	DrawTiresReflection_WritePrimitiveCorners(p, sxy);
	DrawTiresReflection_LinkPrimitive(scratch, p, selectedOTSlot);
	primMem->curr = (char *)primMem->curr + sizeof(POLY_FT4);
	(*primCount)++;
}

static int DrawTiresReflection_ProjectWheelQuads(struct DrawTiresReflectionScratch *scratch, struct PrimMem *primMem, int *primCount)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Reflection walks
	// the projection loop with s7/t8/t9 scratchpad cursors from 0x8006f5b4
	// onward. Native uses explicit scratch offsets while preserving the
	// reflected SXY scratch order used by the later jump-table primitive path.
	for (int centerOffset = 0x88, outputBase = 0x18, wheelIndex = 3; centerOffset >= 0x58; centerOffset -= 0x10, outputBase -= 8, wheelIndex--)
	{
		struct DrawTiresReflectionProjectedWheel projectedWheel;

		DrawTiresReflection_LoadCorner(scratch, 0, centerOffset, outputBase, -1, -1);
		DrawTiresReflection_LoadCorner(scratch, 1, centerOffset, outputBase, 1, -1);
		DrawTiresReflection_LoadCorner(scratch, 2, centerOffset, outputBase, -1, 1);

		gte_rtpt_b();
		DrawTiresReflection_WriteS32(scratch, 0x11c, MFC2(12));
		DrawTiresReflection_WriteS32(scratch, 0x118, MFC2(13));
		DrawTiresReflection_WriteS32(scratch, 0x124, MFC2(14));

		DrawTiresReflection_LoadCorner(scratch, 0, centerOffset, outputBase, 1, 1);
		gte_rtps_b();
		DrawTiresReflection_WriteS32(scratch, 0x120, MFC2(14));

		projectedWheel = DrawTiresReflection_SelectProjectedWheel(scratch, outputBase, wheelIndex);
		DrawTiresReflection_EmitProjectedWheel(scratch, &projectedWheel, primMem, primCount, centerOffset);
	}

	return 1;
}

static int DrawTiresReflection_StagePlayer(struct DrawTiresReflectionScratch *scratch, struct Driver *driver, struct Instance *inst, int playerIndex,
                                           struct PrimMem *primMem, int *primCount)
{
	struct InstDrawPerPlayer *idpp = DrawTiresReflection_GetIdpp(inst, playerIndex);
	struct PushBuffer *pb = idpp->pushBuffer;
	int flags = idpp->instFlags;

	scratch->playerCounter = scratch->numPlyr - playerIndex;
	scratch->vertSplit = inst->vertSplit;
	scratch->depthOffsetStartBytes = idpp->depthOffset[0] << 2;
	scratch->depthOffsetEndBytes = idpp->depthOffset[1] << 2;
	scratch->instFlags = flags;

	if ((flags & DRAW_SUCCESSFUL) == 0)
		return 0;

	if ((flags & 0x4000) == 0)
		return 0;

	if ((idpp->lodIndex - scratch->lodThreshold) > 0)
		return 0;

	if (pb == 0)
		return 0;

	scratch->wheelSprites = driver->wheelSprites;
	scratch->tireColor = ((flags & PUSHBUFFER_EXISTS) != 0) ? 0x2e808080 : driver->tireColor;

#ifdef CTR_INTERNAL
	if (CtrTireDebug_ShouldLog(CTR_TIREDBG_REFLECT_STAGE) != 0)
	{
		struct GameTracker *gGT = sdata->gGT;
		fprintf(stderr,
		        "[TIREDBG][reflect-stage] frame=%d level=%d mode=%08x player=%d inst=%p driver=%p flags=%08x push=%d refl=%d "
		        "lod=%d/%d tire=%08x driverTire=%08x wheelSize=%d pb=%p pbSize=%dx%d\n",
		        gGT != 0 ? gGT->framesInThisLEV : -1, gGT != 0 ? gGT->levelID : -1, gGT != 0 ? gGT->gameMode1 : 0, playerIndex, (void *)inst, (void *)driver,
		        flags, (flags & PUSHBUFFER_EXISTS) != 0, (flags & REFLECTIVE) != 0, idpp->lodIndex, scratch->lodThreshold, scratch->tireColor,
		        driver->tireColor, driver->wheelSize, (void *)pb, pb->rect.w, pb->rect.h);
	}
#endif

	DrawTiresReflection_BuildWheelLocalPairs(scratch, driver, inst, idpp);
	DrawTiresReflection_SetupGteState(scratch, inst, idpp, pb);
	DrawTiresReflection_BuildWheelAxes(scratch);
	DrawTiresReflection_SetupProjectionState(pb);
	DrawTiresReflection_ProjectWheelQuads(scratch, primMem, primCount);

	return 1;
}

void DrawTires_Reflection(struct Thread *thread, struct PrimMem *primMem, char numPlyr)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006f004-0x8006f9a8;
	// native uses the accepted explicit DrawTiresReflectionScratch stack/scratch ABI.
	struct DrawTiresReflectionScratch scratch = {0};
	int primCount;

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Reflection owns
	// scratchpad 0x1f800000 and live s7/t8/t9/v0/s0 register cursors. Native uses
	// offset-checked stack state and explicit helpers; PSX backfeed must restore
	// the retail scratchpad/register entry protocol.
	if (primMem == 0)
		return;

	primCount = primMem->unk1;
	DrawTiresReflection_InitScratch(&scratch, numPlyr);

	for (struct Thread *currThread = thread; currThread != 0; currThread = currThread->siblingThread)
	{
		struct Driver *driver = (struct Driver *)currThread->object;
		struct Instance *inst = currThread->inst;

		if (driver == 0 || inst == 0)
			continue;

		for (int playerIndex = 0; playerIndex < (int)(u8)numPlyr; playerIndex++)
		{
			if (DrawTiresReflection_StagePlayer(&scratch, driver, inst, playerIndex, primMem, &primCount) == 0)
				continue;
		}
	}

	primMem->unk1 = primCount;
}
