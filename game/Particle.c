#include <ctr_gte_transfer.h>
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
	PARTICLE_SPIT_TIRE_FRAME_3_VELOCITY = -2047,
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
	int rng;

	if (p->axis[PARTICLE_AXIS_POS_Y].velocity < PARTICLE_POTION_SHATTER_Y_SPEED_THRESHOLD)
	{
		if (p->axis[PARTICLE_AXIS_POS_X].velocity != 0)
		{
			goto FadeShatterChannel;
		}

		// random X
		p->axis[PARTICLE_AXIS_POS_X].velocity = MixRNG_Scramble() % PARTICLE_POTION_SHATTER_XZ_RANDOM_RANGE - PARTICLE_POTION_SHATTER_XZ_RANDOM_CENTER;

		// random Z
		p->axis[PARTICLE_AXIS_POS_Z].velocity = MixRNG_Scramble() % PARTICLE_POTION_SHATTER_XZ_RANDOM_RANGE - PARTICLE_POTION_SHATTER_XZ_RANDOM_CENTER;

		// random scale
		rng = MixRNG_Scramble();
		p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].velocity = rng % PARTICLE_POTION_SHATTER_SCALE_RANDOM_RANGE + PARTICLE_POTION_SHATTER_SCALE_RANDOM_BASE;
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
	int scaleFrame;
	// NOTE(aalhendi): Keep the shared branch result in retail's return-value register.
	register s32 targetY CTR_PSX_REGISTER("v0");

	// Wait until tires are 0x10 units above
	// the ground, which is where the plant
	// actually "spits" tires from the mouth
	if ((p->axis[PARTICLE_AXIS_POS_Y].startVal >> 8) >= p->owner.plantInst->matrix.t[1] + PARTICLE_SPIT_TIRE_MOUTH_Y_OFFSET)
	{
		return;
	}

	// random X
	p->axis[PARTICLE_AXIS_POS_X].velocity = MixRNG_Scramble() % PARTICLE_SPIT_TIRE_XZ_RANDOM_RANGE - PARTICLE_SPIT_TIRE_XZ_RANDOM_CENTER;

	// random Z
	p->axis[PARTICLE_AXIS_POS_Z].velocity = MixRNG_Scramble() % PARTICLE_SPIT_TIRE_XZ_RANDOM_RANGE - PARTICLE_SPIT_TIRE_XZ_RANDOM_CENTER;

	// scale value
	scaleFrame = p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal;

	// frame #1
	if (scaleFrame == PARTICLE_SPIT_TIRE_FRAME_1)
	{
		// random Y
		p->axis[PARTICLE_AXIS_POS_Y].velocity = MixRNG_Scramble() % PARTICLE_SPIT_TIRE_FRAME_1_Y_RANDOM_RANGE + PARTICLE_SPIT_TIRE_FRAME_1_Y_BASE;
		targetY = p->owner.plantInst->matrix.t[1];

		// frame #2
		p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal = PARTICLE_SPIT_TIRE_FRAME_2;
	}

	// frame #2
	else if (scaleFrame == PARTICLE_SPIT_TIRE_FRAME_2)
	{
		// random Y
		p->axis[PARTICLE_AXIS_POS_Y].velocity = MixRNG_Scramble() % PARTICLE_SPIT_TIRE_LATER_Y_RANDOM_RANGE + PARTICLE_SPIT_TIRE_FRAME_2_Y_BASE;
		targetY = p->owner.plantInst->matrix.t[1];

		// frame #3
		p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal = PARTICLE_SPIT_TIRE_FRAME_3;
	}

	// frame #3
	else if (scaleFrame == PARTICLE_SPIT_TIRE_FRAME_3)
	{
		// random Y
		p->axis[PARTICLE_AXIS_POS_Y].velocity = MixRNG_Scramble() % PARTICLE_SPIT_TIRE_LATER_Y_RANDOM_RANGE + PARTICLE_SPIT_TIRE_FRAME_3_Y_BASE;
		targetY = p->owner.plantInst->matrix.t[1];

		p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].velocity = PARTICLE_SPIT_TIRE_FRAME_3_VELOCITY;
	}
	else
	{
		return;
	}

	p->axis[PARTICLE_AXIS_POS_Y].startVal = ((u32)targetY + PARTICLE_SPIT_TIRE_MOUTH_Y_OFFSET) << 8;
}


void Particle_FuncPtr_ExhaustUnderwater(struct Particle *p)
{
	struct IconGroup *icon;

	if ((PARTICLE_EXHAUST_WATER_HEIGHT_THRESHOLD < ((p->axis[PARTICLE_AXIS_POS_Y].startVal >> 8) + p->owner.driverInst->matrix.t[1])) &&
	    (p->framesLeftInLife < PARTICLE_EXHAUST_POP_LIFE_THRESHOLD))
	{
		// bubblepop
		icon = GAME_TRACKER->iconGroup[PARTICLE_EXHAUST_BUBBLEPOP_ICON_GROUP];
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

		LIST_AddFront(&GAME_TRACKER->JitPools.oscillator.free, (struct Item *)osc);
		osc = next;
	}
}


void Particle_UpdateList(struct Particle **listHead, struct Particle *p)
{
	struct Particle *next;
	struct ParticleOscillator *osc;
	struct ParticleAxis *axis;
	s16 *velocity;
	s32 axisFlags;
	u16 flagsSetColor;
	s32 value;

	while (p != NULL)
	{
		next = p->next;
		p->framesLeftInLife--;
		if (p->framesLeftInLife == -1)
			goto destroyParticle;
		flagsSetColor = p->flagsSetColor;
		if (flagsSetColor & PARTICLE_SET_COLOR_FLAG_DESTROY_NOW)
			goto destroyParticle;
		if (flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE)
		{
			p->axis[PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X].startVal = p->axis[PARTICLE_AXIS_POS_X].startVal;
			p->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal = p->axis[PARTICLE_AXIS_POS_Y].startVal;
			p->axis[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z].startVal = p->axis[PARTICLE_AXIS_POS_Z].startVal;
			if (!(flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_KEEP_PREVIOUS))
				p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal = CTR_ReadU32AlignedLE(&p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].velocity);
		}
		// Axis and oscillator bits advance together, while oscillator nodes
		// exist only for axes that have the corresponding high-half bit set.
		axisFlags = CTR_ReadU32AlignedLE(&p->flagsAxis);
		osc = p->oscillator;
		axis = p->axis;
		if (axisFlags)
		{
			velocity = &p->axis[0].velocity;
			do
			{
				if (!(axisFlags & 1))
					goto nextAxis;
				axis->startVal = CTR_MipsAddLo(axis->startVal, velocity[0]);
				velocity[0] += velocity[1];
				if (!((axisFlags >> 16) & 1) || !osc)
					goto nextAxis;
				if (!(osc->config.flags & PARTICLE_OSC_FLAG_SKIP_PREVIOUS_SUBTRACT))
				{
					if (osc->config.flags & PARTICLE_OSC_FLAG_APPLY_TO_VELOCITY)
						velocity[0] -= osc->config.previousValue;
					else
						axis->startVal = CTR_MipsSubLo(axis->startVal, osc->config.previousValue);
				}
				value = GAME_TRACKER->frameTimer_Confetti;
				switch (osc->config.flags & PARTICLE_OSC_FLAG_MODE_MASK)
				{
				case PARTICLE_OSC_MODE_SINE:
					value = CTR_MipsAddLo(value, osc->config.range.phase);
					value = MATH_Sin(CTR_MipsMulLo(osc->config.range.period, value) >> 5);
					goto applyOscillator;
				case PARTICLE_OSC_MODE_ABS_SINE:
					value = CTR_MipsAddLo(value, osc->config.range.phase);
					value = abs(MATH_Sin(CTR_MipsMulLo(osc->config.range.period, value) >> 6));
					value = (u32)value << 1;
					break;
				case PARTICLE_OSC_MODE_SAW:
					value = CTR_MipsAddLo(value, osc->config.range.phase);
					value = CTR_MipsMulLo(osc->config.range.period, value) >> 4;
					value &= 0x1fff;
					break;
				case PARTICLE_OSC_MODE_TRIANGLE:
					value = CTR_MipsAddLo(value, osc->config.range.phase);
					value = CTR_MipsMulLo(osc->config.range.period, value) >> 3;
					value &= 0x3fff;
					if (value > 0x2000)
						value = 0x4000 - value;
					break;
				case PARTICLE_OSC_MODE_SQUARE:
					value = CTR_MipsAddLo(value, osc->config.range.phase);
					value = CTR_MipsMulLo(osc->config.range.period, value) >> 6;
					if (value & 0x400)
						value = 0x1000;
					else
						value = -0x1000;
					goto applyOscillator;
				case PARTICLE_OSC_MODE_RANDOM:
					value = MixRNG_Scramble() >> 3;
					break;
				case PARTICLE_OSC_MODE_SEEDED_RANDOM:
					value = (s32)MixRNG_GetValue(osc->config.previousValue) >> 3;
					break;
				default:
					goto applyOscillator;
				}
				value -= 0x1000;
			applyOscillator:
				value = CTR_MipsAddLo(value, osc->config.range.offset);
				value = CTR_MipsMulLo(value, osc->config.range.scale) >> 12;
				if (osc->config.range.max < value)
					value = osc->config.range.max;
				if (value < osc->config.range.min)
					value = osc->config.range.min;
				if (osc->config.flags & PARTICLE_OSC_FLAG_APPLY_TO_VELOCITY)
					velocity[0] += value;
				else
					axis->startVal = CTR_MipsAddLo(axis->startVal, value);
				osc->config.previousValue = value;
				osc = osc->next;
			nextAxis:
				velocity = (s16 *)((u8 *)velocity + sizeof(*axis));
				axisFlags = (s32)((u32)axisFlags & 0xfffeffffu) >> 1;
				++axis;
			} while (axisFlags);
		}
		if (p->funcPtr)
			((void (*)(struct Particle *))p->funcPtr)(p);
		axisFlags = p->flagsAxis;
		if (flagsSetColor & PARTICLE_SET_COLOR_FLAG_DESTROY_ON_SCALE_EXPIRE)
		{
			if ((axisFlags & PARTICLE_AXIS_FLAG_SCALE_X) && p->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal < 1)
				goto destroyParticle;
			if ((axisFlags & PARTICLE_AXIS_FLAG_SCALE_Y) && p->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal < 1)
				goto destroyParticle;
		}
		if (flagsSetColor & PARTICLE_SET_COLOR_FLAG_DESTROY_ON_COLOR_EXPIRE)
		{
			s32 color = 0;
			if ((axisFlags & PARTICLE_AXIS_FLAG_COLOR_R) && p->axis[PARTICLE_AXIS_COLOR_R].startVal > 0)
				color = p->axis[PARTICLE_AXIS_COLOR_R].startVal;
			if ((axisFlags & PARTICLE_AXIS_FLAG_COLOR_G) && p->axis[PARTICLE_AXIS_COLOR_G].startVal > 0)
				color |= p->axis[PARTICLE_AXIS_COLOR_G].startVal;
			if ((axisFlags & PARTICLE_AXIS_FLAG_COLOR_B) && p->axis[PARTICLE_AXIS_COLOR_B].startVal > 0)
				color |= p->axis[PARTICLE_AXIS_COLOR_B].startVal;
			if (color < 0x800)
				goto destroyParticle;
		}
		goto keepParticle;
	destroyParticle:
		Particle_OnDestroy(p);
		LIST_AddFront(&GAME_TRACKER->JitPools.particle.free, (struct Item *)p);
		GAME_TRACKER->numParticles--;
		*listHead = next;
		goto nextParticle;
	keepParticle:
		// NOTE(aalhendi): The callback may change animation flags. Expiry uses
		// the earlier snapshot, but frame wrapping and bouncing use live flags.
		if ((p->flagsAxis & PARTICLE_AXIS_FLAG_ICON_FRAME_OR_LINE_COLOR) && p->ptrIconGroup)
		{
			s32 frame = p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal;
			s32 limit;
			if (frame < 0)
			{
				if (p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_ICON_WRAP)
				{
					frame += p->ptrIconGroup->numIcons << 8;
					goto storeFrame;
				}
				if (p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_ICON_BOUNCE)
					goto bounceFrame;
				frame = 0;
			}
			else
			{
				limit = p->ptrIconGroup->numIcons << 8;
				if (frame < limit)
				{
					listHead = &p->next;
					goto nextParticle;
				}
				if (p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_ICON_WRAP)
				{
					frame -= limit;
					goto storeFrame;
				}
				if (p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_ICON_BOUNCE)
				{
				bounceFrame:
					frame = CTR_MipsSubLo(frame, p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].velocity * 2);
					p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].velocity = -p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].velocity;
					p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].accel = -p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].accel;
					goto storeFrame;
				}
				frame = limit - 1;
			}
		storeFrame:
			p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal = frame;
		}
		listHead = &p->next;
	nextParticle:
		p = next;
	}
}

void Particle_UpdateAllParticles(void)
{
	struct GameTracker *gGT = GAME_TRACKER;

	if ((gGT->gameMode1 & DEBUG_MENU) != 0)
	{
		return;
	}

	Particle_UpdateList(&gGT->particleList_ordinary, gGT->particleList_ordinary);
	gGT = GAME_TRACKER;
	Particle_UpdateList(&gGT->particleList_heatWarp, gGT->particleList_heatWarp);
}


int Particle_BitwiseClampByte(int *value)
{
	s32 color = *value;
	if (color < PARTICLE_COLOR_CHANNEL_MIN)
	{
		color = PARTICLE_COLOR_CHANNEL_MIN;
		*value = color;
	}
	else if (color > PARTICLE_COLOR_CHANNEL_MAX)
	{
		color = PARTICLE_COLOR_CHANNEL_MAX;
		*value = color;
	}

	return color >> PARTICLE_COLOR_CHANNEL_SHIFT;
}


u32 Particle_SetColors(u32 flagColors, u32 flagAlpha, struct Particle *p)
{
	u32 color;
	u32 red;

	if (flagColors & PARTICLE_SET_COLOR_FLAG_RED)
	{
		color = (u32)Particle_BitwiseClampByte(&p->axis[PARTICLE_AXIS_COLOR_R].startVal);
		red = color;

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
			color |= red << (PARTICLE_COLOR_CHANNEL_SHIFT * 2);
		}
		if (flagAlpha & PARTICLE_SET_COLOR_FLAG_SEMI_TRANSPARENT)
		{
			color |= PARTICLE_GPU_CODE_SEMI_TRANS;
		}
	}
	else
	{
		color = PARTICLE_GPU_CODE_SHADE_TEXTURE;
		if (flagAlpha & PARTICLE_SET_COLOR_FLAG_SEMI_TRANSPARENT)
		{
			color = PARTICLE_GPU_CODE_SHADE_TEXTURE | PARTICLE_GPU_CODE_SEMI_TRANS;
		}
	}

	return color;
}


static inline u32 Particle_RenderList_PackXY(s32 x, s32 y)
{
	return ((u32)x & 0xffff) | ((u32)y << 16);
}

static inline struct InstDrawPerPlayer *Particle_RenderList_GetIdpp(struct Instance *inst, int cameraID)
{
	return (struct InstDrawPerPlayer *)((char *)inst + sizeof(struct Instance) + (cameraID * sizeof(struct InstDrawPerPlayer)));
}

struct ParticleRenderListScratch
{
	// NOTE(aalhendi): The packed view-projection words are also a MATRIX for
	// the native GTE transfer. Both views occupy the same scratchpad bytes.
	union
	{
		u32 viewProjWords[8];
		MATRIX matrix;
	} view;
	u32 *ot;
	s32 cameraOffset[3];
	s32 depth;
};

CTR_STATIC_ASSERT(offsetof(struct ParticleRenderListScratch, ot) == 0x20);
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

CTR_STATIC_ASSERT(sizeof(struct ParticleSpecialPacket) == 0x1C);
CTR_STATIC_ASSERT(offsetof(struct ParticleSpecialPacket, line) == 0x0C);

// NOTE(aalhendi): Keep each vector's two GTE writes adjacent on PSX; native
// uses the same register values through its GTE interface.
#ifdef CTR_NATIVE
#define Particle_LoadVector(xy, z, xyreg, zreg) \
	do                                          \
	{                                           \
		MTC2(xy, xyreg);                        \
		MTC2(z, zreg);                          \
	} while (0)
#else
#define Particle_LoadVector(xy, z, xyreg, zreg) __asm__ volatile("mtc2 %0,$" #xyreg "\n\tmtc2 %1,$" #zreg : : "r"(xy), "r"(z))
#endif

void Particle_RenderList(struct PushBuffer *pb, void *particleList)
{
	struct Particle *particle = particleList;
	struct ParticleRenderListScratch *scratch;
	const struct TrigTable *trigTable;
	u32 *prim;
	u32 *payload;
	s32 cameraID;
	PushBuffer_SetPsyqGeom(pb);
	scratch = CTR_SCRATCHPAD_PTR(struct ParticleRenderListScratch, 0);
	scratch->view.viewProjWords[0] = CTR_ReadU32AlignedLE(&pb->matrix_ViewProj.m[0][0]);
	scratch->view.viewProjWords[1] = CTR_ReadU32AlignedLE(&pb->matrix_ViewProj.m[0][2]);
	scratch->view.viewProjWords[2] = CTR_ReadU32AlignedLE(&pb->matrix_ViewProj.m[1][1]);
	{
		u16 last = pb->matrix_ViewProj.m[2][2];
		u32 fourth = CTR_ReadU32AlignedLE(&pb->matrix_ViewProj.m[2][0]);
		scratch->view.matrix.m[2][2] = last;
		scratch->view.viewProjWords[3] = fourth;
	}
	CTR_GteLoadLightMatrix(&scratch->view.matrix);
	scratch->ot = pb->ptrOT;
	cameraID = (s8)pb->cameraID;
	scratch->cameraOffset[0] = CTR_MipsSll(pb->matrix_Camera.t[0], 2);
	scratch->cameraOffset[1] = CTR_MipsSll(pb->matrix_Camera.t[1], 2);
	scratch->cameraOffset[2] = CTR_MipsSll(pb->matrix_Camera.t[2], 2);
	{
		s32 primBytes = GAME_TRACKER->numParticles * 40;
		struct DB *backBuffer = GAME_TRACKER->backBuffer;
		u32 *cursor = backBuffer->primMem.cursor;
		if ((u8 *)cursor + primBytes >= (u8 *)backBuffer->primMem.guardEnd)
			return;
		prim = cursor;
	}
	if (particle)
	{
		{
			// NOTE(aalhendi): Form the retail table address once, before the
			// loop. Native resolves the same table directly from game data.
			u32 trigPage;
			CTR_PSX_LOAD_SYMBOL_PAGE(trigPage, "data+15360");
			CTR_PSX_ADD_SYMBOL_LOW(trigTable, trigPage, "data+15360", data.trigApprox);
		}
		// NOTE(aalhendi): These liveness hints retain retail's register
		// allocation; they are inert in the native build.
		CTR_PSX_OBSERVE_VALUE(cameraID);
		payload = prim + 8;
		do
		{
			struct IconGroup *iconGroup;
			struct Icon *icon;
			struct InstDrawPerPlayer *idpp;
			u32 flagsAxis;
			u32 flagsSetColor;
			s32 posX, posY, posZ;
			u32 color;
			s32 r11r12;
			s32 r13r21;
			s32 r22r23;
			s32 r31r32;
			s32 r33;
			s32 cosY;
			s32 sinX;
			s32 cosX;
			s32 sinY;

			CTR_PSX_OBSERVE_VALUE(cameraID);
			if ((s8)particle->driverID != -1 && (s8)particle->driverID != cameraID)
				goto nextParticle;
			iconGroup = particle->ptrIconGroup;
			if (!iconGroup)
				goto nextParticle;
			flagsAxis = particle->flagsAxis;
			if (flagsAxis & PARTICLE_AXIS_FLAG_ICON_FRAME_OR_LINE_COLOR)
			{
				r11r12 = particle->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal >> 8;
				if (r11r12 < 0)
					r11r12 = 0;
				if (iconGroup->numIcons <= r11r12)
					r11r12 = iconGroup->numIcons - 1;
				if (r11r12 < 0)
					goto nextParticle;
				{
					u32 offset = r11r12 * sizeof(struct Icon *);
					struct Icon *loaded = *(struct Icon **)((u8 *)iconGroup + offset + sizeof(*iconGroup));
					CTR_PSX_OBSERVE_VALUE(loaded);
					icon = loaded;
				}
				particle->ptrIconArray = icon;
			}
			else
				icon = particle->ptrIconArray;
			idpp = NULL;
			if (!icon)
				goto nextParticle;
			CTR_PSX_OBSERVE_VALUE(cameraID);
			posX = particle->axis[PARTICLE_AXIS_POS_X].startVal >> 6;
			posY = particle->axis[PARTICLE_AXIS_POS_Y].startVal >> 6;
			posZ = particle->axis[PARTICLE_AXIS_POS_Z].startVal >> 6;
			flagsSetColor = particle->flagsSetColor;
			if ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL) && particle->owner.driverInst)
			{
				struct Instance *inst = particle->owner.driverInst;
				u32 idppFlags;
				idpp = Particle_RenderList_GetIdpp(inst, cameraID);
				idppFlags = idpp->instFlags;
				if (!(idppFlags & DRAW_SUCCESSFUL))
					goto nextParticle;
				posX = CTR_MipsAddLo(posX, CTR_MipsSll(inst->matrix.t[0], 2));
				if (!(flagsSetColor & PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL_IGNORE_Y))
					posY = CTR_MipsAddLo(posY, CTR_MipsSll(inst->matrix.t[1], 2));
				posZ = CTR_MipsAddLo(posZ, CTR_MipsSll(inst->matrix.t[2], 2));
				if (idppFlags & PUSHBUFFER_EXISTS)
					idpp = NULL;
			}
			posX = CTR_MipsSubLo(posX, scratch->cameraOffset[0]);
			posY = CTR_MipsSubLo(posY, scratch->cameraOffset[1]);
			posZ = CTR_MipsSubLo(posZ, scratch->cameraOffset[2]);
			if (abs(posX) > 30000 || abs(posY) > 30000 || abs(posZ) > 30000)
				goto nextParticle;
			Particle_LoadVector(Particle_RenderList_PackXY(posX, posY), posZ, 0, 1);
			CTR_GteLoadDelay();
			gte_llv0_b();
			{
				register s32 macX CTR_PSX_REGISTER("$12") = MFC2(25);
				register s32 macY CTR_PSX_REGISTER("$13") = MFC2(26);
				register s32 macZ CTR_PSX_REGISTER("$14") = MFC2(27);
				CTC2(macX, 5);
				CTC2(macY, 6);
				CTC2(macZ, 7);
			}
			CTR_GteReadDataDelayed(r13r21, 27);
			if (r13r21 < 0)
				goto nextParticle;
			if (CTR_MipsSll(particle->renderDepthLimit, 2) < r13r21)
				goto nextParticle;
			color = Particle_SetColors(flagsAxis, flagsSetColor, particle);
			if (flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE)
			{
				struct ParticleSpecialPacket *packet = (struct ParticleSpecialPacket *)prim;
				CTR_GteSetRotMatrix((MATRIX *)scratch);
#ifdef CTR_NATIVE
				MTC2(0, 0);
				MTC2(0, 1);
#else
				// NOTE(aalhendi): Retail stages zero in t9 before both GTE
				// writes. Binding t9 as a C variable spills the trig table.
				__asm__ volatile("move $25,$0\n\tmtc2 $25,$0\n\tmtc2 $25,$1");
#endif
				if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X)
				{
					s32 scale;
					s32 deltaX, deltaY, deltaZ;
					scale = particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal;
					deltaY = CTR_MipsMulLo(
					    CTR_MipsSubLo(particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal, particle->axis[PARTICLE_AXIS_POS_Y].startVal) >> 6, scale);
					deltaZ = CTR_MipsMulLo(
					    CTR_MipsSubLo(particle->axis[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z].startVal, particle->axis[PARTICLE_AXIS_POS_Z].startVal) >> 6, scale);
					deltaX = CTR_MipsMulLo(
					    CTR_MipsSubLo(particle->axis[PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X].startVal, particle->axis[PARTICLE_AXIS_POS_X].startVal) >> 6, scale);
					r13r21 = deltaY >> 16;
					deltaZ >>= 16;
					{
						register s32 gteZ CTR_PSX_REGISTER("$5") = deltaZ;
						Particle_LoadVector(((u32)deltaX >> 16) | ((u32)r13r21 << 16), gteZ, 2, 3);
					}
				}
				else
				{
					MTC2(Particle_RenderList_PackXY(
					         CTR_MipsSubLo(particle->axis[PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X].startVal, particle->axis[PARTICLE_AXIS_POS_X].startVal) >> 6,
					         CTR_MipsSubLo(particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal, particle->axis[PARTICLE_AXIS_POS_Y].startVal) >> 6),
					     2);
					MTC2(CTR_MipsSubLo(particle->axis[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z].startVal, particle->axis[PARTICLE_AXIS_POS_Z].startVal) >> 6, 3);
				}
				CTR_GteLoadDelay();
				gte_rtpt_b();
				color |= PARTICLE_GPU_CODE_LINE_G2;
				if (flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_SWAP_COLORS)
				{
					u32 lineColor = particle->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal;
					packet->line.color1 = color;
					packet->line.color0AndCode = lineColor;
				}
				else
				{
					u32 lineColor = particle->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal;
					packet->line.color0AndCode = color;
					packet->line.color1 = lineColor;
				}
				CTR_WriteU32AlignedLE(&particle->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].velocity, color);
				packet->drawMode = PARTICLE_GPU_DRAWMODE_BASE | (flagsSetColor & PARTICLE_SET_COLOR_FLAG_DRAW_MODE_MASK);
				packet->pad = 0;
				CTR_PSX_STORE_COP2_WORD(&packet->line.xy0, 12);
				CTR_PSX_STORE_COP2_WORD(&packet->line.xy1, 13);
				CTR_PSX_STORE_COP2_WORD(&scratch->depth, 17);
				goto linkPrimitive;
			}
			color |= PARTICLE_GPU_CODE_POLY_FT4;
			r11r12 = 0x2000;
			r13r21 = 0;
			r22r23 = 0x1000;
			r31r32 = 0;
			r33 = 0x1000;
			if (flagsAxis & PARTICLE_AXIS_FLAG_ROT_X)
			{
				if (flagsAxis & PARTICLE_AXIS_FLAG_ROT_Y)
				{
					{
						s32 angle = particle->axis[PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X].startVal;
						sinX = CTR_ReadU32AlignedLE(&trigTable[angle & 0x3ff]);
						if (angle & 0x400)
						{
							cosX = (s16)sinX;
							sinX >>= 16;
							if (angle & 0x800)
								sinX = -sinX;
							else
								cosX = -cosX;
						}
						else
						{
							cosX = sinX >> 16;
							{
								register s32 fixed CTR_PSX_REGISTER("$4") = sinX;
								CTR_PSX_OBSERVE_VALUE(fixed);
							}
							sinX = (s16)sinX;
							if (angle & 0x800)
							{
								cosX = -cosX;
								sinX = -sinX;
							}
						}
					}
					{
						s32 angle = particle->axis[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z].startVal;
						sinY = CTR_ReadU32AlignedLE(&trigTable[angle & 0x3ff]);
						if (angle & 0x400)
						{
							cosY = (s16)sinY;
							sinY >>= 16;
							if (angle & 0x800)
								sinY = -sinY;
							else
								cosY = -cosY;
						}
						else
						{
							cosY = sinY >> 16;
							{
								register s32 fixed CTR_PSX_REGISTER("$6") = sinY;
								CTR_PSX_OBSERVE_VALUE(fixed);
							}
							sinY = (s16)sinY;
							if (angle & 0x800)
							{
								cosY = -cosY;
								sinY = -sinY;
							}
						}
					}
					if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X)
					{
						r11r12 = particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal;
						{
							if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y)
							{
								s32 cx = CTR_MipsMulLo(cosY, r11r12);
								s32 sx = CTR_MipsMulLo(sinY, r11r12);
								s32 xy = CTR_MipsMulLo(cosX, -sinY);
								s32 yy = CTR_MipsMulLo(cosY, cosX);
								r22r23 = particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal;
								{
									s32 yx = CTR_MipsMulLo(xy >> 12, r22r23);
									s32 ys = CTR_MipsMulLo(yy >> 12, r22r23);
									s32 zx = CTR_MipsMulLo(sinX, sinY);
									s32 zy = CTR_MipsMulLo(-sinX, cosY);
									s32 sy = CTR_MipsMulLo(sinX, r22r23);
									r33 = cosX & 0xffff;
									r11r12 = ((cx >> 11) & 0xffff) | ((u32)(sx >> 11) << 16);
									r31r32 = ((zx >> 12) & 0xffff) | ((u32)(zy >> 12) << 16);
									r13r21 = (u32)(yx >> 12) << 16;
									r22r23 = ((ys >> 12) & 0xffff) | ((u32)(sy >> 12) << 16);
								}
							}
							else
							{
								s32 xy, yy, zx, zy;
								s32 lowX;
								cosX = CTR_MipsMulLo(cosX, r11r12) >> 12;
								sinX = CTR_MipsMulLo(sinX, r11r12) >> 12;
								sinY = CTR_MipsMulLo(sinY, r11r12) >> 12;
								cosY = CTR_MipsMulLo(cosY, r11r12) >> 12;
								xy = CTR_MipsMulLo(cosX, -sinY);
								yy = CTR_MipsMulLo(cosY, cosX);
								zx = CTR_MipsMulLo(sinX, sinY);
								lowX = ((u32)cosY << 1) & 0xffff;
								zy = CTR_MipsMulLo(-sinX, cosY);
								r11r12 = lowX | ((u32)sinY << 17);
								r33 = cosX & 0xffff;
								r13r21 = (u32)(xy >> 12) << 16;
								r22r23 = ((yy >> 12) & 0xffff) | ((u32)sinX << 16);
								r31r32 = ((zx >> 12) & 0xffff) | ((u32)(zy >> 12) << 16);
							}
						}
					}
					else
					{
						if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y)
						{
							s32 xy = CTR_MipsMulLo(cosX, -sinY);
							s32 yy = CTR_MipsMulLo(cosY, cosX);
							r22r23 = particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal;
							{
								s32 yx = CTR_MipsMulLo(xy >> 12, r22r23);
								s32 ys = CTR_MipsMulLo(yy >> 12, r22r23);
								s32 zx = CTR_MipsMulLo(sinX, sinY);

								s32 zy = CTR_MipsMulLo(-sinX, cosY);
								s32 sy = CTR_MipsMulLo(sinX, r22r23);
								r33 = cosX & 0xffff;
								{
									u32 hi = (u32)sinY << 17;
									u32 lo = ((u32)cosY << 1) & 0xffff;
									r11r12 = lo | hi;
								}
								r31r32 = ((zx >> 12) & 0xffff) | ((u32)(zy >> 12) << 16);
								r13r21 = (u32)(yx >> 12) << 16;
								r22r23 = ((ys >> 12) & 0xffff) | ((u32)(sy >> 12) << 16);
							}
						}
						else
						{
							s32 xy = CTR_MipsMulLo(cosX, -sinY);
							s32 yy = CTR_MipsMulLo(cosY, cosX);
							s32 zx = CTR_MipsMulLo(sinX, sinY);
							s32 lowX = ((u32)cosY << 1) & 0xffff;
							s32 zy = CTR_MipsMulLo(-sinX, cosY);
							r11r12 = lowX | ((u32)sinY << 17);
							r33 = cosX & 0xffff;
							r13r21 = (u32)(xy >> 12) << 16;
							r22r23 = ((yy >> 12) & 0xffff) | ((u32)sinX << 16);
							r31r32 = ((zx >> 12) & 0xffff) | ((u32)(zy >> 12) << 16);
						}
					}
					goto matrixReady;
				}
				{
					s32 angle = particle->axis[PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X].startVal;
					sinX = CTR_ReadU32AlignedLE(&trigTable[angle & 0x3ff]);
					if (angle & 0x400)
					{
						cosX = (s16)sinX;
						sinX >>= 16;
						if (angle & 0x800)
							sinX = -sinX;
						else
							cosX = -cosX;
					}
					else
					{
						cosX = sinX >> 16;
						{
							register s32 fixed CTR_PSX_REGISTER("$4") = sinX;
							CTR_PSX_OBSERVE_VALUE(fixed);
						}
						sinX = (s16)sinX;
						if (angle & 0x800)
						{
							cosX = -cosX;
							sinX = -sinX;
						}
					}
				}
				if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X)
				{
					r11r12 = particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal;
					{
						if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y)
						{
							r22r23 = particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal;
							{
								s32 y = CTR_MipsMulLo(cosX, r22r23);
								s32 z = CTR_MipsMulLo(sinX, r22r23);
								r11r12 = ((u32)r11r12 << 1) & 0xffff;
								r31r32 = (u32)-sinX << 16;
								r33 = cosX & 0xffff;
								r22r23 = ((y >> 12) & 0xffff) | ((u32)(z >> 12) << 16);
							}
						}
						else
						{
							cosX = CTR_MipsMulLo(cosX, r11r12) >> 12;
							sinX = CTR_MipsMulLo(sinX, r11r12) >> 12;
							r11r12 = ((u32)r11r12 << 1) & 0xffff;
							r22r23 = (cosX & 0xffff) | ((u32)sinX << 16);
							r31r32 = (u32)-sinX << 16;
							r33 = cosX & 0xffff;
						}
					}
				}
				else if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y)
				{
					r22r23 = particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal;
					{
						s32 y = CTR_MipsMulLo(cosX, r22r23);
						s32 z = CTR_MipsMulLo(sinX, r22r23);
						r31r32 = (u32)-sinX << 16;
						r33 = cosX & 0xffff;
						r22r23 = ((y >> 12) & 0xffff) | ((u32)(z >> 12) << 16);
					}
				}
				else
				{
					r22r23 = (cosX & 0xffff) | ((u32)sinX << 16);
					r31r32 = (u32)-sinX << 16;
					r33 = cosX & 0xffff;
				}
				goto matrixReady;
			}
			if (flagsAxis & PARTICLE_AXIS_FLAG_ROT_Y)
			{
				{
					s32 angle = particle->axis[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z].startVal;
					sinX = CTR_ReadU32AlignedLE(&trigTable[angle & 0x3ff]);
					if (angle & 0x400)
					{
						cosX = (s16)sinX;
						sinX >>= 16;
						if (angle & 0x800)
							sinX = -sinX;
						else
							cosX = -cosX;
					}
					else
					{
						cosX = sinX >> 16;
						{
							register s32 fixed CTR_PSX_REGISTER("$4") = sinX;
							CTR_PSX_OBSERVE_VALUE(fixed);
						}
						sinX = (s16)sinX;
						if (angle & 0x800)
						{
							cosX = -cosX;
							sinX = -sinX;
						}
					}
				}
				if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X)
				{
					r11r12 = particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal;
					{
						if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y)
						{
							s32 x = CTR_MipsMulLo(cosX, r11r12);
							s32 y = CTR_MipsMulLo(sinX, r11r12);
							r22r23 = particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal;
							{
								s32 zx = CTR_MipsMulLo(-sinX, r22r23);
								s32 zy = CTR_MipsMulLo(cosX, r22r23);
								r11r12 = ((x >> 11) & 0xffff) | ((u32)(y >> 11) << 16);
								r13r21 = (u32)(zx >> 12) << 16;
								r22r23 = (zy >> 12) & 0xffff;
							}
						}
						else
						{
							cosX = CTR_MipsMulLo(cosX, r11r12) >> 12;
							sinX = CTR_MipsMulLo(sinX, r11r12) >> 12;
							r22r23 = cosX & 0xffff;
							r11r12 = (((u32)cosX << 1) & 0xffff) | ((u32)sinX << 17);
							r13r21 = (u32)-sinX << 16;
						}
					}
				}
				else if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y)
				{
					r22r23 = particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal;
					{
						s32 zx = CTR_MipsMulLo(-sinX, r22r23);
						s32 zy = CTR_MipsMulLo(cosX, r22r23);
						r11r12 = (((u32)cosX << 1) & 0xffff) | ((u32)sinX << 17);
						r13r21 = (u32)(zx >> 12) << 16;
						r22r23 = (zy >> 12) & 0xffff;
					}
				}
				else
				{
					r11r12 = (((u32)cosX << 1) & 0xffff) | ((u32)sinX << 17);
					r13r21 = (u32)-sinX << 16;
					r22r23 = cosX & 0xffff;
				}
				goto matrixReady;
			}
			if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_X)
			{
				r11r12 = (u32)particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal << 1;
				r22r23 = (s32)r11r12 >> 1;
				if (flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y)
					goto loadScaleY;
				goto matrixReady;
			}
			if (!(flagsAxis & PARTICLE_AXIS_FLAG_SCALE_Y))
				goto matrixReady;
		loadScaleY:
			r22r23 = particle->axis[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y].startVal;
		matrixReady:
		{
			register s32 m4 CTR_PSX_REGISTER("$12") = r33;
			CTC2(r11r12, 0);
			CTC2(r13r21, 1);
			CTC2(r22r23, 2);
			CTC2(r31r32, 3);
			CTC2(m4, 4);
		}
			{
				s32 width, height, halfWidth, halfHeight;
				u32 *caseTwoAddress;
				register s32 one CTR_PSX_REGISTER("$13");
				register s32 two CTR_PSX_REGISTER("$12");
				s32 three;

				s32 zero;
				CtrGpu_WriteColorCode((u8 *)(payload - 7), color);
				CtrGpu_WritePackedUVWord((u8 *)(payload - 5), CTR_ReadU32AlignedLE(&icon->texLayout.u0));
				CtrGpu_WritePackedUVWord((u8 *)(payload - 3), (CTR_ReadU32AlignedLE(&icon->texLayout.u1) & PARTICLE_TEXTURE_DRAW_MODE_MASK) |
				                                                  ((flagsSetColor & PARTICLE_SET_COLOR_FLAG_DRAW_MODE_MASK) << 16));
				CtrGpu_WritePackedUV((u8 *)(payload - 1), CTR_ReadU16AlignedLE(&icon->texLayout.u2));
				CtrGpu_WritePackedUV((u8 *)(payload + 1), CTR_ReadU16AlignedLE(&icon->texLayout.u3));
				width = icon->texLayout.u1 - icon->texLayout.u0 + 1;
				height = icon->texLayout.v2 - icon->texLayout.v0 + 1;
				halfWidth = width << 1;
				halfHeight = height << 1;
				if (flagsSetColor & PARTICLE_SET_COLOR_FLAG_LARGE_QUAD)
				{
					halfWidth = width << 4;
					halfHeight = height << 4;
				}
				r22r23 = (((u32)(-halfWidth) & 0xffff) | ((u32)(-halfHeight) << 16));
				zero = 0;
				MTC2(zero, 1);
				// NOTE(aalhendi): Trigonometry is finished here. Reusing sinX for
				// the corner loop and later OT index preserves retail's register life.
				sinX = 0;
				CTR_PSX_LOAD_IMMEDIATE(one, 1);
				two = 2;
				CTR_PSX_LOAD_IMMEDIATE(three, 3);
				for (; sinX < 4; ++sinX)
				{
					MTC2(r22r23, 0);
					CTR_GteLoadDelay();
					gte_rtps_b();
					if (sinX == one)
						goto cornerOne;
					if (sinX < 2)
					{
						if (sinX == 0)
							goto cornerZero;
						goto cornerEnd;
					}
					caseTwoAddress = prim + 6;
					if (sinX == two)
						goto cornerTwo;
					if (sinX == three)
						goto cornerThree;
					goto cornerEnd;
				cornerZero:
					r22r23 = (((u32)(halfWidth) & 0xffff) | ((u32)(-halfHeight) << 16));
					CTR_PSX_STORE_COP2_WORD(prim + 2, 14);
					{
						u32 *depthAddress;
						// NOTE(aalhendi): Keep this address calculation beside the
						// GTE store; the nonvolatile offset helper moves it.
#ifdef CTR_NATIVE
						depthAddress = (u32 *)&scratch->depth;
#else
						__asm__ volatile("addiu %0,%1,48" : "=r"(depthAddress) : "r"(scratch));
#endif
						CTR_PSX_STORE_COP2_WORD(depthAddress, 19);
					}
					goto cornerEnd;
				cornerOne:
					r22r23 = (((u32)(-halfWidth) & 0xffff) | ((u32)(halfHeight) << 16));
					{
						u32 *caseOneAddress;
#ifdef CTR_NATIVE
						caseOneAddress = prim + 4;
#else
						__asm__ volatile("addiu %0,%1,16" : "=r"(caseOneAddress) : "r"(prim));
#endif
						CTR_PSX_STORE_COP2_WORD(caseOneAddress, 14);
					}
					goto cornerEnd;
				cornerTwo:
					r22r23 = (((u32)(halfWidth) & 0xffff) | ((u32)(halfHeight) << 16));
					CTR_PSX_STORE_COP2_WORD(caseTwoAddress, 14);
					goto cornerEnd;
				cornerThree:
					CTR_PSX_STORE_COP2_WORD(payload, 14);
					goto cornerEnd;
				cornerEnd:;
				}
			}
		linkPrimitive:
		{
			u32 *ot, *otBase;
			if (idpp)
			{
				sinX = scratch->depth >> 5;
				if (sinX < (u16)idpp->depthOffset[0])
					sinX = (u16)idpp->depthOffset[0];
				if ((u16)idpp->depthOffset[1] < sinX)
					sinX = (u16)idpp->depthOffset[1];
				otBase = (u32 *)(u32)idpp->otRangeNormal;
			}
			else
			{
				sinX = (scratch->depth >> 8) + particle->otIndexOffset;
				if (sinX < 0)
					sinX = 0;
				if (sinX >= 0x400)
					sinX = 0x3ff;
				otBase = scratch->ot;
			}
			ot = otBase + sinX;
			if (flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE)
			{
				u32 link = CtrGpu_PrimToOTLink24(prim);
				payload += 7;
				*prim = *ot | PARTICLE_GPU_TAG_LENGTH_SPECIAL_LINE;
				*ot = link;
				prim += 7;
			}
			else
			{
				u32 link = CtrGpu_PrimToOTLink24(prim);
				payload += 10;
				*prim = *ot | PARTICLE_GPU_TAG_LENGTH_POLY_FT4;
				*ot = link;
				prim += 10;
			}
		}
		nextParticle:
			CTR_PSX_KEEP_VALUE(payload);
			particle = particle->next;
		} while (particle);
	}
	GAME_TRACKER->backBuffer->primMem.cursor = prim;
}

#undef Particle_LoadVector

static inline struct ParticleOscillator **Particle_OscillatorSlot(struct ParticleOscillator **table, u32 index)
{
	// NOTE(aalhendi): Keep the byte offset separate so GCC preserves retail's
	// table-base-first address calculation when the table pointer is spilled.
	u32 offset = index * sizeof(*table);

	return (struct ParticleOscillator **)((u8 *)table + offset);
}
struct Particle *Particle_Init(u32 unused, struct IconGroup *ig, struct ParticleEmitter *emSet)
{
	struct ParticleOscillator *localOsc[12];
	s32 particleType = 0;
	struct Particle *p;
	struct ParticleOscillator *osc;
	struct ParticleOscillator **oscTable;
	struct ParticleOscillator **link;
	u8 *axisOffset;
	struct ParticleOscillator **scan;
	u32 one;
	s32 flagsAxis;
	u32 flags;
	u32 axisIndex;
	s32 value;

	(void)unused;

	p = (struct Particle *)LIST_RemoveFront(&GAME_TRACKER->JitPools.particle.free);
	if (!p)
		goto done;
	flagsAxis = 0;
	GAME_TRACKER->numParticles++;
	p->ptrIconGroup = ig;
	if (ig && ig->numIcons != 0 && ig->numIcons > 0)
		p->ptrIconArray = ((struct Icon **)ICONGROUP_GETICONS(ig))[0];
	else
	{
		p->ptrIconGroup = NULL;
		p->ptrIconArray = NULL;
	}

	if (emSet && (flags = emSet->flags) != 0)
	{
		one = 1;
		oscTable = localOsc;
		do
		{
			axisIndex = (u8)emSet->initOffset;
			if (axisIndex == PARTICLE_EMITTER_INIT_FUNC_OFFSET)
			{
				if (!(flags & PARTICLE_EMITTER_FLAG_NON_FUNC_INIT_MASK))
				{
					p->funcPtr = emSet->InitTypes.FuncInit.particle_funcPtr;
					p->flagsSetColor = emSet->InitTypes.FuncInit.particle_colorFlags;
					p->framesLeftInLife = emSet->InitTypes.FuncInit.particle_lifespan;
					flagsAxis |= PARTICLE_AXIS_FLAG_FUNC_INIT;
					particleType = emSet->InitTypes.FuncInit.particle_Type;
				}
			}
			else if (flags & PARTICLE_EMITTER_FLAG_OSCILLATOR_RANDOMIZE)
			{
				if (flagsAxis & CTR_MipsSll(one, axisIndex + 16))
				{
					osc = *Particle_OscillatorSlot(oscTable, axisIndex);
					value = emSet->tail.oscillator.range.period;
					if (value)
						osc->config.range.period += MixRNG_Particles(value);
					value = emSet->tail.oscillator.range.phase;
					if (value)
						osc->config.range.phase += MixRNG_Particles(value);
					value = emSet->tail.oscillator.range.scale;
					if (value)
						osc->config.range.scale += MixRNG_Particles(value);
					value = emSet->tail.oscillator.range.offset;
					if (value)
						osc->config.range.offset += MixRNG_Particles(value);
					value = emSet->tail.oscillator.range.min;
					if (value)
						osc->config.range.min += MixRNG_Particles(value);
					value = emSet->tail.oscillator.range.max;
					if (value)
						osc->config.range.max += MixRNG_Particles(value);
				}
			}
			else if (flags & PARTICLE_EMITTER_FLAG_OSCILLATOR)
			{
				if (flagsAxis & CTR_MipsSll(one, axisIndex + 16))
					osc = *Particle_OscillatorSlot(oscTable, axisIndex);
				else
				{
					osc = (struct ParticleOscillator *)LIST_RemoveFront(&GAME_TRACKER->JitPools.oscillator.free);
					if (!osc)
						goto nextEmitter;
					*Particle_OscillatorSlot(oscTable, axisIndex) = osc;
				}
				osc->config = emSet->tail.oscillator;
				if (emSet->tail.oscillator.flags & PARTICLE_OSC_FLAG_PHASE_RELATIVE_TO_NOW)
					osc->config.range.phase -= (u16)GAME_TRACKER->frameTimer_Confetti;
				if ((emSet->tail.oscillator.flags & PARTICLE_OSC_FLAG_MODE_MASK) == PARTICLE_OSC_MODE_SEEDED_RANDOM)
					osc->config.previousValue = osc->config.range.phase;
				flagsAxis |= CTR_MipsSll(one, axisIndex + 16);
				if (!(flagsAxis & CTR_MipsSll(one, axisIndex)))
				{
					flagsAxis |= CTR_MipsSll(one, axisIndex);
					p->axis[axisIndex].startVal = 0;
					p->axis[axisIndex].velocity = 0;
					p->axis[axisIndex].accel = 0;
				}
			}
			else
			{
				value = 0;
				if (flags & PARTICLE_EMITTER_FLAG_BASE_START)
					value = emSet->InitTypes.AxisInit.baseValue.startVal;
				if (flags & PARTICLE_EMITTER_FLAG_RANDOM_START)
					value = CTR_MipsAddLo(value, MixRNG_Particles(emSet->InitTypes.AxisInit.rngSeed.startVal));
				// NOTE(aalhendi): Retail keeps the axis stride in its cursor
				// and folds the particle header offset into each field access.
				axisOffset = (u8 *)p + axisIndex * sizeof(struct ParticleAxis);
				((struct ParticleAxis *)(axisOffset + offsetof(struct Particle, axis)))->startVal = value;
				value = 0;
				if (flags & PARTICLE_EMITTER_FLAG_BASE_VELOCITY)
					value = emSet->InitTypes.AxisInit.baseValue.velocity;
				if (flags & PARTICLE_EMITTER_FLAG_RANDOM_VELOCITY)
					value = CTR_MipsAddLo(value, MixRNG_Particles(emSet->InitTypes.AxisInit.rngSeed.velocity));
				((struct ParticleAxis *)(axisOffset + offsetof(struct Particle, axis)))->velocity = value;
				value = 0;
				if (flags & PARTICLE_EMITTER_FLAG_BASE_ACCEL)
					value = emSet->InitTypes.AxisInit.baseValue.accel;
				if (flags & PARTICLE_EMITTER_FLAG_RANDOM_ACCEL)
					value = CTR_MipsAddLo(value, MixRNG_Particles(emSet->InitTypes.AxisInit.rngSeed.accel));
				((struct ParticleAxis *)(axisOffset + offsetof(struct Particle, axis)))->accel = value;
				flagsAxis |= CTR_MipsSll(one, axisIndex);
			}
		nextEmitter:
			++emSet;
			flags = emSet->flags;
		} while (flags);
	}

	// Link successful allocations in axis order, independently of emitter order.
	link = &p->oscillator;
	value = flagsAxis >> 16;
	scan = localOsc;
	while (value)
	{
		if (value & 1)
		{
			struct ParticleOscillator *next = *scan;
			*link = next;
			link = &next->next;
		}
		value >>= 1;
		++scan;
	}
	*link = NULL;
	if (!(flagsAxis & PARTICLE_AXIS_FLAG_FUNC_INIT))
	{
		p->funcPtr = NULL;
		p->flagsSetColor = 0;
		p->framesLeftInLife = 0;
		p->ptrIconArray = NULL;
		p->ptrIconGroup = NULL;
	}
	CTR_WriteU32AlignedLE(&p->flagsAxis, flagsAxis & ~PARTICLE_AXIS_FLAG_FUNC_INIT);
	if (particleType)
	{
		p->next = GAME_TRACKER->particleList_heatWarp;
		GAME_TRACKER->particleList_heatWarp = p;
	}
	else
	{
		p->next = GAME_TRACKER->particleList_ordinary;
		GAME_TRACKER->particleList_ordinary = p;
	}
	p->renderDepthLimit = 0x400;
	p->driverID = -1;
	p->otIndexOffset = 0;
	if (p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE)
	{
		u32 color;
		if (p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_KEEP_PREVIOUS)
		{
			color = PARTICLE_GPU_CODE_LINE_G2;
			if (p->flagsSetColor & PARTICLE_SET_COLOR_FLAG_SEMI_TRANSPARENT)
				color |= PARTICLE_GPU_CODE_SEMI_TRANS;
			p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal = color;
		}
		else
		{
			color = Particle_SetColors(flagsAxis, p->flagsSetColor, p) | PARTICLE_GPU_CODE_LINE_G2;
			p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal = color;
			CTR_WriteU32AlignedLE(&p->axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].velocity, color);
		}
	}
done:
	return p;
}
