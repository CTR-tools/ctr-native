#include <common.h>

static const u8 sDrawConfettiColorTable8008a2a0[36] = {
    0x39, 0x39, 0x3a, 0x3b, 0x3d, 0x3f, 0x41, 0x44, 0x47, 0x4b, 0x4f, 0x53, 0x58, 0x5d, 0x63, 0x69, 0x6f, 0x76,
    0x7d, 0x85, 0x8d, 0x95, 0x9e, 0xa7, 0xb1, 0xbb, 0xc5, 0xd0, 0xdb, 0xe7, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff,
};

struct DrawConfettiRng
{
	u32 x;
	u32 y;
	u32 z;
};

struct DrawConfettiScratch
{
	u32 remainingParticles;
	u32 color;
	u8 pad_08[0x20];
	u8 colorTable[36];
};

CTR_STATIC_ASSERT(offsetof(struct DrawConfettiScratch, remainingParticles) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DrawConfettiScratch, color) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct DrawConfettiScratch, colorTable) == 0x28);

static u32 DrawConfetti_ReadWord(const void *base, int offset)
{
	return *(const u32 *)(const void *)((const char *)base + offset);
}

static struct TrigPair DrawConfetti_TrigAngleSinCos(int angle)
{
	u32 packed = DrawConfetti_ReadWord(&data.trigApprox[ANG_MODULO_HALF_PI(angle)], 0);
	struct TrigPair pair;

	// NOTE(aalhendi): PSX-backfeed blocker: retail calls TRIG_AngleSinCos_r15r16r17 with angle in t7 and returns sine/cosine in s0/s1.
	// Native C uses an explicit helper; restore the register ABI before PSX backfeed.
	if (IS_ANG_FIRST_OR_THIRD_QUADRANT(angle))
	{
		pair.sin = (s16)packed;
		pair.cos = (s16)(packed >> 16);

		if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
		{
			pair.sin = -pair.sin;
			pair.cos = -pair.cos;
		}
	}
	else
	{
		pair.sin = (s16)(packed >> 16);
		pair.cos = (s16)packed;

		if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
		{
			pair.sin = -pair.sin;
		}
		else
		{
			pair.cos = -pair.cos;
		}
	}

	return pair;
}

static u32 DrawConfetti_NextRngByte(u32 *state0, u32 *state1)
{
	u32 state1Shifted = *state1 >> 8;
	u32 mixed = *state0 + state1Shifted;

	*state0 = (*state0 >> 8) | (*state1 << 24);
	mixed += *state0 >> 8;
	mixed <<= 24;
	*state1 = state1Shifted | mixed;

	return mixed;
}

static struct DrawConfettiRng DrawConfetti_NextRng(u32 *state0, u32 *state1)
{
	struct DrawConfettiRng rng;

	rng.x = DrawConfetti_NextRngByte(state0, state1);
	rng.y = DrawConfetti_NextRngByte(state0, state1);
	rng.z = DrawConfetti_NextRngByte(state0, state1);

	return rng;
}

static u32 DrawConfetti_PackXY(s32 x, s32 y)
{
	return (u32)(u16)x | ((u32)(u16)y << 16);
}

static u32 DrawConfetti_BuildColor(u32 cycle, u8 shade)
{
	u32 color = 0x28000000;

	if ((cycle & 1) != 0)
	{
		color |= shade;
	}

	if ((cycle & 2) != 0)
	{
		color |= (u32)shade << 8;
	}

	if ((cycle & 4) != 0)
	{
		color |= (u32)shade << 16;
	}

	return color;
}

static int DrawConfetti_BoundsVisible(u32 sxy0, u32 sxy1, u32 sxy2, u32 sxy3, u32 screenBounds)
{
	u32 overlap;
	u32 bounds;

	overlap = sxy0 & sxy1 & sxy2 & sxy3;
	// Retail uses the raw fourth SXY here, not `sxy3 - screenBounds`.
	bounds = ~((sxy0 - screenBounds) | (sxy1 - screenBounds) | (sxy2 - screenBounds) | sxy3) | overlap;
	if ((s32)bounds < 0)
	{
		return 0;
	}

	return (s32)(bounds << 16) >= 0;
}

static void DrawConfetti_LinkPrimitive(POLY_F4 *poly, uint32_t *ot)
{
	CtrGpu_LinkPacket24(ot, &poly->tag, poly, 0x05000000);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80069ffc-0x8006a4c4
void DrawConfetti(struct PushBuffer *pb, struct PrimMem *primMem, struct GameTrackerConfetti *confetti, int frameTimer, int gameMode1)
{
	POLY_F4 *prim = primMem->cursor;
	struct DrawConfettiScratch *scratch = CTR_SCRATCHPAD_PTR(struct DrawConfettiScratch, 0x30);
	struct TrigPair cameraTrig;
	u32 screenBounds;
	uint32_t *otBase;
	u32 currentParticles;
	u32 particleCount;
	s32 centerX;
	s32 centerZ;
	s32 baseX;
	s32 baseY;
	s32 baseZ;
	u32 timer;
	u32 cycle;
	u32 state0;
	u32 state1;
	struct DrawConfettiRng rng;

	// NOTE(aalhendi): PSX-backfeed blocker: retail saves/restores callee
	// registers in scratchpad 0x00-0x2c. Native C relies on the host ABI; the
	// retail data scratch range starts at 0x30 and is explicit below.
	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	for (int i = 0; i < (int)sizeof(sDrawConfettiColorTable8008a2a0); i++)
	{
		scratch->colorTable[i] = sDrawConfettiColorTable8008a2a0[i];
	}

	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x00), 0);
	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x04), 1);
	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x08), 2);
	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x0c), 3);
	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x10), 4);

	cameraTrig = DrawConfetti_TrigAngleSinCos(pb->rot.y);
	centerX = (cameraTrig.sin >> 2) + 0x400;
	centerZ = (cameraTrig.cos >> 2) + 0x400;

	CTC2((u32)(s32)pb->rect.w << 15, 24);
	CTC2((u32)(s32)pb->rect.h << 15, 25);
	CTC2((u32)pb->distanceToScreen_PREV, 26);

	currentParticles = confetti->numParticles_currWord;
	if (gameMode1 == 0)
	{
		u32 maxParticles = (u16)confetti->numParticles_max;
		u32 vanishRate = (u16)confetti->vanishRate;
		u32 diff = maxParticles - currentParticles;

		if (maxParticles != currentParticles)
		{
			if ((s32)diff < 0)
			{
				currentParticles -= vanishRate;
				if ((s32)(diff + vanishRate) > 0)
				{
					currentParticles = maxParticles;
				}
			}
			else
			{
				currentParticles += vanishRate;
				if ((s32)(diff - vanishRate) < 0)
				{
					currentParticles = maxParticles;
				}
			}

			confetti->numParticles_currWord = currentParticles;
		}
	}

	screenBounds = DrawConfetti_ReadWord(pb, 0x20);
	otBase = pb->ptrOT;
	timer = (u32)frameTimer;
	baseX = -pb->pos.x + centerX;
	baseY = (s32)((u32)confetti->velY * timer - (u32)(s32)pb->pos.y);
	baseZ = -pb->pos.z + centerZ;

	scratch->color = 0x28000000;
	cycle = 6;
	state0 = 0x30125400;
	state1 = 0x493583fe;
	rng = DrawConfetti_NextRng(&state0, &state1);
	particleCount = currentParticles;

	while (1)
	{
		s32 x;
		s32 y;
		s32 z;
		s32 angle;
		struct TrigPair spinA;
		struct TrigPair spinB;
		u8 shade;
		s32 halfY;
		s32 skewX;
		s32 halfX;
		s32 skewZ;
		s32 halfZ;
		u32 xy0;
		u32 xy1;
		u32 xy2;
		u32 xy3;
		s32 z0;
		s32 z1;
		s32 z2;
		s32 z3;
		u32 sxy0;
		u32 sxy1;
		u32 sxy2;
		u32 sxy3;
		u32 flag0;
		u32 flag1;
		u32 angleWord;

		cycle--;
		timer += 23;
		if (cycle == 0)
		{
			cycle = 6;
		}

		if (particleCount == 0)
		{
			break;
		}

		particleCount--;
		scratch->remainingParticles = particleCount;

		x = (((s32)rng.x >> 21) + baseX) & 0x7ff;
		y = (((s32)rng.y >> 21) + baseY) & 0x7ff;
		z = (((s32)rng.z >> 21) + baseZ) & 0x7ff;
		x -= centerX;
		y -= 0x400;
		z -= centerZ;

		angleWord = (cycle & 1) != 0 ? 0u - timer : timer;
		angle = (s32)(angleWord << 4);
		spinA = DrawConfetti_TrigAngleSinCos(angle);
		x += spinA.sin >> 5;
		z += spinA.cos >> 5;

		angle = (s32)((u32)angle << 3);
		spinB = DrawConfetti_TrigAngleSinCos(angle);

		shade = scratch->colorTable[((spinB.cos > 0) ? spinB.cos : -spinB.cos) >> 7];
		scratch->color = DrawConfetti_BuildColor(cycle, shade);

		halfY = spinB.cos >> 8;
		skewX = (spinB.sin * spinA.sin) >> 20;
		halfX = spinA.cos >> 8;
		skewZ = (spinB.sin * spinA.cos) >> 20;
		halfZ = spinA.sin >> 8;

		xy0 = DrawConfetti_PackXY(x - halfX - skewX, y - halfY);
		z0 = z - halfZ + skewZ;
		xy1 = DrawConfetti_PackXY(x + halfX - skewX, y - halfY);
		z1 = z + halfZ + skewZ;
		xy2 = DrawConfetti_PackXY(x - halfX + skewX, y + halfY);
		z2 = z - halfZ - skewZ;
		xy3 = DrawConfetti_PackXY(x + halfX + skewX, y + halfY);
		z3 = z + halfZ - skewZ;

		MTC2(xy0, 0);
		MTC2((u32)z0, 1);
		MTC2(xy1, 2);
		MTC2((u32)z1, 3);
		MTC2(xy2, 4);
		MTC2((u32)z2, 5);

		gte_rtpt_b();
		rng = DrawConfetti_NextRng(&state0, &state1);

		sxy0 = MFC2(12);
		flag0 = CFC2(31);
		sxy1 = MFC2(13);
		sxy2 = MFC2(14);
		CtrGpu_WritePackedXY(&prim->x0, sxy0);

		MTC2(xy3, 0);
		MTC2((u32)z3, 1);

		if ((s32)(flag0 << 14) >= 0)
		{
			gte_rtps_b();
			sxy3 = MFC2(14);
			flag1 = CFC2(31);

			if ((s32)(flag1 << 14) >= 0)
			{
				gte_avsz4_b();
				if (DrawConfetti_BoundsVisible(sxy0, sxy1, sxy2, sxy3, screenBounds))
				{
					u32 depth;
					uint32_t *ot;

					CtrGpu_WritePackedXY(&prim->x1, MFC2(12));
					CtrGpu_WritePackedXY(&prim->x2, MFC2(13));
					CtrGpu_WritePackedXY(&prim->x3, MFC2(14));
					depth = MFC2(24);
					CtrGpu_WriteColorCode(&prim->r0, scratch->color);
					ot = (uint32_t *)(void *)((char *)otBase + ((depth >> 18) << 2));
					DrawConfetti_LinkPrimitive(prim, ot);
					particleCount = scratch->remainingParticles;
					prim++;
					continue;
				}
			}
		}

		particleCount = scratch->remainingParticles;
	}

	primMem->cursor = prim;
}
