#include <common.h>

struct AnimateQuadTrig
{
	s32 sin;
	s32 cos;
};

static struct AnimateQuadTrig AnimateQuad_GetTrig(s32 angle)
{
	u32 packed = CTR_ReadU32LE(&data.trigApprox[ANG_MODULO_HALF_PI(angle)]);
	struct AnimateQuadTrig trig;

	if (IS_ANG_FIRST_OR_THIRD_QUADRANT(angle))
	{
		trig.sin = (s16)packed;
		trig.cos = (s32)packed >> 16;
	}
	else
	{
		trig.sin = (s32)packed >> 16;
		trig.cos = -(s16)packed;
	}

	if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
	{
		trig.sin = -trig.sin;
		trig.cos = -trig.cos;
	}

	return trig;
}

void AnimateQuadVertex(int timer, struct SCVert *scVert, u32 *visBits)
{
	u32 bits = *visBits;
	*visBits = bits >> 1;

	if ((bits & 1) == 0)
	{
		return;
	}

	struct LevVertex *levVert = scVert->v;
	u32 offsetPosZw = (u32)scVert->offset_pos_zw;
	s32 flags = (s32)offsetPosZw >> 16;
	struct AnimateQuadTrig trig = AnimateQuad_GetTrig((flags & 0x3fff) + timer);

	if (flags < 0)
	{
		u32 offsetPosXy = (u32)scVert->offset_pos_xy;
		u32 originalY = ((offsetPosXy >> 16) << 16);
		u32 animatedXy = offsetPosXy + (u32)(trig.sin >> 7);
		u32 animatedZw = offsetPosZw + (u32)(trig.cos >> 7);

		CTR_WriteU32LE(&levVert->pos, animatedXy | originalY);
		levVert->pos.z = (s16)animatedZw;
	}

	if ((flags & 0x4000) != 0)
	{
		MTC2((u32)((trig.sin + 0x1000) >> 2), 8);
		MTC2((u32)scVert->offset_color_rgba, 6);
		gte_dpcs();

		u32 color = MFC2(22);
		CTR_WriteU32LE(&levVert->color_hi[0], color);
		CTR_WriteU32LE(&levVert->color_lo[0], color);
	}
}

void AnimateQuad(int timer, int numSCVert, struct SCVert *scVert, int *visSCVertList)
{
#if defined(CTR_NATIVE)
	// MultiTrack: custom tracks can have animated vertices without their
	// visibility list: the PS1 reads its RAM at 0 there, natively that faults.
	// A missing list counts as everything visible (it advances over a stand-in)
	static u32 s_allVisible[1] = {0xffffffffu};
	int haveList = (visSCVertList != NULL);
	if ((numSCVert == 0) || (scVert == NULL))
	{
		return;
	}
	u32 *visList = haveList ? (u32 *)visSCVertList : s_allVisible;
	u32 visBits = *visList;
	visList += haveList;
#else
	u32 *visList = (u32 *)visSCVertList;
	u32 visBits = *visList++;
#endif
	int bitCount = 32;

	CTC2(0, 21);
	CTC2(0, 22);
	CTC2(0, 23);

	scVert--;

	while (numSCVert != 0)
	{
		numSCVert--;

		if (bitCount == 0)
		{
#if defined(CTR_NATIVE)
			visBits = *visList;
			visList += haveList;
#else
			visBits = *visList++;
#endif
			bitCount = 31;
		}
		else
		{
			bitCount--;
		}

		scVert++;
		AnimateQuadVertex(timer, scVert, &visBits);
	}
}

void AnimateWaterVertex(struct WaterVert *waterVert, u16 colorOffset, int firstOffset, int secondOffset, u32 *visBits)
{
	u32 bits = *visBits;
	*visBits = bits >> 1;

	if ((bits & 1) == 0)
	{
		return;
	}

	struct LevVertex *levVert = waterVert->v;
	struct OVert *waterColor = waterVert->w;
	u16 first = CTR_ReadU16LE((char *)waterColor + firstOffset);
	u16 second = CTR_ReadU16LE((char *)waterColor + secondOffset);

	u32 rgb = (first & 0x3f);
	u32 farR = (second & 0x3f) << 4;
	u32 farG = (second & 0xfc0) >> 2;
	u32 farB = (second & 0xf000) >> 4;

	rgb |= (first & 0xfc0) << 2;
	rgb |= (first & 0xf000) << 8;
	rgb |= (first & 0xf000) << 4;
	farB |= (second & 0xf000) >> 8;

	MTC2(rgb, 6);
	CTC2(farR, 21);
	CTC2(farG, 22);
	CTC2(farB, 23);
	gte_dpcs();

	u32 color = MFC2(22);
	CTR_WriteU16LE(&levVert->color_lo[0], (u16)(color + colorOffset));

	color = (color >> 16) & 0xff;
	CTR_WriteU32LE(&levVert->color_hi[0], (color << 16) | (color << 8) | color);
}

static u16 AnimateWater_ReadTextureHalf(const struct TextureLayout *layout, int offset)
{
	return CTR_ReadU16LE((const u8 *)layout + offset);
}

static void AnimateWater_Common(int timer, int numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, int numLists,
                                int **visLists)
{
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): CTR_NATIVE divergence, not retail ASM: native tracks can
	// supply empty water lists with null water data.
	if (numWaterVertices == 0)
	{
		return;
	}
	// MultiTrack: custom tracks can have water without its visibility lists (or
	// env map): the PS1 reads its RAM at 0 there, natively that faults. A
	// missing list counts as everything visible (the water still moves)
	if (waterEnvMap == NULL)
	{
		return;
	}
	static u32 s_allVisible[1];
	for (int i = 0; i < numLists; i++)
	{
		if (visLists[i] == NULL)
		{
			visLists[i] = (int *)s_allVisible;
		}
	}
	s_allVisible[0] = 0xffffffffu;
#endif

	u16 colorOffset = AnimateWater_ReadTextureHalf(waterEnvMap, 0);
	u32 visBits = 0;
	u32 *visList[4];
	int firstFrame;
	int secondFrame;
	int firstOffset;
	int secondOffset;
	int bitCount = 32;

	for (int i = 0; i < numLists; i++)
	{
		visList[i] = (u32 *)visLists[i];
		visBits |= *visList[i];
#if defined(CTR_NATIVE)
		visList[i] += (visLists[i] != (int *)s_allVisible); // (the stand-in list doesn't advance)
#else
		visList[i]++;
#endif
	}

	firstFrame = ((u32)timer >> 3) % 28;
	secondFrame = (firstFrame + 1) % 28;
	firstOffset = firstFrame * 2;
	secondOffset = secondFrame * 2;

	MTC2((timer & 7) << 9, 8);

	waterVert--;

	while (numWaterVertices != 0)
	{
		numWaterVertices--;

		if (bitCount == 0)
		{
			visBits = 0;

			for (int i = 0; i < numLists; i++)
			{
				visBits |= *visList[i];
#if defined(CTR_NATIVE)
				visList[i] += (visLists[i] != (int *)s_allVisible);
#else
				visList[i]++;
#endif
			}

			bitCount = 31;
		}
		else
		{
			bitCount--;
		}

		waterVert++;
		AnimateWaterVertex(waterVert, colorOffset, firstOffset, secondOffset, &visBits);
	}
}

void AnimateWater1P(int timer, int numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, int *visOVertList)
{
	int *visLists[1] = {visOVertList};
	AnimateWater_Common(timer, numWaterVertices, waterVert, waterEnvMap, 1, visLists);
}

void AnimateWater2P(int timer, int numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, int *visOVertList0,
                    int *visOVertList1)
{
	int *visLists[2] = {visOVertList0, visOVertList1};
	AnimateWater_Common(timer, numWaterVertices, waterVert, waterEnvMap, 2, visLists);
}

void AnimateWater3P(int timer, int numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, int *visOVertList0,
                    int *visOVertList1, int *visOVertList2)
{
	int *visLists[3] = {visOVertList0, visOVertList1, visOVertList2};
	AnimateWater_Common(timer, numWaterVertices, waterVert, waterEnvMap, 3, visLists);
}

void AnimateWater4P(int timer, int numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, int *visOVertList0,
                    int *visOVertList1, int *visOVertList2, int *visOVertList3)
{
	int *visLists[4] = {visOVertList0, visOVertList1, visOVertList2, visOVertList3};
	AnimateWater_Common(timer, numWaterVertices, waterVert, waterEnvMap, 4, visLists);
}
