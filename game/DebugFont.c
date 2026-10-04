#include <common.h>

enum DebugFontConstants
{
	DEBUG_FONT_ICON_INDEX = 0x42,
	DEBUG_FONT_DIGIT_TILE_SIZE = 7,
	DEBUG_FONT_DIGIT_ZERO_ATLAS_OFFSET = 5,
	DEBUG_FONT_PRIM_WORD_COUNT = 9,
};


void DebugFont_Init(struct GameTracker *gGT)
{
	struct Icon *debugFontIcon = gGT->ptrIcons[DEBUG_FONT_ICON_INDEX];
	u32 u;
	u32 v;
	u16 clut;
	u16 tpage;

	if (debugFontIcon == 0)
	{
		return;
	}

	u = debugFontIcon->texLayout.u0;
	clut = debugFontIcon->texLayout.clut;
	sdata->debugFont.u = u;
	v = debugFontIcon->texLayout.v0;
	tpage = debugFontIcon->texLayout.tpage;
	sdata->debugFont.clut = clut;
	sdata->debugFont.tpage = tpage;
	sdata->debugFont.v = v;
}


void DebugFont_DrawNumbers(int index, int screenPosX, int screenPosY)
{
	// NOTE(aalhendi): GCC 2.8.1 keeps the cursor and serial packet-word
	// lifetimes in v0, and the paired lanes live until their word stores.
	POLY_FT4 *p;
	register POLY_FT4 *nextPrim CTR_PSX_REGISTER("$2");
	struct PrimMem *primMem;
	u32 *ot;
	register u32 bottomX CTR_PSX_REGISTER("$10");
	register u32 bottomY CTR_PSX_REGISTER("$9");
	register u32 packetWord CTR_PSX_REGISTER("$2");
	u32 topX;
	register u32 topY CTR_PSX_REGISTER("$6");
	register u32 baseV CTR_PSX_REGISTER("$6");
	u32 topU;
	register u32 bottomU CTR_PSX_REGISTER("$10");
	register u32 topV CTR_PSX_REGISTER("$5");
	register u32 bottomV CTR_PSX_REGISTER("$9");
	u16 clut;
	u16 tpage;
	register struct GameTracker *gGT CTR_PSX_REGISTER("$2");

	bottomX = (u32)(screenPosX + DEBUG_FONT_DIGIT_TILE_SIZE);
	screenPosX = (u16)screenPosX;
	CTR_PSX_OBSERVE_VALUE(screenPosX);
	bottomX = (u16)bottomX;
	bottomY = (u32)(screenPosY + DEBUG_FONT_DIGIT_TILE_SIZE);
	screenPosY = (u32)screenPosY << 16;
	CTR_PSX_OBSERVE_VALUE(screenPosY);
	gGT = GAME_TRACKER;
	bottomY <<= 16;
	index += DEBUG_FONT_DIGIT_ZERO_ATLAS_OFFSET;

	primMem = &gGT->backBuffer->primMem;
	CTR_PSX_KEEP_VALUE_RELAXED(primMem);
	p = (POLY_FT4 *)primMem->cursor;
	ot = (u32 *)gGT->pushBuffer_UI.ptrOT;
	nextPrim = p + 1;
	CTR_PSX_KEEP_VALUE_RELAXED(nextPrim);
	primMem->cursor = nextPrim;

	packetWord = 0x2e000000;
	CTR_PSX_KEEP_VALUE_RELAXED(packetWord);
	CtrGpu_WriteColorCode(&p->r0, packetWord);
	topX = screenPosX;
	topY = screenPosY;
	packetWord = topX | topY;
	CTR_PSX_KEEP_VALUE_RELAXED(packetWord);
	topY = bottomX | topY;
	topX = topX | bottomY;
	// topY and topX now hold XY words for the top right and bottom left corners.
	CTR_PSX_ORDER_VALUES(topY, topX);
	CtrGpu_WritePackedXY(&p->x0, packetWord);
	packetWord = bottomX | bottomY;
	CTR_PSX_KEEP_VALUE_RELAXED(packetWord);
	CtrGpu_WritePackedXY(&p->x3, packetWord);
	packetWord = index * DEBUG_FONT_DIGIT_TILE_SIZE;
	CTR_PSX_KEEP_VALUE_RELAXED(packetWord);
	CtrGpu_WritePackedXY(&p->x1, topY);
	CtrGpu_WritePackedXY(&p->x2, topX);

	// Each character is 7x7 pixels,
	// '0' is 6th character on 2nd row
	baseV = sdata->debugFont.v;
	topU = sdata->debugFont.u;
	CTR_PSX_ORDER_VALUES(baseV, topU);
	topV = baseV + DEBUG_FONT_DIGIT_TILE_SIZE;
	topU += packetWord;
	bottomU = topU + DEBUG_FONT_DIGIT_TILE_SIZE;
	bottomV = baseV + 2 * DEBUG_FONT_DIGIT_TILE_SIZE;
	topV <<= 8;
	bottomV <<= 8;

	packetWord = topU | topV;
	CTR_PSX_KEEP_VALUE_RELAXED(packetWord);
	topV = bottomU | topV;
	topU = topU | bottomV;
	// topV and topU now hold UV words for the top right and bottom left corners.
	CTR_PSX_ORDER_VALUES(topV, topU);
	CtrGpu_WritePackedUVWord(&p->u0, packetWord);
	packetWord = bottomU | bottomV;
	CtrGpu_WritePackedUVWord(&p->u1, topV);
	CtrGpu_WritePackedUVWord(&p->u2, topU);
	CtrGpu_WritePackedUVWord(&p->u3, packetWord);

	clut = sdata->debugFont.clut;
	tpage = sdata->debugFont.tpage;
	CTR_PSX_ORDER_VALUES(clut, tpage);
	p->clut = clut;
	p->tpage = tpage;

	// NOTE(aalhendi): Retail UI heads contain 24-bit links. Native must mask
	// the head and use the host-pointer token bridge.
#if defined(CTR_NATIVE)
	p->tag = CtrGpu_PackOTTag(*ot, DEBUG_FONT_PRIM_WORD_COUNT << 0x18);
	*ot = CtrGpu_PrimToOTLink24(p);
#else
	p->tag = *ot | (DEBUG_FONT_PRIM_WORD_COUNT << 0x18);
	*ot = ((u32)p << 8) >> 8;
#endif
}
