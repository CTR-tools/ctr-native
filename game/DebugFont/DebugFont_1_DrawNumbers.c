#include <common.h>

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

	p = (POLY_FT4 *)gGT->backBuffer->primMem.curr;
	ot = (u32 *)gGT->pushBuffer_UI.ptrOT;
	gGT->backBuffer->primMem.curr = p + 1;

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

	*(int *)p = *ot | 0x9000000;
	*ot = CtrGpu_PrimToOTLink24(p);
}
