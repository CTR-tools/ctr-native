#include <common.h>

#define RED_BEAKER_CENTER_XY 0x02000080u
#define RED_BEAKER_XY_MASK   0xfffeffffu
#define RED_BEAKER_WRAP_MASK 0x01fe00ffu

struct RedBeakerRng
{
	u32 xy;
	s32 z;
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(LINE_G2) == 0x14);
CTR_STATIC_ASSERT_LAYOUT(offsetof(LINE_G2, tag) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(LINE_G2, r0) == 0x04);
CTR_STATIC_ASSERT_LAYOUT(offsetof(LINE_G2, x0) == 0x08);
CTR_STATIC_ASSERT_LAYOUT(offsetof(LINE_G2, r1) == 0x0C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(LINE_G2, x1) == 0x10);

static u32 RedBeaker_ReadWord(const void *base, int offset)
{
	return *(const u32 *)(const void *)((const char *)base + offset);
}

static s16 RedBeaker_ReadS16(const void *base, int offset)
{
	return *(const s16 *)(const void *)((const char *)base + offset);
}

static s8 RedBeaker_ReadS8(const void *base, int offset)
{
	return *(const s8 *)(const void *)((const char *)base + offset);
}

static u32 RedBeaker_NextRngByte(u32 *state0, u32 *state1)
{
	u32 state1Shifted = *state1 >> 8;
	u32 mixed = *state0 + state1Shifted;

	*state0 = (*state0 >> 8) | (*state1 << 24);
	mixed += *state0 >> 8;
	mixed <<= 24;
	*state1 = state1Shifted | mixed;

	return mixed;
}

static struct RedBeakerRng RedBeaker_NextRng(u32 *state0, u32 *state1)
{
	u32 xSeed = RedBeaker_NextRngByte(state0, state1);
	u32 ySeed = RedBeaker_NextRngByte(state0, state1);
	u32 zSeed = RedBeaker_NextRngByte(state0, state1);
	struct RedBeakerRng rng;

	rng.xy = ((u32)((s32)xSeed >> 24) & 0xffff) | (u32)((s32)ySeed >> 7);
	rng.z = (s32)zSeed >> 23;

	return rng;
}

static int RedBeaker_IsVisible(u32 gteFlag, u32 sxy0, u32 sxy1, u32 screenBounds)
{
	u32 overlap;
	u32 bounds;

	if ((s32)(gteFlag << 14) < 0)
		return 0;

	overlap = sxy0 & sxy1;
	bounds = ~((sxy0 - screenBounds) | (sxy1 - screenBounds)) | overlap;
	if ((s32)bounds < 0)
		return 0;

	return (s32)(bounds << 16) >= 0;
}

static void RedBeaker_EmitLine(u32 **primCursor, uint32_t *ot, u32 color)
{
	LINE_G2 *line = (LINE_G2 *)*primCursor;

	CtrGpu_WriteColorCode(&line->r0, 0x52000000);
	CtrGpu_WritePackedXY(&line->x0, MFC2(12));
	CtrGpu_WriteColorCode(&line->r1, color);
	CtrGpu_WritePackedXY(&line->x1, MFC2(13));
	CtrGpu_LinkPacket24(ot, &line->tag, line, 0x04000000);
	*primCursor = (u32 *)(line + 1);
}

static void RedBeaker_EmitDrawMode(u32 **primCursor, uint32_t *ot, u32 drawMode)
{
	struct CtrGpuDrawModePacket *packet = (struct CtrGpuDrawModePacket *)*primCursor;

	packet->drawMode = drawMode;
	packet->terminator = 0;
	CtrGpu_LinkPacket24(ot, &packet->tag, packet, 0x02000000);
	*primCursor = (u32 *)(packet + 1);
}

static void RedBeaker_RenderPass(u32 **primCursor, uint32_t *ot, u32 color, u32 drawMode, s32 frameCount, u32 scrollXY, u32 nextScrollXY, s32 scrollZ,
                                 s32 nextScrollZ, u32 spanXY, s32 spanZ, u32 screenBounds)
{
	u32 state0 = 0x30125400;
	u32 state1 = 0x493583fe;
	struct RedBeakerRng rng = RedBeaker_NextRng(&state0, &state1);
	s32 remaining = frameCount;

	while (remaining != 0)
	{
		u32 xy0;
		u32 xy1;
		u32 z0;
		u32 z1;

		remaining--;

		xy0 = ((rng.xy + scrollXY) & RED_BEAKER_WRAP_MASK) - RED_BEAKER_CENTER_XY;
		xy1 = ((rng.xy + nextScrollXY) & RED_BEAKER_WRAP_MASK) - RED_BEAKER_CENTER_XY;

		if ((u32)(xy1 - xy0) == spanXY)
		{
			xy0 &= RED_BEAKER_XY_MASK;
			xy1 &= RED_BEAKER_XY_MASK;

			MTC2(xy0, 0);
			MTC2(xy1, 2);
			MTC2(xy1, 4);

			z0 = (u32)(rng.z + scrollZ) & 0xff;
			z1 = (u32)(rng.z + nextScrollZ) & 0xff;

			if ((s32)(z1 - z0) == spanZ)
			{
				u32 sxy0;
				u32 sxy1;
				u32 gteFlag;

				z0 -= 0x80;
				z1 -= 0x80;

				MTC2(z0, 1);
				MTC2(z1, 3);
				MTC2(z1, 5);

				gte_rtpt_b();
				rng = RedBeaker_NextRng(&state0, &state1);

				sxy0 = MFC2(12);
				gteFlag = CFC2(31);
				sxy1 = MFC2(13);

				if (RedBeaker_IsVisible(gteFlag, sxy0, sxy1, screenBounds))
					RedBeaker_EmitLine(primCursor, ot, color);
			}
		}

		rng = RedBeaker_NextRng(&state0, &state1);
	}

	RedBeaker_EmitDrawMode(primCursor, ot, drawMode);
}

struct RedBeakerRainScratch
{
	u32 centerXY;
	u8 pad_04[0x14];
	u32 colorTop;
	u32 colorBottom;
};

CTR_STATIC_ASSERT_LAYOUT(offsetof(struct RedBeakerRainScratch, centerXY) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct RedBeakerRainScratch, colorTop) == 0x18);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct RedBeakerRainScratch, colorBottom) == 0x1C);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006dc30-0x8006e26c
void RedBeaker_RenderRain(struct PushBuffer *pb, struct PrimMem *primMem, struct JitPool *rain, char numPlyr, int gameMode1)
{
	u32 *prim = (u32 *)primMem->cursor;
	struct RainLocal *firstRain = (struct RainLocal *)rain->taken.first;
	struct RedBeakerRainScratch *scratch = CTR_SCRATCHPAD_PTR(struct RedBeakerRainScratch, 0x30);

	// NOTE(aalhendi): PSX-backfeed blocker: retail saves/restores callee registers in scratchpad 0x00-0x2c and stores 32-bit pointer cursors at 0x34/0x38.
	// Native C keeps pointer cursors as host-width locals; restore those exact scratchpad pointer stores before PSX backfeed.
	if (firstRain == NULL)
	{
		primMem->cursor = prim;
		return;
	}

	scratch->centerXY = RED_BEAKER_CENTER_XY;
	scratch->colorTop = 0;
	scratch->colorBottom = 0;

	for (int playerIndex = 0; playerIndex < numPlyr; playerIndex++, pb++)
	{
		struct RainLocal *rainLocal;
		u32 screenBounds;
		uint32_t *otBase;
		int playerOffset;

		CTC2(RedBeaker_ReadWord(&pb->matrix_ViewProj, 0x00), 0);
		CTC2(RedBeaker_ReadWord(&pb->matrix_ViewProj, 0x04), 1);
		CTC2(RedBeaker_ReadWord(&pb->matrix_ViewProj, 0x08), 2);
		CTC2(RedBeaker_ReadWord(&pb->matrix_ViewProj, 0x0c), 3);
		CTC2(RedBeaker_ReadWord(&pb->matrix_ViewProj, 0x10), 4);

		CTC2((u32)(s32)pb->rect.w << 15, 24);
		CTC2((u32)(s32)pb->rect.h << 15, 25);
		CTC2((u32)pb->distanceToScreen_PREV, 26);

		screenBounds = RedBeaker_ReadWord(pb, 0x20);
		otBase = pb->ptrOT;
		playerOffset = playerIndex * sizeof(struct InstDrawPerPlayer);

		for (rainLocal = firstRain; rainLocal != NULL; rainLocal = rainLocal->next)
		{
			char *instBase;
			s32 cloudZ;
			u32 scrollXY;
			s32 scrollZ;
			u32 velocityXY;
			s32 velocityZ;
			u32 nextScrollXY;
			s32 nextScrollZ;
			u8 fade;
			u8 top;
			s32 otOffset;
			uint32_t *ot;

			if (rainLocal->cloudInst == NULL)
				continue;

			scrollXY = RedBeaker_ReadWord(rainLocal, 0x0c);
			scrollZ = RedBeaker_ReadS16(rainLocal, 0x10);
			velocityXY = RedBeaker_ReadWord(rainLocal, 0x14) & RED_BEAKER_XY_MASK;
			velocityZ = RedBeaker_ReadS16(rainLocal, 0x18);
			nextScrollXY = (scrollXY + velocityXY) & RED_BEAKER_XY_MASK;
			nextScrollZ = scrollZ + velocityZ;

			if (gameMode1 == 0)
			{
				*(u32 *)(void *)((char *)rainLocal + 0x0c) = nextScrollXY;
				*(s16 *)(void *)((char *)rainLocal + 0x10) = (s16)nextScrollZ;
			}

			instBase = (char *)rainLocal->cloudInst + playerOffset;
			CTC2((u32)(s32)RedBeaker_ReadS16(instBase, 0x8c), 5);
			CTC2((u32)(s32)RedBeaker_ReadS16(instBase, 0x90), 6);
			cloudZ = RedBeaker_ReadS16(instBase, 0x94);
			CTC2((u32)cloudZ, 7);

			if (cloudZ < 0 || cloudZ >= 0xc00)
				continue;

			if (cloudZ < 0x400)
			{
				fade = 0xff;
			}
			else
			{
				fade = (u8)(0xff - (((u32)(cloudZ - 0x400) >> 3) & 0xff));
			}

			top = (u8)(fade - (fade >> 2));
			scratch->colorTop = (u32)top | ((u32)top << 8) | ((u32)fade << 16);
			scratch->colorBottom = (u32)fade | ((u32)fade << 8) | ((u32)fade << 16);

			otOffset = ((cloudZ >> 7) + RedBeaker_ReadS8(instBase, 0x50) - 6);
			if (otOffset < 0)
				otOffset = 0;
			else
				otOffset <<= 2;

			if (otOffset >= 0x1000)
				otOffset = 0xffc;

			ot = (uint32_t *)(void *)((char *)otBase + otOffset);

			RedBeaker_RenderPass(&prim, ot, scratch->colorTop, 0xe1000a20, rainLocal->frameCount, scrollXY, nextScrollXY, scrollZ, nextScrollZ, velocityXY,
			                     velocityZ, screenBounds);
			RedBeaker_RenderPass(&prim, ot, scratch->colorBottom, 0xe1000a40, rainLocal->frameCount, scrollXY, nextScrollXY, scrollZ, nextScrollZ, velocityXY,
			                     velocityZ, screenBounds);
		}
	}

	primMem->cursor = prim;
}
