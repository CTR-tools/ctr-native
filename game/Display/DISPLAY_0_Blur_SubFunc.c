#include <common.h>

static u32 DISPLAY_Blur_Ptr24(const void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static u32 DISPLAY_Blur_PackS16Pair(int x, int y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

static void DISPLAY_Blur_WriteLo16(u32 *word, u16 value)
{
	*(u16 *)word = value;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80023a40-0x80023d4c
u32 *DISPLAY_Blur_SubFunc(u32 *prim, s16 *tile)
{
	int srcX = tile[0];
	int srcY = tile[1];
	int srcW = tile[2];
	int srcH = tile[3];

	if (((srcX + srcW) & -0x100) != (srcX & -0x100))
	{
		tile[10] = (s16)((srcX & -0x100) - (srcX - 0xff));
		tile[14] = (s16)((tile[10] * tile[6]) / srcW);

		tile[8] = (s16)srcX;
		tile[9] = (s16)srcY;
		tile[11] = (s16)srcH;
		tile[15] = tile[7];
		tile[12] = tile[4];
		tile[13] = tile[5];

		if (tile[6] != 0)
			prim = DISPLAY_Blur_SubFunc(prim, tile + 8);

		tile[8] = (s16)(tile[10] + tile[8] + 1);
		tile[10] = (s16)(tile[2] - tile[10] - 1);
		tile[12] = (s16)(tile[12] + tile[14]);
		tile[14] = (s16)(tile[6] - tile[14]);

		if (tile[6] != 0)
			prim = DISPLAY_Blur_SubFunc(prim, tile + 8);

		return prim;
	}

	if (((srcY + srcH) & -0x100) != (srcY & -0x100))
	{
		tile[11] = (s16)((srcY & -0x100) - (srcY - 0xff));
		tile[15] = (s16)((tile[11] * tile[7]) / srcH);

		tile[8] = (s16)srcX;
		tile[9] = (s16)srcY;
		tile[10] = (s16)srcW;
		tile[14] = tile[6];
		tile[12] = tile[4];
		tile[13] = tile[5];

		if (tile[7] != 0)
			prim = DISPLAY_Blur_SubFunc(prim, tile + 8);

		tile[9] = (s16)(tile[11] + tile[9] + 1);
		tile[11] = (s16)(tile[3] - tile[11] - 1);
		tile[13] = (s16)(tile[13] + tile[15]);
		tile[15] = (s16)(tile[7] - tile[15]);

		if (tile[7] != 0)
			prim = DISPLAY_Blur_SubFunc(prim, tile + 8);

		return prim;
	}

	u32 u0 = (u32)srcX & 0x3f;
	u32 v0 = ((u32)srcY & 0xff) << 8;
	u32 u1 = (u32)(srcW + (int)u0);
	u32 v1 = v0 + ((u32)srcH << 8);
	int dstX = tile[4];
	int dstY = tile[5];
	int dstW = tile[6];
	int dstH = tile[7];
	u32 tpage = (((u32)srcY & 0x100) >> 4) | (((u32)srcX & 0x3ff) >> 6) | 0x100 | (((u32)srcY & 0x200) << 2);

	prim[3] = u0 | v0;
	prim[2] = DISPLAY_Blur_PackS16Pair(dstX, dstY);
	prim[4] = DISPLAY_Blur_PackS16Pair(dstX + dstW, dstY);
	prim[6] = DISPLAY_Blur_PackS16Pair(dstX, dstY + dstH);
	prim[8] = DISPLAY_Blur_PackS16Pair(dstX + dstW, dstY + dstH);
	DISPLAY_Blur_WriteLo16(&prim[7], (u16)(u0 | v1));
	DISPLAY_Blur_WriteLo16(&prim[9], (u16)(u1 | v1));
	*(u8 *)((u8 *)prim + 7) = 0x2f;
	prim[0] = DISPLAY_Blur_Ptr24(prim + 10) | 0x09000000;
	prim[5] = u1 | v0 | (tpage << 16);

	return prim + 10;
}
