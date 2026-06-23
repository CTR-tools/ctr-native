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

	// 0x14
	// one bit per axis
	u16 flagsAxis;

	// 0x16
	s16 unk16;

	// 0x18
	// Signed OT/depth adjustment for non-IDPP render lists.
	s8 otIndexOffset;

	// Driver/camera filter; -1 means all cameras.
	s8 driverID;
	s16 unk1A;

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

	/*
	    0x24: posX
	    0x2C: posY
	    0x34: posZ
	    0x3C: rotX ???
	    0x44: rotY ???
	    0x4C: scale -- yes, they mixed the order up
	    0x54: rotZ ???
	    0x5C: colorR
	    0x64: colorG
	    0x6C: colorB
	    0x74: spitTire
	*/

	// 0x24
	struct ParticleAxis axis[0xB];

	// 0x7C bytes each
};

struct ParticleEmitter
{
	// 0x0
	// flags = 0 for last emitter
	// flags = 1 for FuncInit
	// flags = 0xC0 for AxisInit
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

	// 0x14
	// this gets memcpy'd into particle,
	// & 0x40 == 1

	// possibly two more ParticleAxis structs
	char data[0x10];

	// 0x24 bytes each
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct ParticleAxis) == 8);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct ParticleOscillator) == 0x18);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct Particle) == 0x7c);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct Particle, otIndexOffset) == 0x18);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct Particle, driverID) == 0x19);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct Particle, unk1A) == 0x1a);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct ParticleEmitter) == 0x24);
