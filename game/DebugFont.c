#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800222e0-0x80022318.
void DebugFont_Init(struct GameTracker *gGT)
{
	struct Icon *debugFontIcon = gGT->ptrIcons[0x42];

	if (debugFontIcon == 0)
		return;

	u8 u = debugFontIcon->texLayout.u0;
	u8 v = debugFontIcon->texLayout.v0;
	u16 clut = debugFontIcon->texLayout.clut;
	u16 tpage = debugFontIcon->texLayout.tpage;
	sdata->debugFont.u = u;
	sdata->debugFont.v = v;
	sdata->debugFont.clut = clut;
	sdata->debugFont.tpage = tpage;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022318-0x800223f4.
void DebugFont_DrawNumbers(int index, int screenPosX, int screenPosY)
{
	POLY_FT4 *p;
	u32 *ot;
	u32 uVar4;
	u32 uVar5;
	u32 uVar6;
	u32 topU;
	u32 bottomU;
	u32 topV;
	u32 bottomV;
	struct GameTracker *gGT = sdata->gGT;

	uVar6 = screenPosX + 7;
	uVar5 = (screenPosY + 0) << 0x10;
	uVar4 = (screenPosY + 7) << 0x10;

	p = (POLY_FT4 *)gGT->backBuffer->primMem.cursor;
	ot = (u32 *)gGT->pushBuffer_UI.ptrOT;
	gGT->backBuffer->primMem.cursor = p + 1;

	*(int *)&p->r0 = 0x2e000000;
	*(int *)&p->x0 = screenPosX | uVar5;
	*(int *)&p->x3 = uVar6 | uVar4;
	*(int *)&p->x1 = uVar6 | uVar5;
	*(int *)&p->x2 = screenPosX | uVar4;

	// Each character is 7x7 pixels,
	// '0' is 6th character on 2nd row
	topU = sdata->debugFont.u + (index + 5) * 7;
	bottomU = topU + 7;
	topV = sdata->debugFont.v + 7;
	bottomV = topV + 7;

	*(int *)&p->u0 = topU | (topV << 8);
	*(int *)&p->u1 = bottomU | (topV << 8);
	*(int *)&p->u2 = topU | (bottomV << 8);
	*(int *)&p->u3 = bottomU | (bottomV << 8);

	p->clut = sdata->debugFont.clut;
	p->tpage = sdata->debugFont.tpage;

	*(int *)p = CtrGpu_PackOTTag(*ot, 0x9000000);
	*ot = CtrGpu_PrimToOTLink24(p);
}
