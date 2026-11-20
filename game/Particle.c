#include <common.h>

enum
{
	PARTICLE_POTION_SHATTER_Y_SPEED_THRESHOLD = 0x578,
	PARTICLE_POTION_SHATTER_XZ_RANDOM_RANGE = 800,
	PARTICLE_POTION_SHATTER_XZ_RANDOM_CENTER = 400,
	PARTICLE_POTION_SHATTER_SCALE_RANDOM_RANGE = 0x100,
	PARTICLE_POTION_SHATTER_SCALE_RANDOM_BASE = 0x100,
	PARTICLE_POTION_SHATTER_FADE_STEP = 0x1200,

	PARTICLE_SPIT_TIRE_MOUTH_Y_OFFSET = 0x10,
	PARTICLE_SPIT_TIRE_XZ_RANDOM_RANGE = 0x1640,
	PARTICLE_SPIT_TIRE_XZ_RANDOM_CENTER = 0xb20,
	PARTICLE_SPIT_TIRE_FRAME_1 = 0x1000,
	PARTICLE_SPIT_TIRE_FRAME_2 = 0xfff,
	PARTICLE_SPIT_TIRE_FRAME_3 = 0xffe,
	PARTICLE_SPIT_TIRE_FRAME_3_VELOCITY = 0xf801,
	PARTICLE_SPIT_TIRE_FRAME_1_Y_RANDOM_RANGE = 0x12c0,
	PARTICLE_SPIT_TIRE_FRAME_1_Y_BASE = 0x1900,
	PARTICLE_SPIT_TIRE_LATER_Y_RANDOM_RANGE = 800,
	PARTICLE_SPIT_TIRE_FRAME_2_Y_BASE = 8000,
	PARTICLE_SPIT_TIRE_FRAME_3_Y_BASE = 6000,

	PARTICLE_EXHAUST_WATER_HEIGHT_THRESHOLD = 3,
	PARTICLE_EXHAUST_POP_LIFE_THRESHOLD = 27,
	PARTICLE_EXHAUST_BUBBLEPOP_ICON_GROUP = 8,
	PARTICLE_EXHAUST_ROTATION_RANDOM_MASK = 0xfff,

	PARTICLE_OSC_WAVE_CENTER = 0x1000,
	PARTICLE_OSC_SAW_PHASE_SHIFT = 4,
	PARTICLE_OSC_SAW_PHASE_MASK = 0x1fff,
	PARTICLE_OSC_TRIANGLE_PHASE_SHIFT = 3,
	PARTICLE_OSC_TRIANGLE_PHASE_MASK = 0x3fff,
	PARTICLE_OSC_TRIANGLE_PEAK = 0x2000,
	PARTICLE_OSC_TRIANGLE_PERIOD = 0x4000,
	PARTICLE_OSC_SQUARE_PHASE_SHIFT = 6,
	PARTICLE_OSC_SQUARE_HIGH_BIT = 0x400,
	PARTICLE_OSC_RANDOM_SHIFT = 3,
	PARTICLE_OSC_SINE_PHASE_SHIFT = 5,
	PARTICLE_OSC_ABS_SINE_PHASE_SHIFT = 6,
	PARTICLE_OSC_SCALE_SHIFT = 12,

	PARTICLE_COLOR_CHANNEL_MIN = 0,
	PARTICLE_COLOR_CHANNEL_MAX = 0xff00,
	PARTICLE_COLOR_CHANNEL_SHIFT = 8,
	PARTICLE_COLOR_BYTE_MASK = 0xff,
};

enum
{
	PARTICLE_GPU_CODE_SHADE_TEXTURE = 0x01000000u,
	PARTICLE_GPU_CODE_SEMI_TRANS = 0x02000000u,
	PARTICLE_GPU_CODE_POLY_FT4 = 0x2c000000u,
	PARTICLE_GPU_CODE_LINE_G2 = 0x50000000u,
	PARTICLE_GPU_TAG_LENGTH_SPECIAL_LINE = 0x06000000u,
	PARTICLE_GPU_TAG_LENGTH_POLY_FT4 = 0x09000000u,
	PARTICLE_GPU_DRAWMODE_BASE = 0xe1000a00u,
	PARTICLE_TEXTURE_DRAW_MODE_MASK = 0xff9fffffu,
};


void Particle_FuncPtr_PotionShatter(struct Particle *p)
{
	s16 scaleRandomQuotient;
	int rng;

	if (p->axis[PARTICLE_AXIS_POS_Y].velocity < PARTICLE_POTION_SHATTER_Y_SPEED_THRESHOLD)
	{
		if (p->axis[PARTICLE_AXIS_POS_X].velocity != 0)
		{
			goto FadeShatterChannel;
		}

		// random X
		rng = MixRNG_Scramble();
		p->axis[PARTICLE_AXIS_POS_X].velocity =
		    rng + (rng / PARTICLE_POTION_SHATTER_XZ_RANDOM_RANGE) * -PARTICLE_POTION_SHATTER_XZ_RANDOM_RANGE - PARTICLE_POTION_SHATTER_XZ_RANDOM_CENTER;

		// random Z
		rng = MixRNG_Scramble();
		p->axis[PARTICLE_AXIS_POS_Z].velocity =
		    rng + (rng / PARTICLE_POTION_SHATTER_XZ_RANDOM_RANGE) * -PARTICLE_POTION_SHATTER_XZ_RANDOM_RANGE - PARTICLE_POTION_SHATTER_XZ_RANDOM_CENTER;

		// random scale
		rng = MixRNG_Scramble();
		scaleRandomQuotient = (rng >> 8);
		if (rng < 0)
		{
			scaleRandomQuotient = ((rng + 0xff) >> 8);
		}
		p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].velocity =
		    rng + scaleRandomQuotient * -PARTICLE_POTION_SHATTER_SCALE_RANDOM_RANGE + PARTICLE_POTION_SHATTER_SCALE_RANDOM_BASE;
	}
	if (p->axis[PARTICLE_AXIS_POS_X].velocity == 0)
	{
		return;
	}

FadeShatterChannel:

	// green shatter or red shatter
	if (p->owner.modelID == STATIC_SHOCKWAVE_GREEN)
	{
		if (0 < p->axis[PARTICLE_AXIS_COLOR_G].startVal)
		{
			p->axis[PARTICLE_AXIS_COLOR_G].startVal -= PARTICLE_POTION_SHATTER_FADE_STEP;
		}
	}
	else
	{
		if (0 < p->axis[PARTICLE_AXIS_COLOR_R].startVal)
		{
			p->axis[PARTICLE_AXIS_COLOR_R].startVal -= PARTICLE_POTION_SHATTER_FADE_STEP;
		}
	}
}


void Particle_FuncPtr_SpitTire(struct Particle *p)
{
	int rng;
	int scaleFrame;
	int targetY;

	// Wait until tires are 0x10 units above
	// the ground, which is where the plant
	// actually "spits" tires from the mouth
	targetY = p->owner.plantInst->matrix.t[1] + PARTICLE_SPIT_TIRE_MOUTH_Y_OFFSET;

	if ((p->axis[PARTICLE_AXIS_POS_Y].startVal >> 8) >= targetY)
	{
		return;
	}

	// random X
	rng = MixRNG_Scramble();
	p->axis[PARTICLE_AXIS_POS_X].velocity =
	    rng + (rng / PARTICLE_SPIT_TIRE_XZ_RANDOM_RANGE) * -PARTICLE_SPIT_TIRE_XZ_RANDOM_RANGE - PARTICLE_SPIT_TIRE_XZ_RANDOM_CENTER;

	// random Z
	rng = MixRNG_Scramble();
	p->axis[PARTICLE_AXIS_POS_Z].velocity =
	    rng + (rng / PARTICLE_SPIT_TIRE_XZ_RANDOM_RANGE) * -PARTICLE_SPIT_TIRE_XZ_RANDOM_RANGE - PARTICLE_SPIT_TIRE_XZ_RANDOM_CENTER;

	// scale value
	scaleFrame = p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal;

	switch (scaleFrame)
	{
	// frame #1
	case PARTICLE_SPIT_TIRE_FRAME_1:
	{
		// random Y
		rng = MixRNG_Scramble();
		p->axis[PARTICLE_AXIS_POS_Y].velocity =
		    rng + (rng / PARTICLE_SPIT_TIRE_FRAME_1_Y_RANDOM_RANGE) * -PARTICLE_SPIT_TIRE_FRAME_1_Y_RANDOM_RANGE + PARTICLE_SPIT_TIRE_FRAME_1_Y_BASE;

		// frame #2
		p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal = PARTICLE_SPIT_TIRE_FRAME_2;
		break;
	}

	// frame #2
	case PARTICLE_SPIT_TIRE_FRAME_2:
	{
		// random Y
		rng = MixRNG_Scramble();
		p->axis[PARTICLE_AXIS_POS_Y].velocity =
		    rng + (rng / PARTICLE_SPIT_TIRE_LATER_Y_RANDOM_RANGE) * -PARTICLE_SPIT_TIRE_LATER_Y_RANDOM_RANGE + PARTICLE_SPIT_TIRE_FRAME_2_Y_BASE;

		// frame #3
		p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal = PARTICLE_SPIT_TIRE_FRAME_3;
		break;
	}

	// frame #3
	case PARTICLE_SPIT_TIRE_FRAME_3:
	{
		// random Y
		rng = MixRNG_Scramble();
		p->axis[PARTICLE_AXIS_POS_Y].velocity =
		    rng + (rng / PARTICLE_SPIT_TIRE_LATER_Y_RANDOM_RANGE) * -PARTICLE_SPIT_TIRE_LATER_Y_RANDOM_RANGE + PARTICLE_SPIT_TIRE_FRAME_3_Y_BASE;

		p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].velocity = PARTICLE_SPIT_TIRE_FRAME_3_VELOCITY;
		break;
	}

	default:
		return;
	}

	p->axis[PARTICLE_AXIS_POS_Y].startVal = targetY * 0x100;
}


void Particle_FuncPtr_ExhaustUnderwater(struct Particle *p)
{
	struct IconGroup *icon;

	if ((PARTICLE_EXHAUST_WATER_HEIGHT_THRESHOLD < ((p->axis[PARTICLE_AXIS_POS_Y].startVal >> 8) + p->owner.driverInst->matrix.t[1])) &&
	    (p->framesLeftInLife < PARTICLE_EXHAUST_POP_LIFE_THRESHOLD))
	{
		// bubblepop
		icon = sdata->gGT->iconGroup[PARTICLE_EXHAUST_BUBBLEPOP_ICON_GROUP];
		p->ptrIconGroup = icon;

		if (icon != NULL)
		{
			struct Icon **ptrIconArray = ICONGROUP_GETICONS(icon);

			// actually the first icon pointer in the array,
			// not the pointer to the array itself
			p->ptrIconArray = ptrIconArray[0];
		}

		p->axis[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z].startVal = MixRNG_Scramble() & PARTICLE_EXHAUST_ROTATION_RANDOM_MASK;
		p->framesLeftInLife = 0;
	}
}


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
	return CTR_ReadU32LE(&p->flagsAxis);
}

static int Particle_OscillatorValue(struct ParticleOscillator *osc)
{
	int value;
	int timer = sdata->gGT->frameTimer_Confetti;
	int phase = timer + osc->phase;
	int product = (int)osc->period * phase;

	switch (osc->flags & PARTICLE_OSC_FLAG_MODE_MASK)
	{
	case PARTICLE_OSC_MODE_SINE:
		value = MATH_Sin(product >> PARTICLE_OSC_SINE_PHASE_SHIFT);
		break;

	case PARTICLE_OSC_MODE_ABS_SINE:
		value = MATH_Sin(product >> PARTICLE_OSC_ABS_SINE_PHASE_SHIFT);
		if (value < 0)
		{
			value = -value;
		}
		value = (value << 1) - PARTICLE_OSC_WAVE_CENTER;
		break;

	case PARTICLE_OSC_MODE_SAW:
		value = ((product >> PARTICLE_OSC_SAW_PHASE_SHIFT) & PARTICLE_OSC_SAW_PHASE_MASK) - PARTICLE_OSC_WAVE_CENTER;
		break;

	case PARTICLE_OSC_MODE_TRIANGLE:
		value = (product >> PARTICLE_OSC_TRIANGLE_PHASE_SHIFT) & PARTICLE_OSC_TRIANGLE_PHASE_MASK;
		if (value > PARTICLE_OSC_TRIANGLE_PEAK)
		{
			value = PARTICLE_OSC_TRIANGLE_PERIOD - value;
		}
		value -= PARTICLE_OSC_WAVE_CENTER;
		break;

	case PARTICLE_OSC_MODE_SQUARE:
		value = -PARTICLE_OSC_WAVE_CENTER;
		if (((product >> PARTICLE_OSC_SQUARE_PHASE_SHIFT) & PARTICLE_OSC_SQUARE_HIGH_BIT) != 0)
		{
			value = PARTICLE_OSC_WAVE_CENTER;
		}
		break;

	case PARTICLE_OSC_MODE_RANDOM:
		value = (MixRNG_Scramble() >> PARTICLE_OSC_RANDOM_SHIFT) - PARTICLE_OSC_WAVE_CENTER;
		break;

	case PARTICLE_OSC_MODE_SEEDED_RANDOM:
		value = ((int)MixRNG_GetValue((s16)osc->previousValue) >> PARTICLE_OSC_RANDOM_SHIFT) - PARTICLE_OSC_WAVE_CENTER;
		break;

	case PARTICLE_OSC_MODE_TIMER:
	default:
		value = timer;
		break;
	}

	value = ((value + osc->offset) * (int)osc->scale) >> PARTICLE_OSC_SCALE_SHIFT;

	if (value > osc->max)
	{
		value = osc->max;
	}
	if (value < osc->min)
	{
		value = osc->min;
	}

	return value;
}

static void Particle_ApplyOscillator(struct ParticleAxis *axis, struct ParticleOscillator *osc)
{
	int value;

	if ((osc->flags & PARTICLE_OSC_FLAG_SKIP_PREVIOUS_SUBTRACT) == 0)
	{
		if ((osc->flags & PARTICLE_OSC_FLAG_APPLY_TO_VELOCITY) == 0)
		{
			axis->startVal -= (s16)osc->previousValue;
		}
		else
		{
			axis->velocity = (s16)(axis->velocity - osc->previousValue);
		}
	}

	value = Particle_OscillatorValue(osc);

	if ((osc->flags & PARTICLE_OSC_FLAG_APPLY_TO_VELOCITY) == 0)
	{
		axis->startVal += value;
	}
	else
	{
		axis->velocity = (s16)(axis->velocity + value);
	}

	osc->previousValue = (s16)value;
}

static int Particle_ColorExpired(struct Particle *p, u16 activeFlags)
{
	int value = 0;

	if ((activeFlags & PARTICLE_AXIS_FLAG_COLOR_R) != 0 && p->axis[PARTICLE_AXIS_COLOR_R].startVal > 0)
	{
		value = p->axis[PARTICLE_AXIS_COLOR_R].startVal;
	}

	if ((activeFlags & PARTICLE_AXIS_FLAG_COLOR_G) != 0 && p->axis[PARTICLE_AXIS_COLOR_G].startVal > 0)
	{
		value |= p->axis[PARTICLE_AXIS_COLOR_G].startVal;
	}

	if ((activeFlags & PARTICLE_AXIS_FLAG_COLOR_B) != 0 && p->axis[PARTICLE_AXIS_COLOR_B].startVal > 0)
	{
		value |= p->axis[PARTICLE_AXIS_COLOR_B].startVal;
	}

	return value < 0x800;
}

static void Particle_UpdateIconFrame(struct Particle *p, u16 flagsSetColor)
{
	struct ParticleAxis *frameAxis = &p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR];
	int frame = frameAxis->startVal;
	int frameLimit = p->ptrIconGroup->numIcons << 8;

	if (frame < 0)
	{
		if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_ICON_WRAP) != 0)
		{
			frame += frameLimit;
		}
		else if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_ICON_BOUNCE) != 0)
		{
			frame -= frameAxis->velocity * 2;
			frameAxis->accel = -frameAxis->accel;
			frameAxis->velocity = -frameAxis->velocity;
		}
		else
		{
			frame = 0;
		}
	}
	else
	{
		if (frame < frameLimit)
		{
			return;
		}

		if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_ICON_WRAP) != 0)
		{
			frame -= frameLimit;
		}
		else if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_ICON_BOUNCE) != 0)
		{
			frame -= frameAxis->velocity * 2;
			frameAxis->accel = -frameAxis->accel;
			frameAxis->velocity = -frameAxis->velocity;
		}
		else
		{
			frame = frameLimit - 1;
		}
	}

	frameAxis->startVal = frame;
}

void Particle_UpdateList(struct Particle **listHead, struct Particle *p)
{
	struct Particle **link = listHead;

	while (p != NULL)
	{
		struct Particle *next = p->next;
		if (Chaos_ShouldFreezeParticle(p))
		{
			link = &p->next;
			p = next;
			continue;
		}
		u16 flagsSetColor;
		u32 axisFlags;
		u16 activeFlags;
		struct ParticleOscillator *osc;

		p->framesLeftInLife = (s16)(p->framesLeftInLife - 1);
		if (p->framesLeftInLife == -1)
		{
			goto destroyParticle;
		}

		flagsSetColor = p->flagsSetColor;
		if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_DESTROY_NOW) != 0)
		{
			goto destroyParticle;
		}

		if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE) != 0)
		{
			p->axis[PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X].startVal = p->axis[PARTICLE_AXIS_POS_X].startVal;
			p->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal = p->axis[PARTICLE_AXIS_POS_Y].startVal;
			p->axis[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z].startVal = p->axis[PARTICLE_AXIS_POS_Z].startVal;

			if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_KEEP_PREVIOUS) == 0)
			{
				CTR_WriteU32LE(&p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal,
				               CTR_ReadU32LE(&p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].velocity));
			}
		}

		axisFlags = Particle_GetAxisFlags(p);
		osc = p->oscillator;

		for (int axisIndex = 0; axisIndex < (int)len(p->axis) && axisFlags != 0; axisIndex++)
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

		if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_DESTROY_ON_SCALE_EXPIRE) != 0)
		{
			if (((activeFlags & PARTICLE_AXIS_FLAG_SCALE_X) != 0 && p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal < 1) ||
			    ((activeFlags & PARTICLE_AXIS_FLAG_SCALE_Y) != 0 && p->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal < 1))
			{
				goto destroyParticle;
			}
		}

		if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_DESTROY_ON_COLOR_EXPIRE) != 0 && Particle_ColorExpired(p, activeFlags))
		{
			goto destroyParticle;
		}

		link = &p->next;

		if ((activeFlags & PARTICLE_AXIS_FLAG_ICON_FRAME_OR_LINE_COLOR) != 0 && p->ptrIconGroup != NULL)
		{
			Particle_UpdateIconFrame(p, flagsSetColor);
		}

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


void Particle_UpdateAllParticles(void)
{
	struct GameTracker *gGT = sdata->gGT;

	if ((gGT->gameMode1 & DEBUG_MENU) != 0)
	{
		return;
	}

	Particle_UpdateList(&gGT->particleList_ordinary, gGT->particleList_ordinary);
	Particle_UpdateList(&gGT->particleList_heatWarp, gGT->particleList_heatWarp);
}


int Particle_BitwiseClampByte(int *value)
{
	if (*value < PARTICLE_COLOR_CHANNEL_MIN)
	{
		*value = PARTICLE_COLOR_CHANNEL_MIN;
	}
	else if (*value > PARTICLE_COLOR_CHANNEL_MAX)
	{
		*value = PARTICLE_COLOR_CHANNEL_MAX;
	}

	return *value >> PARTICLE_COLOR_CHANNEL_SHIFT;
}


u32 Particle_SetColors(u32 flagColors, u32 flagAlpha, struct Particle *p)
{
	u32 color = 0;

	if (flagColors & PARTICLE_SET_COLOR_FLAG_RED)
	{
		color = (u32)Particle_BitwiseClampByte(&p->axis[PARTICLE_AXIS_COLOR_R].startVal);

		if (flagColors & PARTICLE_SET_COLOR_FLAG_GREEN)
		{
			color |= (u32)Particle_BitwiseClampByte(&p->axis[PARTICLE_AXIS_COLOR_G].startVal) << PARTICLE_COLOR_CHANNEL_SHIFT;
		}
		else
		{
			color |= color << PARTICLE_COLOR_CHANNEL_SHIFT;
		}

		if (flagColors & PARTICLE_SET_COLOR_FLAG_BLUE)
		{
			color |= (u32)Particle_BitwiseClampByte(&p->axis[PARTICLE_AXIS_COLOR_B].startVal) << (PARTICLE_COLOR_CHANNEL_SHIFT * 2);
		}
		else
		{
			color |= (color & PARTICLE_COLOR_BYTE_MASK) << (PARTICLE_COLOR_CHANNEL_SHIFT * 2);
		}
	}
	else
	{
		color = PARTICLE_GPU_CODE_SHADE_TEXTURE;
	}

	if (flagAlpha & PARTICLE_SET_COLOR_FLAG_SEMI_TRANSPARENT)
	{
		color |= PARTICLE_GPU_CODE_SEMI_TRANS;
	}

	return color;
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
	{
		value = -value;
	}

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
			// NOTE(aalhendi): Retail leaves this high half as scratch residue.
			u16 viewProjR33ScratchResidue;
		};
	};
	u8 pad_14[0x0c];
	u32 *ot;
	s32 cameraOffset[3];
	s32 depth;
};

CTR_STATIC_ASSERT(offsetof(struct ParticleRenderListScratch, viewProjWords) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct ParticleRenderListScratch, viewProjR33Low) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct ParticleRenderListScratch, viewProjR33ScratchResidue) == 0x12);
CTR_STATIC_ASSERT(offsetof(struct ParticleRenderListScratch, ot) == 0x20);
CTR_STATIC_ASSERT(offsetof(struct ParticleRenderListScratch, cameraOffset) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct ParticleRenderListScratch, depth) == 0x30);

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

CTR_STATIC_ASSERT(sizeof(struct ParticleSpecialLineBody) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct ParticleSpecialLineBody, color0AndCode) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct ParticleSpecialLineBody, xy0) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct ParticleSpecialLineBody, color1) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct ParticleSpecialLineBody, xy1) == 0x0C);

CTR_STATIC_ASSERT(sizeof(struct ParticleSpecialPacket) == 0x1C);
CTR_STATIC_ASSERT(offsetof(struct ParticleSpecialPacket, tag) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct ParticleSpecialPacket, drawMode) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct ParticleSpecialPacket, pad) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct ParticleSpecialPacket, line) == 0x0C);

static struct ParticleRenderListTrig Particle_RenderList_ReadTrig(s32 angle)
{
	struct TrigTable trigApprox = data.trigApprox[ANG_MODULO_HALF_PI(angle)];
	struct ParticleRenderListTrig trig;

	if (IS_ANG_FIRST_OR_THIRD_QUADRANT(angle))
	{
		trig.sin = trigApprox.sin;
		trig.cos = trigApprox.cos;

		if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
		{
			trig.sin = -trig.sin;
			trig.cos = -trig.cos;
		}
	}
	else
	{
		trig.sin = trigApprox.cos;

		if (!IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
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

static void Particle_RenderList_LinkPrimitive(u32 *tagWord, const void *packet, u32 *ot, u32 tag)
{
	CtrGpu_LinkPacket24(ot, tagWord, packet, tag);
}

static void Particle_RenderList_LinkAndAdvance(u32 **primCursor, u32 **payloadCursor, struct Particle *particle, struct InstDrawPerPlayer *idpp,
                                               u16 flagsSetColor, s32 depth, u32 *defaultOT)
{
	u32 *prim = *primCursor;
	u32 *otBase;
	s32 otIndex;

	if (idpp != NULL)
	{
		otIndex = depth >> 5;

		if (otIndex < (u16)idpp->depthOffset[0])
		{
			otIndex = (u16)idpp->depthOffset[0];
		}

		if ((u16)idpp->depthOffset[1] < otIndex)
		{
			otIndex = (u16)idpp->depthOffset[1];
		}

		otBase = (u32 *)(u32)idpp->otRangeNormal;
	}
	else
	{
		otIndex = (depth >> 8) + (s8)particle->otIndexOffset;

		if (otIndex < 0)
		{
			otIndex = 0;
		}

		if (otIndex >= 0x400)
		{
			otIndex = 0x3ff;
		}

		otBase = defaultOT;
	}

	if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE) != 0)
	{
		struct ParticleSpecialPacket *packet = (struct ParticleSpecialPacket *)prim;

		Particle_RenderList_LinkPrimitive(&packet->tag, packet, &otBase[otIndex], PARTICLE_GPU_TAG_LENGTH_SPECIAL_LINE);
		*primCursor = (u32 *)(packet + 1);
		*payloadCursor += 7;
	}
	else
	{
		POLY_FT4 *poly = (POLY_FT4 *)prim;

		Particle_RenderList_LinkPrimitive(&poly->tag, poly, &otBase[otIndex], PARTICLE_GPU_TAG_LENGTH_POLY_FT4);
		*primCursor = (u32 *)(poly + 1);
		*payloadCursor += 10;
	}
}

static void Particle_RenderList_WriteSpecialPrimitive(struct ParticleSpecialPacket *packet, struct Particle *particle, u16 flagsAxis, u16 flagsSetColor,
                                                      u32 color, struct ParticleRenderListScratch *scratch)
{
	const struct ParticleAxis *posX = &particle->axis[PARTICLE_AXIS_POS_X];
	const struct ParticleAxis *posY = &particle->axis[PARTICLE_AXIS_POS_Y];
	const struct ParticleAxis *posZ = &particle->axis[PARTICLE_AXIS_POS_Z];
	const struct ParticleAxis *linePrevX = &particle->axis[PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X];
	const struct ParticleAxis *linePrevY = &particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y];
	const struct ParticleAxis *linePrevZ = &particle->axis[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z];
	struct ParticleAxis *lineColor = &particle->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR];

	CTC2(scratch->viewProjWords[0], 0);
	CTC2(scratch->viewProjWords[1], 1);
	CTC2(scratch->viewProjWords[2], 2);
	CTC2(scratch->viewProjWords[3], 3);
	CTC2(scratch->viewProjWords[4], 4);

	MTC2(0, 0);
	MTC2(0, 1);

	if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X) != 0)
	{
		s32 scale = particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal;
		s32 deltaX = Particle_RenderList_MulLo((linePrevX->startVal - posX->startVal) >> 6, scale);
		s32 deltaY = Particle_RenderList_MulLo((linePrevY->startVal - posY->startVal) >> 6, scale);
		s32 deltaZ = Particle_RenderList_MulLo((linePrevZ->startVal - posZ->startVal) >> 6, scale);

		MTC2(((u32)deltaX >> 16) | ((u32)(deltaY >> 16) << 16), 2);
		MTC2((u32)(deltaZ >> 16), 3);
	}
	else
	{
		s32 deltaX = (linePrevX->startVal - posX->startVal) >> 6;
		s32 deltaY = (linePrevY->startVal - posY->startVal) >> 6;
		s32 deltaZ = (linePrevZ->startVal - posZ->startVal) >> 6;

		MTC2(Particle_RenderList_PackXY(deltaX, deltaY), 2);
		MTC2((u32)deltaZ, 3);
	}

	gte_rtpt_b();

	color |= PARTICLE_GPU_CODE_LINE_G2;

	if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_SWAP_COLORS) != 0)
	{
		packet->line.color1 = color;
		packet->line.color0AndCode = lineColor->startVal;
	}
	else
	{
		packet->line.color0AndCode = color;
		packet->line.color1 = lineColor->startVal;
	}

	CTR_WriteU32LE(&lineColor->velocity, color);
	packet->drawMode = PARTICLE_GPU_DRAWMODE_BASE | (flagsSetColor & PARTICLE_SET_COLOR_FLAG_DRAW_MODE_MASK);
	packet->pad = 0;
	packet->line.xy0 = MFC2(12);
	packet->line.xy1 = MFC2(13);
	scratch->depth = (s32)MFC2(17);
}

static struct ParticleRenderListMatrix Particle_RenderList_BuildNormalMatrix(struct Particle *particle, u16 flagsAxis)
{
	const struct ParticleAxis *rotXAxis = &particle->axis[PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X];
	const struct ParticleAxis *rotYAxis = &particle->axis[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z];
	const struct ParticleAxis *scaleXAxis = &particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE];
	const struct ParticleAxis *scaleYAxis = &particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y];
	struct ParticleRenderListMatrix matrix;

	matrix.r11r12 = 0x2000;
	matrix.r13r21 = 0;
	matrix.r22r23 = 0x1000;
	matrix.r31r32 = 0;
	matrix.r33 = 0x1000;

	if ((flagsAxis & PARTICLE_AXIS_FLAG_ROT_X) == 0)
	{
		if ((flagsAxis & PARTICLE_AXIS_FLAG_ROT_Y) == 0)
		{
			if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X) != 0)
			{
				matrix.r11r12 = (u32)scaleXAxis->startVal << 1;
				matrix.r22r23 = (s32)matrix.r11r12 >> 1;
			}

			if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y) != 0)
			{
				matrix.r22r23 = scaleYAxis->startVal;
			}

			return matrix;
		}

		struct ParticleRenderListTrig rotY = Particle_RenderList_ReadTrig(rotYAxis->startVal);

		if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X) == 0)
		{
			matrix.r11r12 = (((u32)rotY.cos & 0x7fff) << 1) | ((u32)rotY.sin << 17);

			if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y) != 0)
			{
				s32 scaleY = scaleYAxis->startVal;

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
			s32 scaleX = scaleXAxis->startVal;

			if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y) != 0)
			{
				s32 scaleY = scaleYAxis->startVal;

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

	if ((flagsAxis & PARTICLE_AXIS_FLAG_ROT_Y) == 0)
	{
		struct ParticleRenderListTrig rotX = Particle_RenderList_ReadTrig(rotXAxis->startVal);

		if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X) != 0)
		{
			s32 scaleX = scaleXAxis->startVal;

			matrix.r11r12 = ((u32)scaleX << 1) & 0xffff;

			if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y) != 0)
			{
				s32 scaleY = scaleYAxis->startVal;

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

			if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y) != 0)
			{
				s32 scaleY = scaleYAxis->startVal;

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

	struct ParticleRenderListTrig rotX = Particle_RenderList_ReadTrig(rotXAxis->startVal);
	struct ParticleRenderListTrig rotY = Particle_RenderList_ReadTrig(rotYAxis->startVal);

	if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X) != 0)
	{
		s32 scaleX = scaleXAxis->startVal;

		if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y) != 0)
		{
			s32 scaleY = scaleYAxis->startVal;
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
		if ((flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y) != 0)
		{
			s32 scaleY = scaleYAxis->startVal;
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
	(void)flagsAxis;
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

	CtrGpu_WriteColorCode(&poly->r0, color | PARTICLE_GPU_CODE_POLY_FT4);
	CtrGpu_WritePackedUVWord(&poly->u0, CTR_ReadU32LE(&icon->texLayout.u0));
	CtrGpu_WritePackedUVWord(&poly->u1, (CTR_ReadU32LE(&icon->texLayout.u1) & PARTICLE_TEXTURE_DRAW_MODE_MASK) |
	                                        ((u32)(flagsSetColor & PARTICLE_SET_COLOR_FLAG_DRAW_MODE_MASK) << 16));
	CtrGpu_WritePackedUV(&poly->u2, CTR_ReadU16LE(&icon->texLayout.u2));
	CtrGpu_WritePackedUV(&poly->u3, CTR_ReadU16LE(&icon->texLayout.u3));

	width = (icon->texLayout.u1 - icon->texLayout.u0) + 1;
	height = (icon->texLayout.v3 - icon->texLayout.v0) + 1;

	halfWidth = width << 1;
	halfHeight = height << 1;

	if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_LARGE_QUAD) != 0)
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

void Particle_RenderList(struct PushBuffer *pb, void *particleList)
{
	struct GameTracker *gGT = sdata->gGT;
	struct PrimMem *primMem = &gGT->backBuffer->primMem;
	struct Particle *particle = particleList;
	struct ParticleRenderListScratch *scratch = CTR_SCRATCHPAD_PTR(struct ParticleRenderListScratch, 0x00);
	u32 *prim = (u32 *)primMem->cursor;
	u32 *primPayload = prim + 8;
	s8 cameraID;

	PushBuffer_SetPsyqGeom(pb);

	scratch->viewProjWords[0] = CTR_ReadU32LE(&pb->matrix_ViewProj.m[0][0]);
	scratch->viewProjWords[1] = CTR_ReadU32LE(&pb->matrix_ViewProj.m[0][2]);
	scratch->viewProjWords[2] = CTR_ReadU32LE(&pb->matrix_ViewProj.m[1][1]);
	scratch->viewProjWords[3] = CTR_ReadU32LE(&pb->matrix_ViewProj.m[2][0]);
	scratch->viewProjR33Low = CTR_ReadU16LE(&pb->matrix_ViewProj.m[2][2]);

	CTC2(scratch->viewProjWords[0], 8);
	CTC2(scratch->viewProjWords[1], 9);
	CTC2(scratch->viewProjWords[2], 10);
	CTC2(scratch->viewProjWords[3], 11);
	CTC2(scratch->viewProjWords[4], 12);

	scratch->ot = pb->ptrOT;
	cameraID = (s8)pb->cameraID;
	scratch->cameraOffset[0] = CTR_MipsSll(pb->matrix_Camera.t[0], 2);
	scratch->cameraOffset[1] = CTR_MipsSll(pb->matrix_Camera.t[1], 2);
	scratch->cameraOffset[2] = CTR_MipsSll(pb->matrix_Camera.t[2], 2);

	if (prim + (gGT->numParticles * 10) >= (u32 *)primMem->guardEnd)
	{
		return;
	}

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
			{
				goto next_particle;
			}

			iconGroup = particle->ptrIconGroup;
			if (iconGroup == NULL)
			{
				goto next_particle;
			}

			flagsAxis = particle->flagsAxis;
			if ((flagsAxis & PARTICLE_AXIS_FLAG_ICON_FRAME_OR_LINE_COLOR) == 0)
			{
				icon = particle->ptrIconArray;
			}
			else
			{
				int frame = particle->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal >> 8;

				if (frame < 0)
				{
					frame = 0;
				}

				if (iconGroup->numIcons <= frame)
				{
					frame = iconGroup->numIcons - 1;
				}

				if (frame < 0)
				{
					goto next_particle;
				}

				icon = ((struct Icon **)ICONGROUP_GETICONS(iconGroup))[frame];
				particle->ptrIconArray = icon;
			}

			if (icon == NULL)
			{
				goto next_particle;
			}

			idpp = NULL;
			posX = particle->axis[PARTICLE_AXIS_POS_X].startVal >> 6;
			posY = particle->axis[PARTICLE_AXIS_POS_Y].startVal >> 6;
			posZ = particle->axis[PARTICLE_AXIS_POS_Z].startVal >> 6;
			flagsSetColor = particle->flagsSetColor;

			if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL) != 0 && particle->owner.driverInst != NULL)
			{
				struct Instance *inst = particle->owner.driverInst;
				u32 idppFlags;

				idpp = Particle_RenderList_GetIdpp(inst, cameraID);
				idppFlags = idpp->instFlags;

				if ((idppFlags & DRAW_SUCCESSFUL) == 0)
				{
					goto next_particle;
				}

				posX += CTR_MipsSll(inst->matrix.t[0], 2);
				if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL_IGNORE_Y) == 0)
				{
					posY += CTR_MipsSll(inst->matrix.t[1], 2);
				}
				posZ += CTR_MipsSll(inst->matrix.t[2], 2);

				if ((idppFlags & PUSHBUFFER_EXISTS) != 0)
				{
					idpp = NULL;
				}
			}

			posX -= scratch->cameraOffset[0];
			posY -= scratch->cameraOffset[1];
			posZ -= scratch->cameraOffset[2];

			if (!Particle_RenderList_IsNearCamera(posX))
			{
				goto next_particle;
			}
			if (!Particle_RenderList_IsNearCamera(posY))
			{
				goto next_particle;
			}
			if (!Particle_RenderList_IsNearCamera(posZ))
			{
				goto next_particle;
			}

			MTC2((u32)(u16)posX | ((u32)posY << 16), 0);
			MTC2((u32)posZ, 1);
			gte_llv0_b();

			CTC2(MFC2(25), 5);
			CTC2(MFC2(26), 6);
			CTC2(MFC2(27), 7);

			depth = (s32)MFC2(27);
			if (depth < 0)
			{
				goto next_particle;
			}

			if (CTR_MipsSll(particle->renderDepthLimit, 2) < depth)
			{
				goto next_particle;
			}

			color = Particle_SetColors(flagsAxis, flagsSetColor, particle);

			if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE) != 0)
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
	return CTR_ReadU32LE(&p->flagsAxis);
}

static void Particle_Init_SetAxisFlags(struct Particle *p, u32 flags)
{
	CTR_WriteU32LE(&p->flagsAxis, flags);
}

static u8 ParticleEmitter_GetInitOffset(const struct ParticleEmitter *emSet)
{
	return (u8)emSet->initOffset;
}

static void ParticleEmitter_CopyOscillator(struct ParticleOscillator *osc, const struct ParticleEmitter *emSet)
{
	const struct ParticleOscillatorConfig *src = &emSet->tail.oscillator;

	CTR_WriteU32LE(&osc->flags, CTR_ReadU32LE(&src->flags));
	CTR_WriteU32LE((u8 *)&osc->flags + 4, CTR_ReadU32LE(&src->range.period));
	CTR_WriteU32LE((u8 *)&osc->flags + 8, CTR_ReadU32LE(&src->range.scale));
	CTR_WriteU32LE((u8 *)&osc->flags + 12, CTR_ReadU32LE(&src->range.min));
}

static void Particle_InitAxis(struct Particle *p, const struct ParticleEmitter *emSet, u8 axisIndex, u32 *flagsAxis)
{
	struct ParticleAxis *axis = &p->axis[axisIndex];
	u16 flags = emSet->flags;
	int value = 0;
	s16 velocity = 0;
	s16 accel = 0;

	if ((flags & PARTICLE_EMITTER_FLAG_BASE_START) != 0)
	{
		value = emSet->InitTypes.AxisInit.baseValue.startVal;
	}
	if ((flags & PARTICLE_EMITTER_FLAG_RANDOM_START) != 0)
	{
		value += MixRNG_Particles(emSet->InitTypes.AxisInit.rngSeed.startVal);
	}

	axis->startVal = value;

	if ((flags & PARTICLE_EMITTER_FLAG_BASE_VELOCITY) != 0)
	{
		velocity = emSet->InitTypes.AxisInit.baseValue.velocity;
	}
	if ((flags & PARTICLE_EMITTER_FLAG_RANDOM_VELOCITY) != 0)
	{
		velocity = (s16)(velocity + MixRNG_Particles(emSet->InitTypes.AxisInit.rngSeed.velocity));
	}

	axis->velocity = velocity;

	if ((flags & PARTICLE_EMITTER_FLAG_BASE_ACCEL) != 0)
	{
		accel = emSet->InitTypes.AxisInit.baseValue.accel;
	}
	if ((flags & PARTICLE_EMITTER_FLAG_RANDOM_ACCEL) != 0)
	{
		accel = (s16)(accel + MixRNG_Particles(emSet->InitTypes.AxisInit.rngSeed.accel));
	}

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
		{
			return;
		}

		localOsc[axisIndex] = osc;
	}
	else
	{
		osc = localOsc[axisIndex];
	}

	ParticleEmitter_CopyOscillator(osc, emSet);

	if ((osc->flags & PARTICLE_OSC_FLAG_PHASE_RELATIVE_TO_NOW) != 0)
	{
		osc->phase = (s16)(osc->phase - (u16)sdata->gGT->frameTimer_Confetti);
	}

	if ((osc->flags & PARTICLE_OSC_FLAG_MODE_MASK) == PARTICLE_OSC_MODE_SEEDED_RANDOM)
	{
		osc->previousValue = osc->phase;
	}

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
	const struct ParticleOscillatorRandomRange *rng;

	if ((flagsAxis & (1u << ((axisIndex + 0x10) & 0x1f))) == 0)
	{
		return;
	}

	osc = localOsc[axisIndex];
	rng = &emSet->tail.oscillator.range;

	if (rng->period != 0)
	{
		osc->period = (u16)(osc->period + MixRNG_Particles(rng->period));
	}
	if (rng->phase != 0)
	{
		osc->phase = (s16)(osc->phase + MixRNG_Particles(rng->phase));
	}
	if (rng->scale != 0)
	{
		osc->scale = (u16)(osc->scale + MixRNG_Particles(rng->scale));
	}
	if (rng->offset != 0)
	{
		osc->offset = (s16)(osc->offset + MixRNG_Particles(rng->offset));
	}
	if (rng->min != 0)
	{
		osc->min = (s16)(osc->min + MixRNG_Particles(rng->min));
	}
	if (rng->max != 0)
	{
		osc->max = (s16)(osc->max + MixRNG_Particles(rng->max));
	}
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
	{
		return NULL;
	}

	gGT->numParticles++;
	p->owner.driverInst = NULL;
	Chaos_ClearParticleOwner(p);

	p->ptrIconGroup = ig;
	if (ig != NULL && ig->numIcons != 0 && ig->numIcons > 0)
	{
		p->ptrIconArray = ((struct Icon **)ICONGROUP_GETICONS(ig))[0];
	}
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

			if (axisIndex == PARTICLE_EMITTER_INIT_FUNC_OFFSET)
			{
				if ((flags & PARTICLE_EMITTER_FLAG_NON_FUNC_INIT_MASK) == 0)
				{
					p->funcPtr = emSet->InitTypes.FuncInit.particle_funcPtr;
					p->flagsSetColor = emSet->InitTypes.FuncInit.particle_colorFlags;
					p->framesLeftInLife = emSet->InitTypes.FuncInit.particle_lifespan;
					flagsAxis |= PARTICLE_AXIS_FLAG_FUNC_INIT;
					particleType = emSet->InitTypes.FuncInit.particle_Type;
				}
			}
			else if ((flags & PARTICLE_EMITTER_FLAG_OSCILLATOR_RANDOMIZE) != 0)
			{
				Particle_RandomizeOscillator(localOsc, emSet, axisIndex, flagsAxis);
			}
			else if ((flags & PARTICLE_EMITTER_FLAG_OSCILLATOR) != 0)
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

	if ((flagsAxis & PARTICLE_AXIS_FLAG_FUNC_INIT) == 0)
	{
		p->funcPtr = NULL;
		p->flagsSetColor = 0;
		p->framesLeftInLife = 0;
		p->ptrIconArray = NULL;
		p->ptrIconGroup = NULL;
	}

	Particle_Init_SetAxisFlags(p, flagsAxis & ~PARTICLE_AXIS_FLAG_FUNC_INIT);

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

	p->renderDepthLimit = 0x400;
	p->driverID = -1;
	p->otIndexOffset = 0;

	if ((p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE) != 0)
	{
		if ((p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_KEEP_PREVIOUS) == 0)
		{
			u32 color = Particle_SetColors(Particle_Init_GetAxisFlags(p), p->flagsSetColor, p) | PARTICLE_GPU_CODE_LINE_G2;

			p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal = color;
			CTR_WriteU32LE(&p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].velocity, color);
		}
		else
		{
			u32 color = PARTICLE_GPU_CODE_LINE_G2;

			if ((p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_SEMI_TRANSPARENT) != 0)
			{
				color = PARTICLE_GPU_CODE_LINE_G2 | PARTICLE_GPU_CODE_SEMI_TRANS;
			}

			p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal = color;
		}
	}

	return p;
}
