#include <common.h>

struct AnimateQuadTrig
{
	s32 sin;
	s32 cos;
};

static struct AnimateQuadTrig AnimateQuad_GetTrig(s32 angle)
{
	u32 packed = *(u32 *)&data.trigApprox[angle & 0x3ff];
	struct AnimateQuadTrig trig;

	if ((angle & 0x400) == 0)
	{
		trig.sin = (s16)packed;
		trig.cos = (s32)packed >> 16;
	}
	else
	{
		trig.sin = (s32)packed >> 16;
		trig.cos = -(s16)packed;
	}

	if ((angle & 0x800) != 0)
	{
		trig.sin = -trig.sin;
		trig.cos = -trig.cos;
	}

	return trig;
}

void AnimateQuadVertex(int timer, struct SCVert *scVert, u32 *visBits)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80069f0c-0x80069f94.
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

		*(u32 *)&levVert->pos = animatedXy | originalY;
		levVert->pos.z = (s16)animatedZw;
	}

	if ((flags & 0x4000) != 0)
	{
		MTC2((u32)((trig.sin + 0x1000) >> 2), 8);
		MTC2((u32)scVert->offset_color_rgba, 6);
		gte_dpcs();

		u32 color = MFC2(22);
		*(u32 *)&levVert->color_hi[0] = color;
		*(u32 *)&levVert->color_lo[0] = color;
	}
}

void AnimateQuad(int timer, int numSCVert, void *ptrSCVert, int *visSCVertList)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80069e70-0x80069f0c.
	struct SCVert *scVert = (struct SCVert *)ptrSCVert;
	u32 *visList = (u32 *)visSCVertList;
	u32 visBits = *visList++;
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
			visBits = *visList++;
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
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006db7c-0x8006dc30.
	u32 bits = *visBits;
	*visBits = bits >> 1;

	if ((bits & 1) == 0)
	{
		return;
	}

	struct LevVertex *levVert = waterVert->v;
	struct OVert *waterColor = waterVert->w;
	u16 first = *(u16 *)((char *)waterColor + firstOffset);
	u16 second = *(u16 *)((char *)waterColor + secondOffset);

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
	*(u16 *)&levVert->color_lo[0] = (u16)(color + colorOffset);

	color = (color >> 16) & 0xff;
	*(u32 *)&levVert->color_hi[0] = (color << 16) | (color << 8) | color;
}

static u16 AnimateWater_ReadTextureHalf(const struct TextureLayout *layout, int offset)
{
	const u8 *bytes = (const u8 *)layout + offset;

	return (u16)((u16)bytes[0] | ((u16)bytes[1] << 8));
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
		visBits |= *visList[i]++;
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
				visBits |= *visList[i]++;
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

void AnimateWater1P(int timer, int numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, int *param_5)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006d79c-0x8006d864.
	int *visLists[1] = {param_5};
	AnimateWater_Common(timer, numWaterVertices, waterVert, waterEnvMap, 1, visLists);
}

void AnimateWater2P(int timer, int numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, int *param_5, int *param_6)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006d864-0x8006d948.
	int *visLists[2] = {param_5, param_6};
	AnimateWater_Common(timer, numWaterVertices, waterVert, waterEnvMap, 2, visLists);
}

void AnimateWater3P(int timer, int numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, int *param_5, int *param_6,
                    int *param_7)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006d948-0x8006da50.
	int *visLists[3] = {param_5, param_6, param_7};
	AnimateWater_Common(timer, numWaterVertices, waterVert, waterEnvMap, 3, visLists);
}

void AnimateWater4P(int timer, int numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, int *param_5, int *param_6,
                    int *param_7, int *param_8)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006da50-0x8006db7c.
	int *visLists[4] = {param_5, param_6, param_7, param_8};
	AnimateWater_Common(timer, numWaterVertices, waterVert, waterEnvMap, 4, visLists);
}
