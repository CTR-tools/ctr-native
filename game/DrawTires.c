#include <common.h>

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

struct DrawTiresSolidProjectedWheel
{
	struct Icon *wheelSprite;
	int selectedOT;
	int selectedOTSlot;
	int jumpIndex;
};

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

static struct TrigPair DrawTiresSolid_TrigAngleSinCos(int angle)
{
	struct TrigPair pair;

	TRIG_AngleSinCos_r9r8r10(angle, &pair.sin, &pair.cos);

	return pair;
}

static int DrawTiresSolid_GetLodThreshold(u8 numPlyr)
{
	return (((int)numPlyr - 2) > 0) ? 0 : 2;
}

static struct InstDrawPerPlayer *DrawTiresSolid_GetIdpp(struct Instance *inst, int playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)INST_GETIDPP(inst) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static void DrawTiresSolid_InitScratch(struct DrawTiresScratch *scratch, u8 numPlyr)
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

static void DrawTiresSolid_AddHazardOffset(struct DrawTiresPackedVec3 *rim, int angle, int shift)
{
	struct TrigPair spin = DrawTiresSolid_TrigAngleSinCos(angle);

	rim->y = (s16)(rim->y + (spin.cos >> shift));
	rim->z.halves.lo = (s16)(rim->z.halves.lo + (spin.sin >> shift));
}

s32 Unknown_8006ef98(s32 radicand)
{
	int root = 0;
	int remainder = 0;

	// NOTE(aalhendi): Retail input is s5 and output is s6.
	if (radicand == 0)
	{
		return 0;
	}

	int bitCount = DrawTiresSolid_CountLeadingSignBits(radicand) & 0x1e;
	u32 work = ((u32)radicand) << bitCount;

	for (bitCount ^= 0x1e; bitCount >= 0; bitCount -= 2)
	{
		remainder |= work >> 30;
		int trial = remainder - ((root << 2) + 1);
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

static void DrawTiresSolid_BuildWheelLocalPairs(struct DrawTiresScratch *scratch, struct Driver *driver, struct Instance *inst, struct InstDrawPerPlayer *idpp)
{
	int wheelX = (inst->scale.x * 0x90) >> 12;
	int negWheelX = -wheelX;
	int hazardShift = 9;

	scratch->wheelLocal[0].center.x = wheelX;
	scratch->wheelLocal[2].center.x = wheelX;
	scratch->wheelLocal[1].center.x = negWheelX;
	scratch->wheelLocal[3].center.x = negWheelX;
	scratch->wheelLocal[0].rim.x = wheelX - 0x1000;
	scratch->wheelLocal[2].rim.x = wheelX - 0x1000;
	scratch->wheelLocal[1].rim.x = negWheelX + 0x1000;
	scratch->wheelLocal[3].rim.x = negWheelX + 0x1000;

	int wheelY = (inst->scale.y * 0x40) >> 12;
	for (int wheelIndex = 0; wheelIndex < 4; wheelIndex++)
	{
		scratch->wheelLocal[wheelIndex].center.y = wheelY;
		scratch->wheelLocal[wheelIndex].rim.y = 0;
	}

	int wheelFrontZ = (inst->scale.z * 0xc7) >> 12;
	scratch->wheelLocal[0].center.z.word = wheelFrontZ;
	scratch->wheelLocal[1].center.z.word = wheelFrontZ;

	int wheelRearZ = (inst->scale.z * -0x60) >> 12;
	scratch->wheelLocal[2].center.z.word = wheelRearZ;
	scratch->wheelLocal[2].rim.z.word = wheelRearZ;
	scratch->wheelLocal[3].center.z.word = wheelRearZ;
	scratch->wheelLocal[3].rim.z.word = wheelRearZ;

	scratch->otRangeNormal = idpp->otRangeNormal;
	scratch->otRangeSecondary = idpp->otRangeSecondary;
	scratch->wheelSize.word = (s16)driver->wheelSize;

	struct TrigPair steering = DrawTiresSolid_TrigAngleSinCos(driver->wheelRotation << 2);
	scratch->wheelLocal[0].rim.x = scratch->wheelLocal[0].center.x - steering.cos;
	scratch->wheelLocal[0].rim.z.word = scratch->wheelLocal[0].center.z.word + steering.sin;
	scratch->wheelLocal[1].rim.x = scratch->wheelLocal[1].center.x + steering.cos;
	scratch->wheelLocal[1].rim.z.word = scratch->wheelLocal[1].center.z.word - steering.sin;

	int hazardAngle = driver->hazardTimer << 5;
	if ((driver->hazardTimer & 1) != 0)
	{
		hazardShift = 6;
		hazardAngle <<= 1;
	}

	DrawTiresSolid_AddHazardOffset(&scratch->wheelLocal[0].rim, hazardAngle, hazardShift);
	DrawTiresSolid_AddHazardOffset(&scratch->wheelLocal[2].rim, hazardAngle + 0x400, hazardShift);
	DrawTiresSolid_AddHazardOffset(&scratch->wheelLocal[1].rim, hazardAngle + 0x800, hazardShift);
	DrawTiresSolid_AddHazardOffset(&scratch->wheelLocal[3].rim, hazardAngle + 0xc00, hazardShift);
}

static void DrawTiresSolid_SetupGteState(struct DrawTiresScratch *scratch, struct Instance *inst, struct InstDrawPerPlayer *idpp, struct PushBuffer *pb)
{
	int relX = (inst->matrix.t[0] - pb->pos.x) << 2;
	int relY = (inst->matrix.t[1] - pb->pos.y) << 2;
	int relZ = (inst->matrix.t[2] - pb->pos.z) << 2;
	int splitCameraY = (scratch->vertSplit - pb->pos.y) << 2;

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

	scratch->splitCameraY = splitCameraY;
}

static void DrawTiresSolid_BuildWheelAxes(struct DrawTiresScratch *scratch)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail uses s7/t8 as scratchpad
	// cursors initialized at 0x8006e8e4-0x8006e8e8, then walks four wheel records
	// backward through scratchpad. Native uses explicit offsets while preserving
	// the same GTE operations and scratch writes.
	for (int wheelIndex = 3; wheelIndex >= 0; wheelIndex--)
	{
		struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
		struct DrawTiresPackedVec3 *viewNormal = &scratch->viewNormalVectors[wheelIndex];
		struct DrawTiresPackedVec3 *transformedRim = &scratch->transformedRimVectors[wheelIndex];
		SVec3Slot *axisA = &scratch->tireAxisA[wheelIndex];
		SVec3Slot *axisB = &scratch->tireAxisB[wheelIndex];

		MTC2(CTR_PackS16Pair(wheelLocal->center.x, wheelLocal->center.y), 0);
		MTC2(wheelLocal->center.z.word, 1);
		MTC2(CTR_PackS16Pair(wheelLocal->rim.x, wheelLocal->rim.y), 2);
		MTC2(wheelLocal->rim.z.word, 3);

		gte_lcv0tr_b();
		int centerX = MFC2_S(9);
		int centerY = MFC2_S(10);
		int centerZ = MFC2_S(11);

		gte_sqr0_b();
		wheelLocal->center.x = centerX;
		wheelLocal->center.y = centerY;
		int len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));

		gte_lcv1_b();
		wheelLocal->center.z.word = centerZ;
		int invLen = -(0x10000 / len);

		int rimX = MFC2_S(9);
		int rimY = MFC2_S(10);
		int rimZ = MFC2_S(11);

		transformedRim->x = rimX;
		int normalX = (centerX * invLen) >> 4;
		viewNormal->x = normalX;

		int normalY = (centerY * invLen) >> 4;
		transformedRim->y = rimY;
		viewNormal->y = normalY;

		int normalZ = (centerZ * invLen) >> 4;
		transformedRim->z.word = rimZ;
		viewNormal->z.word = normalZ;

		CTC2(normalX & 0xffff, 0);
		CTC2(normalY & 0xffff, 2);
		CTC2(normalZ, 4);

		gte_op12_b();
		int axisX = MFC2_S(25);
		int axisY = MFC2_S(26);
		int axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));
		invLen = 0x10000 / len;

		axisX = (axisX * invLen) >> 4;
		MTC2(axisX, 9);
		axisY = (axisY * invLen) >> 4;
		MTC2(axisY, 10);
		axisZ = (axisZ * invLen) >> 4;
		MTC2(axisZ, 11);

		int wheelSize = scratch->wheelSize.word;
		gte_op12_b();

		axisA->x = (axisX * wheelSize) >> 0x12;
		axisA->y = (axisY * wheelSize) >> 0x12;
		axisA->z = (axisZ * wheelSize) >> 0x12;

		axisX = MFC2_S(25);
		axisY = MFC2_S(26);
		axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));
		invLen = (wheelSize * -(0x10000 / len)) >> 12;

		axisB->x = (axisX * invLen) >> 10;
		axisB->y = (axisY * invLen) >> 10;
		axisB->z = (axisZ * invLen) >> 10;
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

static void DrawTiresSolid_LoadCorner(struct DrawTiresScratch *scratch, int vectorIndex, int wheelIndex, int axisSign, int rimSign)
{
	struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
	SVec3Slot *axisA = &scratch->tireAxisA[wheelIndex];
	SVec3Slot *axisB = &scratch->tireAxisB[wheelIndex];
	int x = wheelLocal->center.x + (axisSign * axisA->x) + (rimSign * axisB->x);
	int y = wheelLocal->center.y + (axisSign * axisA->y) + (rimSign * axisB->y);
	int z = wheelLocal->center.z.halves.lo + (axisSign * axisA->z) + (rimSign * axisB->z);

	MTC2(CTR_PackS16Pair(x, y), vectorIndex * 2);
	MTC2(z, (vectorIndex * 2) + 1);
}

static int DrawTiresSolid_SelectSpriteIndex(int angleValue)
{
	int tableIndex = angleValue >> 5;

	if (angleValue < 0)
	{
		tableIndex = -tableIndex;
	}

	if ((tableIndex - 0x80) > 0)
	{
		tableIndex = 0x80;
	}

	return sDrawTiresSpriteIndexTable[tableIndex];
}

static struct DrawTiresSolidProjectedWheel DrawTiresSolid_SelectProjectedWheel(struct DrawTiresScratch *scratch, int wheelIndex)
{
	struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
	struct DrawTiresPackedVec3 *viewNormal = &scratch->viewNormalVectors[wheelIndex];
	struct DrawTiresPackedVec3 *transformedRim = &scratch->transformedRimVectors[wheelIndex];
	int selectedOT = scratch->otRangeNormal;
	int splitDelta = scratch->splitCameraY - wheelLocal->center.y;
	struct DrawTiresSolidProjectedWheel selected = {
	    .selectedOT = selectedOT,
	    .jumpIndex = wheelIndex,
	};

	gte_avsz4_b();

	if (splitDelta >= 0)
	{
		selectedOT = scratch->otRangeSecondary;
	}

	MTC2(CTR_PackS16Pair(viewNormal->x, viewNormal->y), 0);
	MTC2(viewNormal->z.word, 1);
	CTC2(CTR_PackS16Pair(transformedRim->x, transformedRim->y), 8);
	CTC2(transformedRim->z.word, 9);

	scratch->otRangeStart = scratch->depthOffsetStartBytes + selectedOT;
	scratch->otRangeEnd = scratch->depthOffsetEndBytes + selectedOT;

	gte_llv0_b();
	u32 depthValue = MFC2(24);
	selected.selectedOT = selectedOT;
	selected.selectedOTSlot = selectedOT + (int)((depthValue >> 0x11) << 2);
	int angleValue = MFC2_S(9);
	int spriteIndex = DrawTiresSolid_SelectSpriteIndex(angleValue);
	if (angleValue < 0)
	{
		selected.jumpIndex += 4;
	}

	selected.wheelSprite = scratch->wheelSprites[spriteIndex];

	return selected;
}

static void DrawTiresSolid_CopyIconUV(POLY_FT4 *p, struct Icon *icon)
{
	u32 uv23 = CTR_ReadU32LE(&icon->texLayout.u2);

	CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32LE(&icon->texLayout.u0));
	CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32LE(&icon->texLayout.u1));
	CtrGpu_WritePackedUVWord(&p->u2, uv23);
	CtrGpu_WritePackedUVWord(&p->u3, uv23 >> 16);
}

static int DrawTiresSolid_ApplyCornerOrder(struct DrawTiresScratch *scratch, int jumpIndex, int *selectedOTSlot, int sxy[4])
{
	switch (jumpIndex)
	{
	case 0:
		sxy[0] = scratch->projectedSxy[3];
		sxy[1] = scratch->projectedSxy[1];
		sxy[2] = scratch->projectedSxy[2];
		sxy[3] = scratch->projectedSxy[0];
		*selectedOTSlot -= scratch->cornerDepthBias[0];
		break;
	case 1:
		sxy[0] = scratch->projectedSxy[2];
		sxy[1] = scratch->projectedSxy[0];
		sxy[2] = scratch->projectedSxy[3];
		sxy[3] = scratch->projectedSxy[1];
		*selectedOTSlot -= scratch->cornerDepthBias[1];
		break;
	case 2:
		sxy[0] = scratch->projectedSxy[3];
		sxy[1] = scratch->projectedSxy[1];
		sxy[2] = scratch->projectedSxy[2];
		sxy[3] = scratch->projectedSxy[0];
		scratch->cornerDepthBias[0] = 0;
		break;
	case 3:
		sxy[0] = scratch->projectedSxy[2];
		sxy[1] = scratch->projectedSxy[0];
		sxy[2] = scratch->projectedSxy[3];
		sxy[3] = scratch->projectedSxy[1];
		scratch->cornerDepthBias[1] = 0;
		break;
	case 4:
		sxy[0] = scratch->projectedSxy[1];
		sxy[1] = scratch->projectedSxy[3];
		sxy[2] = scratch->projectedSxy[0];
		sxy[3] = scratch->projectedSxy[2];
		*selectedOTSlot -= scratch->cornerDepthBias[0];
		break;
	case 5:
		sxy[0] = scratch->projectedSxy[0];
		sxy[1] = scratch->projectedSxy[2];
		sxy[2] = scratch->projectedSxy[1];
		sxy[3] = scratch->projectedSxy[3];
		*selectedOTSlot -= scratch->cornerDepthBias[1];
		break;
	case 6:
		sxy[0] = scratch->projectedSxy[1];
		sxy[1] = scratch->projectedSxy[3];
		sxy[2] = scratch->projectedSxy[0];
		sxy[3] = scratch->projectedSxy[2];
		*selectedOTSlot -= 8;
		scratch->cornerDepthBias[0] = 8;
		break;
	case 7:
		sxy[0] = scratch->projectedSxy[0];
		sxy[1] = scratch->projectedSxy[2];
		sxy[2] = scratch->projectedSxy[1];
		sxy[3] = scratch->projectedSxy[3];
		*selectedOTSlot -= 8;
		scratch->cornerDepthBias[1] = 8;
		break;
	default:
		return 0;
	}

	return 1;
}

static void DrawTiresSolid_WritePrimitiveCorners(POLY_FT4 *p, int sxy[4])
{
	CtrGpu_WritePackedXY(&p->x0, (u32)sxy[0]);
	CtrGpu_WritePackedXY(&p->x1, (u32)sxy[1]);
	CtrGpu_WritePackedXY(&p->x2, (u32)sxy[2]);
	CtrGpu_WritePackedXY(&p->x3, (u32)sxy[3]);
}

static void DrawTiresSolid_LinkPrimitive(struct DrawTiresScratch *scratch, POLY_FT4 *p, int selectedOTSlot)
{
	int otRangeStart = scratch->otRangeStart;
	int otRangeEnd = scratch->otRangeEnd;

	if ((otRangeStart - selectedOTSlot) > 0)
	{
		selectedOTSlot = otRangeStart;
	}

	if ((otRangeEnd - selectedOTSlot) < 0)
	{
		selectedOTSlot = otRangeEnd;
	}

	u32 *otSlot = (u32 *)(u32)selectedOTSlot;
	p->tag = CtrGpu_PackOTTag(*otSlot, 0x09000000);
	*otSlot = (u32)CtrGpu_PrimToOTLink24(p);
}

static int DrawTiresSolid_EmitProjectedWheel(struct DrawTiresScratch *scratch, struct DrawTiresSolidProjectedWheel *selected, struct PrimMem *primMem,
                                             int *primCount)
{
	POLY_FT4 *p = (POLY_FT4 *)primMem->cursor;
	int selectedOTSlot = selected->selectedOTSlot;
	int sxy[4];

	CtrGpu_WriteColorCode(&p->r0, scratch->tireColor);

	if (selected->wheelSprite == 0)
	{
		return 1;
	}

	DrawTiresSolid_CopyIconUV(p, selected->wheelSprite);

	if (DrawTiresSolid_ApplyCornerOrder(scratch, selected->jumpIndex, &selectedOTSlot, sxy) == 0)
	{
		return 1;
	}

	if (scratch->otRangeSecondary == selected->selectedOT && (scratch->instFlags & 0x4000) != 0)
	{
		return 0;
	}

	DrawTiresSolid_WritePrimitiveCorners(p, sxy);
	DrawTiresSolid_LinkPrimitive(scratch, p, selectedOTSlot);
	primMem->cursor = (char *)primMem->cursor + sizeof(POLY_FT4);
	(*primCount)++;

	return 1;
}

static int DrawTiresSolid_ProjectWheelQuads(struct DrawTiresScratch *scratch, struct PrimMem *primMem, int *primCount)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Solid walks the
	// projection loop with s7/t8/t9 scratchpad cursors from 0x8006eb34 onward.
	// Native uses explicit scratch offsets while preserving corner ordering,
	// depth-bias scratch updates, UV copy, and OT linking.
	for (int wheelIndex = 3; wheelIndex >= 0; wheelIndex--)
	{
		DrawTiresSolid_LoadCorner(scratch, 0, wheelIndex, -1, -1);
		DrawTiresSolid_LoadCorner(scratch, 1, wheelIndex, 1, -1);
		DrawTiresSolid_LoadCorner(scratch, 2, wheelIndex, -1, 1);

		gte_rtpt_b();
		scratch->projectedSxy[0] = MFC2(12);
		scratch->projectedSxy[1] = MFC2(13);
		scratch->projectedSxy[2] = MFC2(14);

		DrawTiresSolid_LoadCorner(scratch, 0, wheelIndex, 1, 1);
		gte_rtps_b();
		scratch->projectedSxy[3] = MFC2(14);

		struct DrawTiresSolidProjectedWheel projectedWheel = DrawTiresSolid_SelectProjectedWheel(scratch, wheelIndex);

		if (DrawTiresSolid_EmitProjectedWheel(scratch, &projectedWheel, primMem, primCount) == 0)
		{
			return 0;
		}
	}

	return 1;
}

static int DrawTiresSolid_StagePlayer(struct DrawTiresScratch *scratch, struct Driver *driver, struct Instance *inst, int playerIndex, struct PrimMem *primMem,
                                      int *primCount)
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
	{
		return 0;
	}

	if ((flags & HIDE_MODEL) != 0)
	{
		return 0;
	}

	if ((idpp->lodIndex - scratch->lodThreshold) > 0)
	{
		return 0;
	}

	scratch->wheelSprites = driver->wheelSprites;
	scratch->tireColor = ((flags & PUSHBUFFER_EXISTS) != 0) ? 0x2e808080 : driver->tireColor;

	if (pb == 0)
	{
		return 0;
	}

	scratch->otRangeNormal = idpp->otRangeNormal;
	scratch->otRangeSecondary = idpp->otRangeSecondary;
	scratch->otRangeStart = scratch->depthOffsetStartBytes + idpp->otRangeNormal;
	scratch->otRangeEnd = scratch->depthOffsetEndBytes + idpp->otRangeNormal;

	DrawTiresSolid_BuildWheelLocalPairs(scratch, driver, inst, idpp);
	DrawTiresSolid_SetupGteState(scratch, inst, idpp, pb);
	DrawTiresSolid_BuildWheelAxes(scratch);
	DrawTiresSolid_SetupProjectionState(pb);
	Rtx_BeginScaled(primMem, pb, 0.25f, inst); // MultiTrack: wheels become ray traced geometry (view space x4 here)
	DrawTiresSolid_ProjectWheelQuads(scratch, primMem, primCount);
	Rtx_EndTag(primMem);

	return 1;
}

void DrawTires_Solid(struct Thread *thread, struct PrimMem *primMem, u8 numPlyr)
{
	// Native uses the explicit DrawTiresScratch stack/scratch ABI.
	struct DrawTiresScratch scratch = {0};

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Solid owns scratchpad
	// 0x1f800000 and live s7/t8/t9/v0/s0 register cursors. Native uses
	// offset-checked stack state and explicit helpers; PSX backfeed must restore
	// the retail scratchpad/register entry protocol.
	if (primMem == 0)
	{
		return;
	}

	int primCount = primMem->primitiveCount;
	DrawTiresSolid_InitScratch(&scratch, numPlyr);

	for (struct Thread *currThread = thread; currThread != 0; currThread = currThread->siblingThread)
	{
		struct Driver *driver = (struct Driver *)currThread->object;
		struct Instance *inst = currThread->inst;

		if (driver == 0 || inst == 0)
		{
			continue;
		}

		for (int playerIndex = 0; playerIndex < (int)(u8)numPlyr; playerIndex++)
		{
			if (DrawTiresSolid_StagePlayer(&scratch, driver, inst, playerIndex, primMem, &primCount) == 0)
			{
				continue;
			}
		}
	}

	primMem->primitiveCount = primCount;
}
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

static int DrawTiresReflection_GetLodThreshold(u8 numPlyr)
{
	return (((int)numPlyr - 2) > 0) ? 0 : 2;
}

static struct InstDrawPerPlayer *DrawTiresReflection_GetIdpp(struct Instance *inst, int playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)INST_GETIDPP(inst) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static int DrawTiresReflection_ReadMatrixWord(MATRIX *matrix, int offset)
{
	return *(int *)((char *)matrix + offset);
}

static void DrawTiresReflection_InitScratch(struct DrawTiresScratch *scratch, u8 numPlyr)
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

static void DrawTiresReflection_AddHazardOffset(struct DrawTiresPackedVec3 *rim, int angle, int shift)
{
	struct TrigPair spin = DrawTiresSolid_TrigAngleSinCos(angle);

	rim->y = (s16)(rim->y + (spin.cos >> shift));
	rim->z.halves.lo = (s16)(rim->z.halves.lo + (spin.sin >> shift));
}

static void DrawTiresReflection_BuildWheelLocalPairs(struct DrawTiresScratch *scratch, struct Driver *driver, struct Instance *inst,
                                                     struct InstDrawPerPlayer *idpp)
{
	int hazardShift = 9;

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Reflection loads
	// Instance.scale[0] at 0x8006f0e0, then the push-buffer branch delay slot at
	// 0x8006f0e8 overwrites v1 with instFlags & 0x100 before the multiply at
	// 0x8006f10c. Native makes that register lineage explicit until the exact
	// delay-slot/register ABI is restored for PSX backfeed.
	int wheelX = ((scratch->instFlags & PUSHBUFFER_EXISTS) * 0x90) >> 12;
	int negWheelX = -wheelX;

	scratch->wheelLocal[0].center.x = wheelX;
	scratch->wheelLocal[2].center.x = wheelX;
	scratch->wheelLocal[1].center.x = negWheelX;
	scratch->wheelLocal[3].center.x = negWheelX;
	scratch->wheelLocal[0].rim.x = wheelX - 0x1000;
	scratch->wheelLocal[2].rim.x = wheelX - 0x1000;
	scratch->wheelLocal[1].rim.x = negWheelX + 0x1000;
	scratch->wheelLocal[3].rim.x = negWheelX + 0x1000;

	int wheelY = (inst->scale.y * 0x40) >> 12;
	for (int wheelIndex = 0; wheelIndex < 4; wheelIndex++)
	{
		scratch->wheelLocal[wheelIndex].center.y = wheelY;
		scratch->wheelLocal[wheelIndex].rim.y = 0;
	}

	int wheelFrontZ = (inst->scale.z * 0xc7) >> 12;
	scratch->wheelLocal[0].center.z.word = wheelFrontZ;
	scratch->wheelLocal[1].center.z.word = wheelFrontZ;

	int wheelRearZ = (inst->scale.z * -0x60) >> 12;
	scratch->wheelLocal[2].center.z.word = wheelRearZ;
	scratch->wheelLocal[2].rim.z.word = wheelRearZ;
	scratch->wheelLocal[3].center.z.word = wheelRearZ;
	scratch->wheelLocal[3].rim.z.word = wheelRearZ;

	scratch->otRangeNormal = idpp->otRangeNormal;
	scratch->otRangeSecondary = idpp->otRangeSecondary;
	scratch->wheelSize.word = (s16)driver->wheelSize;

	struct TrigPair steering = DrawTiresSolid_TrigAngleSinCos(driver->wheelRotation << 2);
	scratch->wheelLocal[0].rim.x = scratch->wheelLocal[0].center.x - steering.cos;
	scratch->wheelLocal[0].rim.z.word = scratch->wheelLocal[0].center.z.word + steering.sin;
	scratch->wheelLocal[1].rim.x = scratch->wheelLocal[1].center.x + steering.cos;
	scratch->wheelLocal[1].rim.z.word = scratch->wheelLocal[1].center.z.word - steering.sin;

	int hazardAngle = driver->hazardTimer << 5;
	if ((driver->hazardTimer & 1) != 0)
	{
		hazardShift = 6;
		hazardAngle <<= 1;
	}

	DrawTiresReflection_AddHazardOffset(&scratch->wheelLocal[0].rim, hazardAngle, hazardShift);
	DrawTiresReflection_AddHazardOffset(&scratch->wheelLocal[2].rim, hazardAngle + 0x400, hazardShift);
	DrawTiresReflection_AddHazardOffset(&scratch->wheelLocal[1].rim, hazardAngle + 0x800, hazardShift);
	DrawTiresReflection_AddHazardOffset(&scratch->wheelLocal[3].rim, hazardAngle + 0xc00, hazardShift);
}

static void DrawTiresReflection_SetupGteState(struct DrawTiresScratch *scratch, struct Instance *inst, struct InstDrawPerPlayer *idpp, struct PushBuffer *pb)
{
	int relX = (inst->matrix.t[0] - pb->pos.x) << 2;
	int relY = (inst->matrix.t[1] - pb->pos.y) << 2;
	int relZ = (inst->matrix.t[2] - pb->pos.z) << 2;
	int splitCameraY = (scratch->vertSplit - pb->pos.y) << 2;

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

	scratch->splitCameraY = splitCameraY;
}

static void DrawTiresReflection_BuildWheelAxes(struct DrawTiresScratch *scratch)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail uses s7/t8 as scratchpad
	// cursors initialized at 0x8006f360-0x8006f364, then walks four wheel
	// records backward through scratchpad. Native uses explicit offsets while
	// preserving the reflected Y mirror and GTE operation sequence.
	for (int wheelIndex = 3; wheelIndex >= 0; wheelIndex--)
	{
		struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
		struct DrawTiresPackedVec3 *viewNormal = &scratch->viewNormalVectors[wheelIndex];
		struct DrawTiresPackedVec3 *transformedRim = &scratch->transformedRimVectors[wheelIndex];
		SVec3Slot *axisA = &scratch->tireAxisA[wheelIndex];
		SVec3Slot *axisB = &scratch->tireAxisB[wheelIndex];
		int splitCameraY = scratch->splitCameraY;

		MTC2(CTR_PackS16Pair(wheelLocal->center.x, wheelLocal->center.y), 0);
		MTC2(wheelLocal->center.z.word, 1);
		MTC2(CTR_PackS16Pair(wheelLocal->rim.x, wheelLocal->rim.y), 2);
		MTC2(wheelLocal->rim.z.word, 3);

		gte_lcv0tr_b();
		int centerX = MFC2_S(9);
		int originalCenterY = MFC2_S(10);
		int centerZ = MFC2_S(11);
		int centerY = splitCameraY - (originalCenterY - splitCameraY);

		gte_sqr0_b();
		wheelLocal->center.x = centerX;
		wheelLocal->center.y = centerY;
		int len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));

		gte_lcv1_b();
		wheelLocal->center.z.word = centerZ;
		int invLen = -(0x10000 / len);

		int rimX = MFC2_S(9);
		int rimY = -MFC2_S(10);
		int rimZ = MFC2_S(11);

		transformedRim->x = rimX;
		int normalX = (centerX * invLen) >> 4;
		viewNormal->x = normalX;

		int normalY = (centerY * invLen) >> 4;
		transformedRim->y = rimY;
		viewNormal->y = normalY;

		int normalZ = (centerZ * invLen) >> 4;
		transformedRim->z.word = rimZ;
		viewNormal->z.word = normalZ;

		CTC2(normalX & 0xffff, 0);
		CTC2(normalY & 0xffff, 2);
		CTC2(normalZ, 4);

		gte_op12_b();
		int axisX = MFC2_S(25);
		int axisY = MFC2_S(26);
		int axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));
		invLen = 0x10000 / len;

		axisX = (axisX * invLen) >> 4;
		MTC2(axisX, 9);
		axisY = (axisY * invLen) >> 4;
		MTC2(axisY, 10);
		axisZ = (axisZ * invLen) >> 4;
		MTC2(axisZ, 11);

		int wheelSize = scratch->wheelSize.word;
		gte_op12_b();

		axisA->x = (axisX * wheelSize) >> 0x12;
		axisA->y = (axisY * wheelSize) >> 0x12;
		axisA->z = (axisZ * wheelSize) >> 0x12;

		axisX = MFC2_S(25);
		axisY = MFC2_S(26);
		axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(MFC2_S(25) + MFC2_S(26) + MFC2_S(27));
		invLen = (wheelSize * -(0x10000 / len)) >> 12;

		axisB->x = (axisX * invLen) >> 10;
		axisB->y = (axisY * invLen) >> 10;
		axisB->z = (axisZ * invLen) >> 10;
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

static void DrawTiresReflection_LoadCorner(struct DrawTiresScratch *scratch, int vectorIndex, int wheelIndex, int axisSign, int rimSign)
{
	struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
	SVec3Slot *axisA = &scratch->tireAxisA[wheelIndex];
	SVec3Slot *axisB = &scratch->tireAxisB[wheelIndex];
	int x = wheelLocal->center.x + (axisSign * axisA->x) + (rimSign * axisB->x);
	int y = wheelLocal->center.y + (axisSign * axisA->y) + (rimSign * axisB->y);
	int z = wheelLocal->center.z.halves.lo + (axisSign * axisA->z) + (rimSign * axisB->z);

	MTC2(CTR_PackS16Pair(x, y), vectorIndex * 2);
	MTC2(z, (vectorIndex * 2) + 1);
}

static struct DrawTiresReflectionProjectedWheel DrawTiresReflection_SelectProjectedWheel(struct DrawTiresScratch *scratch, int wheelIndex)
{
	struct DrawTiresPackedVec3 *viewNormal = &scratch->viewNormalVectors[wheelIndex];
	struct DrawTiresPackedVec3 *transformedRim = &scratch->transformedRimVectors[wheelIndex];
	int selectedOT = scratch->otRangeSecondary;
	struct DrawTiresReflectionProjectedWheel selected = {
	    .selectedOT = selectedOT,
	    .jumpIndex = wheelIndex,
	};

	MTC2(CTR_PackS16Pair(viewNormal->x, viewNormal->y), 0);
	MTC2(viewNormal->z.word, 1);
	CTC2(CTR_PackS16Pair(transformedRim->x, transformedRim->y), 8);
	CTC2(transformedRim->z.word, 9);

	gte_avsz4_b();
	u32 depthValue = MFC2(24);

	scratch->otRangeStart = scratch->depthOffsetStartBytes + selectedOT;
	scratch->otRangeEnd = scratch->depthOffsetEndBytes + selectedOT;

	gte_llv0_b();
	selected.selectedOTSlot = selectedOT + (int)((depthValue >> 0x11) << 2);
	int angleValue = MFC2_S(9);
	int spriteIndex = DrawTiresSolid_SelectSpriteIndex(angleValue);
	if (angleValue < 0)
	{
		selected.jumpIndex += 4;
	}

	selected.wheelSprite = scratch->wheelSprites[spriteIndex];

	return selected;
}

static void DrawTiresReflection_CopyIconUV(POLY_FT4 *p, struct Icon *icon)
{
	u32 uv23 = CTR_ReadU32LE(&icon->texLayout.u2);

	CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32LE(&icon->texLayout.u0));
	CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32LE(&icon->texLayout.u1));
	CtrGpu_WritePackedUVWord(&p->u2, uv23);
	CtrGpu_WritePackedUVWord(&p->u3, uv23 >> 16);
}

static int DrawTiresReflection_ApplyCornerOrder(struct DrawTiresScratch *scratch, int jumpIndex, int *selectedOTSlot, int sxy[4])
{
	switch (jumpIndex)
	{
	case 0:
		sxy[0] = scratch->projectedSxy[3];
		sxy[1] = scratch->projectedSxy[1];
		sxy[2] = scratch->projectedSxy[2];
		sxy[3] = scratch->projectedSxy[0];
		*selectedOTSlot -= scratch->cornerDepthBias[0];
		break;
	case 1:
		sxy[0] = scratch->projectedSxy[2];
		sxy[1] = scratch->projectedSxy[0];
		sxy[2] = scratch->projectedSxy[3];
		sxy[3] = scratch->projectedSxy[1];
		*selectedOTSlot -= scratch->cornerDepthBias[1];
		break;
	case 2:
		sxy[0] = scratch->projectedSxy[3];
		sxy[1] = scratch->projectedSxy[1];
		sxy[2] = scratch->projectedSxy[2];
		sxy[3] = scratch->projectedSxy[0];
		scratch->cornerDepthBias[0] = 0;
		break;
	case 3:
		sxy[0] = scratch->projectedSxy[2];
		sxy[1] = scratch->projectedSxy[0];
		sxy[2] = scratch->projectedSxy[3];
		sxy[3] = scratch->projectedSxy[1];
		scratch->cornerDepthBias[1] = 0;
		break;
	case 4:
		sxy[0] = scratch->projectedSxy[1];
		sxy[1] = scratch->projectedSxy[3];
		sxy[2] = scratch->projectedSxy[0];
		sxy[3] = scratch->projectedSxy[2];
		*selectedOTSlot -= scratch->cornerDepthBias[0];
		break;
	case 5:
		sxy[0] = scratch->projectedSxy[0];
		sxy[1] = scratch->projectedSxy[2];
		sxy[2] = scratch->projectedSxy[1];
		sxy[3] = scratch->projectedSxy[3];
		*selectedOTSlot -= scratch->cornerDepthBias[1];
		break;
	case 6:
		sxy[0] = scratch->projectedSxy[1];
		sxy[1] = scratch->projectedSxy[3];
		sxy[2] = scratch->projectedSxy[0];
		sxy[3] = scratch->projectedSxy[2];
		*selectedOTSlot -= 8;
		scratch->cornerDepthBias[0] = 8;
		break;
	case 7:
		sxy[0] = scratch->projectedSxy[0];
		sxy[1] = scratch->projectedSxy[2];
		sxy[2] = scratch->projectedSxy[1];
		sxy[3] = scratch->projectedSxy[3];
		*selectedOTSlot -= 8;
		scratch->cornerDepthBias[1] = 8;
		break;
	default:
		return 0;
	}

	return 1;
}

static void DrawTiresReflection_WritePrimitiveCorners(POLY_FT4 *p, int sxy[4])
{
	CtrGpu_WritePackedXY(&p->x0, (u32)sxy[0]);
	CtrGpu_WritePackedXY(&p->x1, (u32)sxy[1]);
	CtrGpu_WritePackedXY(&p->x2, (u32)sxy[2]);
	CtrGpu_WritePackedXY(&p->x3, (u32)sxy[3]);
}

static void DrawTiresReflection_LinkPrimitive(struct DrawTiresScratch *scratch, POLY_FT4 *p, int selectedOTSlot)
{
	int otRangeStart = scratch->otRangeStart;
	int otRangeEnd = scratch->otRangeEnd;

	if ((otRangeStart - selectedOTSlot) > 0)
	{
		selectedOTSlot = otRangeStart;
	}

	if ((otRangeEnd - selectedOTSlot) < 0)
	{
		selectedOTSlot = otRangeEnd;
	}

	u32 *otSlot = (u32 *)(u32)selectedOTSlot;
	p->tag = CtrGpu_PackOTTag(*otSlot, 0x09000000);
	*otSlot = (u32)CtrGpu_PrimToOTLink24(p);
}

static void DrawTiresReflection_EmitProjectedWheel(struct DrawTiresScratch *scratch, struct DrawTiresReflectionProjectedWheel *selected,
                                                   struct PrimMem *primMem, int *primCount, int wheelIndex)
{
	POLY_FT4 *p = (POLY_FT4 *)primMem->cursor;
	struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
	int selectedOTSlot = selected->selectedOTSlot;
	int sxy[4];

	CtrGpu_WriteColorCode(&p->r0, scratch->tireColor);

	if (selected->wheelSprite == 0)
	{
		return;
	}

	DrawTiresReflection_CopyIconUV(p, selected->wheelSprite);

	if (DrawTiresReflection_ApplyCornerOrder(scratch, selected->jumpIndex, &selectedOTSlot, sxy) == 0)
	{
		return;
	}

	int splitDelta = scratch->splitCameraY - wheelLocal->center.y;
	if (splitDelta < 0)
	{
		return;
	}

	DrawTiresReflection_WritePrimitiveCorners(p, sxy);
	DrawTiresReflection_LinkPrimitive(scratch, p, selectedOTSlot);
	primMem->cursor = (char *)primMem->cursor + sizeof(POLY_FT4);
	(*primCount)++;
}

static int DrawTiresReflection_ProjectWheelQuads(struct DrawTiresScratch *scratch, struct PrimMem *primMem, int *primCount)
{
	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Reflection walks
	// the projection loop with s7/t8/t9 scratchpad cursors from 0x8006f5b4
	// onward. Native uses explicit scratch offsets while preserving the
	// reflected SXY scratch order used by the later jump-table primitive path.
	for (int wheelIndex = 3; wheelIndex >= 0; wheelIndex--)
	{
		DrawTiresReflection_LoadCorner(scratch, 0, wheelIndex, -1, -1);
		DrawTiresReflection_LoadCorner(scratch, 1, wheelIndex, 1, -1);
		DrawTiresReflection_LoadCorner(scratch, 2, wheelIndex, -1, 1);

		gte_rtpt_b();
		scratch->projectedSxy[1] = MFC2(12);
		scratch->projectedSxy[0] = MFC2(13);
		scratch->projectedSxy[3] = MFC2(14);

		DrawTiresReflection_LoadCorner(scratch, 0, wheelIndex, 1, 1);
		gte_rtps_b();
		scratch->projectedSxy[2] = MFC2(14);

		struct DrawTiresReflectionProjectedWheel projectedWheel = DrawTiresReflection_SelectProjectedWheel(scratch, wheelIndex);
		DrawTiresReflection_EmitProjectedWheel(scratch, &projectedWheel, primMem, primCount, wheelIndex);
	}

	return 1;
}

static int DrawTiresReflection_StagePlayer(struct DrawTiresScratch *scratch, struct Driver *driver, struct Instance *inst, int playerIndex,
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
	{
		return 0;
	}

	if ((flags & 0x4000) == 0)
	{
		return 0;
	}

	if ((idpp->lodIndex - scratch->lodThreshold) > 0)
	{
		return 0;
	}

	if (pb == 0)
	{
		return 0;
	}

	scratch->wheelSprites = driver->wheelSprites;
	scratch->tireColor = ((flags & PUSHBUFFER_EXISTS) != 0) ? 0x2e808080 : driver->tireColor;

	DrawTiresReflection_BuildWheelLocalPairs(scratch, driver, inst, idpp);
	DrawTiresReflection_SetupGteState(scratch, inst, idpp, pb);
	DrawTiresReflection_BuildWheelAxes(scratch);
	DrawTiresReflection_SetupProjectionState(pb);
	DrawTiresReflection_ProjectWheelQuads(scratch, primMem, primCount);

	return 1;
}

void DrawTires_Reflection(struct Thread *thread, struct PrimMem *primMem, u8 numPlyr)
{
	// Native uses the explicit DrawTiresScratch stack/scratch ABI.
	struct DrawTiresScratch scratch = {0};

	// NOTE(aalhendi): PSX-backfeed blocker: retail DrawTires_Reflection owns
	// scratchpad 0x1f800000 and live s7/t8/t9/v0/s0 register cursors. Native uses
	// offset-checked stack state and explicit helpers; PSX backfeed must restore
	// the retail scratchpad/register entry protocol.
	if (primMem == 0)
	{
		return;
	}

	int primCount = primMem->primitiveCount;
	DrawTiresReflection_InitScratch(&scratch, numPlyr);

	for (struct Thread *currThread = thread; currThread != 0; currThread = currThread->siblingThread)
	{
		struct Driver *driver = (struct Driver *)currThread->object;
		struct Instance *inst = currThread->inst;

		if (driver == 0 || inst == 0)
		{
			continue;
		}

		for (int playerIndex = 0; playerIndex < (int)(u8)numPlyr; playerIndex++)
		{
			if (DrawTiresReflection_StagePlayer(&scratch, driver, inst, playerIndex, primMem, &primCount) == 0)
			{
				continue;
			}
		}
	}

	primMem->primitiveCount = primCount;
}
