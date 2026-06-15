#include <common.h>

struct CAMSkyboxGlowGradient
{
	s16 pointFrom;
	s16 pointTo;
	u32 colorFrom;
	u32 colorTo;
};

enum
{
	CAM_FOLLOW_DRIVER_QUAD_FLAGS_SKIP_TERRAIN_HEIGHT = 0x4100,
};

_Static_assert(CAM_FOLLOW_DRIVER_QUAD_FLAGS_SKIP_TERRAIN_HEIGHT == 0x4100);

static u32 CAM_SkyboxGlow_PrimAddr(const void *prim)
{
	return CtrGpu_PrimToOTLink24(prim);
}

static u32 CAM_SkyboxGlow_PackXY(int x, int y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

static u32 CAM_SkyboxGlow_LerpColor(u32 from, u32 to, int ratio)
{
	int r = (u8)from + ((((int)(u8)to - (int)(u8)from) * ratio) >> 12);
	int g = (u8)(from >> 8) + ((((int)(u8)(to >> 8) - (int)(u8)(from >> 8)) * ratio) >> 12);
	int b = (u8)(from >> 16) + ((((int)(u8)(to >> 16) - (int)(u8)(from >> 16)) * ratio) >> 12);

	return ((u32)(u8)r) | ((u32)(u8)g << 8) | ((u32)(u8)b << 16);
}

static int CAM_SkyboxGlow_FixedRatio(int numerator, int denominator)
{
	return (numerator << 12) / denominator;
}

static int CAM_SkyboxGlow_ScreenX(int screenWidth, int ratio)
{
	return (screenWidth * ratio) >> 12;
}

static int CAM_SkyboxGlow_Div2TowardZero(int value)
{
	return (value + ((u32)value >> 31)) >> 1;
}

static int CAM_SkyboxGlow_CalcCenterY(struct PushBuffer *pb)
{
	int pitch = (pb->rot[0] - 0x800) * 0x78;
	int height = (s16)pb->rect.h;

	if (pitch < 0)
		pitch += 0x3ff;

	return (pitch >> 10) + CAM_SkyboxGlow_Div2TowardZero(height);
}

static int CAM_SkyboxGlow_CalcTilt(struct PushBuffer *pb)
{
	int sine = MATH_Sin(pb->rot[2]);
	int cosine = MATH_Cos(pb->rot[2]);
	int ratio;
	int shifted;

	if (cosine == 0)
		cosine = 1;

	ratio = (sine << 12) / cosine;
	shifted = (s32)((u32)ratio << 8);
	shifted >>= 12;

	return CAM_SkyboxGlow_Div2TowardZero(-shifted);
}

static void CAM_SkyboxGlow_EmitG3(struct PrimMem *primMem, u_long *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2)
{
	u32 *prim = primMem->curr;

	prim[1] = (color0 & 0xffffff) | 0x30000000;
	prim[2] = xy0;
	prim[3] = color1 & 0xffffff;
	prim[4] = xy1;
	prim[5] = color2 & 0xffffff;
	prim[6] = xy2;
	prim[0] = *ot | 0x06000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 7;
}

static void CAM_SkyboxGlow_EmitG4(struct PrimMem *primMem, u_long *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2, u32 color3, u32 xy3)
{
	u32 *prim = primMem->curr;

	prim[1] = (color0 & 0xffffff) | 0x38000000;
	prim[2] = xy0;
	prim[3] = color1 & 0xffffff;
	prim[4] = xy1;
	prim[5] = color2 & 0xffffff;
	prim[6] = xy2;
	prim[7] = color3 & 0xffffff;
	prim[8] = xy3;
	prim[0] = *ot | 0x08000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 9;
}

static void CAM_SkyboxGlow_EmitF3(struct PrimMem *primMem, u_long *ot, u32 color, u32 xy0, u32 xy1, u32 xy2)
{
	u32 *prim = primMem->curr;

	prim[1] = (color & 0xffffff) | 0x20000000;
	prim[2] = xy0;
	prim[3] = xy1;
	prim[4] = xy2;
	prim[0] = *ot | 0x04000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 5;
}

static void CAM_SkyboxGlow_EmitF4(struct PrimMem *primMem, u_long *ot, u32 color, u32 xy0, u32 xy1, u32 xy2, u32 xy3)
{
	u32 *prim = primMem->curr;

	prim[1] = (color & 0xffffff) | 0x28000000;
	prim[2] = xy0;
	prim[3] = xy1;
	prim[4] = xy2;
	prim[5] = xy3;
	prim[0] = *ot | 0x05000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 6;
}

static int CAM_SkyboxGlow_HasClearGradient(int gradientIndex)
{
	return gradientIndex == 0 && sdata->gGT->level1->clearColor[2].enable != 0;
}

static u32 CAM_SkyboxGlow_ClearGradientColor(void)
{
	return *(u32 *)&sdata->gGT->level1->clearColor[2].rgb[0] & 0xffffff;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800175cc-0x8001861c
void CAM_SkyboxGlow(s16 *param_1, struct PushBuffer *pb, struct PrimMem *primMem, u_long *ptrOT)
{
	struct CAMSkyboxGlowGradient *grad = (struct CAMSkyboxGlowGradient *)param_1;
	int tilt = CAM_SkyboxGlow_CalcTilt(pb);
	int centerY1 = CAM_SkyboxGlow_CalcCenterY(pb);
	int centerY2 = CAM_SkyboxGlow_CalcCenterY(pb);
	int screenWidth = (s16)pb->rect.w;
	u32 screenWidthXY = (u16)screenWidth;

	for (int i = 0; i < 3; i++, grad++)
	{
		int toLeft = centerY2 - grad->pointTo - tilt;
		int toRight = centerY2 - grad->pointTo + tilt;
		int fromLeft = centerY1 - grad->pointFrom - tilt;
		int fromRight = centerY1 - grad->pointFrom + tilt;
		u32 colorFrom = grad->colorFrom & 0xffffff;
		u32 colorTo = grad->colorTo & 0xffffff;
		u32 mask = 0;

		if (toLeft >= 0)
			mask |= 1;
		if (toRight >= 0)
			mask |= 2;
		if (fromLeft >= 0)
			mask |= 4;
		if (fromRight >= 0)
			mask |= 8;

		switch (mask)
		{
		case 1:
		{
			int leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			int rightRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 rightX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, rightRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, leftColor, 0, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, rightX);
			break;
		}
		case 2:
		{
			int rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			int leftRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);
			u32 leftX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, leftRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, colorTo, leftX, rightColor, screenWidthXY, colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));
			break;
		}
		case 3:
		{
			int leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			int rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, leftColor, 0, rightColor, screenWidthXY, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo,
			                      CAM_SkyboxGlow_PackXY(screenWidth, toRight));
			break;
		}
		case 5:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int toRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 toX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, toRatio);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromX, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft),
			                      colorTo, toX);

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, 0, CAM_SkyboxGlow_PackXY(0, fromLeft));
			break;
		}
		case 7:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromX, rightColor, screenWidthXY);
			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), rightColor, screenWidthXY, colorTo,
			                      CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, 0, CAM_SkyboxGlow_PackXY(0, fromLeft));
			break;
		}
		case 10:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int toRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 toX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, toRatio);
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, fromX, colorFrom, fromRightXY, colorTo, toX, colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, screenWidthXY, fromRightXY);
			break;
		}
		case 11:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, leftColor, 0, colorFrom, fromX, colorFrom, fromRightXY);
			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, leftColor, 0, colorFrom, fromRightXY, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo,
			                      CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, screenWidthXY, fromRightXY);
			break;
		}
		case 15:
		{
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromRightXY, colorTo,
			                      CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF4(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), 0, screenWidthXY, CAM_SkyboxGlow_PackXY(0, fromLeft), fromRightXY);
			break;
		}
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001861c-0x80018818
void CAM_ClearScreen(struct GameTracker *gGT)
{
	char numPlyr;
	char swap;
	s16 x;
	s16 y;
	s16 w;
	s16 h;
	struct DB *backDB;
	int iVar5;
	int iVar7;
	struct PushBuffer *pb;
	TILE *tile;
	int loop;
	struct Level *level1;
	u_long *endOT;

	backDB = gGT->backBuffer;
	tile = backDB->primMem.curr;
	numPlyr = gGT->numPlyrCurrGame;
	swap = gGT->swapchainIndex;
	level1 = gGT->level1;

	for (loop = 0; loop < numPlyr; loop++)
	{
		// pointer to pushBuffer struct
		pb = &gGT->pushBuffer[loop];
		endOT = &pb->ptrOT[0x3FF];

		x = pb->rect.x;
		y = pb->rect.y + swap * 0x128;
		w = pb->rect.w;
		h = pb->rect.h;

		// pushBuffer rotation
		// cam up/down will change where the line splits.
		// At 0x800, camera looks straight, and line is perfectly midpoint
		iVar5 = ((int)pb->rot[0] - 0x800 >> 3) + (h >> 1);

		// if splitline is above top of screen,
		// camera looks far down and only sees bottom half
		if (iVar5 < 0)
		{
			// top half has zero height
			iVar5 = 0;
		}

		// if splitline is below bottom of the screen,
		// top quad height is window sizeY, bottom quad is zero height
		iVar7 = h;

		// if splitline is below top of screen
		if (iVar5 < h)
		{
			// set midpoint accordingly
			iVar7 = iVar5;
		}

		// if top-half clear color exists,
		// and if splitline is below top of screen (so top quad exists)
		if ((level1->clearColor[0].enable != 0) && (0 < iVar7))
		{
			tile->x0 = x;
			tile->y0 = y;
			tile->w = w;
			tile->h = iVar7;

			*(int *)&tile->r0 = *(int *)&level1->clearColor[0].rgb[0];
			tile->code = 0x2;

			tile->tag = *(u32 *)endOT | 0x3000000;
			*(u32 *)endOT = CtrGpu_PrimToOTLink24(tile);

			tile++;
		}

		// if bottom-half clear color exists,
		// and if splitline is above bottom of screen (so bottom quad exists)
		if ((level1->clearColor[1].enable != 0) && (iVar7 < h))
		{
			tile->x0 = x;
			tile->y0 = y + iVar7;
			tile->w = w;
			tile->h = h - iVar7;

			*(int *)&tile->r0 = *(int *)&level1->clearColor[1].rgb[0];
			tile->code = 0x2;

			tile->tag = *(u32 *)endOT | 0x3000000;
			*(u32 *)endOT = CtrGpu_PrimToOTLink24(tile);

			tile++;
		}
	}

	backDB->primMem.curr = tile;

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018818-0x800188a8
void CAM_Init(struct CameraDC *cDC, int cameraID, struct Driver *d, struct PushBuffer *pb)
{
// Naughty Dog debug printf
#if BUILD == SepReview
	printf("camera init\n");
#endif

	PROC_BirthWithObject(0x30f, CAM_ThTick, sdata->s_camera, NULL)->inst = (struct Instance *)cDC;

	memset(cDC, 0, sizeof(struct CameraDC));

	// needed or L2 breaks
	cDC->cameraID = cameraID;

	cDC->driverToFollow = d;
	cDC->pushBuffer = pb;

	// dont set cameraMode to zero,
	// memset makes it already zero

	cDC->flags |= CAMERA_FLAG_DIRECTION_CHANGED;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018b18-0x80018ba0
int CAM_Path_GetNumPoints(void)
{
	struct GameTracker *gGT;
	struct Level *level1;
	struct SpawnType1 *ptrSpawnType1;
	s16 *introCam;
	u16 uVar4;

	uVar4 = 0;

	gGT = sdata->gGT;
	level1 = gGT->level1;
	if (level1 == NULL)
		return 0;

	ptrSpawnType1 = level1->ptrSpawnType1;
	if (ptrSpawnType1->count < 3)
		return 0;

	void **ptrs = ST1_GETPOINTERS(ptrSpawnType1);
	introCam = ptrs[ST1_CAMERA_PATH];
	if (introCam == NULL)
		return 0;

	while (1)
	{
		if (introCam[0] == 0)
			break;
		uVar4 += introCam[0];
		introCam += introCam[0] * 6 + 2;
	}

	return (s16)uVar4;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018ba0-0x80018d20
u8 CAM_Path_Move(int frameIndex, s16 *position, s16 *rotation, s16 *getPath)
{
	s16 frame;
	s16 numPos;
	u16 pathNumNode;
	u16 pathID;
	s16 *ptrCam;
	s16 *move;

	frame = (s16)frameIndex;

	// get number of position on track
	numPos = CAM_Path_GetNumPoints();

	if (frame < 0)
		return 0;
	if (frame >= numPos)
		return 0;

	void **ptrs = ST1_GETPOINTERS(sdata->gGT->level1->ptrSpawnType1);
	ptrCam = ptrs[ST1_CAMERA_PATH];

	pathNumNode = (u16)ptrCam[0];
	pathID = (u16)ptrCam[1];
	move = ptrCam + 2;

	while ((s16)pathNumNode <= frame)
	{
		frame = (s16)(frame - (s16)pathNumNode);
		move = move + (s16)pathNumNode * 6;
		pathNumNode = (u16)move[0];
		pathID = (u16)move[1];
		move = move + 2;
	}

	// advance pointer to pos+rot
	move += (int)frame * 6;

	*getPath = pathID;

	// position of frame
	position[0] = move[0];
	position[1] = move[1];
	position[2] = move[2];

	// rotation of frame
	rotation[0] = ((s16)move[3] >> 4) + 0x800U & 0xfff;
	rotation[1] = (u16)move[4] >> 4;
	rotation[2] = (u16)move[5] >> 4;
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018d20-0x80018d9c
void CAM_StartOfRace(struct CameraDC *cDC)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level1 = gGT->level1;

	// if fly-in camera data exists and there is only one screen
	int hasFlyInCamera = (2 < level1->cnt_restart_points);

	if (hasFlyInCamera)
	{
		int flyInData = (int)level1->ptr_restart_points;
		cDC->trackPathProgress = 0;
		cDC->transitionBlend = 0;

		// make transition to driver last one second
		cDC->transitionFrameCount = 0x1E;
		cDC->desiredRot[6] = 0;

		// when camera reaches player, be zoomed in
		cDC->cameraMode = 0;
		cDC->trackPathNode = (struct CheckpointNode *)(flyInData + 0x18);

		// if 1 or less screens
		// set fly-in to last 165 frames, (5.25 seconds)
		cDC->transitionFrame = 0xA5;
		if (gGT->numPlyrCurrGame > 1)
		{
			// set animation to last one frame
			cDC->transitionFrame = 1;
		}
	}

	cDC->cameraMode = 0;

	cDC->flags &= ~(CAMERA_FLAG_BATTLE_END_OF_RACE | CAMERA_FLAG_ARCADE_END_OF_RACE_ACTIVE);
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018d9c-0x80018e38.
void CAM_EndOfRace_Battle(struct CameraDC *cDC, struct Driver *d)
{
	int dx;
	int dz;
	int height;
	struct PushBuffer *pb;

	// This is NOT TransitionTo,
	// spin360's spinSpeed (missing union)
	// and spinHeight, spinDistance, etc
	height = data.Spin360_heightOffset_cameraPos[sdata->gGT->numPlyrCurrGame];
	cDC->transitionTo.pos[0] = 0xffe5;
	cDC->transitionTo.pos[1] = height;
	cDC->transitionTo.pos[2] = 0xc0;

	// use camera that spins around player
	cDC->flags |= CAMERA_FLAG_BATTLE_END_OF_RACE;

	// transition should take two seconds
	cDC->transitionBlend = 0;
	cDC->transitionFrame = 60;
	cDC->transitionFrameCount = 60;

	// direction to face
	pb = cDC->pushBuffer;
	dx = CTR_MipsSubLo(pb->pos[0], CTR_MipsSra(d->posCurr.x, 8));
	dz = CTR_MipsSubLo(pb->pos[2], CTR_MipsSra(d->posCurr.z, 8));
	cDC->spin360Angle = ratan2(dx, dz);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018e38-0x80018ec0.
void CAM_EndOfRace(struct CameraDC *cDC, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;

#if BUILD > SepReview

	// If not in Battle Mode and track path points exist and game is on 1P or 2P mode
	if (((gGT->gameMode1 & BATTLE_MODE) == 0) && (1 < gGT->level1->ptrSpawnType1->count) && (gGT->numPlyrCurrGame < 3))
	{
		// Activate end-of-race cDC flag in CameraDC struct
		cDC->flags |= CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED;
	}
	else
	{
		// Call function to initialize end-of-race cDC for Battle Mode
		CAM_EndOfRace_Battle(cDC, d);
	}
	return;

#else
	// this is SepReview's more error-prone version of the function

	if (gGT->level1->ptrSpawnType1->count < 2 || gGT->numPlyrCurrGame > 2)
		CAM_EndOfRace_Battle(cDC, d);
	else
		cDC->flags |= CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED;

#endif
}

static s32 CAM_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018fec-0x80019128
void CAM_ProcessTransition(s16 *currPos, s16 *currRot, s16 *startPos, s16 *startRot, s16 *endPos, s16 *endRot, int frame)
{
	int deltaRot;

	currPos[0] = startPos[0] + (s16)(CAM_MulLo((int)endPos[0] - (int)startPos[0], frame) >> 0xc);
	currPos[1] = startPos[1] + (s16)(CAM_MulLo((int)endPos[1] - (int)startPos[1], frame) >> 0xc);
	currPos[2] = startPos[2] + (s16)(CAM_MulLo((int)endPos[2] - (int)startPos[2], frame) >> 0xc);

	deltaRot = (int)endRot[0] - (int)startRot[0] & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}

	currRot[0] = startRot[0] + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc) & 0xfff;
	deltaRot = (int)endRot[1] - (int)startRot[1] & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot[1] = startRot[1] + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc) & 0xfff;
	deltaRot = (int)endRot[2] - (int)startRot[2] & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot[2] = startRot[2] + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc) & 0xfff;
	return;
}

_Static_assert(sizeof(struct QuadBlock) == 0x5c);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800188a8-0x80018b18
void CAM_FindClosestQuadblock(struct ScratchpadStruct *sps, struct CameraDC *cDC, struct Driver *d, const VECTOR *pos)
{
	struct GameTracker *gGT;
	struct mesh_info *meshInfo;
	struct QuadBlock *quad;

	(void)d;

	// NOTE(aalhendi): Retail consumes the low halfword of each VECTOR lane.
	s16 posX = (s16)pos->vx;
	s16 posY = (s16)pos->vy;
	s16 posZ = (s16)pos->vz;

	sps->Union.QuadBlockColl.pos.x = posX;
	sps->Union.QuadBlockColl.pos.y = posY;
	sps->Union.QuadBlockColl.pos.z = posZ;

	sps->Input1.pos.x = posX;
	sps->Input1.pos.y = (s16)CTR_MipsSubLo((u16)posY, 0x800);
	sps->Union.QuadBlockColl.hitPos.x = sps->Input1.pos.x;
	sps->Input1.pos.z = posZ;
	sps->Union.QuadBlockColl.pos.y = (s16)CTR_MipsAddLo((u16)sps->Union.QuadBlockColl.pos.y, 0x100);
	sps->Union.QuadBlockColl.hitPos.y = sps->Input1.pos.y;
	sps->Union.QuadBlockColl.hitPos.z = posZ;

	sps->bbox.min.x = sps->Input1.pos.x < sps->Union.QuadBlockColl.pos.x ? sps->Input1.pos.x : sps->Union.QuadBlockColl.pos.x;
	sps->bbox.min.y = sps->Input1.pos.y < sps->Union.QuadBlockColl.pos.y ? sps->Input1.pos.y : sps->Union.QuadBlockColl.pos.y;
	sps->bbox.min.z = sps->Input1.pos.z < sps->Union.QuadBlockColl.pos.z ? sps->Input1.pos.z : sps->Union.QuadBlockColl.pos.z;
	sps->bbox.max.x = sps->Union.QuadBlockColl.pos.x < sps->Input1.pos.x ? sps->Input1.pos.x : sps->Union.QuadBlockColl.pos.x;
	sps->bbox.max.y = sps->Union.QuadBlockColl.pos.y < sps->Input1.pos.y ? sps->Input1.pos.y : sps->Union.QuadBlockColl.pos.y;
	sps->bbox.max.z = sps->Union.QuadBlockColl.pos.z < sps->Input1.pos.z ? sps->Input1.pos.z : sps->Union.QuadBlockColl.pos.z;

	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_CAMERA_SEARCH;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->boolDidTouchQuadblock = 0;
	sps->numTrianglesTested = 0;
	sps->Union.QuadBlockColl.searchFlags = 0;
	cDC->quadBlockSearchHit = false;

	gGT = sdata->gGT;

	if ((gGT->level1 == NULL) || (gGT->level1->ptr_mesh_info == NULL) || (gGT->level1->ptr_mesh_info->bspRoot == NULL))
	{
		sps->ptr_mesh_info = NULL;
		return;
	}

	meshInfo = gGT->level1->ptr_mesh_info;
	sps->ptr_mesh_info = meshInfo;

	if (cDC->ptrQuadBlock != NULL)
	{
		COLL_FIXED_QUADBLK_TestTriangles(cDC->ptrQuadBlock, sps);
	}

	if (sps->boolDidTouchQuadblock == 0)
	{
		COLL_SearchBSP_CallbackPARAM(meshInfo->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
	}

	if (sps->boolDidTouchQuadblock == 0)
	{
		gGT->unk1cac[0] = -1;
		return;
	}

	cDC->quadBlockSearchHit = true;

	quad = sps->hit.ptrQuadblock;
	cDC->ptrQuadBlock = quad;
	gGT->unk1cac[0] = quad - meshInfo->ptrQuadBlockArray;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018ec0-0x80018fec.
void CAM_StartLine_FlyIn_FixY(s16 *posRot)
{
	struct ScratchpadStruct *sps = &sdata->scratchpadStruct;
	s16 pos[3];
	int i;

	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;

	pos[0] = posRot[0];
	pos[1] = posRot[1];
	pos[2] = posRot[2];

	for (i = 0; i < 8; i++)
	{
		s32 probeOffset = i * 0x400;

		SVec3 posTop = {
		    .x = pos[0],
		    .y = (s16)CTR_MipsSubLo((u16)pos[1], CTR_MipsAddLo(probeOffset, 0x400)),
		    .z = pos[2],
		};
		SVec3 posBottom = {
		    .x = pos[0],
		    .y = (s16)CTR_MipsSubLo((u16)pos[1], CTR_MipsSubLo(probeOffset, 0x100)),
		    .z = pos[2],
		};

		COLL_SearchBSP_CallbackQUADBLK(&posTop, &posBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			CTR_COPY_VEC3(pos, sps->Union.QuadBlockColl.hitPos.v);
			break;
		}
	}

	posRot[1] = pos[1];
}

static s32 CAM_FollowDriver_AngleAxis_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

static s32 CAM_FollowDriver_AngleAxis_Lerp256(s32 current, s32 previous, s32 ratio)
{
	return (CAM_FollowDriver_AngleAxis_MulLo(0x100 - ratio, current) + CAM_FollowDriver_AngleAxis_MulLo(ratio, previous)) >> 8;
}

static void CAM_FollowDriver_AngleAxis_LoadGteMatrix(MATRIX *axisMatrix, struct Driver *d)
{
	gte_SetRotMatrix(axisMatrix);
	gte_SetTransVector(d->instSelf->matrix.t);
}

static void CAM_FollowDriver_AngleAxis_TransformOffset(const SVec3 *offset, VECTOR *out)
{
	gte_ldv0((SVECTOR *)offset);
	gte_rtv0tr();
	gte_stlvnl(out);
}

void CAM_FollowDriver_AngleAxis(struct CameraDC *cDC, struct Driver *d, u8 *workBuffer, s16 *pushBufferPos, s16 *pushBufferRot)
{
	MATRIX *axisMatrix = (MATRIX *)(void *)(workBuffer + 0x220);
	VECTOR *eye = (VECTOR *)(void *)(workBuffer + 0x240);
	VECTOR lookAt;
	int ratio;
	int dx;
	int dy;
	int dz;
	int distanceXZ;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019128-0x800194c8.
	if (cDC->cameraMode == 0xe)
		VehPhysForce_RotAxisAngle(axisMatrix, d->AxisAngle2_normalVec.v, d->angle);
	else
		VehPhysForce_RotAxisAngle(axisMatrix, d->AxisAngle2_normalVec.v, d->rotCurr.y);

	CAM_FollowDriver_AngleAxis_LoadGteMatrix(axisMatrix, d);
	CAM_FollowDriver_AngleAxis_TransformOffset(&cDC->driverOffset_CamEyePos, eye);
	CAM_FollowDriver_AngleAxis_TransformOffset(&cDC->driverOffset_CamLookAtPos, &lookAt);

	if ((cDC->flags & CAMERA_FLAG_DIRECTION_CHANGED) != 0)
	{
		cDC->lookAtPos.x = lookAt.vx;
		cDC->lookAtPos.y = lookAt.vy;
		cDC->lookAtPos.z = lookAt.vz;
	}
	else
	{
		ratio = cDC->angleAxisLerpRatio;

		eye->vx = CAM_FollowDriver_AngleAxis_Lerp256(eye->vx, pushBufferPos[0], ratio);
		eye->vy = CAM_FollowDriver_AngleAxis_Lerp256(eye->vy, pushBufferPos[1], ratio);
		eye->vz = CAM_FollowDriver_AngleAxis_Lerp256(eye->vz, pushBufferPos[2], ratio);

		cDC->lookAtPos.x = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.vx, cDC->lookAtPos.x, ratio);
		cDC->lookAtPos.y = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.vy, cDC->lookAtPos.y, ratio);
		cDC->lookAtPos.z = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.vz, cDC->lookAtPos.z, ratio);
	}

	dx = eye->vx - cDC->lookAtPos.x;
	dy = eye->vy - cDC->lookAtPos.y;
	dz = eye->vz - cDC->lookAtPos.z;

	*(int *)(workBuffer + 0x24c) = dx;
	*(int *)(workBuffer + 0x250) = dy;
	*(int *)(workBuffer + 0x254) = dz;

	pushBufferRot[1] = (s16)ratan2(dx, dz);
	distanceXZ = SquareRoot0_stub(CAM_FollowDriver_AngleAxis_MulLo(dx, dx) + CAM_FollowDriver_AngleAxis_MulLo(dz, dz));
	pushBufferRot[0] = 0x800 - (s16)ratan2(dy, distanceXZ);
	pushBufferRot[2] = 0;

	pushBufferPos[0] = (s16)eye->vx;
	pushBufferPos[1] = (s16)eye->vy;
	pushBufferPos[2] = (s16)eye->vz;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800194c8-0x800198f8.
void CAM_StartLine_FlyIn(struct FlyInData *flyInData, s16 maxFrames, int frame, s16 *desiredPos, s16 *desiredRot)
{
	struct Level *lev = sdata->gGT->level1;
	int frameIndex = (frame << 0x10) >> 4;
	int frameRatio = frameIndex / maxFrames;
	int countEnd = flyInData->frameCount1;
	s16 count = flyInData->frameCount2;
	SVECTOR local_78;
	SVECTOR local_70;
	SVECTOR rot;
	MATRIX matrix;
	VECTOR transformed;
	s32 flags[2];
	s16 *pathEnd;
	s16 *pathStart;
	int pathRatioEnd;

	if (count < countEnd)
		count = countEnd;

	s16 pathIndex = (s16)(count * frameRatio >> 0xc);
	if (pathIndex < countEnd - 1)
	{
		pathEnd = (s16 *)(flyInData->ptrEnd + pathIndex * 6);
		pathRatioEnd = frameRatio;
	}
	else
	{
		pathEnd = (s16 *)(flyInData->ptrEnd + countEnd * 6 - 0xc);
		pathRatioEnd = 0;
	}

	if (pathIndex < flyInData->frameCount2 - 1)
	{
		pathStart = (s16 *)(flyInData->ptrStart + pathIndex * 6);
	}
	else
	{
		pathStart = (s16 *)(flyInData->ptrStart + flyInData->frameCount2 * 6 - 0xc);
		frameRatio = 0;
	}

	int ratio = count * pathRatioEnd & 0xfff;
	local_78.vx = pathEnd[0] + (s16)(((pathEnd[3] - pathEnd[0]) * ratio) >> 0xc);
	local_78.vy = pathEnd[1] + (s16)(((pathEnd[4] - pathEnd[1]) * ratio) >> 0xc);
	local_78.vz = pathEnd[2] + (s16)(((pathEnd[5] - pathEnd[2]) * ratio) >> 0xc);

	ratio = count * frameRatio & 0xfff;
	local_70.vx = pathStart[0] + (s16)(((pathStart[3] - pathStart[0]) * ratio) >> 0xc);
	local_70.vy = pathStart[1] + (s16)(((pathStart[4] - pathStart[1]) * ratio) >> 0xc) - 0x60;
	local_70.vz = pathStart[2] + (s16)(((pathStart[5] - pathStart[2]) * ratio) >> 0xc);

	rot.vx = lev->DriverSpawn[0].rot[0];
	rot.vy = lev->DriverSpawn[0].rot[1] + 0x400;
	rot.vz = lev->DriverSpawn[0].rot[2];

	ConvertRotToMatrix(&matrix, (s16 *)&rot);

	CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[1].pos[0]);
	CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[2].pos[0]);
	CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[5].pos[0]);

	matrix.t[0] = lev->DriverSpawn[1].pos[0] + (lev->DriverSpawn[2].pos[0] - lev->DriverSpawn[1].pos[0]) / 2;
	matrix.t[1] = lev->DriverSpawn[1].pos[1] + (lev->DriverSpawn[2].pos[1] - lev->DriverSpawn[1].pos[1]) / 2 + 0x40;
	matrix.t[2] = lev->DriverSpawn[1].pos[2] + (lev->DriverSpawn[2].pos[2] - lev->DriverSpawn[1].pos[2]) / 2;

	SetRotMatrix(&matrix);
	SetTransMatrix(&matrix);

	RotTrans(&local_78, &transformed, (long *)flags);
	desiredPos[0] = (s16)transformed.vx;
	desiredPos[1] = (s16)transformed.vy;
	desiredPos[2] = (s16)transformed.vz;

	RotTrans(&local_70, &transformed, (long *)flags);

	s16 deltaX = desiredPos[0] - (s16)transformed.vx;
	s16 deltaY = desiredPos[1] - (s16)transformed.vy;
	s16 deltaZ = desiredPos[2] - (s16)transformed.vz;

	desiredRot[1] = (s16)ratan2(deltaX, deltaZ);
	desiredRot[0] = 0x800 - (s16)ratan2(deltaY, SquareRoot0(deltaX * deltaX + deltaZ * deltaZ));
	desiredRot[2] = 0;
}

static s32 CAM_FollowDriver_TrackPath_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

static struct CheckpointNode *CAM_FollowDriver_TrackPath_GetNode(struct CameraDC *cDC, struct CheckpointNode *node, int speed)
{
	struct CheckpointNode *nodes = sdata->gGT->level1->ptr_restart_points;
	u8 nodeIndex;

	if (speed > 0)
	{
		nodeIndex = node->nextIndex_backward;
		if ((cDC->flags & CAMERA_FLAG_TRACK_PATH_ALT_BRANCH) != 0 && node->nextIndex_right != 0xff)
			nodeIndex = node->nextIndex_right;
	}
	else
	{
		nodeIndex = node->nextIndex_forward;
		if ((cDC->flags & CAMERA_FLAG_TRACK_PATH_ALT_BRANCH) != 0 && node->nextIndex_left != 0xff)
			nodeIndex = node->nextIndex_left;
	}

	return &nodes[nodeIndex];
}

static int CAM_FollowDriver_TrackPath_Length(struct CheckpointNode *from, struct CheckpointNode *to, int *dx, int *dy, int *dz)
{
	*dx = to->pos[0] - from->pos[0];
	*dy = to->pos[1] - from->pos[1];
	*dz = to->pos[2] - from->pos[2];

	return SquareRoot0_stub(CAM_FollowDriver_TrackPath_MulLo(*dx, *dx) + CAM_FollowDriver_TrackPath_MulLo(*dy, *dy) +
	                        CAM_FollowDriver_TrackPath_MulLo(*dz, *dz));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800198f8-0x80019e7c.
u32 CAM_FollowDriver_TrackPath(struct CameraDC *cDC, s16 *pos, int speed, int update)
{
	struct CheckpointNode *curr = cDC->trackPathNode;
	struct CheckpointNode *next = CAM_FollowDriver_TrackPath_GetNode(cDC, curr, speed);
	struct CheckpointNode *angleNext;
	int pathProgress;
	int segmentLength;
	int ratio;
	int dx;
	int dy;
	int dz;
	int yaw;
	int nextYaw;
	int yawDelta;

	segmentLength = CAM_FollowDriver_TrackPath_Length(curr, next, &dx, &dy, &dz);

	if ((sdata->gGT->gameMode1 & 0xf) != 0)
		pathProgress = 0;
	else if (speed > 0)
		pathProgress = cDC->trackPathProgress + speed;
	else
		pathProgress = cDC->trackPathProgress - speed;

	while (pathProgress >= segmentLength)
	{
		pathProgress -= segmentLength;
		curr = next;
		next = CAM_FollowDriver_TrackPath_GetNode(cDC, curr, speed);
		segmentLength = CAM_FollowDriver_TrackPath_Length(curr, next, &dx, &dy, &dz);
	}

	if (update)
	{
		cDC->trackPathProgress = pathProgress;
		cDC->trackPathNode = curr;
	}

	if (segmentLength != 0)
		ratio = (pathProgress << 12) / segmentLength;
	else
		ratio = 0;

	pos[0] = curr->pos[0] + (s16)((CAM_FollowDriver_TrackPath_MulLo(dx, ratio)) >> 12);
	pos[1] = curr->pos[1] + (s16)((CAM_FollowDriver_TrackPath_MulLo(dy, ratio)) >> 12) + 0x80;
	pos[2] = curr->pos[2] + (s16)((CAM_FollowDriver_TrackPath_MulLo(dz, ratio)) >> 12);

	yaw = (s16)ratan2(dx, dz) + 0x800;
	angleNext = CAM_FollowDriver_TrackPath_GetNode(cDC, next, speed);
	nextYaw = (s16)ratan2(angleNext->pos[0] - next->pos[0], angleNext->pos[2] - next->pos[2]) + 0x800;

	yawDelta = (nextYaw - yaw) & 0xfff;
	if (yawDelta >= 0x800)
		yawDelta -= 0x1000;

	return (yaw + (CAM_FollowDriver_TrackPath_MulLo(yawDelta, ratio) >> 12)) & 0xfff;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019e7c-0x80019f58
void CAM_LookAtPosition(u8 *scratchpad, int *positions, s16 *desiredPos, s16 *desiredRot)
{
	int dirX = desiredPos[0] - (positions[0] >> 8);
	int dirY = desiredPos[1] - ((positions[1] >> 8) + data.Spin360_heightOffset_driverPos[sdata->gGT->numPlyrCurrGame]);
	int dirZ = desiredPos[2] - (positions[2] >> 8);

	// Store dirX, dirY, dirZ in scratchpad
	*(int *)(scratchpad + 0x24c) = dirX;
	*(int *)(scratchpad + 0x250) = dirY;
	*(int *)(scratchpad + 0x254) = dirZ;

	int distance = SquareRoot0_stub(CAM_MulLo(dirX, dirX) + CAM_MulLo(dirZ, dirZ));

	// rotations
	desiredRot[0] = 0x800 - (s16)ratan2(dirY, distance);
	desiredRot[1] = (s16)ratan2(dirX, dirZ);
	desiredRot[2] = 0;

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019f58-0x8001a054
void CAM_FollowDriver_Spin360(struct CameraDC *cDC, u8 *scratchpad, struct Driver *d, s16 *desiredPos, s16 *desiredRot)
{
	int ratio;

	// === Union Missing ===
	// Not really "transitionTo" but the variables
	// are shared with other camera modes, therefore
	// need a union with proper names for each mode

	// rotate other way for odd number
	if ((d->driverID & 1) != 0)
	{
		cDC->spin360Angle -= cDC->transitionTo.pos[0];
	}
	else
	{
		// rotate one way
		cDC->spin360Angle += cDC->transitionTo.pos[0];
	}

	int angle = cDC->spin360Angle;
	ratio = MATH_Sin(angle);
	desiredPos[0] = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posCurr.x, 8), CTR_MipsSra(CAM_MulLo(ratio, cDC->transitionTo.pos[2]), 0xc));

	ratio = MATH_Cos(angle);
	desiredPos[2] = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posCurr.z, 8), CTR_MipsSra(CAM_MulLo(ratio, cDC->transitionTo.pos[2]), 0xc));

	desiredPos[1] = (s16)CTR_MipsAddLo((u16)cDC->transitionTo.pos[1], CTR_MipsSra(d->posCurr.y, 8));

	CAM_LookAtPosition(scratchpad, (int *)&d->posCurr.x, desiredPos, desiredRot);
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001a054-0x8001a0bc
void CAM_SetDesiredPosRot(struct CameraDC *cDC, s16 *pos, s16 *rot)
{
	int i;

	// save desired pos and rot
	for (i = 0; i < 3; i++)
	{
		cDC->transitionTo.pos[i] = pos[i];
		cDC->transitionTo.rot[i] = rot[i];
	}

	// 1 second, 30 frames
	cDC->transitionFrameCount = 0x1e;

	cDC->transitionFrame = 0;
	cDC->transitionBlend = 0x1000;

	cDC->flags |= CAMERA_FLAG_TRANSITION_AWAY;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001a0bc-0x8001b254.
void CAM_FollowDriver_Normal(struct CameraDC *cDC, struct Driver *d, s16 *pushBuffer, u8 *scratchpad, struct ZoomData *zoom)
{
	struct PushBuffer *pb = (struct PushBuffer *)pushBuffer;
	struct ScratchpadStruct *sps = (struct ScratchpadStruct *)(void *)scratchpad;
	struct GameTracker *gGT = sdata->gGT;
	struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[d->driverID];
	char state;
	s16 uVar8;
	u16 uVar9;
	s16 sVar10;
	u32 backupFlags;
	int x;
	s32 x_00;
	u32 uVar11;
	int iVar12;
	SVECTOR *psVar12;
	u32 uVar13;
	int iVar14;
	s16 local_40[3];
	s16 local_38[3];
	struct FlyInData flyInData;

	// backup flags
	backupFlags = cDC->flags;

	// disable Reverse Cam flag,
	// assuming you dont hold R2
	cDC->flags &= ~CAMERA_FLAG_REVERSE;

	if (
	    // If this is human and not AI
	    ((d->actionsFlagSet & ACTION_BOT) == 0) &&

	    // If not drawing intro-race cutscene
	    ((gGT->gameMode1 & START_OF_RACE) == 0) &&

	    // If you are holding R2
	    ((pad->buttonsHeldCurrFrame & 0x200) != 0))
	{
		// Reverse the camera
		cDC->flags |= CAMERA_FLAG_REVERSE;
	}

	// if camera just changed direction
	// (either forward to reverse, or reverse to forward)
	if (backupFlags != cDC->flags)
	{
		// set flag that cam just changed this frame
		cDC->flags |= CAMERA_FLAG_DIRECTION_CHANGED;
	}

	// 0 = forwards
	// 0x800 = backwards
	sVar10 = ((cDC->flags & CAMERA_FLAG_REVERSE) != 0) * 0x800;

	// if camera angle was not just changed
	if ((cDC->flags & CAMERA_FLAG_DIRECTION_CHANGED) == 0)
	{
		// absolute value driver speed
		x = (int)d->speedApprox;

		if (x < 0)
		{
			x = -x;
		}

		// driver speed slower than camera speed
		if (x < cDC->cameraMoveSpeed)
		{
			// transition inward
			uVar13 = (u32)zoom->percentage2;
			uVar11 = (u32)zoom->percentage1;
		}

		// driver speed faster than camera
		else
		{
			// transition outward
			uVar13 = (u32)zoom->percentage1;
			uVar11 = (u32)zoom->percentage1;
		}
		cDC->cameraMoveSpeed = (int)(uVar11 * cDC->cameraMoveSpeed + (0x100 - uVar13) * x) >> 8;
	}

	// if camera angle changed
	else
	{
		// absolute value driver speed
		x = (int)d->speedApprox;
		if (x < 0)
			x = -x;

		cDC->cameraMoveSpeed = x;
	}

	uVar8 = 0;
	if (gGT->numPlyrCurrGame != 2)
	{
		uVar8 = 0xff9c;
	}

	*(s16 *)(scratchpad + 0x20c) = uVar8;

	// 0x20e
	// camera RotY
	*(u16 *)(scratchpad + 0x20e) = (d->rotCurr.w + d->angle + 0x800 + sVar10) & 0xfff;

	*(s16 *)(scratchpad + 0x210) = cDC->desiredRot[0] * -2;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix((MATRIX *)(void *)(scratchpad + 0x220), (s16 *)(void *)(scratchpad + 0x20c));

	if (((cDC->flags & CAMERA_FLAG_FIRE_SPEED_ZOOM) != 0) && (x = (int)((u32)d->fireSpeedCap << 0x10) >> 0x14, cDC->fireSpeedZoom.timer < x))
	{
		cDC->fireSpeedZoom.timer = x;
	}
	*(s16 *)(scratchpad + 0x20c) = 0;
	*(s16 *)(scratchpad + 0x20e) = 0;

	uVar8 = VehCalc_MapToRange(cDC->cameraMoveSpeed, (int)zoom->speedMin, (int)zoom->speedMax, (int)zoom->distMin, (int)zoom->distMax);

	*(s16 *)(scratchpad + 0x210) = uVar8;

	if (cDC->fireSpeedZoom.timer == 0)
	{
		if (cDC->fireSpeedZoom.distanceOffset != 0)
		{
			cDC->fireSpeedZoom.distanceOffset -= gGT->elapsedTimeMS * 0x10;

			if (cDC->fireSpeedZoom.distanceOffset < 0)
				cDC->fireSpeedZoom.distanceOffset = 0;
		}
	}
	else
	{
		cDC->fireSpeedZoom.distanceOffset += gGT->elapsedTimeMS * 0x40;
		if (cDC->fireSpeedZoom.distanceOffset > 0x6000)
			cDC->fireSpeedZoom.distanceOffset = 0x6000;

		cDC->fireSpeedZoom.timer -= gGT->elapsedTimeMS;
		if (cDC->fireSpeedZoom.timer < 0)
			cDC->fireSpeedZoom.timer = 0;
	}
	*(s16 *)(scratchpad + 0x210) += cDC->fireSpeedZoom.distanceOffset >> 8;

	gte_SetRotMatrix((MATRIX *)(void *)(scratchpad + 0x220));
	psVar12 = (SVECTOR *)(void *)(scratchpad + 0x20c);
	gte_ldv0(psVar12);
	gte_rtv0();
	gte_stlvnl((VECTOR *)(void *)(scratchpad + 0x240));

	*(s16 *)(scratchpad + 0x20c) = 0;
	*(s16 *)(scratchpad + 0x20e) = 0x40;
	*(s16 *)(scratchpad + 0x210) = 0;

	gte_ldv0(psVar12);
	gte_rtv0();
	gte_stlvnl((VECTOR *)(void *)(scratchpad + 0x27c));

	*(int *)(scratchpad + 600) = CTR_MipsSra(d->posCurr.x, 8);
	*(int *)(scratchpad + 0x25c) = CTR_MipsSra(d->posCurr.y, 8);
	*(int *)(scratchpad + 0x260) = CTR_MipsSra(d->posCurr.z, 8);

	*(int *)(scratchpad + 600) += *(int *)(scratchpad + 0x27c);
	*(int *)(scratchpad + 0x260) += *(int *)(scratchpad + 0x284);
	*(int *)(scratchpad + 0x240) += *(int *)(scratchpad + 600);


	// mask-grab
	if ((cDC->flags & CAMERA_FLAG_MASK_GRAB) != 0)
	{
		*(int *)(scratchpad + 0x244) = CTR_MipsSra(d->quadBlockHeight, 8) + (int)cDC->maskGrabHeightOffset + (int)zoom->vertDistance;
	}

	else
	{
		*(int *)(scratchpad + 0x244) += *(int *)(scratchpad + 0x25c) + (int)zoom->vertDistance;
	}

	*(int *)(scratchpad + 0x248) += *(int *)(scratchpad + 0x260);

	uVar8 = 0;
	if (gGT->numPlyrCurrGame != 2)
		uVar8 = 0xff9c;

	// rotX
	*(s16 *)(scratchpad + 0x20c) = uVar8;

	// rotZ
	*(s16 *)(scratchpad + 0x210) = 0;

	// rotY
	*(u16 *)(scratchpad + 0x20e) = (d->rotCurr.w + d->angle + d->turnAngleCurr + 0x800 + sVar10) & 0xfff;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix((MATRIX *)(void *)(scratchpad + 0x220), (s16 *)(void *)(scratchpad + 0x20c));

	// if racer is not damaged,
	// slight-down view angle
	if ((d->actionsFlagSet & ACTION_WARP) == 0)
	{
		cDC->damagePitchOffset -= 8;
		if (cDC->damagePitchOffset < -0x20)
			cDC->damagePitchOffset = -0x20;
	}

	// if racer is damaged,
	// straight-forward angle
	else
	{
		cDC->damagePitchOffset += 8;
		if (cDC->damagePitchOffset > 0)
			cDC->damagePitchOffset = 0;
	}

	// Z, Y, X
	*(s16 *)(scratchpad + 0x210) = cDC->damagePitchOffset;
	*(s16 *)(scratchpad + 0x20e) = 0;
	*(s16 *)(scratchpad + 0x20c) = 0;

	gte_SetRotMatrix((MATRIX *)(void *)(scratchpad + 0x220));

	psVar12 = (SVECTOR *)(void *)(scratchpad + 0x20c);

	gte_ldv0(psVar12);
	gte_rtv0();
	gte_stsv(psVar12);

	*(int *)(scratchpad + 600) += (int)*(s16 *)(scratchpad + 0x20c);
	*(int *)(scratchpad + 0x260) += (int)*(s16 *)(scratchpad + 0x210);
	*(int *)(scratchpad + 0x25c) += (int)*(s16 *)(scratchpad + 0x20e) + (int)zoom->angle[2];

	cDC->desiredRot[0] = ((zoom->angle[1] * (int)cDC->desiredRot[0]) + ((0x100 - (int)zoom->angle[1]) * (int)d->rotCurr.z)) >> 8;


	state = d->kartState;

	if (state == KS_MASK_GRABBED)
	{
		// pushBuffer position
		*(int *)(scratchpad + 0x240) = (int)pb->pos[0];
		*(int *)(scratchpad + 0x244) = (int)pb->pos[1];
		*(int *)(scratchpad + 0x248) = (int)pb->pos[2];

		// reset camera interpolation
		cDC->heightSmoothing.startOffset = 0;
		cDC->heightSmoothing.framesRemaining = 0;
		cDC->heightSmoothing.currentOffset = 0;
	}

	if (state == KS_ENGINE_REVVING)
	{
		// reset camera interpolation
		cDC->heightSmoothing.startOffset = 0;
		cDC->heightSmoothing.framesRemaining = 0;
		cDC->heightSmoothing.currentOffset = 0;
	}

	if ((d->kartState != KS_BLASTED) && ((d->actionsFlagSet & ACTION_TOUCH_GROUND) != 0) && (cDC->BlastedLerp.boolLerpPending != 0))
	{
		cDC->BlastedLerp.boolLerpPending = 0;

		cDC->BlastedLerp.desiredRot[0] = cDC->lookAtPos.x - *(s16 *)(scratchpad + 600);
		cDC->BlastedLerp.desiredRot[1] = cDC->lookAtPos.y - *(s16 *)(scratchpad + 0x25c);
		cDC->BlastedLerp.desiredRot[2] = cDC->lookAtPos.z - *(s16 *)(scratchpad + 0x260);

		cDC->BlastedLerp.desiredPos[0] = cDC->cameraPos.x - *(s16 *)(scratchpad + 0x240);
		cDC->BlastedLerp.desiredPos[1] = cDC->cameraPos.y - *(s16 *)(scratchpad + 0x244);
		cDC->BlastedLerp.desiredPos[2] = cDC->cameraPos.z - *(s16 *)(scratchpad + 0x248);

		cDC->BlastedLerp.framesRemaining = 8;
	}

	// if not arcade end-of-race
	if (((cDC->flags & CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED) == 0) && (cDC->cameraMode == 0))
	{
		if ((d->kartState != KS_BLASTED) && (cDC->BlastedLerp.boolLerpPending == 0))
			goto LAB_8001a8c0;

		if (cDC->BlastedLerp.boolLerpPending == 0)
		{
			*(s16 *)((int)cDC + 0xc8) = cDC->lookAtPos.y - cDC->cameraPos.y;
			*(s16 *)((int)cDC + 0xca) = cDC->cameraPos.y - CTR_MipsSra(d->quadBlockHeight, 8);
		}

		cDC->BlastedLerp.boolLerpPending = 1;

		if ((cDC->cameraPos.y < *(int *)(scratchpad + 0x244)) &&
		    (x = (int)*(s16 *)((int)cDC + 0xca) + CTR_MipsSra(d->quadBlockHeight, 8), x < *(int *)(scratchpad + 0x244)))
		{
			*(int *)(scratchpad + 0x244) = x;
		}

	LAB_8001a8b0:

		if (cDC->BlastedLerp.boolLerpPending == 0)
			goto LAB_8001a8c0;
	}

	// if this is arcade end-of-race
	else
	{
		if (cDC->BlastedLerp.boolLerpPending != 0)
		{
			cDC->BlastedLerp.boolLerpPending = 0;

			cDC->BlastedLerp.desiredRot[0] = cDC->lookAtPos.x - *(s16 *)(scratchpad + 0x258);
			cDC->BlastedLerp.desiredRot[1] = cDC->lookAtPos.y - *(s16 *)(scratchpad + 0x25c);
			cDC->BlastedLerp.desiredRot[2] = cDC->lookAtPos.z - *(s16 *)(scratchpad + 0x260);

			cDC->BlastedLerp.desiredPos[0] = cDC->cameraPos.x - *(s16 *)(scratchpad + 0x240);
			cDC->BlastedLerp.desiredPos[1] = cDC->cameraPos.y - *(s16 *)(scratchpad + 0x244);
			cDC->BlastedLerp.desiredPos[2] = cDC->cameraPos.z - *(s16 *)(scratchpad + 0x248);

			cDC->BlastedLerp.framesRemaining = 8;

			goto LAB_8001a8b0;
		}

	LAB_8001a8c0:

		// if frame countdown is not finished
		if (cDC->BlastedLerp.framesRemaining != 0)
		{
			*(int *)(scratchpad + 0x240) += (cDC->BlastedLerp.desiredPos[0] * cDC->BlastedLerp.framesRemaining) >> 3;
			*(int *)(scratchpad + 0x244) += (cDC->BlastedLerp.desiredPos[1] * cDC->BlastedLerp.framesRemaining) >> 3;
			*(int *)(scratchpad + 0x248) += (cDC->BlastedLerp.desiredPos[2] * cDC->BlastedLerp.framesRemaining) >> 3;

			*(int *)(scratchpad + 0x258) += (cDC->BlastedLerp.desiredRot[0] * cDC->BlastedLerp.framesRemaining) >> 3;
			*(int *)(scratchpad + 0x25c) += (cDC->BlastedLerp.desiredRot[1] * cDC->BlastedLerp.framesRemaining) >> 3;
			*(int *)(scratchpad + 0x260) += (cDC->BlastedLerp.desiredRot[2] * cDC->BlastedLerp.framesRemaining) >> 3;

			// decrease frame countdown
			cDC->BlastedLerp.framesRemaining--;
		}
	}

	CAM_FindClosestQuadblock(sps, cDC, d, (VECTOR *)(void *)(scratchpad + 0x240));

	struct QuadBlock *quad = sps->hit.ptrQuadblock;
	if ((sps->boolDidTouchQuadblock == 0) || ((quad->quadFlags & CAM_FOLLOW_DRIVER_QUAD_FLAGS_SKIP_TERRAIN_HEIGHT) != 0))
	{
		if (*(int *)(scratchpad + 0x244) < (int)cDC->heightSmoothing.currentOffset + CTR_MipsSra(d->posCurr.y, 8))
		{
			cDC->heightSmoothing.framesRemaining = 8;
			cDC->heightSmoothing.startOffset = cDC->heightSmoothing.currentOffset;
			*(int *)(scratchpad + 0x244) = (int)cDC->heightSmoothing.currentOffset + CTR_MipsSra(d->posCurr.y, 8);

			goto LAB_8001ab04;
		}

		cDC->heightSmoothing.framesRemaining = 8;
		cDC->heightSmoothing.startOffset = *(s16 *)(scratchpad + 0x244) - (s16)CTR_MipsSra(d->posCurr.y, 8);
	}

	else
	{
		state = (char)quad->terrain_type;

		// Mud, Water, or FastWater
		if (((state == 0xe) || (state == 4)) || (state == 0xd))
		{
			*(s16 *)(scratchpad + 0x1e) = 0;
		}

		x = (int)*(s16 *)(scratchpad + 0x1e) + (int)zoom->vertDistance;
		if (*(int *)(scratchpad + 0x244) < x)
		{
			*(int *)(scratchpad + 0x244) = x;
		}

		x = cDC->heightSmoothing.framesRemaining;
		if (x != 0)
		{
			*(int *)(scratchpad + 0x244) =

			    (8 - x) * *(int *)(scratchpad + 0x244) + x * ((int)cDC->heightSmoothing.startOffset + CTR_MipsSra(d->posCurr.y, 8)) >> 3;

			cDC->heightSmoothing.framesRemaining += -1;
		}
	}
	cDC->heightSmoothing.currentOffset = *(s16 *)(scratchpad + 0x244) - CTR_MipsSra(d->posCurr.y, 8);
LAB_8001ab04:

	// if mask grabs you when you're underwater
	if (((gGT->level1->configFlags & 2) != 0) && (*(int *)(scratchpad + 0x244) < zoom->vertDistance))
	{
		*(int *)(scratchpad + 0x244) = zoom->vertDistance;
	}

	if (cDC->BlastedLerp.boolLerpPending != 0)
	{
		*(int *)(scratchpad + 0x25c) = *(int *)(scratchpad + 0x244) + (int)*(s16 *)((int)cDC + 0xc8);
	}

	if (d->kartState == KS_MASK_GRABBED)
	{
		pb->rot[2] -= (pb->rot[2] >> 3);

		// camera dirX, cameraPosX minus driverPosX
		*(int *)(scratchpad + 0x24c) = (int)pb->pos[0] - CTR_MipsSra(d->posCurr.x, 8);

		// camera dirY, cameraPosY minus driverPosY, plus something else
		*(int *)(scratchpad + 0x250) = (int)pb->pos[1] - (CTR_MipsSra(d->posCurr.y, 8) + (int)zoom->angle[2]);

		// camera dirZ, cameraPosZ minus driverPosZ
		*(int *)(scratchpad + 0x254) = (int)pb->pos[2] - CTR_MipsSra(d->posCurr.z, 8);

		if (pb->rot[0] < 0x800)
		{
			pb->rot[0] += 0x10;
			if (pb->rot[0] > 0x800)
				pb->rot[0] = 0x800;
		}
	}

	// if not mask grab
	else
	{
		x = *(int *)(scratchpad + 0x248) - *(int *)(scratchpad + 0x260);

		// camera direction
		*(int *)(scratchpad + 0x254) = x;
		*(int *)(scratchpad + 0x24c) = *(int *)(scratchpad + 0x240) - *(int *)(scratchpad + 600);
		*(int *)(scratchpad + 0x250) = *(int *)(scratchpad + 0x244) - *(int *)(scratchpad + 0x25c);

		// camera rotation
		x_00 = ratan2(*(s32 *)(scratchpad + 0x24c), x);
		pb->rot[1] = (s16)x_00;

		x_00 = SquareRoot0_stub(*(int *)(scratchpad + 0x24c) * *(int *)(scratchpad + 0x24c) + *(int *)(scratchpad + 0x254) * *(int *)(scratchpad + 0x254));

		x_00 = ratan2(*(s32 *)(scratchpad + 0x250), x_00);
		pb->rot[0] = 0x800 - (s16)x_00;

		pb->rot[2] = (s16)((u32)((int)zoom->angle[0] * (int)cDC->desiredRot[0]) >> 8);
	}

	// something with pushBuffer position
	*(int *)(scratchpad + 0x214) = *(int *)(scratchpad + 0x240) - (int)pb->pos[0];
	*(int *)(scratchpad + 0x218) = *(int *)(scratchpad + 0x244) - (int)pb->pos[1];
	*(int *)(scratchpad + 0x21c) = *(int *)(scratchpad + 0x248) - (int)pb->pos[2];

	cDC->pushBufferPosCorrection.x -= (*(int *)(scratchpad + 0x240) - cDC->cameraPos.x);
	cDC->pushBufferPosCorrection.y -= (*(int *)(scratchpad + 0x244) - cDC->cameraPos.y);
	cDC->pushBufferPosCorrection.z -= (*(int *)(scratchpad + 0x248) - cDC->cameraPos.z);

	if (cDC->pushBufferPosCorrection.x > 2)
		cDC->pushBufferPosCorrection.x = 2;
	if (cDC->pushBufferPosCorrection.y > 2)
		cDC->pushBufferPosCorrection.y = 2;
	if (cDC->pushBufferPosCorrection.z > 2)
		cDC->pushBufferPosCorrection.z = 2;

	if (cDC->pushBufferPosCorrection.x < -2)
		cDC->pushBufferPosCorrection.x = -2;
	if (cDC->pushBufferPosCorrection.y < -2)
		cDC->pushBufferPosCorrection.y = -2;
	if (cDC->pushBufferPosCorrection.z < -2)
		cDC->pushBufferPosCorrection.z = -2;

	if (d->kartState != KS_MASK_GRABBED)
	{
		// pushBuffer position
		pb->pos[0] += *(s16 *)(scratchpad + 0x214) + cDC->pushBufferPosCorrection.x;
		pb->pos[1] += *(s16 *)(scratchpad + 0x218) + cDC->pushBufferPosCorrection.y;
		pb->pos[2] += *(s16 *)(scratchpad + 0x21c) + cDC->pushBufferPosCorrection.z;
	}

	cDC->cameraPos.x = *(int *)(scratchpad + 0x240);
	cDC->cameraPos.y = *(int *)(scratchpad + 0x244);
	cDC->cameraPos.z = *(int *)(scratchpad + 0x248);
	cDC->lookAtPos.x = *(int *)(scratchpad + 0x258);
	cDC->lookAtPos.y = *(int *)(scratchpad + 0x25c);
	cDC->lookAtPos.z = *(int *)(scratchpad + 0x260);

	// backup flags (again)
	backupFlags = cDC->flags;

	cDC->flags &= ~CAMERA_FLAG_MASK_GRAB;

	if (
	    // transitioning, end-race battle, intro-race
	    ((backupFlags & (CAMERA_FLAG_TRANSITION_AWAY | CAMERA_FLAG_BATTLE_END_OF_RACE)) == 0) && ((gGT->gameMode1 & START_OF_RACE) == 0))
	{
		return;
	}

	// === Transition, end-race battle, intro-race ===

	// if end-of-race battle
	if ((backupFlags & CAMERA_FLAG_BATTLE_END_OF_RACE) != 0)
	{
		CAM_FollowDriver_Spin360(cDC, scratchpad, d, &local_40[0], &local_38[0]);

		// reverse interpolation of fly-in [0x1000 to 0]
		x = 0x1000 - cDC->transitionBlend;
	}

	// if not end-of-race battle
	else
	{
		// if transitioning round-trip
		if ((backupFlags & CAMERA_FLAG_TRANSITION_AWAY) != 0)
		{
			// cameraDC TransitionTo pos and rot
			local_40[0] = cDC->transitionTo.pos[0];
			local_40[1] = cDC->transitionTo.pos[1];
			local_40[2] = cDC->transitionTo.pos[2];

			local_38[0] = cDC->transitionTo.rot[0];
			local_38[1] = cDC->transitionTo.rot[1];
			local_38[2] = cDC->transitionTo.rot[2];

			// interpolate fly-in [0 to 0x1000]
			x = cDC->transitionBlend;
		}

		// if startline camera
		else
		{
			// get camera path from level
			struct SpawnType1 *st1 = gGT->level1->ptrSpawnType1;
			void **pointers = ST1_GETPOINTERS(st1);
			x = pointers[ST1_CAMERA_PATH];
			int flyInDone = 0;

			// No camera + No ghosts (battle maps)
			if (st1->count < 4)
			{
				// startline fly-in is done
				flyInDone = 1;
				x = 0x1000;
			}

			// run fly-in animation
			else
			{
				flyInData.ptrEnd = x + 0x354;
				flyInData.ptrStart = x;
				flyInData.frameCount1 = 0x96;
				flyInData.frameCount2 = 0x8e;

				// which frame of fly-in you are in
				x = 0xa5 - (u16)cDC->transitionFrame;

				if ((s16)x > 0x96)
					x = 0x96;

				CAM_StartLine_FlyIn(&flyInData, 0x96, x, &local_40[0], &local_38[0]);

				// get interpolation of fly-in [0 - 0x1000]
				x = (int)cDC->transitionBlend;
			}

			if (cDC->transitionFrame < 1)
			{
				flyInDone = 1;
			}

			// Press Triangle
			if ((pad->buttonsTapped & BTN_TRIANGLE) != 0)
			{
				cDC->flags |= CAMERA_FLAG_RESET_RAIN_POS | CAMERA_FLAG_DIRECTION_CHANGED;
				flyInDone = 1;
			}

			// if startline fly-in is done
			if (flyInDone)
			{
				gGT->hudFlags |= 0x21;
				gGT->gameMode1 &= ~(START_OF_RACE);
			}
		}
	}

	// use camera pos+rot, TransitionTo pos+rot, camera pos+rot, and interpolation
	CAM_ProcessTransition(&pb->pos[0], &pb->rot[0], &local_40[0], &local_38[0], &pb->pos[0], &pb->rot[0], x);

	*(int *)(scratchpad + 0x240) = (int)pb->pos[0];
	*(int *)(scratchpad + 0x244) = (int)pb->pos[1];
	*(int *)(scratchpad + 0x248) = (int)pb->pos[2];

	CAM_FindClosestQuadblock((struct ScratchpadStruct *)(void *)scratchpad, cDC, d, (VECTOR *)(void *)(scratchpad + 0x240));

	x = cDC->transitionFrameCount;
	iVar14 = cDC->transitionFrameCount;

	if (iVar14 != 0)
	{
		iVar12 = (int)cDC->transitionFrame;

		if (iVar12 <= iVar14)
		{
			x = x >> 1;

			if (iVar12 < x)
			{
				// Sine(angle)
				x = MATH_Sin(0x400 - (iVar12 << 10) / x);

				cDC->transitionBlend = (s16)(x / 2) + 0x800;
			}
			else
			{
				iVar14 = (iVar12 - iVar14) * 0x400;

				// Cosine(angle)
				x = MATH_Cos(iVar14 / x);

				cDC->transitionBlend = 0x800 - (s16)(x / 2);
			}
		}
	}

	// backup  flags
	backupFlags = cDC->flags;

	// if transition is a round-trip,
	// like Load/Save that moves and comes back
	if ((backupFlags & CAMERA_FLAG_TRANSITION_AWAY) != 0)
	{
		// if not transitioning back to player
		if ((backupFlags & CAMERA_FLAG_TRANSITION_BACK) == 0)
		{
			// Definitely >, not >=,
			// or else the transition is off-by-one,

			// |= 0x800, stop transitioning away from player,
			// sit stationary away from player, wait before moving back

			cDC->transitionFrame++;
			if (cDC->transitionFrame > cDC->transitionFrameCount)
			{
				cDC->transitionFrame = cDC->transitionFrameCount;
				cDC->flags |= CAMERA_FLAG_TRANSITION_HOLD;
				return;
			}
		}

		// if transitioning back to player
		else
		{
			// definitely do < 0, not == 0,
			// or else the transition is off-by-one

			// &= ~(0xE00), remove transition flags

			// optimization
			goto CountdownTransitionFrame;
		}
	}

	// if not a round-trip,
	// like startline camera
	else
	{
		// If game is paused
		if ((gGT->gameMode1 & PAUSE_ALL) != 0)
			return;

	CountdownTransitionFrame:

		cDC->transitionFrame--;
		if (cDC->transitionFrame < 0)
		{
			// This is normally not here,
			// but saves byte budget
			cDC->flags &= ~(CAMERA_FLAG_TRANSITION_AWAY | CAMERA_FLAG_TRANSITION_BACK | CAMERA_FLAG_TRANSITION_HOLD);

			cDC->transitionFrame = 0;
			return;
		}
	}

	return;
}

int CAM_MapRange_PosPoints(s16 *pos1, s16 *pos2, s16 *currPos)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001b254-0x8001b334.
	SVec3 pathDelta;
	pathDelta.x = pos1[0] - pos2[0];
	pathDelta.y = pos1[1] - pos2[1];
	pathDelta.z = pos1[2] - pos2[2];

	MATH_VectorNormalize(&pathDelta);

	SVec3 currDelta;
	currDelta.x = currPos[0] - pos1[0];
	currDelta.y = currPos[1] - pos1[1];
	currDelta.z = currPos[2] - pos1[2];

	CTC2(CTR_PackS16Pair(pathDelta.x, pathDelta.y), 0);
	CTC2((s32)pathDelta.z, 1);
	gte_ldv0(&currDelta);
	gte_mvmva(0, 0, 0, 3, 0);

	return MFC2_S(25) >> 12;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001b334-0x8001c360.
void CAM_ThTick(struct Thread *t)
{
	s16 sVar1;
	int bVar2;
	u16 uVar4;
	s16 sVar5;
	s16 sVar6;
	int iVar7;
	int iVar8;
	u32 uVar9;
	u32 uVar10;
	struct PVS *psVar11;
	int *piVar12;
	struct Instance **ppsVar13;
	struct SpawnType1 *psVar14;
	struct CheckpointNode *psVar15;
	u32 uVar16;
	int iVar17;
	int iVar18;
	s16 *psVar19;
	s16 *psVar20;
	s16 *psVar21;
	u32 uVar22;
	struct CameraDC *cDC;
	struct PushBuffer *pb;
	struct ZoomData *ptrZoomData;
	struct Driver *d;
	u8 *scratchpadBytes;
	s16 *scratchpad;
	int iVar24;
	int iVar25;

	struct GameTracker *gGT = sdata->gGT;
	scratchpadBytes = CTR_SCRATCHPAD_PTR(u8, 0x108);
	scratchpad = (s16 *)(void *)scratchpadBytes;
	cDC = (struct CameraDC *)t->inst;
	d = cDC->driverToFollow;
	pb = cDC->pushBuffer;

	if ((cDC->flags & CAMERA_FLAG_FROZEN) != 0)
	{
		return;
	}

	if (((((gGT->gameMode1 & (PAUSE_ALL | START_OF_RACE | MAIN_MENU | GAME_CUTSCENE)) == 0) && (d->instSelf->thread->funcThTick == 0)) &&
	     ((d->actionsFlagSet & ACTION_BOT) == 0)) &&
	    (((d->kartState != KS_WARP_PAD && (d->kartState != KS_FREEZE)) &&
	      (((gGT->gameMode2 & 4) == 0 && ((sdata->gGamepads->gamepad[cDC->cameraID].buttonsTapped & BTN_L2_one) != 0))))))
	{
		uVar4 = cDC->zoomToggleState + 1;
		cDC->zoomToggleState = uVar4;
		if (1 < uVar4)
		{
			cDC->zoomToggleState = 0;
		}

		// NOTE(aalhendi): Retail clamps before this check, making this branch dead in this flow.
		if (cDC->zoomToggleState == 2)
		{
			cDC->cameraMode = 0xf;
		}
		else
		{
			cDC->nearOrFar = cDC->zoomToggleState;
			cDC->cameraMode = 0;
		}
	}

	ptrZoomData = &data.NearCam4x3;
	if (gGT->numPlyrCurrGame == 2)
		ptrZoomData = &data.NearCam8x3;

	ptrZoomData = &ptrZoomData[cDC->nearOrFar * 2];

	if ((cDC->flags & CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED) == 0)
		goto SkipNewCameraEOR;

	psVar14 = gGT->level1->ptrSpawnType1;

	psVar21 = 0;
	if (psVar14->count < 3)
		goto SkipNewCameraEOR;

	void **ptrs = ST1_GETPOINTERS(psVar14);
	psVar19 = ptrs[ST1_CAMERA_EOR];

	// number of EOR cameras
	sVar6 = *psVar19;

	// advance to first EOR
	psVar20 = psVar19 + 1;

	if (sVar6 != 0)
	{
		uVar22 = (u32)d->checkpoint.currentIndex;

		// pointer to modeID in EOR camera
		psVar19 += 2;

		for (; sVar6 != 0; sVar6--)
		{
			// camera mode
			iVar7 = (int)*psVar19;
			if (iVar7 < 0)
			{
				iVar7 = -iVar7;
			}

			// respawnPoint
			uVar16 = (u32)*psVar20;

			// +2 to include respawnPoint and modeID
			psVar20 = (s16 *)((int)psVar19 + data.EndOfRace_Camera_Size[iVar7] + 2);

			psVar15 = &gGT->level1->ptr_restart_points[uVar16];

			if ((uVar22 == uVar16) || (uVar22 == psVar15->nextIndex_forward) || (uVar22 == psVar15->nextIndex_left) ||
			    (uVar22 == psVar15->nextIndex_backward) || (uVar22 == psVar15->nextIndex_right))
			{
				psVar21 = psVar19;
			}
			psVar19 = psVar20 + 1;
		}
	}

	// if no EOR found, or EOR is already in-use
	if ((psVar21 == 0) || (psVar21 == cDC->currEOR))
		goto SkipNewCameraEOR;

	cDC->currEOR = (void *)psVar21;

	sVar6 = *psVar21;
	psVar19 = psVar21 + 1;
	sVar5 = sVar6;
	if (sVar6 < 0)
	{
		sVar5 = -sVar6;
	}
	cDC->cameraMode = sVar5;
	uVar22 = cDC->flags & ~CAMERA_FLAG_ARCADE_END_OF_RACE_ACTIVE;
	cDC->flags = uVar22 | CAMERA_FLAG_RESET_RAIN_POS | CAMERA_FLAG_DIRECTION_CHANGED;
	if (sVar6 < 0)
	{
		cDC->flags = uVar22 | CAMERA_FLAG_RESET_RAIN_POS | CAMERA_FLAG_DIRECTION_CHANGED | CAMERA_FLAG_ARCADE_END_OF_RACE_ACTIVE;
	}
	cDC->flags = cDC->flags | CAMERA_FLAG_ARCADE_END_OF_RACE_ACTIVE;

	switch (cDC->cameraMode)
	{
	case 0:
		pb->pos[0] = *(s16 *)(&d->instSelf->matrix.t[0]);
		pb->pos[1] = *(s16 *)(&d->instSelf->matrix.t[1]);
		pb->pos[2] = *(s16 *)(&d->instSelf->matrix.t[2]);
		pb->rot[0] = d->rotCurr.x;
		pb->rot[1] = d->rotCurr.y;
		pb->rot[2] = d->rotCurr.z;
		cDC->heightSmoothing.startOffset = 0;
		break;
	case 3:
		pb->pos[0] = *psVar19;
		pb->pos[1] = psVar21[2];
		pb->pos[2] = psVar21[3];
		pb->rot[0] = psVar21[4];
		pb->rot[1] = psVar21[5];
		pb->rot[2] = psVar21[6];
		*(s16 *)&cDC->action = *psVar19;
		*(s16 *)((int)&cDC->action + 2) = psVar21[2];
		*(s16 *)&cDC->mode = psVar21[3];
		*(s16 *)&cDC->unk0xC = psVar21[4];
		*(s16 *)((int)&cDC->unk0xC + 2) = psVar21[5];
		cDC->desiredRot[0] = psVar21[6];
		break;
	case 4:
		pb->pos[0] = *psVar19;
		pb->pos[1] = psVar21[2];
		pb->pos[2] = psVar21[3];
		break;
	case 7:
		(cDC->transitionTo).pos[0] = *psVar19;
		sVar6 = psVar21[2];
		uVar22 = cDC->flags & 0xffffffbf;
		cDC->flags = uVar22;
		(cDC->transitionTo).pos[1] = sVar6;
		if (psVar21[3] != 0)
		{
			cDC->flags = uVar22 | 0x40;
		}
		break;
	case 8:
	case 14:
		cDC->driverOffset_CamEyePos.x = *psVar19;
		cDC->driverOffset_CamEyePos.y = psVar21[2];
		cDC->driverOffset_CamEyePos.z = psVar21[3];

		cDC->driverOffset_CamLookAtPos.x = psVar21[4];
		cDC->driverOffset_CamLookAtPos.y = psVar21[5];
		cDC->driverOffset_CamLookAtPos.z = psVar21[6];

		// driverOffset_CamEyePos
		sVar6 = *psVar19;
		sVar5 = psVar21[2];
		sVar1 = psVar21[3];

		iVar7 = VehCalc_MapToRange((int)sVar6 * (int)sVar6 + (int)sVar5 * (int)sVar5 + (int)sVar1 * (int)sVar1, 0x10000, 0x190000, 0x80, 0xf0);

		cDC->angleAxisLerpRatio = (s16)iVar7;
		break;
	case 9:
	case 13:
		sVar6 = *psVar19;
		psVar15 = gGT->level1->ptr_restart_points;
		cDC->trackPathProgress = 0;
		cDC->trackPathNode = psVar15 + sVar6;
		(cDC->transitionTo).pos[0] = psVar21[2];
		(cDC->transitionTo).pos[1] = psVar21[3];
		(cDC->transitionTo).pos[2] = psVar21[4];
		(cDC->transitionTo).rot[0] = psVar21[5];
		(cDC->transitionTo).rot[1] = psVar21[6];
		(cDC->transitionTo).rot[2] = psVar21[7];
		cDC->eorModeData.trackPathSpeed = psVar21[8];
		break;

	// Spin360
	case 10:

		// spinSpeed
		sVar6 = *psVar19;

		psVar19 = psVar21 + 2;
		goto LAB_8001b928;

	case 11:
		sVar6 = *(s16 *)&pb->distanceToScreen_CURR;
		pb->pos[0] = *psVar19;
		pb->pos[1] = psVar21[2];
		pb->pos[2] = psVar21[3];
		psVar19 = psVar21 + 5;
		sVar6 = psVar21[4] - sVar6;
	LAB_8001b928:
		(cDC->transitionTo).pos[0] = sVar6;
		(cDC->transitionTo).pos[1] = *psVar19;
		(cDC->transitionTo).pos[2] = psVar19[1];
		break;
	case 12:
		(cDC->transitionTo).pos[0] = *psVar19;
		(cDC->transitionTo).pos[1] = psVar21[2];
		(cDC->transitionTo).pos[2] = psVar21[3];
		(cDC->transitionTo).rot[0] = psVar21[4];
		(cDC->transitionTo).rot[1] = psVar21[5];
		(cDC->transitionTo).rot[2] = psVar21[6];
		cDC->eorModeData.pointPath.endPos.x = psVar21[7];
		cDC->eorModeData.pointPath.endPos.y = psVar21[8];
		cDC->eorModeData.pointPath.endPos.z = psVar21[9];
		cDC->eorModeData.pointPath.speed = psVar21[10];
	}

SkipNewCameraEOR:

	pb->distanceToScreen_PREV = pb->distanceToScreen_CURR;
	sVar6 = cDC->cameraMode;

	if (sVar6 != 0)
	{
		sVar5 = cDC->cameraMode;
		cDC->BlastedLerp.framesRemaining = 0;

		if (sVar6 != 0)
		{
			if (sVar6 == 4)
			{
			LAB_8001c11c:
				CAM_LookAtPosition(scratchpadBytes, (int *)&d->posCurr.x, &pb->pos[0], &pb->rot[0]);
				psVar21 = scratchpad;
			LAB_8001c128:
				scratchpad = psVar21;
			}
			else
			{
				psVar21 = scratchpad;
				if (sVar6 == 10)
				{
					CAM_FollowDriver_Spin360(cDC, scratchpadBytes, d, (s16 *)pb->pos, (s16 *)pb->rot);
					goto LAB_8001c128;
				}
				if (sVar6 != 0xb)
				{
					if (sVar6 == 0xc)
					{
						if (cDC->cameraModePrev != 0xc)
						{
							cDC->transitionFrame = 0;
						}

						s16 stackMemPos[3];

						stackMemPos[0] = (s16)CTR_MipsSra(d->posCurr.x, 8);
						stackMemPos[1] = (s16)CTR_MipsSra(d->posCurr.y, 8);
						stackMemPos[2] = (s16)CTR_MipsSra(d->posCurr.z, 8);

						iVar8 = CAM_MapRange_PosPoints((cDC->transitionTo).pos, (cDC->transitionTo).rot, &stackMemPos[0]);

						iVar17 = (int)cDC->eorModeData.pointPath.speed;

						stackMemPos[0] = cDC->eorModeData.pointPath.endPos.x - (cDC->transitionTo).rot[0];
						stackMemPos[1] = cDC->eorModeData.pointPath.endPos.y - (cDC->transitionTo).rot[1];
						stackMemPos[2] = cDC->eorModeData.pointPath.endPos.z - (cDC->transitionTo).rot[2];

						iVar7 = iVar17;
						if (iVar17 < 0)
						{
							iVar7 = -iVar17;
						}
						if (iVar17 < 0)
						{
							stackMemPos[0] = -stackMemPos[0];
							stackMemPos[1] = -stackMemPos[1];
							stackMemPos[2] = -stackMemPos[2];
						}

						iVar24 = SquareRoot0_stub((int)stackMemPos[0] * (int)stackMemPos[0] + (int)stackMemPos[1] * (int)stackMemPos[1] +
						                          (int)stackMemPos[2] * (int)stackMemPos[2]);

						iVar18 = cDC->trackPathProgress << 0xc;
						iVar25 = iVar18 / iVar24;
						/*
						if (iVar24 == 0)
						{
						    trap(0x1c00);
						}
						if ((iVar24 == -1) && (iVar18 == -0x80000000))
						{
						    trap(0x1800);
						}
						*/
						cDC->trackPathProgress = cDC->trackPathProgress + (((cDC->transitionFrame * 0x1000) / 0x1e) * iVar7 >> 0xc);
						if (iVar8 < 1)
						{
							if (iVar25 < 0x1001)
							{
								if (cDC->transitionFrame < 0x1e)
								{
									cDC->transitionFrame = cDC->transitionFrame + 1;
								}
							}
							else if (cDC->transitionFrame != 0)
							{
								cDC->transitionFrame = cDC->transitionFrame + -1;
							}
						}
						else
						{
							cDC->trackPathProgress = 0;
							cDC->transitionFrame = 0;
						}
						psVar21 = (cDC->transitionTo).rot;
						if (iVar17 < 0)
						{
							psVar21 = cDC->eorModeData.pointPath.endPos.v;
						}
						pb->pos[0] = psVar21[0] + (s16)((stackMemPos[0] * iVar25) >> 0xc);
						pb->pos[1] = psVar21[1] + (s16)((stackMemPos[1] * iVar25) >> 0xc);
						pb->pos[2] = psVar21[2] + (s16)((stackMemPos[2] * iVar25) >> 0xc);
						goto LAB_8001c11c;
					}
					if (sVar6 == 7)
					{
						pb->pos[0] = (s16)CTR_MipsSra(d->posCurr.x, 8);
						pb->pos[1] = (cDC->transitionTo).pos[0] + (s16)CTR_MipsSra(d->posCurr.y, 8);
						pb->pos[2] = (s16)CTR_MipsSra(d->posCurr.z, 8);
						sVar6 = (cDC->transitionTo).pos[1];
						pb->rot[1] = 0;
						pb->rot[2] = 0;
						pb->rot[0] = sVar6 + 0x400;
						psVar21 = scratchpad;
						if ((cDC->flags & 0x40) != 0)
						{
							pb->rot[1] = d->angle + 0x800;
						}
					}
					else if ((u16)(sVar5 - 0xfU) < 2)
					{
						pb->pos[0] = sdata->FirstPersonCamera.posOffset[0] + (s16)CTR_MipsSra(d->posCurr.x, 8);
						pb->pos[1] = sdata->FirstPersonCamera.posOffset[1] + (s16)CTR_MipsSra(d->posCurr.y, 8);
						pb->pos[2] = sdata->FirstPersonCamera.posOffset[2] + (s16)CTR_MipsSra(d->posCurr.z, 8);

						pb->rot[0] = sdata->FirstPersonCamera.rotOffset[0] + (d->rotCurr).x;
						if (cDC->cameraMode == 0x10)
							pb->rot[1] = sdata->FirstPersonCamera.rotOffset[1] + d->angle;
						else
							pb->rot[1] = sdata->FirstPersonCamera.rotOffset[1] + (d->rotCurr).y;
						pb->rot[2] = sdata->FirstPersonCamera.rotOffset[2] + (d->rotCurr).z;
					}
					else
					{
						if ((sVar6 == 8) || (sVar6 == 0xe))
						{
							if ((d->botData.botFlags & 2U) == 0)
							{
								if ((cDC->botFlagsPrevFrame & 2) != 0)
								{
									cDC->flags = cDC->flags | CAMERA_FLAG_RESET_RAIN_POS | CAMERA_FLAG_DIRECTION_CHANGED;
								}
								CAM_FollowDriver_AngleAxis(cDC, d, scratchpadBytes, pb->pos, pb->rot);
							}
							else
							{
								if ((cDC->botFlagsPrevFrame & 2) == 0)
								{
									cDC->flags = cDC->flags | CAMERA_FLAG_RESET_RAIN_POS | CAMERA_FLAG_DIRECTION_CHANGED;
								}
								CAM_FollowDriver_Normal(cDC, d, pb->pos, scratchpadBytes, ptrZoomData);
							}
							cDC->botFlagsPrevFrame = d->botData.botFlags;
							goto LAB_8001c150;
						}
						if ((cDC->cameraMode == 9) || (psVar21 = scratchpad, cDC->cameraMode == 0xd))
						{
							if ((gGT->level1->cnt_restart_points != 0) && ((gGT->gameMode1 & 0xf) == 0))
							{
								s16 *trackPathPos = CTR_SCRATCHPAD_PTR(s16, 0x390);
								s16 *trackPathLookaheadPos = CTR_SCRATCHPAD_PTR(s16, 0x398);
								s16 *trackPathRot = CTR_SCRATCHPAD_PTR(s16, 0x314);
								MATRIX *trackPathMatrix = CTR_SCRATCHPAD_PTR(MATRIX, 0x328);

								uVar9 = CAM_FollowDriver_TrackPath(cDC, trackPathPos, cDC->eorModeData.trackPathSpeed, 1);

								iVar7 = -0xc0;
								if (-1 < cDC->eorModeData.trackPathSpeed)
									iVar7 = 0xc0;

								uVar10 = CAM_FollowDriver_TrackPath(cDC, trackPathLookaheadPos, iVar7, 0);

								// interpolate two rotations

								iVar8 = ((int)(((uVar10 - uVar9) + 0x800U & 0xfff) - 0x800) >> 1);
								trackPathRot[0] = 0x800;
								trackPathRot[1] = (s16)uVar9 + (s16)iVar8;
								trackPathRot[2] = 0;

								// interpolate two positions
								trackPathPos[0] = (s16)(((int)trackPathPos[0] + (int)trackPathLookaheadPos[0]) >> 1);
								trackPathPos[1] = (s16)(((int)trackPathPos[1] + (int)trackPathLookaheadPos[1]) >> 1);
								trackPathPos[2] = (s16)(((int)trackPathPos[2] + (int)trackPathLookaheadPos[2]) >> 1);
								ConvertRotToMatrix(trackPathMatrix, trackPathRot);
								gte_SetRotMatrix(trackPathMatrix);
								gte_ldv0((SVECTOR *)&cDC->transitionTo);
								gte_rtv0();

								VECTOR pathOffset;
								gte_stlvnl(&pathOffset);
								uVar9 = pathOffset.vx;
								iVar7 = pathOffset.vy;
								iVar8 = pathOffset.vz;

								pb->pos[0] = trackPathPos[0] + (s16)uVar9;
								pb->pos[1] = trackPathPos[1] + (s16)iVar7;
								pb->pos[2] = trackPathPos[2] + (s16)iVar8;
								pb->rot[0] = trackPathRot[0] + cDC->transitionTo.rot[0];
								pb->rot[1] = trackPathRot[1] + cDC->transitionTo.rot[1];
								pb->rot[2] = trackPathRot[2] + cDC->transitionTo.rot[2];
							}
							psVar21 = scratchpad;
							if (cDC->cameraMode == 0xd)
								goto LAB_8001c11c;
						}
					}
					goto LAB_8001c128;
				}

				CAM_LookAtPosition(scratchpadBytes, (int *)&d->posCurr.x, &pb->pos[0], &pb->rot[0]);

				iVar7 = SquareRoot0_stub((*(int *)(scratchpadBytes + 0x24c)) * (*(int *)(scratchpadBytes + 0x24c)) +
				                         (*(int *)(scratchpadBytes + 0x254)) * (*(int *)(scratchpadBytes + 0x254)));
				iVar17 = (int)(cDC->transitionTo).pos[0];
				iVar24 = (iVar7 - (cDC->transitionTo).pos[1]) * iVar17;
				iVar8 = (int)(cDC->transitionTo).pos[2];
				iVar7 = iVar24 / iVar8;
				/*
				if (iVar8 == 0)
				{
				    trap(0x1c00);
				}
				if ((iVar8 == -1) && (iVar24 == -0x80000000))
				{
				    trap(0x1800);
				}
				*/
				bVar2 = iVar17 < iVar7;
				if (iVar7 < 0)
				{
					iVar7 = 0;
					bVar2 = iVar17 < 0;
				}
				if (bVar2)
				{
					iVar7 = iVar17;
				}
				pb->distanceToScreen_PREV = pb->distanceToScreen_CURR + iVar7;
			}

			VECTOR *cameraProbePos = (VECTOR *)(void *)(scratchpadBytes + 0x240);
			cameraProbePos->vx = (int)pb->pos[0];
			cameraProbePos->vy = (int)pb->pos[1];
			cameraProbePos->vz = (int)pb->pos[2];

			CAM_FindClosestQuadblock((struct ScratchpadStruct *)(void *)scratchpadBytes, cDC, d, cameraProbePos);
			goto LAB_8001c150;
		}
	}

	CAM_FollowDriver_Normal(cDC, d, &pb->pos[0], scratchpadBytes, ptrZoomData);

LAB_8001c150:
	cDC->cameraModePrev = cDC->cameraMode;

	if (cDC->ptrQuadBlock != 0)
	{
		psVar11 = cDC->ptrQuadBlock->pvs;
		if ((psVar11 != 0) && (piVar12 = psVar11->visLeafSrc, piVar12 != 0))
		{
			cDC->visLeafSrc = piVar12;
			gGT->unk1cac[1] = cDC->ptrQuadBlock - gGT->level1->ptr_mesh_info->ptrQuadBlockArray;
		}
		if (cDC->ptrQuadBlock != 0)
		{
			psVar11 = cDC->ptrQuadBlock->pvs;
			if ((psVar11 != 0) && (piVar12 = psVar11->visFaceSrc, piVar12 != 0))
			{
				cDC->visFaceSrc = piVar12;
			}
			if (cDC->ptrQuadBlock != 0)
			{
				psVar11 = cDC->ptrQuadBlock->pvs;
				if ((psVar11 != 0) && (ppsVar13 = psVar11->visInstSrc, ppsVar13 != 0))
				{
					cDC->visInstSrc = ppsVar13;
				}
				if (((cDC->ptrQuadBlock != 0) && (psVar11 = cDC->ptrQuadBlock->pvs, psVar11 != 0)) && (piVar12 = psVar11->visExtraSrc, piVar12 != 0))
				{
					if ((gGT->level1->configFlags & 4) == 0)
					{
						cDC->visOVertSrc = piVar12;
					}
					else
					{
						cDC->visSCVertSrc = piVar12;
					}
				}
			}
		}
	}

	if (cDC->quadBlockSearchHit == false)
	{
		cDC->visLeafSrc = 0;
		cDC->visFaceSrc = 0;
	}

	if ((cDC->flags & CAMERA_FLAG_RESET_RAIN_POS) != 0)
	{
		gGT->rainBuffer[cDC->cameraID].cameraPos[0] = pb->pos[0];
		gGT->rainBuffer[cDC->cameraID].cameraPos[1] = pb->pos[1];
		gGT->rainBuffer[cDC->cameraID].cameraPos[2] = pb->pos[2];
		cDC->flags &= ~CAMERA_FLAG_RESET_RAIN_POS;
	}
	cDC->flags &= ~(CAMERA_FLAG_DIRECTION_CHANGED | CAMERA_FLAG_FIRE_SPEED_ZOOM);

	return;
}
