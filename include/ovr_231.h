
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
	// & 1 - used 10 wumpa fruit
	// & 0x20 - bomb backwards
	u16 flags;

	// 0x18
	SVec3 dir;

	// 0x1e
	s16 rotY;

	// 0x20
	s16 frameCount_DontHurtParent;
	s16 frameCount_Blind;

	// 0x24
	int audioPtr;

	// 0x28
	u32 distanceToTarget;

	// === This point and beyond is Warpball ===

	// 0x2c
	// lev->0x14c + "this" * 0xC
	int respawnPointIndex;

	// 0x30
	int fadeAway_frameCount5;

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
	int unk4c;

	// 0x50
	s16 unk50;

	// 0x52
	s16 turnAround;

	// 0x54
	int framesSeekMine;

	// 0x58 bytes large
};

struct RainLocal
{
	// 0x0
	struct RainLocal *next;
	struct RainLocal *prev;

	// 0x8
	int frameCount;

	// 0xC
	s16 unk1[4];

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
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct RainCloud, effect) == 0x6);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct RainCloud) == 0x8);

struct Shield
{
	// 0x0
	int animFrame;

	// 0x4
	s16 duration;

	// 0x6
	// & 1 - popped by VehPickState_NewState
	// & 2 - shooting
	// & 4 - blue shield
	// & ??? - shooting
	s16 flags;

	// 0x8
	struct Instance *instColor;

	// 0xC
	struct Instance *instHighlight;

	// 0x10
	SVec3 highlightRot;
	s16 highlightTimer;

	// 0x18
};

struct MineWeapon;

struct WeaponSlot231
{
	// 0x0
	struct WeaponSlot231 *next;
	struct WeaponSlot231 *prev;

	// 0x8
	struct MineWeapon *mineWeapon;
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
	s16 frameCount_DontHurtParent;

	// 0x26
	s16 tntSpinY;

	// 0x28
	// 1 - red beaker
	// 2 - thrown (papu or komodo joe) (tnt/potion)
	u16 extraFlags;

	// 0x2a
	s16 cooldown;
};

struct Baron
{
	// 0x0
	// for the baron plane
	char unused0[0x6];

	// 0x6
	s16 unk06;

	// 0x8
	char unused8[0x12];

	// 0x1a
	s16 unk1A;

	// 0x1c
	char unused1C[0x6];

	// 0x22
	s16 unk22;

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

struct Blade
{
	int angle;

	// 0x4 bytes large
};

struct Crate
{
	int cooldown;
	s16 boolPauseCooldown;
	s16 padding;

	// 0x8 bytes large
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct Crate) == 0x8);

struct Crystal
{
	SVec3 rot;
	s16 padding;

	// 0x8 bytes large
};

struct CtrLetter
{
	SVec3 rot;
	s16 padding;
	// 0x8 bytes large
};

struct StartBanner
{
	s16 unused;
	s16 numVertices;
	// 0x4 bytes large
};

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

	// 0x20 bytes large
};

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

	// 0x6 (maybe a rot[3]?)
	s16 unused[3];

	// 0xC
	s16 velY;

	// 0xE
	// unused
	s16 direction;

	// 0x10 bytes large
};

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
	s16 unk;

	// 0xe
	s16 cooldown;

	// 0x10
	int audioPtr;

	// 0x14 bytes large
};

struct Follower
{
	int frameCount;
	struct Driver *driver;
	struct Thread *mineTh;
	int backupTimesDestroyed;

	SVec3 realPos;
	s16 _pad_realPos;
};

struct Fruit
{
	struct Driver *driver;
};

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
	int audioPtr;

	// 0x2c bytes large
};

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
	s16 midpoint[3]; // ?

	// 0x2e
	s16 direction;

	// 0x30 bytes large
};

struct Plant
{
	// 0x0
	s16 cycleCount;

	// 0x2
	s16 cooldown;

	// 0x4
	// 0: left side of track
	// 1: right side of track
	s16 LeftOrRight;

	// 0x6
	s16 boolEatingPlayer;

	// 0x8 bytes large
};

struct Seal
{
	// 0x0
	SVec3 spawnPos;

	// 0x6
	s16 sealID;

	// 0x8
	// unused, should erase
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
	// unused, should erase
	SVec3 rotDesiredAlt;

	// 0x26
	s16 numFramesSpinning;

	// 0x28
	SVec3 vel;
	s16 padding2e;

	// 0x30 bytes large
};

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
	int audioPtr;

	// 0x10 bytes large
};

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
	s16 boolNearRoof; // 0 on ground, 1 near roof

	// 0x8
	s16 unused;
	s16 padding;

	// 0xC
	struct Instance *shadowInst;

	// 0x10 bytes large
};
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct Spider, delay) == 0x4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct Spider, shadowInst) == 0xc);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct Spider) == 0x10);

struct Teeth
{
	// 0x0
	// -1: closing
	// 0: not moving
	// 1: opening
	int direction;

	// 0x4
	// countdown starts when door
	// is fully open, door starts
	// closing when it is done
	int timeOpen;

	// 0x8 bytes large
};
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct Teeth) == 0x8);

struct Turtle
{
	// 0x0
	s16 timer;

	// 0x2
	// 0 from moment it hits bottom to moment it hits top
	// 1 from moment it hits top to moment it hits bottom
	s16 direction;

	// 0x4
	s16 turtleID;

	// 0x6
	s16 unk6;

	// 0x8
	// 0 - fully down
	// 1 - not fully down
	s16 state;

	// 0xA
	s16 unkA;

	// 0xC bytes large
};
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct Turtle, turtleID) == 0x4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct Turtle, state) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct Turtle) == 0xc);

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
