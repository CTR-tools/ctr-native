struct BoundingBox
{
	SVec3 min;
	SVec3 max;
};

enum CollFixedPoint
{
	COLL_FRACTION_ONE = FP_ONE,
	COLL_FRACTION_INVALID = -FP_ONE,
};

typedef enum CollSearchFlags : u16
{
	COLL_SEARCH_TEST_INSTANCES = 0x1,
	COLL_SEARCH_HIGH_LOD = 0x2,
	COLL_SEARCH_REUSE_NORMALS = 0x8,
	COLL_SEARCH_WALL_PROJECTION_DONE = 0x10,
	COLL_SEARCH_REPEAT_SCRUB = 0x20,
	COLL_SEARCH_FORCE_INSTANCE_HIT = 0x40,
} CollSearchFlags;

typedef enum CollStepFlags : u32
{
	// NOTE(aalhendi): Trigger quads OR their terrain_type byte into this
	// word; low bits are packed trigger payload, not ordinary terrain.
	COLL_STEP_TRIGGER_TURBO_PAD = 0x1,
	COLL_STEP_TRIGGER_SUPER_TURBO_PAD = 0x2,
	COLL_STEP_TRIGGER_WEAPON_REACT = 0x4,
	COLL_STEP_TRIGGER_SUPPRESS_MASK_GRAB = 0x8,
	COLL_STEP_TRIGGER_HUB_LEVEL_ID_MASK = 0x30,
	COLL_STEP_TRIGGER_HUB_LEVEL_ID_SHIFT = 4,
	COLL_STEP_TRIGGER_HUB_SWAP_NOW_MASK = 0xc0,
	COLL_STEP_TRIGGER_HUB_SWAP_NOW_SHIFT = 6,
	COLL_STEP_TRIGGER_TURBO_PAD_MASK = COLL_STEP_TRIGGER_TURBO_PAD | COLL_STEP_TRIGGER_SUPER_TURBO_PAD,
	COLL_STEP_FLAG_KILL_PLANE = 0x4000,
	COLL_STEP_FLAG_WATER_BSP = 0x8000,
} CollStepFlags;

enum CollModelIDFlags
{
	COLL_MODELID_VALUE_MASK = 0x7fff,
	COLL_MODELID_BLOCKAGE_FLAG = 0x8000,
};

typedef enum CollNormalAxis : s16
{
	COLL_NORMAL_AXIS_Z = 1,
	COLL_NORMAL_AXIS_X = 2,
	COLL_NORMAL_AXIS_Y = 3,
} CollNormalAxis;

enum CollQuadTriangleId
{
	COLL_QUAD_TRIANGLE_LO_0 = 0,
	COLL_QUAD_TRIANGLE_LO_1 = 1,
	COLL_QUAD_TRIANGLE_HI_0 = 2,
	COLL_QUAD_TRIANGLE_HI_1 = 3,
	COLL_QUAD_TRIANGLE_HI_2 = 4,
	COLL_QUAD_TRIANGLE_HI_3 = 5,
	COLL_QUAD_TRIANGLE_HI_4 = 6,
	COLL_QUAD_TRIANGLE_HI_5 = 7,
	COLL_QUAD_TRIANGLE_HI_6 = 8,
	COLL_QUAD_TRIANGLE_HI_7 = 9,
};

typedef enum CollTriangleClipResult : s8
{
	COLL_TRIANGLE_CLIP_MISS = -1,
	COLL_TRIANGLE_CLIP_V1 = 0,
	COLL_TRIANGLE_CLIP_EDGE_V1_V2 = 1,
	COLL_TRIANGLE_CLIP_V2 = 2,
	COLL_TRIANGLE_CLIP_EDGE_V2_V3 = 3,
	COLL_TRIANGLE_CLIP_V3 = 4,
	COLL_TRIANGLE_CLIP_EDGE_V1_V3 = 5,
	COLL_TRIANGLE_CLIP_FACE = 6,
} CollTriangleClipResult;

struct CollPlane
{
	SVec3 normal;

	// NOTE(aalhendi): Retail stores the plane constant shifted right by one;
	// plane-side tests recover it with -2 * halfDistance.
	s16 halfDistance;
};

struct BspSearchVertex
{
	// 0x0
	SVec3 pos;

	// 0x6
	// COLL_FIXED_TRIANGL_GetNormVec writes the dominant normal axis here;
	// COLL_FIXED_TRIANGL_TestPoint reads it during triangle projection.
	CollNormalAxis normalAxis;

	// 0x8
	struct LevVertex *pLevelVertex;

	// 0xC
	struct CollPlane plane;

	// 0x14 large
};

struct BspSearchTriangle
{
	struct QuadBlock *quadblock;
	s32 triangleID;
	s32 scrubDepth;
};

struct BspSearchResult
{
	// 0x0
	SVec3 hitPos;

	// 0x6
	CollNormalAxis normalAxis;

	// 0x8
	struct CollPlane plane;

	// 0x10
	SVec3 pushOut;

	// 0x16
	CollTriangleClipResult reorderResult;

	// enum CollQuadTriangleId stored as one byte in retail scratch.
	u8 triangleID;

	// 0x18
	struct QuadBlock *ptrQuadblock;
};

struct CollInstanceHitboxScratch
{
	s32 segmentDot;
	s32 centerDot;
	s32 lineFactor;
	s32 radiusSquared;
	s32 distanceSquared;
	s32 adjustedFactor;
	Vec3 segmentDelta;
	Vec3 centerDelta;
	Vec3 projectedDelta;
	Vec3 hitDelta;
	Vec3 normal;
	Vec3 scaledNormal;
};

struct CollScratchWork
{
	CollStepFlags stepFlags;
	s16 triNormalVecDividend;
	u8 triNormalVecBitShift;
	u8 triNormalLodShift;
	struct CollInstanceHitboxScratch instanceHitbox;
};

struct CollTriangleBarycentrics
{
	s16 v1;
	s16 v2;
};

struct CollLevelTriangle
{
	struct LevVertex *v0;
	struct LevVertex *v1;
	struct LevVertex *v2;
};

struct CollBspSearchTriangle
{
	struct BspSearchVertex *v0;
	struct BspSearchVertex *v1;
	struct BspSearchVertex *v2;
};

struct BSP;
struct Thread;
struct ScratchpadStruct;
typedef void (*CollBspLeafCallback)(struct BSP *bspLeaf, struct ScratchpadStruct *sps);
// Thread-bucket searches pass `struct Thread *`; level-instance searches pass `struct BSP *`.
typedef void (*CollThBuckCallback)(struct ScratchpadStruct *sps, void *hitObject);

// can be stored in normal RAM,
// usually 1f800108
struct ScratchpadStruct
{
	struct
	{
		// this "pos" for threads: center of object
		// this "pos" for quadblock: posMin of object

		// 0x0
		SVec3 pos;
		s16 hitRadius;

		// 0x8
		s32 hitRadiusSquared;

		// 0xC
		s16 modelID;
		s16 scrubDepth;
	} Input1;

	// 0x10
	union
	{
		// This is why pointer 1f800118 gets passed
		struct
		{
			// this "pos" for quadblock: posMax of object,
			// hitRadius could just be a copy

			// 0x10
			SVec3 pos;
			s16 hitRadius;

			// 0x18
			s32 hitRadiusSquared;

			// 0x1C
			SVec3 hitPos;

			// 0x22
			CollSearchFlags searchFlags;

			// 0x24
			// NOTE(aalhendi): Retail keeps these as word masks even though the
			// source flags are QuadBlockFlags.
			u32 quadFlagsWanted;

			// 0x28
			u32 quadFlagsIgnored;

		} QuadBlockColl;

		// when using this,
		// rest of struct is ignored
		struct
		{
			// 0x10
			// Written before invoking the thread/bsp callback.
			SVec3 centerDelta;

			// 0x16
			s16 pad16;

			// 0x18
			struct Thread *thread;

			// 0x1c
			struct BoundingBox bbox;

			// 0x28
			CollThBuckCallback funcCallback;

		} ThBuckColl;
	} Union;

	// 0x2C
	struct mesh_info *ptr_mesh_info;

	// 0x30
	struct BoundingBox bbox;

	// 0x3C
	s16 numTrianglesTested;

	// 0x3e
	s16 boolDidTouchQuadblock;

	// 0x40
	// Retail clears this during moved-player searches; no current consumer.
	s16 resetOnly40;

	// 0x42
	s16 boolDidTouchHitbox;

	// 0x44
	struct mesh_info *ptr_mesh_info_2;

	// 0x48
	struct BSP *bspHitbox;

	// 0x4c
	struct BspSearchResult candidate;

	// 0x68
	struct BspSearchResult hit;

	// 0x84
	// COLL_FRACTION_ONE means the whole segment is clear; lower values are the first hit fraction.
	s32 hitFraction;

	// 0x88
	// COLL_MOVED_PlayerSearch can test up to 15 hitboxes, so this prevents
	// duplicate collision handling within one search.
	struct BSP *bspHitboxesHit[15];

	// 0xc4
	s32 numBspHitboxesHit;

	// 0xc8, 0xca,
	struct CollTriangleBarycentrics hitBarycentrics;

	// 0xcc, 0xd0, 0xd4
	struct CollLevelTriangle hitLevelTriangle;

	// 0xd8, 0xdc, 0xe0
	struct CollBspSearchTriangle hitBspSearchTriangle;

	// 0xe4, 0xe6, 0xe8
	SVec3 candidateDelta;

	// 0xea
	s16 padEA;

	// 0xec, 0xee
	u16 quadSecondTriIndexA;
	u16 quadSecondTriIndexB;

	// 0xf0
	struct BspSearchVertex bspSearchVert[9];

	// 0x1a4
	struct CollScratchWork collision;

	// 0x20C -- size of struct
};

// only stored in scratchpad by COLL_MOVED_FindScrub
struct ScratchpadStructExtended
{
	// --- top half is for COLL ---

	struct ScratchpadStruct scratchpadStruct;

	// 0x20C
	struct BspSearchTriangle bspSearchTriangle[0xF];

	// 0x2c0
	s32 numTriangles;

	// 0x2c4 - 1f8003cc
	s32 padding2C4[2];

	// --- the rest is for pushBuffer funcs ---

	// 0x2cc - 1f8003d4
	MATRIX cameraMatrix;

	// 0x2ec - 1f8003f4
	SVec3 cameraRot;

	// 0x2f2 - 1f8003fa
	s16 pad2F2;

	// 0x2f4 - 1f8003fc
	s32 pad2F4;

	// 1f800400 end of memory
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct BoundingBox) == 0xC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BoundingBox, min) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BoundingBox, max) == 0x6);
CTR_STATIC_ASSERT_LAYOUT(sizeof(CollThBuckCallback) == sizeof(void *));
_Static_assert(COLL_FRACTION_ONE == 0x1000);
_Static_assert(COLL_FRACTION_INVALID == -0x1000);
CTR_STATIC_ASSERT_LAYOUT(sizeof(CollSearchFlags) == 0x2);
_Static_assert(COLL_SEARCH_TEST_INSTANCES == 0x1);
_Static_assert(COLL_SEARCH_HIGH_LOD == 0x2);
_Static_assert(COLL_SEARCH_REUSE_NORMALS == 0x8);
_Static_assert(COLL_SEARCH_WALL_PROJECTION_DONE == 0x10);
_Static_assert(COLL_SEARCH_REPEAT_SCRUB == 0x20);
_Static_assert(COLL_SEARCH_FORCE_INSTANCE_HIT == 0x40);
CTR_STATIC_ASSERT_LAYOUT(sizeof(CollStepFlags) == 0x4);
_Static_assert(COLL_STEP_TRIGGER_TURBO_PAD == 0x1);
_Static_assert(COLL_STEP_TRIGGER_SUPER_TURBO_PAD == 0x2);
_Static_assert(COLL_STEP_TRIGGER_WEAPON_REACT == 0x4);
_Static_assert(COLL_STEP_TRIGGER_SUPPRESS_MASK_GRAB == 0x8);
_Static_assert(COLL_STEP_TRIGGER_HUB_LEVEL_ID_MASK == 0x30);
_Static_assert(COLL_STEP_TRIGGER_HUB_LEVEL_ID_SHIFT == 4);
_Static_assert(COLL_STEP_TRIGGER_HUB_SWAP_NOW_MASK == 0xc0);
_Static_assert(COLL_STEP_TRIGGER_HUB_SWAP_NOW_SHIFT == 6);
_Static_assert(COLL_STEP_TRIGGER_TURBO_PAD_MASK == 0x3);
_Static_assert(COLL_STEP_FLAG_KILL_PLANE == 0x4000);
_Static_assert(COLL_STEP_FLAG_WATER_BSP == 0x8000);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct CollPlane) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CollPlane, normal) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CollPlane, halfDistance) == 0x6);
CTR_STATIC_ASSERT_LAYOUT(sizeof(SVec3) == 0x6);
CTR_STATIC_ASSERT_LAYOUT(sizeof(Vec3) == 0xC);
CTR_STATIC_ASSERT_LAYOUT(sizeof(CollNormalAxis) == 0x2);
_Static_assert(COLL_NORMAL_AXIS_Z == 1);
_Static_assert(COLL_NORMAL_AXIS_X == 2);
_Static_assert(COLL_NORMAL_AXIS_Y == 3);
_Static_assert(COLL_QUAD_TRIANGLE_LO_0 == 0);
_Static_assert(COLL_QUAD_TRIANGLE_LO_1 == 1);
_Static_assert(COLL_QUAD_TRIANGLE_HI_0 == 2);
_Static_assert(COLL_QUAD_TRIANGLE_HI_7 == 9);
CTR_STATIC_ASSERT_LAYOUT(sizeof(CollTriangleClipResult) == 0x1);
_Static_assert(COLL_TRIANGLE_CLIP_MISS == -1);
_Static_assert(COLL_TRIANGLE_CLIP_V1 == 0);
_Static_assert(COLL_TRIANGLE_CLIP_FACE == 6);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct BspSearchVertex) == 0x14);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct BspSearchTriangle) == 0xC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchTriangle, quadblock) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchTriangle, triangleID) == 0x4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchTriangle, scrubDepth) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct BspSearchResult) == 0x1C);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct CollInstanceHitboxScratch) == 0x60);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CollInstanceHitboxScratch, segmentDelta) == 0x18);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CollInstanceHitboxScratch, centerDelta) == 0x24);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CollInstanceHitboxScratch, projectedDelta) == 0x30);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CollInstanceHitboxScratch, hitDelta) == 0x3C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CollInstanceHitboxScratch, normal) == 0x48);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CollInstanceHitboxScratch, scaledNormal) == 0x54);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct CollScratchWork) == 0x68);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct CollTriangleBarycentrics) == 0x4);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct CollLevelTriangle) == 0xC);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct CollBspSearchTriangle) == 0xC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchVertex, pos) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchVertex, normalAxis) == 0x6);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchVertex, plane) == 0xC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchResult, hitPos) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchResult, normalAxis) == 0x6);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchResult, plane) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchResult, pushOut) == 0x10);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchResult, reorderResult) == 0x16);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchResult, triangleID) == 0x17);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct BspSearchResult, ptrQuadblock) == 0x18);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct ScratchpadStruct) == 0x20C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Input1.pos) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Input1.hitRadius) == 0x06);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Input1.modelID) == 0x0C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Input1.scrubDepth) == 0x0E);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.QuadBlockColl.pos) == 0x10);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.QuadBlockColl.hitRadius) == 0x16);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.QuadBlockColl.searchFlags) == 0x22);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.QuadBlockColl.hitPos) == 0x1C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.QuadBlockColl.quadFlagsWanted) == 0x24);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.QuadBlockColl.quadFlagsIgnored) == 0x28);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.ThBuckColl.centerDelta) == 0x10);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.ThBuckColl.pad16) == 0x16);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.ThBuckColl.thread) == 0x18);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.ThBuckColl.bbox) == 0x1C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, Union.ThBuckColl.funcCallback) == 0x28);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, numTrianglesTested) == 0x3C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, resetOnly40) == 0x40);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, candidate) == 0x4C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, candidate) + offsetof(struct BspSearchResult, hitPos) == 0x4C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, candidate) + offsetof(struct BspSearchResult, plane) == 0x54);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, candidate) + offsetof(struct BspSearchResult, pushOut) == 0x5C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, candidate.triangleID) == 0x63);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hit) == 0x68);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hit) + offsetof(struct BspSearchResult, hitPos) == 0x68);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hit) + offsetof(struct BspSearchResult, plane) == 0x70);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hit) + offsetof(struct BspSearchResult, pushOut) == 0x78);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hit.reorderResult) == 0x7E);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hit.triangleID) == 0x7F);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hit.ptrQuadblock) == 0x80);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitFraction) == 0x84);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, bspHitboxesHit) == 0x88);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, numBspHitboxesHit) == 0xC4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitBarycentrics) == 0xC8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitBarycentrics.v1) == 0xC8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitBarycentrics.v2) == 0xCA);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitLevelTriangle) == 0xCC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitLevelTriangle.v0) == 0xCC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitLevelTriangle.v1) == 0xD0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitLevelTriangle.v2) == 0xD4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitBspSearchTriangle) == 0xD8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitBspSearchTriangle.v0) == 0xD8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitBspSearchTriangle.v1) == 0xDC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, hitBspSearchTriangle.v2) == 0xE0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, candidateDelta) == 0xE4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, padEA) == 0xEA);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, quadSecondTriIndexA) == 0xEC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, quadSecondTriIndexB) == 0xEE);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, collision.stepFlags) == 0x1A4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, collision.triNormalVecDividend) == 0x1A8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, collision.triNormalVecBitShift) == 0x1AA);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, collision.triNormalLodShift) == 0x1AB);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStruct, collision.instanceHitbox) == 0x1AC);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct ScratchpadStructExtended) == 0x2F8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStructExtended, scratchpadStruct) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStructExtended, bspSearchTriangle) == 0x20C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStructExtended, numTriangles) == 0x2C0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStructExtended, padding2C4) == 0x2C4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStructExtended, cameraMatrix) == 0x2CC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStructExtended, cameraRot) == 0x2EC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStructExtended, pad2F2) == 0x2F2);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct ScratchpadStructExtended, pad2F4) == 0x2F4);
