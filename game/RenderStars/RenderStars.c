#include <common.h>

CTR_STATIC_ASSERT(sizeof(TILE_1) == 0x0C);
CTR_STATIC_ASSERT(offsetof(TILE_1, tag) == 0x00);
CTR_STATIC_ASSERT(offsetof(TILE_1, r0) == 0x04);
CTR_STATIC_ASSERT(offsetof(TILE_1, x0) == 0x08);

static s32 RenderStars_MulLowShift(s32 value, s32 scale)
{
	return (s32)(u32)((s64)value * (s64)scale) >> 12;
}

static u32 RenderStars_NextRngSelfOr(u32 *state0, u32 *state1)
{
	u32 shifted1 = *state1 >> 8;
	u32 mixed = *state0 + shifted1;

	*state0 = (*state0 >> 8) | mixed;
	mixed += *state0 >> 8;
	mixed <<= 24;
	*state1 = shifted1 | mixed;
	return mixed;
}

static u32 RenderStars_NextRngWithOr(u32 *state0, u32 *state1, u32 seedOr)
{
	u32 shifted1 = *state1 >> 8;
	u32 mixed = *state0 + shifted1;

	*state0 = (*state0 >> 8) | seedOr;
	mixed += *state0 >> 8;
	mixed <<= 24;
	*state1 = shifted1 | mixed;
	return mixed;
}

static int RenderStars_IsVisible(u32 gteFlag, u32 sxy)
{
	s32 clip = (s32)(gteFlag << 13);
	s32 bounds;

	if (clip < 0)
	{
		return 0;
	}

	bounds = (s32) ~((sxy - 0x00d90200) | sxy);
	if (bounds < 0)
	{
		return 0;
	}

	return ((s32)((u32)bounds << 16) >= 0);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006e26c-0x8006e588
void RenderStars(struct PushBuffer *pb, struct PrimMem *primMem, struct Stars *stars, char numPlyr)
{
	u32 *prim = (u32 *)primMem->cursor;
	int playerCount = numPlyr;

	// NOTE(aalhendi): PSX-backfeed blocker: retail uses 0x1f800000-0x1f80002c
	// to spill callee-save registers. This C port relies on the compiler ABI
	// because no retail data temporary lives in that scratchpad span.
	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	for (int playerIndex = playerCount - 1; playerIndex >= 0; playerIndex--, pb++)
	{
		s32 starIndex;
		u32 state0;
		u32 state1;
		u32 seedX;
		u32 seedY;
		u32 seedZ;
		int seedShift;
		int spread;
		uint32_t *ot;

		CTC2(*(u32 *)((char *)&pb->matrix_ViewProj + 0x00), 0);
		CTC2(*(u32 *)((char *)&pb->matrix_ViewProj + 0x04), 1);
		CTC2(*(u32 *)((char *)&pb->matrix_ViewProj + 0x08), 2);
		CTC2(*(u32 *)((char *)&pb->matrix_ViewProj + 0x0c), 3);
		CTC2(*(u32 *)((char *)&pb->matrix_ViewProj + 0x10), 4);

		CTC2((u32)(s32)pb->rect.w << 15, 24);
		CTC2((u32)(s32)pb->rect.h << 15, 25);
		CTC2((u32)pb->distanceToScreen_PREV, 26);
		CTC2(0, 5);
		CTC2(0, 6);
		CTC2(0, 7);

		starIndex = (u16)stars->numStars;
		spread = (u16)stars->spread;
		seedShift = (u16)stars->seed + 20;
		ot = &pb->ptrOT[(u16)stars->distance];
		state0 = 0x30125400;
		state1 = 0x493583fe;

		while (starIndex >= 0)
		{
			s32 starX;
			s32 starY;
			s32 starZ;
			s32 absX;
			s32 absY;
			s32 absZ;
			s32 scale;
			u32 packedXY;
			u32 colorSeed;
			u32 colorByte;
			u32 color;
			u32 gteFlag;
			u32 sxy;

			seedX = RenderStars_NextRngSelfOr(&state0, &state1);
			seedY = RenderStars_NextRngWithOr(&state0, &state1, seedX);
			seedZ = RenderStars_NextRngWithOr(&state0, &state1, seedX);

			starX = (s32)seedX >> 20;
			starY = (s32)seedY >> (seedShift & 0x1f);
			starZ = (s32)seedZ >> 20;

			absX = starX;
			if (absX < 0)
			{
				absX = -absX;
			}

			absY = starY;
			if (absY < 0)
			{
				absY = -absY;
				if (spread == 0)
				{
					starY = absY;
				}
			}

			absZ = starZ;
			if (absZ < 0)
			{
				absZ = -absZ;
			}

			if (absX - absY <= 0)
			{
				s32 oldAbsX = absX;
				absX = absY;
				absY = oldAbsX;
			}

			if (absX - absZ <= 0)
			{
				s32 oldAbsX = absX;
				absX = absZ;
				absZ = oldAbsX;
			}

			scale = 0x01000000 / (absX + (absY >> 2) + (absZ >> 2));

			packedXY = (u32)(RenderStars_MulLowShift(starX, scale) & 0xffff);
			packedXY |= (u32)RenderStars_MulLowShift(starY, scale) << 16;

			MTC2(packedXY, 0);
			MTC2((u32)RenderStars_MulLowShift(starZ, scale), 1);

			colorSeed = RenderStars_NextRngWithOr(&state0, &state1, packedXY);
			gte_rtps_b();

			colorByte = ((colorSeed >> 24) | 0x40) & 0xff;
			color = 0x68000000 | colorByte | (colorByte << 8) | (colorByte << 16);

			gteFlag = CFC2(31);
			sxy = MFC2(14);

			if (RenderStars_IsVisible(gteFlag, sxy))
			{
				TILE_1 *star = (TILE_1 *)prim;

				CtrGpu_WriteColorCode(&star->r0, color);
				CtrGpu_WritePackedXY(&star->x0, sxy);
				CtrGpu_LinkPacket24(ot, &star->tag, star, 0x02000000);
				prim = (u32 *)(star + 1);
			}

			starIndex--;
		}

		// Retail emits a length-2 draw-env packet: E1 tpage followed by a
		// zero terminator word.
		struct CtrGpuDrawModePacket *drawMode = (struct CtrGpuDrawModePacket *)prim;
		drawMode->drawMode = 0xe1000a20;
		drawMode->terminator = 0;
		CtrGpu_LinkPacket24(ot, &drawMode->tag, drawMode, 0x02000000);
		prim = (u32 *)(drawMode + 1);
	}

	primMem->cursor = prim;
}
