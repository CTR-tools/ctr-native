struct FrustumCornerOUT
{
	SVec3 pos;
};

struct ScratchpadFrustum
{
	// 1f800000
	int pos[3];

	// 1f80000C
	struct FrustumCornerOUT fc[4];

	// 1f800024
	SVec3 camPos;

	// 1f80002A
	// -- end --
};

struct PushBufferSetMatrixVPScratch
{
	u8 reserved0[0x3d4];

	MATRIX cameraMatrix;
	SVec3 rot;
	u8 reserved1[0x6];
};

_Static_assert(offsetof(struct PushBufferSetMatrixVPScratch, cameraMatrix) == 0x3d4);
_Static_assert(offsetof(struct PushBufferSetMatrixVPScratch, rot) == 0x3f4);
_Static_assert(sizeof(struct PushBufferSetMatrixVPScratch) == CTR_SCRATCHPAD_SIZE);

// Let the compiler figure it out,
// the bitshifting annoys me
union FrustumCornerIN
{
	struct
	{
		s16 x;
		s16 y;
	};

	struct
	{
		int self;
	};
};

struct PushBuffer
{
	// 0x0
	SVec3 pos;

	// 0x6
	SVec3 rot;

	// 0xc
	// set at bottom of Camera_UpdateFrustum,
	// used in 226-229 overlays for LEV
	char data6[6];

	// 0x12
	// 0 for black,
	// 0x1000 for normal light
	// 0x2000 for white
	s16 fadeFromBlack_currentValue;

	// 0x14
	s16 fadeFromBlack_desiredResult;

	// 0x16 controls speed of fade in effect
	// if negative then it's fading to black.
	// in this case 0x12 should be positive and 0x14 should be 0
	s16 fade_step;

	// 0x18
	// this value is passed to SetGeomScreen,
	// used for perspective projection math
	// 256 in 1P, 128 in 4P
	int distanceToScreen_PREV;

	// 0x1c
	// position and dimensions
	RECT rect;

	// 0x24
	s16 aspectX;

	// 0x26
	s16 aspectY;

	// 0x28
	MATRIX matrix_ViewProj;

	// 0x48 (Warppad Lightning during Driver Warp effect)
	MATRIX matrix_CameraTranspose;

	// 0x68 (GTE_AudioLR_Inst, Vector_SpecLightSpin3D)
	MATRIX matrix_Camera;

	// 0x88 (built in PushBuffer_Init, never used)
	MATRIX matrix_Proj;

	// Frustum Planes
	// given to FUN_80042e50
	// 0xA8 - plane1
	// 0xB0 - plane2
	// 0xB8 - plane3
	// 0xC0 - plane4

	// 0xA8
	char frustumData[0x28];

	// 0xD0
	int RenderListJmpIndex[6];

	// 0xE8
	struct BoundingBox bbox;

	// 0xF4
	// NOTE(aalhendi): Retail RenderBucket_QueueDraw reuses this field as
	// PUSHBUFFER_EXISTS OT range-start metadata after DecalMP seeds ptrOT.
	u_long *ptrOT;

	// 0xF8
	// RenderBucket PUSHBUFFER_EXISTS range end metadata.
	u_long *renderBucketOTRangeEnd;

	// 0xFC
	int renderBucketOTByteOffset;

	// 0x100
	int renderBucketScreenPos;

	// 0x104
	int renderBucketScreenSize;

	// 0x108
	int cameraID;

	// 0x10c
	int distanceToScreen_CURR;

	// 0x110 - end of struct
};

_Static_assert(sizeof(struct PushBuffer) == 0x110);
_Static_assert(offsetof(struct PushBuffer, distanceToScreen_PREV) == 0x18);
_Static_assert(offsetof(struct PushBuffer, rect) == 0x1c);
_Static_assert(offsetof(RECT, w) == 0x4);
_Static_assert(offsetof(RECT, h) == 0x6);
_Static_assert(offsetof(struct PushBuffer, matrix_ViewProj) == 0x28);
_Static_assert(offsetof(struct PushBuffer, matrix_Camera) == 0x68);
_Static_assert(offsetof(struct PushBuffer, ptrOT) == 0xf4);
_Static_assert(offsetof(struct PushBuffer, renderBucketOTRangeEnd) == 0xf8);
_Static_assert(offsetof(struct PushBuffer, renderBucketOTByteOffset) == 0xfc);
_Static_assert(offsetof(struct PushBuffer, renderBucketScreenPos) == 0x100);
_Static_assert(offsetof(struct PushBuffer, renderBucketScreenSize) == 0x104);
