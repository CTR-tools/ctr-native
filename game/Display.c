#include <common.h>

struct DisplayBlurFlatPacket
{
	u32 tag;
	u32 drawModeStart;
	u32 maskBitEnable;
	u32 colorAndCode;
	u32 xy0;
	u32 xy1;
	u32 xy2;
	u32 xy3;
	u32 drawModeEnd;
	u32 maskBitDisable;
};

CTR_STATIC_ASSERT(sizeof(struct DisplayBlurFlatPacket) == 0x28);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, tag) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, drawModeStart) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, maskBitEnable) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, colorAndCode) == 0x0C);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, xy0) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, xy1) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, xy2) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, xy3) == 0x1C);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, drawModeEnd) == 0x20);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurFlatPacket, maskBitDisable) == 0x24);

struct DisplayBlurTile
{
	s16 srcX;
	s16 srcY;
	s16 srcW;
	s16 srcH;
	s16 dstX;
	s16 dstY;
	s16 dstW;
	s16 dstH;
};

CTR_STATIC_ASSERT(sizeof(struct DisplayBlurTile) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurTile, srcX) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurTile, srcY) == 0x02);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurTile, srcW) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurTile, srcH) == 0x06);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurTile, dstX) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurTile, dstY) == 0x0A);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurTile, dstW) == 0x0C);
CTR_STATIC_ASSERT(offsetof(struct DisplayBlurTile, dstH) == 0x0E);

static u32 DISPLAY_Blur_PackS16Pair(int x, int y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80023a40-0x80023d4c
u32 *DISPLAY_Blur_SubFunc(u32 *prim, struct DisplayBlurTile *tile)
{
	int srcX = tile->srcX;
	int srcY = tile->srcY;
	int srcW = tile->srcW;
	int srcH = tile->srcH;

	if (((srcX + srcW) & -0x100) != (srcX & -0x100))
	{
		struct DisplayBlurTile *child = tile + 1;

		child->srcW = (s16)((srcX & -0x100) - (srcX - 0xff));
		child->dstW = (s16)((child->srcW * tile->dstW) / srcW);

		child->srcX = (s16)srcX;
		child->srcY = (s16)srcY;
		child->srcH = (s16)srcH;
		child->dstH = tile->dstH;
		child->dstX = tile->dstX;
		child->dstY = tile->dstY;

		if (tile->dstW != 0)
		{
			prim = DISPLAY_Blur_SubFunc(prim, child);
		}

		child->srcX = (s16)(child->srcW + child->srcX + 1);
		child->srcW = (s16)(tile->srcW - child->srcW - 1);
		child->dstX = (s16)(child->dstX + child->dstW);
		child->dstW = (s16)(tile->dstW - child->dstW);

		if (tile->dstW != 0)
		{
			prim = DISPLAY_Blur_SubFunc(prim, child);
		}

		return prim;
	}

	if (((srcY + srcH) & -0x100) != (srcY & -0x100))
	{
		struct DisplayBlurTile *child = tile + 1;

		child->srcH = (s16)((srcY & -0x100) - (srcY - 0xff));
		child->dstH = (s16)((child->srcH * tile->dstH) / srcH);

		child->srcX = (s16)srcX;
		child->srcY = (s16)srcY;
		child->srcW = (s16)srcW;
		child->dstW = tile->dstW;
		child->dstX = tile->dstX;
		child->dstY = tile->dstY;

		if (tile->dstH != 0)
		{
			prim = DISPLAY_Blur_SubFunc(prim, child);
		}

		child->srcY = (s16)(child->srcH + child->srcY + 1);
		child->srcH = (s16)(tile->srcH - child->srcH - 1);
		child->dstY = (s16)(child->dstY + child->dstH);
		child->dstH = (s16)(tile->dstH - child->dstH);

		if (tile->dstH != 0)
		{
			prim = DISPLAY_Blur_SubFunc(prim, child);
		}

		return prim;
	}

	u32 u0 = (u32)srcX & 0x3f;
	u32 v0 = ((u32)srcY & 0xff) << 8;
	u32 u1 = (u32)(srcW + (int)u0);
	u32 v1 = v0 + ((u32)srcH << 8);
	int dstX = tile->dstX;
	int dstY = tile->dstY;
	int dstW = tile->dstW;
	int dstH = tile->dstH;
	u32 tpage = (((u32)srcY & 0x100) >> 4) | (((u32)srcX & 0x3ff) >> 6) | 0x100 | (((u32)srcY & 0x200) << 2);
	POLY_FT4 *poly = (POLY_FT4 *)prim;

	CtrGpu_WritePackedUVWord(&poly->u0, u0 | v0);
	CtrGpu_WritePackedXY(&poly->x0, DISPLAY_Blur_PackS16Pair(dstX, dstY));
	CtrGpu_WritePackedXY(&poly->x1, DISPLAY_Blur_PackS16Pair(dstX + dstW, dstY));
	CtrGpu_WritePackedXY(&poly->x2, DISPLAY_Blur_PackS16Pair(dstX, dstY + dstH));
	CtrGpu_WritePackedXY(&poly->x3, DISPLAY_Blur_PackS16Pair(dstX + dstW, dstY + dstH));
	CtrGpu_WritePackedUV(&poly->u2, (u16)(u0 | v1));
	CtrGpu_WritePackedUV(&poly->u3, (u16)(u1 | v1));
	poly->code = 0x2f;
	poly->tag = CtrGpu_PackOTTag(CtrGpu_PrimToOTLink24(poly + 1), 0x09000000);
	CtrGpu_WritePackedUVWord(&poly->u1, u1 | v0 | (tpage << 16));

	return (u32 *)(poly + 1);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80023d4c-0x80023ffc
void DISPLAY_Blur_Main(struct PushBuffer *pb, int strength)
{
	struct GameTracker *gGT = sdata->gGT;
	struct DB *backBuffer = gGT->backBuffer;
	u32 *prim = backBuffer->primMem.cursor;
	u32 *nextPrim;
	s8 cameraID;
	uint32_t *ot;

	cameraID = *(s8 *)&pb->cameraID;

	if (strength < 1 || (((gGT->db[1 - gGT->swapchainIndex].blurCameraMask >> (cameraID & 0x1f)) & 1) == 0))
	{
		struct DisplayBlurFlatPacket *packet = (struct DisplayBlurFlatPacket *)prim;
		int x = pb->rect.x;
		int y = pb->rect.y;
		int w = pb->rect.w;
		int h = pb->rect.h;

		packet->drawModeEnd = 0xe1000a00;
		packet->drawModeStart = 0xe1000a20;
		packet->maskBitEnable = 0xe6000001;
		packet->maskBitDisable = 0xe6000000;
		packet->xy0 = DISPLAY_Blur_PackS16Pair(x, y);
		packet->xy1 = DISPLAY_Blur_PackS16Pair(x + w, y);
		packet->xy2 = DISPLAY_Blur_PackS16Pair(x, y + h);
		packet->xy3 = DISPLAY_Blur_PackS16Pair(x + w, y + h);
		packet->colorAndCode = (strength < 0) ? 0x2affffff : 0x2a000000;

		ot = gGT->otSwapchainDB[gGT->swapchainIndex];
		CtrGpu_LinkPacket24(ot, &packet->tag, packet, 0x09000000);
		nextPrim = (u32 *)(packet + 1);
	}
	else
	{
		int wave = gGT->timer + cameraID;
		struct DisplayBlurTile *scratch = CTR_SCRATCHPAD_PTR(struct DisplayBlurTile, 0);
		struct DisplayBlurTile *tile = &scratch[0];
		u32 oldTag;
		int blur;
		int insetX;
		int insetY;

		if ((cameraID & 1) != 0)
		{
			wave = -wave;
		}

		blur = MATH_Sin(wave * 100);
		if (blur < 0)
		{
			blur = -blur;
		}

		blur = (blur >> 2) + 0x400;
		if (strength < 0x1000)
		{
			blur = (blur * strength) >> 12;
		}

		ot = gGT->otSwapchainDB[gGT->swapchainIndex];
		oldTag = *ot;
		*ot = (uint32_t)CtrGpu_PrimToOTLink24(prim);

		tile->dstX = pb->rect.x;
		tile->dstY = pb->rect.y;
		tile->dstW = pb->rect.w;
		tile->dstH = pb->rect.h;

		insetX = ((blur * 9) >> 12) + 2;
		tile->srcX = backBuffer->dispEnv.disp.x + pb->rect.x + insetX;
		tile->srcW = pb->rect.w - (insetX * 2);

		insetY = ((blur * 6) >> 12) + 2;
		tile->srcY = backBuffer->dispEnv.disp.y + pb->rect.y + insetY;
		tile->srcH = pb->rect.h - (insetY * 2);

		nextPrim = DISPLAY_Blur_SubFunc(prim, tile);
		((POLY_FT4 *)nextPrim - 1)->tag = CtrGpu_PackOTTag(oldTag, 0x09000000);
	}

	backBuffer->primMem.cursor = nextPrim;
	backBuffer->blurCameraMask |= (u8)(1 << (cameraID & 0x1f));
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80023ffc-0x8002406c.
void DISPLAY_Swap(void)
{
	struct GameTracker *gGT;
	struct DB *db;

	gGT = sdata->gGT;

	// get pointer to the "new" frontBufferDB, which is current backBuffer
	db = &gGT->db[gGT->swapchainIndex];

	// flip swapchain index (0->1) (1->0)
	gGT->swapchainIndex = 1 - gGT->swapchainIndex;

	// Set value of frontBuffer DB
	gGT->frontBuffer = db;

	// frontBuffer->dispEnv
	PutDispEnv(&db->dispEnv);

	// frontBuffer
	PutDrawEnv(&db->drawEnv);
	return;
}
