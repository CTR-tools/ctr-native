#include "common.h"

static u32 VehGroundSkids_Ptr24(const void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static void VehGroundSkids_WriteLo16(u32 *word, u16 value)
{
	*(u16 *)word = value;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005c120-0x8005c278.
void VehGroundSkids_Subset1(u32 *currXY, u32 *prevXY, int depth, u8 *scratch)
{
	struct GameTracker *gGT = sdata->gGT;
	struct DB *backBuffer = gGT->backBuffer;
	u32 *prim = backBuffer->primMem.curr;
	u32 *nextPrim = prim + 0xd;

	if ((u32 *)backBuffer->primMem.endMin100 < nextPrim)
		return;

	backBuffer->primMem.curr = nextPrim;

	prim[1] = *(u32 *)(scratch + 0x1c);
	prim[4] = *(u32 *)(scratch + 0x1c);
	prim[7] = *(u32 *)(scratch + 0x20);
	prim[10] = *(u32 *)(scratch + 0x20);

	prim[2] = currXY[0];
	prim[5] = currXY[1];
	prim[8] = prevXY[0];
	prim[11] = prevXY[1];

	struct Icon *icon = gGT->ptrIcons[0x2f];
	prim[3] = *(u32 *)&icon->texLayout.u0;

	u32 tpage = *(u32 *)&icon->texLayout.u1;
	if ((*(u32 *)(scratch + 0x24) & 1) != 0)
		tpage = (tpage & 0xff9fffff) | 0x00600000;
	else
		tpage = (tpage & 0xff9fffff) | 0x00400000;
	prim[6] = tpage;

	VehGroundSkids_WriteLo16(&prim[9], *(u16 *)&icon->texLayout.u2);
	VehGroundSkids_WriteLo16(&prim[12], *(u16 *)&icon->texLayout.u3);

	struct PushBuffer *pb = *(struct PushBuffer **)(scratch + 0x18);
	u_long *ot = pb->ptrOT + ((s32)depth >> 6);
	prim[0] = (u32)*ot | 0x0c000000;
	*ot = (u_long)VehGroundSkids_Ptr24(prim);
}
