#ifndef CTR_NATIVE_NAMESPACE_PUSHBUFFER_H
#define CTR_NATIVE_NAMESPACE_PUSHBUFFER_H

struct FrustumCornerOUT
{
	SVec3 pos;
};

struct PushBufferFrustumPlane
{
	SVec3 normal;
	s16 halfDistance;
};

CTR_STATIC_ASSERT(sizeof(struct PushBufferFrustumPlane) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct PushBufferFrustumPlane, normal) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct PushBufferFrustumPlane, halfDistance) == 0x6);

struct ScratchpadFrustum
{
	// 1f800000
	Vec3 clippedFarPos;

	// 1f80000C
	struct FrustumCornerOUT fc[4];

	// 1f800024
	SVec3 camPos;

	// 1f80002A
	// -- end --
};

CTR_STATIC_ASSERT(sizeof(Vec3) == 0x0c);
CTR_STATIC_ASSERT(offsetof(struct ScratchpadFrustum, clippedFarPos) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct ScratchpadFrustum, fc) == 0x0c);
CTR_STATIC_ASSERT(offsetof(struct ScratchpadFrustum, camPos) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct ScratchpadFrustum, camPos) + sizeof(SVec3) == 0x2a);

struct PushBufferSetMatrixVPScratch
{
	u8 reserved0[0x3d4];

	MATRIX cameraMatrix;
	SVec3 rot;
	u8 reserved1[0x6];
};

CTR_STATIC_ASSERT(offsetof(struct PushBufferSetMatrixVPScratch, cameraMatrix) == 0x3d4);
CTR_STATIC_ASSERT(offsetof(struct PushBufferSetMatrixVPScratch, rot) == 0x3f4);
CTR_STATIC_ASSERT(sizeof(struct PushBufferSetMatrixVPScratch) == CTR_SCRATCHPAD_SIZE);

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
	struct PushBufferFrustumPlane frustumPlanes[5];

	// 0xD0
	int RenderListJmpIndex[6];

	// 0xE8
	struct BoundingBox bbox;

	// 0xF4
	// NOTE(aalhendi): Retail RenderBucket_QueueDraw reuses this field as
	// PUSHBUFFER_EXISTS OT range-start metadata after DecalMP seeds ptrOT.
	uint32_t *ptrOT;

	// 0xF8
	// RenderBucket PUSHBUFFER_EXISTS range end metadata.
	uint32_t *renderBucketOTRangeEnd;

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

CTR_STATIC_ASSERT(sizeof(struct PushBuffer) == 0x110);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, distanceToScreen_PREV) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, rect) == 0x1c);
CTR_STATIC_ASSERT(offsetof(RECT, w) == 0x4);
CTR_STATIC_ASSERT(offsetof(RECT, h) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_ViewProj) == 0x28);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_ViewProj.m[0][0]) == 0x28);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_ViewProj.m[0][2]) == 0x2c);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_ViewProj.m[1][1]) == 0x30);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_ViewProj.m[2][0]) == 0x34);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_ViewProj.m[2][2]) == 0x38);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_Camera) == 0x68);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_Camera.t[0]) == 0x7c);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_Camera.t[1]) == 0x80);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, matrix_Camera.t[2]) == 0x84);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, frustumPlanes) == 0xa8);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, RenderListJmpIndex) == 0xd0);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, bbox) == 0xe8);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, ptrOT) == 0xf4);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, renderBucketOTRangeEnd) == 0xf8);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, renderBucketOTByteOffset) == 0xfc);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, renderBucketScreenPos) == 0x100);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, renderBucketScreenSize) == 0x104);

#endif
