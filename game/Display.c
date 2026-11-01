#include <common.h>

u32 *DISPLAY_Blur_SubFunc(u32 *prim, struct DisplayBlurTile *tile)
{
	s32 srcX = tile->srcX;
	s32 srcY = tile->srcY;
	s32 srcW = tile->srcW;
	s32 srcH = tile->srcH;
	s32 dstX;
	s32 dstY;
	s32 dstW;
	s32 dstH;
	u32 tpage;
	u32 uvRight;
	u32 tagMask;
	s16 textureX;
	s16 textureY;
	POLY_FT4 *poly;
	u32 *nextPrim;

	// Split source pages recursively; the caller owns storage for each depth.
	if (((srcX + srcW) & -0x100) != (srcX & -0x100))
	{
		s32 pageEndBias = srcX - 0xff;
		u16 childX;
		u16 childW;
		u16 parentW;
		u16 consumedW;
		u16 childDstX;
		u16 childDstW;
		tile[1].srcW = (srcX & -0x100) - pageEndBias;

		tile[1].srcX = (s16)srcX;
		tile[1].srcY = (s16)srcY;
		tile[1].srcH = (s16)srcH;
		tile[1].dstX = tile->dstX;
		tile[1].dstY = tile->dstY;
		tile[1].dstH = tile->dstH;
		tile[1].dstW = (s16)((tile[1].srcW * tile->dstW) / srcW);

		if (tile->dstW != 0)
		{
			prim = DISPLAY_Blur_SubFunc(prim, tile + 1);
		}

		childX = tile[1].srcX;
		childW = tile[1].srcW;
		parentW = tile->srcW;
		childX += 1;
		// NOTE(aalhendi): Both axes retain the consumed halfword and publish
		// its source remainder before the destination-span phase.
		consumedW = childW;
		CTR_PSX_KEEP_VALUE(consumedW);
		tile[1].srcX = childW + childX;
		tile[1].srcW = parentW - consumedW - 1;
		childDstX = tile[1].dstX;
		childDstW = tile[1].dstW;
		tile[1].dstX = childDstX + childDstW;
		CTR_PSX_DEPEND_MEMORY(&tile[1].srcW, childDstW);
		tile[1].dstW = (u16)tile->dstW - childDstW;
		if (tile->dstW != 0)
		{
			prim = DISPLAY_Blur_SubFunc(prim, tile + 1);
		}

		return prim;
	}

	if (((srcY + srcH) & -0x100) != (srcY & -0x100))
	{
		s32 pageEndBias = srcY - 0xff;
		u16 childY;
		u16 childH;
		u16 parentH;
		u16 consumedH;
		u16 childDstY;
		u16 childDstH;
		tile[1].srcH = (srcY & -0x100) - pageEndBias;

		tile[1].srcX = (s16)srcX;
		tile[1].srcY = (s16)srcY;
		tile[1].srcW = (s16)srcW;
		tile[1].dstX = tile->dstX;
		tile[1].dstY = tile->dstY;
		tile[1].dstW = tile->dstW;
		tile[1].dstH = (s16)((tile[1].srcH * tile->dstH) / srcH);

		if (tile->dstH != 0)
		{
			prim = DISPLAY_Blur_SubFunc(prim, tile + 1);
		}

		childY = tile[1].srcY;
		childH = tile[1].srcH;
		parentH = tile->srcH;
		childY += 1;
		consumedH = childH;
		CTR_PSX_KEEP_VALUE(consumedH);
		tile[1].srcY = childH + childY;
		tile[1].srcH = parentH - consumedH - 1;
		childDstY = tile[1].dstY;
		childDstH = tile[1].dstH;
		tile[1].dstY = childDstY + childDstH;
		CTR_PSX_DEPEND_MEMORY(&tile[1].srcH, childDstH);
		tile[1].dstH = (u16)tile->dstH - childDstH;
		if (tile->dstH != 0)
		{
			prim = DISPLAY_Blur_SubFunc(prim, tile + 1);
		}

		return prim;
	}

	srcY &= 0xff;
	srcX &= 0x3f;
	srcY = (s32)((u32)srcY << 8);
	srcW += srcX;
	srcH = (s32)((u32)srcY + ((u32)srcH << 8));
	tagMask = 0xffffff;
	poly = (POLY_FT4 *)prim;
	nextPrim = (u32 *)(poly + 1);
	dstX = tile->dstX;
	dstY = tile->dstY;
	dstW = tile->dstW;
	dstH = tile->dstH;

	CtrGpu_WritePackedUVWord(&poly->u0, ((u32)srcX) | ((u32)srcY));
	CtrGpu_WritePackedXY(&poly->x0, ((u32)dstX | ((u32)dstY << 16)));
	CtrGpu_WritePackedXY(&poly->x1, ((u32)(dstX + dstW) | ((u32)dstY << 16)));
	CtrGpu_WritePackedXY(&poly->x2, ((u32)dstX | (((u32)dstY << 16) + ((u32)dstH << 16))));
	CtrGpu_WritePackedXY(&poly->x3, ((u32)(dstX + dstW) | (((u32)dstY << 16) + ((u32)dstH << 16))));
	textureY = tile->srcY;
	textureX = tile->srcX;
	CtrGpu_WritePackedUV(&poly->u2, (u16)(((u32)srcX) | ((u32)srcH)));
	CtrGpu_WritePackedUV(&poly->u3, (u16)(((u32)srcW) | ((u32)srcH)));
	poly->code = 0x2f;
	uvRight = ((u32)srcW) | ((u32)srcY);
	poly->tag = (CtrGpu_PrimToOTLink24(nextPrim) & tagMask) | 0x09000000;
	tpage = getTPage(TEXPAGE_COLOR_15BIT, TRANS_50, textureX, textureY);
	CtrGpu_WritePackedUVWord(&poly->u1, uvRight | (tpage << 16));

	return nextPrim;
}


// NOTE(aalhendi): GCC 2.8 allocates the short-lived tag/slot before the cursor.
// Bind these branch-exclusive owners to retail's reused S1, not a caller ABI.
void DISPLAY_Blur_Main(struct PushBuffer *pb, s32 strength)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct DB *backBuffer = gGT->backBuffer;
	u32 *prim = backBuffer->primMem.cursor;
	struct DisplayBlurTile *tile = CTR_SCRATCHPAD_PTR(struct DisplayBlurTile, 0);
	s32 cameraID;

	if (strength > 0 && (cameraID = (s8)pb->cameraID, (CTR_MipsSra(gGT->db[1 - gGT->swapchainIndex].blurCameraMask, (u32)cameraID) & 1) != 0))
	{
		u32 *ot;
		register u32 oldTag CTR_PSX_REGISTER("s1");
		s32 wave = (s32)((u32)gGT->timer + (u32)cameraID);
		s32 blur;
		s32 insetX;
		s32 insetY;
		s16 sourceX;
		s16 sourceW;
		s32 displayY;
		u16 rectX;
		s32 rectY;
		u16 rectW;
		u16 rectH;

		if ((cameraID & 1) != 0)
		{
			wave = (s32)(0u - (u32)wave);
		}
		blur = MATH_Sin((s32)((u32)wave * 100u));
		if (blur < 0)
		{
			blur = -blur;
		}
		blur = (blur >> 2) + 0x400;
		if (strength < 0x1000)
		{
			blur = (blur * strength) >> 12;
		}

		ot = GAME_TRACKER->otSwapchainDB[GAME_TRACKER->swapchainIndex];
		oldTag = *ot;
		*ot = CtrGpu_PrimToOTLink24(prim);
		rectX = pb->rect.x;
		tile->dstX = rectX;
		rectY = (u16)pb->rect.y;
		tile->dstY = rectY;
		rectW = pb->rect.w;
		tile->dstW = rectW;
		rectH = pb->rect.h;
		tile->dstH = rectH;
		// Keep the cropped X span until Y's inset is ready to publish the ROI.
		insetX = ((blur * 9) >> 12) + 2;
		sourceX = (u16)backBuffer->dispEnv.disp.x + rectX + insetX;
		sourceW = rectW - insetX * 2;
		insetY = ((blur * 6) >> 12) + 2;
		tile->srcX = sourceX;
		displayY = (u16)backBuffer->dispEnv.disp.y;
		tile->srcW = sourceW;
		tile->srcH = rectH - insetY * 2;
		tile->srcY = displayY + rectY + insetY;

		prim = DISPLAY_Blur_SubFunc(prim, tile);
		prim -= sizeof(POLY_FT4) / sizeof(*prim);
		*prim = oldTag | 0x09000000;
		// NOTE(aalhendi): Keep the saved link live so the OR uses a temporary.
		CTR_PSX_OBSERVE_VALUE(oldTag);
		prim += sizeof(POLY_FT4) / sizeof(*prim);
	}
	else
	{
		struct DisplayBlurFlatPacket *packet = (struct DisplayBlurFlatPacket *)prim;
		register u32 *ot CTR_PSX_REGISTER("s1");
		s32 x;
		s32 y;
		s32 w;
		s32 h;

		packet->drawModeStart = 0xe1000a20;
		packet->maskBitEnable = 0xe6000001;
		packet->drawModeEnd = 0xe1000a00;
		packet->maskBitDisable = 0xe6000000;
		x = pb->rect.x;
		y = pb->rect.y;
		w = pb->rect.w;
		h = pb->rect.h;
		packet->xy0 = (u32)x | ((u32)y << 16);
		packet->xy1 = (u32)(x + w) | ((u32)y << 16);
		packet->xy2 = (u32)x | (((u32)y << 16) + ((u32)h << 16));
		packet->xy3 = (u32)(x + w) | (((u32)y << 16) + ((u32)h << 16));
		packet->colorAndCode = strength < 0 ? 0x2affffff : 0x2a000000;
		ot = GAME_TRACKER->otSwapchainDB[GAME_TRACKER->swapchainIndex];
		packet->tag = *ot | 0x09000000;
		*ot = CtrGpu_PrimToOTLink24(packet);
		prim += sizeof(*packet) / sizeof(*prim);
	}

	backBuffer->primMem.cursor = prim;
	backBuffer->blurCameraMask |= CTR_MipsSll(1, (u32)(s8)pb->cameraID);
}


void DISPLAY_Swap(void)
{
	struct GameTracker *gGT;
	struct DB *db;

	gGT = GAME_TRACKER;

	// NOTE(aalhendi): The new render index selects the opposite DB as front.
	// Resolve its published owner again after the display SDK call.
	gGT->swapchainIndex = 1 - gGT->swapchainIndex;
	db = &gGT->db[1] - gGT->swapchainIndex;
	gGT->frontBuffer = db;

	PutDispEnv(&db->dispEnv);
	PutDrawEnv(&GAME_TRACKER->frontBuffer->drawEnv);
	return;
}
