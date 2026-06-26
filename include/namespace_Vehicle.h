#include <ctr_math.h>

struct Thread;
struct Driver;
struct PushBuffer;
typedef void (*DriverFunc)(struct Thread *thread, struct Driver *driver);

enum VehBirthSpawnFlag
{
	VEH_BIRTH_SPAWN_USE_LEVEL_POSITION = 0x1,
	VEH_BIRTH_SPAWN_INIT_RACE_STATE = 0x2,
	VEH_BIRTH_SPAWN_RACE_START = VEH_BIRTH_SPAWN_USE_LEVEL_POSITION | VEH_BIRTH_SPAWN_INIT_RACE_STATE,
};

CTR_STATIC_ASSERT(VEH_BIRTH_SPAWN_RACE_START == 0x3);

enum DriverFuncSlot
{
	DRIVER_FUNC_INIT = 0,
	DRIVER_FUNC_UPDATE,
	DRIVER_FUNC_PHYS_LINEAR,
	DRIVER_FUNC_AUDIO,
	DRIVER_FUNC_PHYS_ANGULAR,
	DRIVER_FUNC_APPLY_FORCES,
	DRIVER_FUNC_COLL_MOVED,
	DRIVER_FUNC_COLLIDE_DRIVERS,
	DRIVER_FUNC_COLL_FIXED,
	DRIVER_FUNC_JUMP_FRICTION,
	DRIVER_FUNC_TRANSLATE_MATRIX,
	DRIVER_FUNC_ANIMATE,
	DRIVER_FUNC_PARTICLES,
	DRIVER_FUNC_COUNT,
};

enum Characters
{
	CRASH_BANDICOOT = 0,
	NEO_CORTEX,
	TINY_TIGER,
	COCO_BANDICOOT,
	N_GIN,
	DINGODILE,
	POLAR,
	PURA,
	PINSTRIPE,
	PAPU_PAPU,
	RIPPER_ROO,
	KOMODO_JOE,
	N_TROPY,
	PENTA_PENGUIN,
	FAKE_CRASH,
	NITROS_OXIDE = 15
};

enum KartState
{
	KS_NORMAL = 0,
	KS_CRASHING = 1,
	KS_DRIFTING = 2,
	KS_SPINNING = 3,
	KS_ENGINE_REVVING = 4,
	KS_MASK_GRABBED = 5,
	KS_BLASTED = 6,
	KS_ANTIVSHIFT = 9,
	KS_WARP_PAD = 10,
	KS_FREEZE = 11
};

enum
{
	REV_ENGINE_CHARGE_IDLE = 0,
	REV_ENGINE_CHARGE_RELEASED_ABOVE_ACCEL = 1,
	REV_ENGINE_CHARGE_ACTIVE = 2,
};
typedef u8 RevEngineChargeState;

enum
{
	REV_ENGINE_LOCKOUT_PEDAL_HELD = 0x1,
	REV_ENGINE_LOCKOUT_REV_DECAY = 0x2,
	REV_ENGINE_LOCKOUT_ALL = REV_ENGINE_LOCKOUT_PEDAL_HELD | REV_ENGINE_LOCKOUT_REV_DECAY,
};
typedef u8 RevEngineLockoutFlags;

enum
{
	ENGINE_SOUND_FADE_OUT = 0,
	ENGINE_SOUND_FADE_IN = 1,
	ENGINE_SOUND_DYNAMIC = 2,
};
typedef u8 EngineSoundMode;

enum RevEnginePackedStatusMask
{
	REV_ENGINE_PACKED_BUSY_MASK = 0x0200ffff,
};

enum DriverAccelTap
{
	DRIVER_ACCEL_TAP_WINDOW_MS = 0x100,
	DRIVER_ACCEL_TAP_STEER_COUNT = 7,
};

enum DriverTireColorCycle
{
	DRIVER_TIRE_COLOR_DEFAULT = 0x2e808080,
	DRIVER_TIRE_COLOR_DARK = 0x2e606061,
	DRIVER_TIRE_COLOR_TIMER_INITIAL = 0xa00,
	DRIVER_TIRE_COLOR_TIMER_RESET = 0x1e00,
	DRIVER_TIRE_COLOR_SPEED_AIRBORNE_BONUS = 0xf00,
	DRIVER_TIRE_COLOR_LOW_SPEED_THRESHOLD = 0x200,
	DRIVER_TIRE_COLOR_SPEED_WEIGHT = 0x89,
	DRIVER_TIRE_COLOR_STEP_WEIGHT = 0x177,
};

enum DriverSkidmarks
{
	DRIVER_SKIDMARK_FRAME_COUNT = 8,
	DRIVER_SKIDMARK_TIRE_COUNT = 4,
};

union VehEmitterSkidmark
{
	SVECTOR edge[2];
	struct
	{
		SVec3 edge0;
		u8 color;
		u8 flags;
		SVec3 edge1;
		s16 pad;
	};
};

struct DriverCheckpointState
{
	// 0x0 - checkpoint chosen after a split, used by warpball pathing
	u8 branchChoiceIndex;

	// 0x1
	u8 currentIndex;
};

// NOTE(aalhendi): Retail reuses this scratch range as transformed tire words,
// a compressed 3x2 light matrix, then a distance vector.
union VehEmitterWallScratch
{
	s32 word[6];
	s16 half[12];
};

struct VehGroundSkidsScratch
{
	SVECTOR projected[3];
	struct PushBuffer *pushBuffer;
	u32 colorNear;
	u32 colorFar;
	union
	{
		u32 segmentFlags;
		struct
		{
			u8 segmentFlagsLow;
			u8 segmentFlagsPadding[3];
		};
	};
	u32 currXY[9];
	u32 prevXY[9];
	s32 currDepth[9];
	s32 prevDepth[9];
	Vec3 origin;
};

// NOTE(aalhendi): Retail VehPhysCrash_AI uses globals 0x8009ae28
// and 0x8009ae38 for this scratch state.
struct VehPhysCrashAiScratch
{
	Vec3 forward;
	s32 reserved_0x0c;
	MATRIX matrix;
};

enum PhysType
{
	// MetaPhys[0]
	GRAVITY = 0,
	// Driver offset
	GRAVITY_OFFSET = 0x416,

	// MetaPhys[1]
	JUMP = 1,
	// Driver offset
	JUMP_OFFSET = 0x418,

	// MetaPhys[2]
	PEDAL_FRICTION_PERPENDICULAR = 0x2,
	// Driver offset
	PEDAL_FRICTION_PERPENDICULAR_OFFSET = 0x41A,

	// MetaPhys[3]
	PEDAL_FRICTION_FORWARD = 0x3,
	// Driver offset
	PEDAL_FRICTION_FORWARD_OFFSET = 0x41C,

	// MetaPhys[4]
	NO_PEDAL_FRICTION_PERPENDICULAR = 0x4,
	// Driver offset
	NO_PEDAL_FRICTION_PERPENDICULAR_OFFSET = 0x41E,

	// MetaPhys[5]
	NO_PEDAL_FRICTION_FORWARD = 0x5,
	// Driver offset
	NO_PEDAL_FRICTION_FORWARD_OFFSET = 0x420,

	// MetaPhys[6]
	BRAKE_FRICTION = 0x6,
	// Driver offset
	BRAKE_FRICTION_OFFSET = 0x422,

	// MetaPhys[7]
	DRIFT_CURVE = 0x7,
	// Driver offset
	DRIFT_CURVE_OFFSET = 0x424,

	// MetaPhys[8]
	DRIFT_FRICTION = 0x8,
	// Driver offset
	DRIFT_FRICTION_OFFSET = 0x426,

	// MetaPhys[9]
	ACCEL_CLASS_STAT = 9,
	// Driver offset
	ACCEL_CLASS_STAT_OFFSET = 0x428,

	// MetaPhys[A]
	ACCEL_RESERVES = 0xA,
	// Driver offset
	ACCEL_RESERVES_OFFSET = 0x42A,

	// MetaPhys[B]
	SPEED_CLASS_STAT = 0xB,
	// Driver offset
	SPEED_CLASS_STAT_OFFSET = 0x42C,

	// MetaPhys[C]
	ACCEL_SPEED_CLASS_STAT = 0xC,
	// Driver offset
	ACCEL_SPEED_CLASS_STAT_OFFSET = 0x42E,

	// MetaPhys[D]
	SINGLE_TURBO_SPEED = 0xD,
	// Driver offset
	SINGLE_TURBO_SPEED_OFFSET = 0x430,

	// MetaPhys[E]
	SACRED_FIRE_SPEED = 0xE,
	// Driver offset
	SACRED_FIRE_SPEED_OFFSET = 0x432,

	// MetaPhys[F]
	BACKWARD_SPEED = 0xF,
	// Driver offset
	BACKWARD_SPEED_OFFSET = 0x434,

	// MetaPhys[10]
	MASK_SPEED = 0x10,
	// Driver offset
	MASK_SPEED_OFFSET = 0x436,

	// MetaPhys[11]
	DAMAGED_SPEED = 0x11,
	// Driver offset
	DAMAGED_SPEED_OFFSET = 0x438,

	// MetaPhys[12]
	TURN_RATE = 0x12,
	// Driver offset
	TURN_RATE_OFFSET = 0x43A,

	// MetaPhys[13]
	BACKWARD_TURN_RATE = 0x13,
	// Driver offset
	BACKWARD_TURN_RATE_OFFSET = 0x43B,

	// MetaPhys[14]
	TURN_DECREASE_RATE = 0x14,
	// Driver offset
	TURN_DECREASE_RATE_OFFSET = 0x43C,

	// MetaPhys[15]
	TURN_INPUT_DELAY = 0x15,
	// Driver offset
	TURN_INPUT_DELAY_OFFSET = 0x43E,

	// MetaPhys[16]
	PRE_TURBO = 0x16,
	// Driver offset
	PRE_TURBO_OFFSET = 0x440,

	// MetaPhys[17]
	TERMINAL_VELOCITY = 0x17,
	// Driver offset
	TERMINAL_VELOCITY_OFFSET = 0x442,

	// MetaPhys[18]
	TERRAIN_FRICTION_BOOST = 0x18,
	// Driver offset
	TERRAIN_FRICTION_BOOST_OFFSET = 0x444,

	// MetaPhys[19]
	STEER_ACCEL_STAGE4_FIRST_FRAME = 0x19,
	// Driver offset
	STEER_ACCEL_STAGE4_FIRST_FRAME_OFFSET = 0x446,

	// MetaPhys[1A]
	STEER_ACCEL_STAGE2_FIRST_FRAME = 0x1A,
	// Driver offset
	STEER_ACCEL_STAGE2_FIRST_FRAME_OFFSET = 0x447,

	// MetaPhys[1B]
	STEER_ACCEL_STAGE2_FRAME_LENGTH = 0x1B,
	// Driver offset
	STEER_ACCEL_STAGE2_FRAME_LENGTH_OFFSET = 0x448,

	// MetaPhys[1C]
	STEER_ACCEL_STAGE1_MAX_STEER = 0x1C,
	// Driver offset
	STEER_ACCEL_STAGE1_MAX_STEER_OFFSET = 0x44A,

	// MetaPhys[1D]
	STEER_ACCEL_STAGE1_MIN_STEER = 0x1D,
	// Driver offset
	STEER_ACCEL_STAGE1_MIN_STEER_OFFSET = 0x44C,

	// MetaPhys[1E]
	STEER_ACCEL_TURN_VEL_SCALE = 0x1E,
	// Driver offset
	STEER_ACCEL_TURN_VEL_SCALE_OFFSET = 0x44E,

	// MetaPhys[1F]
	STEER_ACCEL_TURN_VEL_LIMIT = 0x1F,
	// Driver offset
	STEER_ACCEL_TURN_VEL_LIMIT_OFFSET = 0x450,

	// MetaPhys[20]
	MODEL_ROT_VEL_MAX = 0x20,
	// Driver offset
	MODEL_ROT_VEL_MAX_OFFSET = 0x452,

	// MetaPhys[21]
	MODEL_ROT_VEL_MIN = 0x21,
	// Driver offset
	MODEL_ROT_VEL_MIN_OFFSET = 0x454,

	// MetaPhys[22]
	MODEL_TURN_COUNTER_STEER_STRENGTH = 0x22,
	// Driver offset
	MODEL_TURN_COUNTER_STEER_STRENGTH_OFFSET = 0x457,

	// MetaPhys[23]
	MODEL_TURN_RETURN_STRENGTH = 0x23,
	// Driver offset
	MODEL_TURN_RETURN_STRENGTH_OFFSET = 0x458,

	// MetaPhys[24]
	MODEL_TURN_NEGATIVE_RETURN_STRENGTH = 0x24,
	// Driver offset
	MODEL_TURN_NEGATIVE_RETURN_STRENGTH_OFFSET = 0x459,

	// MetaPhys[25]
	MODEL_TURN_VELOCITY_LERP = 0x25,
	// Driver offset
	MODEL_TURN_VELOCITY_LERP_OFFSET = 0x45A,

	// MetaPhys[26] Kart Turn Animation (speed?)
	TURN_RESIST_MIN = 0x26,
	// Driver offset
	TURN_RESIST_MIN_OFFSET = 0x45C,

	// MetaPhys[27]
	TURN_RESIST_MAX = 0x27,
	// Driver offset
	TURN_RESIST_MAX_OFFSET = 0x45D,

	// MetaPhys[28]
	STEER_VEL_DRIFT_SWITCH_WAY = 0x28,
	// Driver offset
	STEER_VEL_DRIFT_SWITCH_WAY_OFFSET = 0x45E,

	// MetaPhys[29]
	STEER_VEL_DRIFT_STANDARD = 0x29,
	// Driver offset
	STEER_VEL_DRIFT_STANDARD_OFFSET = 0x45F,

	// MetaPhys[2A]
	DRIFT_TURN_BASE = 0x2A,
	// Driver offset
	DRIFT_TURN_BASE_OFFSET = 0x460,

	// MetaPhys[2B]
	DRIFT_TURN_STARTUP_SCALE = 0x2B,
	// Driver offset
	DRIFT_TURN_STARTUP_SCALE_OFFSET = 0x461,

	// MetaPhys[2C]
	DRIFT_TURN_RAMP_FRAMES = 0x2C,
	// Driver offset
	DRIFT_TURN_RAMP_FRAMES_OFFSET = 0x462,

	// MetaPhys[2D]
	DRIFT_FRAMES_TILL_SPINOUT = 0x2D,
	// Driver offset
	DRIFT_FRAMES_TILL_SPINOUT_OFFSET = 0x463,

	// MetaPhys[2E]
	DRIFT_SPIN_RATE_ACCEL = 0x2E,
	// Driver offset
	DRIFT_SPIN_RATE_ACCEL_OFFSET = 0x464,

	// MetaPhys[2F]
	DRIFT_SPIN_RATE_DECEL = 0x2F,
	// Driver offset
	DRIFT_SPIN_RATE_DECEL_OFFSET = 0x466,

	// MetaPhys[30]
	DRIFT_CAMERA_SPIN_RATE = 0x30,
	// Driver offset
	DRIFT_CAMERA_SPIN_RATE_OFFSET = 0x468,

	// MetaPhys[31]
	DRIFT_CAMERA_LERP_STEP = 0x31,
	// Driver offset
	DRIFT_CAMERA_LERP_STEP_OFFSET = 0x46A,

	// MetaPhys[32]
	DRIFT_RELEASE_TURN_ASSIST_FRAMES = 0x32,
	// Driver offset
	DRIFT_RELEASE_TURN_ASSIST_FRAMES_OFFSET = 0x46B,

	// MetaPhys[33]
	METAPHYS_33 = 0x33,
	// Driver offset
	METAPHYS_33_OFFSET = 0x46C,

	// MetaPhys[34]
	METAPHYS_34 = 0x34,
	// Driver offset
	METAPHYS_34_OFFSET = 0x46E,

	// MetaPhys[35]
	DRIFT_TURN_SAME_DIRECTION_ANGLE = 0x35,
	// Driver offset
	DRIFT_TURN_SAME_DIRECTION_ANGLE_OFFSET = 0x470,

	// MetaPhys[36]
	DRIFT_TURN_OPPOSITE_DIRECTION_ANGLE = 0x36,
	// Driver offset
	DRIFT_TURN_OPPOSITE_DIRECTION_ANGLE_OFFSET = 0x472,

	// MetaPhys[37]
	DRIFT_TURN_ANGLE_SCALE = 0x37,
	// Driver offset
	DRIFT_TURN_ANGLE_SCALE_OFFSET = 0x474,

	// MetaPhys[38]
	TURBO_MAX_ROOM = 0x38,
	// Driver offset
	TURBO_MAX_ROOM_OFFSET = 0x476,

	// MetaPhys[39]
	TURBO_LOW_ROOM_WARNING = 0x39,
	// Driver offset
	TURBO_LOW_ROOM_WARNING_OFFSET = 0x477,

	// MetaPhys[3A]
	TURBO_FULL_BAR_RESERVE_GAIN = 0x3A,
	// Driver offset
	TURBO_FULL_BAR_RESERVE_GAIN_OFFSET = 0x478,

	// MetaPhys[3B]
	DRIFT_BOOST_DURATION_FRAMES = 0x3B,
	// Driver offset
	DRIFT_BOOST_DURATION_FRAMES_OFFSET = 0x479,

	// MetaPhys[3C]
	DRIFT_BOOST_AXIS_KICK_RATE = 0x3C,
	// Driver offset
	DRIFT_BOOST_AXIS_KICK_RATE_OFFSET = 0x47A,

	// MetaPhys[3D]
	COLLISION_WEIGHT = 0x3D,
	// Driver offset
	COLLISION_WEIGHT_OFFSET = 0x47C,

	// MetaPhys[3E]
	SLOPE_FORWARD_SPEED_BONUS = 0x3E,
	// Driver offset
	SLOPE_FORWARD_SPEED_BONUS_OFFSET = 0x47E,

	// MetaPhys[3F]
	SIDE_SPEED_CLAMP = 0x3F,
	// Driver offset
	SIDE_SPEED_CLAMP_OFFSET = 0x480,

	// offset 482 is NOT const,
	// that's driver rank

	// MetaPhys[40]
	PROTOTYPE_KEY = 0x40,
	// Driver offset
	PROTOTYPE_KEY_OFFSET = 0x484,

	NUM_PHYS_TYPES = 65

	// ...
	// 65 (0x41) exist
};

enum TurboType
{
	START_LINE_BOOST = 0,
	FREEZE_RESERVES_ON_TURBO_PAD = 0x1,
	POWER_SLIDE_HANG_TIME = 0x2,
	TURBO_PAD = 0x4,
	TURBO_ITEM = 0x8,
	SUPER_ENGINE = 0x10
};

enum EngineClass
{
	// crash, cortex, joe, fake, oxide
	BALANCED,

	// coco, ngin, pinstripe
	ACCEL,

	// tiny, dingo, papu, ntropy
	SPEED,

	// polar, pura, roo (penta in ntsc)
	TURN,

	NUM_CLASSES
};

enum
{
	ACTION_TOUCH_GROUND = 0x1,
	ACTION_STARTED_TOUCH_GROUND = 0x2,
	ACTION_JUMP_BUTTON_HELD = 0x4,
	ACTION_ACCEL_PREVENTION = 0x8,
	ACTION_STEER_LEFT = 0x10,
	ACTION_BRAKE_WITH_ACCEL = 0x20,
	ACTION_HIGH_JUMP = 0x40,
	ACTION_TURBO_INPUT_LATCH = 0x80,
	ACTION_DRIVING_WRONG_WAY = 0x100,
	ACTION_TURBO_ITEM = 0x200,
	ACTION_JUMP_STARTED = 0x400,
	ACTION_BACK_SKID = 0x800,
	ACTION_FRONT_SKID = 0x1000,
	ACTION_DRIVING_AGAINST_WALL = 0x2000,
	ACTION_WARP = 0x4000,
	ACTION_WEAPON_FIRE_REQUEST = 0x8000,
	ACTION_ENGINE_ECHO = 0x10000,
	ACTION_REVERSING_ENGINE = 0x20000,
	ACTION_RACE_TIMER_FROZEN = 0x40000,
	ACTION_AIRBORNE = 0x80000,
	ACTION_BOT = 0x100000,
	ACTION_NEW_BOOST = 0x200000,
	ACTION_ACCEL_RELEASED_WITH_RESERVES = 0x400000,
	ACTION_MASK_WEAPON = 0x800000,
	ACTION_BEHIND_START_LINE = 0x1000000,
	ACTION_RACE_FINISHED = 0x2000000,
	ACTION_TRACKER_TARGETED = 0x4000000,
	ACTION_CHECKPOINT_BRANCH_PENDING = 0x8000000,
	ACTION_HUMAN_HUMAN_COLLISION = 0x10000000,
	ACTION_REVERSE_STEER_LEFT = 0x20000000,
	ACTION_REVERSE_STEER_RIGHT = 0x40000000,
	ACTION_DROPPING_MINE = 0x80000000u,
};
typedef u32 Actions;

enum
{
	DRIVER_COLL_FLAG_MASK_GRAB_REQUEST = 0x1,
	DRIVER_COLL_FLAG_SURFACE_PUSHBACK = 0x2,
	DRIVER_COLL_FLAG_TOUCHED_QUADBLOCK = 0x4,
	DRIVER_COLL_FLAG_GROUNDED = 0x8,
};
typedef s16 DriverCollisionFlags;

enum
{
	RAIN_CLOUD_EFFECT_SLOW = 0,
	RAIN_CLOUD_EFFECT_ITEM_ROLL = 1,
	RAIN_CLOUD_EFFECT_ICE_TERRAIN = 2,
	RAIN_CLOUD_EFFECT_JUMP_LOCKOUT = 3,
	RAIN_CLOUD_EFFECT_NONE = 4,
	RAIN_CLOUD_EFFECT_HEAVY_FRICTION = 5,
	RAIN_CLOUD_EFFECT_RESERVE_RELEASE = 6,
};
typedef s16 RainCloudEffect;

enum
{
	FORCED_JUMP_NONE = 0,
	FORCED_JUMP_LOW = 1,
	FORCED_JUMP_HIGH = 2,
};
typedef u8 ForcedJumpType;

enum BotFlags
{
	BOT_FLAG_ESTIMATE_NAV = 0x1,
	BOT_FLAG_DAMAGE_ACTIVE = 0x2,
	BOT_FLAG_DAMAGE_SUPPRESS_EMITTER = 0x4,
	BOT_FLAG_FREE_PHYSICS = 0x8,
	BOT_FLAG_NAV_BOOST_ACTIVE = 0x10,
	BOT_FLAG_MOON_GRAVITY = 0x20,
	BOT_FLAG_BOSS_PATH_REQUESTED = 0x40,
	BOT_FLAG_BOSS_PATH_ACTIVE = 0x80,
	BOT_FLAG_DEMO_CAMERA_STARTED = 0x100,
	BOT_FLAG_STARTLINE_INIT_DONE = 0x200,
};

struct MetaPhys
{
	u32 unusedDebugStr;
	int offset;
	int size;
	int value[NUM_CLASSES];
};


struct Turbo
{
	struct Instance *inst;
	struct Driver *driver;

	// 0x8
	// Index for the animation frame of the exhaust fire
	// Value 0 means it uses turbo0, 1 means it uses turbo1, so on until 7 is reached and it loops back to 0
	s16 fireAnimIndex;

	// 0xA
	// Stores the visual size of the exhaust fire
	// Integer value ranging from 4 to 8
	// One power-slide and green hang time is 5
	// Two power-slides and yellow hang time is 6
	// Three power-slides, red hang time, and start boost is 7
	// Turbo pad and USF is 8
	// 4 appears to be unused
	// Fire doesn't get any smaller at values lower than 4 nor bigger at values higher than 8
	s16 fireSize;

	// 0xC
	// Value that decreases every time VehTurbo_ThTick gets called
	// If it reaches 0 it makes the fire start disappearing
	s8 fireDisappearCountdown;

	// 0xD
	// Used for the distortion of the sound that indicates active fire/reserves
	u8 fireAudioDistort;

	// 0xE
	// Cooldown for when fire is visible
	// Set to 96 (which makes fire invisible for 0.1 seconds, 96 / 1000 = 96ms = 0.1s) when obtaining turbo from certain sources, namely those from
	// power-sliding (used to make fire pop with each power-slide)
	s16 fireVisibilityCooldown;
};

struct BotPhysics
{
	// 0x0, Driver + 0x5bc
	s16 rotXZ;

	// 0x2, Driver + 0x5be
	s16 driftTarget;

	// 0x4, Driver + 0x5c0
	s16 mulDrift;

	// 0x6, Driver + 0x5c2
	s16 simpTurnState;

	// 0x8, Driver + 0x5c4
	s16 turboMeter;

	// 0xa, Driver + 0x5c6
	s16 fireLevel;

	// 0xc, Driver + 0x5c8
	int squishCooldown; // retail uses both halfword and word operations on this slot

	// 0x10, Driver + 0x5cc - retail reset-only slot, no known consumer
	int reserved_0x5cc;

	// 0x14, Driver + 0x5d0
	int speedY;

	// 0x18, Driver + 0x5d4
	int speedLinear;

	// 0x1c, Driver + 0x5d8
	Vec3 accel;

	// 0x28, Driver + 0x5e4
	Vec3 velocity;
};

struct BotData
{
	// these offset are from the perspective as they exist from within `struct Driver`

	// 0x598, offset in `struct BotData` == 0x0
	struct Item item;

	// 0x5a0, offset in `struct BotData` == 0x8
	int reserved_0x5a0;

	// 0x5a4, offset in `struct BotData` == 0xc
	struct NavFrame *botNavFrame;

	// 0x5a8
	int navProgressRemainder;

	// 0x5ac
	int reserved_0x5ac;

	// 0x5b0
	// u32 BotFlags plus other AI-only state bits.
	// bits 9-16 might be = (navframe flags << 8)
	u32 botFlags;

	// 0x5b4
	// acceleration from start-line
	int botAccel;

	// 0x5b8
	// s16 path index
	s16 botPath;

	// 0x5ba
	s16 aiDamageState;

	// 0x5bc, offset in `struct BotData` == 0x24
	struct BotPhysics aiPhysics;

	// 0x5f0
	Vec3 positionBackup;

	// 0x5fc
	SVec3 aiRot;
	s16 _pad_aiRot;

	// 0x604
	int ai_progress_cooldown;

	// 0x608
	s16 ai_rotY_608;

	// 0x60a
	u8 ai_quadblock_checkpointIndex;

	// 0x60b
	u8 padding_0x60b;

	// 0x60c
	union
	{
		struct NavFrame estimateNavFrame;
		struct
		{
			SVec3 estimatePosition;
			u8 estimateRotNav[3];
			u8 estimateRotCurrY;
			s16 distToNextNavXYZ;
			s16 distToNextNavXZ;
			s16 estimateFlags;
			union
			{
				int estimateTail;
				struct
				{
					s16 estimatePathChangeOpcode;
					u8 estimateGoBackCount;
					u8 estimateSpecialBits;
				};
			};
		};
	};

	// 0x620
	struct MaskHeadWeapon *maskObj;

	// 0x624
	s16 weaponCooldown;

	// 0x626
	u8 blastBounceCount;
	u8 desiredPath_BossOnly;

	// 0x628
	int reserved_0x628;
};

// for Players, AIs and Ghosts
struct Driver
{
	// 0x0
	struct Icon **wheelSprites;
	// 0x4
	u16 wheelSize;

	// 0x6
	// Front wheel rotation sprite frame offset
	// Also controls the LR panning of the Engine Sound
	// Default: 0
	// Steering left ranges between 0 to 64
	// Steering right ranges between 0 to -64 (if we display it as a signed number)
	s16 wheelRotation;
	// 0x8
	u32 tireColor;
	// 0xC
	s16 clockReceive;
	// 0xE
	s16 hazardTimer;
	// 0x10
	struct Instance *instBombThrow;
	// 0x14
	struct Instance *instBubbleHold;
	// 0x18
	struct Instance *instTntRecv; // on your head
	// 0x1C
	struct Instance *instSelf;

// Not in Aug Review
#if BUILD >= SepReview
	// 0x20
	struct Instance *instTntSend; // on the ground
#endif

	// 0x24
	int invincibleTimer;
	// 0x28
	int invisibleTimer;
	// 0x2C
	u32 instFlagsBackup;
	// 0x30
	s8 numWumpas;
	// 0x31
	char numCrystals;
	// 0x32
	char numTimeCrates;
	// 0x33
	s8 accelConst;
	// 0x34
	s8 turnConst;

	// 0x35
	// Super Engine from Beta,
	// ever used in retail?
	s8 turboConst;

	// 0x36
	u8 heldItemID;
	// 0x37
	u8 numHeldItems;
	// 0x38
	s16 superEngineTimer;
	// 0x3A
	s16 itemRollTimer;
	// 0x3C
	s16 noItemTimer;
	// 0x3E
	s16 padding_0x3e;
	// 0x40
	int lapTime;
	// 0x44
	u8 lapIndex;
	// 0x45
	u8 clockSend;
	// 0x46
	s16 jumpMeter;
	// 0x48
	s16 jumpMeterTimer;
	// 0x4A
	u8 driverID;
	// 0x4B
	s8 simpTurnState;
	// 0x4C
	u8 matrixArray;
	// 0x4D
	u8 matrixIndex;

#if BUILD >= EurRetail
	s16 compilerPadding_0x4E;

	// 0x50
	// highest amount of consecutive turbos in a race
	// exclusive to Japan Retail
	int numTurbosHighScore;
#endif

#if BUILD >= SepReview
	// 0x4E -- UsaRetail
	// 0x54 -- EurRetail, JpnRetail
	s16 numTurbos;
	// 0x50
	u16 frameAgainstWall; // allocated in Sep3, does not function
#endif

#if BUILD < EurRetail
	// There is no "s16" on 0x52,
	// there is padding for the next
	// 4-byte void* that is unused
	s16 funcPtrs_compilerpadding;
#endif

	// 0x54 (UsaRetail) / 0x58 (EurRetail, JpnRetail) - OnInit, First function for spawn, drifting, damage, etc
	// 0x58 - OnUpdate, updates per frame for any generic purpose
	// 0x5C - OnPhysLinear
	// 0x60 - OnAudio, engine sounds (always same)
	// 0x64 - OnPhysAngular
	// 0x68 - VehPhysForce_OnApplyForces
	// 0x6C - COLL_MOVED_PlayerSearch
	// 0x70 - VehPhysForce_CollideDrivers
	// 0x74 - COLL_FIXED_PlayerSearch
	// 0x78 - VehPhysGeneral_JumpAndFriction
	// 0x7C - VehPhysForce_TranslateMatrix (pos, rot, scale)
	// 0x80 - OnAnimate
	// 0x84 - OnParticles
	DriverFunc funcPtrs[DRIVER_FUNC_COUNT];

	// 0x88
	Vec3 velocity;

	// 0x94
	Vec3 originToCenter;

	// 0xA0 - quadblock currently touched,
	// it is zero while airborne
	struct QuadBlock *currBlockTouching;

	// 0xA4
	SVec3 normalVecUP;
	DriverCollisionFlags collisionFlags;

	// 0xac
	union
	{
		s16 spsHitPosRaw[4];
		struct
		{
			SVec3 spsHitPos;
			s16 padding_0xb2;
		};
	};

	// 0xb4
	union
	{
		s16 spsNormalVecRaw[4];
		struct
		{
			SVec3 spsNormalVec;
			s16 padding_0xba;
		};
	};

	// 0xBC
	// 0xBD is waterFlag
	CollStepFlags stepFlagSet;

	// 0xC0
	s16 ampTurnState;

	// 0xC2
	u8 currentTerrain;

	// 0xc3
	// eight frames, 0-7
	char skidmarkFrameIndex;

	// 0xC4
	// 0x010 per tire,
	// 0x040 per frame (4 tires)
	// 0x200 total (8 frames)
	union VehEmitterSkidmark skidmarks[DRIVER_SKIDMARK_FRAME_COUNT][DRIVER_SKIDMARK_TIRE_COUNT];

	// 0x2C4
	u32 skidmarkEnableFlags;

	// actions:
	// 0x00000001 - touching quadblock
	// 0x00000002 - started touching quadblock this frame
	// 0x00000004 - holding L1 or R1
	// 0x00000008 - accel prevention (holding square)
	// 0x00000010 - steering left
	// 0x00000020 - gas and brake together
	// 0x00000040 - ? COLL-related (ghost)
	// 0x00000080 - ? VehPhysForce_TranslateMatrix-related, turbo-related
	// 0x00000100 - driving wrong way
	// 0x00000200 - turbo item active
	// 0x00000400 - started jump this frame
	// 0x00000800 - back wheel skid
	// 0x00001000 - front wheel skid
	// 0x00002000 - driving against wall
	// 0x00004000 - warping (or blasted, or LastSpin)
	// 0x00008000 - weapon fire request
	// 0x00010000 - echo engine
	// 0x00020000 - reversing engine
	// 0x00040000 - race timer frozen
	// 0x00080000 - airborne
	// 0x00100000 - driver is an AI
	// 0x00200000 - new boost this frame
	// 0x00400000 - accel released while reserves keep speed
	// 0x00800000 - using mask weapon
	// 0x01000000 - behind start line
	// 0x02000000 - race finished for this driver
	// 0x04000000 - tracker chasing you
	// 0x08000000 - checkpoint branch choice pending
	// 0x10000000 - human-human collision
	// 0x20000000 - reverse steer left latch
	// 0x40000000 - reverse steer right latch
	// 0x80000000 - dropping mine


	// 0x2C8
	Actions actionsFlagSet;

	// 0x2CC
	Actions actionsFlagSetPrevFrame;

	// 0x2D0
	int quadBlockHeight;

	// 0x2D4
	Vec3 posCurr;

	// 0x2E0
	// used for velocity in 231
	Vec3 posPrev;

	// 0x2EC
	// This is render rotation, not velocity direction,
	// these are the variables that get turned into
	// instance matrix
	SVec3Slot rotCurr;

	// 0x2F4
	// used for velocity in 231
	SVec3Slot rotPrev;

	// 0x2FC
	int sfxDistortOffset;

	// 0x300
	// 0x300 = Kart skidmarks sound
	// 0x304 = No sound yet defined* (VehEmitter.c: VehEmitter_DriverMain)
	// 0x308 = Kart "kirb_dirt" sound
	// 0x30C = Kart "engine_jet" sound
	void *driverAudioPtrs[4];

	// 0x310
	MATRIX matrixMovingDir;

	// 0x330
	MATRIX matrixFacingDir;

	// 0x350
	// continues updating while driver is airborne,
	// used for VisMem (sometimes?)
	struct QuadBlock *underDriver;

	// 0x354
	// last "valid" quadblock the driver touched
	// used for mask grab if next block is invalid
	struct QuadBlock *lastValid;

	// 0x358
	// is it ice, gravel, or what?
	struct Terrain *terrainMeta1;

	// 0x35C
	struct Terrain *terrainMeta2;

	// each normalVec is 8 bytes apart,
	// used as an array of vec4s, with
	// variables sneaked in-between

	// 0x360
	// used in PhysLinear, reset in VehPhysForce_OnApplyForces, calculated in StartSearch,
	// all three are funcPtrs in the driver struct, in that order of operation
	SVec3 AxisAngle1_normalVec;

	// 0x366
	ForcedJumpType forcedJumpType;

	// 0x367
	// clock effect
	char clockFlash;

	// 0x368
	SVec3 AxisAngle2_normalVec;

	// 0x36e
	s16 speedometerNeedleValue;

	// 0x370
	SVec3 AxisAngle3_normalVec;

	// 0x376
	u8 kartState;

	// 0x377
	s8 Screen_OffsetY;

	// 0x378
	SVec3 AxisAngle4_normalVec;

	// 0x37e
	s16 padding_0x37e;

	// 0x380
	char normalVecID;
	u8 failedBoostExhaustTimer;

	// 0x382
	s16 buttonUsedToStartDrift;

	// 0x384
	SVec3 posWallColl;

	// 0x38A
	s16 wallRubSpeedLimit;

	// 0x38C
	s16 speed;

	// 0x38E
	s16 speedApprox;

	// 0x390
	s16 jumpHeightCurr;

	// 0x392
	s16 jumpHeightPrev;

	// 0x394
	s16 axisRotationY;

	// 0x396
	s16 axisRotationX;

	// 0x398
	s16 padding_0x398;

	// 0x39A
	s16 angle;

	// 0x39C
	s16 baseSpeed;

	// 0x39E
	s16 fireSpeed;

	// 0x3A0
	int xSpeed;

	// 0x3A4
	int ySpeed;

	// 0x3A8
	int zSpeed;

	// 0x3AC
	SVec3 forwardAccelVector;
	// 0x3B2
	s16 forwardAccelImpulse;

	// 0x3B4
	s16 rotationSpinRate;

	// 0x3B6
	s16 engineSoundVolumeState;

	// 0x3B8
	s16 engineSoundPitchState;

	// 0x3BA
	// in japanese VehFire_Increment
	s16 japanTurboUnknown;

	// 0x3BC
	s16 tireColorCycleTimer;

	// 0x3BE
	s16 tireColorCycleStep;

	// 0x3C0
	// Repeated accel taps count only while this window is active.
	s16 accelTapWindowTimer;

	// 0x3C2
	// Repeated accel taps sharpen steering and reduce turn lean.
	s16 accelTapCount;

	// 0x3C4
	// base speed after terrain speed multiplier
	s16 terrainScaledBaseSpeed;

	// 0x3C6 0x3C8
	// in VehPhysProc_Driving_PhysLinear,
	// and VehPhysForce_AccelTerrainSlope,
	// and VehPhysProc_SpinLast_Update
	s16 turnAngleCurr;
	s16 turnAnglePrev;

	// 0x3CA
	s16 turnAngleLerpTarget;

	// 0x3CC
	// from VehPhysForce_CollideDrivers
	SVec3 accel;

	// 0x3D2
	s16 turnAngleLerpVel;

	// 0x3d4
	// This is a UNION between kart states
	union
	{
		s16 turnWobbleRaw[3];
		struct
		{
			s16 turnWobbleAngle;
			s16 turnWobbleVelocity;
			s16 turnWobbleTimer;
		};
	};

	// 0x3DA
	// also drift direction
	s16 multDrift;

	// 0x3DC
	// turbo_MeterRoomLeft has values 0-1000 (dec),
	// can go negative and it draws farther left
	s16 turbo_MeterRoomLeft;

	// 0x3DE
	s16 turbo_outsideTimer;

	// 0x3E0
	s16 VehFire_AudioCooldown;

	// 0x3E2
	s16 reserves;

	// 0x3E4
	s16 fireSpeedCap;

	// 0x3E6
	s16 numFramesSpentSteering;

	// 0x3E8
	s16 forwardDir;

	// 0x3EA
	s16 previousFrameMultDrift; // previous frame or just repeat?

	// 0x3ec
	s16 timeUntilDriftSpinout;

	// 0x3ee
	s16 distanceFromGround;

	// 0x3F0
	// 10-frame buffer
	s16 jump_TenBuffer;

	// 0x3F2
	// so you can't spam jump too fast
	s16 jump_CooldownMS;

	// 0x3F4
	// time since driver left quadblock,
	// the speedrunners call this "coyote jump"
	s16 jump_CoyoteTimerMS;

	// 0x3F6
	// if not zero, and if touch ground,
	// it forces player to jump
	s16 jump_ForcedMS;

	// 0x3F8
	s16 jump_InitialVelY;

	// 0x3FA
	s16 jump_HighJumpTimerMS;

	// 0x3FC
	s16 jump_LandingBoost;

	// 0x3FE
	s16 wallRubTimer;

	// 0x400
	s16 NoInputTimer;

	// 0x402
	s16 burnTimer;

	// 0x404
	s16 squishTimer;

	// 0x406
	// Initial driving grace period before repeated V-shifts can freeze the kart.
	s16 vShiftStartGuardTimer;

	// 0x408
	// Rolling window for counting local-Z sign flips.
	s16 vShiftWindowTimer;

	// 0x40A
	// Counts local-Z sign flips during the rollback window; high values trigger FreezeVShift.
	s16 vShiftCount;

	// 0x40C
	// when jumping and when hitting ground
	s16 jumpSquishStretch;

	// Retail clears this during SlamWall init; no known consumer.
	s16 reserved_0x40e;

	// 0x410
	// used to calculate the other ^^
	s16 jumpSquishStretch2;

	// Retail initializes this to 0x600 during VehBirth; no known consumer.
	s16 reserved_0x412;

	// 0x414 (physics/terrain related)
	s16 terrainFrictionTimer;

	/* Constant table - Metaphysics */

	// 0x416 - 0x00
	s16 const_Gravity; // OK

	// 0x418 - 0x01
	s16 const_JumpForce; // OK

	// 0x41A - 0x02
	s16 const_PedalFriction_Perpendicular; // OK applied sideways to the kart

	// 0x41C - 0x03
	s16 const_PedalFriction_Forward; // OK

	// 0x41E - 0x04
	s16 const_NoPedalFriction_Perpendicular; // OK applied sideways to the kart

	// 0x420 - 0x05
	s16 const_NoPedalFriction_Forward; // OK

	// 0x422 - 0x06
	s16 const_BrakeFriction; // OK

	// 0x424 - 0x07
	s16 const_DriftCurve; // OK

	// 0x426 - 0x08
	s16 const_DriftFriction; // OK

	// 0x428 - 0x09
	s16 const_Accel_ClassStat; // OK

	// 0x42A - 0x0A
	s16 const_Accel_Reserves; // OK

	// 0x42C - 0x0B
	s16 const_Speed_ClassStat; // OK

	// 0x42E - 0x0C
	s16 const_AccelSpeed_ClassStat; // OK

	// 0x430 - 0x0D
	s16 const_SingleTurboSpeed; // OK

	// 0x432 - 0x0E
	s16 const_SacredFireSpeed; // OK

	// 0x434 - 0x0F
	s16 const_BackwardSpeed; // OK

	// 0x436 - 0x10
	// for aku and uka weapons
	s16 const_MaskSpeed;

	// 0x438 - 0x11
	s16 const_DamagedSpeed;

	// 0x43A - 0x12
	u8 const_TurnRate; // OK

	// 0x43B - 0x13
	u8 const_BackwardTurnRate; // OK

	// 0x43C - 0x14
	s16 const_TurnDecreaseRate; // OK

	// 0x43E - 0x15
	s16 const_TurnInputDelay; // OK

	// 0x440 - 0x16
	s16 const_PreTurbo;

	// 0x442 - 0x17
	s16 const_TerminalVelocity; // OK

	// 0x444 - 0x18
	s16 const_TerrainFrictionBoost;

	// 0x446 - 0x19
	s8 const_SteerAccel_Stage4_FirstFrame;

	// 0x447 - 0x1A
	s8 const_SteerAccel_Stage2_FirstFrame;

	// 0x448 - 0x1B
	s8 const_SteerAccel_Stage2_FrameLength;

	// not part of metaPhys
	u8 boolFirstFrameSinceRevEngine;

	// 0x44a - 0x1C
	s16 const_SteerAccel_Stage1_MaxSteer;

	// 0x44c - 0x1D
	s16 const_SteerAccel_Stage1_MinSteer;

	// 0x44e - 0x1E
	s16 const_SteerAccelTurnVelScale;

	// 0x450 - 0x1F
	s16 const_SteerAccelTurnVelLimit;

	// 0x452, 0x454 - 0x20, 0x21
	// exaggerate model rotation while steering
	s16 const_modelRotVelMax; // OK
	s16 const_modelRotVelMin;

	// 0x456 - [part of 0x22?]
	// either unused, or a s16,
	// and then 457 is just the top
	// byte of the s16
	char unusedPadding;

	// all related to VehPhysGeneral_LerpToForwards
	// only affected by steering without sliding
	u8 const_ModelTurnCounterSteerStrength;
	u8 const_ModelTurnReturnStrength;
	u8 const_ModelTurnNegativeReturnStrength;
	u8 const_ModelTurnVelocityLerp;

	char padding_0x45b;

	// 0x45c, 0x45d - 0x26, 0x27
	// resist turning at low speed
	u8 const_turnResistMin;
	u8 const_turnResistMax;

	// 0x45e, 0x45f - 0x28, 0x29
	// hold drift in direction it started,
	// compared to switching direction "SwitchWay"
	s8 const_SteerVel_DriftSwitchWay;
	s8 const_SteerVel_DriftStandard;

	// all VehPhysProc_PowerSlide_PhysAngular
	// or VehPhysProc_PowerSlide_Finalize
	s8 const_DriftTurnBase;
	s8 const_DriftTurnStartupScale;
	u8 const_DriftTurnRampFrames;

	// 0x463 - 0x2D
	u8 const_Drifting_FramesTillSpinout;

	// 0x464, 0x466, 0x468 - 0x2E, 0x2F, 0x30
	s16 const_DriftSpinRateAccel;
	s16 const_DriftSpinRateDecel;
	s16 const_Drifting_CameraSpinRate;

	// 0x46A, 0x46B - 0x31, 0x32
	u8 const_DriftCameraLerpStep;
	u8 const_DriftReleaseTurnAssistFrames;

	// 0x46C, 0x46E, 0x470, 0x472, 0x474 - 0x33, 0x34, 0x35, 0x36, 0x37
	s16 const_MetaPhys33;
	s16 const_MetaPhys34;
	s16 const_DriftTurnSameDirectionAngle;
	s16 const_DriftTurnOppositeDirectionAngle;
	s16 const_DriftTurnAngleScale;

	// 0x476, 0x477, 0x478, 0x479, 0x47A - 0x38, 0x39, 0x3A, 0x3B, 0x3C
	u8 const_turboMaxRoom;        // point where turbo meter is empty
	u8 const_turboLowRoomWarning; // point where turbo turns red
	u8 const_turboFullBarReserveGain;
	u8 const_DriftBoostDurationFrames;
	u8 const_DriftBoostAxisKickRate;

	EngineSoundMode engineSoundMode;

	// 0x47C, 0x47E, 0x480 - 0x3D, 0x3E, 0x3F
	s16 const_CollisionWeight;
	s16 const_SlopeForwardSpeedBonus;
	s16 const_SideSpeedClamp;

	// 0x484 - last of "main" constants,
	// skip over 482, which is non-const

	// 0x482
	// 1st, 2nd, 3rd, etc
	s16 driverRank;

	// 0x484 - MetaPhys stat no. 0x40
	// Used in Aug4 and Aug14
	int const_prototypeKey;

	// 0x484 - last of constants

	// 0x488
	u32 distanceToFinish_curr;

	// 0x48C
	u32 distanceToFinish_checkpoint;

	// 0x490
	u32 distanceDrivenBackwards;

	// 0x494
	struct DriverCheckpointState checkpoint;

	// 0x496
	u16 engineVol;

	// 0x498
	struct Instance *instBigNum;

	// 0x49c
	struct Instance *instFruitDisp;

	// 0x4a0
	// raincloud when you hit red potion
	struct Thread *thCloud;

	// 0x4a4
	// pointer Tracking thread that
	// is chasing this driver (missile/warpball)
	struct Thread *thTrackingMe;

	// 0x4a8
	// Papu pyramid plants, see 231.c
	struct Thread *plantEatingMe;

	// 0x4ac
	int damageColorTimer;

	struct
	{
		// 0x4b0
		int cooldown;

		// 0x4b4
		s16 startX;
		s16 startY;

	} PickupTimeboxHUD;

	struct
	{
		// 0x4b8
		int cooldown;

		// 0x4bc
		s16 startX;
		s16 startY;

		// 0x4c0
		int numCollected;

	} PickupWumpaHUD;

	struct
	{
		// 0x4c4
		s16 cooldown;

		// 0x4c6
		s16 modelID;

		// 0x4c8
		s16 startX;
		s16 startY;

		// 0x4cc
		int numCollected;

	} PickupLetterHUD;

	struct
	{
		// 0x4d0
		int cooldown;

		// 0x4d4
		s16 startX;
		s16 startY;

		// 0x4d8
		int reserved_0x4d8;

		// 0x4dc
		int scoreDelta; // -1, 0, 1

		// 0x4e0
		int juicedUpCooldown;

		// 0x4e4
		int numLives;

		// 0x4e8
		int teamID;

	} BattleHUD;

	// 0x4ec
	int framesSinceRaceEnded_forThisDriver;

	// 0x4f0
	// naughty dog bug
	s16 quip1;

	// 0x4f2
	s16 quip2;

	// 0x4f4
	s16 quip3;

	// 0x4f6
	s16 quip4;

	// 0x4f8
	struct Instance *wakeInst;

	// 0x4fc
	s16 wakeScale;

	// 0x4fe
	// 0, 1, 2, depending on rev level
	char revEngineState;

	// 0x4ff
	u8 pendingDamageType;

	// 0x500
	struct Driver *pendingDamageAttacker;

	// 0x504
	u8 pendingDamageReasonByte;
	u8 pendingDamageReasonPadding[3];

	// 0x508
	// backup of alpha, used for turbo fire
	s16 alphaScaleBackup;

	// 0x50A
	RainCloudEffect rainCloudEffect;

	// 0x50C
	char numTimesAttackingPlayer[8];

	// 0x514
	int timeElapsedInRace;

	// 0x518
	int distanceDriven;

	// 0x51c
	int timeSpentWithHighSpeed;

	// 0x520
	int timeSpentReversing;

	// 0x524
	int timeSpentInMud;

	// 0x528
	int timeSpentInLastPlace;

	// 0x52C
	int timeSpentInTenWumpa;

	// 0x530
	int timeSpentAgainstWall;

	// 0x534
	int timeSpentUsingReserves;

	// 0x538
	int timeSpentDrifting;

	// 0x53C
	int timeSpentJumping;

	// 0x540
	int timeSpentSpinningOut;

	// 0x544
	int timeSpentSquished;

	// 0x548
	int timeSpentEaten;

	// 0x54c
	int timeSpentBurnt;

	// 0x550 (not an int)
	s16 highestJump;

	// 0x552
	s16 longestShot;

	// 0x554
	s16 numberOfJumps;

	// 0x556
	char numTimesMovingPotionHitSomeone;

	// 0x557
	char numTimesMissileHitSomeone;

	// 0x558
	char numTimesClockWeaponUsed;

	// 0x559
	char numTimesAttacking;

	// 0x55a
	char numTimesBombsHitSomeone;

	// 0x55b
	char numTimesSquishedSomeone;

	// 0x55c
	char numTimesMissileLaunched;

	// 0x55d
	char numTimesMissileHitYou;

	// 0x55e
	char numTimesBombHitYou;

	// 0x55f
	char numTimesMotionlessPotionHitYou;

	// 0x560
	char numTimesAttackedByPlayer[8];

	// 0x568
	char numTimesHitWeaponBox;

	// 0x569
	u8 numTimesWumpa;

	// 0x56a
	char numTimesMaskGrab;

	// 0x56b
	// padding for the next int
	char unused_alignment_56b;

	// example, is this ever used btw?
	// lngIndex "Hit Most By..."
	// characterID "Crash Bandicoot"

	// 0x56c
	s16 *EndOfRaceComment_ptrQuip;

	// 0x570
	int EndOfRaceComment_characterID;

	// 0x574
	int NumMissilesComparedToNumAttacks;

	// 0x578
	// probably for "comeback kid!" comment
	int TimeWinningDriverSpentLastPlace;

	// 0x57c
	int numTimesAttacked;

	// ==== End of Quip Data ====

	// 0x580 - 0x598
	union
	{
		// state 2
		struct
		{
			// 0x580
			// used to determine spinout
			s16 numFramesDrifting;

			// 0x582
			s16 driftBoostTimeMS;

			// 0x584
			s16 driftTotalTimeMS;

			// 0x586
			s8 numBoostsAttempted;

			// 0x587
			s8 numBoostsSuccess;

			// == end ==

		} Drifting;

		// state 2
		struct
		{
			// 0x580
			s16 driftSpinRate;

			// 0x582
			s16 spinDir;

			// more?

		} Spinning;

		// state 4
		struct
		{
			// 0x580
			// object connected to thread
			struct MaskHeadWeapon *maskObj;

			// == Needs More Research ==

			// 0x584
			int boostMeter;

			// 0x588
			int fireLevel;

			// 0x58c
			s16 overRevTimerMS;

			// 0x58e
			s16 releaseCooldownTimerMS;

			// 0x590
			s16 emptyCooldownTimerMS;

			// 0x592
			RevEngineChargeState chargeState;
			RevEngineLockoutFlags lockoutFlags;

			// 0x594
			int boolMaskGrab;

			// == end ==

		} RevEngine;

		// state 5
		struct
		{
			// 0x580
			// object connected to thread
			struct MaskHeadWeapon *maskObj;

			// 0x584
			SVec3 AngleAxis_NormalVec;

			// 0x58a
			s16 animFrame;

			// 0x58c
			u8 boolParticlesSpawned;
			u8 boolStillFalling;
			u8 boolLiftingPlayer;
			u8 boolWhistle;

			// 0x590
			u8 unused[8];

		} MaskGrab;

		// state 5
		struct
		{
			// 0x580
			u8 boolInited;

			// == end ==

		} EatenByPlant;

		// state 6
		struct
		{
			u8 boolPlayBackwards;
		} Blasted;

		// state 10
		struct
		{
			// 0x580
			int timer;
			// 0x584
			int heightOffset;
			// 0x588
			int quadHeight;
			// 0x58c
			int numParticle;
			// 0x590
			int beamHeight;
		} Warp;

	} KartStates;

	// 0x598
	// === Robotcar and Ghost ===
	struct BotData botData;

	// ===========================================

	// NTSC is 0x62C bytes large
	// PAL is 0x630 bytes large

	// ===========================================

	// 0x62C - 0x670 reserved for ghost

	// 0x62C
	struct GhostTape *ghostTape;

	// 0x630
	s16 ghostID;

	// 0x632
	s16 ghostBoolInit;

	// 0x634
	s16 ghostBoolStarted;
	s16 ghostPadding_0x636;

	// 0x638
	// end of ghost struct (as determined by memset)

	// 0x30 in between are never used, "0x670" was simply the
	// largest Naughty Dog would allow themselves to push the driver,
	// but it is not the "real" size

	// 0x668 - size of pool object, minus object pool pointers
	// 0x670 - size of pool object
};

enum
{
	DRIVER_NTSC_RETAIL_SIZE = 0x62c,
};

CTR_STATIC_ASSERT(sizeof(struct MetaPhys) == 0x1C);
CTR_STATIC_ASSERT(sizeof(DriverFunc) == sizeof(void *));
CTR_STATIC_ASSERT(DRIVER_FUNC_INIT == 0);
CTR_STATIC_ASSERT(DRIVER_FUNC_UPDATE == 1);
CTR_STATIC_ASSERT(DRIVER_FUNC_PHYS_LINEAR == 2);
CTR_STATIC_ASSERT(DRIVER_FUNC_AUDIO == 3);
CTR_STATIC_ASSERT(DRIVER_FUNC_PHYS_ANGULAR == 4);
CTR_STATIC_ASSERT(DRIVER_FUNC_APPLY_FORCES == 5);
CTR_STATIC_ASSERT(DRIVER_FUNC_COLL_MOVED == 6);
CTR_STATIC_ASSERT(DRIVER_FUNC_COLLIDE_DRIVERS == 7);
CTR_STATIC_ASSERT(DRIVER_FUNC_COLL_FIXED == 8);
CTR_STATIC_ASSERT(DRIVER_FUNC_JUMP_FRICTION == 9);
CTR_STATIC_ASSERT(DRIVER_FUNC_TRANSLATE_MATRIX == 10);
CTR_STATIC_ASSERT(DRIVER_FUNC_ANIMATE == 11);
CTR_STATIC_ASSERT(DRIVER_FUNC_PARTICLES == 12);
CTR_STATIC_ASSERT(DRIVER_FUNC_COUNT == 13);
CTR_STATIC_ASSERT(sizeof(struct DriverCheckpointState) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct DriverCheckpointState, branchChoiceIndex) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct DriverCheckpointState, currentIndex) == 0x1);

CTR_STATIC_ASSERT(sizeof(struct BotPhysics) == 0x34);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, rotXZ) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, driftTarget) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, mulDrift) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, simpTurnState) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, turboMeter) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, fireLevel) == 0xa);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, squishCooldown) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, reserved_0x5cc) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, speedY) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, speedLinear) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, accel) == 0x1c);
CTR_STATIC_ASSERT(offsetof(struct BotPhysics, velocity) == 0x28);
CTR_STATIC_ASSERT(sizeof(((struct BotPhysics *)0)->accel) == 0xc);
CTR_STATIC_ASSERT(sizeof(((struct BotPhysics *)0)->velocity) == 0xc);
CTR_STATIC_ASSERT(sizeof(struct BotData) == 0x94);
CTR_STATIC_ASSERT(offsetof(struct BotData, aiPhysics) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct BotData, reserved_0x5a0) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct BotData, reserved_0x5ac) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct BotData, positionBackup) == 0x58);
CTR_STATIC_ASSERT(offsetof(struct BotData, ai_quadblock_checkpointIndex) == 0x72);
CTR_STATIC_ASSERT(offsetof(struct BotData, reserved_0x628) == 0x90);
CTR_STATIC_ASSERT(offsetof(struct BotData, estimateNavFrame) == 0x74);
CTR_STATIC_ASSERT(offsetof(struct BotData, estimatePosition) == 0x74);
CTR_STATIC_ASSERT(sizeof(((struct BotData *)0)->estimatePosition) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct BotData, estimateFlags) == 0x82);
CTR_STATIC_ASSERT(offsetof(struct BotData, estimateTail) == 0x84);
CTR_STATIC_ASSERT(BOT_FLAG_ESTIMATE_NAV == 0x1);
CTR_STATIC_ASSERT(BOT_FLAG_DAMAGE_ACTIVE == 0x2);
CTR_STATIC_ASSERT(BOT_FLAG_DAMAGE_SUPPRESS_EMITTER == 0x4);
CTR_STATIC_ASSERT(BOT_FLAG_FREE_PHYSICS == 0x8);
CTR_STATIC_ASSERT(BOT_FLAG_NAV_BOOST_ACTIVE == 0x10);
CTR_STATIC_ASSERT(BOT_FLAG_MOON_GRAVITY == 0x20);
CTR_STATIC_ASSERT(BOT_FLAG_BOSS_PATH_REQUESTED == 0x40);
CTR_STATIC_ASSERT(BOT_FLAG_BOSS_PATH_ACTIVE == 0x80);
CTR_STATIC_ASSERT(BOT_FLAG_DEMO_CAMERA_STARTED == 0x100);
CTR_STATIC_ASSERT(BOT_FLAG_STARTLINE_INIT_DONE == 0x200);
CTR_STATIC_ASSERT(sizeof(Actions) == 0x4);
CTR_STATIC_ASSERT(ACTION_TOUCH_GROUND == 0x1);
CTR_STATIC_ASSERT(ACTION_STARTED_TOUCH_GROUND == 0x2);
CTR_STATIC_ASSERT(ACTION_JUMP_BUTTON_HELD == 0x4);
CTR_STATIC_ASSERT(ACTION_ACCEL_PREVENTION == 0x8);
CTR_STATIC_ASSERT(ACTION_STEER_LEFT == 0x10);
CTR_STATIC_ASSERT(ACTION_BRAKE_WITH_ACCEL == 0x20);
CTR_STATIC_ASSERT(ACTION_HIGH_JUMP == 0x40);
CTR_STATIC_ASSERT(ACTION_TURBO_INPUT_LATCH == 0x80);
CTR_STATIC_ASSERT(ACTION_DRIVING_WRONG_WAY == 0x100);
CTR_STATIC_ASSERT(ACTION_TURBO_ITEM == 0x200);
CTR_STATIC_ASSERT(ACTION_JUMP_STARTED == 0x400);
CTR_STATIC_ASSERT(ACTION_BACK_SKID == 0x800);
CTR_STATIC_ASSERT(ACTION_FRONT_SKID == 0x1000);
CTR_STATIC_ASSERT(ACTION_DRIVING_AGAINST_WALL == 0x2000);
CTR_STATIC_ASSERT(ACTION_WARP == 0x4000);
CTR_STATIC_ASSERT(ACTION_WEAPON_FIRE_REQUEST == 0x8000);
CTR_STATIC_ASSERT(ACTION_ENGINE_ECHO == 0x10000);
CTR_STATIC_ASSERT(ACTION_REVERSING_ENGINE == 0x20000);
CTR_STATIC_ASSERT(ACTION_RACE_TIMER_FROZEN == 0x40000);
CTR_STATIC_ASSERT(ACTION_AIRBORNE == 0x80000);
CTR_STATIC_ASSERT(ACTION_BOT == 0x100000);
CTR_STATIC_ASSERT(ACTION_NEW_BOOST == 0x200000);
CTR_STATIC_ASSERT(ACTION_ACCEL_RELEASED_WITH_RESERVES == 0x400000);
CTR_STATIC_ASSERT(ACTION_MASK_WEAPON == 0x800000);
CTR_STATIC_ASSERT(ACTION_BEHIND_START_LINE == 0x1000000);
CTR_STATIC_ASSERT(ACTION_RACE_FINISHED == 0x2000000);
CTR_STATIC_ASSERT(ACTION_TRACKER_TARGETED == 0x4000000);
CTR_STATIC_ASSERT(ACTION_CHECKPOINT_BRANCH_PENDING == 0x8000000);
CTR_STATIC_ASSERT(ACTION_HUMAN_HUMAN_COLLISION == 0x10000000);
CTR_STATIC_ASSERT(ACTION_REVERSE_STEER_LEFT == 0x20000000);
CTR_STATIC_ASSERT(ACTION_REVERSE_STEER_RIGHT == 0x40000000);
CTR_STATIC_ASSERT(ACTION_DROPPING_MINE == 0x80000000u);
CTR_STATIC_ASSERT(sizeof(DriverCollisionFlags) == 0x2);
CTR_STATIC_ASSERT(sizeof(RainCloudEffect) == 0x2);
CTR_STATIC_ASSERT(sizeof(union VehEmitterSkidmark) == 0x10);
CTR_STATIC_ASSERT(offsetof(union VehEmitterSkidmark, edge[0]) == 0x0);
CTR_STATIC_ASSERT(offsetof(union VehEmitterSkidmark, edge[1]) == 0x8);
CTR_STATIC_ASSERT(offsetof(union VehEmitterSkidmark, edge0) == 0x0);
CTR_STATIC_ASSERT(offsetof(union VehEmitterSkidmark, color) == 0x6);
CTR_STATIC_ASSERT(offsetof(union VehEmitterSkidmark, flags) == 0x7);
CTR_STATIC_ASSERT(offsetof(union VehEmitterSkidmark, edge1) == 0x8);
CTR_STATIC_ASSERT(sizeof(union VehEmitterWallScratch) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, projected) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, pushBuffer) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, colorNear) == 0x1c);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, colorFar) == 0x20);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, segmentFlags) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, currXY) == 0x28);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, prevXY) == 0x4c);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, currDepth) == 0x70);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, prevDepth) == 0x94);
CTR_STATIC_ASSERT(offsetof(struct VehGroundSkidsScratch, origin) == 0xb8);
CTR_STATIC_ASSERT(sizeof(struct VehGroundSkidsScratch) == 0xc4);
CTR_STATIC_ASSERT(offsetof(struct VehPhysCrashAiScratch, forward) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct VehPhysCrashAiScratch, matrix) == 0x10);
CTR_STATIC_ASSERT(sizeof(struct VehPhysCrashAiScratch) == 0x30);
CTR_STATIC_ASSERT(RAIN_CLOUD_EFFECT_NONE == 0x4);
CTR_STATIC_ASSERT(DRIVER_COLL_FLAG_MASK_GRAB_REQUEST == 0x1);
CTR_STATIC_ASSERT(DRIVER_COLL_FLAG_SURFACE_PUSHBACK == 0x2);
CTR_STATIC_ASSERT(DRIVER_COLL_FLAG_TOUCHED_QUADBLOCK == 0x4);
CTR_STATIC_ASSERT(DRIVER_COLL_FLAG_GROUNDED == 0x8);
CTR_STATIC_ASSERT(sizeof(ForcedJumpType) == 0x1);
CTR_STATIC_ASSERT(FORCED_JUMP_NONE == 0);
CTR_STATIC_ASSERT(FORCED_JUMP_LOW == 1);
CTR_STATIC_ASSERT(FORCED_JUMP_HIGH == 2);
CTR_STATIC_ASSERT(sizeof(RevEngineChargeState) == 0x1);
CTR_STATIC_ASSERT(sizeof(RevEngineLockoutFlags) == 0x1);
CTR_STATIC_ASSERT(sizeof(EngineSoundMode) == 0x1);

CTR_STATIC_ASSERT(offsetof(struct Driver, ghostTape) == DRIVER_NTSC_RETAIL_SIZE);
CTR_STATIC_ASSERT(sizeof(((struct Driver *)0)->funcPtrs) == DRIVER_FUNC_COUNT * sizeof(DriverFunc));
#if BUILD < EurRetail
CTR_STATIC_ASSERT(offsetof(struct Driver, funcPtrs) == 0x54);
#else
CTR_STATIC_ASSERT(offsetof(struct Driver, funcPtrs) == 0x58);
#endif
CTR_STATIC_ASSERT(offsetof(struct Driver, velocity) == 0x88);
CTR_STATIC_ASSERT(sizeof(((struct Driver *)0)->velocity) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct Driver, collisionFlags) == 0xaa);
CTR_STATIC_ASSERT(offsetof(struct Driver, spsHitPos) == 0xac);
CTR_STATIC_ASSERT(offsetof(struct Driver, padding_0xb2) == 0xb2);
CTR_STATIC_ASSERT(offsetof(struct Driver, spsNormalVec) == 0xb4);
CTR_STATIC_ASSERT(offsetof(struct Driver, padding_0xba) == 0xba);
CTR_STATIC_ASSERT(offsetof(struct Driver, stepFlagSet) == 0xbc);
CTR_STATIC_ASSERT(offsetof(struct Driver, skidmarks) == 0xc4);
CTR_STATIC_ASSERT(offsetof(struct Driver, skidmarkEnableFlags) == 0x2c4);
CTR_STATIC_ASSERT(sizeof(((struct Driver *)0)->stepFlagSet) == 0x4);
CTR_STATIC_ASSERT(sizeof(((struct Driver *)0)->spsHitPosRaw) == 0x8);
CTR_STATIC_ASSERT(sizeof(((struct Driver *)0)->spsNormalVecRaw) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct Driver, padding_0x3e) == 0x3e);
CTR_STATIC_ASSERT(offsetof(struct Driver, actionsFlagSet) == 0x2c8);
CTR_STATIC_ASSERT(offsetof(struct Driver, actionsFlagSetPrevFrame) == 0x2cc);
CTR_STATIC_ASSERT(offsetof(struct Driver, quadBlockHeight) == 0x2d0);
CTR_STATIC_ASSERT(offsetof(struct Driver, posCurr) == 0x2d4);
CTR_STATIC_ASSERT(sizeof(((struct Driver *)0)->posCurr) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct Driver, forcedJumpType) == 0x366);
CTR_STATIC_ASSERT(offsetof(struct Driver, AxisAngle2_normalVec) == 0x368);
CTR_STATIC_ASSERT(offsetof(struct Driver, speedometerNeedleValue) == 0x36e);
CTR_STATIC_ASSERT(offsetof(struct Driver, AxisAngle3_normalVec) == 0x370);
CTR_STATIC_ASSERT(offsetof(struct Driver, AxisAngle4_normalVec) == 0x378);
CTR_STATIC_ASSERT(offsetof(struct Driver, padding_0x37e) == 0x37e);
CTR_STATIC_ASSERT(offsetof(struct Driver, failedBoostExhaustTimer) == 0x381);
CTR_STATIC_ASSERT(offsetof(struct Driver, forwardAccelVector) == 0x3ac);
CTR_STATIC_ASSERT(offsetof(struct Driver, forwardAccelImpulse) == 0x3b2);
CTR_STATIC_ASSERT(offsetof(struct Driver, engineSoundVolumeState) == 0x3b6);
CTR_STATIC_ASSERT(offsetof(struct Driver, engineSoundPitchState) == 0x3b8);
CTR_STATIC_ASSERT(offsetof(struct Driver, tireColorCycleTimer) == 0x3bc);
CTR_STATIC_ASSERT(offsetof(struct Driver, tireColorCycleStep) == 0x3be);
CTR_STATIC_ASSERT(offsetof(struct Driver, accelTapWindowTimer) == 0x3c0);
CTR_STATIC_ASSERT(offsetof(struct Driver, accelTapCount) == 0x3c2);
CTR_STATIC_ASSERT(offsetof(struct Driver, terrainScaledBaseSpeed) == 0x3c4);
CTR_STATIC_ASSERT(offsetof(struct Driver, accel) == 0x3cc);
CTR_STATIC_ASSERT(sizeof(((struct Driver *)0)->accel) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct Driver, turnWobbleAngle) == 0x3d4);
CTR_STATIC_ASSERT(offsetof(struct Driver, turnWobbleVelocity) == 0x3d6);
CTR_STATIC_ASSERT(offsetof(struct Driver, turnWobbleTimer) == 0x3d8);
CTR_STATIC_ASSERT(offsetof(struct Driver, jump_HighJumpTimerMS) == 0x3fa);
CTR_STATIC_ASSERT(offsetof(struct Driver, posWallColl) == 0x384);
CTR_STATIC_ASSERT(offsetof(struct Driver, wallRubSpeedLimit) == 0x38a);
CTR_STATIC_ASSERT(offsetof(struct Driver, padding_0x398) == 0x398);
CTR_STATIC_ASSERT(offsetof(struct Driver, wallRubTimer) == 0x3fe);
CTR_STATIC_ASSERT(offsetof(struct Driver, vShiftStartGuardTimer) == 0x406);
CTR_STATIC_ASSERT(offsetof(struct Driver, vShiftWindowTimer) == 0x408);
CTR_STATIC_ASSERT(offsetof(struct Driver, vShiftCount) == 0x40a);
CTR_STATIC_ASSERT(offsetof(struct Driver, reserved_0x40e) == 0x40e);
CTR_STATIC_ASSERT(offsetof(struct Driver, reserved_0x412) == 0x412);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_PreTurbo) == 0x440);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_SteerAccelTurnVelScale) == 0x44e);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_SteerAccelTurnVelLimit) == 0x450);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_ModelTurnCounterSteerStrength) == 0x457);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_ModelTurnReturnStrength) == 0x458);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_ModelTurnNegativeReturnStrength) == 0x459);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_ModelTurnVelocityLerp) == 0x45a);
CTR_STATIC_ASSERT(offsetof(struct Driver, padding_0x45b) == 0x45b);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftTurnBase) == 0x460);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftTurnStartupScale) == 0x461);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftTurnRampFrames) == 0x462);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_Drifting_FramesTillSpinout) == 0x463);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftSpinRateAccel) == 0x464);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftSpinRateDecel) == 0x466);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_Drifting_CameraSpinRate) == 0x468);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftCameraLerpStep) == 0x46a);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftReleaseTurnAssistFrames) == 0x46b);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_MetaPhys33) == 0x46c);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_MetaPhys34) == 0x46e);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftTurnSameDirectionAngle) == 0x470);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftTurnOppositeDirectionAngle) == 0x472);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftTurnAngleScale) == 0x474);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_turboFullBarReserveGain) == 0x478);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftBoostDurationFrames) == 0x479);
CTR_STATIC_ASSERT(offsetof(struct Driver, const_DriftBoostAxisKickRate) == 0x47a);
CTR_STATIC_ASSERT(offsetof(struct Driver, engineSoundMode) == 0x47b);
CTR_STATIC_ASSERT(offsetof(struct Driver, checkpoint) == 0x494);
CTR_STATIC_ASSERT(offsetof(struct Driver, checkpoint.branchChoiceIndex) == 0x494);
CTR_STATIC_ASSERT(offsetof(struct Driver, checkpoint.currentIndex) == 0x495);
CTR_STATIC_ASSERT(offsetof(struct Driver, BattleHUD.reserved_0x4d8) == 0x4d8);
CTR_STATIC_ASSERT(offsetof(struct Driver, pendingDamageType) == 0x4ff);
CTR_STATIC_ASSERT(offsetof(struct Driver, pendingDamageAttacker) == 0x500);
CTR_STATIC_ASSERT(offsetof(struct Driver, pendingDamageReasonByte) == 0x504);
CTR_STATIC_ASSERT(sizeof(((struct Driver *)0)->pendingDamageReasonPadding) == 0x3);
CTR_STATIC_ASSERT(offsetof(struct Driver, rainCloudEffect) == 0x50a);
CTR_STATIC_ASSERT(offsetof(struct Driver, numTimesWumpa) == 0x569);
CTR_STATIC_ASSERT(offsetof(struct Driver, ghostPadding_0x636) == 0x636);
CTR_STATIC_ASSERT(offsetof(struct Driver, KartStates.RevEngine.overRevTimerMS) == 0x58c);
CTR_STATIC_ASSERT(offsetof(struct Driver, KartStates.RevEngine.releaseCooldownTimerMS) == 0x58e);
CTR_STATIC_ASSERT(offsetof(struct Driver, KartStates.RevEngine.emptyCooldownTimerMS) == 0x590);
CTR_STATIC_ASSERT(offsetof(struct Driver, KartStates.RevEngine.chargeState) == 0x592);
CTR_STATIC_ASSERT(offsetof(struct Driver, KartStates.RevEngine.lockoutFlags) == 0x593);
CTR_STATIC_ASSERT(offsetof(struct Driver, rotCurr.x) == 0x2ec);
CTR_STATIC_ASSERT(offsetof(struct Driver, rotCurr.y) == 0x2ee);
CTR_STATIC_ASSERT(offsetof(struct Driver, rotCurr.z) == 0x2f0);
CTR_STATIC_ASSERT(offsetof(struct Driver, rotCurr.w) == 0x2f2);
CTR_STATIC_ASSERT(offsetof(struct Driver, rotPrev.x) == 0x2f4);
CTR_STATIC_ASSERT(offsetof(struct Driver, rotPrev.y) == 0x2f6);
CTR_STATIC_ASSERT(offsetof(struct Driver, rotPrev.z) == 0x2f8);
CTR_STATIC_ASSERT(offsetof(struct Driver, rotPrev.w) == 0x2fa);
CTR_STATIC_ASSERT(offsetof(struct Driver, KartStates.MaskGrab.AngleAxis_NormalVec) == 0x584);
