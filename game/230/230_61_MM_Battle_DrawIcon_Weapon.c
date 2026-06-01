#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800b1660-0x800b1830.
void MM_Battle_DrawIcon_Weapon(struct Icon *icon, u32 posX, int posY, struct PrimMem *primMem, u32 *ot, char transparency, s16 param_7, u16 param_8, u32 *color)
{
	if (!icon)
		return;

	POLY_FT4 *p = (POLY_FT4 *)primMem->curr;

	u32 uv0 = *(u32 *)&icon->texLayout.u0;
	u32 uv1 = *(u32 *)&icon->texLayout.u1;
	u32 uv2 = *(u32 *)&icon->texLayout.u2;
	s32 scaledWidth = (((s32)((u8)icon->texLayout.u1 - (u8)icon->texLayout.u0)) * param_7) >> 0xc;
	s32 scaledHeight = (((s32)((u8)icon->texLayout.v2 - (u8)icon->texLayout.v0)) * param_7) >> 0xc;
	u32 code = 0x2c000000;
	u32 packedY = (u32)(u16)posY << 0x10;
	u32 packedBottomY;
	u32 packedSideY;
	u32 rightX;
	u32 sidewaysX;

	if ((u8)transparency != 0)
	{
		code = 0x2e000000;
		uv1 = (uv1 & 0xff9fffff) | ((((u32)(u8)transparency - 1) << 0x15));
	}

	*(u32 *)&p->r0 = (*color & 0xffffff) | code;
	*(u32 *)&p->u0 = uv0;
	*(u32 *)&p->u1 = uv1;
	*(u16 *)&p->u2 = (u16)uv2;
	*(u16 *)&p->u3 = (u16)(uv2 >> 0x10);

	if ((param_8 & 1) != 0)
	{
		sidewaysX = posX + scaledHeight;
		packedSideY = packedY + ((u32)scaledWidth << 0x10);

		if ((s16)param_8 == 1)
		{
			*(u32 *)&p->x1 = posX | packedY;
			*(u32 *)&p->x3 = sidewaysX | packedY;
			*(u32 *)&p->x0 = posX | packedSideY;
			*(u32 *)&p->x2 = sidewaysX | packedSideY;
		}
		else
		{
			*(u32 *)&p->x2 = posX | packedY;
			*(u32 *)&p->x0 = sidewaysX | packedY;
			*(u32 *)&p->x3 = posX | packedSideY;
			*(u32 *)&p->x1 = sidewaysX | packedSideY;
		}
	}
	else
	{
		rightX = posX + scaledWidth;
		packedBottomY = packedY + ((u32)scaledHeight << 0x10);

		if (((u32)param_8 << 0x10) == 0)
		{
			*(u32 *)&p->x0 = posX | packedY;
			*(u32 *)&p->x1 = rightX | packedY;
			*(u32 *)&p->x2 = posX | packedBottomY;
			*(u32 *)&p->x3 = rightX | packedBottomY;
		}
		else
		{
			*(u32 *)&p->x3 = posX | packedY;
			*(u32 *)&p->x2 = rightX | packedY;
			*(u32 *)&p->x1 = posX | packedBottomY;
			*(u32 *)&p->x0 = rightX | packedBottomY;
		}
	}

	*(int *)p = *(int *)ot | 0x9000000;
	*(int *)ot = (int)CtrGpu_PrimToOTLink24(p);

	primMem->curr = p + 1;
}
