#ifndef CTR_NATIVE_REGIONSEXE_H
#define CTR_NATIVE_REGIONSEXE_H


enum
{
	MATRIX_ND_BAKED_MATRIX_OFFSET = 0x8,
};

struct MatrixND
{
	s16 m[3][3];
	s16 extraShort;
	int t[3];
};

// NOTE(aalhendi): Retail stores vehicle animation source values in the same
// 0x20-byte words later consumed as overlapping matrices.
typedef struct CTR_MAY_ALIAS MatrixNDAuthored
{
	SVec3Slot bakedOffset;
	SVec3Slot authoredRot;
	SVec3Slot authoredScale;
	s32 authoredPad[2];
} MatrixNDAuthored;

typedef struct CTR_MAY_ALIAS MatrixNDOverlapMatrix
{
	s16 m[3][3];
	Vec3 t;
} MatrixNDOverlapMatrix;

CTR_STATIC_ASSERT(sizeof(MatrixNDOverlapMatrix) == sizeof(MATRIX));

static inline MatrixNDOverlapMatrix *MatrixND_GetOverlapMatrix(struct MatrixND *matrix)
{
	return (MatrixNDOverlapMatrix *)((u8 *)matrix + MATRIX_ND_BAKED_MATRIX_OFFSET);
}

static inline MatrixNDAuthored *MatrixND_GetAuthored(struct MatrixND *matrix)
{
	return (MatrixNDAuthored *)matrix;
}

enum
{
	BAKED_GTE_MATRIX_NONE = 0,
	BAKED_GTE_MATRIX_WHEELIE_START = 1,
	BAKED_GTE_MATRIX_WHEELIE_HOLD = 2,
	BAKED_GTE_MATRIX_WHEELIE_RECOVER = 3,
	BAKED_GTE_MATRIX_CRASH_FALL = 4,
	BAKED_GTE_MATRIX_SQUISH_RECOVER = 5,
	BAKED_GTE_MATRIX_BLASTED = 6,
	BAKED_GTE_MATRIX_JUMP_BASE = 7,
	BAKED_GTE_MATRIX_JUMP_OXIDE = BAKED_GTE_MATRIX_JUMP_BASE,
	BAKED_GTE_MATRIX_COUNT = 0x14,
};

CTR_STATIC_ASSERT(sizeof(struct MatrixND) == 0x20);
CTR_STATIC_ASSERT(sizeof(MatrixNDAuthored) == 0x20);
CTR_STATIC_ASSERT(offsetof(MatrixNDAuthored, bakedOffset) == 0x0);
CTR_STATIC_ASSERT(offsetof(MatrixNDAuthored, authoredRot) == 0x8);
CTR_STATIC_ASSERT(offsetof(MatrixNDAuthored, authoredScale) == 0x10);
CTR_STATIC_ASSERT(MATRIX_ND_BAKED_MATRIX_OFFSET == offsetof(MatrixNDAuthored, authoredRot));

struct SoundFadeInput
{
	int unk;
	int desiredVolume;
	int currentVolume;
	int soundID_soundCount;
};

CTR_STATIC_ASSERT(sizeof(struct SoundFadeInput) == 0x10);

typedef union DriverModelExtraSlot
{
	void *fileBase;
	struct Model *model;
} DriverModelExtraSlot;

CTR_STATIC_ASSERT(sizeof(DriverModelExtraSlot) == sizeof(void *));
CTR_STATIC_ASSERT(offsetof(DriverModelExtraSlot, fileBase) == 0x0);
CTR_STATIC_ASSERT(offsetof(DriverModelExtraSlot, model) == 0x0);

enum
{
	SCRUB_FLAG_APPLY_IMPACT = 0x1,
	SCRUB_FLAG_SLAM_ON_HARD_IMPACT = 0x2,
	SCRUB_FLAG_SKIP_WALL_RUB_TIMER = 0x4,
	SCRUB_FLAG_KEEP_RESERVES = 0x8,
};
typedef u32 ScrubFlags;

enum
{
	TERRAIN_FLAG_RAISE_GROUND_OFFSET = 0x1,
	TERRAIN_FLAG_ACCEL_WHILE_REVERSE_SLIDING = 0x4,
	TERRAIN_FLAG_FORCE_SKIDMARKS = 0x8,
	TERRAIN_FLAG_SKIP_TURN_ASSIST = 0x10,
	TERRAIN_FLAG_ONESHOT_GROUND_SOUND = 0x20,
	TERRAIN_FLAG_LANDING_SPARKS = 0x40,
	TERRAIN_FLAG_MUD_PHYSICS = 0x80,
	TERRAIN_FLAG_SIDESLIP_FRICTION = 0x100,
};
typedef u32 TerrainFlags;

enum
{
	TERRAIN_BOT_FLAG_DECEL_TO_TARGET_SPEED = 0x80,
};
typedef u16 TerrainBotFlags;

union TerrainBotData
{
	u16 raw[4];
	struct
	{
		u16 unk_0x34_0;
		TerrainBotFlags speedFlags;
		s16 targetSpeedScale;
		s16 accelerationScale;
	} fields;
};

struct Scrub
{
	// see FUN_80020c58

	// 0x0
	char *name;

	// 0x4
	ScrubFlags flags;

	// 0x8
	int speedLimit;

	// 0xC
	// given to trig table
	int impactAngle;

	// last valid index is 6, so 7 elements
};

struct Terrain
{
	// 0
	char *name;

	// 4
	TerrainFlags flags;

	// 0x8
	// 0x100 is neutral
	int speedMultiplier;

	// 0xC
	// if 0, driver will slow down until completely stuck
	int slowUntilSpeed;

	// 0x10
	// 0% ice, 100% road
	int counterSteerRatio;

	// 0x14
	// Scales body lean from turn force; 0x100 is neutral.
	int turnLeanScale;

	// 0x18
	struct ParticleEmitter *em_OddFrame;

	// 0x1C
	struct ParticleEmitter *em_EvenFrame;

	// 0x20
	// Scales local-X/Z ground friction; 0x100 is neutral.
	int groundFrictionScale;

	// 0x24
	// Scales turn-angle integration; 0x100 is neutral.
	int turnAngleScale;

	// 0x28
	// Scales rotationSpinRate response; 0x100 is neutral.
	int turnResponseScale;

	// 0x2c - 0x2f - vibration?
	char vibrationData[4];

	// 0x30
	s16 skidSound;

	// 0x32 sound?
	s16 sound;

	// 0x34
	union TerrainBotData bot;

	// 0x3C
	// BOTS-only speed decay scale; 0x100 is neutral.
	s16 botFrictionScale;

	// 0x3E
	s16 padding_0x3e;
};

struct MetaDataLEV
{
	// 0x0
	s16 hubID;
	s16 padding;
	// 0 - gemstone
	// 1 - n sanity
	// 2 - lost ruin
	// 3 - glacier
	// 4 - citadel
	// 0xffff - does not belong

	// 0x4
	// debug name of level
	// "hub1", "hub2", etc
	char *name_Debug;

	// 0x8
	// lng index of level,
	// "Gem Stone Valley", "N Sanity Beach", etc,
	// that is, if it exists for this LEV
	s16 name_LNG;
	s16 padding_0xa;

	// 0xC
	// this is the amount of time you need
	// to beat, to open N Tropy's Ghost
	int timeTrial;

	// 0x10
	// in adventure arena
	s16 numTrophiesToOpen;

	// 0x12
	// 0 - red
	// 1 - green
	// 2 - blue
	// 3 - yellow
	// 4 - purple
	s16 ctrTokenGroupID;

	// 0x14
	// 0x0A - Roo on Roo's Tubes,
	// 0xFF - Nobody (most tracks)
	s16 characterID_Boss;

	// 0x16
	// N Tropy calls it "Speed Champion"
	s16 characterID_Champion;
};

struct MetaDataMODEL
{
	// debug
	char *name;

	// Level Instance Birth
	// callback after converting
	// Lev InstDef to Lev Instance
	void (*LInB)(struct Instance *inst);

	// Level Instance Collision
	// callback after detecting
	// Lev BSP hitbox collision
	int (*LInC)(struct Instance *i, struct Thread *t, struct ScratchpadStruct *sps);
};

enum BossWeaponMetaThrowFlag
{
	BOSS_WEAPON_THROW = 2,
	BOSS_WEAPON_NORMAL = 3,
};

enum BossWeaponMetaType
{
	BOSS_WEAPON_NONE = 0xf,
	BOSS_WEAPON_ENCODED_TNT = 0x64,
	BOSS_WEAPON_ENCODED_BOMB = 0x65,
	BOSS_WEAPON_ENCODED_POTION = 0x66,
};

enum BossWeaponMetaJuiceFlag
{
	BOSS_WEAPON_JUICED = 0x1,
	BOSS_WEAPON_RANDOM_JUICE = 0x2,
};

struct MetaDataBOSS
{
	// 0x0
	// where this section starts
	u8 trackCheckpoint;

	// 2=throw, 3=normal
	u8 throwFlag;

	// May be a direct pickup ID, or 0x64=tnt, 0x65=bomb, 0x66=potion, 0xf=none
	u8 weaponType;
	u8 pathChangeDisabled;

	// 0x4
	u16 weaponCooldown;

	// 0x6
	// (0=no juice) (1=juice) (2=random)
	u16 juiceFlag;
};

// starts at 0x80086d84
struct MetaDataCHAR
{
	// 0
	// "crash", "pen"
	// only used for time trial ghost save data in final game
	char *name_Debug;

	// 4
	// "Crash Bandicoot", "Penta Penguin"
	// for character selection
	s16 name_LNG_long;

	// 6
	// "Crash", "Penguin"
	// for default high scores
	s16 name_LNG_short;

	// 8
	// index in ptrIcons
	s16 iconID;

	// 0xA
	s16 padding_0xa;

	// 0xC
	// Can be 0, 1, 2, or 3; changes engine sound
	int engineID;
};

// always starts at address 0x80010000,
// which is 0x800 bytes into the EXE file
struct rData
{
	// 80010000
	char s_asphalt2_thisAppearsTwice[0xC];

	// 8001000C
	char s_robotcar[0xC];

	// 80010018
	// used in the following funcs:
	// 80016b5c
	// 8001758c
	// 800177a0
	// 8001b678
	void *jumpPointers1[0x2D];

	// 800100CC
	char s_STATIC_GNORMALZ[0x10];

	// 800100DC
	char s_STATIC_INTROSPARKS[0x14];

	// 800100F0
	char s_STATIC_OXIDESPEAKER[0x14];

	// 80010104
	char s_STATIC_JLOGO_FLAG[0x14];

	// 80010118
	char s_STATIC_OXIDECAMEND02[0x18];

	// 80010130
	char s_STATIC_OXIDECAMEND[0x14];

	// 80010144
	char s_STATIC_ENDOXIDELILSHIP[0x18];

	// 8001015C
	char s_STATIC_ENDOXIDEBIGSHIP[0x18];

	// 80010174
	char s_STATIC_ENDIGNOXIDE02[0x18];

	// 8001018C
	char s_STATIC_ENDINGOXIDE[0x14];

	// 800101A0
	char s_STATIC_ENDFLASH[0x10];

	// 800101B0
	char s_STATIC_ENDDOORS[0x10];

	// 800101C0
	char s_STATIC_PURASELECT[0x14];

	// 800101D4
	char s_STATIC_POLARSELECT[0x14];

	// 800101E8
	char s_STATIC_DINGOSELECT[0x14];

	// 800101FC
	char s_STATIC_NGINSELECT[0x14];

	// 80010210
	char s_STATIC_COCOSELECT[0x14];

	// 80010224
	char s_STATIC_TINYSELECT[0x14];

	// 80010238
	char s_STATIC_CORTEXSELECT[0x14];

	// 8001024C
	char s_STATIC_CRASHSELECT[0x14];

	// 80010260
	char s_STATIC_INTRODOORS[0x14];

	// 80010274
	char s_STATIC_INTROFLASH[0x14];

	// 80010288
	char s_NDI_BOX_LIDD[0x10];

	// 80010298
	char s_NDI_BOX_LIDC[0x10];

	// 800102A8
	char s_NDI_BOX_LIDB[0x10];

	// 800102B8
	char s_NDI_KART_7[0xC];

	// 800102C4
	char s_NDI_KART_6[0xC];

	// 800102D0
	char s_NDI_KART_5[0xC];

	// 800102DC
	char s_NDI_KART_4[0xC];

	// 800102E8
	char s_NDI_KART_3[0xC];

	// 800102F4
	char s_NDI_KART_2[0xC];

	// 80010300
	char s_NDI_KART_1[0xC];

	// 8001030C
	char s_NDI_KART_0[0xC];

	// 80010318
	char s_NDI_BOX_PARTICLES_01[0x18];

	// 80010330
	char s_NDI_BOX_LID2[0x10];

	// 80010340
	char s_NDI_BOX_LID[0xC];

	// 8001034C
	char s_NDI_BOX_GLOW[0x10];

	// 8001035C
	char s_NDI_BOX_CODE[0x10];

	// 8001036C
	char s_NDI_BOX_BOX_03[0x10];

	// 8001037C
	char s_NDI_BOX_BOX_02A[0x10];

	// 8001038C
	char s_NDI_BOX_BOX_02_FRONT[0x18];

	// 800103A4
	char s_NDI_BOX_BOX_02_BOTTOM[0x18];

	// 800103BC
	char s_NDI_BOX_BOX_02[0x10];

	// 800103CC
	char s_NDI_BOX_BOX_01[0x10];

	// 800103DC
	char s_STATIC_DOOR3[0x10];

	// 800103EC
	char s_STATIC_UNUSED03[0x10];

	// 800103FC
	char s_STATIC_UKAMOUTH[0x10];

	// 8001040C
	char s_STATIC_UKABIG[0x10];

	// 8001041C
	char s_STATIC_AKUBIG[0x10];

	// 8001042C
	char s_STATIC_TOMBSTONE[0x14];

	// 80010440
	char s_STATIC_DINGOFIRE[0x14];

	// 80010454
	char s_STATIC_AKUMOUTH[0x10];

	// 80010464
	char s_STATIC_OXIDEHEAD[0x14];

	// 80010478
	char s_STATIC_JOEHEAD[0x10];

	// 80010488
	char s_STATIC_ROOHEAD[0x10];

	// 80010498
	char s_STATIC_PAPUHEAD[0x10];

	// 800104A8
	char s_STATIC_PINHEAD[0x10];

	// 800104B8
	char s_STATIC_PODIUM[0x10];

	// 800104C8
	char s_STATIC_DOOR2[0x10];

	// 800104D8
	char s_STATIC_STARTBANNERWAVE[0x18];

	// 800104F0
	char s_STATIC_INTROOXIDEBODY[0x18];

	// 80010508
	char s_STATIC_INTROOXIDECAM[0x18];

	// 80010520
	char s_STATIC_OXIDELILSHIP[0x14];

	// 80010534
	char s_STATIC_PRTHINGINTRO[0x14];

	// 80010548
	char s_STATIC_PPOINTTHINGINTRO[0x18];

	// 80010560
	char s_STATIC_SIMPLEOBJINTRO[0x18];

	// 80010578
	char s_STATIC_DINGOKARTINTRO[0x18];

	// 80010590
	char s_STATIC_TINYKARTINTRO[0x18];

	// 800105A8
	char s_STATIC_SIMPLEKARTINTRO[0x18];

	// 800105C0
	char s_STATIC_OXIDEINTRO[0x14];

	// 800105D4
	char s_STATIC_DINGOINTRO[0x14];

	// 800105E8
	char s_STATIC_POLARINTRO[0x14];

	// 800105FC
	char s_STATIC_TINYINTRO[0x14];

	// 80010610
	char s_STATIC_CORTEXINTRO[0x14];

	// 80010624
	char s_STATIC_COCOINTRO[0x14];

	// 80010638
	char s_STATIC_CRASHINTRO[0x14];

	// 8001064C
	char s_STATIC_R[0xC];

	// 80010658
	char s_STATIC_T[0xC];

	// 80010664
	char s_STATIC_C[0xC];

	// 80010670
	char s_STATIC_TAWNA4[0x10];

	// 80010680
	char s_STATIC_TAWNA3[0x10];

	// 80010690
	char s_STATIC_TAWNA2[0x10];

	// 800106A0
	char s_STATIC_TAWNA1[0x10];

	// 800106B0
	char s_STATIC_GARAGETOP[0x14];

	// 800106C4
	char s_STATIC_OXIDEDANCE[0x14];

	// 800106D8
	char s_STATIC_FAKEDANCE[0x14];

	// 800106EC
	char s_STATIC_PENDANCE[0x10];

	// 800106FC
	char s_STATIC_NTROPYDANCE[0x14];

	// 80010710
	char s_STATIC_JOEDANCE[0x10];

	// 80010720
	char s_STATIC_ROODANCE[0x10];

	// 80010730
	char s_STATIC_PAPUDANCE[0x14];

	// 80010744
	char s_STATIC_PINSTRIPEDANCE[0x18];

	// 8001075C
	char s_STATIC_PURADANCE[0x14];

	// 80010770
	char s_STATIC_POLARDANCE[0x14];

	// 80010784
	char s_STATIC_DINGODANCE[0x14];

	// 80010798
	char s_STATIC_NGINDANCE[0x14];

	// 800107AC
	char s_STATIC_COCODANCE[0x14];

	// 800107C0
	char s_STATIC_TINYDANCE[0x14];

	// 800107D4
	char s_STATIC_CORTEXDANCE[0x14];

	// 800107E8
	char s_STATIC_CRASHDANCE[0x14];

	// 800107FC
	char s_STATIC_TOKEN[0x10];

	// 8001080C
	char s_STATIC_BOTTOMRING[0x14];

	// 80010820
	char s_STATIC_BEAM[0xC];

	// 8001082C
	char s_STATIC_DOOR[0xC];

	// 80010838
	char s_STATIC_SUBTRACT[0x10];

	// 80010848
	char s_STATIC_SCAN[0xC];

	// 80010854
	char s_STATIC_OXIDEGARAGE[0x14];

	// 80010868
	char s_STATIC_JOEGARAGE[0x14];

	// 8001087c
	char s_STATIC_ROOGARAGE[0x14];

	// 80010890
	char s_STATIC_PAPUGARAGE[0x14];

	// 800108a4
	char s_STATIC_PINGARAGE[0x14];

	// 800108b8
	char s_STATIC_SAVEOBJ[0x10];

	// 800108c8
	char s_STATIC_STARTTEXT[0x14];

	// 800108dc
	char s_STATIC_TEETH[0x10];

	// 800108ec
	char s_STATIC_BIGX[0xC];

	// 800108f8
	char s_STATIC_BIG9[0xC];

	// 80010904
	char s_STATIC_BIG0[0xC];

	// 80010910
	char s_STATIC_WARPPAD[0x10];

	// 80010920
	char s_STATIC_BANNER[0x10];

	// 80010930
	char s_STATIC_CTR[0xC];

	// 8001093c
	char s_STATIC_RINGBOTTOM[0x14];

	// 80010950
	char s_STATIC_RINGTOP[0x10];

	// 80010960
	char s_STATIC_INTRO_CRASH[0x14];

	// 80010974
	char s_STATIC_INTRO_TROPHY[0x14];

	// 80010988
	char s_STATIC_TIME_CRATE_03[0x18];

	// 800109a0
	char s_STATIC_TIME_CRATE_02[0x18];

	// 800109b8
	char s_STATIC_KEY[0xC];

	// 800109c4
	char s_STATIC_TROPHY[0x10];

	// 800109d4
	char s_STATIC_RELIC[0x10];

	// 800109e4
	char s_STATIC_CRYSTAL[0x10];

	// 800109f4
	char s_STATIC_GEM[0xC];

	// 80010a00
	char s_DYNAMIC_SHIELD_GREEN[0x18];

	// 80010a18
	char s_DYNAMIC_HIGHLIGHT[0x14];

	// 80010a2c
	char s_STATIC_TIME_CRATE_01[0x18];

	// 80010a44
	char s_STATIC_CASTLE_SIGN[0x14];

	// 80010a58
	char s_DYNAMIC_SHIELD_DARK[0x14];

	// 80010a6c
	char s_STATIC_VENT[0xC];

	// 80010a78
	char s_STATIC_PIPE2[0x10];

	// 80010a88
	char s_STATIC_PIPE1[0x10];

	// 80010a98
	char s_DYNAMIC_SHIELD[0x10];

	// 80010aa8
	char s_DYNAMIC_DRUM[0x10];

	// 80010ab8
	char s_DYNAMIC_FIREBALL[0x14];

	// 80010acc
	char s_DYNAMIC_SPIDERSHADOW[0x18];

	// 80010ae4
	char s_DYNAMIC_SPIDER[0x10];

	// 80010af4
	char s_DYNAMIC_TURTLE[0x10];

	// 80010b04
	char s_DYNAMIC_SKUNK[0x10];

	// 80010b14
	char s_DYNAMIC_VONLABASS[0x14];

	// 80010b28
	char s_DYNAMIC_BARREL[0x10];

	// 80010b38
	char s_DYNAMIC_ORCA[0x10];

	// 80010b48
	char s_DYNAMIC_SEAL[0x10];

	// 80010b58
	char s_DYNAMIC_GHOST[0x10];

	// 80010b68
	char s_LENSFLARE[0xC];

	// 80010b74
	char s_STATIC_BEAKERBREAK_GREEN[0x1c];

	// 80010b90
	char s_STATIC_BEAKERBREAK_RED[0x18];

	// 80010ba8
	char s_STATIC_BEAKER_GREEN[0x14];

	// 80010bbc
	char s_STATIC_BEAKER_RED[0x14];

	// 80010bd0
	char s_STATIC_SHOCKWAVE_GREEN[0x18];

	// 80010be8
	char s_STATIC_SHOCKWAVE_RED[0x18];

	// 80010c00
	char s_STATIC_WAKE[0xC];

	// 80010c0c
	char s_STATIC_CLOUD[0x10];

	// 80010c1c
	char s_STATIC_SHIELD[0x10];

	// 80010c2c
	char s_STATIC_UKABEAM[0x10];

	// 80010c3c
	char s_DYNAMIC_ROBOT_CAR[0x14];

	// 80010c50
	char s_STATIC_AKUBEAM[0x10];

	// 80010c60
	char s_STATIC_BEAKERBREAK[0x14];

	// 80010c74
	char s_STATIC_BEAKER[0x10];

	// 80010c84
	char s_DYNAMIC_BOMB[0x10];

	// 80010c94
	char s_STATIC_UKAUKA[0x10];

	// 80010ca4
	char s_STATIC_AKUAKU[0x10];

	// 80010cb4
	char s_STATIC_BIG1[0xC];

	// 80010cc0
	char s_STATIC_FRUITDISP[0x14];

	// 80010cd4
	char s_DYNAMIC_WARPBALL[0x14];

	// 80010ce8
	char s_STATIC_SHOCKWAVE[0x14];

	// 80010cfc
	char s_STATIC_TURBO_EFFECT_DARK[0x1C];

	// 80010d18
	char s_STATIC_TURBO_EFFECT7[0x18];

	// 80010d30
	char s_STATIC_TURBO_EFFECT6[0x18];

	// 80010d48
	char s_STATIC_TURBO_EFFECT5[0x18];

	// 80010d60
	char s_STATIC_TURBO_EFFECT4[0x18];

	// 80010d78
	char s_STATIC_TURBO_EFFECT3[0x18];

	// 80010d90
	char s_STATIC_TURBO_EFFECT2[0x18];

	// 80010da8
	char s_STATIC_TURBO_EFFECT1[0x18];

	// 80010dc0
	char s_STATIC_TURBO_EFFECT[0x14];

	// 80010dd4
	char s_STATIC_WARPDUST[0x14];

	// 80010de8
	char s_DYNAMIC_BIGROCKET[0x14];

	// 80010dfc
	char s_DYNAMIC_ROCKET[0x10];

	// 80010e0c
	char s_STATIC_CRATE_NITRO[0x14];

	// 80010e20
	char s_STATIC_CRATE_TNT[0x14];

	// 80010e34
	char s_STATIC_CRATE_EXPLOSION[0x18];

	// 80010e4c
	char s_ARMADILLO[0xc];

	// 80010e58
	char s_FINISH_LINE[0xc];

	// 80010e64
	char s_TEMP_SNOWBALL[0x10];

	// 80010e74
	char s_DYNAMIC_MINE_CART[0x14];

	// 80010e88
	char s_DYNAMIC_SNOWBALL[0x14];

	// 80010e9c
	char s_DYNAMIC_SPLINE_FOLLOWER[0x18];

	// 80010eb4
	char s_DYNAMIC_INVICIBILITY[0x18];

	// 80010ecc
	char s_DYNAMIC_POISON[0x10];

	// 80010edc
	char s_DYNAMIC_EXPLOSIVE_CRATE[0x18];

	// 80010ef4
	char s_DYNAMIC_MISSILE[0x10];

	// 80010f04
	char s_DYNAMIC_LARGE_BOMB[0x14];

	// 80010f18
	char s_DYNAMIC_SMALL_BOMB[0x14];

	// 80010f2C
	char s_DYNAMIC_PLAYER[0x10];

	// 80010f3C
	char s_START_LINE[0xC];

	// 80010f48
	char s_PIRANHA_PLANT[0x10];

	// 80010f58
	char s_FLAME_JET[0xC];

	// 80010f64
	char s_PU_TURBO[0xC];

	// 80010f70
	char s_PU_CLOCK[0xC];

	// 80010f7C
	char s_PU_SPRING[0xC];

	// 80010f88
	char s_PU_INVINCIBILITY[0x14];

	// 80010f9C
	char s_PU_SHIELD[0xC];

	// 80010fa8
	char s_PU_POISON[0xC];

	// 80010fb4
	char s_PU_TIME_CRATE_3[0x10];

	// 80010fc4
	char s_PU_TIME_CRATE_2[0x10];

	// 80010fd4
	char s_PU_TIME_CRATE_1[0x10];

	// 80010fe4
	char s_PU_RANDOM_CRATE[0x10];

	// 80010ff4
	char s_PU_FRUIT_CRATE[0x10];

	// 80011004
	char s_PU_EXPLOSIVE_CRATE[0x14];

	// 80011018
	char s_PU_MISSILE[0xC];

	// 80011024
	char s_PU_LARGE_BOMB[0x10];

	// 80011034
	char s_PU_SMALL_BOMB[0x10];

	// 80011044
	char s_PU_WUMPA_FRUIT[0x10];

	// 80011054
	char s_ANIMATE_IF_HIT[0x10];

	// 80011064
	// for pause screenshot
	char rdataPauseData[0x10];

	// 80011074
	char s_lensflare[0xC];

	// 80011080
	char s_ghost_record_buffer[0x14];

	// 80011094
	char s_GhostTape_[0xC];

	// 800110A0
	void *jumpPointers2[0xB]; // 0x2C bytes

	// 800110CC
	char s_LoadSampleBlock[0x14];

	// 800110e0
	void *jumpPointers3[0x25]; // 0x94 bytes

	// 80011174
	char s_lang_buffer[0xC];

	// 80011180
	struct
	{
		// each hub can connect to a max
		// of three other hubs in adv arena
		int connectedHub_LevID[3];

	} MetaDataHubs[5];

	// 800111bc
	char s_HUB_ALLOC[0xC];

	// 800111c8
	char s_Patch_Table_Memory[0x14];

	// 800111dc
	char s_circle[0x10];

	// 800111ec
	char s_clod[0x10];

	// 800111fc
	char s_dustpuff[0x10];

	// 8001120c
	char s_smokering[0x10];

	// 8001121c
	char s_sparkle[0x10];

	// 8001122c
	char s_lightredoff[0x1c];

	// 80011248
	char s_lightredon[0x1c];

	// 80011264
	char s_lightgreenoff[0x20];

	// 80011284
	char s_lightgreenon[0x20];


	// 800112a4
	void *LOAD_TenStages_jumpPointers4[0x10];

	// 800112e4
	char s_screen18[0xC];

	// 800112f0
	char s_credit20[0xC];

	// 800112fc
	char s_credit19[0xC];

	// 80011308
	char s_credit18[0xC];

	// 80011314
	char s_credit17[0xC];

	// 80011320
	char s_credit16[0xC];

	// 8001132c
	char s_credit15[0xC];

	// 80011338
	char s_credit14[0xC];

	// 80011344
	char s_credit13[0xC];

	// 80011350
	char s_credit12[0xC];

	// 8001135c
	char s_credit11[0xC];

	// 80011368
	char s_credit10[0xC];

	// 80011374
	// wipe to break main menu
	char s_screen15[0xC];

	// 80011380
	char s_asphalt2[0xC];

	// 8001138c
	char s_asphalt1[0xC];

	// 80011398
	char s_ThreadPool[0xC];

	// 800113a4
	char s_InstancePool[0x10];

	// 800113b4
	char s_SmallStackPool[0x10];

	// 800113c4
	char s_MediumStackPool[0x10];

	// 800113d4
	char s_LargeStackPool[0x10];

	// 800113e4
	char s_ParticlePool[0x10];

	// 800113f4
	char s_OscillatorPool[0x10];

	// 80011404
	char s_RainPool[0xC];

	// 80011410
	char s_RENDER_BUCKET_INSTANCE[0x18];

	// 80011428
	char s_Clip_Buffer[0xC];

	// 80011434
	char s_TRACKING[0xC];

	// 80011440
	char s_FOLLOWER[0xC];

	// 8001144C
	char s_STARTTEXT[0xC];

	// 80011458
	char s_PathTo_SpyroExe[0x14];

	// 8001146c
	char s_VlcTable[0xC];

	// 80011478
	char s_PathTo_Bigfile[0x14];


	// 8001148C
	void *jumpPointers5[0x65];

	// 80011620
	char s_timeString[0x14];

	// 80011634
	char s_LoadSave[0xC];

	// 80011640
	char s_loadsave[0x10];

	// 80011650
	void *jumpPointers6[0x18]; // 0x60 bytes

	// 800116B0
	char s_crystal1[0xC];

	// 800116BC
	char s_fruitdisp[0xc];

	// 800116C8
	char s_timebox1[0xc];

	// 800116d4
	// for 7 lap races up to 99:59:99
	char s_timeString_empty[0xc];


	// 800116e0
	char s_lapString[0xc];

	// 800116ec
	char rdata54[0x54];

	// 80011740
	char s_oxide[0x18];

	// 80011758
	char s_fake[0x18];

	// 80011770
	char s_pen[0x14];

	// 80011784
	char s_ntropy[0x18];

	// 8001179c
	char s_joe[0x14];

	// 800117b0
	char s_roo[0x14];

	// 800117c4
	char s_papu[0x18];

	// 800117dc
	char s_pinstripe[0x1c];

	// 800117f8
	char s_pura[0x18];

	// 80011810
	char s_polar[0x18];

	// 80011828
	char s_dingo[0x18];

	// 80011840
	char s_ngin[0x18];

	// 80011858
	char s_coco[0x18];

	// 80011870
	char s_tiny[0x18];

	// 80011888
	char s_cortex[0x18];

	// 800118a0
	char s_crash[0x18];

	// 800118b8
	char s_NO_EFFECT[0xc];

	// 800118c4
	char s_SLOWDOWN[0xc];

	// 800118d0
	char s_slowdirt[0xc];

	// 800118dc
	char s_slowgrass[0xc];

	// 800118e8
	char s_oceanasphalt[0x10];

	// 800118f8
	char s_steamasphalt[0x10];

	// 80011908
	char s_riverasphalt[0x10];

	// 80011918
	char s_sideslip[0xc];

	// 80011924
	char s_fastwater[0xc];

	// 80011930
	char s_hardpack[0xc];

	// 8001193c
	char s_icy_road[0xc];

	// 80011948
	char s_door5[0x18];

	// 80011960
	char s_akubeam1[0xc];

	// 8001196c
	char s_bombtracker1[0x10];

	// 8001197c
	char s_shielddark[0xc];

	// 80011988
	char s_highlight[0xc];

	// 80011994
	char s_warpball[0xc];

	// 800119a0
	// more jump pointers, and psyq rdata
	// until eventually...

	// Ghidra says rdata ends at 800123df

	// first func at FUN_800123e0
};

struct Difficulty
{
	s16 params1[14];
	s16 params2[14];
};
// Last PsyQ function?
// 80080990 FlushCache

struct AdventureCup
{
	s16 lngIndex_CupName;
	// color of gem and token in portals
	s16 color[3];
};

// 800809a0
struct Data
{
	// 800809a0
	// 18 tracks
	struct Difficulty ArcadeDifficulty[18];

	// 80080d90
	// 6 boss races
	struct Difficulty BossDifficulty[6];

	// 80080ee0
	s16 advDifficulty[0xC];

	// --------------------------

	// Spawn index (0-7) for player index (0-7)
	// for all types of game modes in CTR

	// 80080ef8
	struct
	{
		// 80080ef8 (different with no in-game effect)
		int VS_2P_1;
		int VS_2P_2;

		// 80080f00 (default)
		int VS_3P_4P_1;
		int VS_3P_4P_2;

		// 80080f08 (spawn p1 in center of track)
		int time_trial_1;
		int time_trial_2;

		// 80080f10 (default)
		int crystal_challenge_1;
		int crystal_challenge_2;

		// 80080f18 (default)
		int boss_challenge_1;
		int boss_challenge_2;

		// 80080f20 (default)
		int purple_cup_1;
		int purple_cup_2;

		// 80080f28 (default)
		int arcade_1;
		int arcade_2;

	} kartSpawnOrder;

	// 80080F30
	s16 botsThrottle[0xC];

	// 80080F48
	MATRIX identity;

	// 80080F68
	struct ZoomData NearCam4x3; // 1P,3P,4P
	struct ZoomData NearCam8x3; // 2P
	struct ZoomData FarCam4x3;  // 1P,3P,4P
	struct ZoomData FarCam8x3;  // 2P

	// 80080FB0
	// end of race camera
	s16 EndOfRace_Camera_Size[0x12];

	// 80080fd4
	int Spin360_heightOffset_cameraPos[5];

	// 80080fe8 from LookAt
	int Spin360_heightOffset_driverPos[5];

	// 80080FFC
	char s_XA_ENG_XNF[0x10];
	char s_XA_MUSIC[0x14];
	char s_XA_ENG_EXTRA[0x18];
	char s_XA_ENG_GAME[0x18];

	// 80081050

	// Might only be 7, but 8th
	// is always zero, and the
	// 4 bytes of 8th slot is never used
	char *xaLanguagePtrs[8];

	// 80081070
	struct AudioMeta audioMeta[3];

#define NUM_MDM 0xe2

	// 80081088
	struct MetaDataMODEL MetaDataModels[NUM_MDM];

	// 80081B20
	void *ptrRenderedQuadblockDestination_forEachPlayer[4];

	// the quadblock destination is in the 80096404 region

	// 80081B30
	void *ptrRenderedQuadblockDestination_again[4];

	// 80081B40
	// 0x230 bytes of colors
	u32 colors[NUM_COLORS][4];

	// 80081D70
	u32 *ptrColor[NUM_COLORS];

	// 80081dfc
	u16 ghostScrambleData[0x2ae];

	// 80082358
	s16 font_IconGroupID[FONT_NUM];

	// 80082360
	s16 font_charPixWidth[FONT_NUM];

	// 80082368
	s16 font_charPixHeight[FONT_NUM];

	// 80082370
	// punctuation, like period and colon
	s16 font_puncPixWidth[FONT_NUM];

	// 80082378
	// Contains the icon IDs of each ASCII character and reserved kana slots.
	u8 font_characterIconID[0xE0];

	// 80082458
	s16 font_buttonScale[FONT_NUM];

	// 80082460
	// added to character width, when using buttons
	s16 font_buttonPixWidth[FONT_NUM];

	// 80082468
	s16 font_buttonPixHeight[FONT_NUM];

	// 80082470
	char font_indentIconID[FONT_NUM * 2];

	// 80082478
	// 0x8 is debug width, 0xA is debug height, 0xC is small width, 0xE is small height, so on
	s16 font_indentPixDimensions[FONT_NUM * 2];

	// 80082488
	u16 pauseScreenStrip[0x10];

	// 800824a8
	struct GamepadButtonMap gamepadMapBtn[20];

	// 80082548
	SpuReverbAttr reverbParams[5]; // 5*0x14 = 0x64

	// distort = [0, 0xff],
	// one value for each distortion

	// 800825ac
	int distortConst_Engine[0x100];

	// 800829ac
	int distortConst_OtherFX[0x100];

	// 80082dac
	u8 volumeLR[0x100];

	// 80082eac
	// for notes ABCDEFG, and if all
	// are set to the same value, all notes are the same
	u16 noteFrequency[0x6C];

	// 80082f84
	// array of data for Instrument Pitch
	u16 distortConst_Music[0x40];


	// 80083004
	void (*opcodeFunc[0xb])(struct SongSeq *);

	// 80083030
	int opcodeOffset[0xb];

	// 8008305c
	// 0x84-byte entry
	struct
	{
		s16 index[0x16]; // contains 0x13 numbers

		struct
		{
			void *ptr;
			int num;
		} voiceSet[0xb];

	} voiceData[0x10];

	// 8008389c
	int voiceSetPtr[0x10];

	// 800838dc
	u8 voiceID[0x18];


	// 800838f4
	u8 advHubSongSetBytes[0x14];

	// 80083908
	s16 nTropyXA[6];

	// 80083914
	char reverbMode[0x38];

	// 8008394c
	char levBank_FX[0x20];

	// 8008396c
	char levBank_Song[0x20];

	// 8008398c
	u8 levAmbientSound[0x84];

	// 80083a10
	DriverModelExtraSlot driverModelExtras[LOAD_DRIVER_MODEL_EXTRA_COUNT];

	// 80083a1c
	struct Model *podiumModel_firstPlace;
	struct Model *podiumModel_secondPlace;
	struct Model *podiumModel_thirdPlace;
	struct Model *podiumModel_tawna;

	// 80083a2c
	struct Model *podiumModel_unk1;

	// 80083a30
	struct Model *podiumModel_dingoFire;

	// 80083a34
	struct Model *podiumModel_unk2;

	// 80083a38
	struct Model *podiumModel_podiumStands;

	// 80083a3c
	struct LoadQueueSlot currSlot;

	// 80083a54
	// see FUN_80032700
	u8 characterIDs_2P_AIs[LOAD_2P_AI_SET_COUNT][LOAD_2P_AI_SET_RACER_COUNT];

	// 80083A70
	// funcptrs callback 230,231,232,233
	// 230 = 800319f4
	// 231 = 80031a08
	// 232 = 80031a20
	// 233 = 80031a38
	void *overlayCallbackFuncs[4]; // probably `(void(*)(struct LoadQueueSlot*))[4]`

	// 80083A80
	struct MetaDataLEV metaDataLEV[0x41];

	// 80084098
	// [0] (gemstone) = 0x62 (beat oxide)
	// [1] (n sanity) = 0x5E (ripper roo key)
	// etc
	s16 BeatBossPrize[6];

	// 800840a4
	// 5 cups, 4 tracks each
	int advCupTrackIDs[5 * 4];

	// 800840f4
	// array of track IDs in Adv, in order of trophies
	s16 advHubTrackIDs[16];

	// 80084114
	struct AdventureCup AdvCups[5];

	// 8008413C
	// 0 - ripper roo's challenge
	// 1 - papu's challenge
	// ...
	s16 lng_challenge[6];

	// 80084148
	struct
	{
		// used to draw each string
		// in cup menu, and intro-race cutscene
		s16 lngIndex_CupName;

		struct
		{
			// used to load LEV
			s16 trackID;

			// used to draw in cup menu
			s16 iconID;
		} CupTrack[4];

	} ArcadeCups[4];

	// 0x2C bytes large
	// 80084190
	struct RectMenu menuRacingWheelConfig;

	// 0x800841BC
	struct MenuRow rowsQuit[3];

	// 0x2C bytes large
	// 0x800841D0
	struct RectMenu menuQuit;

	// 800841FC -- 24 bytes (0x18)
	struct RacingWheelData rwd[4];

	// 80084214
	char kartHwlPath[0x14];

	// 80084228
	void *PtrClipBuffer[4];

	// 80084238
	s16 lngIndex_unused_multiplayerDirections[6];

	// 80084244
	s16 lngIndex_gamepadUnplugged[6];

	// 80084250
	// heights that "gamepad unplugged" can be at
	s16 errorPosY[4];

	// 80084258

	// block should be 0x78 bytes

	// 80084258
	struct
	{
		s16 lngIndex;
		s16 lo16;
		s16 hi1;
	} raceConfig_DeadZone[4];

	// 80084270
	struct
	{
		s16 lngIndex;
		s16 lo16;
		s16 hi1;
	} raceConfig_Range[5];
	s16 padding8008428e;

	// 80084290
	// number of options on each page of race wheel config
	u16 raceConfig_unk80084290[14];

	// 800842ac
	u8 raceConfig_colors_arrows[0xc];

	// 800842B8
	u8 raceConfig_colors_blueRect[0x18];

	// 800842D0
	u8 jogConTriangleColors[0xc];

	// 800842DC
	s16 unkNamcoGamepad_800842DC[0x30 / 2];

	// 8008430c
	int gGT_gameMode1_VibPerPlayer[4];

	// 8008431c
	// Controller 1, 2, 1A, 1B, 1C, 1D
	s16 Options_StringIDs_Gamepads[6];

	// 80084328
	// FX, MUSIC, VOICE, 4th for padding
	s16 Options_StringIDs_Audio[4];

	// 0x80084330
	// horizontal bar (blue),
	// vertical bar (white),
	// outer vertical bars (black)
	char Options_VolumeSlider_Colors[0x14];

	// 0x80084344
	struct
	{
		u16 posY;
		u16 sizeY;
	} Options_HighlightBar[9];

	// 0x80084368
	struct MenuRow rowsAdvHub[5];

	// +2 padding
	char padding_advhub[2];

	// 0x80084388 -- 2C
	struct RectMenu menuAdvHub;

	// 0x800843B4
	struct MenuRow rowsAdvRace[5];

	// +2 padding
	char padding_advrace[2];

	// 0x800843D4 -- 2C
	struct RectMenu menuAdvRace;

	// 0x80084400
	struct MenuRow rowsAdvCup[4];

	// 0x80084418 -- 2C
	struct RectMenu menuAdvCup;

	// 0x80084444
	struct MenuRow rowsBattle[8];

	// 0x80084474 -- 2c
	struct RectMenu menuBattle;

	// 0x800844A0
	struct MenuRow rowsArcadeCup[4];

	// 0x800844B8 -- 2c
	struct RectMenu menuArcadeCup;

	// 0x800844E4
	struct MenuRow rowsArcadeRace[7];

	// +2 padding
	char padding_arcaderace[2];

	// 0x80084510
	struct RectMenu menuArcadeRace;

	// 0x8008453C
	struct
	{
		// 0x8008453C
		int nTropyOpen; // 1

		// 0x80084540
		int nOxideOpen; // 2

	} bitIndex_timeTrialFlags_saveData;

	// 0x80084544
	struct
	{
		// 0x80084544
		int nTropyOpen; // 1

		// 0x80084548
		int nOxideOpen; // 2

	} bitIndex_timeTrialFlags_flashingText;

	// 0x8008454C
	// 1P, 2P, 4P
	// LEV 0 - 0x19 (0x1c for alignment)
	u8 primMem_SizePerLEV_1P[0x1c];
	u8 primMem_SizePerLEV_2P[0x1c];
	u8 primMem_SizePerLEV_4P[0x1c];

	// 0x800845a0
	// trigonometry approximation table (1kb)
	struct TrigTable trigApprox[0x400];

	// 0x800855A0
	int memcardIcon_CrashHead[0x40];

	// 0x800856A0
	int memcardIcon_Ghost[0x40];

	// 0x800857A0
	int memcardIcon_PsyqHand[0x40];

	// 0x800858A0
	struct MetaDataBOSS BossWeaponOxide[7 * 2];

	// 0x80085910
	struct MetaDataBOSS BossWeaponRoo[3 * 2];

	// 0x80085940
	struct MetaDataBOSS BossWeaponPapu[3 * 2];

	// 0x80085970
	struct MetaDataBOSS BossWeaponJoe[3 * 2];

	// 0x800859A0
	struct MetaDataBOSS BossWeaponPinstripe[3 * 2];

	// 0x800859d0
	struct MetaDataBOSS *bossWeaponMetaPtr[5];


	// address 0x800859E4
	// size 0x14
	// BASCUS-94426-SLOTS
	char s_BASCUS_94426_SLOTS[0x14];

	// 0x800859F8
	u8 memcardIcon_HeaderSLOTS[0x38];

	// Question marks are part of strings
	// 80085A30
	char s_BASCUS_94426G_Question[0x14];

	// 0x80085A44
	// size 0x10
	// BASCUS-94426G*
	char s_BASCUS_94426G_Star[0x10];

	// 0x80085A54
	struct MenuRow rowsSaveGame[3];

	// +2 padding (0x14 total) 0x14 = 20, 3*6 +2
	char padding_savegame[2];

	struct RectMenu menuSaveGame;

	// 0x80085A94
	// has internal pointer to 80043b30,
	// which loads a LEV from track selection
	struct RectMenu menuQueueLoadTrack;

	// 80085AC0
	// MATRIX struct at 80085AC0
	MATRIX matrixTitleFlag;

	// 80085AE0
	int checkerFlagVariables[5];

	// 80085AF4
	s16 PlayerCommentBoxParams[8];

	// 80085B04
	// 0 - Warning, No Memory Card (save/load)
	// 1 - Warning, Memory Card Unformatted
	// 2 - Formatting...
	// 3 - Saving to Memory Card...
	// 4 - Loading...
	// 5 - Checking... No Memory Card (new profile)
	// 6 - Warning, not enough room to save ghost (two blocks)
	// 7 - error occured whlie reading memory card
	// 8 - NULL (draw no box)
	// 9 - No Data
	int messageScreens[10]; // or is it (dec) 14?

	// 80085B2C
	// [0] = 0x13C - FINISHED!
	// [1,2,3] = 0xFFFF
	int lngIndex_FinishOrLoser[4];

	// 80085B3C
	struct MenuRow rowsGreenLoadSave[5];

	// +2 padding (0x14 total) 0x14 = 20, 3*6 +2
	char padding_greenloadsave[2];

	// 80085b5c
	// menu for green save/load screen
	struct RectMenu menuGreenLoadSave;


	// 80085b88
	// menu to draw adventure profiles
	struct RectMenu menuFourAdvProfiles;

	// 80085bb4
	// menu for ghosts
	struct RectMenu menuGhostSelection;

	// 80085be0
	// menu for "Warning, no memory card"
	struct RectMenu menuWarning2;

	// 80085c0c
	// menu for "Please Enter Your Name"
	struct RectMenu menuSubmitName; // on-screen keyboard

	// 80085c38
	struct RectMenu menuQueueLoadHub;

	// 0x80085C64
	// appear in save/load screen
	struct
	{
		// 0x0
		s16 modelID;
		s16 scale;

		// 0x4
		// parameter to Vector_SpecLightSpin3D
		SVec3 vec3_specular_inverted;

		// 0xA
		char r;
		char g;
		char b;
		char a;

		// 0xE - size of struct

		// [0] = Relic
		// [1] = Key
		// [2] = Trophy
		// four sets of three,
		// one for each AdvProfile

	} MetaDataLoadSave[3 * 4];

	// 80085D0C
	// used in the same function as
	// MetaDataSaveLoad, but usage is unknown
	s16 spinOffset_LoadSave[4];

	// 80085d14
	s16 lngIndex_LoadSave[4];

	// "overwrite" can be triggered when
	// going to Adventure -> New and trying
	// to save over an existing file

	// 80085D1C
	struct MenuRow rowsOverwrite[3];

	// 80085D30
	struct RectMenu menuOverwriteAdv;

	// 80085D5C
	struct RectMenu menuOverwriteGhost;

	// 80085D88
	s16 lngStringsSaveLoadDelete[6];

	// A.B.C.D... all the way to Z and then numbers,
	// used in high score "Enter Your Name" window
	// 80085D94
	// 26 letters, 10 numbers, period, underscore, arrow, nullptr
	u16 unicodeAscii[40];

	// 80085DE4
	// pointer to sounds for menus
	int soundIndexArray[6];

	// 80085dfc

	// 00 - weaponIcon
	// 01 - lapCount
	// 02 - bigNumber
	// 03 - wumpaModel
	// 04 - wumpaNum
	// 05 - bigNumberSuffix
	// 06 - jumpMeter
	// 07 - ???
	// 08 - driftMeter
	// 09 - speedometer
	// 0A - ???
	// 0B - juicedUp_behindWeapon
	// 0C - juicedUp_behindWumpa
	// 0D - numLives
	// 0E - advhub_numRelic
	// 0F - advhub_numKey
	// 10 - advhub_numTrophy
	// 11 - numCrystal
	// 12 - LetterCTR
	// 13 - timeCrateNum
	// 14 - num elements

	struct UiElement2D hud_1P_P1[0x14];

	struct UiElement2D hud_2P_P1[0x14];
	struct UiElement2D hud_2P_P2[0x14];

	struct UiElement2D hud_4P_P1[0x14];
	struct UiElement2D hud_4P_P2[0x14];
	struct UiElement2D hud_4P_P3[0x14];
	struct UiElement2D hud_4P_P4[0x14];

	// 8008625c, array of four pointers,
	// [0] = 1P P1
	// [1] = 2P P1
	// [2] = 4P P1
	// [3] = 4P P1
	struct UiElement2D *hudStructPtr[4];

	// 8008626c
	// 8 XY values
	s16 trackerAnim1[8 * 2];

	// 8008628c
	// 11 XY values, but ND code
	// sets variable to 12 instead of 11,
	// Naughty Dog bug?
	s16 trackerAnim2[0xb * 2];

	// 800862b8
	s16 rankIconsDesired[8];

	// 800862c8
	s16 rankIconsCurr[8];

	// 800862d8
	s16 rankIconsTransitionTimer[8];

	// 800862e8
	s16 trackerTimer[4];

	// 800862f0
	s16 trackerDist[4];

	// 800862f8
	s16 trackerType[4];

	// 0x80086300
	struct MenuRow rowsRetryExit[3];

	// +2 padding

	// 0x80086314
	struct RectMenu menuRetryExit;

	// 0x80086340
	int RelicTime[3 * 18];

	// 0x80086418
	struct
	{
		SVec2 pos[3];

		// Two blink frames, each containing the three packed vertex colors.
		s32 colors[2][3];

	} playerIconAdvMap;

	// 0x8008643C
	// FUN_8005045c
	s16 stringIndexSuffix[8];

	// 0x8008644C
	// FUN_80050654
	s16 battleScoreColor[4][4];

	// 0x8008646C
	Point speedometerBG_vertData[2][14];

	// 0x800864DC
	// LNG index for end-of-race comments
	u8 data850[0x850];

#if 0

	// VS QUIPs
	// [0x00] = HOP HAPPY,		RARE AIR, 		0, 1, 0x28, 	0x554(numberOfJumps), 2
	// [0x01] = RARE AIR, 		WALLFLOWER, 	1, 1, 0x12c0, 	0x53c(timeSpentJumping), 4
	// [0x02] = WALLFLOWER, 	MISSILE MAGNET	0, 1, 0xf00,	0x530(timeSpentAgainstWall), 4
	// [0x03] = MISSILE MAGNET,		---,		0, 0, 2, 		0x55d(numTimesMissileHitYou), 1
	// [0x04] = MOST BOMBED, 		---, 		0, 0, 2,		0x55e(numTimesBombHitYou), 1
	// [0x05] = WHAT'S A PICKUP?, 	---,		1, 1, 1,		0x568(numTimesHitWeaponBox), 1	
	// [0x06] = SLOWPOKE,			---,		1, 0, 0xfa0,	0x518(distanceDriven), 4
	// [0x07] = MOST DIZZY, 		---,		0, 1, 0xb40,	0x540(timeSpentSpinningOut), 4
	// [0x08] = SITTING DUCK,		---,		4, 0, 3,		0x57c(numTimesAttacked), 4
	// [0x09] = S'S CHUMP,			---,		3, 0, 3,		0x560(numTimesAttackedByPlayer), 8
	// [0x0A] = BAD SHOT,			---,		1, 0, 0x800,	0x574(NumMissilesComparedToNumAttacks), 4
	// [0x0B] = FALL GUY,			---,		0, 0, 3,		0x56a(numTimesMaskGrab), 1
	// [0x0C] = LEAST FRUIT,		---,		1, 1, 6,		0x569(numTimesWumpa), 1
	// [0x0D] = MINE SWEEPER,		---,		0, 1, 1,		0x55f(numTimesMotionlessPotionHitYou), 1
	// [0x0E] = MUD DOG,			---,		0, 1, 0xf00,	0x524(timeSpentInMud), 4
	// [0x0F] = PLANT FOOD,			---,		0, 0, 0xb40,	0x548(timeSpentEaten), 4
	// [0x10] = MOST BURNED,		---,		0, 1, 0xf00,	0x54c(timeSpentBurnt), 4
	// [0x11] = FLATMAN,			---,		0, 1, 0x12c0,	0x544(timeSpentSquished), 4
	// [0x12] = MR. REVERSE,		---,		0, 1, 0x1e00,	0x520(timeSpentReversing), 4
	// [0x13] = SKID SQUID,			---,		0, 1, 0x12c0,	0x51c(timeSpentWithHighSpeed), 4
	// [0x14] = TURBO MASTER,		---,		0, 1, 0x4b00,	0x534(timeSpentUsingReserves), 4
	// [0x15] = SUPER SLIDER,		---,		0, 1, 0x4b00, 	0x538(timeSpentDrifting), 4
	// [0x16] = MOST AIR TIME,		---,		0, 1, 0x1e00,	0x53c(timeSpentJumping), 4
	// [0x17] = BIGGEST AIR,		---,		0, 0, 0x3c0,	0x550(highestJump), 2
	// [0x18] = BEAKER ACE,			---,		0, 0, 0,		0x556(numTimesMovingPotionHitSomeone), 1
	// [0x19] = MISSILE MASTER,		---,		0, 0, 2,		0x557(numTimesMissileHitSomeone), 1
	// [0x1a] = CRATES 'R' US,		---,		0, 1, 2,		0x568(numTimesHitWeaponBox), 1
	// [0x1b] = FASTEST RACER,		---,		0, 0, 0xfa0,	0x518(distanceDriven), 4
	// [0x1c] = KING OF THE PACK,	---,		0, 0, 3,		0x559(numTimesAttacking), 1
	// [0x1d] = UNTOUCHABLE,		---,		1, 0, 1,		0x57c(numTimesAttacked), 4
	// [0x1e] = BEST AIM,			---,		0, 0, 0x800,	0x574(NumMissilesComparedToNumAttacks), 4
	// [0x1f] = LONGEST SHOT,		---,		0, 0, 0x3c0,	0x552(longestShot), 2
	// [0x20] = MOST JUICED UP,		---,		0, 1, 0x12c0,	0x52c(timeSpentInTenWumpa), 4
	// [0x21] = FRUIT HOG,			---,		0, 1, 0xc,		0x569(numTimesWumpa), 1
	// [0x22] = COMEBACK KID,		---,		0, 0, 0x2580,	0x578(TimeWinningDriverSpentLastPlace), 4
	// [0x23] = CLOCK JOCK,			---,		0, 0, 1,		0x558(numTimesClockWeaponUsed), 1
	// [0x24] = SPEED DEMON!,		---,		5, 0, 0x2580,	0x514(timeElapsedInRace), 4
	// [0x25] = STEAMROLLER,		---,		0, 0, 1,		0x55b(numTimesSquishedSomeone), 1
	// [0x26] = OUT OF GAS!,		NULL,		9, 0, 0,		0x559(numTimesAttacking), 1

	// 0x27 quip metas,
	800864DC - 8008664c // VS QuipStr array (0x2E)
	8008664c - 800869f4 // VS QuipMeta array (0x27)

	// BATTLE QUIPs
	// [0x0] = SMACK MAN, 	DISASTER MASTER!,	7, 0, 0,	0x559(numTimesAttacking), 1
	// [0x1] = SO CLOSE!,	SMACK MAN,			7, 0, 1,	0x559(numTimesAttacking), 1
	// [0x2] = TOASTED!,	LOST IN SPACE,		8, 8, 0,	0x559(numTimesAttacking), 1
	// [0x3] = DISASTER MASTER!, THE DOMINATOR!,8, 2, 0xF,	0x4f6(quip4), 2
	// [0x4] = THE DOMINATOR!, COMEBACK KID!,	4, 6, 2,	0x559(numTimesAttacking), 1
	// [0x5] = COMEBACK KID!, GAVE IT AWAY!,	0, 6, 3,	0x4f2(quip2), 2
	// [0x6] = GAVE IT AWAY!, DEATH TOUCH!,		0, 2, 2,	0x4f4(quip3), 2
	// [0x7] = DEATH TOUCH!, ACID RAINMAN!,		8, 6, 6,	0x4f0(quip1), 2
	// [0x8] = ACID RAINMAN!, BULL'S EYE!,		8, 6, 4,	0x4f0(quip1), 2
	// [0x9] = BULL'S EYE!, FISH IN A BARREL!,	8, 6, 1,	0x4f0(quip1), 2
	// [0xA] = FISH IN A BARREL!, --invalid--,	6, 4, 0,	0x575(NumMissilesComparedToNumAttacks>>8), 1
	// [0xB] = LOST IN SPACE, SO CLOSE!,		9, 0, 0, 	0x559(numTimesAttacking, 1)
	
	// --invalid-- maybe should've been TOASTED,
	// accidentally pointed to 80086c0c instead of QuipStr

	// 0xC quip metas,
	800869f4 - 80086c0c // Battle QuipStr array (0x43)
	80086c0c - 80086d2c // Battle QuipMeta array (0xC)

	struct QuipStr
	{
		s16 lngIndex;
		s16 flag; // &1 means include Crash/Cortex/Coco name
		
		s16 unused1;
		s16 unused2;
	};

	// Quip = End-Of-Race comment
	struct QuipMeta
	{
		// QuipMeta->next = QuipMeta->next->next,
		// This removes QuipStr from search algorithm,
		// to make it impossible for quips to repeat.
		// Somewhere the linked list needs to restart itself
		struct QuipStr* ptrQuipStrCurr;
		struct QuipStr* ptrQuipStrNext;
		
		s16 unk0;

		// 0xA
		s16 unk1;

		// 0xC
		int unk2;

		// 0x10
		// offset of driver to pull data from,
		// and the amount of bytes in the variable
		int driverOffset;
		int dataSize;

		// 0x18 each
	};
#endif

	// 0x80086d2c
	// Red, Green, Blue, Yellow, Purple, NULL
	s16 advCupStringIndex[6];

	// 0x80086d38
	// Wumpa, Crystal, Nitro, Crash, NULL, NULL
	s16 arcadeVsCupStringIndex[6];

	// 0x80086d44
	// characterIDs of all racers
	// in order of cup rank
	// 1st, 2nd, 3rd, etc.
	int cupPositionPerPlayer[8];

	// 0x80086d64
	// 9, 6, 3, 1, 0, 0, 0, 0
	int cupPointsPerPosition[8];

	// 0x80086d84
	// Copy one row into another
	// to make duplicates of icons
	// in character selection screen
	struct MetaDataCHAR MetaDataCharacters[0x10];

	// 0x80086e84
	s16 characterIDs[8];

	// 0x80086e94
	// bakedGteMath[BAKED_GTE_MATRIX_NONE] is blank,
	// all the rest correspond
	struct MatrixND matArr01[0xB];  // BAKED_GTE_MATRIX_WHEELIE_START
	struct MatrixND matArr02[0x1];  // BAKED_GTE_MATRIX_WHEELIE_HOLD
	struct MatrixND matArr03[0x9];  // BAKED_GTE_MATRIX_WHEELIE_RECOVER
	struct MatrixND matArr04[0x10]; // BAKED_GTE_MATRIX_CRASH_FALL
	struct MatrixND matArr05[0xF];  // BAKED_GTE_MATRIX_SQUISH_RECOVER
	struct MatrixND matArr06[0x1B]; // BAKED_GTE_MATRIX_BLASTED

	// jump animations
	struct MatrixND matArr07[0x4]; // Crash Bandicoot jump
	struct MatrixND matArr08[0x4]; // cortex
	struct MatrixND matArr09[0x4]; // tiny
	struct MatrixND matArr0A[0x4];
	struct MatrixND matArr0B[0x4]; // ...
	struct MatrixND matArr0C[0x4];
	struct MatrixND matArr0D[0x4];
	struct MatrixND matArr0E[0x4];
	struct MatrixND matArr0F[0x4];
	struct MatrixND matArr10[0x4];
	struct MatrixND matArr11[0x4];
	struct MatrixND matArr12[0x4];
	struct MatrixND matArr13[0x4]; // N Tropy jump

	// ^^^
	// (0xD) penta uses ripper roo
	// (0xE) fake crash uses crash bandicoot
	// (0xF) oxide uses crash bandicoot

	// 0x80087EF4 - pointer to 0x80086e94
	struct
	{
		void *physEntry;
		int numEntries;
	} bakedGteMath[BAKED_GTE_MATRIX_COUNT];

	// 0x80087f94
	struct Scrub MetaDataScrub[7];

	// 0x80088004
	// MetaDataTerrain offset 0x18
	struct ParticleEmitter emSet_DirtLR[8];
	struct ParticleEmitter emSet_GrassL[10];
	struct ParticleEmitter emSet_GrassR[10];
	struct ParticleEmitter emSet_SnowLR[6];

	// 0x800884CC
	struct Terrain MetaDataTerrain[0x15];

	// 0x80088A0C
	struct MetaPhys metaPhys[65]; // 0x71C bytes total

	// 80089128, nullify to remove Player bubble exhaust underwater in 1P mode
	struct ParticleEmitter emSet_Exhaust_Water[7];

	// 80089224
	struct ParticleEmitter emSet_Exhaust_High[8];

	// 80089344, nullify to remove Player exhaust in 2P mode
	struct ParticleEmitter emSet_Exhaust_Med[8];

	// 80089464, nullify to remove AI exhaust, or players in 3P+4P mode
	struct ParticleEmitter emSet_Exhaust_Low[8];

	// 80089584, nullify to remove Ground sparks
	struct ParticleEmitter emSet_GroundSparks[9];

	// 800896c8, nullify to remove Wall sparks
	struct ParticleEmitter emSet_WallSparks[9];

	// 8008980c, nullify to remove Mud splashes
	struct ParticleEmitter emSet_MudSplash[0xA];

	// 80089974, nullify to remove smoke particles while burned (tiger temple, mystery caves)
	struct ParticleEmitter emSet_BurnSmoke[8];

	// 80089a94, nullify to remove "falling" particles, when you splash in water or something
	struct ParticleEmitter emSet_Falling[6];

	// refer to chart
	// https://media.discordapp.net/attachments/637616020177289236/982513721354092564/unknown.png?width=602&height=431

	// 80089b6c, Itemset 1 for Races
	char RNG_itemSetRace1[0x14];

	// 80089b80, Itemset 2 for Races
	char RNG_itemSetRace2[0x34];

	// 80089bb4, Itemset 3 for Races
	char RNG_itemSetRace3[0x14];

	// 80089bc8, Itemset 4 for Races
	char RNG_itemSetRace4[0x14];

	// 80089bdc, Itemset for Boss races
	char RNG_itemSetBossrace[0x14];

	// 80089bf0, Itemset for Battle Mode
	char RNG_itemSetBattleDefault[0x14];

	// 80089c04, nullify to remove warpball particles
	struct ParticleEmitter emSet_Warpball[0xB];

	// 80089eb0 and 8008a018 are unidentified effects.

	// 80089d90, nullify to remove maskgrab particles (similar to "drop", not the same)
	struct ParticleEmitter emSet_Maskgrab[0x1C];

	// 8008a180, nullify to remove warppad dustpuff particles
	struct ParticleEmitter emSet_Warppad[0x8];

	// 8008a2a0
	// int placeholder_lastByte;

	// 8008a2a0 -- confetti data
	// 8008a344 -- 8 jmp pointers for normal tire drawing
	// 8008a364 -- 8 jmp pointers for reflected tire drawing
	// 8008a384 -- 0x80 bytes, tire sprite index for each rotation angle
	// 8008a404 -- int last rotation?
	// 8008a408 -- 8 jmp pointers for RenderListInit
	// 8008a428 -- 7 jmp pointers for RenderBucket
	// 8008a444 -- 7 jmp pointers for RenderBucket
	// 8008a474 -- 3 funcPtrs for RenderBucket

	// ==== End of ND ====
	// ==== Start PSYQ ====

	// From here to end is probably all psyq data
	// 8008a480 -- count 0 to 0x200
	// 8008ac84 -- mostly-empty data
	// 8008ad7c -- funcPtr to Printf
	// 8008b000 to 8008c000 -- $sp for psyq libs
	// 8008c050 -- check if booted before
	// 8008c054 -- pointer to first overlay
	// 8008c05c -- pointer to first exe function
	// 8008cf6b -- end of Data
};

CTR_STATIC_ASSERT(offsetof(struct Data, podiumModel_firstPlace) == offsetof(struct Data, driverModelExtras) + sizeof(((struct Data *)0)->driverModelExtras));
CTR_STATIC_ASSERT(offsetof(struct Data, currSlot) == offsetof(struct Data, driverModelExtras) + 11 * sizeof(void *));
CTR_STATIC_ASSERT(sizeof(((struct Data *)0)->characterIDs_2P_AIs) == 0x1c);
CTR_STATIC_ASSERT(offsetof(struct Data, bakedGteMath) == 0x7554);
CTR_STATIC_ASSERT(sizeof(((struct Data *)0)->bakedGteMath) == BAKED_GTE_MATRIX_COUNT * 8);

// 0x8008CF6C
struct sData
{
	// 8008CF6C
	int langBufferSize;

	// 0x8008CF70
	s16 *arcade_difficultyParams;

	// 0x8008CF74
	s16 *cup_difficultyParams;

	// 0x8008CF78
	// path index for each AI
	char driver_pathIndexIDs[8];

	// 0x8008CF80
	// both these are multiplied by accelerateOrder,
	// used to spread out AIs during first few seconds of race
	int AI_AccelFrameCount;
	int AI_AccelFrameSteps;

	// 0x8008CF88
	int AI_VelY_WhenBlasted_0x3000;

	// 8008CF8C
	char s_camera[8];

	// 8008CF94
	struct
	{
		// 8008CF94
		SVec3 posOffset;
		s16 _pad_posOffset;

		// 8008CF9C
		SVec3 rotOffset;
		s16 _pad_rotOffset;

	} FirstPersonCamera;

	// 8008CFA4
	char s_DCH[4];
	char s_SPN[4];
	char s_ITL[4];
	char s_GRM[4];
	char s_FRN[4];
	char s_ENG[4];
	char s_JPN[4];

	// 8008CFC0
	char s_XINF[8];


	char s_BLADE[8];
	char s_TURBO[8];
	char s_SPRING[8];
	char s_GATE[8];

	// 8008CFE8
	char s_NO_FUNC[8];

	// 8008CFF0
	int boolDebugDispEnv;

	// 8008CFF4
	int LevClearColorRGB[3];

	// 8008D000
	u32 HudAndDebugFlags;

	// 8008D004
	char unk_CTR_MatrixToRot_table[0x10];

	// 8008d014
	// used for "honk" sounds
	int trafficLightsTimer_prevFrame;

	// 8008d018
	// 6 four-byte pointers for pause/unpause,
	// all relative to gGT->DB[0,1].primMem.end
	char *PausePtrsVRAM[6]; // should maybe be `u32*[]` instead of `char*[]`

	// 8008d030
	u32 pause_backup_renderFlags;

	// 8008d034
	u16 pause_backup_hudFlags;

	// 8008d036
	// (0,1,2,3)
	// "Kyle's Mom"
	s16 pause_state;

	// 8008d038
	u8 padActuatorAlignment[8];

	// 8008D040
	char s_ghost[8];

	// 8008D048
	char s_ghost1[8];

	// 8008D050
	char s_ghost0[8];

	// 8008D058
	u32 audioRNG;

	// 8008D05C
	u8 boolAudioEnabled;
	u8 _pad_boolAudioEnabled[3];

	// 8008D060
	char s_HOWL[8];

	// 8008D068
	struct SongSet advHubSongSet;

	// 8008d070
	char reverbModeBossID[8];

	// 8008d078
	u8 songBankBossID[8];


	// 8008d080
	// used in Garage_LerpFX
	u8 garageSoundIDs[8];

	// ============================

	// 8008d088
	int ptrMPK; // maybe is `void*` instead of `int`

	// 8008d08c
	// ptrLEV, stored here during loading,
	// before passed to gGT for drawing
	struct Level *ptrLevelFile;

	// 8008d090
	void *PatchMem_Ptr;

	// 8008d094 - size of PatchMem
	int PatchMem_Size;

	// 8008d098
	// MEMPACK_PopToState(x)
	int bookmarkID;

	// 8008d09c
	struct BigHeader *ptrBigfileCdPos_2;

	// 8008d0a0
	// 1 for ready, 0 for not ready
	char queueReady;

	// 1 for retry, 0 to proceed,
	// For when CD reading is interrupted???
	char queueRetry;

	// 8008d0a2
	s16 queueLength;

	// 8008d0a4
	// Original Naughty Dog name
	int load_inProgress;

	// 8008d0a8
	int frameFinishedVRAM;

	// 8008D0AC
	char s_FILE[8];

	// 8008D0B4
	// used to get which lev file
	// to load, depending on LOD
	char levBigLodIndex[0x8];

	// 8008d0bc
	int boolFirstBoot;

	// 8008D0C0
	char s_ndi[4];
	char s_ending[8];
	char s_intro[8];
	char s_screen[8];
	char s_garage[8];
	char s_hub[4];
	char s_credit[8];

	// 8008d0f0
	// set to zero to cause LOD bugs
	struct Model *modelMaskHints3D;

	// 8008d0f4
	int mainGameState;

	// 8008d0f8
	struct
	{
		// stage = 0 - 9
		// FUN_80033610
		// Think back to rebuild-intros
		LoadStage stage;

		// 8008d0fc
		u32 Lev_ID_To_Load;

		// 8008d100
		struct
		{
			u32 AddBitsConfig0;
			u32 RemBitsConfig0;
			u32 AddBitsConfig8;
			u32 RemBitsConfig8;
		} OnBegin;

	} Loading;

	// 8008d110
	int numLngStrings; // 0 in exe

	// 8008d114
	char s_credit9[8];
	char s_credit8[8];
	char s_credit7[8];
	char s_credit6[8];
	char s_credit5[8];
	char s_credit4[8];
	char s_credit3[8];
	char s_credit2[8];
	char s_credit1[8];
	char s_ending2[8];
	char s_ending1[8];

	// the "ndi" string shows twice
	char s_ndi_needToRename[4];
	char s_garage1[8];


	char s_intro9[8];
	char s_intro8[8];
	char s_intro7[8];
	char s_intro6[8];
	char s_intro5[8];
	char s_intro4[8];
	char s_intro3[8];
	char s_intro2[8];
	char s_intro1[8];

	char s_hub5[8];
	char s_hub4[8];
	char s_hub3[8];
	char s_hub2[8];
	char s_hub1[8];

	char s_battle7[8];
	char s_battle6[8];
	char s_battle5[8];
	char s_battle4[8];
	char s_battle3[8];
	char s_battle2[8];
	char s_battle1[8];

	char s_secret2[8];
	char s_secret1[8];
	char s_space[8];
	char s_ice1[8];
	char s_labs1[8];
	char s_castle1[8];
	char s_cave1[8];
	char s_sewer1[8];
	char s_blimp1[8];
	char s_tube1[8];
	char s_temple2[8];
	char s_temple1[8];
	char s_island1[8];
	char s_desert2[8];
	char s_proto9[8];
	char s_proto8[8];

	// 8008d2a0
	u16 finalLapTextTimer[4];

	// 8008d2a8
	s16 TurboDisplayPos_Only1P;
	s16 numPlayersFinishedRace;

	// 8008d2ac
	struct GameTracker *gGT; // real ND name

	// 8008d2b0
	struct GamepadSystem *gGamepads;

	// draw the same frame twice in a row
	// making 60fps look like 30fps

	// 8008d2b4
	int vsyncTillFlip;

	// 8008d2b8
	char s_PrimMem[8];

	// 8008d2c0
	char s_OTMem[8];

	// 8008d2c8
	int boolPlayVideoSTR;

	// 8008d2cc
	// (actually two shorts, not one int),
	// (both related to requesting Aku Hint)
	s16 AkuHint_RequestedHint;

	// 8008d2ce
	s16 AkuHint_boolInterruptWarppad;

	// 8008d2d0
	// 1,2,4,4,8,8,8,8
	// LOD == 8 means Time Trial or Relic Race,
	// index is number players,
	// (numPlayer | 4) for LOD == 8
	char LOD[8];

	// 8008d2d8
	// Y positions for analog controller config display (RaceWheel, Namco)
	int analogConfigY[2];

	// 8008d2e0
	// one for RaceWheel, one for Namco
	s16 posY_MultiLine[4];

	// 8008d2e8
	// one for RaceWheel, one for Namco
	s16 posY_Arrows[4];

	// 8008d2f0
	int unk_RaceWheelConfig[3];

	// 8008d2fc
	char s_ot0[8];
	char s_ot1[8];
	char s_PLAYER[8];
	char s_PLYR[8];
	char s_ROBOT[8];
	char s_RBOT[8];
	char s_STATIC[8];
	char s_STAT[8];
	char s_MINE[8];
	char s_WARPPAD[8];
	char s_WRPD[8];
	char s_TRKG[8];
	char s_BURST[8];
	char s_BRST[8];
	char s_BLOWUP[8];
	char s_BLWP[8];
	char s_TURBO_again[8];
	char s_TRBO[8];
	char s_SPIDER[8];
	char s_SPDR[8];
	char s_FLWR[8];
	char s_STXT[8];
	char s_OTHER[8];
	char s_OTHR[8];
	char s_AKUAKU[8];
	char s_AKUU[8];
	char s_CAMERA[8];
	char s_CAMR[8];
	char s_HUD[4];

	// 8008D3E0
	char s_THUD[8];
	char s_PAUSE[8];

	// 8008D3F0
	char s_PAUS[8];

	// 8008D3F8
	void *MainDrawCb_DrawSyncPtr;

	// 8008D3FC
	void *ptrVlcTable;

	// 8008D400
	int bool_IsLoaded_VlcTable;

	// ====== Should be memcard struct ================

	// 8008D404
	int memcard_stage;

	// 8008d408
	u8 *memcard_ptrStart;

	// 8008d40c
	// memcard file descriptor, returned from "open(xxx)"
	int memcard_fd;

	// 8008D410
	int memcard_remainingAttempts;

	// 8008d414
	char s_memcardDirHeader[8];

	// 8008d41c
	char s_AnyFile[4];

	// ===================================================

	// 8008d420
	struct Mempack *PtrMempack; // all good

	// 8008d424
	// Used to calculate random numbers, like weapon roulette,
	// see FUN_8003ea28 to understand how it works
	int randomNumber;

	// 8008d428
	s16 bossPathRequestTimer;

	s16 bossPathRequestPhase;

	// 8008d42C
	s16 bossJuiceCounter;
	s16 padding_8008d42e;

	// 8008d430
	// -1 for no expansion
	// or it holds the row
	// you want to expand
	s16 battleSetupExpandMenu;

	// 8008d432
	s16 battleSetupRowHighlighted;

	// 8008d434
	s16 battleSetupWeaponHighlighted;
	s16 padding_8008d436;

	// 8008d438 UI color data
	u32 battleSetup_Color_UI_1;
	u32 battleSetup_Color_UI_2;

	// 8008d440
	int RaceFlag_AnimationType;

	// 8008d444
	s16 RaceFlag_Position;

	// 8008d446
	s16 RaceFlag_TransitionSpeed;

	// 8008d448
	s16 RaceFlag_CanDraw;

	// 8008d44a
	s16 RaceFlag_DrawInitialized;

	// 8008d44c
	s16 RaceFlag_DrawOrder;
	s16 padding_8008d44e;

	// 8008d450
	int RaceFlag_LoadingTextAnimFrame;

	// 8008d454
	int RaceFlag_Transition;

	// 8008d458
	int RaceFlag_ElapsedTime;


	// 8008d45c
	int RaceFlag_CopyLoadStage;

	// 8008d460
	Color DrawSolidBoxData[3];

	char strcatData1_colon[4];

	// 8008d470
	void *ptrToMemcardBuffer1;

	// 8008d474
	// pointer to memcard bytes (again?) 800992E4
	void *ptrToMemcardBuffer2;

	// 2 - NULL
	// 3 - MC_START_SAVE_MAIN
	// 4 - NULL
	// 5 - MC_START_LOAD_GHOST
	// 6 - MC_START_SAVE_GHOST
	// 7 - MC_START_LOAD_MAIN

	// 8008d478
	// MC Start
	s16 mcStart;

	// 8008d47a (MC_SCREEN)
	s16 mcScreenText;

	// 8008d47c
	s16 LoadSave_SpinRateY[4];

	// 8008d484
	char stringFormat1[8];
	char stringFormat2[4];

	// 8008d490
	// grey color (80 80 80)
	int greyColor;

	// 8008d494
	// green color (20 c0 20)
	int greenColor;

	// 8008d498
	// percent sign in ASCII hex code (0x25)
	// for drawing adv profile
	int s_percent_sign;

	// 8008d49c
	// red color drawn on ghost profile, (a0 a0 00)
	// when it cannot be selected, due to wrong track
	int redColor;

	// 8008d4a0
	int ghostIconColor;

	// 8008d4a4
	char unk_BeforeTokenMenu[0x8];

	// 8008d4ac
	char str_underscore[4];


	// 8008d4b0
	// check if CTR Token/Relic adv menu is open or not
	int boolOpenTokenRelicMenu;

	// 8008d4b4 -- ptr to 8009ad18 (pushBuffer) for multiplayer wumpa
	int ptrPushBufferUI;

	// 8008d4b8
	int ptrFruitDisp; // should maybe be `struct Instance*` instead of `int`?

	// 8008d4bc
	int menuReadyToPass;

	// 8008d4c0
	char s_token[8];
	char s_relic1[8];
	char s_key1[8];
	char s_trophy1[8];
	char s_big1[8];
	char s_hudc[8];
	char s_hudt[8];
	char s_hudr[8];

	// 8008d500
	int unknownWhatThisIs;

	// 8008d504
	// for non-7lap-races up to 9:59:99
	char raceClockStr[8];

	char s_Ln[4]; // L1, L2, L3...


	char s_int[4];         // %d
	char s_intDividing[8]; // %d/%d
	char s_printDividing[4];
	char s_longInt[4];
	char s_spacebar[4];
	char s_x[4]; // for wumpa counter

	// 8008d52c
	int framesSinceRaceEnded;

	// 8008d530

	// same stuff, different order
	char s_subtractLongInt[8];
	char s_additionLongInt[8];
	char s_subtractInt[4];
	char s_999[8];
	char s_intSpace[4];
	char s_str[4];

	// 8008d554
	// one for each player
	int multiplayerWumpaHudData[4];

	// 8008d564
	struct
	{
		// 8008d564
		// one byte per player
		// Battle and VS
		char boolPressX[4];

		// 8008d568
		// Normal 3P/4P positions are copied from retail data,
		// while 2P positions are stored in RAM.
		SVec2 textPos2P[2];

	} Battle_EndOfRace;

	// 8008d570
	// end of race Arcade Adventure
	// counts 1 - 8 over a few seconds
	s16 numIconsEOR;

	// 8008d572
	s16 unknown_8d572;

	// 8d574
	char s_additionInt[4];

	// 8008D578
	struct
	{
		// 8008D578
		// easy, medium, hard, null
		s16 CupCompletion_curr[4];

		// 8008D580
		s16 CupCompletion_prev[4];

		// 8008D588
		// parking lot, north bowl, lab basement
		s16 UnlockBattleMap[4];
	} UnlockBitIndex;

	// 8008D590
	int matrixTableBaked;

	// 8008D594
	char s_NOSCRUB[8];
	char s_ROAD[8];
	char s_SANDBAG[8];
	char s_RUBBER[8];
	char s_SOILD[8];
	char s_mud[4];
	char s_metal[8];
	char s_none[8];
	char s_snow[8];
	char s_track[8];
	char s_ice[4];
	char s_stone[8];
	char s_water[8];
	char s_wood[8];
	char s_grass[8];
	char s_dirt[8];
	char s_asphalt[8];
	char s_player[8];
	char s_turbo1[8];
	char s_turbo2[8];
	char s_doctor1[8];
	char s_bomb1[8];
	char s_shield[8];
	char s_nitro1[8];
	char s_tnt1[8];
	char s_beaker1[8];

	// 8008d65c
	u8 boolIsMaskThreadAlive;
	u8 boolIsMaskThreadAlive_pad[3];

	// 8008d660
	char s_head[8];

// This prevents the EXE file from getting
// bloated with zeros, bss gets zero'd from
// entry function of the game
#ifndef NO_BSS

	// BSS is still addressed by $gp,
	// so they share SDATA struct,

	// ===== BSS Region ========

	// 8008d668
	// used for RNG
	struct RngDeadCoedState advRng;

	// 8008d670
	// Unused path-file index
	int lastPathIndex;

	// 8008d674
	// whoever leads out of all human drivers,
	// even if that person is not winning the race,
	// pointer to that driver goes here
	struct Driver *bestHumanRank;

	// 8008d678
	s16 *difficultyParams[2];

	// 8008d680
	// if these are all zero, all AIs
	// will reach top speed after race starts at same time
	u8 accelerateOrder[8];

	// 8008d688
	struct NavFrame *nav_ptrFirstPoint;

	// 8008d68c
	struct NavFrame *nav_ptrLastPoint;

	// 8008d690
	// whoever leads out of all AI drivers,
	// even if that AI is not winning the race,
	// pointer to that driver goes here
	struct Driver *bestRobotRank;

	// 8008d694
	int nav_NumPointsOnPath;

	// 8008d698
	int aiCollisionDelayFrameCount;

	// 8008d69c
	char kartSpawnOrderArray[0x8];

	// 8008d6a4
	char unk_paddingAfterKartSpawn[0x8];

	// 8008d6ac
	// stream audio, or stream data
	int discMode;

	// ----------------------------

	// 8008d6b0
	// 0 for parallel port
	// 1 for CD
	int boolUseDisc;

	// 8008d6b4
	int bool_XnfLoaded;

	// 8008d6b8
	// = 0, most of the time
	// = 1, finished, set on IRQ
	int XA_boolFinished;

	// 8008d6bc
	int XA_MaxSampleIndex;

	// 8008d6c0
	int XA_MaxSampleNumSaved;

	// 8008d6c4
	int irqAddr;

	// 8008d6c8
	int XA_VolumeDeduct;

	// 8008d6cc
	int *ptrArray_numSongs;

	// 8008d6d0
	int *ptrArray_firstSongIndex;

	// 8008d6d4
	int XA_CurrPos;

	// 8008d6d8
	int countPass_CdTransferCallback;

	// 8008d6dc
	int xa_numTypes;

	// 8008d6e0
	int XA_Playing_Index; // 0 - 99

	// 8008d6e4
	int XA_Playing_Category; // 0 - 4

	// 8008d6e8
	struct XaSize *ptrArray_XaSize;

	// 8008d6ec
	int XA_PauseFrame;

	// 8008d6f0
	int XA_StartPos;

	// 8008d6f4
	int XA_MaxSampleValInArr;

	// 8008d6f8
	int XA_CurrOffset;

	// 8008d6fc
	int *ptrArray_NumXAs;

	// 8008d700
	int unused_8008d700;

	// 8008d704
	int countPass_CdReadyCallback;

	// 8008d708
	XAState XA_State;

	// 8008d70c
	int XA_VolumeBitshift;

	// 8008d710
	int XA_EndPos;

	// 8008d714
	int XA_MaxSampleVal;

	// 8008d718
	int *ptrArray_XaCdPos; // maybe should be `struct XaSize*`?

	// 8008d71c
	int *ptrArray_firstXaIndex;

	// 8008d720
	// count fails of CdSyncCallback
	int countFail_CdSyncCallback;

	// 8008d724
	// count fails of CdReadyCallback
	int countFail_CdReadyCallback;

	// 8008d728
	// adv flags that control door access
	// & 1 for garage door + tiger temple
	// & 2 for hub doors
	// see 8001fc40
	u32 doorAccessFlags;

	// 8008d72c
	// All these are related drawing debug strings
	struct
	{
		u32 u;
		u32 v;
		u16 clut;
		u16 tpage;
	} debugFont;

	// 8008d738
	struct HighScoreEntry *ptrActiveHighScoreEntry;

	// 8008d73C
	int unk_8008d73C_relatedToRowHighlighted;

	// 8008d740
	b16 boolGhostsDrawing;
	u16 padGhostsDrawing;

	// 8008d744
	b16 boolGhostTooBigToSave;
	u16 padGhostTooBigToSave;

	// 8008d748
	s16 ghostOverflowTextTimer;
	u16 padGhostOverflowTextTimer;

	// 8008d74c
	struct GhostTape *ptrGhostTape[2];

	// 8008d754
	struct GhostHeader *ptrGhostTapePlaying;

	// 8008d758
	b16 boolCanSaveGhost;
	u16 padCanSaveGhost;

	// 8008d75c
	int countSounds;

	// 8008d760
	u16 curReverb;
	u16 padCurReverb;

	// 8008d764
	int audioAllocSize;

	// 8008d768
	int numAudioSectors;

	// 8008d76c
	int numAudioBanks;

	// 8008d770
	// spu address of some type
	int audioAllocPtr;

	// 8008d774
	int bankLoadStage;

	// 8008d778
	int bankSectorOffset;

	// 8008d77C
	int bankFlags;

	// 8008d780
	struct Bank *ptrLastBank;

	// 8008d784
	struct SampleBlockHeader *ptrSampleBlock1;

	// 8008d788
	struct SampleBlockHeader *ptrSampleBlock2;

	// 8008d78c
	int criticalSectionCount;

	// 8008d790
	int numBackup_ChannelStats;

	// 8008d794 -- both OptionsSliders related
	int OptionSlider_BoolPlay;
	int OptionSlider_Index;

	// 8008d79c
	s16 currentVolume;

	// 8008d79e
	u8 storedVolume;

	// 8008d79f
	char boolStoringVolume;

	// 8008d7a0
	int OptionSlider_soundID;

	// 8008d7a4
	int songLoadStage;

	// 8008d7a8
	int songSectorOffset;

	// 8008d7ac
	u8 vol_FX;
	u8 _pad_vol_FX[3];

	// 8008d7b0
	struct CseqHeader *ptrCseqHeader;

	// 8008d7b4
	s16 *ptrCseqSongStartOffset;

	// 8008d7b8
	u8 vol_Music;
	u8 padding_afterVolMusic[3];

	// 8008d7bc
	u8 vol_Voice;
	u8 _pad_vol_Voice[3];

	// 8008d7c0
	struct HowlHeader *ptrHowlHeader;

	// 8008d7c4
	struct SampleDrums *ptrCseqShortSamples;

	// 8008d7c8
	// NOTE(aalhendi): Raw byte buffer of parsed CSEQ song data.
	// Indexed by ptrCseqSongStartOffset[] (byte offsets), then cast to CseqSongHeader* at use sites.
	char *ptrCseqSongData;

	// 8008d7cc
	u8 boolStereoEnabled;
	u8 _pad_boolStereoEnabled[3];

	// 8008d7d0
	struct EngineFX *howl_metaEngineFX;

	// 8008d7d4
	int howl_endOfHowl;

	// 8008d7d8
	struct OtherFX *howl_metaOtherFX;

	// 8008d7dc
	struct SpuAddrEntry *howl_spuAddrs;

	// 8008d7e0
	u16 *howl_songOffsets;

	// 8008d7e4
	u16 *howl_bankOffsets;

	// 8008d7e8
	struct SampleInstrument *ptrCseqLongSamples;

	// 8008d7ec
	char boolCanPlayVoicelines;

	// 8008d7ed
	char boolCanPlayWrongWaySFX;

	// 8008d7ee
	s16 voicelineCooldown;

	// 8008d7f0
	AudioState audioState;

	// 8008d7f2
	s16 desiredXA_RaceIntroIndex;

	// 8008d7f4
	// unused, would have changed the Final Lap XA,
	// (at least assumed? its between Intro and End?)
	int desiredXA_FinalLapIndex;

	// 8008d7f8
	s16 desiredXA_RaceEndIndex;

	// 8008d7fa
	s16 nTropyVoiceCount;

	// 8008d7fc
	int boolNeedXASeek;

	// 8008d800 -- end of sData (due to alignment)
	int bankCount;

	// 8008d804
	int bankPodiumStage;

	// 8008d808 (1)
	// Loads bank 54 (0x36)
	int bankLoad54;

	// 8008d80c
	int cseqBoolPlay;

	// 8008d810
	// definitely unsigned, we can tell from assembly
	// in Music_LowerVolume and Music_RaiseVolume
	u32 cseqHighestIndex;

	// 8008d814
	int cseqTempo;


	// 8008d818 -- FUN_8002dc4c
	int audioDefaults[9]; // maybe should be `char*[]` instead of `int[]`

	// 8008d818
	// array of 8 (ai engine?)

	// 8008d820
	// array of 8

	// 8008d828
	// array of 8

	// 8008d830
	// 8008d835 audio state

	// 8008d83c usaRetil
	s16 levelLOD;
	s16 levelID;

	// 8008d840
	// need to rename, can be -1, 0, 1
	int howlChainState;

	// 8008d844
	// save parameters so you can
	// call the function over and over
	int howlChainParams[4];

	// 8008d854
	void *ptrHubAlloc;

	// 8008d858
	void *lngFile;

	// 8008d85c

	// Not used in decomp/General,
	// replaced 3-part callback with 2-part callback
	void (*callbackCdReadSuccess)(struct LoadQueueSlot *);

	// 8008d860
	// lock to zero, mask wont appear to give hints
	struct Instance *instMaskHints3D;

	// 8008d864
	int boolOpenWheelConfig;

	// 8008d868
	int WheelConfigOption;

	// 8008d86C
	// same as ptrBigfileCdPos_2
	struct BigHeader *ptrBigfile1;

	// 8008d870
	// ptr to array of model pointers (real ND name)
	int **PLYROBJECTLIST; // maybe should be `struct Model**`

	// 8008d874
	// activated in FUN_80035e20,
	// search for FUN_800b3f88,
	// determines if Aku is talking, to disable
	// on-screen text, or delay track loading
	s16 AkuAkuHintState;
	s16 padding_AkuAkuHintState;

	// 8008d878
	char **lngStrings;

	// 8008d87c
	int gamepadID_OwnerRaceWheelConfig;


	// 8008d880
	RECT videoSTR_src_vramRect;

	// 8008d888
	// used by 232, talking mask that gives hints
	s16 boolDraw3D_AdvMask;
	s16 padding_boolDraw3D_AdvMask;

	// 8008d88c
	// error message posY choice (0,1,2) (high, mid, low)
	s16 errorMessagePosIndex;
	s16 unk_aftererrormessageposindex;

	// 8008d890
	int unk_8008d890;

	// 8008d894
	int raceWheelConfigOptionIndex;

	// 8008d898
	int raceWheelConfigPageIndex;

	// 8008d89c
	int videoSTR_dst_vramX;

	// 8008d8a0
	// always changing cause of
	// where each swapchain image is in RAM
	int videoSTR_dst_vramY;

	// 8008d8a4
	int boolSoundPaused;

	// 8008d8a8
	int memcardFileSize;

	// 8008d8ac
	int memoryCard_SizeRemaining;

	// 8008d8b0
	int crc16_checkpoint_byteIndex;

	// 8008d8b4
	int HwCARD_EvSpERROR;

	// 8008d8b8
	int memcardSlot; // hard-coded ZERO

	// 8008d8bc
	int HwCARD_EvSpNEW;

	// 8008d8c0
	int SwCARD_EvSpNEW;

	// 8008d8c4
	int SwCARD_EvSpERROR;

	// 8008d8c8
	int HwCARD_EvSpIOE;

	// 8008d8cc
	u32 memcardStatusFlags;

	// 8008d8d0
	int crc16_checkpoint_status;

	// 8008d8d4
	int SwCARD_EvSpIOE;

	// 8008d8d8
	// always 256
	int memcardIconSize;

	// 8008d8dc
	int HwCARD_EvSpTIMOUT;

	// 8008d8e0
	int SwCARD_EvSpTIMOUT;


	// 8008d8e4 - boss weapon cooldown (after getting damage)
	// 8008d8e8 - pointer to boss weapon meta

	// 8008d8e4
	s16 bossWeaponCooldown;
	s16 padding_8008d8e6;
	struct MetaDataBOSS *bossWeaponMeta;
	char data10_aaaaa[4];

	// 8008d8f0
	b16 boolHasLoadedOptions;

	u16 typeTimer;

	// 8008d8f4
	int ptrLoadSaveObj;

	// 8008d8f8

	// ---
	// 8008d904 - timerSaveComplete
	// 8008d906 - submitNameMode
	// --

	struct SelectProfileRuntimeState selectProfileState;

	// 8008d908
	struct RectMenu *ptrActiveMenu;

	// 8008d90c
	// Never used to detect dead menu
	s16 framesRemainingInMenu;
	s16 padding_8008d90e;

	// 8008d910
	int unk_8008d910;

	// 8008d914
	s16 advCharSelectIndex_curr;
	s16 padding_8008d916;

	// 8008d918
	b16 boolSaveCupProgress;
	s16 _pad_boolSaveCupProgress;

	// 8008d91c
	s16 advCharSelectIndex_prev;
	s16 padding_8008d91e;

	// 8008d920
	u16 uselessLapRowCopy;
	s16 padding_8008d922;

	// 8008d924
	// Becomes nullptr after ptrActiveMenu is set
	struct RectMenu *ptrDesiredMenu;

	// 8008d928
	char unk_memcardRelated_8008d928[0x8];

	// 8008d930
	u16 trackSelBackup;
	s16 padding_8008d932;

	// 8008d934
	// one bit for each player that has
	// pressed X to select a character
	u32 characterSelectFlags;

	// 8008d938
	// next is 0x236 or 0x237
	// Must be some unused placeholder,
	// both 0x236 and 0x237 are the same
	// "Save your cup progress?"
	s16 stringIndexSaveCupProgress;
	s16 _pad_stringIndexSaveCupProgress;

	// 8008d93c
	int unk_lala[2];

	// 8008d944
	// 0 for outdated
	// 1 for this version
	int boolMemcardDataValid;

	// 8008d948
	Color menuRowHighlight_Green;

	// 8008d94c
	Color menuRowHighlight_Normal;

	// 8008d950
	int AnyPlayerTap;

	// 8008d954
	struct RectMenu *activeSubMenu;

	// 8008d958
	b16 boolReplayHumanGhost;
	s16 padding_8008d95a;

	// 8008d95c
	int unk8008d95c;

	// 8008d960
	s8 teamOfEachPlayer[4];

	// 8008d964
	int unk8008d964;

	// 8008d968
	int boolAdvProfilesChecked;

	// 8008d96c
	int advProfileIndex;

	// 8008d970
	// frame timer for color animation:
	//	- flashing menu rows
	//	- main menu character icon border
	//	- main menu character window border
	//	etc
	int frameCounter;

	// 8008d974
	int AnyPlayerHold;

	// 8008d978
	// saving, loading, or deleting
	int memcardAction;

	// 8008d97C
	MainMenuState mainMenuState;

	// 8008d980
	// 0 - 7
	int demoModeIndex;

	// 8008d984
	int boolError;

	// 8008d988
	// root counter
	int rcntTotalUnits;

	// 8008d98C
	u32 flags_timeTrialEndOfRace;

	// 8008d990
	int wumpaShineTheta;

	// 8008d994
	int wumpaShineResult;

	// 8008d998
	u8 wumpaShineColor1[3][4];
	u8 wumpaShineColor2[3][4];

	// 8008d9b0
	int relicTime_1min;

	// 8008d9b4
	struct Instance *ptrRelic;


	// 8008d9b8
	int relicTime_1ms;

	// 8008d9bc
	struct Instance *ptrHudCrystal;

	// 8008d9c0
	struct Instance *ptrMenuCrystal;

	// 8008d9c4
	struct Instance *ptrHudT;

	// 8008d9c8
	struct Instance *ptrHudR;

	// 8008d9cc
	struct Instance *ptrHudC;

	// 8008d9d0
	struct Instance *ptrToken;

	// 8008d9d4
	int relicTime_10ms;


	// 8008d9d8
	struct Instance *ptrTimebox1;

	// 8008d9dc
	int WrongWayDirection_bool;

	// 8008d9e0
	int relicTime_10sec;

	// 8008d9e4
	int framesDrivingSameDirection;

	// 8008d9e8
	int relicTime_1sec;

	// 8008d9ec
	SVec3 botCrashNavRot;
	s16 botCrashNavRotPadding;

	// 8008d9f4
	int vehicleCollisionImpactStrength;
	int talkMaskXASamplePeak;
	int talkMaskMaxMouthFrame;

	// 8008da00
	u8 talkMask_boolDead;
	u8 talkMask_boolDead_pad[3];

	// 8008da04
	// return address, from entry() back to bios,
	// entry func might be called start() in ghidra output
	int entryRA;

	// 8008da08
	MATRIX rotXYZ;

	// 8008da28
	MATRIX rotXZ;

	// 8008da48
	// pointer at 8008cf70
	s16 arcadeDiff[0xE];

	// 8008da64
	// pointer at 8008cf74
	s16 cupDiff[0xE];

	// 8008da80
	struct NavHeader blank_NavHeader;
	struct NavFrame blank_NavFrame;

	// 8008dae0
	struct NavFrame *NavPath_ptrNavFrameArray[3];

	// 8008daec
	struct NavHeader *NavPath_ptrHeader[3];

	// 8008daf8
	struct LinkedList navBotList[3];

	// 8008DB1C - BSP Search Result
	struct ScratchpadStruct scratchpadStruct;
	// 8008db40: search flags
	// 8008db48: lev -> mesh_info


	// 8008dD28
	// see FUN_8001c8e4
	char SpuDecodedBuf[0x800];

	// 8008E528
	// 400 bytes, 100 ints
	int unused400[100];

	// 8008e6b8 (half of a CdlFILE struct)
	CdlLOC cdlFile_CdReady[4];

	// 8008e6c8 array of 3 ints
	int XA_MaxSampleValArr[3];


	// 8008e6d4
	DISPENV blank_debug_DispEnv;


	// 8008e6e8: progress (0x1494 bytes), then options at 8008fb7c.
	struct GameSave gameSave;

	// double-check these, address starts
	// 0x18 bytes from "name", not 0x20
	// 8008fba4
	// Adv Profile system
	struct AdvProgress advProgress;

	// 8008fbf4
	// Ghost system
	struct GhostRecording
	{
		// 8008fbf4
		// Start of entire ghost,
		// first byte of header
		struct GhostHeader *ptrGhost;

		// 8008fbf8
		// beginning of recording buffer,
		// after ghost header
		char *ptrStartOffset;

		// 8008fbfc
		// max address a ghost can record to
		char *ptrEndOffset;

		// 8008fc00
		// current "end", where you append buffer
		char *ptrCurrOffset;

		// DAT_8008fc04
		// Emit a movement sample every eight frames.
		s32 frameCount;

		// DAT_8008fc08
		// Emit an absolute position at least every 32 movement samples.
		s32 sampleCount;

		// 8008fc0c
		s32 lastPositionPacketTimeMS;

		// 8008fc10
		s32 timeElapsedInRace;

		// 8008fc14
		s32 turboPadBoostCooldownFrames;

		// 8008fc18
		s16 lastSampledPositionX;

		// 8008fc1a
		s16 lastSampledPositionY;

		// 8008fc1C
		s16 lastSampledPositionZ;

		// 8008fc1e
		s16 unk_8fc1e;

		// 8008fc20
		s32 animFrame;
		s32 animIndex;
		u32 instanceFlags;

	} GhostRecording;

	// 8008fc2C
	struct Bank bank[8];

#define NUM_SFX_CHANNELS 24

	// 8008fc6c
	// & 0x01 - set status to OFF
	// & 0x02 - set status to ON
	// & 0x04 - startAddr
	// & 0x08 - ADSR
	// & 0x10 - pitch
	// & 0x20 - reverb voice
	// & 0x40 - volume
	u32 ChannelUpdateFlags[NUM_SFX_CHANNELS];

	// 8008fccc
	// backed up every time Curr changes
	struct ChannelAttr channelAttrNew[NUM_SFX_CHANNELS];

	// 8008fe4c
	// backed up every time Curr changes
	struct ChannelAttr channelAttrCur[NUM_SFX_CHANNELS];

	// 8008FFCC
	// backed up during pause
	struct ChannelStats channelStatsCurr[NUM_SFX_CHANNELS];

	// 800902cc
	struct SongSeq songSeq[NUM_SFX_CHANNELS];

	// 8009056C
	// ChannelStats (allocates to prev?)
	struct LinkedList channelTaken;

	// 80090578
	// ChannelStats (allocates to prev?)
	struct LinkedList channelFree;

	// 80090584
	// destinations for loading HWL
	// exactly 1 SampleBlock here
	char sampleBlock1[0x800];

	// 80090d84, 0x5000 bytes
	char tenSampleBlocks[10 * 0x800];

	// 80095D84
	// one for each cseq song, each can be
	// in either slot, always check SongType
	// - level song / game aku
	// - menu aku song
	struct Song songPool[2];

	// 80095e7c
	CdlFILE KartHWL_CdFile;

	// 80095e94
	// backed up during pause
	struct ChannelStats channelStatsPrev[NUM_SFX_CHANNELS];

	// ==================================

	// 80096194 -- next variable
	int backupParams_FUN_8002cf28[4];

	// 800961a4
	struct LinkedList Voiceline1;

	// 800961b0
	int unk_hole1;

	// 800961b4
	struct LinkedList Voiceline2;

	// 800961c0
	int unk_hole2;

	// 800961c4
	// eight members, 0x10 each
	struct VoicelineItem voicelinePool[8];

	// 80096244
	int timeSet1[0x10];

	// 80096284
	int timeSet2[0x10];

	// 800962c4 and 800962d4
	struct SoundFadeInput SoundFadeInput[2];

	// 800962E4
	// eight members, each 0xc
	struct GarageFX garageSoundPool[8];

	// 80096344
	struct LoadQueueSlot queueSlots[LOAD_QUEUE_SLOT_COUNT];

	// 80096404
	// filler


	// 64 quadblocks per player, 256 total
	struct QuadBlock *quadBlocksRendered[0x100];


	// 80096804
	struct GamepadSystem gamepadSystem;

	// 80096b20
	struct GameTracker gameTracker;

	// 800990A4
	char s_memcardFileCurr[0x20];

	// 800990c4
	char s_memcardFindGhostFile[0x20];

	// 800990E4
	struct Mempack mempack[4]; // each is 0x60 bytes

	// 80099264 (time string at end of time trial race)
	char ghostStrTrackTime[0x20];

	// 80099284
	char ghostFileNameFinal[0x20];

	// 800992a4
	char memcardIcon_HeaderGHOST[0x40];

	// 800992E4
	// literally every byte of memory card,
	// dumped into RAM, right here
	char memcardBytes[0x1680];

	// 8009A964
	int buttonHeldPerPlayer[4];

	// 8009A974
	s16 battleSettings[6];

	// 8009A980
	s16 characterIDs_backup[8];

	// 8009A990
	u32 buttonTapPerPlayer[4];

	// 8009A9A0
	// 4 profiles, 3 instances per profile, 0x90 bytes total
	struct SelectProfileLoadSaveIcon LoadSaveData[12];

	// 0x8009AA30
	// & 1: frame2->frame1
	int memcardUnk1;

	// result

	// 8009AA34 (MC_RESULT)
	s16 desired_memcardResult;

	// 8009aa36 (MC_ACTION)
	s16 frame1_memcardAction;

	// 8009aa38
	s16 frame1_memcardSlot;

	// 8009aa3a
	s16 frame2_memcardAction;

	// 8009aa3c
	s16 frame2_memcardSlot;

	// 8009aa3e
	s16 frame3_memcardAction;

	// 8009aa40
	s16 frame3_memcardSlot;

	// 8009aa42
	s16 frame4_memcardAction;

	// 8009aa44
	s16 frame4_memcardSlot;

	s16 padding8009aa46;

	// 8009aa48
	char *ghostProfile_fileName;

	// 8009aa4c
	char *ghostProfile_fileIconHeader;

	// 8009aa50
	// Points to Destination (ghost load)
	// Points to Source (ghost save)
	struct GhostHeader *ghostProfile_ptrGhostHeader;

	// 8009aa54 -- Size (saving = 3E00)
	s16 ghostProfile_size3E00;

	// 8009aa56
	// only set for one frame,
	// then resets to -1
	s16 ghostProfile_rowSelect;

	// 8009aa58
	s16 ghostProfile_indexSave;

	// 8009aa5a
	s16 ghostProfile_indexLoad;

	// 8009aa5c
	int numGhostProfilesSaved;

	// 8009aa60
	struct GhostProfile ghostProfile_memcard[7];

	// 8009ABCC
	struct GhostProfile ghostProfile_current;

	// 8009AC00
	struct
	{
		// 8009AC00
		// 0x:00:00
		int p1_Min1s[7];
		int p2_Min1s[7];

		// 8009AC38
		// 00:00:x0
		int p1_Ms10s[7];
		int p2_Ms10s[7];

		// 8009ac70
		// 00:00:0x
		int p1_Ms1s[7];
		int p2_Ms1s[7];

		// 8009aca8
		// 00:x0:00
		int p1_Sec10s[7];
		int p2_Sec10s[7];

		// 8009ace0
		// 00:0x:00
		int p1_Sec1s[7];
		int p2_Sec1s[7];

	} LapTimes;

	// 8009ad18 -- 1c34 from mempack

	// 8009AD18 - pushBuffer_DecalMP
	struct PushBuffer pushBuffer_DecalMP;

#if 0
	// 8009AE28
	// first byte after PushBuffer,
	// used by FUN_8005d0d0 for collision?

	// 8009ae38 used as rotation vector

	// 8009AE58 start of psyq lib data
#endif

	// 8009AE28
	char dataLibFiller[0x30];

	// after dataLibFiller is 8009AE58,
	// where psyq allocates globals

	// Address of Crystal Challenge end of race
	// 8009f710

	// 8009f6fc end of BSS

// NO_BSS
#endif
};

// ".rData"
// ram:80010000-ram:800123df
// constant, initialized by compiler
// switch jmp pointers, string parameters, etc
extern struct rData rdata;

// .text
// ram:800123e0-ram:8008099f
// assembly code

// ".data"
// ram:800809a0-ram:8008cf6b
// hand-initialized global objects
extern struct Data data;

// ".sdata", "$gp" register
// ram:8008cf6c-ram:0x8008d667 (aligned to 8008d7ff)
// non-object globals (int, pointer, etc)
extern struct sData sdata_static;

// BSS is not a part of the EXE file,
// it is allocated into RAM at startup
// 0x8008d668 - 0x8009f6fc
extern struct BSS bss;

#ifndef CTR_NATIVE
// optimal use for modding
register struct sData *sdata asm("gp");
#else
struct sData *sdata = &sdata_static;
#endif

CTR_STATIC_ASSERT(sizeof(struct MetaDataCHAR) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct MetaDataCHAR, iconID) == 0x8);
CTR_STATIC_ASSERT(sizeof(((struct MetaDataCHAR *)0)->iconID) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct MetaDataCHAR, engineID) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct sData, numIconsEOR) == 0x604);
CTR_STATIC_ASSERT(sizeof(((struct sData *)0)->numIconsEOR) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct sData, s_additionInt) == 0x608);

// OVR1
// 8009f6fc - 800a0cb8

// OVR2
// 800a0cb8 - 800ab9f0

// OVR3
// 800ab9f0 - 800ba9f0

// MEMPACK
// 800ba9f0 - 801ff800

// SP
// 801ff800 - 80200000

CTR_STATIC_ASSERT(sizeof(struct Terrain) == 0x40);
CTR_STATIC_ASSERT(offsetof(struct Terrain, speedMultiplier) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct Terrain, slowUntilSpeed) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct Terrain, counterSteerRatio) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct Terrain, turnLeanScale) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct Terrain, groundFrictionScale) == 0x20);
CTR_STATIC_ASSERT(offsetof(struct Terrain, turnAngleScale) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct Terrain, turnResponseScale) == 0x28);
CTR_STATIC_ASSERT(offsetof(struct Terrain, skidSound) == 0x30);
CTR_STATIC_ASSERT(offsetof(struct Terrain, bot.fields.speedFlags) == 0x36);
CTR_STATIC_ASSERT(offsetof(struct Terrain, bot.fields.targetSpeedScale) == 0x38);
CTR_STATIC_ASSERT(offsetof(struct Terrain, bot.fields.accelerationScale) == 0x3a);
CTR_STATIC_ASSERT(offsetof(struct Terrain, botFrictionScale) == 0x3c);
CTR_STATIC_ASSERT(offsetof(struct Terrain, padding_0x3e) == 0x3e);
CTR_STATIC_ASSERT(sizeof(((struct Terrain *)0)->bot.fields.speedFlags) == 0x2);
CTR_STATIC_ASSERT(sizeof(((struct Terrain *)0)->bot.fields.targetSpeedScale) == 0x2);
CTR_STATIC_ASSERT(sizeof(((struct Terrain *)0)->bot.fields.accelerationScale) == 0x2);
CTR_STATIC_ASSERT(sizeof(((struct Terrain *)0)->botFrictionScale) == 0x2);
CTR_STATIC_ASSERT(sizeof(struct Scrub) == 0x10);
CTR_STATIC_ASSERT(sizeof(ScrubFlags) == 0x4);
CTR_STATIC_ASSERT(sizeof(TerrainFlags) == 0x4);
CTR_STATIC_ASSERT(sizeof(TerrainBotFlags) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct Scrub, flags) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct Scrub, speedLimit) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct Scrub, impactAngle) == 0xC);
CTR_STATIC_ASSERT(sizeof(struct MetaDataLEV) == 0x18);
CTR_STATIC_ASSERT(sizeof(struct MetaDataMODEL) == 0xC);

#define OFFSETOF_SDATA(ELEMENT) ((u32)0x8008cf6c + OFFSETOF(struct sData, ELEMENT))
#define OFFSETOF_DATA(ELEMENT)  ((u32)0x800809a0 + OFFSETOF(struct Data, ELEMENT))

CTR_STATIC_ASSERT(OFFSETOF_DATA(rowsQuit) == 0x800841BC);
CTR_STATIC_ASSERT(OFFSETOF_DATA(menuQuit) == 0x800841D0);
CTR_STATIC_ASSERT(OFFSETOF_DATA(playerIconAdvMap) == 0x80086418);
CTR_STATIC_ASSERT(OFFSETOF_SDATA(AkuAkuHintState) == 0x8008D874);
CTR_STATIC_ASSERT(OFFSETOF_SDATA(lngStrings) == 0x8008D878);
CTR_STATIC_ASSERT(OFFSETOF_SDATA(botCrashNavRot) == 0x8008D9EC);
CTR_STATIC_ASSERT(OFFSETOF_SDATA(vehicleCollisionImpactStrength) == 0x8008D9F4);
CTR_STATIC_ASSERT(OFFSETOF_SDATA(talkMaskXASamplePeak) == 0x8008D9F8);
CTR_STATIC_ASSERT(OFFSETOF_SDATA(talkMaskMaxMouthFrame) == 0x8008D9FC);

#endif
