#include <common.h>

static inline u32 FLARE_Ptr24(const void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static inline u32 FLARE_PackXY(s16 x, s16 y)
{
	return (u16)x | ((u32)(u16)y << 16);
}

static inline void FLARE_LoadGridRow(s16 y)
{
	MTC2(FLARE_PackXY(-409, y), 0);
	MTC2(0, 1);
	MTC2(FLARE_PackXY(0, y), 2);
	MTC2(0, 3);
	MTC2(FLARE_PackXY(409, y), 4);
	MTC2(0, 5);
}

static inline void FLARE_WriteTexture(u32 *prim, struct Icon *icon, u32 texWord1)
{
	prim[3] = *(u32 *)&icon->texLayout.u0;
	prim[6] = texWord1;
	*(u16 *)&prim[9] = *(u16 *)&icon->texLayout.u2;
	*(u16 *)&prim[12] = *(u16 *)&icon->texLayout.u3;
}

static inline void FLARE_WriteColors(u32 *prim)
{
	prim[1] = 0x3e000000;
	prim[4] = 0;
	prim[7] = 0;
	prim[10] = 0x007f7f7f;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80024c4c-0x80025138.
void FLARE_ThTick(struct Thread *th)
{
	struct GameTracker *gGT = sdata->gGT;
	struct PushBuffer *pb = &gGT->pushBuffer[0];
	s32 *flare = th->object;
	s32 timer = flare[0];

	flare[0] = timer + 1;

	if (timer >= 20)
	{
		th->flags |= 0x800;
		return;
	}

	u32 *prim = gGT->backBuffer->primMem.curr;
	if (prim + 0x34 >= (u32 *)gGT->backBuffer->primMem.endMin100)
		return;

	PushBuffer_SetPsyqGeom(pb);
	gte_SetLightMatrix(&pb->matrix_ViewProj);

	s32 relX = ((s32) * (s16 *)&flare[1] - pb->matrix_Camera.t[0]) << 2;
	s32 relY = ((s32) * (s16 *)((u8 *)flare + 6) - pb->matrix_Camera.t[1]) << 2;
	s32 relZ = ((s32) * (s16 *)&flare[2] - pb->matrix_Camera.t[2]) << 2;

	MTC2(((u32)(u16)relX) | ((u32)relY << 16), 0);
	MTC2(relZ, 1);
	gte_llv0();

	CTC2(MFC2(25), 5);
	CTC2(MFC2(26), 6);
	CTC2(MFC2(27), 7);

	s32 oldMin;
	s32 oldMax;
	s32 newMin;
	s32 newMax;
	if (timer < 2)
	{
		oldMin = 0;
		oldMax = 2;
		newMin = 0x400;
		newMax = 0x2000;
	}
	else if (timer < 4)
	{
		oldMin = 2;
		oldMax = 4;
		newMin = 0x2000;
		newMax = 0xc00;
	}
	else if (timer < 8)
	{
		oldMin = 4;
		oldMax = 8;
		newMin = 0xc00;
		newMax = 0x266;
	}
	else
	{
		oldMin = 8;
		oldMax = 20;
		newMin = 0x266;
		newMax = 0;
	}

	s32 scale = VehCalc_MapToRange(timer, oldMin, oldMax, newMin, newMax);
	u32 angle = ((u32)flare[0] << 12) / 20;
	s32 sin = (MATH_Sin(angle) * scale) >> 12;
	s32 cos = (MATH_Cos(angle) * scale) >> 12;
	s32 scaledCos = (cos << 9) / 0xf0;
	s32 scaledSin = (sin << 9) / 0xf0;

	CTC2(((u16)scaledCos) | ((u32)(u16)-scaledSin << 16), 0);
	CTC2((u32)sin << 16, 1);
	CTC2((u16)cos, 2);
	CTC2(0, 3);
	CTC2(scale, 4);

	struct Icon *icon = gGT->ptrIcons[0x87];
	if (icon == NULL)
		return;

	u32 texWord1 = (*(u32 *)&icon->texLayout.u1 & 0xff9fffff) | 0x00200000;
	u32 *p0 = prim;
	u32 *p1 = prim + 0xd;
	u32 *p2 = prim + 0x1a;
	u32 *p3 = prim + 0x27;

	FLARE_LoadGridRow(-409);
	gte_rtpt();
	FLARE_WriteTexture(p0, icon, texWord1);
	FLARE_WriteTexture(p1, icon, texWord1);
	p0[2] = MFC2(12);
	p0[5] = MFC2(13);
	p1[5] = MFC2(13);
	p1[2] = MFC2(14);

	FLARE_LoadGridRow(0);
	gte_rtpt();
	FLARE_WriteColors(p0);
	FLARE_WriteColors(p1);
	FLARE_WriteTexture(p2, icon, texWord1);
	p0[8] = MFC2(12);
	p0[11] = MFC2(13);
	p1[8] = MFC2(13);
	p1[11] = MFC2(14);
	p2[2] = MFC2(12);
	p2[5] = MFC2(13);
	p3[5] = MFC2(13);
	p3[2] = MFC2(14);
	s32 depth = MFC2(18);

	FLARE_LoadGridRow(409);
	gte_rtpt();
	FLARE_WriteColors(p2);
	FLARE_WriteColors(p3);
	FLARE_WriteTexture(p3, icon, texWord1);
	p2[8] = MFC2(12);
	p2[11] = MFC2(13);
	p3[8] = MFC2(13);
	p3[11] = MFC2(14);

	depth = (depth >> 8) - 2;
	if (depth < 0)
		depth = 0;
	if (depth > 0x3ff)
		depth = 0x3ff;

	u_long *ot = &pb->ptrOT[depth];
	p0[0] = FLARE_Ptr24(p1) | 0x0c000000;
	p1[0] = FLARE_Ptr24(p2) | 0x0c000000;
	p2[0] = FLARE_Ptr24(p3) | 0x0c000000;
	p3[0] = *ot | 0x0c000000;
	*ot = FLARE_Ptr24(p0);

	gGT->backBuffer->primMem.curr = prim + 0x34;
}
