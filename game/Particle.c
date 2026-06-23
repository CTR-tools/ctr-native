#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003eae0-0x8003ec18.
void Particle_FuncPtr_PotionShatter(struct Particle *p)
{
	s16 sVar2;
	int rng;

	if (p->axis[1].velocity < 0x578)
	{
		if (p->axis[0].velocity != 0)
			goto LAB_8003ebc8;

		// random X
		rng = MixRNG_Scramble();
		p->axis[0].velocity = rng + (rng / 800) * -800 - 400;

		// random Z
		rng = MixRNG_Scramble();
		p->axis[2].velocity = rng + (rng / 800) * -800 - 400;

		// random scale
		rng = MixRNG_Scramble();
		sVar2 = (rng >> 8);
		if (rng < 0)
		{
			sVar2 = ((rng + 0xff) >> 8);
		}
		p->axis[5].velocity = rng + sVar2 * -0x100 + 0x100;
	}
	if (p->axis[0].velocity == 0)
		return;

LAB_8003ebc8:

	// green shatter or red shatter
	if (p->modelID == STATIC_SHOCKWAVE_GREEN)
	{
		if (0 < p->axis[8].startVal)
		{
			p->axis[8].startVal -= 0x1200;
		}
	}
	else
	{
		if (0 < p->axis[7].startVal)
		{
			p->axis[7].startVal -= 0x1200;
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003ec18-0x8003ee20.
void Particle_FuncPtr_SpitTire(struct Particle *p)
{
	int rng;
	int iVar2;
	int targetY;

	// Wait until tires are 0x10 units above
	// the ground, which is where the plant
	// actually "spits" tires from the mouth
	targetY = p->plantInst->matrix.t[1] + 0x10;

	if ((p->axis[1].startVal >> 8) >= targetY)
	{
		return;
	}

	// random X
	rng = MixRNG_Scramble();
	p->axis[0].velocity = rng + (rng / 0x1640) * -0x1640 - 0xb20;

	// random Z
	rng = MixRNG_Scramble();
	p->axis[2].velocity = rng + (rng / 0x1640) * -0x1640 - 0xb20;

	// scale value
	iVar2 = p->axis[5].startVal;

	switch (iVar2)
	{
	// frame #1
	case 0x1000:
	{
		// random Y
		rng = MixRNG_Scramble();
		p->axis[1].velocity = rng + (rng / 0x12c0) * -0x12c0 + 0x1900;

		// frame #2
		p->axis[5].startVal = 0xfff;
		break;
	}

	// frame #2
	case 0xfff:
	{
		// random Y
		rng = MixRNG_Scramble();
		p->axis[1].velocity = rng + (rng / 800) * -800 + 8000;

		// frame #3
		p->axis[5].startVal = 0xffe;
		break;
	}

	// frame #3
	case 0xffe:
	{
		// random Y
		rng = MixRNG_Scramble();
		p->axis[1].velocity = rng + (rng / 800) * -800 + 6000;

		p->axis[5].velocity = 0xf801;
		break;
	}

	default:
		return;
	}

	p->axis[1].startVal = targetY * 0x100;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003ee20-0x8003eeb0
void Particle_FuncPtr_ExhaustUnderwater(struct Particle *p)
{
	struct IconGroup *icon;

	if ((3 < ((p->axis[1].startVal >> 8) + p->driverInst->matrix.t[1])) && (p->framesLeftInLife < 27))
	{
		// bubblepop
		icon = sdata->gGT->iconGroup[8];
		p->ptrIconGroup = icon;

		if (icon != NULL)
		{
			struct Icon **ptrIconArray = ICONGROUP_GETICONS(icon);

			// actually the first icon pointer in the array,
			// not the pointer to the array itself
			p->ptrIconArray = ptrIconArray[0];
		}

		p->axis[4].startVal = MixRNG_Scramble() & 0xfff;
		p->framesLeftInLife = 0;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003eeb0-0x8003eefc.
void Particle_OnDestroy(struct Particle *p)
{
	struct ParticleOscillator *osc;

	osc = p->oscillator;

	while (osc != NULL)
	{
		struct ParticleOscillator *next = osc->next;

		LIST_AddFront(&sdata->gGT->JitPools.oscillator.free, (struct Item *)osc);
		osc = next;
	}
}


static u32 Particle_GetAxisFlags(const struct Particle *p)
{
	return *(const u32 *)&p->flagsAxis;
}

static void Particle_SetAxisFlags(struct Particle *p, u32 flags)
{
	*(u32 *)&p->flagsAxis = flags;
}

static int Particle_OscillatorValue(struct ParticleOscillator *osc)
{
	int value;
	int timer = sdata->gGT->frameTimer_Confetti;
	int phase = timer + osc->phase;
	int product = (int)osc->period * phase;

	switch (osc->flags & 7)
	{
	case 0:
		value = MATH_Sin(product >> 5);
		break;

	case 1:
		value = MATH_Sin(product >> 6);
		if (value < 0)
			value = -value;
		value = (value << 1) - 0x1000;
		break;

	case 2:
		value = ((product >> 4) & 0x1fff) - 0x1000;
		break;

	case 3:
		value = (product >> 3) & 0x3fff;
		if (value > 0x2000)
			value = 0x4000 - value;
		value -= 0x1000;
		break;

	case 4:
		value = -0x1000;
		if (((product >> 6) & 0x400) != 0)
			value = 0x1000;
		break;

	case 5:
		value = (MixRNG_Scramble() >> 3) - 0x1000;
		break;

	case 6:
		value = ((int)MixRNG_GetValue((s16)osc->previousValue) >> 3) - 0x1000;
		break;

	default:
		value = timer;
		break;
	}

	value = ((value + osc->offset) * (int)osc->scale) >> 12;

	if (value > osc->max)
		value = osc->max;
	if (value < osc->min)
		value = osc->min;

	return value;
}

static void Particle_ApplyOscillator(struct ParticleAxis *axis, struct ParticleOscillator *osc)
{
	int value;

	if ((osc->flags & 8) == 0)
	{
		if ((osc->flags & 0x10) == 0)
			axis->startVal -= (s16)osc->previousValue;
		else
			axis->velocity = (s16)(axis->velocity - osc->previousValue);
	}

	value = Particle_OscillatorValue(osc);

	if ((osc->flags & 0x10) == 0)
		axis->startVal += value;
	else
		axis->velocity = (s16)(axis->velocity + value);

	osc->previousValue = (s16)value;
}

static int Particle_ColorExpired(struct Particle *p, u16 activeFlags)
{
	int value = 0;

	if ((activeFlags & 0x80) != 0 && p->axis[7].startVal > 0)
		value = p->axis[7].startVal;

	if ((activeFlags & 0x100) != 0 && p->axis[8].startVal > 0)
		value |= p->axis[8].startVal;

	if ((activeFlags & 0x200) != 0 && p->axis[9].startVal > 0)
		value |= p->axis[9].startVal;

	return value < 0x800;
}

static void Particle_UpdateIconFrame(struct Particle *p, u16 flagsSetColor)
{
	int frame = p->axis[10].startVal;
	int frameLimit = p->ptrIconGroup->numIcons << 8;

	if (frame < 0)
	{
		if ((flagsSetColor & 0x100) != 0)
		{
			frame += frameLimit;
		}
		else if ((flagsSetColor & 0x200) != 0)
		{
			frame -= p->axis[10].velocity * 2;
			p->axis[10].accel = -p->axis[10].accel;
			p->axis[10].velocity = -p->axis[10].velocity;
		}
		else
		{
			frame = 0;
		}
	}
	else
	{
		if (frame < frameLimit)
			return;

		if ((flagsSetColor & 0x100) != 0)
		{
			frame -= frameLimit;
		}
		else if ((flagsSetColor & 0x200) != 0)
		{
			frame -= p->axis[10].velocity * 2;
			p->axis[10].accel = -p->axis[10].accel;
			p->axis[10].velocity = -p->axis[10].velocity;
		}
		else
		{
			frame = frameLimit - 1;
		}
	}

	p->axis[10].startVal = frame;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003eefc-0x8003f434
void Particle_UpdateList(struct Particle **listHead, struct Particle *p)
{
	struct Particle **link = listHead;

	while (p != NULL)
	{
		struct Particle *next = p->next;
		u16 flagsSetColor;
		u32 axisFlags;
		u16 activeFlags;
		struct ParticleOscillator *osc;

		p->framesLeftInLife = (s16)(p->framesLeftInLife - 1);
		if (p->framesLeftInLife == -1)
			goto destroyParticle;

		flagsSetColor = p->flagsSetColor;
		if ((flagsSetColor & 8) != 0)
			goto destroyParticle;

		if ((flagsSetColor & 0x1000) != 0)
		{
			p->axis[3].startVal = p->axis[0].startVal;
			p->axis[6].startVal = p->axis[1].startVal;
			p->axis[4].startVal = p->axis[2].startVal;

			if ((flagsSetColor & 0x4000) == 0)
				*(u32 *)&p->axis[10].startVal = *(u32 *)&p->axis[10].velocity;
		}

		axisFlags = Particle_GetAxisFlags(p);
		osc = p->oscillator;

		for (int axisIndex = 0; axisFlags != 0; axisIndex++)
		{
			if ((axisFlags & 1) != 0)
			{
				struct ParticleAxis *axis = &p->axis[axisIndex];

				axis->startVal += axis->velocity;
				axis->velocity = (s16)(axis->velocity + axis->accel);

				if (((axisFlags >> 16) & 1) != 0 && osc != NULL)
				{
					Particle_ApplyOscillator(axis, osc);
					osc = osc->next;
				}
			}

			axisFlags = (axisFlags & 0xfffeffffu) >> 1;
		}

		if (p->funcPtr != NULL)
		{
			void (*funcPtr)(struct Particle *) = (void (*)(struct Particle *))p->funcPtr;
			funcPtr(p);
		}

		activeFlags = p->flagsAxis;

		if ((flagsSetColor & 1) != 0)
		{
			if (((activeFlags & 0x20) != 0 && p->axis[5].startVal < 1) || ((activeFlags & 0x40) != 0 && p->axis[6].startVal < 1))
				goto destroyParticle;
		}

		if ((flagsSetColor & 2) != 0 && Particle_ColorExpired(p, activeFlags))
			goto destroyParticle;

		link = &p->next;

		if ((activeFlags & 0x400) != 0 && p->ptrIconGroup != NULL)
			Particle_UpdateIconFrame(p, flagsSetColor);

		p = next;
		continue;

	destroyParticle:
		Particle_OnDestroy(p);
		LIST_AddFront(&sdata->gGT->JitPools.particle.free, (struct Item *)p);
		sdata->gGT->numParticles--;
		*link = next;
		p = next;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003f434-0x8003f48c.
void Particle_UpdateAllParticles(void)
{
	struct GameTracker *gGT = sdata->gGT;

	if ((gGT->gameMode1 & DEBUG_MENU) != 0)
		return;

	Particle_UpdateList((struct Particle **)&gGT->particleList_ordinary, gGT->particleList_ordinary);
	Particle_UpdateList((struct Particle **)&gGT->particleList_heatWarp, gGT->particleList_heatWarp);
}


#define CLAMP_LOW  0
#define CLAMP_HIGH 0xff00

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003f48c-0x8003f4c4.
int Particle_BitwiseClampByte(int *value)
{
	if (*value < CLAMP_LOW)
		*value = CLAMP_LOW;
	else if (*value > CLAMP_HIGH)
		*value = CLAMP_HIGH;

	return *value >> 8;
}


#define COLOR_FLAG_R 0x80
#define COLOR_FLAG_G 0x100
#define COLOR_FLAG_B 0x200

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003f4c4-0x8003f590
u32 Particle_SetColors(u32 flagColors, u32 flagAlpha, struct Particle *p)
{
	u32 color = 0;

	if (flagColors & COLOR_FLAG_R)
	{
		color = (u32)Particle_BitwiseClampByte(&p->axis[7].startVal);

		if (flagColors & COLOR_FLAG_G)
			color |= (u32)Particle_BitwiseClampByte(&p->axis[8].startVal) << 8;
		else
			color |= color << 8;

		if (flagColors & COLOR_FLAG_B)
			color |= (u32)Particle_BitwiseClampByte(&p->axis[9].startVal) << 16;
		else
			color |= (color & 0xff) << 16;
	}
	else
	{
		color = 0x01000000;
	}

	if (flagAlpha & 0x80)
		color |= 0x2000000;

	return color;
}


static u32 Particle_RenderList_ReadWord(const void *base, int offset)
{
	return *(const u32 *)(const void *)((const char *)base + offset);
}

static u8 Particle_RenderList_ReadByte(const void *base, int offset)
{
	return *(const u8 *)(const void *)((const char *)base + offset);
}

static s32 Particle_RenderList_MulLo(s32 left, s32 right)
{
	return (s32)(u32)((s64)left * (s64)right);
}

static s32 Particle_RenderList_MulShift(s32 left, s32 right, int shift)
{
	return Particle_RenderList_MulLo(left, right) >> shift;
}

static u32 Particle_RenderList_PackXY(s32 x, s32 y)
{
	return ((u32)x & 0xffff) | ((u32)y << 16);
}

static int Particle_RenderList_IsNearCamera(s32 value)
{
	if (value < 0)
		value = -value;

	return value < 30001;
}

static struct InstDrawPerPlayer *Particle_RenderList_GetIdpp(struct Instance *inst, int cameraID)
{
	return (struct InstDrawPerPlayer *)((char *)inst + sizeof(struct Instance) + (cameraID * sizeof(struct InstDrawPerPlayer)));
}

struct ParticleRenderListTrig
{
	s32 sin;
	s32 cos;
};

struct ParticleRenderListMatrix
{
	u32 r11r12;
	u32 r13r21;
	u32 r22r23;
	u32 r31r32;
	u32 r33;
};

struct ParticleRenderListScratch
{
	union
	{
		u32 viewProjWords[5];
		struct
		{
			u32 viewProjR11R12;
			u32 viewProjR13R21;
			u32 viewProjR22R23;
			u32 viewProjR31R32;
			u16 viewProjR33Low;
		};
	};
	u8 pad_14[0x0c];
	uint32_t *ot;
	s32 cameraOffset[3];
	s32 depth;
};

CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleRenderListScratch, viewProjWords) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleRenderListScratch, viewProjR33Low) == 0x10);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleRenderListScratch, ot) == 0x20);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleRenderListScratch, cameraOffset) == 0x24);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleRenderListScratch, depth) == 0x30);

struct ParticleSpecialLineBody
{
	u32 color0AndCode;
	u32 xy0;
	u32 color1;
	u32 xy1;
};

struct ParticleSpecialPacket
{
	u32 tag;
	u32 drawMode;
	u32 pad;
	struct ParticleSpecialLineBody line;
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct ParticleSpecialLineBody) == 0x10);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleSpecialLineBody, color0AndCode) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleSpecialLineBody, xy0) == 0x04);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleSpecialLineBody, color1) == 0x08);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleSpecialLineBody, xy1) == 0x0C);

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct ParticleSpecialPacket) == 0x1C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleSpecialPacket, tag) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleSpecialPacket, drawMode) == 0x04);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleSpecialPacket, pad) == 0x08);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ParticleSpecialPacket, line) == 0x0C);

static struct ParticleRenderListTrig Particle_RenderList_ReadTrig(s32 angle)
{
	struct TrigTable trigApprox = data.trigApprox[angle & 0x3ff];
	struct ParticleRenderListTrig trig;

	if ((angle & 0x400) == 0)
	{
		trig.sin = trigApprox.sin;
		trig.cos = trigApprox.cos;

		if ((angle & 0x800) != 0)
		{
			trig.sin = -trig.sin;
			trig.cos = -trig.cos;
		}
	}
	else
	{
		trig.sin = trigApprox.cos;

		if ((angle & 0x800) == 0)
		{
			trig.cos = -trigApprox.sin;
		}
		else
		{
			trig.sin = -trig.sin;
			trig.cos = trigApprox.sin;
		}
	}

	return trig;
}

static void Particle_RenderList_LinkPrimitive(u32 *tagWord, const void *packet, uint32_t *ot, u32 tag)
{
	CtrGpu_LinkPacket24(ot, tagWord, packet, tag);
}

static void Particle_RenderList_LinkAndAdvance(u32 **primCursor, u32 **payloadCursor, struct Particle *particle, struct InstDrawPerPlayer *idpp,
                                               u16 flagsSetColor, s32 depth, uint32_t *defaultOT)
{
	u32 *prim = *primCursor;
	uint32_t *otBase;
	s32 otIndex;

	if (idpp != NULL)
	{
		otIndex = depth >> 5;

		if (otIndex < (u16)idpp->depthOffset[0])
			otIndex = (u16)idpp->depthOffset[0];

		if ((u16)idpp->depthOffset[1] < otIndex)
			otIndex = (u16)idpp->depthOffset[1];

		otBase = (uint32_t *)(uintptr_t)idpp->otRangeNormal;
	}
	else
	{
		otIndex = (depth >> 8) + (s8)particle->otIndexOffset;

		if (otIndex < 0)
			otIndex = 0;

		if (otIndex >= 0x400)
			otIndex = 0x3ff;

		otBase = defaultOT;
	}

	if ((flagsSetColor & 0x1000) != 0)
	{
		struct ParticleSpecialPacket *packet = (struct ParticleSpecialPacket *)prim;

		Particle_RenderList_LinkPrimitive(&packet->tag, packet, &otBase[otIndex], 0x06000000);
		*primCursor = (u32 *)(packet + 1);
		*payloadCursor += 7;
	}
	else
	{
		POLY_FT4 *poly = (POLY_FT4 *)prim;

		Particle_RenderList_LinkPrimitive(&poly->tag, poly, &otBase[otIndex], 0x09000000);
		*primCursor = (u32 *)(poly + 1);
		*payloadCursor += 10;
	}
}

static void Particle_RenderList_WriteSpecialPrimitive(struct ParticleSpecialPacket *packet, struct Particle *particle, u16 flagsAxis, u16 flagsSetColor,
                                                      u32 color, struct ParticleRenderListScratch *scratch)
{
	CTC2(scratch->viewProjWords[0], 0);
	CTC2(scratch->viewProjWords[1], 1);
	CTC2(scratch->viewProjWords[2], 2);
	CTC2(scratch->viewProjWords[3], 3);
	CTC2(scratch->viewProjWords[4], 4);

	MTC2(0, 0);
	MTC2(0, 1);

	if ((flagsAxis & 0x20) != 0)
	{
		s32 scale = particle->axis[5].startVal;
		s32 deltaX = Particle_RenderList_MulLo((particle->axis[3].startVal - particle->axis[0].startVal) >> 6, scale);
		s32 deltaY = Particle_RenderList_MulLo((particle->axis[6].startVal - particle->axis[1].startVal) >> 6, scale);
		s32 deltaZ = Particle_RenderList_MulLo((particle->axis[4].startVal - particle->axis[2].startVal) >> 6, scale);

		MTC2(((u32)deltaX >> 16) | ((u32)(deltaY >> 16) << 16), 2);
		MTC2((u32)(deltaZ >> 16), 3);
	}
	else
	{
		s32 deltaX = (particle->axis[3].startVal - particle->axis[0].startVal) >> 6;
		s32 deltaY = (particle->axis[6].startVal - particle->axis[1].startVal) >> 6;
		s32 deltaZ = (particle->axis[4].startVal - particle->axis[2].startVal) >> 6;

		MTC2(Particle_RenderList_PackXY(deltaX, deltaY), 2);
		MTC2((u32)deltaZ, 3);
	}

	gte_rtpt_b();

	color |= 0x50000000;

	if ((flagsSetColor & 0x2000) != 0)
	{
		packet->line.color1 = color;
		packet->line.color0AndCode = particle->axis[10].startVal;
	}
	else
	{
		packet->line.color0AndCode = color;
		packet->line.color1 = particle->axis[10].startVal;
	}

	*(u32 *)(void *)&particle->axis[10].velocity = color;
	packet->drawMode = 0xe1000a00 | (flagsSetColor & 0x60);
	packet->pad = 0;
	packet->line.xy0 = MFC2(12);
	packet->line.xy1 = MFC2(13);
	scratch->depth = (s32)MFC2(17);
}

static struct ParticleRenderListMatrix Particle_RenderList_BuildNormalMatrix(struct Particle *particle, u16 flagsAxis)
{
	struct ParticleRenderListMatrix matrix;

	matrix.r11r12 = 0x2000;
	matrix.r13r21 = 0;
	matrix.r22r23 = 0x1000;
	matrix.r31r32 = 0;
	matrix.r33 = 0x1000;

	if ((flagsAxis & 0x8) == 0)
	{
		if ((flagsAxis & 0x10) == 0)
		{
			if ((flagsAxis & 0x20) != 0)
			{
				matrix.r11r12 = (u32)particle->axis[5].startVal << 1;
				matrix.r22r23 = (s32)matrix.r11r12 >> 1;
			}

			if ((flagsAxis & 0x40) != 0)
				matrix.r22r23 = particle->axis[6].startVal;

			return matrix;
		}

		struct ParticleRenderListTrig rotY = Particle_RenderList_ReadTrig(particle->axis[4].startVal);

		if ((flagsAxis & 0x20) == 0)
		{
			matrix.r11r12 = (((u32)rotY.cos & 0x7fff) << 1) | ((u32)rotY.sin << 17);

			if ((flagsAxis & 0x40) != 0)
			{
				s32 scaleY = particle->axis[6].startVal;

				matrix.r13r21 = (u32)(Particle_RenderList_MulLo(-rotY.sin, scaleY) >> 12) << 16;
				matrix.r22r23 = (u32)Particle_RenderList_MulShift(rotY.cos, scaleY, 12) & 0xffff;
			}
			else
			{
				matrix.r13r21 = (u32)-rotY.sin << 16;
				matrix.r22r23 = (u32)rotY.cos & 0xffff;
			}
		}
		else
		{
			s32 scaleX = particle->axis[5].startVal;

			if ((flagsAxis & 0x40) != 0)
			{
				s32 scaleY = particle->axis[6].startVal;

				matrix.r11r12 =
				    ((u32)Particle_RenderList_MulShift(rotY.cos, scaleX, 11) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotY.sin, scaleX, 11) << 16);
				matrix.r13r21 = (u32)Particle_RenderList_MulShift(-rotY.sin, scaleY, 12) << 16;
				matrix.r22r23 = (u32)Particle_RenderList_MulShift(rotY.cos, scaleY, 12) & 0xffff;
			}
			else
			{
				s32 scaledCos = Particle_RenderList_MulShift(rotY.cos, scaleX, 12);
				s32 scaledSin = Particle_RenderList_MulShift(rotY.sin, scaleX, 12);

				matrix.r11r12 = (((u32)scaledCos & 0x7fff) << 1) | ((u32)scaledSin << 17);
				matrix.r13r21 = (u32)-scaledSin << 16;
				matrix.r22r23 = (u32)scaledCos & 0xffff;
			}
		}

		return matrix;
	}

	if ((flagsAxis & 0x10) == 0)
	{
		struct ParticleRenderListTrig rotX = Particle_RenderList_ReadTrig(particle->axis[3].startVal);

		if ((flagsAxis & 0x20) != 0)
		{
			s32 scaleX = particle->axis[5].startVal;

			matrix.r11r12 = ((u32)scaleX << 1) & 0xffff;

			if ((flagsAxis & 0x40) != 0)
			{
				s32 scaleY = particle->axis[6].startVal;

				matrix.r31r32 = (u32)-rotX.sin << 16;
				matrix.r33 = (u32)rotX.cos & 0xffff;
				matrix.r22r23 =
				    ((u32)Particle_RenderList_MulShift(rotX.cos, scaleY, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotX.sin, scaleY, 12) << 16);
			}
			else
			{
				s32 scaledSin = Particle_RenderList_MulShift(rotX.sin, scaleX, 12);
				s32 scaledCos = Particle_RenderList_MulShift(rotX.cos, scaleX, 12);

				matrix.r33 = (u32)scaledCos & 0xffff;
				matrix.r22r23 = matrix.r33 | ((u32)scaledSin << 16);
				matrix.r31r32 = (u32)-scaledSin << 16;
			}
		}
		else
		{
			matrix.r33 = (u32)rotX.cos & 0xffff;

			if ((flagsAxis & 0x40) != 0)
			{
				s32 scaleY = particle->axis[6].startVal;

				matrix.r31r32 = (u32)-rotX.sin << 16;
				matrix.r22r23 =
				    ((u32)Particle_RenderList_MulShift(rotX.cos, scaleY, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotX.sin, scaleY, 12) << 16);
			}
			else
			{
				matrix.r22r23 = ((u32)rotX.cos & 0xffff) | ((u32)rotX.sin << 16);
				matrix.r31r32 = (u32)-rotX.sin << 16;
			}
		}

		return matrix;
	}

	struct ParticleRenderListTrig rotX = Particle_RenderList_ReadTrig(particle->axis[3].startVal);
	struct ParticleRenderListTrig rotY = Particle_RenderList_ReadTrig(particle->axis[4].startVal);

	if ((flagsAxis & 0x20) != 0)
	{
		s32 scaleX = particle->axis[5].startVal;

		if ((flagsAxis & 0x40) != 0)
		{
			s32 scaleY = particle->axis[6].startVal;
			s32 r13Base = Particle_RenderList_MulShift(rotX.cos, -rotY.sin, 12);
			s32 r22Base = Particle_RenderList_MulShift(rotY.cos, rotX.cos, 12);

			matrix.r33 = (u32)rotX.cos & 0xffff;
			matrix.r11r12 =
			    ((u32)Particle_RenderList_MulShift(rotY.cos, scaleX, 11) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotY.sin, scaleX, 11) << 16);
			matrix.r31r32 =
			    ((u32)Particle_RenderList_MulShift(rotX.sin, rotY.sin, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(-rotX.sin, rotY.cos, 12) << 16);
			matrix.r13r21 = (u32)Particle_RenderList_MulShift(r13Base, scaleY, 12) << 16;
			matrix.r22r23 = ((u32)Particle_RenderList_MulShift(r22Base, scaleY, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotX.sin, scaleY, 12) << 16);
		}
		else
		{
			s32 scaledCosX = Particle_RenderList_MulShift(rotX.cos, scaleX, 12);
			s32 scaledSinX = Particle_RenderList_MulShift(rotX.sin, scaleX, 12);
			s32 scaledCosY = Particle_RenderList_MulShift(rotY.cos, scaleX, 12);
			s32 scaledSinY = Particle_RenderList_MulShift(rotY.sin, scaleX, 12);

			matrix.r33 = (u32)scaledCosX & 0xffff;
			matrix.r11r12 = (((u32)scaledCosY << 1) & 0xffff) | ((u32)scaledSinY << 17);
			matrix.r13r21 = (u32)Particle_RenderList_MulShift(scaledCosX, -scaledSinY, 12) << 16;
			matrix.r22r23 = ((u32)Particle_RenderList_MulShift(scaledCosY, scaledCosX, 12) & 0xffff) | ((u32)scaledSinX << 16);
			matrix.r31r32 = ((u32)Particle_RenderList_MulShift(scaledSinX, scaledSinY, 12) & 0xffff) |
			                ((u32)Particle_RenderList_MulShift(-scaledSinX, scaledCosY, 12) << 16);
		}
	}
	else
	{
		if ((flagsAxis & 0x40) != 0)
		{
			s32 scaleY = particle->axis[6].startVal;
			s32 r13Base = Particle_RenderList_MulShift(rotX.cos, -rotY.sin, 12);
			s32 r22Base = Particle_RenderList_MulShift(rotY.cos, rotX.cos, 12);

			matrix.r33 = (u32)rotX.cos & 0xffff;
			matrix.r11r12 = (((u32)rotY.cos << 1) & 0xffff) | ((u32)rotY.sin << 17);
			matrix.r31r32 =
			    ((u32)Particle_RenderList_MulShift(rotX.sin, rotY.sin, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(-rotX.sin, rotY.cos, 12) << 16);
			matrix.r13r21 = (u32)Particle_RenderList_MulShift(r13Base, scaleY, 12) << 16;
			matrix.r22r23 = ((u32)Particle_RenderList_MulShift(r22Base, scaleY, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotX.sin, scaleY, 12) << 16);
		}
		else
		{
			matrix.r33 = (u32)rotX.cos & 0xffff;
			matrix.r11r12 = (((u32)rotY.cos << 1) & 0xffff) | ((u32)rotY.sin << 17);
			matrix.r13r21 = (u32)Particle_RenderList_MulShift(rotX.cos, -rotY.sin, 12) << 16;
			matrix.r22r23 = ((u32)Particle_RenderList_MulShift(rotY.cos, rotX.cos, 12) & 0xffff) | ((u32)rotX.sin << 16);
			matrix.r31r32 =
			    ((u32)Particle_RenderList_MulShift(rotX.sin, rotY.sin, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(-rotX.sin, rotY.cos, 12) << 16);
		}
	}

	return matrix;
}

static void Particle_RenderList_WriteNormalPrimitive(POLY_FT4 *poly, struct Icon *icon, u16 flagsAxis, u16 flagsSetColor, u32 color,
                                                     struct ParticleRenderListMatrix *matrix, s32 *scratchDepth)
{
	s32 width;
	s32 height;
	s32 halfWidth;
	s32 halfHeight;
	u32 input;

	CTC2(matrix->r11r12, 0);
	CTC2(matrix->r13r21, 1);
	CTC2(matrix->r22r23, 2);
	CTC2(matrix->r31r32, 3);
	CTC2(matrix->r33, 4);

	CtrGpu_WriteColorCode(&poly->r0, color | 0x2c000000);
	CtrGpu_WritePackedUVWord(&poly->u0, Particle_RenderList_ReadWord(icon, 0x14));
	CtrGpu_WritePackedUVWord(&poly->u1, (Particle_RenderList_ReadWord(icon, 0x18) & 0xff9fffff) | ((u32)(flagsSetColor & 0x60) << 16));
	CtrGpu_WritePackedUV(&poly->u2, *(u16 *)(void *)((char *)icon + 0x1c));
	CtrGpu_WritePackedUV(&poly->u3, *(u16 *)(void *)((char *)icon + 0x1e));

	width = (Particle_RenderList_ReadByte(icon, 0x18) - Particle_RenderList_ReadByte(icon, 0x14)) + 1;
	height = (Particle_RenderList_ReadByte(icon, 0x1d) - Particle_RenderList_ReadByte(icon, 0x15)) + 1;

	halfWidth = width << 1;
	halfHeight = height << 1;

	if ((flagsSetColor & 0x400) != 0)
	{
		halfWidth = width << 4;
		halfHeight = height << 4;
	}

	input = Particle_RenderList_PackXY(-halfWidth, -halfHeight);
	MTC2(0, 1);

	MTC2(input, 0);
	gte_rtps_b();
	CtrGpu_WritePackedXY(&poly->x0, MFC2(14));
	*scratchDepth = (s32)MFC2(19);

	input = Particle_RenderList_PackXY(halfWidth, -halfHeight);
	MTC2(input, 0);
	gte_rtps_b();
	CtrGpu_WritePackedXY(&poly->x1, MFC2(14));

	input = Particle_RenderList_PackXY(-halfWidth, halfHeight);
	MTC2(input, 0);
	gte_rtps_b();
	CtrGpu_WritePackedXY(&poly->x2, MFC2(14));

	input = Particle_RenderList_PackXY(halfWidth, halfHeight);
	MTC2(input, 0);
	gte_rtps_b();
	CtrGpu_WritePackedXY(&poly->x3, MFC2(14));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003f590-0x80040308
void Particle_RenderList(struct PushBuffer *pb, void *particleList)
{
	struct GameTracker *gGT = sdata->gGT;
	struct PrimMem *primMem = &gGT->backBuffer->primMem;
	struct Particle *particle = particleList;
	struct ParticleRenderListScratch *scratch = CTR_SCRATCHPAD_PTR(struct ParticleRenderListScratch, 0x00);
	u32 *prim = (u32 *)primMem->cursor;
	u32 *primPayload = prim + 8;
	char cameraID;

	PushBuffer_SetPsyqGeom(pb);

	scratch->viewProjWords[0] = Particle_RenderList_ReadWord(&pb->matrix_ViewProj, 0x00);
	scratch->viewProjWords[1] = Particle_RenderList_ReadWord(&pb->matrix_ViewProj, 0x04);
	scratch->viewProjWords[2] = Particle_RenderList_ReadWord(&pb->matrix_ViewProj, 0x08);
	scratch->viewProjWords[3] = Particle_RenderList_ReadWord(&pb->matrix_ViewProj, 0x0c);
	scratch->viewProjR33Low = *(u16 *)(void *)((char *)&pb->matrix_ViewProj + 0x10);

	CTC2(scratch->viewProjWords[0], 8);
	CTC2(scratch->viewProjWords[1], 9);
	CTC2(scratch->viewProjWords[2], 10);
	CTC2(scratch->viewProjWords[3], 11);
	CTC2(scratch->viewProjWords[4], 12);

	scratch->ot = pb->ptrOT;
	cameraID = (char)pb->cameraID;
	scratch->cameraOffset[0] = (s32)Particle_RenderList_ReadWord(pb, 0x7c) << 2;
	scratch->cameraOffset[1] = (s32)Particle_RenderList_ReadWord(pb, 0x80) << 2;
	scratch->cameraOffset[2] = (s32)Particle_RenderList_ReadWord(pb, 0x84) << 2;

	if (prim + (gGT->numParticles * 10) >= (u32 *)primMem->guardEnd)
		return;

	if (particle != NULL)
	{
		u32 *primCursor = prim;
		u32 *payloadCursor = primPayload;

		do
		{
			struct IconGroup *iconGroup;
			struct Icon *icon;
			struct InstDrawPerPlayer *idpp;
			u16 flagsAxis;
			u16 flagsSetColor;
			s8 driverID;
			s32 posX;
			s32 posY;
			s32 posZ;
			s32 depth;
			u32 color;

			prim = primCursor;
			driverID = (s8)particle->driverID;

			if (driverID != -1 && driverID != cameraID)
				goto next_particle;

			iconGroup = particle->ptrIconGroup;
			if (iconGroup == NULL)
				goto next_particle;

			flagsAxis = particle->flagsAxis;
			if ((flagsAxis & 0x400) == 0)
			{
				icon = particle->ptrIconArray;
			}
			else
			{
				int frame = particle->axis[10].startVal >> 8;

				if (frame < 0)
					frame = 0;

				if (iconGroup->numIcons <= frame)
					frame = iconGroup->numIcons - 1;

				if (frame < 0)
					goto next_particle;

				icon = ((struct Icon **)ICONGROUP_GETICONS(iconGroup))[frame];
				particle->ptrIconArray = icon;
			}

			if (icon == NULL)
				goto next_particle;

			idpp = NULL;
			posX = particle->axis[0].startVal >> 6;
			posY = particle->axis[1].startVal >> 6;
			posZ = particle->axis[2].startVal >> 6;
			flagsSetColor = particle->flagsSetColor;

			if ((flagsSetColor & 0x800) != 0 && particle->driverInst != NULL)
			{
				struct Instance *inst = particle->driverInst;
				u32 idppFlags;

				idpp = Particle_RenderList_GetIdpp(inst, cameraID);
				idppFlags = idpp->instFlags;

				if ((idppFlags & 0x40) == 0)
					goto next_particle;

				posX += inst->matrix.t[0] << 2;
				if ((flagsSetColor & 0x8000) == 0)
					posY += inst->matrix.t[1] << 2;
				posZ += inst->matrix.t[2] << 2;

				if ((idppFlags & 0x100) != 0)
					idpp = NULL;
			}

			posX -= scratch->cameraOffset[0];
			posY -= scratch->cameraOffset[1];
			posZ -= scratch->cameraOffset[2];

			if (!Particle_RenderList_IsNearCamera(posX))
				goto next_particle;
			if (!Particle_RenderList_IsNearCamera(posY))
				goto next_particle;
			if (!Particle_RenderList_IsNearCamera(posZ))
				goto next_particle;

			MTC2((u32)(u16)posX | ((u32)posY << 16), 0);
			MTC2((u32)posZ, 1);
			gte_llv0_b();

			CTC2(MFC2(25), 5);
			CTC2(MFC2(26), 6);
			CTC2(MFC2(27), 7);

			depth = (s32)MFC2(27);
			if (depth < 0)
				goto next_particle;

			if ((particle->unk1A << 2) < depth)
				goto next_particle;

			color = Particle_SetColors(flagsAxis, flagsSetColor, particle);

			if ((flagsSetColor & 0x1000) != 0)
			{
				Particle_RenderList_WriteSpecialPrimitive((struct ParticleSpecialPacket *)prim, particle, flagsAxis, flagsSetColor, color, scratch);
				Particle_RenderList_LinkAndAdvance(&primCursor, &payloadCursor, particle, idpp, flagsSetColor, scratch->depth, scratch->ot);
				prim = primCursor;
				goto next_particle;
			}

			struct ParticleRenderListMatrix matrix = Particle_RenderList_BuildNormalMatrix(particle, flagsAxis);

			Particle_RenderList_WriteNormalPrimitive((POLY_FT4 *)prim, icon, flagsAxis, flagsSetColor, color, &matrix, &scratch->depth);
			Particle_RenderList_LinkAndAdvance(&primCursor, &payloadCursor, particle, idpp, flagsSetColor, scratch->depth, scratch->ot);
			prim = primCursor;

		next_particle:
			particle = particle->next;
		} while (particle != NULL);
	}

	primMem->cursor = prim;
}


static u32 Particle_Init_GetAxisFlags(const struct Particle *p)
{
	return *(const u32 *)&p->flagsAxis;
}

static void Particle_Init_SetAxisFlags(struct Particle *p, u32 flags)
{
	*(u32 *)&p->flagsAxis = flags;
}

static u8 ParticleEmitter_GetInitOffset(const struct ParticleEmitter *emSet)
{
	return ((const u8 *)emSet)[2];
}

static void ParticleEmitter_CopyOscillator(struct ParticleOscillator *osc, const struct ParticleEmitter *emSet)
{
	const u32 *src = (const u32 *)emSet->data;
	u32 *dst = (u32 *)&osc->flags;

	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}

static void Particle_InitAxis(struct Particle *p, const struct ParticleEmitter *emSet, u8 axisIndex, u32 *flagsAxis)
{
	struct ParticleAxis *axis = &p->axis[axisIndex];
	u16 flags = emSet->flags;
	int value = 0;
	s16 velocity = 0;
	s16 accel = 0;

	if ((flags & 1) != 0)
		value = emSet->InitTypes.AxisInit.baseValue.startVal;
	if ((flags & 8) != 0)
		value += MixRNG_Particles(emSet->InitTypes.AxisInit.rngSeed.startVal);

	axis->startVal = value;

	if ((flags & 2) != 0)
		velocity = emSet->InitTypes.AxisInit.baseValue.velocity;
	if ((flags & 0x10) != 0)
		velocity = (s16)(velocity + MixRNG_Particles(emSet->InitTypes.AxisInit.rngSeed.velocity));

	axis->velocity = velocity;

	if ((flags & 4) != 0)
		accel = emSet->InitTypes.AxisInit.baseValue.accel;
	if ((flags & 0x20) != 0)
		accel = (s16)(accel + MixRNG_Particles(emSet->InitTypes.AxisInit.rngSeed.accel));

	axis->accel = accel;

	*flagsAxis |= 1u << (axisIndex & 0x1f);
}

static void Particle_InitOscillator(struct Particle *p, struct ParticleOscillator *localOsc[12], const struct ParticleEmitter *emSet, u8 axisIndex,
                                    u32 *flagsAxis)
{
	struct ParticleOscillator *osc;
	u32 oscBit = 1u << ((axisIndex + 0x10) & 0x1f);
	u32 axisBit = 1u << (axisIndex & 0x1f);

	if ((*flagsAxis & oscBit) == 0)
	{
		osc = (struct ParticleOscillator *)LIST_RemoveFront(&sdata->gGT->JitPools.oscillator.free);
		if (osc == NULL)
			return;

		localOsc[axisIndex] = osc;
	}
	else
	{
		osc = localOsc[axisIndex];
	}

	ParticleEmitter_CopyOscillator(osc, emSet);

	if ((osc->flags & 0x20) != 0)
		osc->phase = (s16)(osc->phase - (u16)sdata->gGT->frameTimer_Confetti);

	if ((osc->flags & 7) == 6)
		osc->previousValue = osc->phase;

	*flagsAxis |= oscBit;

	if ((*flagsAxis & axisBit) == 0)
	{
		struct ParticleAxis *axis = &p->axis[axisIndex];

		*flagsAxis |= axisBit;
		axis->startVal = 0;
		axis->velocity = 0;
		axis->accel = 0;
	}
}

static void Particle_RandomizeOscillator(struct ParticleOscillator *localOsc[12], const struct ParticleEmitter *emSet, u8 axisIndex, u32 flagsAxis)
{
	struct ParticleOscillator *osc;
	const s16 *rng;

	if ((flagsAxis & (1u << ((axisIndex + 0x10) & 0x1f))) == 0)
		return;

	osc = localOsc[axisIndex];
	rng = (const s16 *)&((const u8 *)emSet)[0x18];

	if ((u16)rng[0] != 0)
		osc->period = (u16)(osc->period + MixRNG_Particles((u16)rng[0]));
	if (rng[1] != 0)
		osc->phase = (s16)(osc->phase + MixRNG_Particles(rng[1]));
	if ((u16)rng[2] != 0)
		osc->scale = (u16)(osc->scale + MixRNG_Particles((u16)rng[2]));
	if (rng[3] != 0)
		osc->offset = (s16)(osc->offset + MixRNG_Particles(rng[3]));
	if (rng[4] != 0)
		osc->min = (s16)(osc->min + MixRNG_Particles(rng[4]));
	if (rng[5] != 0)
		osc->max = (s16)(osc->max + MixRNG_Particles(rng[5]));
}

static void Particle_LinkOscillators(struct Particle *p, struct ParticleOscillator *localOsc[12], u32 flagsAxis)
{
	struct ParticleOscillator **link = &p->oscillator;
	u32 oscFlags = (s32)flagsAxis >> 16;
	int axisIndex = 0;

	while (oscFlags != 0)
	{
		if ((oscFlags & 1) != 0)
		{
			*link = localOsc[axisIndex];
			link = &localOsc[axisIndex]->next;
		}

		oscFlags = (s32)oscFlags >> 1;
		axisIndex++;
	}

	*link = NULL;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80040308-0x80040850
struct Particle *Particle_Init(u32 param_1, struct IconGroup *ig, struct ParticleEmitter *emSet)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Particle *p;
	struct ParticleOscillator *localOsc[12];
	u32 flagsAxis = 0;
	int particleType = 0;

	(void)param_1;

	p = (struct Particle *)LIST_RemoveFront(&gGT->JitPools.particle.free);
	if (p == NULL)
		return NULL;

	gGT->numParticles++;

	p->ptrIconGroup = ig;
	if (ig != NULL && ig->numIcons != 0 && ig->numIcons > 0)
		p->ptrIconArray = ((struct Icon **)ICONGROUP_GETICONS(ig))[0];
	else
	{
		p->ptrIconGroup = NULL;
		p->ptrIconArray = NULL;
	}

	if (emSet != NULL)
	{
		while (emSet->flags != 0)
		{
			u16 flags = emSet->flags;
			u8 axisIndex = ParticleEmitter_GetInitOffset(emSet);

			if (axisIndex == 0xc)
			{
				if ((flags & 0xc0) == 0)
				{
					p->funcPtr = emSet->InitTypes.FuncInit.particle_funcPtr;
					p->flagsSetColor = emSet->InitTypes.FuncInit.particle_colorFlags;
					p->framesLeftInLife = emSet->InitTypes.FuncInit.particle_lifespan;
					flagsAxis |= 0x1000;
					particleType = emSet->InitTypes.FuncInit.particle_Type;
				}
			}
			else if ((flags & 0x80) != 0)
			{
				Particle_RandomizeOscillator(localOsc, emSet, axisIndex, flagsAxis);
			}
			else if ((flags & 0x40) != 0)
			{
				Particle_InitOscillator(p, localOsc, emSet, axisIndex, &flagsAxis);
			}
			else
			{
				Particle_InitAxis(p, emSet, axisIndex, &flagsAxis);
			}

			emSet++;
		}
	}

	Particle_LinkOscillators(p, localOsc, flagsAxis);

	if ((flagsAxis & 0x1000) == 0)
	{
		p->funcPtr = NULL;
		p->flagsSetColor = 0;
		p->framesLeftInLife = 0;
		p->ptrIconArray = NULL;
		p->ptrIconGroup = NULL;
	}

	Particle_Init_SetAxisFlags(p, flagsAxis & 0xffffefff);

	if (particleType == 0)
	{
		p->next = gGT->particleList_ordinary;
		gGT->particleList_ordinary = p;
	}
	else
	{
		p->next = gGT->particleList_heatWarp;
		gGT->particleList_heatWarp = p;
	}

	p->unk1A = 0x400;
	p->driverID = -1;
	p->otIndexOffset = 0;

	if ((p->flagsSetColor & 0x1000) != 0)
	{
		if ((p->flagsSetColor & 0x4000) == 0)
		{
			u32 color = Particle_SetColors(Particle_Init_GetAxisFlags(p), p->flagsSetColor, p) | 0x50000000;

			p->axis[10].startVal = color;
			*(u32 *)&p->axis[10].velocity = color;
		}
		else
		{
			u32 color = 0x50000000;

			if ((p->flagsSetColor & 0x80) != 0)
				color = 0x52000000;

			p->axis[10].startVal = color;
		}
	}

	return p;
}
