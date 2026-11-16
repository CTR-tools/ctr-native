#include <common.h>
#include <widescreen.h>

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

	if (debugFontIcon == 0)
	{
		return;
	}

	u8 u = debugFontIcon->texLayout.u0;
	u8 v = debugFontIcon->texLayout.v0;
	u16 clut = debugFontIcon->texLayout.clut;
	u16 tpage = debugFontIcon->texLayout.tpage;
	sdata->debugFont.u = u;
	sdata->debugFont.v = v;
	sdata->debugFont.clut = clut;
	sdata->debugFont.tpage = tpage;
}


void DebugFont_DrawNumbers(int index, int screenPosX, int screenPosY)
{
	POLY_FT4 *p;
	u32 *ot;
	u32 topU;
	u32 bottomU;
	u32 topV;
	u32 bottomV;
	struct GameTracker *gGT = sdata->gGT;

	p = (POLY_FT4 *)gGT->backBuffer->primMem.cursor;
	ot = (u32 *)gGT->pushBuffer_UI.ptrOT;
	gGT->backBuffer->primMem.cursor = p + 1;

	CtrGpu_WriteColorCode(&p->r0, 0x2e000000);
	CtrGpu_WritePackedXY(&p->x0, CTR_PackS16Pair(screenPosX, screenPosY));
	CtrGpu_WritePackedXY(&p->x3, CTR_PackS16Pair(screenPosX + DEBUG_FONT_DIGIT_TILE_SIZE, screenPosY + DEBUG_FONT_DIGIT_TILE_SIZE));
	CtrGpu_WritePackedXY(&p->x1, CTR_PackS16Pair(screenPosX + DEBUG_FONT_DIGIT_TILE_SIZE, screenPosY));
	CtrGpu_WritePackedXY(&p->x2, CTR_PackS16Pair(screenPosX, screenPosY + DEBUG_FONT_DIGIT_TILE_SIZE));
	Widescreen_CompressFT4(p);

	// Each character is 7x7 pixels,
	// '0' is 6th character on 2nd row
	topU = sdata->debugFont.u + (index + DEBUG_FONT_DIGIT_ZERO_ATLAS_OFFSET) * DEBUG_FONT_DIGIT_TILE_SIZE;
	bottomU = topU + DEBUG_FONT_DIGIT_TILE_SIZE;
	topV = sdata->debugFont.v + DEBUG_FONT_DIGIT_TILE_SIZE;
	bottomV = topV + DEBUG_FONT_DIGIT_TILE_SIZE;

	CtrGpu_WritePackedUVWord(&p->u0, topU | (topV << 8));
	CtrGpu_WritePackedUVWord(&p->u1, bottomU | (topV << 8));
	CtrGpu_WritePackedUVWord(&p->u2, topU | (bottomV << 8));
	CtrGpu_WritePackedUVWord(&p->u3, bottomU | (bottomV << 8));

	p->clut = sdata->debugFont.clut;
	p->tpage = sdata->debugFont.tpage;

	p->tag = CtrGpu_PackOTTag(*ot, DEBUG_FONT_PRIM_WORD_COUNT << 0x18);
	*ot = CtrGpu_PrimToOTLink24(p);
}
