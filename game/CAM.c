#include <common.h>


enum
{
	CAM_FOLLOW_DRIVER_QUAD_FLAGS_SKIP_TERRAIN_HEIGHT = 0x4100,
};

CTR_STATIC_ASSERT(CAM_FOLLOW_DRIVER_QUAD_FLAGS_SKIP_TERRAIN_HEIGHT == 0x4100);

static u32 CAM_SkyboxGlow_PrimAddr(const void *prim)
{
	return CtrGpu_PrimToOTLink24(prim);
}

static u32 CAM_SkyboxGlow_PackXY(s32 x, s32 y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

static u32 CAM_SkyboxGlow_LerpColor(u32 from, u32 to, s32 ratio)
{
	s32 r = (u8)from + ((((s32)(u8)to - (s32)(u8)from) * ratio) >> 12);
	s32 g = (u8)(from >> 8) + ((((s32)(u8)(to >> 8) - (s32)(u8)(from >> 8)) * ratio) >> 12);
	s32 b = (u8)(from >> 16) + ((((s32)(u8)(to >> 16) - (s32)(u8)(from >> 16)) * ratio) >> 12);

	return ((u32)(u8)r) | ((u32)(u8)g << 8) | ((u32)(u8)b << 16);
}

static s32 CAM_SkyboxGlow_FixedRatio(s32 numerator, s32 denominator)
{
	return (numerator << 12) / denominator;
}

static s32 CAM_SkyboxGlow_ScreenX(s32 screenWidth, s32 ratio)
{
	return (screenWidth * ratio) >> 12;
}

static s32 CAM_SkyboxGlow_Div2TowardZero(s32 value)
{
	return (value + ((u32)value >> 31)) >> 1;
}

static s32 CAM_SkyboxGlow_CalcCenterY(struct PushBuffer *pb)
{
	s32 pitch = (pb->rot.x - 0x800) * 0x78;
	s32 height = (s16)pb->rect.h;

	if (pitch < 0)
	{
		pitch += 0x3ff;
	}

	return (pitch >> 10) + CAM_SkyboxGlow_Div2TowardZero(height);
}

static s32 CAM_SkyboxGlow_CalcTilt(struct PushBuffer *pb)
{
	s32 cosine = MATH_Cos(pb->rot.z);
	if (cosine == 0)
	{
		cosine = 1;
	}

	s32 ratio = (MATH_Sin(pb->rot.z) << 12) / cosine;
	s32 shifted = (s32)((u32)ratio << 8);
	shifted >>= 12;

	return CAM_SkyboxGlow_Div2TowardZero(-shifted);
}

static void CAM_SkyboxGlow_EmitG3(struct PrimMem *primMem, uint32_t *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2)
{
	POLY_G3 *poly = primMem->cursor;

	CtrGpu_WriteColorCode(&poly->r0, CtrGpu_PackColorCode(color0, 0x30));
	CtrGpu_WritePackedXY(&poly->x0, xy0);
	CtrGpu_WriteColorCode(&poly->r1, color1 & 0xffffff);
	CtrGpu_WritePackedXY(&poly->x1, xy1);
	CtrGpu_WriteColorCode(&poly->r2, color2 & 0xffffff);
	CtrGpu_WritePackedXY(&poly->x2, xy2);
	poly->tag = CtrGpu_PackOTTag(*ot, 0x06000000);
	*ot = CAM_SkyboxGlow_PrimAddr(poly);

	primMem->cursor = poly + 1;
}

static void CAM_SkyboxGlow_EmitG4(struct PrimMem *primMem, uint32_t *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2, u32 color3, u32 xy3)
{
	POLY_G4 *poly = primMem->cursor;

	CtrGpu_WriteColorCode(&poly->r0, CtrGpu_PackColorCode(color0, 0x38));
	CtrGpu_WritePackedXY(&poly->x0, xy0);
	CtrGpu_WriteColorCode(&poly->r1, color1 & 0xffffff);
	CtrGpu_WritePackedXY(&poly->x1, xy1);
	CtrGpu_WriteColorCode(&poly->r2, color2 & 0xffffff);
	CtrGpu_WritePackedXY(&poly->x2, xy2);
	CtrGpu_WriteColorCode(&poly->r3, color3 & 0xffffff);
	CtrGpu_WritePackedXY(&poly->x3, xy3);
	poly->tag = CtrGpu_PackOTTag(*ot, 0x08000000);
	*ot = CAM_SkyboxGlow_PrimAddr(poly);

	primMem->cursor = poly + 1;
}

static void CAM_SkyboxGlow_EmitF3(struct PrimMem *primMem, uint32_t *ot, u32 color, u32 xy0, u32 xy1, u32 xy2)
{
	POLY_F3 *poly = primMem->cursor;

	CtrGpu_WriteColorCode(&poly->r0, CtrGpu_PackColorCode(color, 0x20));
	CtrGpu_WritePackedXY(&poly->x0, xy0);
	CtrGpu_WritePackedXY(&poly->x1, xy1);
	CtrGpu_WritePackedXY(&poly->x2, xy2);
	poly->tag = CtrGpu_PackOTTag(*ot, 0x04000000);
	*ot = CAM_SkyboxGlow_PrimAddr(poly);

	primMem->cursor = poly + 1;
}

static void CAM_SkyboxGlow_EmitF4(struct PrimMem *primMem, uint32_t *ot, u32 color, u32 xy0, u32 xy1, u32 xy2, u32 xy3)
{
	POLY_F4 *poly = primMem->cursor;

	CtrGpu_WriteColorCode(&poly->r0, CtrGpu_PackColorCode(color, 0x28));
	CtrGpu_WritePackedXY(&poly->x0, xy0);
	CtrGpu_WritePackedXY(&poly->x1, xy1);
	CtrGpu_WritePackedXY(&poly->x2, xy2);
	CtrGpu_WritePackedXY(&poly->x3, xy3);
	poly->tag = CtrGpu_PackOTTag(*ot, 0x05000000);
	*ot = CAM_SkyboxGlow_PrimAddr(poly);

	primMem->cursor = poly + 1;
}

static s32 CAM_SkyboxGlow_HasClearGradient(s32 gradientIndex)
{
	return gradientIndex == 0 && sdata->gGT->level1->clearColor[2].enable != 0;
}

static u32 CAM_SkyboxGlow_ClearGradientColor(void)
{
	return *(u32 *)&sdata->gGT->level1->clearColor[2].rgb[0] & 0xffffff;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800175cc-0x8001861c
void CAM_SkyboxGlow(struct SkyboxGlowGradient *grad, struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *ptrOT)
{
	s32 tilt = CAM_SkyboxGlow_CalcTilt(pb);
	s32 centerY1 = CAM_SkyboxGlow_CalcCenterY(pb);
	s32 centerY2 = CAM_SkyboxGlow_CalcCenterY(pb);
	s32 screenWidth = (s16)pb->rect.w;
	u32 screenWidthXY = (u16)screenWidth;

	for (s32 i = 0; i < 3; i++, grad++)
	{
		s32 toLeft = centerY2 - grad->pointTo - tilt;
		s32 toRight = centerY2 - grad->pointTo + tilt;
		s32 fromLeft = centerY1 - grad->pointFrom - tilt;
		s32 fromRight = centerY1 - grad->pointFrom + tilt;
		u32 colorFrom = grad->colorFrom & 0xffffff;
		u32 colorTo = grad->colorTo & 0xffffff;
		u32 mask = 0;

		if (toLeft >= 0)
		{
			mask |= 1;
		}
		if (toRight >= 0)
		{
			mask |= 2;
		}
		if (fromLeft >= 0)
		{
			mask |= 4;
		}
		if (fromRight >= 0)
		{
			mask |= 8;
		}

		switch (mask)
		{
		case 1:
		{
			s32 leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			s32 rightRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 rightX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, rightRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, leftColor, 0, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, rightX);
			break;
		}
		case 2:
		{
			s32 rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			s32 leftRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);
			u32 leftX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, leftRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, colorTo, leftX, rightColor, screenWidthXY, colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));
			break;
		}
		case 3:
		{
			s32 leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			s32 rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, leftColor, 0, rightColor, screenWidthXY, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo,
			                      CAM_SkyboxGlow_PackXY(screenWidth, toRight));
			break;
		}
		case 5:
		{
			s32 fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			s32 toRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 toX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, toRatio);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromX, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft),
			                      colorTo, toX);

			if (CAM_SkyboxGlow_HasClearGradient(i))
			{
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, 0, CAM_SkyboxGlow_PackXY(0, fromLeft));
			}
			break;
		}
		case 7:
		{
			s32 fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			s32 rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromX, rightColor, screenWidthXY);
			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), rightColor, screenWidthXY, colorTo,
			                      CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
			{
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, 0, CAM_SkyboxGlow_PackXY(0, fromLeft));
			}
			break;
		}
		case 10:
		{
			s32 fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			s32 toRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 toX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, toRatio);
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, fromX, colorFrom, fromRightXY, colorTo, toX, colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
			{
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, screenWidthXY, fromRightXY);
			}
			break;
		}
		case 11:
		{
			s32 fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			s32 leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, leftColor, 0, colorFrom, fromX, colorFrom, fromRightXY);
			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, leftColor, 0, colorFrom, fromRightXY, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo,
			                      CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
			{
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, screenWidthXY, fromRightXY);
			}
			break;
		}
		case 15:
		{
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromRightXY, colorTo,
			                      CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
			{
				CAM_SkyboxGlow_EmitF4(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), 0, screenWidthXY, CAM_SkyboxGlow_PackXY(0, fromLeft), fromRightXY);
			}
			break;
		}
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001861c-0x80018818
void CAM_ClearScreen(struct GameTracker *gGT)
{
	s8 numPlyr = gGT->numPlyrCurrGame;
	s8 swap = gGT->swapchainIndex;
	struct Level *level1 = gGT->level1;
	struct DB *backDB = gGT->backBuffer;
	TILE *tile = backDB->primMem.cursor;

	for (s32 loop = 0; loop < numPlyr; loop++)
	{
		struct PushBuffer *pb = &gGT->pushBuffer[loop];
		uint32_t *endOT = &pb->ptrOT[0x3FF];

		s16 x = pb->rect.x;
		s16 y = pb->rect.y + swap * 0x128;
		s16 w = pb->rect.w;
		s16 h = pb->rect.h;

		// cam up/down changes where the line splits.
		// At 0x800, camera looks straight, and line is perfectly midpoint
		s32 splitLine = (((s32)pb->rot.x - 0x800) >> 3) + (h >> 1);

		if (splitLine < 0)
		{
			splitLine = 0;
		}

		s32 topHeight = h;
		if (splitLine < h)
		{
			topHeight = splitLine;
		}

		// top half
		if ((level1->clearColor[0].enable != 0) && (0 < topHeight))
		{
			tile->x0 = x;
			tile->y0 = y;
			tile->w = w;
			tile->h = topHeight;

			*(s32 *)&tile->r0 = *(s32 *)&level1->clearColor[0].rgb[0];
			tile->code = 0x2;

			tile->tag = CtrGpu_PackOTTag(*(u32 *)endOT, 0x3000000);
			*(u32 *)endOT = CtrGpu_PrimToOTLink24(tile);

			tile++;
		}

		// bottom half
		if ((level1->clearColor[1].enable != 0) && (topHeight < h))
		{
			tile->x0 = x;
			tile->y0 = y + topHeight;
			tile->w = w;
			tile->h = h - topHeight;

			*(s32 *)&tile->r0 = *(s32 *)&level1->clearColor[1].rgb[0];
			tile->code = 0x2;

			tile->tag = CtrGpu_PackOTTag(*(u32 *)endOT, 0x3000000);
			*(u32 *)endOT = CtrGpu_PrimToOTLink24(tile);

			tile++;
		}
	}

	backDB->primMem.cursor = tile;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018818-0x800188a8
void CAM_Init(struct CameraDC *cDC, s32 cameraID, struct Driver *d, struct PushBuffer *pb)
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
s32 CAM_Path_GetNumPoints(void)
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
	{
		return 0;
	}

	ptrSpawnType1 = level1->ptrSpawnType1;
	if (ptrSpawnType1->count < 3)
	{
		return 0;
	}

	void **ptrs = ST1_GETPOINTERS(ptrSpawnType1);
	introCam = ptrs[ST1_CAMERA_PATH];
	if (introCam == NULL)
	{
		return 0;
	}

	while (1)
	{
		if (introCam[0] == 0)
		{
			break;
		}
		uVar4 += introCam[0];
		introCam += introCam[0] * 6 + 2;
	}

	return (s16)uVar4;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018ba0-0x80018d20
u8 CAM_Path_Move(s32 frameIndex, s16 *position, s16 *rotation, s16 *getPath)
{
	s16 frame = (s16)frameIndex;
	s16 numPos = CAM_Path_GetNumPoints();

	if (frame < 0)
	{
		return 0;
	}
	if (frame >= numPos)
	{
		return 0;
	}

	void **ptrs = ST1_GETPOINTERS(sdata->gGT->level1->ptrSpawnType1);
	s16 *ptrCam = ptrs[ST1_CAMERA_PATH];

	u16 pathNumNode = (u16)ptrCam[0];
	u16 pathID = (u16)ptrCam[1];
	s16 *move = ptrCam + 2;

	while ((s16)pathNumNode <= frame)
	{
		frame = (s16)(frame - (s16)pathNumNode);
		move = move + (s16)pathNumNode * 6;
		pathNumNode = (u16)move[0];
		pathID = (u16)move[1];
		move = move + 2;
	}

	// advance pointer to pos+rot
	move += (s32)frame * 6;

	*getPath = pathID;

	// position of frame
	position[0] = move[0];
	position[1] = move[1];
	position[2] = move[2];

	// rotation of frame
	rotation[0] = (((s16)move[3] >> 4) + 0x800U) & 0xfff;
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
	s32 hasFlyInCamera = (2 < level1->cnt_restart_points);

	if (hasFlyInCamera)
	{
		s32 flyInData = (s32)level1->ptr_restart_points;
		cDC->trackPathProgress = 0;
		cDC->transitionBlend = 0;

		cDC->transitionFrameCount = 0x1E;
		cDC->nearOrFar = 0;

		// when camera reaches player, be zoomed in
		cDC->cameraMode = 0;
		cDC->trackPathNode = (struct CheckpointNode *)(flyInData + 0x18);

		// if 1 or less screens
		cDC->transitionFrame = 0xA5;
		if (gGT->numPlyrCurrGame > 1)
		{
			cDC->transitionFrame = 1;
		}
	}

	cDC->cameraMode = 0;

	cDC->flags &= ~(CAMERA_FLAG_BATTLE_END_OF_RACE | CAMERA_FLAG_ARCADE_END_OF_RACE_ACTIVE);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018d9c-0x80018e38.
void CAM_EndOfRace_Battle(struct CameraDC *cDC, struct Driver *d)
{
	s32 height = data.Spin360_heightOffset_cameraPos[sdata->gGT->numPlyrCurrGame];
	cDC->transitionTo.pos.x = 0xffe5;
	cDC->transitionTo.pos.y = height;
	cDC->transitionTo.pos.z = 0xc0;

	cDC->flags |= CAMERA_FLAG_BATTLE_END_OF_RACE;

	cDC->transitionBlend = 0;
	cDC->transitionFrame = 60;
	cDC->transitionFrameCount = 60;

	struct PushBuffer *pb = cDC->pushBuffer;
	s32 dx = CTR_MipsSubLo(pb->pos.x, CTR_MipsSra(d->posCurr.x, 8));
	s32 dz = CTR_MipsSubLo(pb->pos.z, CTR_MipsSra(d->posCurr.z, 8));
	cDC->spin360Angle = ratan2(dx, dz);
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
void CAM_ProcessTransition(SVec3 *currPos, SVec3 *currRot, SVec3 *startPos, SVec3 *startRot, SVec3 *endPos, SVec3 *endRot, s32 frame)
{
	s32 deltaRot;

	currPos->x = startPos->x + (s16)(CAM_MulLo((s32)endPos->x - (s32)startPos->x, frame) >> 0xc);
	currPos->y = startPos->y + (s16)(CAM_MulLo((s32)endPos->y - (s32)startPos->y, frame) >> 0xc);
	currPos->z = startPos->z + (s16)(CAM_MulLo((s32)endPos->z - (s32)startPos->z, frame) >> 0xc);

	deltaRot = ((s32)endRot->x - (s32)startRot->x) & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}

	currRot->x = (startRot->x + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc)) & 0xfff;
	deltaRot = ((s32)endRot->y - (s32)startRot->y) & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot->y = (startRot->y + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc)) & 0xfff;
	deltaRot = ((s32)endRot->z - (s32)startRot->z) & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot->z = (startRot->z + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc)) & 0xfff;
}


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
void CAM_StartLine_FlyIn_FixY(SVec3 *posRot)
{
	struct ScratchpadStruct *sps = &sdata->scratchpadStruct;
	SVec3 pos;
	s32 i;

	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;

	pos.x = posRot->x;
	pos.y = posRot->y;
	pos.z = posRot->z;

	for (i = 0; i < 8; i++)
	{
		s32 probeOffset = i * 0x400;

		SVec3 posTop = {
		    .x = pos.x,
		    .y = (s16)CTR_MipsSubLo((u16)pos.y, CTR_MipsAddLo(probeOffset, 0x400)),
		    .z = pos.z,
		};
		SVec3 posBottom = {
		    .x = pos.x,
		    .y = (s16)CTR_MipsSubLo((u16)pos.y, CTR_MipsSubLo(probeOffset, 0x100)),
		    .z = pos.z,
		};

		COLL_SearchBSP_CallbackQUADBLK(&posTop, &posBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			pos = sps->Union.QuadBlockColl.hitPos;
			break;
		}
	}

	posRot->y = pos.y;
}

static s32 CAM_FollowDriver_AngleAxis_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

static s32 CAM_FollowDriver_AngleAxis_Lerp256(s32 current, s32 previous, s32 ratio)
{
	s32 currentPart = CAM_FollowDriver_AngleAxis_MulLo(0x100 - ratio, current);
	s32 previousPart = CAM_FollowDriver_AngleAxis_MulLo(ratio, previous);
	return CTR_MipsSra(CTR_MipsAddLo(currentPart, previousPart), 8);
}

static void CAM_FollowDriver_AngleAxis_LoadGteMatrix(MATRIX *axisMatrix, struct Driver *d)
{
	gte_SetRotMatrix(axisMatrix);
	gte_SetTransVector(d->instSelf->matrix.t);
}

static void CAM_FollowDriver_AngleAxis_TransformOffset(const SVec3 *offset, VECTOR *out)
{
	CTR_GteLoadSVec3V0(offset);
	gte_rtv0tr();
	CTR_GteStoreMAC(&out->vx);
}

void CAM_FollowDriver_AngleAxis(struct CameraDC *cDC, struct Driver *d, struct CameraAngleAxisScratch *scratchWork, SVec3 *pushBufferPos, SVec3 *pushBufferRot)
{
	MATRIX *axisMatrix = &scratchWork->camera.matrix;
	VECTOR *eye = (VECTOR *)&scratchWork->camera.pos;
	VECTOR lookAt;
	s32 ratio;
	s32 dx;
	s32 dy;
	s32 dz;
	s32 distanceXZ;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019128-0x800194c8.
	if (cDC->cameraMode == 0xe)
	{
		VehPhysForce_RotAxisAngle(axisMatrix, d->AxisAngle2_normalVec.v, d->angle);
	}
	else
	{
		VehPhysForce_RotAxisAngle(axisMatrix, d->AxisAngle2_normalVec.v, d->rotCurr.y);
	}

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

		eye->vx = CAM_FollowDriver_AngleAxis_Lerp256(eye->vx, pushBufferPos->x, ratio);
		eye->vy = CAM_FollowDriver_AngleAxis_Lerp256(eye->vy, pushBufferPos->y, ratio);
		eye->vz = CAM_FollowDriver_AngleAxis_Lerp256(eye->vz, pushBufferPos->z, ratio);

		cDC->lookAtPos.x = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.vx, cDC->lookAtPos.x, ratio);
		cDC->lookAtPos.y = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.vy, cDC->lookAtPos.y, ratio);
		cDC->lookAtPos.z = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.vz, cDC->lookAtPos.z, ratio);
	}

	dx = eye->vx - cDC->lookAtPos.x;
	dy = eye->vy - cDC->lookAtPos.y;
	dz = eye->vz - cDC->lookAtPos.z;

	scratchWork->camera.dir.x = dx;
	scratchWork->camera.dir.y = dy;
	scratchWork->camera.dir.z = dz;

	pushBufferRot->y = (s16)ratan2(dx, dz);
	distanceXZ = SquareRoot0_stub(CTR_MipsAddLo(CAM_FollowDriver_AngleAxis_MulLo(dx, dx), CAM_FollowDriver_AngleAxis_MulLo(dz, dz)));
	pushBufferRot->x = 0x800 - (s16)ratan2(dy, distanceXZ);
	pushBufferRot->z = 0;

	pushBufferPos->x = (s16)eye->vx;
	pushBufferPos->y = (s16)eye->vy;
	pushBufferPos->z = (s16)eye->vz;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800194c8-0x800198f8.
void CAM_StartLine_FlyIn(struct FlyInData *flyInData, s16 maxFrames, s32 frame, SVec3 *desiredPos, SVec3 *desiredRot)
{
	struct Level *lev = sdata->gGT->level1;
	s32 frameIndex = (frame << 0x10) >> 4;
	s32 frameRatio = frameIndex / maxFrames;
	s32 countEnd = flyInData->frameCount1;
	s16 count = flyInData->frameCount2;
	SVECTOR local_78;
	SVECTOR local_70;
	SVec3 rot;
	MATRIX matrix;
	VECTOR transformed;
	s32 flags[2];
	s16 *pathEnd;
	s16 *pathStart;
	s32 pathRatioEnd;

	if (count < countEnd)
	{
		count = countEnd;
	}

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

	s32 ratio = count * pathRatioEnd & 0xfff;
	local_78.vx = pathEnd[0] + (s16)(((pathEnd[3] - pathEnd[0]) * ratio) >> 0xc);
	local_78.vy = pathEnd[1] + (s16)(((pathEnd[4] - pathEnd[1]) * ratio) >> 0xc);
	local_78.vz = pathEnd[2] + (s16)(((pathEnd[5] - pathEnd[2]) * ratio) >> 0xc);

	ratio = count * frameRatio & 0xfff;
	local_70.vx = pathStart[0] + (s16)(((pathStart[3] - pathStart[0]) * ratio) >> 0xc);
	local_70.vy = pathStart[1] + (s16)(((pathStart[4] - pathStart[1]) * ratio) >> 0xc) - 0x60;
	local_70.vz = pathStart[2] + (s16)(((pathStart[5] - pathStart[2]) * ratio) >> 0xc);

	rot.x = lev->DriverSpawn[0].rot.x;
	rot.y = lev->DriverSpawn[0].rot.y + 0x400;
	rot.z = lev->DriverSpawn[0].rot.z;

	ConvertRotToMatrix(&matrix, &rot);

	CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[1].pos);
	CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[2].pos);
	CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[5].pos);

	matrix.t[0] = lev->DriverSpawn[1].pos.x + (lev->DriverSpawn[2].pos.x - lev->DriverSpawn[1].pos.x) / 2;
	matrix.t[1] = lev->DriverSpawn[1].pos.y + (lev->DriverSpawn[2].pos.y - lev->DriverSpawn[1].pos.y) / 2 + 0x40;
	matrix.t[2] = lev->DriverSpawn[1].pos.z + (lev->DriverSpawn[2].pos.z - lev->DriverSpawn[1].pos.z) / 2;

	SetRotMatrix(&matrix);
	SetTransMatrix(&matrix);

	RotTrans(&local_78, &transformed, flags);
	desiredPos->x = (s16)transformed.vx;
	desiredPos->y = (s16)transformed.vy;
	desiredPos->z = (s16)transformed.vz;

	RotTrans(&local_70, &transformed, flags);

	s16 deltaX = desiredPos->x - (s16)transformed.vx;
	s16 deltaY = desiredPos->y - (s16)transformed.vy;
	s16 deltaZ = desiredPos->z - (s16)transformed.vz;

	desiredRot->y = (s16)ratan2(deltaX, deltaZ);
	desiredRot->x = 0x800 - (s16)ratan2(deltaY, SquareRoot0(deltaX * deltaX + deltaZ * deltaZ));
	desiredRot->z = 0;
}

static s32 CAM_FollowDriver_TrackPath_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

static struct CheckpointNode *CAM_FollowDriver_TrackPath_GetNode(struct CameraDC *cDC, struct CheckpointNode *node, s32 speed)
{
	struct CheckpointNode *nodes = sdata->gGT->level1->ptr_restart_points;
	u8 nodeIndex;

	if (speed > 0)
	{
		nodeIndex = node->nextIndex_backward;
		if ((cDC->flags & CAMERA_FLAG_TRACK_PATH_ALT_BRANCH) != 0 && node->nextIndex_right != 0xff)
		{
			nodeIndex = node->nextIndex_right;
		}
	}
	else
	{
		nodeIndex = node->nextIndex_forward;
		if ((cDC->flags & CAMERA_FLAG_TRACK_PATH_ALT_BRANCH) != 0 && node->nextIndex_left != 0xff)
		{
			nodeIndex = node->nextIndex_left;
		}
	}

	return &nodes[nodeIndex];
}

static s32 CAM_FollowDriver_TrackPath_Length(struct CheckpointNode *from, struct CheckpointNode *to, s32 *dx, s32 *dy, s32 *dz)
{
	*dx = to->pos.x - from->pos.x;
	*dy = to->pos.y - from->pos.y;
	*dz = to->pos.z - from->pos.z;

	s32 sum = CTR_MipsAddLo(CAM_FollowDriver_TrackPath_MulLo(*dx, *dx), CAM_FollowDriver_TrackPath_MulLo(*dy, *dy));
	return SquareRoot0_stub(CTR_MipsAddLo(sum, CAM_FollowDriver_TrackPath_MulLo(*dz, *dz)));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800198f8-0x80019e7c.
u32 CAM_FollowDriver_TrackPath(struct CameraDC *cDC, SVec3 *pos, s32 speed, s32 update)
{
	struct CheckpointNode *curr = cDC->trackPathNode;
	struct CheckpointNode *next = CAM_FollowDriver_TrackPath_GetNode(cDC, curr, speed);
	struct CheckpointNode *angleNext;
	s32 pathProgress;
	s32 segmentLength;
	s32 ratio;
	s32 dx;
	s32 dy;
	s32 dz;
	s32 yaw;
	s32 nextYaw;
	s32 yawDelta;

	segmentLength = CAM_FollowDriver_TrackPath_Length(curr, next, &dx, &dy, &dz);

	if ((sdata->gGT->gameMode1 & PAUSE_ALL) != 0)
	{
		pathProgress = 0;
	}
	else if (speed > 0)
	{
		pathProgress = cDC->trackPathProgress + speed;
	}
	else
	{
		pathProgress = cDC->trackPathProgress - speed;
	}

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
	{
		ratio = (pathProgress << 12) / segmentLength;
	}
	else
	{
		ratio = 0;
	}

	pos->x = curr->pos.x + (s16)((CAM_FollowDriver_TrackPath_MulLo(dx, ratio)) >> 12);
	pos->y = curr->pos.y + (s16)((CAM_FollowDriver_TrackPath_MulLo(dy, ratio)) >> 12) + 0x80;
	pos->z = curr->pos.z + (s16)((CAM_FollowDriver_TrackPath_MulLo(dz, ratio)) >> 12);

	yaw = (s16)ratan2(dx, dz) + 0x800;
	angleNext = CAM_FollowDriver_TrackPath_GetNode(cDC, next, speed);
	nextYaw = (s16)ratan2(angleNext->pos.x - next->pos.x, angleNext->pos.z - next->pos.z) + 0x800;

	yawDelta = (nextYaw - yaw) & 0xfff;
	if (yawDelta >= 0x800)
	{
		yawDelta -= 0x1000;
	}

	return (yaw + (CAM_FollowDriver_TrackPath_MulLo(yawDelta, ratio) >> 12)) & 0xfff;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019e7c-0x80019f58
void CAM_LookAtPosition(struct CameraScratchWork *scratchWork, Vec3 *positions, SVec3 *desiredPos, SVec3 *desiredRot)
{
	struct CameraScratch *cam = &scratchWork->camera;

	s32 dirX = desiredPos->x - (positions->x >> 8);
	s32 dirY = desiredPos->y - ((positions->y >> 8) + data.Spin360_heightOffset_driverPos[sdata->gGT->numPlyrCurrGame]);
	s32 dirZ = desiredPos->z - (positions->z >> 8);

	cam->dir.x = dirX;
	cam->dir.y = dirY;
	cam->dir.z = dirZ;

	s32 distance = SquareRoot0_stub(CAM_MulLo(dirX, dirX) + CAM_MulLo(dirZ, dirZ));

	// rotations
	desiredRot->x = 0x800 - (s16)ratan2(dirY, distance);
	desiredRot->y = (s16)ratan2(dirX, dirZ);
	desiredRot->z = 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019f58-0x8001a054
void CAM_FollowDriver_Spin360(struct CameraDC *cDC, struct CameraScratchWork *scratchWork, struct Driver *d, SVec3 *desiredPos, SVec3 *desiredRot)
{
	s32 ratio;

	// Not really "transitionTo" but the variables
	// are shared with other camera modes, therefore
	// need a union with proper names for each mode

	// rotate other way for odd number
	if ((d->driverID & 1) != 0)
	{
		cDC->spin360Angle -= cDC->transitionTo.pos.x;
	}
	else
	{
		// rotate one way
		cDC->spin360Angle += cDC->transitionTo.pos.x;
	}

	s32 angle = cDC->spin360Angle;
	ratio = MATH_Sin(angle);
	desiredPos->x = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posCurr.x, 8), CTR_MipsSra(CAM_MulLo(ratio, cDC->transitionTo.pos.z), 0xc));

	ratio = MATH_Cos(angle);
	desiredPos->z = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posCurr.z, 8), CTR_MipsSra(CAM_MulLo(ratio, cDC->transitionTo.pos.z), 0xc));

	desiredPos->y = (s16)CTR_MipsAddLo((u16)cDC->transitionTo.pos.y, CTR_MipsSra(d->posCurr.y, 8));

	CAM_LookAtPosition(scratchWork, &d->posCurr, desiredPos, desiredRot);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001a054-0x8001a0bc
void CAM_SetDesiredPosRot(struct CameraDC *cDC, const SVec3 *pos, const SVec3 *rot)
{
	cDC->transitionTo.pos = *pos;
	cDC->transitionTo.rot = *rot;

	// 1 second, 30 frames
	cDC->transitionFrameCount = 0x1e;

	cDC->transitionFrame = 0;
	cDC->transitionBlend = 0x1000;

	cDC->flags |= CAMERA_FLAG_TRANSITION_AWAY;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001a0bc-0x8001b254.
void CAM_FollowDriver_Normal(struct CameraDC *cDC, struct Driver *d, SVec3 *pushBufferPos, struct CameraScratchWork *scratchWork, struct ZoomData *zoom)
{
	struct PushBuffer *pb = (struct PushBuffer *)pushBufferPos;
	struct ScratchpadStruct *sps = (struct ScratchpadStruct *)scratchWork;
	struct CameraScratch *cam = &scratchWork->camera;
	struct GameTracker *gGT = sdata->gGT;
	struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[d->driverID];
	s8 state;
	s16 uVar8;
	u16 uVar9;
	s16 sVar10;
	u32 backupFlags;
	s32 x;
	s32 x_00;
	u32 uVar11;
	s32 iVar12;
	u32 uVar13;
	s32 iVar14;
	SVec3 local_40;
	SVec3 local_38;
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
		cDC->flags |= CAMERA_FLAG_DIRECTION_CHANGED;
	}

	// 0 = forwards
	// 0x800 = backwards
	sVar10 = ((cDC->flags & CAMERA_FLAG_REVERSE) != 0) * 0x800;

	// if camera angle was not just changed
	if ((cDC->flags & CAMERA_FLAG_DIRECTION_CHANGED) == 0)
	{
		// absolute value driver speed
		x = (s32)d->speedApprox;

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
		cDC->cameraMoveSpeed = (s32)(uVar11 * cDC->cameraMoveSpeed + (0x100 - uVar13) * x) >> 8;
	}

	// if camera angle changed
	else
	{
		// absolute value driver speed
		x = (s32)d->speedApprox;
		if (x < 0)
		{
			x = -x;
		}

		cDC->cameraMoveSpeed = x;
	}

	uVar8 = 0;
	if (gGT->numPlyrCurrGame != 2)
	{
		uVar8 = 0xff9c;
	}

	cam->rot.x = uVar8;

	// 0x20e
	// camera RotY
	cam->rot.y = (d->rotCurr.w + d->angle + 0x800 + sVar10) & 0xfff;

	cam->rot.z = cDC->desiredRot.x * -2;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix(&cam->matrix, &cam->rot);

	if (((cDC->flags & CAMERA_FLAG_FIRE_SPEED_ZOOM) != 0) && (x = (s32)((u32)d->fireSpeedCap << 0x10) >> 0x14, cDC->fireSpeedZoom.timer < x))
	{
		cDC->fireSpeedZoom.timer = x;
	}
	cam->rot.x = 0;
	cam->rot.y = 0;

	uVar8 = VehCalc_MapToRange(cDC->cameraMoveSpeed, (s32)zoom->speedMin, (s32)zoom->speedMax, (s32)zoom->distMin, (s32)zoom->distMax);

	cam->rot.z = uVar8;

	if (cDC->fireSpeedZoom.timer == 0)
	{
		if (cDC->fireSpeedZoom.distanceOffset != 0)
		{
			cDC->fireSpeedZoom.distanceOffset -= gGT->elapsedTimeMS * 0x10;

			if (cDC->fireSpeedZoom.distanceOffset < 0)
			{
				cDC->fireSpeedZoom.distanceOffset = 0;
			}
		}
	}
	else
	{
		cDC->fireSpeedZoom.distanceOffset += gGT->elapsedTimeMS * 0x40;
		if (cDC->fireSpeedZoom.distanceOffset > 0x6000)
		{
			cDC->fireSpeedZoom.distanceOffset = 0x6000;
		}

		cDC->fireSpeedZoom.timer -= gGT->elapsedTimeMS;
		if (cDC->fireSpeedZoom.timer < 0)
		{
			cDC->fireSpeedZoom.timer = 0;
		}
	}
	cam->rot.z += cDC->fireSpeedZoom.distanceOffset >> 8;

	gte_SetRotMatrix(&cam->matrix);
	CTR_GteLoadSVec3V0(&cam->rot);
	gte_rtv0();
	CTR_GteStoreMAC(cam->pos.v);

	cam->rot.x = 0;
	cam->rot.y = 0x40;
	cam->rot.z = 0;

	CTR_GteLoadSVec3V0(&cam->rot);
	gte_rtv0();
	CTR_GteStoreMAC(scratchWork->sideOffset.v);

	cam->delta.x = CTR_MipsSra(d->posCurr.x, 8);
	cam->delta.y = CTR_MipsSra(d->posCurr.y, 8);
	cam->delta.z = CTR_MipsSra(d->posCurr.z, 8);

	cam->delta.x += scratchWork->sideOffset.x;
	cam->delta.z += scratchWork->sideOffset.z;
	cam->pos.x += cam->delta.x;


	// mask-grab
	if ((cDC->flags & CAMERA_FLAG_MASK_GRAB) != 0)
	{
		cam->pos.y = CTR_MipsSra(d->quadBlockHeight, 8) + (s32)cDC->maskGrabHeightOffset + (s32)zoom->vertDistance;
	}

	else
	{
		cam->pos.y += cam->delta.y + (s32)zoom->vertDistance;
	}

	cam->pos.z += cam->delta.z;

	uVar8 = 0;
	if (gGT->numPlyrCurrGame != 2)
	{
		uVar8 = 0xff9c;
	}

	// rotX
	cam->rot.x = uVar8;

	// rotZ
	cam->rot.z = 0;

	// rotY
	cam->rot.y = (d->rotCurr.w + d->angle + d->turnAngleCurr + 0x800 + sVar10) & 0xfff;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix(&cam->matrix, &cam->rot);

	// if racer is not damaged,
	// slight-down view angle
	if ((d->actionsFlagSet & ACTION_WARP) == 0)
	{
		cDC->damagePitchOffset -= 8;
		if (cDC->damagePitchOffset < -0x20)
		{
			cDC->damagePitchOffset = -0x20;
		}
	}

	// if racer is damaged,
	// straight-forward angle
	else
	{
		cDC->damagePitchOffset += 8;
		if (cDC->damagePitchOffset > 0)
		{
			cDC->damagePitchOffset = 0;
		}
	}

	// Z, Y, X
	cam->rot.z = cDC->damagePitchOffset;
	cam->rot.y = 0;
	cam->rot.x = 0;

	gte_SetRotMatrix(&cam->matrix);

	CTR_GteLoadSVec3V0(&cam->rot);
	gte_rtv0();
	CTR_GteStoreS16Triplet(cam->rot.v);

	cam->delta.x += (s32)cam->rot.x;
	cam->delta.z += (s32)cam->rot.z;
	cam->delta.y += (s32)cam->rot.y + (s32)zoom->angle[2];

	cDC->desiredRot.x = ((zoom->angle[1] * (s32)cDC->desiredRot.x) + ((0x100 - (s32)zoom->angle[1]) * (s32)d->rotCurr.z)) >> 8;


	state = d->kartState;

	if (state == KS_MASK_GRABBED)
	{
		// pushBuffer position
		cam->pos.x = (s32)pb->pos.x;
		cam->pos.y = (s32)pb->pos.y;
		cam->pos.z = (s32)pb->pos.z;

		cDC->heightSmoothing.startOffset = 0;
		cDC->heightSmoothing.framesRemaining = 0;
		cDC->heightSmoothing.currentOffset = 0;
	}

	if (state == KS_ENGINE_REVVING)
	{
		cDC->heightSmoothing.startOffset = 0;
		cDC->heightSmoothing.framesRemaining = 0;
		cDC->heightSmoothing.currentOffset = 0;
	}

	if ((d->kartState != KS_BLASTED) && ((d->actionsFlagSet & ACTION_TOUCH_GROUND) != 0) && (cDC->BlastedLerp.boolLerpPending != 0))
	{
		cDC->BlastedLerp.boolLerpPending = 0;

		cDC->BlastedLerp.desiredRot.x = cDC->lookAtPos.x - *(s16 *)&cam->delta.x;
		cDC->BlastedLerp.desiredRot.y = cDC->lookAtPos.y - *(s16 *)&cam->delta.y;
		cDC->BlastedLerp.desiredRot.z = cDC->lookAtPos.z - *(s16 *)&cam->delta.z;

		cDC->BlastedLerp.desiredPos.x = cDC->cameraPos.x - *(s16 *)&cam->pos.x;
		cDC->BlastedLerp.desiredPos.y = cDC->cameraPos.y - *(s16 *)&cam->pos.y;
		cDC->BlastedLerp.desiredPos.z = cDC->cameraPos.z - *(s16 *)&cam->pos.z;

		cDC->BlastedLerp.framesRemaining = 8;
	}

	// if not arcade end-of-race
	if (((cDC->flags & CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED) == 0) && (cDC->cameraMode == 0))
	{
		if ((d->kartState != KS_BLASTED) && (cDC->BlastedLerp.boolLerpPending == 0))
		{
			goto LAB_8001a8c0;
		}

		if (cDC->BlastedLerp.boolLerpPending == 0)
		{
			*(s16 *)((s32)cDC + 0xc8) = cDC->lookAtPos.y - cDC->cameraPos.y;
			*(s16 *)((s32)cDC + 0xca) = cDC->cameraPos.y - CTR_MipsSra(d->quadBlockHeight, 8);
		}

		cDC->BlastedLerp.boolLerpPending = 1;

		if ((cDC->cameraPos.y < cam->pos.y) && (x = (s32) * (s16 *)((s32)cDC + 0xca) + CTR_MipsSra(d->quadBlockHeight, 8), x < cam->pos.y))
		{
			cam->pos.y = x;
		}

	LAB_8001a8b0:

		if (cDC->BlastedLerp.boolLerpPending == 0)
		{
			goto LAB_8001a8c0;
		}
	}

	// if this is arcade end-of-race
	else
	{
		if (cDC->BlastedLerp.boolLerpPending != 0)
		{
			cDC->BlastedLerp.boolLerpPending = 0;

			cDC->BlastedLerp.desiredRot.x = cDC->lookAtPos.x - *(s16 *)&cam->delta.x;
			cDC->BlastedLerp.desiredRot.y = cDC->lookAtPos.y - *(s16 *)&cam->delta.y;
			cDC->BlastedLerp.desiredRot.z = cDC->lookAtPos.z - *(s16 *)&cam->delta.z;

			cDC->BlastedLerp.desiredPos.x = cDC->cameraPos.x - *(s16 *)&cam->pos.x;
			cDC->BlastedLerp.desiredPos.y = cDC->cameraPos.y - *(s16 *)&cam->pos.y;
			cDC->BlastedLerp.desiredPos.z = cDC->cameraPos.z - *(s16 *)&cam->pos.z;

			cDC->BlastedLerp.framesRemaining = 8;

			goto LAB_8001a8b0;
		}

	LAB_8001a8c0:

		// if frame countdown is not finished
		if (cDC->BlastedLerp.framesRemaining != 0)
		{
			cam->pos.x += (cDC->BlastedLerp.desiredPos.x * cDC->BlastedLerp.framesRemaining) >> 3;
			cam->pos.y += (cDC->BlastedLerp.desiredPos.y * cDC->BlastedLerp.framesRemaining) >> 3;
			cam->pos.z += (cDC->BlastedLerp.desiredPos.z * cDC->BlastedLerp.framesRemaining) >> 3;

			cam->delta.x += (cDC->BlastedLerp.desiredRot.x * cDC->BlastedLerp.framesRemaining) >> 3;
			cam->delta.y += (cDC->BlastedLerp.desiredRot.y * cDC->BlastedLerp.framesRemaining) >> 3;
			cam->delta.z += (cDC->BlastedLerp.desiredRot.z * cDC->BlastedLerp.framesRemaining) >> 3;

			// decrease frame countdown
			cDC->BlastedLerp.framesRemaining--;
		}
	}

	CAM_FindClosestQuadblock(sps, cDC, d, (VECTOR *)&cam->pos);

	struct QuadBlock *quad = sps->hit.ptrQuadblock;
	if ((sps->boolDidTouchQuadblock == 0) || ((quad->quadFlags & CAM_FOLLOW_DRIVER_QUAD_FLAGS_SKIP_TERRAIN_HEIGHT) != 0))
	{
		if (cam->pos.y < (s32)cDC->heightSmoothing.currentOffset + CTR_MipsSra(d->posCurr.y, 8))
		{
			cDC->heightSmoothing.framesRemaining = 8;
			cDC->heightSmoothing.startOffset = cDC->heightSmoothing.currentOffset;
			cam->pos.y = (s32)cDC->heightSmoothing.currentOffset + CTR_MipsSra(d->posCurr.y, 8);

			goto LAB_8001ab04;
		}

		cDC->heightSmoothing.framesRemaining = 8;
		cDC->heightSmoothing.startOffset = *(s16 *)&cam->pos.y - (s16)CTR_MipsSra(d->posCurr.y, 8);
	}

	else
	{
		state = (s8)quad->terrain_type;

		// Mud, Water, or FastWater
		if (((state == 0xe) || (state == 4)) || (state == 0xd))
		{
			scratchWork->terrainHeightFloor = 0;
		}

		x = (s32)scratchWork->terrainHeightFloor + (s32)zoom->vertDistance;
		if (cam->pos.y < x)
		{
			cam->pos.y = x;
		}

		x = cDC->heightSmoothing.framesRemaining;
		if (x != 0)
		{
			cam->pos.y =

			    ((8 - x) * cam->pos.y + x * ((s32)cDC->heightSmoothing.startOffset + CTR_MipsSra(d->posCurr.y, 8))) >> 3;

			cDC->heightSmoothing.framesRemaining += -1;
		}
	}
	cDC->heightSmoothing.currentOffset = *(s16 *)&cam->pos.y - CTR_MipsSra(d->posCurr.y, 8);
LAB_8001ab04:

	// if mask grabs you when you're underwater
	if (((gGT->level1->configFlags & 2) != 0) && (cam->pos.y < zoom->vertDistance))
	{
		cam->pos.y = zoom->vertDistance;
	}

	if (cDC->BlastedLerp.boolLerpPending != 0)
	{
		cam->delta.y = cam->pos.y + (s32) * (s16 *)((s32)cDC + 0xc8);
	}

	if (d->kartState == KS_MASK_GRABBED)
	{
		pb->rot.z -= (pb->rot.z >> 3);

		// camera dirX, cameraPosX minus driverPosX
		cam->dir.x = (s32)pb->pos.x - CTR_MipsSra(d->posCurr.x, 8);

		// camera dirY, cameraPosY minus driverPosY, plus something else
		cam->dir.y = (s32)pb->pos.y - (CTR_MipsSra(d->posCurr.y, 8) + (s32)zoom->angle[2]);

		// camera dirZ, cameraPosZ minus driverPosZ
		cam->dir.z = (s32)pb->pos.z - CTR_MipsSra(d->posCurr.z, 8);

		if (pb->rot.x < 0x800)
		{
			pb->rot.x += 0x10;
			if (pb->rot.x > 0x800)
			{
				pb->rot.x = 0x800;
			}
		}
	}

	// if not mask grab
	else
	{
		x = cam->pos.z - cam->delta.z;

		// camera direction
		cam->dir.z = x;
		cam->dir.x = cam->pos.x - cam->delta.x;
		cam->dir.y = cam->pos.y - cam->delta.y;

		// camera rotation
		x_00 = ratan2(cam->dir.x, x);
		pb->rot.y = (s16)x_00;

		x_00 = SquareRoot0_stub(CTR_MipsAddLo(CAM_MulLo(cam->dir.x, cam->dir.x), CAM_MulLo(cam->dir.z, cam->dir.z)));

		x_00 = ratan2(cam->dir.y, x_00);
		pb->rot.x = 0x800 - (s16)x_00;

		pb->rot.z = (s16)CTR_MipsSra(CAM_MulLo((s32)zoom->angle[0], (s32)cDC->desiredRot.x), 8);
	}

	// something with pushBuffer position
	cam->posCopy.x = cam->pos.x - (s32)pb->pos.x;
	cam->posCopy.y = cam->pos.y - (s32)pb->pos.y;
	cam->posCopy.z = cam->pos.z - (s32)pb->pos.z;

	cDC->pushBufferPosCorrection.x -= (cam->pos.x - cDC->cameraPos.x);
	cDC->pushBufferPosCorrection.y -= (cam->pos.y - cDC->cameraPos.y);
	cDC->pushBufferPosCorrection.z -= (cam->pos.z - cDC->cameraPos.z);

	if (cDC->pushBufferPosCorrection.x > 2)
	{
		cDC->pushBufferPosCorrection.x = 2;
	}
	if (cDC->pushBufferPosCorrection.y > 2)
	{
		cDC->pushBufferPosCorrection.y = 2;
	}
	if (cDC->pushBufferPosCorrection.z > 2)
	{
		cDC->pushBufferPosCorrection.z = 2;
	}

	if (cDC->pushBufferPosCorrection.x < -2)
	{
		cDC->pushBufferPosCorrection.x = -2;
	}
	if (cDC->pushBufferPosCorrection.y < -2)
	{
		cDC->pushBufferPosCorrection.y = -2;
	}
	if (cDC->pushBufferPosCorrection.z < -2)
	{
		cDC->pushBufferPosCorrection.z = -2;
	}

	if (d->kartState != KS_MASK_GRABBED)
	{
		// pushBuffer position
		pb->pos.x += *(s16 *)&cam->posCopy.x + cDC->pushBufferPosCorrection.x;
		pb->pos.y += *(s16 *)&cam->posCopy.y + cDC->pushBufferPosCorrection.y;
		pb->pos.z += *(s16 *)&cam->posCopy.z + cDC->pushBufferPosCorrection.z;
	}

	cDC->cameraPos.x = cam->pos.x;
	cDC->cameraPos.y = cam->pos.y;
	cDC->cameraPos.z = cam->pos.z;
	cDC->lookAtPos.x = cam->delta.x;
	cDC->lookAtPos.y = cam->delta.y;
	cDC->lookAtPos.z = cam->delta.z;

	// backup flags (again)
	backupFlags = cDC->flags;

	cDC->flags &= ~CAMERA_FLAG_MASK_GRAB;

	if (
	    // transitioning, end-race battle, intro-race
	    ((backupFlags & (CAMERA_FLAG_TRANSITION_AWAY | CAMERA_FLAG_BATTLE_END_OF_RACE)) == 0) && ((gGT->gameMode1 & START_OF_RACE) == 0))
	{
		return;
	}


	// if end-of-race battle
	if ((backupFlags & CAMERA_FLAG_BATTLE_END_OF_RACE) != 0)
	{
		CAM_FollowDriver_Spin360(cDC, scratchWork, d, &local_40, &local_38);

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
			local_40.x = cDC->transitionTo.pos.x;
			local_40.y = cDC->transitionTo.pos.y;
			local_40.z = cDC->transitionTo.pos.z;

			local_38.x = cDC->transitionTo.rot.x;
			local_38.y = cDC->transitionTo.rot.y;
			local_38.z = cDC->transitionTo.rot.z;

			// interpolate fly-in [0 to 0x1000]
			x = cDC->transitionBlend;
		}

		// if startline camera
		else
		{
			struct SpawnType1 *st1 = gGT->level1->ptrSpawnType1;
			void **pointers = ST1_GETPOINTERS(st1);
			u8 *cameraPath = pointers[ST1_CAMERA_PATH];
			s32 flyInDone = 0;

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
				flyInData.ptrEnd = cameraPath + 0x354;
				flyInData.ptrStart = cameraPath;
				flyInData.frameCount1 = 0x96;
				flyInData.frameCount2 = 0x8e;

				// which frame of fly-in you are in
				x = 0xa5 - (u16)cDC->transitionFrame;

				if ((s16)x > 0x96)
				{
					x = 0x96;
				}

				CAM_StartLine_FlyIn(&flyInData, 0x96, x, &local_40, &local_38);

				x = (s32)cDC->transitionBlend;
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
	CAM_ProcessTransition(&pb->pos, &pb->rot, &local_40, &local_38, &pb->pos, &pb->rot, x);

	cam->pos.x = (s32)pb->pos.x;
	cam->pos.y = (s32)pb->pos.y;
	cam->pos.z = (s32)pb->pos.z;

	CAM_FindClosestQuadblock((struct ScratchpadStruct *)scratchWork, cDC, d, (VECTOR *)&cam->pos);

	x = cDC->transitionFrameCount;
	iVar14 = cDC->transitionFrameCount;

	if (iVar14 != 0)
	{
		iVar12 = (s32)cDC->transitionFrame;

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
		{
			return;
		}

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
}

s32 CAM_MapRange_PosPoints(SVec3 *pos1, SVec3 *pos2, SVec3 *currPos)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001b254-0x8001b334.
	SVec3 pathDelta;
	pathDelta.x = pos1->x - pos2->x;
	pathDelta.y = pos1->y - pos2->y;
	pathDelta.z = pos1->z - pos2->z;

	MATH_VectorNormalize(&pathDelta);

	SVec3 currDelta;
	currDelta.x = currPos->x - pos1->x;
	currDelta.y = currPos->y - pos1->y;
	currDelta.z = currPos->z - pos1->z;

	CTC2(CTR_PackS16Pair(pathDelta.x, pathDelta.y), 0);
	CTC2((s32)pathDelta.z, 1);
	CTR_GteLoadSVec3V0(&currDelta);
	gte_mvmva(0, 0, 0, 3, 0);

	return MFC2_S(25) >> 12;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001b334-0x8001c360.
void CAM_ThTick(struct Thread *t)
{
	s16 sVar1;
	s32 bVar2;
	u16 uVar4;
	s16 sVar5;
	s16 sVar6;
	s32 iVar7;
	s32 iVar8;
	u32 uVar9;
	u32 uVar10;
	struct PVS *psVar11;
	s32 *piVar12;
	struct Instance **ppsVar13;
	struct SpawnType1 *psVar14;
	struct CheckpointNode *psVar15;
	u32 uVar16;
	s32 iVar17;
	s32 iVar18;
	s16 *psVar19;
	s16 *psVar20;
	s16 *psVar21;
	u32 uVar22;
	struct CameraDC *cDC;
	struct PushBuffer *pb;
	struct ZoomData *ptrZoomData;
	struct Driver *d;
	s16 *scratchpad;
	s32 iVar24;
	s32 iVar25;

	struct GameTracker *gGT = sdata->gGT;
	struct CameraScratchWork *scratchWork = CTR_SCRATCHPAD_PTR(struct CameraScratchWork, 0x108);
	scratchpad = (s16 *)scratchWork;
	struct CameraScratch *camThTick = &scratchWork->camera;
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
	{
		ptrZoomData = &data.NearCam8x3;
	}

	ptrZoomData = &ptrZoomData[cDC->nearOrFar * 2];

	if ((cDC->flags & CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED) == 0)
	{
		goto SkipNewCameraEOR;
	}

	psVar14 = gGT->level1->ptrSpawnType1;

	psVar21 = 0;
	if (psVar14->count < 3)
	{
		goto SkipNewCameraEOR;
	}

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
			iVar7 = (s32)*psVar19;
			if (iVar7 < 0)
			{
				iVar7 = -iVar7;
			}

			// respawnPoint
			uVar16 = (u32)*psVar20;

			// +2 to include respawnPoint and modeID
			psVar20 = (s16 *)((s32)psVar19 + data.EndOfRace_Camera_Size[iVar7] + 2);

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
	{
		goto SkipNewCameraEOR;
	}

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
		pb->pos.x = *(s16 *)(&d->instSelf->matrix.t[0]);
		pb->pos.y = *(s16 *)(&d->instSelf->matrix.t[1]);
		pb->pos.z = *(s16 *)(&d->instSelf->matrix.t[2]);
		pb->rot.x = d->rotCurr.x;
		pb->rot.y = d->rotCurr.y;
		pb->rot.z = d->rotCurr.z;
		cDC->heightSmoothing.startOffset = 0;
		break;
	case 3:
		pb->pos.x = *psVar19;
		pb->pos.y = psVar21[2];
		pb->pos.z = psVar21[3];
		pb->rot.x = psVar21[4];
		pb->rot.y = psVar21[5];
		pb->rot.z = psVar21[6];
		*(s16 *)&cDC->action = *psVar19;
		*(s16 *)((s32)&cDC->action + 2) = psVar21[2];
		*(s16 *)&cDC->mode = psVar21[3];
		*(s16 *)&cDC->unk0xC = psVar21[4];
		*(s16 *)((s32)&cDC->unk0xC + 2) = psVar21[5];
		cDC->desiredRot.x = psVar21[6];
		break;
	case 4:
		pb->pos.x = *psVar19;
		pb->pos.y = psVar21[2];
		pb->pos.z = psVar21[3];
		break;
	case 7:
		(cDC->transitionTo).pos.x = *psVar19;
		sVar6 = psVar21[2];
		uVar22 = cDC->flags & ~CAMERA_FLAG_TRACK_PATH_FACE_DRIVER;
		cDC->flags = uVar22;
		(cDC->transitionTo).pos.y = sVar6;
		if (psVar21[3] != 0)
		{
			cDC->flags = uVar22 | CAMERA_FLAG_TRACK_PATH_FACE_DRIVER;
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

		iVar7 = VehCalc_MapToRange((s32)sVar6 * (s32)sVar6 + (s32)sVar5 * (s32)sVar5 + (s32)sVar1 * (s32)sVar1, 0x10000, 0x190000, 0x80, 0xf0);

		cDC->angleAxisLerpRatio = (s16)iVar7;
		break;
	case 9:
	case 13:
		sVar6 = *psVar19;
		psVar15 = gGT->level1->ptr_restart_points;
		cDC->trackPathProgress = 0;
		cDC->trackPathNode = psVar15 + sVar6;
		(cDC->transitionTo).pos.x = psVar21[2];
		(cDC->transitionTo).pos.y = psVar21[3];
		(cDC->transitionTo).pos.z = psVar21[4];
		(cDC->transitionTo).rot.x = psVar21[5];
		(cDC->transitionTo).rot.y = psVar21[6];
		(cDC->transitionTo).rot.z = psVar21[7];
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
		pb->pos.x = *psVar19;
		pb->pos.y = psVar21[2];
		pb->pos.z = psVar21[3];
		psVar19 = psVar21 + 5;
		sVar6 = psVar21[4] - sVar6;
	LAB_8001b928:
		(cDC->transitionTo).pos.x = sVar6;
		(cDC->transitionTo).pos.y = *psVar19;
		(cDC->transitionTo).pos.z = psVar19[1];
		break;
	case 12:
		(cDC->transitionTo).pos.x = *psVar19;
		(cDC->transitionTo).pos.y = psVar21[2];
		(cDC->transitionTo).pos.z = psVar21[3];
		(cDC->transitionTo).rot.x = psVar21[4];
		(cDC->transitionTo).rot.y = psVar21[5];
		(cDC->transitionTo).rot.z = psVar21[6];
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
				CAM_LookAtPosition(scratchWork, &d->posCurr, &pb->pos, &pb->rot);
				psVar21 = scratchpad;
			LAB_8001c128:
				scratchpad = psVar21;
			}
			else
			{
				psVar21 = scratchpad;
				if (sVar6 == 10)
				{
					CAM_FollowDriver_Spin360(cDC, scratchWork, d, &pb->pos, &pb->rot);
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

						SVec3 stackMemPos;

						stackMemPos.x = (s16)CTR_MipsSra(d->posCurr.x, 8);
						stackMemPos.y = (s16)CTR_MipsSra(d->posCurr.y, 8);
						stackMemPos.z = (s16)CTR_MipsSra(d->posCurr.z, 8);

						iVar8 = CAM_MapRange_PosPoints(&cDC->transitionTo.pos, &cDC->transitionTo.rot, &stackMemPos);

						iVar17 = (s32)cDC->eorModeData.pointPath.speed;

						stackMemPos.x = cDC->eorModeData.pointPath.endPos.x - (cDC->transitionTo).rot.x;
						stackMemPos.y = cDC->eorModeData.pointPath.endPos.y - (cDC->transitionTo).rot.y;
						stackMemPos.z = cDC->eorModeData.pointPath.endPos.z - (cDC->transitionTo).rot.z;

						iVar7 = iVar17;
						if (iVar17 < 0)
						{
							iVar7 = -iVar17;
						}
						if (iVar17 < 0)
						{
							stackMemPos.x = -stackMemPos.x;
							stackMemPos.y = -stackMemPos.y;
							stackMemPos.z = -stackMemPos.z;
						}

						iVar24 = SquareRoot0_stub(
						    CTR_MipsAddLo(CTR_MipsAddLo(CAM_MulLo((s32)stackMemPos.x, (s32)stackMemPos.x), CAM_MulLo((s32)stackMemPos.y, (s32)stackMemPos.y)),
						                  CAM_MulLo((s32)stackMemPos.z, (s32)stackMemPos.z)));

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
						psVar21 = (cDC->transitionTo).rot.v;
						if (iVar17 < 0)
						{
							psVar21 = cDC->eorModeData.pointPath.endPos.v;
						}
						pb->pos.x = psVar21[0] + (s16)((stackMemPos.x * iVar25) >> 0xc);
						pb->pos.y = psVar21[1] + (s16)((stackMemPos.y * iVar25) >> 0xc);
						pb->pos.z = psVar21[2] + (s16)((stackMemPos.z * iVar25) >> 0xc);
						goto LAB_8001c11c;
					}
					if (sVar6 == 7)
					{
						pb->pos.x = (s16)CTR_MipsSra(d->posCurr.x, 8);
						pb->pos.y = (cDC->transitionTo).pos.x + (s16)CTR_MipsSra(d->posCurr.y, 8);
						pb->pos.z = (s16)CTR_MipsSra(d->posCurr.z, 8);
						sVar6 = (cDC->transitionTo).pos.y;
						pb->rot.y = 0;
						pb->rot.z = 0;
						pb->rot.x = sVar6 + 0x400;
						psVar21 = scratchpad;
						if ((cDC->flags & CAMERA_FLAG_TRACK_PATH_FACE_DRIVER) != 0)
						{
							pb->rot.y = d->angle + 0x800;
						}
					}
					else if ((u16)(sVar5 - 0xfU) < 2)
					{
						pb->pos.x = sdata->FirstPersonCamera.posOffset.x + (s16)CTR_MipsSra(d->posCurr.x, 8);
						pb->pos.y = sdata->FirstPersonCamera.posOffset.y + (s16)CTR_MipsSra(d->posCurr.y, 8);
						pb->pos.z = sdata->FirstPersonCamera.posOffset.z + (s16)CTR_MipsSra(d->posCurr.z, 8);

						pb->rot.x = sdata->FirstPersonCamera.rotOffset.x + (d->rotCurr).x;
						if (cDC->cameraMode == 0x10)
						{
							pb->rot.y = sdata->FirstPersonCamera.rotOffset.y + d->angle;
						}
						else
						{
							pb->rot.y = sdata->FirstPersonCamera.rotOffset.y + (d->rotCurr).y;
						}
						pb->rot.z = sdata->FirstPersonCamera.rotOffset.z + (d->rotCurr).z;
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
								CAM_FollowDriver_AngleAxis(cDC, d, &scratchWork->angleAxis, &pb->pos, &pb->rot);
							}
							else
							{
								if ((cDC->botFlagsPrevFrame & 2) == 0)
								{
									cDC->flags = cDC->flags | CAMERA_FLAG_RESET_RAIN_POS | CAMERA_FLAG_DIRECTION_CHANGED;
								}
								CAM_FollowDriver_Normal(cDC, d, &pb->pos, scratchWork, ptrZoomData);
							}
							cDC->botFlagsPrevFrame = d->botData.botFlags;
							goto LAB_8001c150;
						}
						if ((cDC->cameraMode == 9) || (psVar21 = scratchpad, cDC->cameraMode == 0xd))
						{
							if ((gGT->level1->cnt_restart_points != 0) && ((gGT->gameMode1 & PAUSE_ALL) == 0))
							{
								SVec3 *trackPathPos = &scratchWork->trackPathPos;
								SVec3 *trackPathLookaheadPos = &scratchWork->trackPathLookaheadPos;


								uVar9 = CAM_FollowDriver_TrackPath(cDC, trackPathPos, cDC->eorModeData.trackPathSpeed, 1);

								iVar7 = -0xc0;
								if (-1 < cDC->eorModeData.trackPathSpeed)
								{
									iVar7 = 0xc0;
								}

								uVar10 = CAM_FollowDriver_TrackPath(cDC, trackPathLookaheadPos, iVar7, 0);

								// interpolate two rotations

								iVar8 = ((s32)((((uVar10 - uVar9) + 0x800U) & 0xfff) - 0x800) >> 1);
								camThTick->rot.x = 0x800;
								camThTick->rot.y = (s16)uVar9 + (s16)iVar8;
								camThTick->rot.z = 0;

								// interpolate two positions
								trackPathPos->x = (s16)(((s32)trackPathPos->x + (s32)trackPathLookaheadPos->x) >> 1);
								trackPathPos->y = (s16)(((s32)trackPathPos->y + (s32)trackPathLookaheadPos->y) >> 1);
								trackPathPos->z = (s16)(((s32)trackPathPos->z + (s32)trackPathLookaheadPos->z) >> 1);
								ConvertRotToMatrix(&camThTick->matrix, &camThTick->rot);
								gte_SetRotMatrix(&camThTick->matrix);
								CTR_GteLoadSVec3V0(&cDC->transitionTo.pos);
								gte_rtv0();

								VECTOR pathOffset;
								CTR_GteStoreMAC(&pathOffset.vx);
								uVar9 = pathOffset.vx;
								iVar7 = pathOffset.vy;
								iVar8 = pathOffset.vz;

								pb->pos.x = trackPathPos->x + (s16)uVar9;
								pb->pos.y = trackPathPos->y + (s16)iVar7;
								pb->pos.z = trackPathPos->z + (s16)iVar8;
								pb->rot.x = camThTick->rot.x + cDC->transitionTo.rot.x;
								pb->rot.y = camThTick->rot.y + cDC->transitionTo.rot.y;
								pb->rot.z = camThTick->rot.z + cDC->transitionTo.rot.z;
							}
							psVar21 = scratchpad;
							if (cDC->cameraMode == 0xd)
							{
								goto LAB_8001c11c;
							}
						}
					}
					goto LAB_8001c128;
				}

				CAM_LookAtPosition(scratchWork, &d->posCurr, &pb->pos, &pb->rot);

				iVar7 = SquareRoot0_stub(CTR_MipsAddLo(CAM_MulLo(camThTick->dir.x, camThTick->dir.x), CAM_MulLo(camThTick->dir.z, camThTick->dir.z)));
				iVar17 = (s32)(cDC->transitionTo).pos.x;
				iVar24 = (iVar7 - (cDC->transitionTo).pos.y) * iVar17;
				iVar8 = (s32)(cDC->transitionTo).pos.z;
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

			VECTOR *cameraProbePos = (VECTOR *)&camThTick->pos;
			cameraProbePos->vx = (s32)pb->pos.x;
			cameraProbePos->vy = (s32)pb->pos.y;
			cameraProbePos->vz = (s32)pb->pos.z;

			CAM_FindClosestQuadblock((struct ScratchpadStruct *)scratchWork, cDC, d, cameraProbePos);
			goto LAB_8001c150;
		}
	}

	CAM_FollowDriver_Normal(cDC, d, &pb->pos, scratchWork, ptrZoomData);

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
		gGT->rainBuffer[cDC->cameraID].cameraPos = pb->pos;
		cDC->flags &= ~CAMERA_FLAG_RESET_RAIN_POS;
	}
	cDC->flags &= ~(CAMERA_FLAG_DIRECTION_CHANGED | CAMERA_FLAG_FIRE_SPEED_ZOOM);
}
