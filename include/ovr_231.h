#ifndef CTR_NATIVE_OVR_231_H
#define CTR_NATIVE_OVR_231_H


// used all over 231,
// Plant, FlameJet, etc
struct HitboxDesc
{
	// check collision
	struct Instance *inst;
	struct Thread *thread;
	struct Thread *bucket;
	struct BoundingBox bbox;

	// post collision
	struct Thread *threadHit; // from bucket
	void *funcThCollide;
};

struct MaskHeadScratch
{
	// 0x1f800108
	MATRIX m;

	// 0x1f800128
	SVec3 rot;
	s16 _pad_rot;

	// 0x1f800130
	SVec3 posOffset;
	s16 _pad_posOffset;
};

struct MaskHeadWeapon
{
	// 0x0
	SVec3 rot;

	// 0x6
	s16 duration;

	// 0x8
	struct Instance *maskBeamInst;

	// 0xC
	SVec3 pos;

	// 0x12
	s16 scale;

	// 0x14 bytes large
};
CTR_STATIC_ASSERT(sizeof(struct MaskHeadWeapon) == 0x14);

enum
{
	MASK_HEAD_DURATION_NORMAL = 0x1e00,
	MASK_HEAD_DURATION_JUICED = 0x2d00,
	MASK_HEAD_SCALE_NORMAL = FP(1.0),
	MASK_HEAD_ROT_WORLD_SPACE = 0x1,
};


typedef u16 TrackerWeaponFlags;

enum
{
	TRACKER_FLAG_POWERED_UP = 0x0001,
	TRACKER_FLAG_WARPBALL_TARGET_PATH = 0x0004,
	TRACKER_FLAG_WARPBALL_FALLBACK_PATH = 0x0008,
	TRACKER_FLAG_WARPBALL_MASK_REPATH = 0x0010,
	TRACKER_FLAG_BOMB_BACKWARD = 0x0020,
	TRACKER_FLAG_WARPBALL_HIT_DRIVER = 0x0040,
	TRACKER_FLAG_WARPBALL_TURN_AROUND = 0x0100,
	TRACKER_FLAG_WARPBALL_BACKTRACKING = 0x0200,

	TRACKER_FLAG_WARPBALL_PATH_MODE = TRACKER_FLAG_WARPBALL_TARGET_PATH | TRACKER_FLAG_WARPBALL_FALLBACK_PATH,
	TRACKER_FLAG_WARPBALL_TARGET_REFRESH_BLOCKED = TRACKER_FLAG_WARPBALL_TARGET_PATH | TRACKER_FLAG_WARPBALL_BACKTRACKING,
};


// bomb, missile, warpball
// yes, the bomb chases you and steers towards targets
struct TrackerWeapon
{
	// 0x0
	struct Driver *driverTarget; // being chased

	// 0x4
	struct Driver *driverParent; // who shot me

	// 0x8
	struct Instance *instParent; // of driver who shot me

	// 0xC
	struct Particle *ptrParticle;

	// 0x10
	SVec3 vel;

	// 0x16
	TrackerWeaponFlags flags;

	// 0x18
	SVec3 dir;

	// 0x1e
	s16 rotY;

	// 0x20
	s16 parentSafetyFrames;
	s16 blindFrames;

	// 0x24
	u32 soundIDCount;

	// 0x28
	u32 distanceToTarget;

	// === This point and beyond is Warpball ===

	// 0x2c
	int pathProgress;

	// 0x30
	int fadeFrame;

	// 0x34
	// bitshift with driver->driverID
	// one bit for every driver hit
	u32 driversHit;

	// 0x38
	// distY from track floor
	int distFromGround;

	// 0x3C
	struct CheckpointNode *ptrNodeCurr;

	// 0x40
	struct CheckpointNode *ptrNodeNext;

	// 0x44
	u8 nodeCurrIndex;

	// 0x45
	u8 nodeNextIndex;

	// 0x46
	s16 padding;

	// 0x48
	int timeAlive;

	// 0x4c
	u32 savedPosXY;

	// 0x50
	s16 savedPosZ;

	// 0x52
	s16 turnAroundFrames;

	// 0x54
	int framesSeekTargetTnt;

	// 0x58
	int orbTimeAlive;

	// 0x5c bytes large
};
CTR_STATIC_ASSERT(offsetof(struct TrackerWeapon, pathProgress) == 0x2c);
CTR_STATIC_ASSERT(offsetof(struct TrackerWeapon, savedPosXY) == 0x4c);
CTR_STATIC_ASSERT(offsetof(struct TrackerWeapon, savedPosZ) == 0x50);
CTR_STATIC_ASSERT(sizeof(struct TrackerWeapon) == 0x5c);

struct RainLocal
{
	// 0x0
	struct RainLocal *next;
	struct RainLocal *prev;

	// 0x8
	int frameCount;

	// 0xC
	SVec3 scroll;
	s16 _pad_scroll;

	// 0x14
	SVec3 vel;
	s16 _pad_vel;

	// 0x1c
	SVec3 pos;
	s16 _pad_pos;

	// 0x24
	struct Instance *cloudInst;

	// 0x28 -- size
};
CTR_STATIC_ASSERT(offsetof(struct RainLocal, scroll) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct RainLocal, vel) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct RainLocal, pos) == 0x1c);
CTR_STATIC_ASSERT(sizeof(struct RainLocal) == 0x28);

struct RainCloud
{
	// 0x0
	struct RainLocal *rainLocal;

	// 0x4
	s16 timeMS;

	// 0x6
	RainCloudEffect effect;

	// size - 0x8
};
CTR_STATIC_ASSERT(offsetof(struct RainCloud, effect) == 0x6);
CTR_STATIC_ASSERT(sizeof(struct RainCloud) == 0x8);

typedef u16 ShieldFlags;

enum
{
	SHIELD_FLAG_POP_ON_DAMAGE = 0x1,
	SHIELD_FLAG_SHOOT = 0x2,
	SHIELD_FLAG_BLUE = 0x4,
	SHIELD_FLAG_CRASH_ATTACK = 0x8,
};

struct Shield
{
	// 0x0
	int animFrame;

	// 0x4
	s16 duration;

	// 0x6
	ShieldFlags flags;

	// 0x8
	struct Instance *instColor;

	// 0xC
	struct Instance *instHighlight;

	// 0x10
	SVec3 highlightRot;
	s16 highlightTimer;

	// 0x18
};
CTR_STATIC_ASSERT(offsetof(struct Shield, flags) == 0x6);
CTR_STATIC_ASSERT(sizeof(struct Shield) == 0x18);

struct MineWeapon;

struct WeaponSlot231
{
	// 0x0
	struct Item item;

	// 0x8
	struct MineWeapon *mineWeapon;
};
CTR_STATIC_ASSERT(sizeof(struct WeaponSlot231) == 0xc);

typedef u16 MineWeaponFlags;

enum
{
	MINE_WEAPON_FLAG_RED_BEAKER = 0x1,
	MINE_WEAPON_FLAG_THROWN = 0x2,
};

// Tnt, Nitro, Beaker
struct MineWeapon
{
	// 0x0
	struct Driver *driverTarget; // who hit me

	// 0x4
	struct Instance *instParent; // of driver who placed me

	// 0x8
	struct Instance *crateInst; // if colliding with one

	// 0xc
	SVec3 velocity;

	// 0x12
	// used by tnt, and potion_inAir
	s16 stopFallAtY;

	// 0x14
	// causes explosion if != 0,
	// this is how mine pool destroys oldest mine
	s16 boolDestroyed;

	// 0x16
	// animation frame (on head)
	s16 numFramesOnHead;

	// 0x18
	struct WeaponSlot231 *weaponSlot231;

	// 0x1C
	// relative to driver
	SVec3 deltaPos;

	// 0x22
	// how many more jumps until
	// the tnt flies off your head
	s16 jumpsRemaining;

	// 0x24
	// number of frames that mine can't hurt parent
	s16 parentSafetyFrames;

	// 0x26
	s16 tntSpinY;

	// 0x28
	MineWeaponFlags flags;

	// 0x2a
	s16 cooldown;
};
CTR_STATIC_ASSERT(offsetof(struct MineWeapon, flags) == 0x28);
CTR_STATIC_ASSERT(sizeof(struct MineWeapon) == 0x2c);

struct Baron
{
	// 0x0
	// for the baron plane
	char pad_00[0x6];

	// 0x6
	s16 unused06;

	// 0x8
	char pad_08[0x12];

	// 0x1a
	s16 unused1A;

	// 0x1c
	char pad_1C[0x6];

	// 0x22
	s16 unused22;

	// 0x24
	u32 soundID_flags;

	// 0x28
	// unused, for baron
	struct Instance *otherInst;

	// 0x2c
	s16 pointIndex;
	s16 footerPaddingUnused;

	// 0x30 bytes large
};
CTR_STATIC_ASSERT(offsetof(struct Baron, soundID_flags) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct Baron, pointIndex) == 0x2c);
CTR_STATIC_ASSERT(sizeof(struct Baron) == 0x30);

struct Blade
{
	int angle;

	// 0x4 bytes large
};
CTR_STATIC_ASSERT(sizeof(struct Blade) == 0x4);

struct Crate
{
	int cooldown;
	s16 boolPauseCooldown;
	s16 padding;

	// 0x8 bytes large
};

CTR_STATIC_ASSERT(sizeof(struct Crate) == 0x8);

struct Crystal
{
	SVec3 rot;
	s16 padding;

	// 0x8 bytes large
};
CTR_STATIC_ASSERT(sizeof(struct Crystal) == 0x8);

struct CtrLetter
{
	SVec3 rot;
	s16 padding;
	// 0x8 bytes large
};
CTR_STATIC_ASSERT(sizeof(struct CtrLetter) == 0x8);

struct StartBanner
{
	s16 unused;
	s16 numVertices;
	// 0x4 bytes large
};
CTR_STATIC_ASSERT(sizeof(struct StartBanner) == 0x4);

struct Armadillo
{
	// 0x0
	SVec3 rotCurr;

	// 0x6
	s16 velX;

	// 0x8
	SVec3 rotDesired;

	// 0xe
	s16 velZ;

	// 0x10
	// at 0x500, it stops, and turns around
	s16 timeRolling;

	// 0x12
	// unused
	s16 numFramesSpinning;

	// 0x14
	// starts at zero, rolls, gets to high number,
	// then rolls back, and number goes back to zero
	s16 distFromSpawn;

	// 0x16
	s16 spawnPosX;

	// 0x18
	s16 spawnPosZ;

	// 0 for one direction, 1 for the other
	// 0x1a
	s16 direction;

	// frames at edge, waiting to roll again
	// 0x1c
	s16 timeAtEdge;

	// 0x1e
	s16 padding1E;

	// 0x20 bytes large
};
CTR_STATIC_ASSERT(offsetof(struct Armadillo, direction) == 0x1a);
CTR_STATIC_ASSERT(sizeof(struct Armadillo) == 0x20);

struct Fireball
{
	// 0x0
	s16 cycleTimer;

	// 0x2
	s16 fireballID;

	// 0x4
	// puts fireballs on different cycles,
	// just like spiders
	s16 cooldown;

	// 0x6
	SVec3 rot_unused;

	// 0xC
	s16 velY;

	// 0xE
	// unused
	s16 direction;

	// 0x10 bytes large
};
CTR_STATIC_ASSERT(offsetof(struct Fireball, rot_unused) == 0x6);
CTR_STATIC_ASSERT(sizeof(struct Fireball) == 0x10);

struct FlameJet
{
	// 0x0
	int cycleTimer;

	// 0x4
	int dirX;

	// 0x8
	int dirZ;

	// 0xC
	// adds 0x100 per frame
	s16 unusedPhase;

	// 0xe
	s16 cooldown;

	// 0x10
	u32 soundIDCount;

	// 0x14 bytes large
};
CTR_STATIC_ASSERT(offsetof(struct FlameJet, unusedPhase) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct FlameJet, soundIDCount) == 0x10);
CTR_STATIC_ASSERT(sizeof(struct FlameJet) == 0x14);

struct Follower
{
	int frameCount;
	struct Driver *driver;
	struct Thread *mineTh;
	int backupTimesDestroyed;

	SVec3 realPos;
	s16 _pad_realPos;
};
CTR_STATIC_ASSERT(sizeof(struct Follower) == 0x18);

struct Fruit
{
	struct Driver *driver;
};
CTR_STATIC_ASSERT(sizeof(struct Fruit) == 0x4);

struct Minecart
{
	// 0x0
	// removed in decomp optimizations
	SVec3 posStart;

	// 0x6
	s16 betweenPoints_currFrame;

	// 0x8
	// removed in decomp optimizations
	SVec3 posEnd;

	// 0xe
	SVec3 dir;

	// 0x14
	int posIndex;

	// 0x18
	SVec3 rotCurr;

	// 0x1e
	s16 betweenPoints_numFrames;

	// 0x20
	SVec3 rotDesired;

	// 0x26
	s16 rotSpeed;

	// 0x28
	u32 soundIDCount;

	// 0x2c bytes large
};
CTR_STATIC_ASSERT(offsetof(struct Minecart, posIndex) == 0x14);
CTR_STATIC_ASSERT(sizeof(struct Minecart) == 0x2c);

struct Orca
{
	// 0x0
	SVec3 startPos;

	// 0x6
	s16 orcaID;

	// 0x8
	SVec3 endPos;

	// 0xE
	s16 cooldown;

	// 0x10
	SVec3 instDefRot;

	// 0x16
	s16 animIndex;

	// 0x18
	s16 unused[7];

	// 0x26
	s16 numFrames;

	// 0x28
	SVec3 pathDelta;

	// 0x2e
	s16 direction;

	// 0x30 bytes large
};
CTR_STATIC_ASSERT(offsetof(struct Orca, pathDelta) == 0x28);
CTR_STATIC_ASSERT(sizeof(struct Orca) == 0x30);

struct Plant
{
	// 0x0
	s16 cycleCount;

	// 0x2
	s16 cooldown;

	// 0x4
	// 0: left side of track
	// 1: right side of track
	s16 side;

	// 0x6
	s16 boolEatingPlayer;

	// 0x8 bytes large
};
CTR_STATIC_ASSERT(offsetof(struct Plant, side) == 0x4);
CTR_STATIC_ASSERT(sizeof(struct Plant) == 0x8);

struct Seal
{
	// 0x0
	SVec3 spawnPos;

	// 0x6
	s16 sealID;

	// 0x8
	SVec3 endPos;

	// 0xe
	s16 distFromSpawn;

	// 0x10
	SVec3 rotCurr;
	s16 padding16;

	// 0x18
	SVec3 rotDesired;

	// 0x1e
	s16 direction;

	// 0x20
	SVec3 turnAroundRot;

	// 0x26
	s16 numFramesSpinning;

	// 0x28
	SVec3 vel;
	s16 padding2e;

	// 0x30 bytes large
};
CTR_STATIC_ASSERT(offsetof(struct Seal, endPos) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct Seal, turnAroundRot) == 0x20);
CTR_STATIC_ASSERT(sizeof(struct Seal) == 0x30);

struct Snowball
{
	// 0x0
	SVec3 rot_unused;

	// 0x6
	s16 pointIndex;

	// 0x8
	s16 numPoints;

	// 0xA
	s16 snowID;

	// 0xC
	u32 soundIDCount;

	// 0x10 bytes large
};
CTR_STATIC_ASSERT(sizeof(struct Snowball) == 0x10);

struct Spider
{
	// 0x0
	// counts five times
	s16 animLoopCount;

	// 0x2
	s16 spiderID;

	// 0x4
	s16 delay;

	// 0x6
	s16 isNearRoof;

	// 0x8
	s16 unused;
	s16 padding;

	// 0xC
	struct Instance *shadowInst;

	// 0x10 bytes large
};
CTR_STATIC_ASSERT(offsetof(struct Spider, isNearRoof) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct Spider, delay) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct Spider, shadowInst) == 0xc);
CTR_STATIC_ASSERT(sizeof(struct Spider) == 0x10);

typedef s32 TeethDirection;

enum
{
	TEETH_DIRECTION_CLOSING = -1,
	TEETH_DIRECTION_IDLE = 0,
	TEETH_DIRECTION_OPENING = 1,
};

struct Teeth
{
	// 0x0
	TeethDirection direction;

	// 0x4
	// countdown starts when door
	// is fully open, door starts
	// closing when it is done
	int timeOpen;

	// 0x8 bytes large
};
CTR_STATIC_ASSERT(sizeof(struct Teeth) == 0x8);

typedef s16 TurtleDirection;
typedef s16 TurtleState;

enum
{
	TURTLE_DIRECTION_RISING = 0,
	TURTLE_DIRECTION_FALLING = 1,

	TURTLE_STATE_FULLY_DOWN = 0,
	TURTLE_STATE_NOT_FULLY_DOWN = 1,
};

struct Turtle
{
	// 0x0
	s16 timer;

	// 0x2
	TurtleDirection direction;

	// 0x4
	s16 turtleID;

	// 0x6
	s16 padding6;

	// 0x8
	TurtleState state;

	// 0xA
	s16 paddingA;

	// 0xC bytes large
};
CTR_STATIC_ASSERT(offsetof(struct Turtle, turtleID) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct Turtle, state) == 0x8);
CTR_STATIC_ASSERT(sizeof(struct Turtle) == 0xc);

struct OverlayRDATA_231
{
	// 0x800b2ae4
	struct ParticleEmitter emSet_Missile[11];

	// 0x800b2cc4
	s16 maskPosArr[40];
};

struct OverlayDATA_231
{
	// written by TheUbMunster, this may be wrong/have mistakes!

	// 0x800b2eb4
	struct WeaponSlot231 minePoolItem[40];

	// 0x800b2e9c
	struct LinkedList minePoolTaken;

	// 0x800b2ea8
	struct LinkedList minePoolFree;
};

extern struct OverlayRDATA_231 R231;
extern struct OverlayDATA_231 D231;

#endif
