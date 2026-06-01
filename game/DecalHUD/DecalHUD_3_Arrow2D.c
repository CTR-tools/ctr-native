#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80023190-0x80023488.
void DecalHUD_Arrow2D(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *otMemPtr, u32 color1, u32 color2, u32 color3, u32 color4,
                      char transparency, int scale, u16 rot)
{
	u8 y2;
	u32 code;
	int bitshiftTopRightCorner;
	u32 topRightCornerAndPageXY;
	int bitshiftPosY;
	int iVar6;
	int iVar7;
	s16 sVar8;
	u32 bottomMargin;
	int iVar10;
	u32 topLeftCornerAndPaletteXY;
	int iVar12;
	int iVar13;

	POLY_GT4 *p;

	if (icon == 0)
	{
		return;
	}

	topRightCornerAndPageXY = *(u32 *)&icon->texLayout.u1;
	topLeftCornerAndPaletteXY = *(u32 *)&icon->texLayout.u0;
	y2 = icon->texLayout.v2;
	bottomMargin = *(u32 *)&icon->texLayout.u2;

	p = (POLY_GT4 *)primMem->curr;

	if (transparency == 0)
	{
		code = 0x3c000000;
		*(int *)&p->u1 = topRightCornerAndPageXY;
	}

	else
	{
		code = 0x3e000000;

		// set top right corner UVs and texpage of primitive, and alter the blending mode bits of the texpage from 11 (Mode 3, which is no blending) to 00 (Mode
		// 0, equivalent to regular 50% opacity)
		*(int *)&p->u1 = topRightCornerAndPageXY & 0xff9fffff | ((u32)transparency - 1) * 0x200000;
	}

	// set top left vertex color, and code in 7th byte of prim
	*(int *)&p->r0 = color1 & 0xffffff | code;

	posX = posX & 0xffff;
	*(int *)&p->u0 = topLeftCornerAndPaletteXY;
	*(s16 *)&p->u2 = (s16)bottomMargin;

	bitshiftPosY = (int)(((u32)y2 - ((int)topLeftCornerAndPaletteXY >> 8 & 0xffU)) * (int)scale) >> 0xd;

	*(u16 *)&p->u3 = *(u16 *)&icon->texLayout.u3;

	bitshiftTopRightCorner = (int)(((topRightCornerAndPageXY & 0xff) - (topLeftCornerAndPaletteXY & 0xff)) * (int)scale) >> 0xd;

	// stuff for rotation of primitive
	iVar13 = *(int *)(&data.trigApprox[((u32)rot & 0x3ff) * 4]) >> 0x10;
	sVar8 = (s16) * (int *)(&data.trigApprox[((u32)rot & 0x3ff) * 4]);

	if ((rot & 0x400) == 0)
	{
		iVar10 = (int)sVar8;
		if ((rot & 0x800) == 0)
			goto LAB_800232d8;
		iVar12 = -iVar13;
	}
	else
	{
		iVar12 = (int)sVar8;
		iVar10 = iVar13;
		if ((rot & 0x800) == 0)
		{
			iVar13 = -iVar12;
			goto LAB_800232d8;
		}
	}
	iVar10 = -iVar10;
	iVar13 = iVar12;

LAB_800232d8:
	iVar12 = -bitshiftPosY;
	bitshiftPosY = bitshiftPosY + 1;
	iVar6 = iVar12 * iVar10 >> 0xc;
	iVar12 = posY + (iVar12 * iVar13 >> 0xc);

	*(int *)&p->x0 = posX + (-bitshiftTopRightCorner * iVar13 >> 0xc) + iVar6 & 0xffff | (iVar12 - (-bitshiftTopRightCorner * iVar10 >> 0xc)) * 0x10000;

	iVar7 = bitshiftPosY * iVar10 >> 0xc;

	*(int *)&p->x1 =
	    posX + ((bitshiftTopRightCorner + 1) * iVar13 >> 0xc) + iVar6 & 0xffff | (iVar12 - ((bitshiftTopRightCorner + 1) * iVar10 >> 0xc)) * 0x10000;

	posY = posY + (bitshiftPosY * iVar13 >> 0xc);

	*(int *)&p->x2 = posX + (-bitshiftTopRightCorner * iVar13 >> 0xc) + iVar7 & 0xffff | (posY - (-bitshiftTopRightCorner * iVar10 >> 0xc)) * 0x10000;
	*(int *)&p->x3 = posX + ((bitshiftTopRightCorner + 1) * iVar13 >> 0xc) + iVar7 & 0xffff | (posY - ((bitshiftTopRightCorner + 1) * iVar10 >> 0xc)) * 0x10000;

	*(int *)&p->r1 = color2;
	*(int *)&p->r2 = color3;
	*(int *)&p->r3 = color4;

	*(int *)p = *otMemPtr | 0xc000000;
	*otMemPtr = CtrGpu_PrimToOTLink24(p);

	// POLY_GT4 is 0x34 bytes large
	primMem->curr = p + 1;
	return;
}
