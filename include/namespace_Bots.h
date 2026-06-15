struct NavFrame
{
	// 0
	s16 pos[3];

	// 6
	u8 rot[4];

	// 0xA
	union
	{
		s16 distToNextNav[2];
		struct
		{
			// 0xA
			s16 distToNextNavXYZ;

			// 0xC
			s16 distToNextNavXZ;
		};
	};

	// 0xE - flags
	// bit 1 (0x1): TurboIncrement(normal turbo)
	// bit 2 (0x2): skid back wheels
	// bit 3 (0x4): skid front wheels
	// bits 4,5,6,7: 15 terrains in 4 bits
	// bit 8 (0x100) TurboIncrement(super turbo)
	// bit 9 (0x200) something to do with killplane?
	// 0x400: jumping
	// 0x800: drift left
	// 0x1000: drift right
	// 0x2000: echo sound
	// 0x4000: ???
	// 0x8000: instance split-line
	s16 flags;

	// 0x10
	s16 pathChangeOpcode;

	// 0x12
	// how many nodes to go back, if AI hits killplane
	u8 goBackCount;

	// 0x13 - flags
	// & 0x10 - need rampPhys
	// & 0x20 - is reflective
	// & 0x40 - run Level Instance collision
	// & 0xF - (if & 0x10) NavHeader rampPhys index
	// & 0xF - (if & 0x20) reflection variable
	// & 0xF - (if not 0x10 and not 0x20)
	// & 0x80 - tagged as "moon gravity"
	// 15 levels of alpha,
	// for darkening model in coco park tunnel)
	u8 specialBits;

	// 0x14 - size of struct
};

struct NavHeader
{
	// 0
	s16 magicNumber;

	// 2
	s16 numPoints;

	// 4
	// either int at 0x4, or s16 at 0x6
	int posY_firstNode;

	// 8
	struct NavFrame *last;

	// 0xC
	s16 rampPhys1[0x10]; // speedXZ? driver->0x5d4
	s16 rampPhys2[0x10]; // speedY? driver->0x5d0

	// 0x4C
	// struct NavFrame frame[0];
};
#define NAVHEADER_GETFRAME(x)    ((struct NavFrame *)((u32)x + sizeof(struct NavHeader)))

#define NAVFRAME_GETNEXTFRAME(x) (((struct NavFrame *)x) + 1)

_Static_assert(sizeof(struct NavFrame) == 0x14);
_Static_assert(sizeof(struct NavHeader) == 0x4c);
