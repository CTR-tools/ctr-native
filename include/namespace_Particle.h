#ifndef CTR_NATIVE_NAMESPACE_PARTICLE_H
#define CTR_NATIVE_NAMESPACE_PARTICLE_H

struct ParticleAxis
{
	int startVal;
	s16 velocity;
	s16 accel;
};

struct ParticleOscillator
{
	struct ParticleOscillator *next;
	struct ParticleOscillator *prev;
	u16 flags;
	s16 previousValue;
	u16 period;
	s16 phase;
	u16 scale;
	s16 offset;
	s16 min;
	s16 max;
};

struct ParticleOscillatorRandomRange
{
	u16 period;
	s16 phase;
	u16 scale;
	s16 offset;
	s16 min;
	s16 max;
};

struct ParticleOscillatorConfig
{
	u16 flags;
	s16 previousValue;
	union
	{
		struct
		{
			u16 period;
			s16 phase;
			u16 scale;
			s16 offset;
			s16 min;
			s16 max;
		};
		struct ParticleOscillatorRandomRange randomRange;
	};
};

enum ParticleOscillatorFlags
{
	PARTICLE_OSC_FLAG_MODE_MASK = 0x0007,
	PARTICLE_OSC_FLAG_SKIP_PREVIOUS_SUBTRACT = 0x0008,
	PARTICLE_OSC_FLAG_APPLY_TO_VELOCITY = 0x0010,
	PARTICLE_OSC_FLAG_PHASE_RELATIVE_TO_NOW = 0x0020,
};

enum ParticleOscillatorMode
{
	PARTICLE_OSC_MODE_SINE = 0,
	PARTICLE_OSC_MODE_ABS_SINE = 1,
	PARTICLE_OSC_MODE_SAW = 2,
	PARTICLE_OSC_MODE_TRIANGLE = 3,
	PARTICLE_OSC_MODE_SQUARE = 4,
	PARTICLE_OSC_MODE_RANDOM = 5,
	PARTICLE_OSC_MODE_SEEDED_RANDOM = 6,
	PARTICLE_OSC_MODE_TIMER = 7,
};

enum ParticleSetColorFlags
{
	PARTICLE_SET_COLOR_FLAG_DESTROY_ON_SCALE_EXPIRE = 0x0001,
	PARTICLE_SET_COLOR_FLAG_DESTROY_ON_COLOR_EXPIRE = 0x0002,
	PARTICLE_SET_COLOR_FLAG_DESTROY_NOW = 0x0008,
	PARTICLE_SET_COLOR_FLAG_DRAW_MODE_MASK = 0x0060,
	PARTICLE_SET_COLOR_FLAG_DRAW_MODE_40 = 0x0040,
	PARTICLE_SET_COLOR_FLAG_RED = 0x0080,
	PARTICLE_SET_COLOR_FLAG_GREEN = 0x0100,
	PARTICLE_SET_COLOR_FLAG_BLUE = 0x0200,
	PARTICLE_SET_COLOR_FLAG_SEMI_TRANSPARENT = PARTICLE_SET_COLOR_FLAG_RED,
	PARTICLE_SET_COLOR_FLAG_ICON_WRAP = PARTICLE_SET_COLOR_FLAG_GREEN,
	PARTICLE_SET_COLOR_FLAG_ICON_BOUNCE = PARTICLE_SET_COLOR_FLAG_BLUE,
	PARTICLE_SET_COLOR_FLAG_LARGE_QUAD = 0x0400,
	PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL = 0x0800,
	PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE = 0x1000,
	PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_SWAP_COLORS = 0x2000,
	PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_KEEP_PREVIOUS = 0x4000,
	PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL_IGNORE_Y = 0x8000,
};

enum ParticleEmitterFlags
{
	PARTICLE_EMITTER_FLAG_BASE_START = 0x0001,
	PARTICLE_EMITTER_FLAG_BASE_VELOCITY = 0x0002,
	PARTICLE_EMITTER_FLAG_BASE_ACCEL = 0x0004,
	PARTICLE_EMITTER_FLAG_RANDOM_START = 0x0008,
	PARTICLE_EMITTER_FLAG_RANDOM_VELOCITY = 0x0010,
	PARTICLE_EMITTER_FLAG_RANDOM_ACCEL = 0x0020,
	PARTICLE_EMITTER_FLAG_OSCILLATOR = 0x0040,
	PARTICLE_EMITTER_FLAG_OSCILLATOR_RANDOMIZE = 0x0080,
	PARTICLE_EMITTER_FLAG_NON_FUNC_INIT_MASK = 0x00c0,
};

enum ParticleEmitterInitOffset
{
	PARTICLE_EMITTER_INIT_FUNC_OFFSET = 0x0c,
};

enum ParticleAxisIndex
{
	PARTICLE_AXIS_POS_X = 0,
	PARTICLE_AXIS_POS_Y = 1,
	PARTICLE_AXIS_POS_Z = 2,
	PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X = 3,
	PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z = 4,
	PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE = 5,
	PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y = 6,
	PARTICLE_AXIS_COLOR_R = 7,
	PARTICLE_AXIS_COLOR_G = 8,
	PARTICLE_AXIS_COLOR_B = 9,
	PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR = 10,
};

enum ParticleAxisFlags
{
	PARTICLE_AXIS_FLAG_ROT_X = 0x0008,
	PARTICLE_AXIS_FLAG_ROT_Y = 0x0010,
	PARTICLE_AXIS_FLAG_SCALE_X = 0x0020,
	PARTICLE_AXIS_FLAG_SCALE_Y = 0x0040,
	PARTICLE_AXIS_FLAG_COLOR_R = 0x0080,
	PARTICLE_AXIS_FLAG_COLOR_G = 0x0100,
	PARTICLE_AXIS_FLAG_COLOR_B = 0x0200,
	PARTICLE_AXIS_FLAG_ICON_FRAME_OR_LINE_COLOR = 0x0400,
	PARTICLE_AXIS_FLAG_FUNC_INIT = 0x1000,
};

struct Particle
{
	// 0x0
	struct Particle *next;

	// 0x4
	// Active particles use this as the oscillator chain head. While a particle
	// is inside a JitPool free list, the same word is the list Item prev field.
	struct ParticleOscillator *oscillator;

	// 0x8
	struct Icon *ptrIconArray;

	// 0xC
	struct IconGroup *ptrIconGroup;

	// 0x10 (s16)
	s16 framesLeftInLife;

	// 0x12 (s16)
	u16 flagsSetColor;

	union
	{
		// 0x14
		u32 flagsAxisWord;
		struct
		{
			// 0x14
			// one bit per initialized axis
			u16 flagsAxis;

			// 0x16
			// one bit per axis with an oscillator chain node
			u16 flagsOscillatorAxis;
		};
	};

	// 0x18
	// Signed OT/depth adjustment for non-IDPP render lists.
	s8 otIndexOffset;

	// Driver/camera filter; -1 means all cameras.
	s8 driverID;

	// Render culls when transformed depth exceeds this value shifted left by 2.
	s16 renderDepthLimit;

	// 0x1C
	void *funcPtr;

	// 0x20
	union
	{
		// used by VehEmitter
		struct Instance *driverInst;

		// used by plant SpitTire
		struct Instance *plantInst;

		// used for potion shatter
		int modelID;
	};

	// 0x24
	struct ParticleAxis axis[0xB];

	// 0x7C bytes each
};

struct ParticleEmitter
{
	// 0x0
	// 0 terminates the emitter list. At initOffset 0x0c, flags without
	// oscillator bits select FuncInit; otherwise low bits select AxisInit
	// base/random start, velocity, and acceleration fields.
	u16 flags;

	// 0x2
	// determines which axis is initialized
	s16 initOffset;

	union
	{
		struct
		{
			// 0x4
			void *particle_funcPtr;

			// 0x8
			// flags, passed to SetColors
			u16 particle_colorFlags;

			// 0xA
			s16 particle_lifespan;

			// 0xC
			//(ordinary, or heatWarp)
			int particle_Type;

			// 0x10
			int emptyFiller;

			// 0x14
		} FuncInit;

		struct
		{
			// 0x4
			struct ParticleAxis baseValue;

			// 0xC
			struct ParticleAxis rngSeed;

			// 0x14
		} AxisInit;

	} InitTypes;

	union
	{
		// 0x14
		char data[0x10];

		// 0x14
		// Copied into ParticleOscillator::flags when PARTICLE_EMITTER_FLAG_OSCILLATOR is set.
		struct ParticleOscillatorConfig oscillator;
	};

	// 0x24 bytes each
};

CTR_STATIC_ASSERT(sizeof(struct ParticleAxis) == 8);
CTR_STATIC_ASSERT(sizeof(struct ParticleOscillator) == 0x18);
CTR_STATIC_ASSERT(sizeof(struct ParticleOscillatorRandomRange) == 0x0c);
CTR_STATIC_ASSERT(sizeof(struct ParticleOscillatorConfig) == 0x10);
CTR_STATIC_ASSERT(PARTICLE_OSC_FLAG_MODE_MASK == 0x0007);
CTR_STATIC_ASSERT(PARTICLE_OSC_FLAG_SKIP_PREVIOUS_SUBTRACT == 0x0008);
CTR_STATIC_ASSERT(PARTICLE_OSC_FLAG_APPLY_TO_VELOCITY == 0x0010);
CTR_STATIC_ASSERT(PARTICLE_OSC_FLAG_PHASE_RELATIVE_TO_NOW == 0x0020);
CTR_STATIC_ASSERT(PARTICLE_OSC_MODE_SINE == 0);
CTR_STATIC_ASSERT(PARTICLE_OSC_MODE_ABS_SINE == 1);
CTR_STATIC_ASSERT(PARTICLE_OSC_MODE_SAW == 2);
CTR_STATIC_ASSERT(PARTICLE_OSC_MODE_TRIANGLE == 3);
CTR_STATIC_ASSERT(PARTICLE_OSC_MODE_SQUARE == 4);
CTR_STATIC_ASSERT(PARTICLE_OSC_MODE_RANDOM == 5);
CTR_STATIC_ASSERT(PARTICLE_OSC_MODE_SEEDED_RANDOM == 6);
CTR_STATIC_ASSERT(PARTICLE_OSC_MODE_TIMER == 7);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_DESTROY_ON_SCALE_EXPIRE == 0x0001);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_DESTROY_ON_COLOR_EXPIRE == 0x0002);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_DESTROY_NOW == 0x0008);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_DRAW_MODE_MASK == 0x0060);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_DRAW_MODE_40 == 0x0040);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_RED == 0x0080);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_GREEN == 0x0100);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_BLUE == 0x0200);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_SEMI_TRANSPARENT == 0x0080);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_ICON_WRAP == 0x0100);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_ICON_BOUNCE == 0x0200);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_LARGE_QUAD == 0x0400);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL == 0x0800);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE == 0x1000);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_SWAP_COLORS == 0x2000);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE_KEEP_PREVIOUS == 0x4000);
CTR_STATIC_ASSERT(PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL_IGNORE_Y == 0x8000);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_FLAG_BASE_START == 0x0001);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_FLAG_BASE_VELOCITY == 0x0002);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_FLAG_BASE_ACCEL == 0x0004);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_FLAG_RANDOM_START == 0x0008);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_FLAG_RANDOM_VELOCITY == 0x0010);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_FLAG_RANDOM_ACCEL == 0x0020);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_FLAG_OSCILLATOR == 0x0040);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_FLAG_OSCILLATOR_RANDOMIZE == 0x0080);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_FLAG_NON_FUNC_INIT_MASK == 0x00c0);
CTR_STATIC_ASSERT(PARTICLE_EMITTER_INIT_FUNC_OFFSET == 0x0c);
CTR_STATIC_ASSERT(PARTICLE_AXIS_POS_X == 0);
CTR_STATIC_ASSERT(PARTICLE_AXIS_POS_Y == 1);
CTR_STATIC_ASSERT(PARTICLE_AXIS_POS_Z == 2);
CTR_STATIC_ASSERT(PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X == 3);
CTR_STATIC_ASSERT(PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z == 4);
CTR_STATIC_ASSERT(PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE == 5);
CTR_STATIC_ASSERT(PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y == 6);
CTR_STATIC_ASSERT(PARTICLE_AXIS_COLOR_R == 7);
CTR_STATIC_ASSERT(PARTICLE_AXIS_COLOR_G == 8);
CTR_STATIC_ASSERT(PARTICLE_AXIS_COLOR_B == 9);
CTR_STATIC_ASSERT(PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR == 10);
CTR_STATIC_ASSERT(PARTICLE_AXIS_FLAG_ROT_X == 0x0008);
CTR_STATIC_ASSERT(PARTICLE_AXIS_FLAG_ROT_Y == 0x0010);
CTR_STATIC_ASSERT(PARTICLE_AXIS_FLAG_SCALE_X == 0x0020);
CTR_STATIC_ASSERT(PARTICLE_AXIS_FLAG_SCALE_Y == 0x0040);
CTR_STATIC_ASSERT(PARTICLE_AXIS_FLAG_COLOR_R == 0x0080);
CTR_STATIC_ASSERT(PARTICLE_AXIS_FLAG_COLOR_G == 0x0100);
CTR_STATIC_ASSERT(PARTICLE_AXIS_FLAG_COLOR_B == 0x0200);
CTR_STATIC_ASSERT(PARTICLE_AXIS_FLAG_ICON_FRAME_OR_LINE_COLOR == 0x0400);
CTR_STATIC_ASSERT(PARTICLE_AXIS_FLAG_FUNC_INIT == 0x1000);
CTR_STATIC_ASSERT(offsetof(struct ParticleOscillatorConfig, flags) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct ParticleOscillatorConfig, previousValue) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct ParticleOscillatorConfig, period) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct ParticleOscillatorConfig, phase) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct ParticleOscillatorConfig, scale) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct ParticleOscillatorConfig, offset) == 0xa);
CTR_STATIC_ASSERT(offsetof(struct ParticleOscillatorConfig, min) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct ParticleOscillatorConfig, max) == 0xe);
CTR_STATIC_ASSERT(offsetof(struct ParticleOscillatorConfig, randomRange) == 0x4);
CTR_STATIC_ASSERT(sizeof(struct Particle) == 0x7c);
CTR_STATIC_ASSERT(offsetof(struct Particle, flagsAxisWord) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct Particle, flagsAxis) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct Particle, flagsOscillatorAxis) == 0x16);
CTR_STATIC_ASSERT(offsetof(struct Particle, otIndexOffset) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct Particle, driverID) == 0x19);
CTR_STATIC_ASSERT(offsetof(struct Particle, renderDepthLimit) == 0x1a);
CTR_STATIC_ASSERT(sizeof(struct ParticleEmitter) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct ParticleEmitter, flags) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct ParticleEmitter, initOffset) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct ParticleEmitter, InitTypes) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct ParticleEmitter, data) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct ParticleEmitter, oscillator) == 0x14);

#endif
