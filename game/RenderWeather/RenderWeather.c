#include <common.h>

#define RENDER_WEATHER_XY_MASK   0xfffeffffu
#define RENDER_WEATHER_WRAP_MASK 0x07fe07ffu

struct RenderWeatherTrigPair
{
	s32 sin;
	s32 cos;
};

static u32 RenderWeather_ReadWord(const void *base, int offset)
{
	return *(const u32 *)(const void *)((const char *)base + offset);
}

static u32 RenderWeather_Ptr24(const void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static struct RenderWeatherTrigPair RenderWeather_TrigAngleSinCos(int angle)
{
	u32 packed = RenderWeather_ReadWord(&data.trigApprox[angle & 0x3ff], 0);
	struct RenderWeatherTrigPair pair;

	// NOTE(aalhendi): PSX-backfeed blocker: retail calls
	// TRIG_AngleSinCos_r16r17r18 with angle in s0 and returns sine/cosine in
	// s1/s2. Native C uses an explicit helper; restore the register ABI before
	// PSX backfeed.
	if ((angle & 0x400) == 0)
	{
		pair.sin = (s16)packed;
		pair.cos = (s16)(packed >> 16);

		if ((angle & 0x800) != 0)
		{
			pair.sin = -pair.sin;
			pair.cos = -pair.cos;
		}
	}
	else
	{
		pair.sin = (s16)(packed >> 16);
		pair.cos = (s16)packed;

		if ((angle & 0x800) != 0)
			pair.sin = -pair.sin;
		else
			pair.cos = -pair.cos;
	}

	return pair;
}

static u32 RenderWeather_NextRngByte(u32 *state0, u32 *state1)
{
	u32 state1Shifted = *state1 >> 8;
	u32 mixed = *state0 + state1Shifted;

	*state0 = (*state0 >> 8) | (*state1 << 24);
	mixed += *state0 >> 8;
	mixed <<= 24;
	*state1 = state1Shifted | mixed;

	return mixed;
}

static void RenderWeather_NextRng(u32 *state0, u32 *state1, u32 *xy, u32 *z)
{
	u32 xSeed = RenderWeather_NextRngByte(state0, state1);
	u32 ySeed = RenderWeather_NextRngByte(state0, state1);
	u32 zSeed = RenderWeather_NextRngByte(state0, state1);

	*xy = (u32)(((s32)xSeed >> 21) & 0xffff) | (u32)((s32)ySeed >> 5);
	*z = zSeed >> 21;
}

static int RenderWeather_IsVisible(u32 gteFlag, u32 sxy0, u32 sxy1, u32 screenBounds)
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

static void RenderWeather_LinkPrimitive(u32 *prim, u_long *ot, u32 tag)
{
	prim[0] = (u32)*ot | tag;
	*ot = (u_long)RenderWeather_Ptr24(prim);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006f9a8-0x8006fe08
void RenderWeather(struct PushBuffer *pb, struct PrimMem *primMem, struct RainBuffer *rainBuffer, char numPlyr, int gameMode1)
{
	u32 *prim = (u32 *)primMem->curr;
	u32 *rainWords = (u32 *)(void *)rainBuffer;
	// NOTE(aalhendi): PSX-backfeed blocker: retail saves/restores callee
	// registers in scratchpad 0x00-0x2c. Native C relies on the host ABI; restore
	// the scratchpad register-save prologue/epilogue before PSX backfeed.
	u32 *scratchColorTop = CTR_SCRATCHPAD_PTR(u32, 0x30);
	u32 *scratchColorBottom = CTR_SCRATCHPAD_PTR(u32, 0x34);
	u32 *scratchPackedCenterXY = CTR_SCRATCHPAD_PTR(u32, 0x38);
	u32 *scratchCenterZ = CTR_SCRATCHPAD_PTR(u32, 0x3c);
	struct RenderWeatherTrigPair trig;
	u32 screenBounds;
	u_long *ot;
	s32 currentParticles;
	u32 particleCount;
	u32 scrollXY;
	s32 scrollZ;
	u32 velocityXY;
	s32 velocityZ;
	u32 scrollXYEnd;
	s32 scrollZEnd;
	u32 scrollXYStart;
	s32 scrollZStart;
	u32 cameraXY;
	s32 cameraZ;
	u32 prevCameraXY;
	s32 prevCameraZ;
	u32 cameraDeltaXY;
	u32 cameraCorrectionXY;
	u32 smoothedCameraXY;
	s32 smoothedCameraZ;
	u32 startXY;
	u32 endXY;
	s32 startZ;
	s32 endZ;
	u32 spanXY;
	u32 spanZ;
	u32 fillMode;
	u32 state0;
	u32 state1;
	u32 rngXY;
	u32 rngZ;

	(void)numPlyr;

	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x00), 0);
	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x04), 1);
	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x08), 2);
	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x0c), 3);
	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x10), 4);

	trig = RenderWeather_TrigAngleSinCos(pb->rot[1]);
	*scratchPackedCenterXY = 0x04000000u | (u32)((trig.sin >> 2) + 0x400);
	*scratchCenterZ = (u32)((trig.cos >> 2) + 0x400);

	screenBounds = RenderWeather_ReadWord(pb, 0x20);
	ot = &pb->ptrOT[rainBuffer->offsetOT];

	CTC2((u32)(s32)pb->rect.w << 15, 24);
	CTC2((u32)(s32)pb->rect.h << 15, 25);
	CTC2((u32)pb->distanceToScreen_PREV, 26);

	currentParticles = rainBuffer->numParticles_curr;
	if (gameMode1 == 0)
	{
		s32 maxParticles = (u16)rainBuffer->numParticles_max;
		s32 vanishRate = (u16)rainBuffer->vanishRate;
		s32 diff = maxParticles - currentParticles;

		if (maxParticles != currentParticles)
		{
			if (diff < 0)
			{
				currentParticles -= vanishRate;
				if (diff + vanishRate > 0)
					currentParticles = maxParticles;
			}
			else
			{
				currentParticles += vanishRate;
				if (diff - vanishRate < 0)
					currentParticles = maxParticles;
			}

			rainBuffer->numParticles_curr = currentParticles;
		}
	}

	particleCount = (u32)currentParticles;

	scrollXY = rainWords[2];
	scrollZ = (s32)rainWords[3];
	velocityXY = rainWords[4] & RENDER_WEATHER_XY_MASK;
	velocityZ = (s32)rainWords[5];

	scrollXYEnd = (scrollXY + velocityXY) & RENDER_WEATHER_XY_MASK;
	scrollZEnd = scrollZ + velocityZ;
	if (gameMode1 == 0)
	{
		rainWords[2] = scrollXYEnd;
		rainWords[3] = (u32)scrollZEnd;
	}

	scrollXYStart = (scrollXY + ((u32)((s32)velocityXY >> 1) & RENDER_WEATHER_XY_MASK)) & RENDER_WEATHER_XY_MASK;
	scrollZStart = scrollZ + (velocityZ >> 1);

	cameraXY = RenderWeather_ReadWord(pb, 0x00) & RENDER_WEATHER_XY_MASK;
	cameraZ = pb->pos[2];
	prevCameraXY = rainWords[6];
	prevCameraZ = *(s16 *)(void *)((char *)rainBuffer + 0x1c);
	if (gameMode1 == 0)
	{
		rainWords[6] = cameraXY;
		*(s16 *)(void *)((char *)rainBuffer + 0x1c) = pb->pos[2];
	}

	cameraDeltaXY = cameraXY - prevCameraXY;
	cameraCorrectionXY = (u32)(((s32)cameraDeltaXY >> 20) << 17) | (u32)((((s32)(cameraDeltaXY << 16)) >> 19) & 0xffff);
	smoothedCameraXY = (cameraXY - cameraCorrectionXY) & RENDER_WEATHER_XY_MASK;
	smoothedCameraZ = cameraZ - ((cameraZ - prevCameraZ) >> 3);

	startXY = ((scrollXYStart - smoothedCameraXY) & RENDER_WEATHER_XY_MASK) + *scratchPackedCenterXY;
	startXY &= RENDER_WEATHER_XY_MASK;
	endXY = ((scrollXYEnd - cameraXY) & RENDER_WEATHER_XY_MASK) + *scratchPackedCenterXY;
	endXY &= RENDER_WEATHER_XY_MASK;
	startZ = (scrollZStart - smoothedCameraZ) + (s32)*scratchCenterZ;
	endZ = (scrollZEnd - cameraZ) + (s32)*scratchCenterZ;
	spanXY = endXY - startXY;
	spanZ = (u32)(endZ - startZ);

	*scratchColorTop = rainBuffer->colorRGBA_top | 0x52000000u;
	*scratchColorBottom = rainBuffer->colorRGBA_bottom;
	fillMode = rainBuffer->fillMode;

	state0 = 0x30125400;
	state1 = 0x493583fe;
	RenderWeather_NextRng(&state0, &state1, &rngXY, &rngZ);

	while (particleCount != 0)
	{
		u32 xy0;
		u32 xy1;
		u32 z0;
		u32 z1;

		particleCount--;

		xy0 = ((rngXY + startXY) & RENDER_WEATHER_WRAP_MASK) - *scratchPackedCenterXY;
		xy1 = ((rngXY + endXY) & RENDER_WEATHER_WRAP_MASK) - *scratchPackedCenterXY;

		if ((u32)(xy1 - xy0) == spanXY)
		{
			xy0 &= RENDER_WEATHER_XY_MASK;
			xy1 &= RENDER_WEATHER_XY_MASK;

			MTC2(xy0, 0);
			MTC2(xy1, 2);
			MTC2(xy1, 4);

			z0 = ((rngZ + (u32)startZ) & 0x7ff);
			z1 = ((rngZ + (u32)endZ) & 0x7ff);
			if ((u32)(z1 - z0) == spanZ)
			{
				u32 sxy0;
				u32 sxy1;
				u32 gteFlag;

				z0 -= *scratchCenterZ;
				z1 -= *scratchCenterZ;

				MTC2(z0, 1);
				MTC2(z1, 3);
				MTC2(z1, 5);

				gte_rtpt_b();
				RenderWeather_NextRng(&state0, &state1, &rngXY, &rngZ);

				sxy0 = MFC2(12);
				gteFlag = CFC2(31);
				sxy1 = MFC2(13);

				if (RenderWeather_IsVisible(gteFlag, sxy0, sxy1, screenBounds))
				{
					prim[1] = *scratchColorTop;
					prim[3] = *scratchColorBottom;
					prim[2] = sxy0;
					prim[4] = sxy1;
					RenderWeather_LinkPrimitive(prim, ot, 0x04000000);
					prim += 5;
				}

				continue;
			}
		}

		RenderWeather_NextRng(&state0, &state1, &rngXY, &rngZ);
	}

	prim[1] = fillMode;
	prim[2] = 0;
	RenderWeather_LinkPrimitive(prim, ot, 0x02000000);
	prim += 3;

	primMem->curr = prim;
}
