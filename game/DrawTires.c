#include <common.h>
#ifndef CTR_NATIVE
#include "DrawTires_Retail.inc"
#endif

// NOTE(aalhendi): Index 128 reads the 0x10 endpoint word at 0x8008a404.
// Its remaining three bytes are retained so the complete table range is owned.
static const u8 sDrawTiresSpriteIndexTable[0x84] = {
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x10, 0x10, 0,    0,    0,
};

#ifdef CTR_NATIVE
#include "DrawTires_Sqrt.c"

static s32 DrawTires_MulShift(s32 lhs, s32 rhs, u32 shift)
{
	return CTR_MipsSra(CTR_MipsMulLo(lhs, rhs), shift);
}

static s32 DrawTires_ReciprocalLength(s32 length)
{
	// NOTE(aalhendi): Retail's raw div/divu instructions have no C divide
	// guard. A zero square-root result leaves LO at 0xffffffff; nonzero
	// helper results are positive halfwords for both division variants.
	return length == 0 ? -1 : (s32)(0x10000u / (u32)length);
}

static const u32 sDrawTiresSolidJumpTable[8] = {
    0x8006ed7c, 0x8006ed98, 0x8006edb4, 0x8006edcc, 0x8006ede4, 0x8006ee00, 0x8006ee1c, 0x8006ee3c,
};

struct DrawTiresSolidProjectedWheel
{
	struct Icon *wheelSprite;
	s32 selectedOT;
	s32 selectedOTSlot;
	s32 jumpIndex;
};

static s32 DrawTiresSolid_ReadMatrixWord(MATRIX *matrix, s32 offset)
{
	return (s32)CTR_ReadU32LE((u8 *)matrix + offset);
}

static struct TrigPair DrawTiresSolid_TrigAngleSinCos(s32 angle)
{
	struct TrigPair pair;

	TRIG_AngleSinCos_r9r8r10(angle, &pair.sin, &pair.cos);

	return pair;
}

static s32 DrawTiresSolid_GetLodThreshold(u8 numPlyr)
{
	return (((s32)numPlyr - 2) > 0) ? 0 : 2;
}

static struct InstDrawPerPlayer *DrawTiresSolid_GetIdpp(struct Instance *inst, s32 playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)INST_GETIDPP(inst) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static void DrawTiresSolid_InitScratch(struct DrawTiresScratch *scratch, u8 numPlyr)
{
	scratch->numPlyr = numPlyr;
	scratch->lodThreshold = DrawTiresSolid_GetLodThreshold(numPlyr);

	// NOTE(aalhendi): Native retains the retail jump-address scratch image,
	// but dispatches its corner ordering with the switch below.
	for (s32 i = 0; i < 8; i++)
	{
		scratch->jumpTable[i] = sDrawTiresSolidJumpTable[i];
	}
}

static void DrawTiresSolid_AddHazardOffset(struct DrawTiresPackedVec3 *rim, s32 angle, s32 shift)
{
	struct TrigPair spin = DrawTiresSolid_TrigAngleSinCos(angle);

	rim->y = (s16)(rim->y + (spin.cos >> shift));
	rim->z.halves.lo = (s16)(rim->z.halves.lo + (spin.sin >> shift));
}

static void DrawTiresSolid_BuildWheelLocalPairs(struct DrawTiresScratch *scratch, struct Driver *driver, struct Instance *inst, struct InstDrawPerPlayer *idpp)
{
	s32 wheelX = DrawTires_MulShift(inst->scale.x, 0x90, 12);
	s32 negWheelX = -wheelX;
	s32 hazardShift = 9;

	scratch->wheelLocal[0].center.x = wheelX;
	scratch->wheelLocal[2].center.x = wheelX;
	scratch->wheelLocal[1].center.x = negWheelX;
	scratch->wheelLocal[3].center.x = negWheelX;
	scratch->wheelLocal[0].rim.x = wheelX - 0x1000;
	scratch->wheelLocal[2].rim.x = wheelX - 0x1000;
	scratch->wheelLocal[1].rim.x = negWheelX + 0x1000;
	scratch->wheelLocal[3].rim.x = negWheelX + 0x1000;

	s32 wheelY = DrawTires_MulShift(inst->scale.y, 0x40, 12);
	for (s32 wheelIndex = 0; wheelIndex < 4; wheelIndex++)
	{
		scratch->wheelLocal[wheelIndex].center.y = wheelY;
		scratch->wheelLocal[wheelIndex].rim.y = 0;
	}

	s32 wheelFrontZ = DrawTires_MulShift(inst->scale.z, 0xc7, 12);
	scratch->wheelLocal[0].center.z.word = wheelFrontZ;
	scratch->wheelLocal[1].center.z.word = wheelFrontZ;

	s32 wheelRearZ = DrawTires_MulShift(inst->scale.z, -0x60, 12);
	scratch->wheelLocal[2].center.z.word = wheelRearZ;
	scratch->wheelLocal[2].rim.z.word = wheelRearZ;
	scratch->wheelLocal[3].center.z.word = wheelRearZ;
	scratch->wheelLocal[3].rim.z.word = wheelRearZ;

	scratch->otRangeNormal = idpp->otRangeNormal;
	scratch->otRangeSecondary = idpp->otRangeSecondary;
	scratch->wheelSize.word = (s16)driver->wheelSize;

	struct TrigPair steering = DrawTiresSolid_TrigAngleSinCos(CTR_MipsSll(driver->wheelRotation, 2));
	scratch->wheelLocal[0].rim.x = scratch->wheelLocal[0].center.x - steering.cos;
	scratch->wheelLocal[0].rim.z.word = scratch->wheelLocal[0].center.z.word + steering.sin;
	scratch->wheelLocal[1].rim.x = scratch->wheelLocal[1].center.x + steering.cos;
	scratch->wheelLocal[1].rim.z.word = scratch->wheelLocal[1].center.z.word - steering.sin;

	s32 hazardAngle = CTR_MipsSll(driver->hazardTimer, 5);
	if ((driver->hazardTimer & 1) != 0)
	{
		hazardShift = 6;
		hazardAngle = CTR_MipsSll(hazardAngle, 1);
	}

	DrawTiresSolid_AddHazardOffset(&scratch->wheelLocal[0].rim, hazardAngle, hazardShift);
	DrawTiresSolid_AddHazardOffset(&scratch->wheelLocal[2].rim, hazardAngle + 0x400, hazardShift);
	DrawTiresSolid_AddHazardOffset(&scratch->wheelLocal[1].rim, hazardAngle + 0x800, hazardShift);
	DrawTiresSolid_AddHazardOffset(&scratch->wheelLocal[3].rim, hazardAngle + 0xc00, hazardShift);
}

static void DrawTiresSolid_SetupGteState(struct DrawTiresScratch *scratch, struct Instance *inst, struct InstDrawPerPlayer *idpp, struct PushBuffer *pb)
{
	s32 relX = CTR_MipsSll(CTR_MipsSubLo(inst->matrix.t[0], pb->pos.x), 2);
	s32 relY = CTR_MipsSll(CTR_MipsSubLo(inst->matrix.t[1], pb->pos.y), 2);
	s32 relZ = CTR_MipsSll(CTR_MipsSubLo(inst->matrix.t[2], pb->pos.z), 2);
	s32 splitCameraY = CTR_MipsSll(CTR_MipsSubLo(scratch->vertSplit, pb->pos.y), 2);

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
	// NOTE(aalhendi): Retail's s7/t8 cursors walk these records backwards.
	for (s32 wheelIndex = 3; wheelIndex >= 0; wheelIndex--)
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
		s32 centerX = MFC2_S(9);
		s32 centerY = MFC2_S(10);
		s32 centerZ = MFC2_S(11);

		gte_sqr0_b();
		wheelLocal->center.x = centerX;
		wheelLocal->center.y = centerY;
		s32 len = Unknown_8006ef98(CTR_MipsAddLo(CTR_MipsAddLo(MFC2_S(25), MFC2_S(26)), MFC2_S(27)));

		gte_lcv1_b();
		wheelLocal->center.z.word = centerZ;
		s32 invLen = CTR_MipsNegLo(DrawTires_ReciprocalLength(len));

		s32 rimX = MFC2_S(9);
		s32 rimY = MFC2_S(10);
		s32 rimZ = MFC2_S(11);

		transformedRim->x = rimX;
		s32 normalX = DrawTires_MulShift(centerX, invLen, 4);
		viewNormal->x = normalX;

		s32 normalY = DrawTires_MulShift(centerY, invLen, 4);
		transformedRim->y = rimY;
		viewNormal->y = normalY;

		s32 normalZ = DrawTires_MulShift(centerZ, invLen, 4);
		transformedRim->z.word = rimZ;
		viewNormal->z.word = normalZ;

		CTC2(normalX & 0xffff, 0);
		CTC2(normalY & 0xffff, 2);
		CTC2(normalZ, 4);

		gte_op12_b();
		s32 axisX = MFC2_S(25);
		s32 axisY = MFC2_S(26);
		s32 axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(CTR_MipsAddLo(CTR_MipsAddLo(MFC2_S(25), MFC2_S(26)), MFC2_S(27)));
		invLen = DrawTires_ReciprocalLength(len);

		axisX = DrawTires_MulShift(axisX, invLen, 4);
		MTC2(axisX, 9);
		axisY = DrawTires_MulShift(axisY, invLen, 4);
		MTC2(axisY, 10);
		axisZ = DrawTires_MulShift(axisZ, invLen, 4);
		MTC2(axisZ, 11);

		s32 wheelSize = scratch->wheelSize.word;
		gte_op12_b();

		axisA->x = DrawTires_MulShift(axisX, wheelSize, 0x12);
		axisA->y = DrawTires_MulShift(axisY, wheelSize, 0x12);
		axisA->z = DrawTires_MulShift(axisZ, wheelSize, 0x12);

		axisX = MFC2_S(25);
		axisY = MFC2_S(26);
		axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(CTR_MipsAddLo(CTR_MipsAddLo(MFC2_S(25), MFC2_S(26)), MFC2_S(27)));
		invLen = DrawTires_MulShift(wheelSize, CTR_MipsNegLo(DrawTires_ReciprocalLength(len)), 12);

		axisB->x = DrawTires_MulShift(axisX, invLen, 10);
		axisB->y = DrawTires_MulShift(axisY, invLen, 10);
		axisB->z = DrawTires_MulShift(axisZ, invLen, 10);
	}
}

static void DrawTiresSolid_SetupProjectionState(struct PushBuffer *pb)
{
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0x0), 0);
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0x4), 1);
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0x8), 2);
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0xc), 3);
	CTC2(DrawTiresSolid_ReadMatrixWord(&pb->matrix_ViewProj, 0x10), 4);

	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	CTC2(CTR_MipsSll(pb->rect.w, 0xf), 24);
	CTC2(CTR_MipsSll(pb->rect.h, 0xf), 25);
	CTC2(pb->distanceToScreen_PREV, 26);
}

static void DrawTiresSolid_LoadCorner(struct DrawTiresScratch *scratch, s32 vectorIndex, s32 wheelIndex, s32 axisSign, s32 rimSign)
{
	struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
	SVec3Slot *axisA = &scratch->tireAxisA[wheelIndex];
	SVec3Slot *axisB = &scratch->tireAxisB[wheelIndex];
	s32 x = wheelLocal->center.x + (axisSign * axisA->x) + (rimSign * axisB->x);
	s32 y = wheelLocal->center.y + (axisSign * axisA->y) + (rimSign * axisB->y);
	s32 z = wheelLocal->center.z.halves.lo + (axisSign * axisA->z) + (rimSign * axisB->z);

	MTC2(CTR_PackS16Pair(x, y), vectorIndex * 2);
	MTC2(z, (vectorIndex * 2) + 1);
}

static s32 DrawTiresSolid_SelectSpriteIndex(s32 angleValue)
{
	s32 tableIndex = angleValue >> 5;

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

static struct DrawTiresSolidProjectedWheel DrawTiresSolid_SelectProjectedWheel(struct DrawTiresScratch *scratch, s32 wheelIndex)
{
	struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
	struct DrawTiresPackedVec3 *viewNormal = &scratch->viewNormalVectors[wheelIndex];
	struct DrawTiresPackedVec3 *transformedRim = &scratch->transformedRimVectors[wheelIndex];
	s32 selectedOT = scratch->otRangeNormal;
	s32 splitDelta = scratch->splitCameraY - wheelLocal->center.y;
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

	scratch->otRangeStart = CTR_MipsAddLo(scratch->depthOffsetStartBytes, selectedOT);
	scratch->otRangeEnd = CTR_MipsAddLo(scratch->depthOffsetEndBytes, selectedOT);

	gte_llv0_b();
	u32 depthValue = MFC2(24);
	selected.selectedOT = selectedOT;
	selected.selectedOTSlot = CTR_MipsAddLo(selectedOT, (s32)((depthValue >> 0x11) << 2));
	s32 angleValue = MFC2_S(9);
	s32 spriteIndex = DrawTiresSolid_SelectSpriteIndex(angleValue);
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

static s32 DrawTiresSolid_ApplyCornerOrder(struct DrawTiresScratch *scratch, s32 jumpIndex, s32 *selectedOTSlot, s32 sxy[4])
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

static void DrawTires_WriteRemainingPrimitiveCorners(POLY_FT4 *p, s32 sxy[4])
{
	CtrGpu_WritePackedXY(&p->x1, (u32)sxy[1]);
	CtrGpu_WritePackedXY(&p->x2, (u32)sxy[2]);
	CtrGpu_WritePackedXY(&p->x3, (u32)sxy[3]);
}

static void DrawTires_LinkPrimitive(struct DrawTiresScratch *scratch, POLY_FT4 *p, s32 selectedOTSlot)
{
	s32 otRangeStart = scratch->otRangeStart;
	s32 otRangeEnd = scratch->otRangeEnd;

	s32 startDelta = CTR_MipsSubLo(otRangeStart, selectedOTSlot);
	// NOTE(aalhendi): The end delta is computed in retail's branch delay
	// slot before the start clamp, including when the bounds are reversed.
	s32 endDelta = CTR_MipsSubLo(otRangeEnd, selectedOTSlot);
	if (startDelta > 0)
	{
		selectedOTSlot = otRangeStart;
	}

	if (endDelta < 0)
	{
		selectedOTSlot = otRangeEnd;
	}

	u32 *otSlot = (u32 *)(u32)selectedOTSlot;
	p->tag = CtrGpu_PackOTTag(*otSlot, 0x09000000);
	*otSlot = (u32)CtrGpu_PrimToOTLink24(p);
}

static s32 DrawTiresSolid_EmitProjectedWheel(struct DrawTiresScratch *scratch, struct DrawTiresSolidProjectedWheel *selected, struct PrimMem *primMem,
                                             s32 *primCount)
{
	POLY_FT4 *p = (POLY_FT4 *)primMem->cursor;
	s32 selectedOTSlot = selected->selectedOTSlot;
	s32 sxy[4];

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

	// NOTE(aalhendi): The retail culling branch writes corner 0 in its delay
	// slot even when the unlinked packet is rejected.
	CtrGpu_WritePackedXY(&p->x0, (u32)sxy[0]);
	if (scratch->otRangeSecondary == selected->selectedOT && (scratch->instFlags & 0x4000) != 0)
	{
		return 0;
	}

	DrawTires_WriteRemainingPrimitiveCorners(p, sxy);
	DrawTires_LinkPrimitive(scratch, p, selectedOTSlot);
	primMem->cursor = (char *)primMem->cursor + sizeof(POLY_FT4);
	(*primCount)++;

	return 1;
}

static s32 DrawTiresSolid_ProjectWheelQuads(struct DrawTiresScratch *scratch, struct PrimMem *primMem, s32 *primCount)
{
	for (s32 wheelIndex = 3; wheelIndex >= 0; wheelIndex--)
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

static s32 DrawTiresSolid_StagePlayer(struct DrawTiresScratch *scratch, struct Driver *driver, struct Instance *inst, s32 playerIndex, struct PrimMem *primMem,
                                      s32 *primCount)
{
	struct InstDrawPerPlayer *idpp = DrawTiresSolid_GetIdpp(inst, playerIndex);
	struct PushBuffer *pb = idpp->pushBuffer;
	s32 flags = idpp->instFlags;

	scratch->playerCounter = scratch->numPlyr - playerIndex;
	scratch->vertSplit = inst->vertSplit;
	scratch->depthOffsetStartBytes = CTR_MipsSll(idpp->depthOffset[0], 2);
	scratch->depthOffsetEndBytes = CTR_MipsSll(idpp->depthOffset[1], 2);
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
	scratch->otRangeStart = CTR_MipsAddLo(scratch->depthOffsetStartBytes, idpp->otRangeNormal);
	scratch->otRangeEnd = CTR_MipsAddLo(scratch->depthOffsetEndBytes, idpp->otRangeNormal);

	DrawTiresSolid_BuildWheelLocalPairs(scratch, driver, inst, idpp);
	DrawTiresSolid_SetupGteState(scratch, inst, idpp, pb);
	DrawTiresSolid_BuildWheelAxes(scratch);
	DrawTiresSolid_SetupProjectionState(pb);
	DrawTiresSolid_ProjectWheelQuads(scratch, primMem, primCount);

	return 1;
}

void DrawTires_Solid(struct Thread *thread, struct PrimMem *primMem, u8 numPlyr)
{
	// NOTE(aalhendi): Keep the retail scratch writes and untouched padding in
	// the native scratchpad too. Only the default entry saves MIPS GPRs in the
	// first 0x30 bytes; native has no corresponding register-save image.
	struct DrawTiresScratch *scratch = CTR_SCRATCHPAD_PTR(struct DrawTiresScratch, 0);
	if (primMem == 0)
	{
		return;
	}

	s32 primCount = primMem->primitiveCount;
	DrawTiresSolid_InitScratch(scratch, numPlyr);

	for (struct Thread *currThread = thread; currThread != 0; currThread = currThread->siblingThread)
	{
		struct Driver *driver = (struct Driver *)currThread->object;
		struct Instance *inst = currThread->inst;

		if (driver == 0 || inst == 0)
		{
			continue;
		}

		// NOTE(aalhendi): Retail enters its player loop once even for zero.
		for (s32 playerIndex = 0; playerIndex < (numPlyr == 0 ? 1 : (s32)numPlyr); playerIndex++)
		{
			DrawTiresSolid_StagePlayer(scratch, driver, inst, playerIndex, primMem, &primCount);
			scratch->playerCounter--;
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
	s32 selectedOT;
	s32 selectedOTSlot;
	s32 jumpIndex;
};

static s32 DrawTiresReflection_GetLodThreshold(u8 numPlyr)
{
	return (((s32)numPlyr - 2) > 0) ? 0 : 2;
}

static struct InstDrawPerPlayer *DrawTiresReflection_GetIdpp(struct Instance *inst, s32 playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)INST_GETIDPP(inst) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static s32 DrawTiresReflection_ReadMatrixWord(MATRIX *matrix, s32 offset)
{
	return (s32)CTR_ReadU32LE((u8 *)matrix + offset);
}

static void DrawTiresReflection_InitScratch(struct DrawTiresScratch *scratch, u8 numPlyr)
{
	scratch->numPlyr = numPlyr;
	scratch->lodThreshold = DrawTiresReflection_GetLodThreshold(numPlyr);

	// NOTE(aalhendi): Preserve the reflection jump-address scratch image;
	// native dispatch uses the reflected corner-order switch below.
	for (s32 i = 0; i < 8; i++)
	{
		scratch->jumpTable[i] = sDrawTiresReflectionJumpTable[i];
	}
}

static void DrawTiresReflection_AddHazardOffset(struct DrawTiresPackedVec3 *rim, s32 angle, s32 shift)
{
	struct TrigPair spin = DrawTiresSolid_TrigAngleSinCos(angle);

	rim->y = (s16)(rim->y + (spin.cos >> shift));
	rim->z.halves.lo = (s16)(rim->z.halves.lo + (spin.sin >> shift));
}

static void DrawTiresReflection_BuildWheelLocalPairs(struct DrawTiresScratch *scratch, struct Driver *driver, struct Instance *inst,
                                                     struct InstDrawPerPlayer *idpp)
{
	s32 hazardShift = 9;

	// NOTE(aalhendi): Retail loads Instance.scale[0] at 0x8006f0e0, then the
	// push-buffer branch delay slot at
	// 0x8006f0e8 overwrites v1 with instFlags & 0x100 before the multiply at
	// 0x8006f10c. Keep that observed register lineage, not the unused scale.
	s32 wheelX = DrawTires_MulShift(scratch->instFlags & PUSHBUFFER_EXISTS, 0x90, 12);
	s32 negWheelX = -wheelX;

	scratch->wheelLocal[0].center.x = wheelX;
	scratch->wheelLocal[2].center.x = wheelX;
	scratch->wheelLocal[1].center.x = negWheelX;
	scratch->wheelLocal[3].center.x = negWheelX;
	scratch->wheelLocal[0].rim.x = wheelX - 0x1000;
	scratch->wheelLocal[2].rim.x = wheelX - 0x1000;
	scratch->wheelLocal[1].rim.x = negWheelX + 0x1000;
	scratch->wheelLocal[3].rim.x = negWheelX + 0x1000;

	s32 wheelY = DrawTires_MulShift(inst->scale.y, 0x40, 12);
	for (s32 wheelIndex = 0; wheelIndex < 4; wheelIndex++)
	{
		scratch->wheelLocal[wheelIndex].center.y = wheelY;
		scratch->wheelLocal[wheelIndex].rim.y = 0;
	}

	s32 wheelFrontZ = DrawTires_MulShift(inst->scale.z, 0xc7, 12);
	scratch->wheelLocal[0].center.z.word = wheelFrontZ;
	scratch->wheelLocal[1].center.z.word = wheelFrontZ;

	s32 wheelRearZ = DrawTires_MulShift(inst->scale.z, -0x60, 12);
	scratch->wheelLocal[2].center.z.word = wheelRearZ;
	scratch->wheelLocal[2].rim.z.word = wheelRearZ;
	scratch->wheelLocal[3].center.z.word = wheelRearZ;
	scratch->wheelLocal[3].rim.z.word = wheelRearZ;

	scratch->otRangeNormal = idpp->otRangeNormal;
	scratch->otRangeSecondary = idpp->otRangeSecondary;
	scratch->wheelSize.word = (s16)driver->wheelSize;

	struct TrigPair steering = DrawTiresSolid_TrigAngleSinCos(CTR_MipsSll(driver->wheelRotation, 2));
	scratch->wheelLocal[0].rim.x = scratch->wheelLocal[0].center.x - steering.cos;
	scratch->wheelLocal[0].rim.z.word = scratch->wheelLocal[0].center.z.word + steering.sin;
	scratch->wheelLocal[1].rim.x = scratch->wheelLocal[1].center.x + steering.cos;
	scratch->wheelLocal[1].rim.z.word = scratch->wheelLocal[1].center.z.word - steering.sin;

	s32 hazardAngle = CTR_MipsSll(driver->hazardTimer, 5);
	if ((driver->hazardTimer & 1) != 0)
	{
		hazardShift = 6;
		hazardAngle = CTR_MipsSll(hazardAngle, 1);
	}

	DrawTiresReflection_AddHazardOffset(&scratch->wheelLocal[0].rim, hazardAngle, hazardShift);
	DrawTiresReflection_AddHazardOffset(&scratch->wheelLocal[2].rim, hazardAngle + 0x400, hazardShift);
	DrawTiresReflection_AddHazardOffset(&scratch->wheelLocal[1].rim, hazardAngle + 0x800, hazardShift);
	DrawTiresReflection_AddHazardOffset(&scratch->wheelLocal[3].rim, hazardAngle + 0xc00, hazardShift);
}

static void DrawTiresReflection_SetupGteState(struct DrawTiresScratch *scratch, struct Instance *inst, struct InstDrawPerPlayer *idpp, struct PushBuffer *pb)
{
	s32 relX = CTR_MipsSll(CTR_MipsSubLo(inst->matrix.t[0], pb->pos.x), 2);
	s32 relY = CTR_MipsSll(CTR_MipsSubLo(inst->matrix.t[1], pb->pos.y), 2);
	s32 relZ = CTR_MipsSll(CTR_MipsSubLo(inst->matrix.t[2], pb->pos.z), 2);
	s32 splitCameraY = CTR_MipsSll(CTR_MipsSubLo(scratch->vertSplit, pb->pos.y), 2);

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
	// NOTE(aalhendi): Mirror Y after lcv0tr, but square its original GTE IR
	// value. Retail only changes the CPU copy before the square-root call.
	for (s32 wheelIndex = 3; wheelIndex >= 0; wheelIndex--)
	{
		struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
		struct DrawTiresPackedVec3 *viewNormal = &scratch->viewNormalVectors[wheelIndex];
		struct DrawTiresPackedVec3 *transformedRim = &scratch->transformedRimVectors[wheelIndex];
		SVec3Slot *axisA = &scratch->tireAxisA[wheelIndex];
		SVec3Slot *axisB = &scratch->tireAxisB[wheelIndex];
		s32 splitCameraY = scratch->splitCameraY;

		MTC2(CTR_PackS16Pair(wheelLocal->center.x, wheelLocal->center.y), 0);
		MTC2(wheelLocal->center.z.word, 1);
		MTC2(CTR_PackS16Pair(wheelLocal->rim.x, wheelLocal->rim.y), 2);
		MTC2(wheelLocal->rim.z.word, 3);

		gte_lcv0tr_b();
		s32 centerX = MFC2_S(9);
		s32 originalCenterY = MFC2_S(10);
		s32 centerZ = MFC2_S(11);
		s32 centerY = splitCameraY - (originalCenterY - splitCameraY);

		gte_sqr0_b();
		wheelLocal->center.x = centerX;
		wheelLocal->center.y = centerY;
		s32 len = Unknown_8006ef98(CTR_MipsAddLo(CTR_MipsAddLo(MFC2_S(25), MFC2_S(26)), MFC2_S(27)));

		gte_lcv1_b();
		wheelLocal->center.z.word = centerZ;
		s32 invLen = CTR_MipsNegLo(DrawTires_ReciprocalLength(len));

		s32 rimX = MFC2_S(9);
		s32 rimY = -MFC2_S(10);
		s32 rimZ = MFC2_S(11);

		transformedRim->x = rimX;
		s32 normalX = DrawTires_MulShift(centerX, invLen, 4);
		viewNormal->x = normalX;

		s32 normalY = DrawTires_MulShift(centerY, invLen, 4);
		transformedRim->y = rimY;
		viewNormal->y = normalY;

		s32 normalZ = DrawTires_MulShift(centerZ, invLen, 4);
		transformedRim->z.word = rimZ;
		viewNormal->z.word = normalZ;

		CTC2(normalX & 0xffff, 0);
		CTC2(normalY & 0xffff, 2);
		CTC2(normalZ, 4);

		gte_op12_b();
		s32 axisX = MFC2_S(25);
		s32 axisY = MFC2_S(26);
		s32 axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(CTR_MipsAddLo(CTR_MipsAddLo(MFC2_S(25), MFC2_S(26)), MFC2_S(27)));
		invLen = DrawTires_ReciprocalLength(len);

		axisX = DrawTires_MulShift(axisX, invLen, 4);
		MTC2(axisX, 9);
		axisY = DrawTires_MulShift(axisY, invLen, 4);
		MTC2(axisY, 10);
		axisZ = DrawTires_MulShift(axisZ, invLen, 4);
		MTC2(axisZ, 11);

		s32 wheelSize = scratch->wheelSize.word;
		gte_op12_b();

		axisA->x = DrawTires_MulShift(axisX, wheelSize, 0x12);
		axisA->y = DrawTires_MulShift(axisY, wheelSize, 0x12);
		axisA->z = DrawTires_MulShift(axisZ, wheelSize, 0x12);

		axisX = MFC2_S(25);
		axisY = MFC2_S(26);
		axisZ = MFC2_S(27);

		gte_sqr0_b();
		len = Unknown_8006ef98(CTR_MipsAddLo(CTR_MipsAddLo(MFC2_S(25), MFC2_S(26)), MFC2_S(27)));
		invLen = DrawTires_MulShift(wheelSize, CTR_MipsNegLo(DrawTires_ReciprocalLength(len)), 12);

		axisB->x = DrawTires_MulShift(axisX, invLen, 10);
		axisB->y = DrawTires_MulShift(axisY, invLen, 10);
		axisB->z = DrawTires_MulShift(axisZ, invLen, 10);
	}
}

static void DrawTiresReflection_SetupProjectionState(struct PushBuffer *pb)
{
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0x0), 0);
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0x4), 1);
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0x8), 2);
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0xc), 3);
	CTC2(DrawTiresReflection_ReadMatrixWord(&pb->matrix_ViewProj, 0x10), 4);

	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	CTC2(CTR_MipsSll(pb->rect.w, 0xf), 24);
	CTC2(CTR_MipsSll(pb->rect.h, 0xf), 25);
	CTC2(pb->distanceToScreen_PREV, 26);
}

static void DrawTiresReflection_LoadCorner(struct DrawTiresScratch *scratch, s32 vectorIndex, s32 wheelIndex, s32 axisSign, s32 rimSign)
{
	struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
	SVec3Slot *axisA = &scratch->tireAxisA[wheelIndex];
	SVec3Slot *axisB = &scratch->tireAxisB[wheelIndex];
	s32 x = wheelLocal->center.x + (axisSign * axisA->x) + (rimSign * axisB->x);
	s32 y = wheelLocal->center.y + (axisSign * axisA->y) + (rimSign * axisB->y);
	s32 z = wheelLocal->center.z.halves.lo + (axisSign * axisA->z) + (rimSign * axisB->z);

	MTC2(CTR_PackS16Pair(x, y), vectorIndex * 2);
	MTC2(z, (vectorIndex * 2) + 1);
}

static struct DrawTiresReflectionProjectedWheel DrawTiresReflection_SelectProjectedWheel(struct DrawTiresScratch *scratch, s32 wheelIndex)
{
	struct DrawTiresPackedVec3 *viewNormal = &scratch->viewNormalVectors[wheelIndex];
	struct DrawTiresPackedVec3 *transformedRim = &scratch->transformedRimVectors[wheelIndex];
	s32 selectedOT = scratch->otRangeSecondary;
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

	scratch->otRangeStart = CTR_MipsAddLo(scratch->depthOffsetStartBytes, selectedOT);
	scratch->otRangeEnd = CTR_MipsAddLo(scratch->depthOffsetEndBytes, selectedOT);

	gte_llv0_b();
	selected.selectedOTSlot = CTR_MipsAddLo(selectedOT, (s32)((depthValue >> 0x11) << 2));
	s32 angleValue = MFC2_S(9);
	s32 spriteIndex = DrawTiresSolid_SelectSpriteIndex(angleValue);
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

static s32 DrawTiresReflection_ApplyCornerOrder(struct DrawTiresScratch *scratch, s32 jumpIndex, s32 *selectedOTSlot, s32 sxy[4])
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

static void DrawTiresReflection_EmitProjectedWheel(struct DrawTiresScratch *scratch, struct DrawTiresReflectionProjectedWheel *selected,
                                                   struct PrimMem *primMem, s32 *primCount, s32 wheelIndex)
{
	POLY_FT4 *p = (POLY_FT4 *)primMem->cursor;
	struct DrawTiresWheelLocal *wheelLocal = &scratch->wheelLocal[wheelIndex];
	s32 selectedOTSlot = selected->selectedOTSlot;
	s32 sxy[4];

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

	// NOTE(aalhendi): A rejected reflected wheel still writes corner 0 in
	// the retail branch delay slot; the other three corners remain untouched.
	CtrGpu_WritePackedXY(&p->x0, (u32)sxy[0]);
	s32 splitDelta = scratch->splitCameraY - wheelLocal->center.y;
	if (splitDelta < 0)
	{
		return;
	}

	DrawTires_WriteRemainingPrimitiveCorners(p, sxy);
	DrawTires_LinkPrimitive(scratch, p, selectedOTSlot);
	primMem->cursor = (char *)primMem->cursor + sizeof(POLY_FT4);
	(*primCount)++;
}

static s32 DrawTiresReflection_ProjectWheelQuads(struct DrawTiresScratch *scratch, struct PrimMem *primMem, s32 *primCount)
{
	// NOTE(aalhendi): Reflection stores a different SXY order before applying
	// the same eight corner-order cases.
	for (s32 wheelIndex = 3; wheelIndex >= 0; wheelIndex--)
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

static s32 DrawTiresReflection_StagePlayer(struct DrawTiresScratch *scratch, struct Driver *driver, struct Instance *inst, s32 playerIndex,
                                           struct PrimMem *primMem, s32 *primCount)
{
	struct InstDrawPerPlayer *idpp = DrawTiresReflection_GetIdpp(inst, playerIndex);
	struct PushBuffer *pb = idpp->pushBuffer;
	s32 flags = idpp->instFlags;

	scratch->playerCounter = scratch->numPlyr - playerIndex;
	scratch->vertSplit = inst->vertSplit;
	scratch->depthOffsetStartBytes = CTR_MipsSll(idpp->depthOffset[0], 2);
	scratch->depthOffsetEndBytes = CTR_MipsSll(idpp->depthOffset[1], 2);
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
	// NOTE(aalhendi): As in the solid path, preserve the shared scratch image
	// without synthesizing a native counterpart of the MIPS saved GPRs.
	struct DrawTiresScratch *scratch = CTR_SCRATCHPAD_PTR(struct DrawTiresScratch, 0);
	if (primMem == 0)
	{
		return;
	}

	s32 primCount = primMem->primitiveCount;
	DrawTiresReflection_InitScratch(scratch, numPlyr);

	for (struct Thread *currThread = thread; currThread != 0; currThread = currThread->siblingThread)
	{
		struct Driver *driver = (struct Driver *)currThread->object;
		struct Instance *inst = currThread->inst;

		if (driver == 0 || inst == 0)
		{
			continue;
		}

		for (s32 playerIndex = 0; playerIndex < (numPlyr == 0 ? 1 : (s32)numPlyr); playerIndex++)
		{
			DrawTiresReflection_StagePlayer(scratch, driver, inst, playerIndex, primMem, &primCount);
			scratch->playerCounter--;
		}
	}

	primMem->primitiveCount = primCount;
}
#endif
