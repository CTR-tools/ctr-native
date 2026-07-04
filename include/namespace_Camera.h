#ifndef CTR_NATIVE_NAMESPACE_CAMERA_H
#define CTR_NATIVE_NAMESPACE_CAMERA_H

enum CameraPathFlags
{
	CAM_PATH_FLAG_CLOCK_EFFECT = 0x0001,
	CAM_PATH_FLAG_DISTANCE_TO_SCREEN_0x50 = 0x0002,
	CAM_PATH_FLAG_DISTANCE_TO_SCREEN_0x278 = 0x0004,
	CAM_PATH_FLAG_RANDOM_CLEAR_BOX = 0x0010,
	CAM_PATH_FLAG_DISTANCE_TO_SCREEN_0x1EB = 0x0020,
	CAM_PATH_FLAG_DISTANCE_TO_SCREEN_0x14D = 0x0040,
};

CTR_STATIC_ASSERT(CAM_PATH_FLAG_CLOCK_EFFECT == 0x0001);
CTR_STATIC_ASSERT(CAM_PATH_FLAG_DISTANCE_TO_SCREEN_0x50 == 0x0002);
CTR_STATIC_ASSERT(CAM_PATH_FLAG_DISTANCE_TO_SCREEN_0x278 == 0x0004);
CTR_STATIC_ASSERT(CAM_PATH_FLAG_RANDOM_CLEAR_BOX == 0x0010);
CTR_STATIC_ASSERT(CAM_PATH_FLAG_DISTANCE_TO_SCREEN_0x1EB == 0x0020);
CTR_STATIC_ASSERT(CAM_PATH_FLAG_DISTANCE_TO_SCREEN_0x14D == 0x0040);

// Camera scratchpad overlay rooted at retail scratchpad 0x1f800108.
// Camera-owned fields begin at work+0x20c, absolute scratchpad 0x1f800314.
struct CameraCollisionScratch
{
	union
	{
		u8 collisionScratch[0x20c];
		struct
		{
			u8 pad_000[0x1e];
			s16 terrainHeightFloor;
		};
	};
};

CTR_STATIC_ASSERT(offsetof(struct CameraCollisionScratch, terrainHeightFloor) == 0x01e);
CTR_STATIC_ASSERT(sizeof(struct CameraCollisionScratch) == 0x20c);

struct CameraAngleAxisScratchCamera
{
	SVec3 rot;     // +0x00 (abs 0x314)
	s16 _pad0;     // +0x06
	Vec3 posCopy;  // +0x08 (abs 0x31C) s32 copies of pos below
	MATRIX matrix; // +0x14 (abs 0x328)
	Vec3 pos;      // +0x34 (abs 0x348)
	Vec3 dir;      // +0x40 (abs 0x354) written by CAM_LookAtPosition
};

CTR_STATIC_ASSERT(offsetof(struct CameraAngleAxisScratchCamera, rot) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct CameraAngleAxisScratchCamera, posCopy) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct CameraAngleAxisScratchCamera, matrix) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct CameraAngleAxisScratchCamera, pos) == 0x34);
CTR_STATIC_ASSERT(offsetof(struct CameraAngleAxisScratchCamera, dir) == 0x40);
CTR_STATIC_ASSERT(sizeof(struct CameraAngleAxisScratchCamera) == 0x4c);

struct CameraScratch
{
	union
	{
		struct CameraAngleAxisScratchCamera angleAxis;
		struct
		{
			SVec3 rot;
			s16 _pad0;
			Vec3 posCopy;
			MATRIX matrix;
			Vec3 pos;
			Vec3 dir;
		};
	};

	Vec3 delta; // +0x4C (abs 0x360)
};

CTR_STATIC_ASSERT(offsetof(struct CameraScratch, angleAxis) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct CameraScratch, angleAxis.rot) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct CameraScratch, angleAxis.posCopy) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct CameraScratch, angleAxis.matrix) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct CameraScratch, angleAxis.pos) == 0x34);
CTR_STATIC_ASSERT(offsetof(struct CameraScratch, angleAxis.dir) == 0x40);
CTR_STATIC_ASSERT(offsetof(struct CameraScratch, delta) == 0x4C);
CTR_STATIC_ASSERT(sizeof(struct CameraScratch) == 0x58);

struct CameraAngleAxisScratch
{
	struct CameraCollisionScratch collision;
	struct CameraAngleAxisScratchCamera camera;
};

CTR_STATIC_ASSERT(offsetof(struct CameraAngleAxisScratch, collision) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CameraAngleAxisScratch, collision.terrainHeightFloor) == 0x01e);
CTR_STATIC_ASSERT(offsetof(struct CameraAngleAxisScratch, camera) == 0x20c);
CTR_STATIC_ASSERT(sizeof(struct CameraAngleAxisScratch) == 0x258);

struct CameraScratchWork
{
	union
	{
		struct CameraAngleAxisScratch angleAxis;
		struct
		{
			union
			{
				struct CameraCollisionScratch collision;
				u8 collisionScratch[0x20c];
				struct
				{
					u8 pad_000[0x1e];
					s16 terrainHeightFloor;
				};
			};

			struct CameraScratch camera;
		};
	};

	u8 pad_264[0x18];
	Vec3 sideOffset;
	SVec3 trackPathPos;
	s16 pad_28e;
	SVec3 trackPathLookaheadPos;
};

CTR_STATIC_ASSERT(offsetof(struct CameraScratchWork, collision) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CameraScratchWork, collision.terrainHeightFloor) == 0x01e);
CTR_STATIC_ASSERT(offsetof(struct CameraScratchWork, angleAxis) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CameraScratchWork, camera) == 0x20c);
CTR_STATIC_ASSERT(offsetof(struct CameraScratchWork, camera.angleAxis) == 0x20c);
CTR_STATIC_ASSERT(offsetof(struct CameraScratchWork, sideOffset) == 0x27c);
CTR_STATIC_ASSERT(offsetof(struct CameraScratchWork, trackPathPos) == 0x288);
CTR_STATIC_ASSERT(offsetof(struct CameraScratchWork, trackPathLookaheadPos) == 0x290);

struct ZoomData
{
	// get distance by mapping camera speeed from:
	// [speedMin, speedMax] to [distMin, distMax]

	// 0x0
	s16 distMin;
	s16 distMax;

	// 0x4
	s16 speedMin;
	s16 speedMax;

	// 0x8
	u8 percentage1;
	u8 percentage2;

	// 0xA
	s16 angle[3];

	// 0x10
	s16 vertDistance;
};

struct FlyInData
{
	u8 *ptrEnd;
	u8 *ptrStart;
	s16 frameCount1;
	s16 frameCount2;
};

struct CameraFireSpeedZoom
{
	// 0x0
	s32 distanceOffset;

	// 0x4
	s32 timer;
};

struct CameraHeightSmoothing
{
	// 0x0
	s16 startOffset;

	// 0x2
	s16 framesRemaining;

	// 0x4
	s16 currentOffset;
};

enum
{
	CAMERA_FLAG_RESET_RAIN_POS = 0x1,
	CAMERA_FLAG_BATTLE_END_OF_RACE = 0x4,
	CAMERA_FLAG_DIRECTION_CHANGED = 0x8,
	CAMERA_FLAG_MASK_GRAB = 0x10,
	CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED = 0x20,
	CAMERA_FLAG_TRACK_PATH_FACE_DRIVER = 0x40,
	CAMERA_FLAG_FIRE_SPEED_ZOOM = 0x80,
	CAMERA_FLAG_TRACK_PATH_ALT_BRANCH = 0x100,
	CAMERA_FLAG_TRANSITION_AWAY = 0x200,
	CAMERA_FLAG_TRANSITION_BACK = 0x400,
	CAMERA_FLAG_TRANSITION_HOLD = 0x800,
	CAMERA_FLAG_ARCADE_END_OF_RACE_ACTIVE = 0x1000,
	CAMERA_FLAG_FROZEN = 0x8000,
	CAMERA_FLAG_REVERSE = 0x10000,
};
typedef u32 CameraFlags;

enum CameraMode
{
	CAMERA_MODE_FREECAM = 3,
};

CTR_STATIC_ASSERT(CAMERA_MODE_FREECAM == 3);

struct CameraDC
{
	// 0x0
	int cameraID;

	// 0x4
	// action,
	// 0x20000 constantly swaps L2 zoom
	u32 action;

	// 0x08
	// camera mode, zoom out and such
	u16 mode;

	// 0x0A
	u16 nearOrFar;

	// 0xC
	u32 unk0xC;

	// 0x10
	// desired rotation (only X used, others reserved)
	SVec3 desiredRot;
	s16 _pad_desiredRot;

	// 0x18
	s16 unk18;
	// 0x1a
	s16 damagePitchOffset;

	// 0x1c - ptrQuadBlock
	// similar to driver +a0,
	// quadblock camera is currently above
	struct QuadBlock *ptrQuadBlock;

	// 0x20
	// VisMem->0x40[player], quadblock->0x44->0x0
	int *visLeafSrc;

	// 0x24
	// VisMem->0x50[player], quadblock->0x44->0x4
	int *visFaceSrc;

	// 0x28
	// quadblock->0x44->0x8
	// which instances are visible from quadblock
	struct Instance **visInstSrc;

	// 0x2c
	// VisMem->0x60[player]
	int *visOVertSrc;

	// 0x30
	// VisMem->0x70[player],
	int *visSCVertSrc;

	// 0x34
	char padding34[8];

	// 0x3c
	b32 quadBlockSearchHit;

	// 96b20+14c0

	// 0x40
	int cameraMoveSpeed;

	// 0x44
	struct Driver *driverToFollow;

	// 0x48
	struct PushBuffer *pushBuffer;

	// 0x4C
	Vec3 pushBufferPosCorrection;

	// 0x58
	Vec3 cameraPos;

	// 0x64
	Vec3 lookAtPos;

	// 0x70
	CameraFlags flags;

	// 0x74 (cam->0x9a is 8 or 0xe)
	SVec3 driverOffset_CamEyePos;
	s16 angleAxisLerpRatio;

	// 0x7c (cam->0x9a is 8 or 0xe)
	SVec3 driverOffset_CamLookAtPos;
	s16 unk82;

	// 0x84 - previous Driver.botData.botFlags
	u32 botFlagsPrevFrame;

	// 0x88
	struct CheckpointNode *trackPathNode;

	// 0x8C - Q12 blend: 0x0000 is transition camera, 0x1000 is driver camera
	s16 transitionBlend;

	// 0x8E - current frame/ramp for camera transitions
	s16 transitionFrame;

	// 0x90
	s16 spin360Angle;

	// 0x92 - near/far camera toggle state
	s16 zoomToggleState;

	// 0x94
	s32 trackPathProgress;

	// 0x98
	s16 maskGrabHeightOffset;

	// 0x9a - semi-unused camera mode swap
	s16 cameraMode; // Curr

	// 0x9C
	s16 cameraModePrev; // previous frame

	// 0x9e - transition duration in frames
	s16 transitionFrameCount;

	// 0xa0
	void *currEOR;

	// 0xa4
	// union shared between camera modes
	// Spin360 uses 0xa4 for spin speed
	struct
	{
		SVec3 pos;
		SVec3 rot;
	} transitionTo;

	// 0xb0 - union between multiple end-of-race camera modes
	union
	{
		s16 trackPathSpeed;

		struct
		{
			SVec3 endPos;
			s16 speed;
		} pointPath;
	} eorModeData;

	// 0xb8
	struct CameraFireSpeedZoom fireSpeedZoom;

	// 0xc0
	struct CameraHeightSmoothing heightSmoothing;

// Sep3
#if BUILD < UsaRetail

	// 0xc6
	s16 paddingC6;

#else // >= UsaRetail

	// Store data on first frame of BLASTED,
	// Use data on first frame of NOT BLASTED,
	// 8-frame lerp to bring camera back to kart
	struct
	{
		// 0xC6
		s16 boolLerpPending;

		// 0xc8
		s16 unkOffset[2];

		// 0xcc
		SVec3 desiredRot;
		s16 _pad_desiredRot;

		// 0xd4
		SVec3 desiredPos;

		// 0xda
		s16 framesRemaining;

	} BlastedLerp;

// 0xdc - end of struct
#endif

	// 0xC8 bytes large in sep3
	// 0xDC bytes large in usaRetail
};

CTR_STATIC_ASSERT(sizeof(struct ZoomData) == 0x12);
CTR_STATIC_ASSERT(sizeof(CameraFlags) == 0x4);
CTR_STATIC_ASSERT(CAMERA_FLAG_RESET_RAIN_POS == 0x1);
CTR_STATIC_ASSERT(CAMERA_FLAG_BATTLE_END_OF_RACE == 0x4);
CTR_STATIC_ASSERT(CAMERA_FLAG_DIRECTION_CHANGED == 0x8);
CTR_STATIC_ASSERT(CAMERA_FLAG_MASK_GRAB == 0x10);
CTR_STATIC_ASSERT(CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED == 0x20);
CTR_STATIC_ASSERT(CAMERA_FLAG_TRACK_PATH_FACE_DRIVER == 0x40);
CTR_STATIC_ASSERT(CAMERA_FLAG_FIRE_SPEED_ZOOM == 0x80);
CTR_STATIC_ASSERT(CAMERA_FLAG_TRACK_PATH_ALT_BRANCH == 0x100);
CTR_STATIC_ASSERT(CAMERA_FLAG_TRANSITION_AWAY == 0x200);
CTR_STATIC_ASSERT(CAMERA_FLAG_TRANSITION_BACK == 0x400);
CTR_STATIC_ASSERT(CAMERA_FLAG_TRANSITION_HOLD == 0x800);
CTR_STATIC_ASSERT(CAMERA_FLAG_ARCADE_END_OF_RACE_ACTIVE == 0x1000);
CTR_STATIC_ASSERT(CAMERA_FLAG_FROZEN == 0x8000);
CTR_STATIC_ASSERT(CAMERA_FLAG_REVERSE == 0x10000);
CTR_STATIC_ASSERT(sizeof(struct CameraFireSpeedZoom) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct CameraFireSpeedZoom, distanceOffset) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CameraFireSpeedZoom, timer) == 0x4);
CTR_STATIC_ASSERT(sizeof(struct CameraHeightSmoothing) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct CameraHeightSmoothing, startOffset) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CameraHeightSmoothing, framesRemaining) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct CameraHeightSmoothing, currentOffset) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, damagePitchOffset) == 0x1a);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, quadBlockSearchHit) == 0x3c);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, pushBufferPosCorrection) == 0x4c);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, cameraPos) == 0x58);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, lookAtPos) == 0x64);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, flags) == 0x70);
CTR_STATIC_ASSERT(sizeof(((struct CameraDC *)0)->flags) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, driverOffset_CamEyePos) == 0x74);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, angleAxisLerpRatio) == 0x7a);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, driverOffset_CamLookAtPos) == 0x7c);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, unk82) == 0x82);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, botFlagsPrevFrame) == 0x84);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, trackPathNode) == 0x88);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, transitionBlend) == 0x8c);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, transitionFrame) == 0x8e);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, spin360Angle) == 0x90);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, zoomToggleState) == 0x92);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, trackPathProgress) == 0x94);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, maskGrabHeightOffset) == 0x98);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, transitionFrameCount) == 0x9e);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, eorModeData) == 0xb0);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, eorModeData.trackPathSpeed) == 0xb0);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, eorModeData.pointPath.endPos) == 0xb0);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, eorModeData.pointPath.speed) == 0xb6);
CTR_STATIC_ASSERT(sizeof(((struct CameraDC *)0)->eorModeData) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, fireSpeedZoom) == 0xb8);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, heightSmoothing) == 0xc0);
#if BUILD >= UsaRetail
CTR_STATIC_ASSERT(sizeof(struct CameraDC) == 0xDC);
#else
CTR_STATIC_ASSERT(sizeof(struct CameraDC) == 0xC8);
#endif

#endif
