#include <common.h>


enum
{
	CAM_FOLLOW_DRIVER_QUAD_FLAGS_SKIP_TERRAIN_HEIGHT = 0x4100,
};


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
	// NOTE(aalhendi): Clipping can have negative numerators; preserve the
	// CPU's wrapping shift without signed-left-shift undefined behavior.
	return CTR_MipsSll(numerator, 12) / denominator;
}

static s32 CAM_SkyboxGlow_ScreenX(s32 screenWidth, s32 ratio)
{
	return (screenWidth * ratio) >> 12;
}

static s32 CAM_SkyboxGlow_Div2TowardZero(s32 value)
{
	// prevent implicit unsigned promotion by casting to s32 before the rightsift -penta3
	return (s32)(value + ((u32)value >> 31)) >> 1;
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

	{
		s32 ratio = CTR_MipsSll(MATH_Sin(pb->rot.z), 12) / cosine;
		s32 shifted = (s32)((u32)ratio << 8);
		shifted >>= 12;

		return CAM_SkyboxGlow_Div2TowardZero(-shifted);
	}
}

static void CAM_SkyboxGlow_EmitG3(struct PrimMem *primMem, u32 *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2)
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

static void CAM_SkyboxGlow_EmitG4(struct PrimMem *primMem, u32 *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2, u32 color3, u32 xy3)
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

static void CAM_SkyboxGlow_EmitF3(struct PrimMem *primMem, u32 *ot, u32 color, u32 xy0, u32 xy1, u32 xy2)
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

static void CAM_SkyboxGlow_EmitF4(struct PrimMem *primMem, u32 *ot, u32 color, u32 xy0, u32 xy1, u32 xy2, u32 xy3)
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

void CAM_SkyboxGlow(struct SkyboxGlowGradient *grad, struct PushBuffer *pb, struct PrimMem *primMem, u32 *ptrOT)
{
	s32 tilt = CAM_SkyboxGlow_CalcTilt(pb);
	s32 centerY1 = CAM_SkyboxGlow_CalcCenterY(pb);
	s32 centerY2 = CAM_SkyboxGlow_CalcCenterY(pb);
	s32 screenWidth = (s16)pb->rect.w;
	u32 screenWidthXY = (u16)screenWidth;

	{
		s32 i;
		for (i = 0; i < 3; i++, grad++)
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

				CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromX, colorTo,
				                      CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, toX);

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

				CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, fromX, colorFrom, fromRightXY, colorTo, toX, colorTo,
				                      CAM_SkyboxGlow_PackXY(screenWidth, toRight));

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
					CAM_SkyboxGlow_EmitF4(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), 0, screenWidthXY, CAM_SkyboxGlow_PackXY(0, fromLeft),
					                      fromRightXY);
				}
				break;
			}
			}
		}
	}
}

void CAM_ClearScreen(struct GameTracker *gGT)
{
	TILE *tile = gGT->backBuffer->primMem.cursor;

	{
		s32 loop;
		for (loop = 0; loop < gGT->numPlyrCurrGame; loop++)
		{
			struct PushBuffer *pb = &gGT->pushBuffer[loop];

			register s32 w CTR_PSX_REGISTER("$11") = pb->rect.w;
			s32 x = pb->rect.x;
			s32 y = pb->rect.y;
			s32 h = pb->rect.h;

			// Camera pitch moves the split around the midpoint at 0x800.
			s32 pitchShift = ((s32)pb->rot.x - 0x800) >> 3;
			s32 halfHeight = h / 2;
			register s32 splitLine CTR_PSX_REGISTER("$6") = pitchShift + halfHeight;

			register s32 clampedSplitLine CTR_PSX_REGISTER("$4") = (splitLine < 0) ? 0 : splitLine;
			// NOTE(aalhendi): Keep retail input/result lifetimes distinct;
			// native evaluates these as ordinary C values.
			CTR_PSX_DEPEND_VALUE(splitLine, pitchShift);
			CTR_PSX_DEPEND_VALUE(splitLine, halfHeight);
			CTR_PSX_DEPEND_VALUE(clampedSplitLine, splitLine);
			CTR_PSX_OBSERVE_VALUE(w);

			{
				s32 minHeight = (clampedSplitLine < h) ? clampedSplitLine : h;
				register s32 topHeight CTR_PSX_REGISTER("$6") = minHeight;
				u32 *endOT;
				CTR_PSX_DEPEND_VALUE(topHeight, minHeight);

				endOT = &pb->ptrOT[0x3FF];

				// NOTE(aalhendi): PushBuffer_Init supplies nonnegative X/width.
				// Native writes aligned packet words within the complete TILE.
				// OT heads contain only packed 24-bit links.

				// top half
				if ((gGT->level1->clearColor[0].enable != 0) && (0 < topHeight))
				{
					s32 swap = gGT->swapchainIndex;

					CTR_WriteU32AlignedLE((u8 *)tile + offsetof(TILE, w), (u32)w | ((u32)topHeight << 16));
					CTR_WriteU32AlignedLE((u8 *)tile + offsetof(TILE, x0), (u32)x | ((u32)(y + swap * 0x128) << 16));

					CTR_WriteU32AlignedLE((u8 *)tile + offsetof(TILE, r0), (u32)(u8)gGT->level1->clearColor[0].rgb[0] |
					                                                           ((u32)(u8)gGT->level1->clearColor[0].rgb[1] << 8) |
					                                                           ((u32)(u8)gGT->level1->clearColor[0].rgb[2] << 16) | 0x02000000u);

					tile->tag = *endOT | 0x03000000u;
					*endOT = CtrGpu_PrimToOTLink24(tile);

					tile++;
				}

				// bottom half
				if ((gGT->level1->clearColor[1].enable != 0) && (topHeight < h))
				{
					s32 swap = gGT->swapchainIndex;

					CTR_WriteU32AlignedLE((u8 *)tile + offsetof(TILE, w), (u32)w | ((u32)(h - topHeight) << 16));
					CTR_WriteU32AlignedLE((u8 *)tile + offsetof(TILE, x0), (u32)x | ((u32)(y + swap * 0x128 + topHeight) << 16));

					CTR_WriteU32AlignedLE((u8 *)tile + offsetof(TILE, r0), (u32)(u8)gGT->level1->clearColor[1].rgb[0] |
					                                                           ((u32)(u8)gGT->level1->clearColor[1].rgb[1] << 8) |
					                                                           ((u32)(u8)gGT->level1->clearColor[1].rgb[2] << 16) | 0x02000000u);

					tile->tag = *endOT | 0x03000000u;
					*endOT = CtrGpu_PrimToOTLink24(tile);

					tile++;
				}
			}
			// Preserve width after the bottom packet word is built.
			CTR_PSX_OBSERVE_VALUE(w);
		}
	}

	gGT->backBuffer->primMem.cursor = tile;
}

void CAM_Init(struct CameraDC *cDC, s32 cameraID, struct Driver *d, struct PushBuffer *pb)
{
	PROC_BirthWithObject(0x30f, CAM_ThTick, GAME_CAMERA_THREAD_NAME, NULL)->inst = (struct Instance *)cDC;

	memset(cDC, 0, sizeof(struct CameraDC));

	// needed or L2 breaks
	cDC->cameraID = cameraID;

	cDC->driverToFollow = d;
	cDC->pushBuffer = pb;

	// NOTE(aalhendi): Retail writes this after memset; the explicit store is
	// part of the camera initialization sequence.
	cDC->cameraMode = 0;

	cDC->flags |= CAMERA_FLAG_DIRECTION_CHANGED;
}

s32 CAM_Path_GetNumPoints(void)
{
	struct GameTracker *gGT;
	struct Level *level1;
	struct SpawnType1 *ptrSpawnType1;
	s16 *introCam;
	u16 totalFrames;
	u16 nodeCount;
	s32 sentinel;

	totalFrames = 0;

	gGT = GAME_TRACKER;
	level1 = gGT->level1;
	if (level1 == NULL)
	{
		goto done;
	}

	ptrSpawnType1 = level1->ptrSpawnType1;
	if (ptrSpawnType1->count < 3)
	{
		goto done;
	}

	{
		void **ptrs = ST1_GETPOINTERS(ptrSpawnType1);
		introCam = ptrs[ST1_CAMERA_PATH];
		if (introCam == NULL)
		{
			goto done;
		}

		goto checkNode;
		while (1)
		{
			totalFrames += nodeCount;
			{
				register s32 signedCount CTR_PSX_REGISTER("$2");

				// NOTE(aalhendi): The unsigned sum and signed packet stride have distinct retail register lifetimes.
				signedCount = (s16)nodeCount;
				CTR_PSX_OBSERVE_VALUE(signedCount);
				introCam += signedCount * 6;
			}
		checkNode:
			sentinel = introCam[0];
			// NOTE(aalhendi): Retail tests the signed header, then reloads its unsigned count before advancing past both header words.
			CTR_PSX_DEPEND_MEMORY(introCam, sentinel);
			nodeCount = ((u16 *)introCam)[0];
			introCam += 2;
			if (sentinel == 0)
			{
				break;
			}
		}

	done:
		return (s16)totalFrames;
	}
}

u8 CAM_Path_Move(s32 frameIndex, s16 *position, s16 *rotation, s16 *pathFlagsOut)
{
	s16 frame = (s16)frameIndex;
	s16 numPos = CAM_Path_GetNumPoints();

	if (frame < 0)
	{
		goto failed;
	}
	if (frame >= numPos)
	{
		goto failed;
	}

	{
		void **ptrs = ST1_GETPOINTERS(GAME_TRACKER->level1->ptrSpawnType1);
		u16 *cursor = ptrs[ST1_CAMERA_PATH];
		u16 pathNumNode = *cursor++;
		u16 pathFlags = *cursor++;

		while ((s16)pathNumNode <= frame)
		{
			frame = (s16)(frame - (s16)pathNumNode);
			// NOTE(aalhendi): The unsigned header remains live across the signed frame subtraction and payload stride.
			CTR_PSX_FORGET_VALUE(pathNumNode);
			cursor = cursor + (s16)pathNumNode * 6;
			pathNumNode = *cursor++;
			pathFlags = *cursor++;
		}

		// advance pointer to pos+rot
		{
			// NOTE(aalhendi): Keep the retail frame base, signed index and output cursor distinct; native uses the same C accesses without register
			// constraints.
			register u16 *frameData CTR_PSX_REGISTER("$4") = cursor;
			register s32 sampleIndex CTR_PSX_REGISTER("$2");
			register u16 *sample CTR_PSX_REGISTER("$3");
			CTR_PSX_DEPEND_VALUE(frame, frameData);
			sampleIndex = frame;
			CTR_PSX_OBSERVE_VALUE(sampleIndex);
			sample = frameData + sampleIndex * 6;
			CTR_PSX_OBSERVE_VALUE(sample);

			*pathFlagsOut = pathFlags;

			// position of frame
			position[0] = sample[0];
			position[1] = sample[1];
			position[2] = sample[2];

			// rotation of frame
			sample += 3;
			rotation[0] = (((s16)sample[0] >> 4) + 0x800U) & 0xfff;
			rotation[1] = (u16)sample[1] >> 4;
			rotation[2] = (u16)sample[2] >> 4;
			return 1;
		}
	}
failed:
	return 0;
}

void CAM_StartOfRace(struct CameraDC *cDC)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct Level *level1 = gGT->level1;

	// Initialize the fly-in when the level has camera restart data.
	if (2 < level1->cnt_restart_points)
	{
		s32 flyInData = (s32)level1->ptr_restart_points;
		cDC->trackPathProgress = 0;
		cDC->transitionBlend = 0;

		cDC->transitionFrameCount = 0x1E;
		cDC->nearOrFar = 0;

		// when camera reaches player, be zoomed in
		cDC->cameraMode = 0;
		cDC->trackPathNode = (struct CheckpointNode *)(flyInData + 0x18);

		// Split-screen skips most of the fly-in.
		if (gGT->numPlyrCurrGame > 1)
		{
			cDC->transitionFrame = 1;
		}
		else
		{
			cDC->transitionFrame = 0xA5;
		}
	}

	cDC->cameraMode = 0;

	cDC->flags &= ~(CAMERA_FLAG_BATTLE_END_OF_RACE | CAMERA_FLAG_ARCADE_END_OF_RACE_ACTIVE);
}

void CAM_EndOfRace_Battle(struct CameraDC *cDC, struct Driver *d)
{
	u16 height;
	struct PushBuffer *pb;
	s32 dx;
	s32 dz;

	cDC->transitionTo.pos.x = 0xffe5;
	cDC->flags |= CAMERA_FLAG_BATTLE_END_OF_RACE;
	// NOTE(aalhendi): Each height occupies a 32-bit table slot; the camera consumes its low halfword.
	height = (u16)GAME_BATTLE_CAMERA_HEIGHTS[(s32)GAME_TRACKER->numPlyrCurrGame];
	cDC->transitionTo.pos.z = 0xc0;
	cDC->transitionTo.pos.y = height;

	pb = cDC->pushBuffer;
	dx = CTR_MipsSubLo(pb->pos.x, CTR_MipsSra(d->posCurr.x, 8));
	dz = CTR_MipsSubLo(pb->pos.z, CTR_MipsSra(d->posCurr.z, 8));
	cDC->spin360Angle = ratan2(dx, dz);

	cDC->transitionBlend = 0;
	cDC->transitionFrame = 60;
	cDC->transitionFrameCount = 60;
}

void CAM_EndOfRace(struct CameraDC *cDC, struct Driver *d)
{
	struct GameTracker *gGT = GAME_TRACKER;


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
}

static s32 CAM_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

void CAM_ProcessTransition(SVec3 *currPos, SVec3 *currRot, SVec3 *startPos, SVec3 *startRot, SVec3 *endPos, SVec3 *endRot, s32 frame)
{
	s32 deltaRot;

	currPos->x = startPos->x + (s16)(CTR_MipsMulLo((s32)endPos->x - (s32)startPos->x, frame) >> 0xc);
	currPos->y = startPos->y + (s16)(CTR_MipsMulLo((s32)endPos->y - (s32)startPos->y, frame) >> 0xc);
	currPos->z = startPos->z + (s16)(CTR_MipsMulLo((s32)endPos->z - (s32)startPos->z, frame) >> 0xc);

	deltaRot = ((s32)endRot->x - (s32)startRot->x) & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}

	currRot->x = (startRot->x + (s16)(CTR_MipsMulLo(deltaRot, frame) >> 0xc)) & 0xfff;
	deltaRot = ((s32)endRot->y - (s32)startRot->y) & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot->y = (startRot->y + (s16)(CTR_MipsMulLo(deltaRot, frame) >> 0xc)) & 0xfff;
	deltaRot = ((s32)endRot->z - (s32)startRot->z) & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot->z = (startRot->z + (s16)(CTR_MipsMulLo(deltaRot, frame) >> 0xc)) & 0xfff;
}


void CAM_FindClosestQuadblock(struct ScratchpadStruct *sps, struct CameraDC *cDC, struct Driver *d, const Vec3 *pos)
{
	struct QuadBlock *quad;
	register u16 inputX CTR_PSX_REGISTER("$4");
	register struct CameraDC *incomingCamera CTR_PSX_REGISTER("$5");
	register struct CameraDC *camera CTR_PSX_REGISTER("$17");
	s16 minXResult;

	(void)d;
	incomingCamera = cDC;

	// NOTE(aalhendi): Retail consumes the low halfword of each VECTOR lane.
	{
		sps->Union.QuadBlockColl.pos.x = (s16)pos->x;
		sps->Union.QuadBlockColl.pos.y = (s16)pos->y;
		sps->Union.QuadBlockColl.pos.z = (s16)pos->z;

		sps->Input1.pos.x = (s16)pos->x;
		sps->Input1.pos.y = (s16)pos->y;
		// NOTE(aalhendi): Keep the halfword reload and incoming/saved camera
		// lifetimes distinct; retail copies the camera after reading input X.
		CTR_PSX_RELOAD(sps->Input1.pos.x);
		inputX = (u16)sps->Input1.pos.x;
		CTR_PSX_DEPEND_VALUE(incomingCamera, inputX);
		camera = incomingCamera;
		CTR_PSX_DEPEND_VALUE(pos, incomingCamera);
		CTR_PSX_DEPEND_VALUE(pos, camera);
		CTR_PSX_DEPEND_VALUE(pos, d);
		sps->Input1.pos.z = (s16)pos->z;
		sps->Input1.pos.y -= 0x800;
		sps->Union.QuadBlockColl.hitPos.x = inputX;
		sps->Union.QuadBlockColl.pos.y = (s16)CTR_MipsAddLo((u16)sps->Union.QuadBlockColl.pos.y, 0x100);
		sps->Union.QuadBlockColl.hitPos.y = sps->Input1.pos.y;
		sps->Union.QuadBlockColl.hitPos.z = sps->Input1.pos.z;

		minXResult = sps->Input1.pos.x < sps->Union.QuadBlockColl.pos.x ? sps->Input1.pos.x : sps->Union.QuadBlockColl.pos.x;
		// NOTE(aalhendi): Keep X through its signed minimum, then release it
		// before the Y-bound load. Native keeps the same values and stores.
		CTR_PSX_DEPEND_VALUE(camera, inputX);
		CTR_PSX_DEPEND_VALUE(sps, camera);
		sps->bbox.min.x = minXResult;
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
		camera->quadBlockSearchHit = false;

		if ((GAME_TRACKER->level1 != NULL) && (GAME_TRACKER->level1->ptr_mesh_info != NULL) && (GAME_TRACKER->level1->ptr_mesh_info->bspRoot != NULL))
		{
			sps->ptr_mesh_info = GAME_TRACKER->level1->ptr_mesh_info;

			if (camera->ptrQuadBlock != NULL)
			{
				COLL_FIXED_QUADBLK_TestTriangles(camera->ptrQuadBlock, sps);
			}

			if (sps->boolDidTouchQuadblock == 0)
			{
				COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
			}

			if (sps->boolDidTouchQuadblock != 0)
			{
				camera->quadBlockSearchHit = true;
				quad = sps->hit.ptrQuadblock;
				camera->ptrQuadBlock = quad;
				GAME_TRACKER->unk1cac[0] = quad - GAME_TRACKER->level1->ptr_mesh_info->ptrQuadBlockArray;
			}
			else
			{
				GAME_TRACKER->unk1cac[0] = -1;
			}
		}
		else
		{
			sps->ptr_mesh_info = NULL;
		}
	}
}

struct ScratchpadStruct *CAM_StartLine_FlyIn_FixY(SVec3 *posRot)
{
	struct ScratchpadStruct *setupSps = &GAME_COLLISION_WORK;
	struct ScratchpadStruct *sps;
	struct mesh_info *meshInfo = GAME_TRACKER->level1->ptr_mesh_info;
	SVec3 pos;
	SVec3 posTop;
	SVec3 posBottom;
	s16 i;

	setupSps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;
	setupSps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	setupSps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	setupSps->ptr_mesh_info = meshInfo;

	pos.x = posRot->x;
	pos.y = posRot->y;
	pos.z = posRot->z;

	// NOTE(aalhendi): Separate setup and query lifetimes preserve retail allocation.
	i = 0;
	sps = &GAME_COLLISION_WORK;
	for (; i < 8; i++)
	{
		s32 probeOffset = i * 0x400;

		// NOTE(aalhendi): Probe endpoints wrap as halfwords, including negative Y.
		u16 x = (u16)pos.x;
		u16 y = (u16)pos.y;
		u16 z = (u16)pos.z;
		u16 topY = (u16)CTR_MipsSubLo(y, CTR_MipsAddLo(probeOffset, 0x400));
		u16 bottomY = (u16)CTR_MipsSubLo(y, CTR_MipsSubLo(probeOffset, 0x100));

		posTop.x = x;
		posTop.y = topY;
		posTop.z = z;
		posBottom.x = x;
		posBottom.y = bottomY;
		posBottom.z = z;

		COLL_SearchBSP_CallbackQUADBLK(&posTop, &posBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			pos.x = sps->Union.QuadBlockColl.hitPos.x;
			pos.y = sps->Union.QuadBlockColl.hitPos.y;
			pos.z = sps->Union.QuadBlockColl.hitPos.z;
			break;
		}
	}

	posRot->y = pos.y;
	return &GAME_COLLISION_WORK;
}

#include "CAM/CAM_FollowDriver_AngleAxis.c"
#include "CAM/CAM_StartLine_FlyIn.c"

#include "CAM/CAM_FollowDriver_TrackPath.c"

void CAM_LookAtPosition(struct CameraScratchWork *scratchWork, Vec3 *positions, SVec3 *desiredPos, SVec3 *desiredRot)
{
	// NOTE(aalhendi): Publish the scratch lanes before yaw, then reload them
	// for distance and pitch. This preserves retail call order and lifetimes.
	scratchWork->camera.dir.x = desiredPos->x - (positions->x >> 8);
	scratchWork->camera.dir.y = CTR_MipsSubLo(desiredPos->y, CTR_MipsAddLo(positions->y >> 8, GAME_SPIN360_DRIVER_HEIGHTS[(s32)GAME_TRACKER->numPlyrCurrGame]));
	scratchWork->camera.dir.z = desiredPos->z - (positions->z >> 8);
	desiredRot->y = (s16)ratan2(scratchWork->camera.dir.x, scratchWork->camera.dir.z);

	{
		s32 distance = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsMulLo(scratchWork->camera.dir.x, scratchWork->camera.dir.x),
		                                              CTR_MipsMulLo(scratchWork->camera.dir.z, scratchWork->camera.dir.z)));

		// rotations
		desiredRot->x = 0x800 - (s16)ratan2(scratchWork->camera.dir.y, distance);
		desiredRot->z = 0;
	}
}

void CAM_FollowDriver_Spin360(struct CameraDC *cDC, struct CameraScratchWork *scratchWork, struct Driver *d, SVec3 *desiredPos, SVec3 *desiredRot)
{
	s32 ratio;
	s32 angle = cDC->spin360Angle;
	s32 nextAngle;

	// NOTE(aalhendi): transitionTo.pos holds the spin step, height and radius.
	// Keep the signed angle and result separate, then publish/reload the
	// low halfword. Publish Y before the trig calls, as retail does.

	// rotate other way for odd number
	if ((d->driverID & 1) != 0)
	{
		nextAngle = angle - cDC->transitionTo.pos.x;
	}
	else
	{
		// rotate one way
		nextAngle = angle + cDC->transitionTo.pos.x;
	}
	cDC->spin360Angle = (s16)nextAngle;

	desiredPos->y = (s16)CTR_MipsAddLo((u16)cDC->transitionTo.pos.y, CTR_MipsSra(d->posCurr.y, 8));
	ratio = MATH_Sin(cDC->spin360Angle);
	desiredPos->x = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posCurr.x, 8), CTR_MipsSra(CTR_MipsMulLo(ratio, cDC->transitionTo.pos.z), 0xc));

	ratio = MATH_Cos(cDC->spin360Angle);
	desiredPos->z = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posCurr.z, 8), CTR_MipsSra(CTR_MipsMulLo(ratio, cDC->transitionTo.pos.z), 0xc));

	CAM_LookAtPosition(scratchWork, &d->posCurr, desiredPos, desiredRot);
}

void CAM_SetDesiredPosRot(struct CameraDC *cDC, const SVec3 *pos, const SVec3 *rot)
{
	// NOTE(aalhendi): Preserve position-then-rotation halfword transfer order.
	cDC->transitionTo.pos.x = pos->x;
	cDC->transitionTo.pos.y = pos->y;
	cDC->transitionTo.pos.z = pos->z;
	cDC->transitionTo.rot.x = rot->x;
	cDC->transitionTo.rot.y = rot->y;
	cDC->transitionTo.rot.z = rot->z;

	// 1 second, 30 frames
	cDC->transitionFrameCount = 0x1e;

	cDC->transitionFrame = 0;
	cDC->transitionBlend = 0x1000;

	cDC->flags |= CAMERA_FLAG_TRANSITION_AWAY;
}

void CAM_FollowDriver_Normal(struct CameraDC *cDC, struct Driver *d, SVec3 *pushBufferPos, struct CameraScratchWork *scratchWork, struct ZoomData *zoom)
{
	// NOTE(aalhendi): Use pushBufferPos directly instead of a local pb copy;
	// a separate local shortens pb's live range and swaps retail's s3/s4 (d/pb)
	// allocation in GCC 2.8.1 global.c.
#define pb ((struct PushBuffer *)pushBufferPos)
	s16 uVar8;
	s32 sVar10;
	u32 backupFlags;
	s32 blend;
	SVec3 local_40;
	SVec3 local_38;
	struct FlyInData flyInData;

	// backup flags
	{
		u32 origFlags = cDC->flags;

		// disable Reverse Cam flag,
		// assuming you dont hold R2
		cDC->flags &= ~CAMERA_FLAG_REVERSE;

		if (
		    // If this is human and not AI
		    ((d->actionsFlagSet & ACTION_BOT) == 0) &&

		    // If not drawing intro-race cutscene
		    ((GAME_TRACKER->gameMode1 & START_OF_RACE) == 0) &&

		    // If you are holding R2
		    ((GAMEPADS->gamepad[d->driverID].buttonsHeldCurrFrame & 0x200) != 0))
		{
			// Reverse the camera
			cDC->flags |= CAMERA_FLAG_REVERSE;
		}

		// if camera just changed direction
		// (either forward to reverse, or reverse to forward)
		if (origFlags != cDC->flags)
		{
			cDC->flags |= CAMERA_FLAG_DIRECTION_CHANGED;
		}
	}

	// 0 = forwards
	// 0x800 = backwards
	sVar10 = ((cDC->flags & CAMERA_FLAG_REVERSE) != 0) * 0x800;

	// if camera angle changed
	if ((cDC->flags & CAMERA_FLAG_DIRECTION_CHANGED) != 0)
	{
		// absolute value driver speed
		cDC->cameraMoveSpeed = abs(d->speedApprox);
	}

	// if camera angle was not just changed
	else
	{
		// absolute value driver speed
		s32 speed = abs(d->speedApprox);

		// driver speed slower than camera speed
		if (speed < cDC->cameraMoveSpeed)
		{
			cDC->cameraMoveSpeed = (s32)((u32)zoom->percentage1 * cDC->cameraMoveSpeed + (0x100 - (u32)zoom->percentage2) * speed) >> 8;
		}
		else
		{
			cDC->cameraMoveSpeed = (s32)((u32)zoom->percentage1 * cDC->cameraMoveSpeed + (0x100 - (u32)zoom->percentage1) * speed) >> 8;
		}
	}

	uVar8 = 0;
	if (GAME_TRACKER->numPlyrCurrGame != 2)
	{
		uVar8 = 0xff9c;
	}

	scratchWork->camera.rot.x = uVar8;

	// 0x20e
	// camera RotY
	scratchWork->camera.rot.y = (d->angle + 0x800 + d->rotCurr.w + sVar10) & 0xfff;

	scratchWork->camera.rot.z = (s16)((-(u32)(u16)cDC->desiredRot.x) << 1);

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix(&scratchWork->camera.matrix, &scratchWork->camera.rot);

	if ((cDC->flags & CAMERA_FLAG_FIRE_SPEED_ZOOM) != 0)
	{
		s32 fireTimer = (s32)((u32)(u16)d->fireSpeedCap << 0x10) >> 0x14;
		if (cDC->fireSpeedZoom.timer < fireTimer)
		{
			cDC->fireSpeedZoom.timer = fireTimer;
		}
	}
	scratchWork->camera.rot.x = 0;
	scratchWork->camera.rot.y = 0;

	uVar8 = VehCalc_MapToRange(cDC->cameraMoveSpeed, (s32)zoom->speedMin, (s32)zoom->speedMax, (s32)zoom->distMin, (s32)zoom->distMax);

	scratchWork->camera.rot.z = uVar8;

	if (cDC->fireSpeedZoom.timer != 0)
	{
		cDC->fireSpeedZoom.distanceOffset += GAME_TRACKER->elapsedTimeMS * 0x40;
		if (cDC->fireSpeedZoom.distanceOffset > 0x6000)
		{
			cDC->fireSpeedZoom.distanceOffset = 0x6000;
		}

		cDC->fireSpeedZoom.timer -= GAME_TRACKER->elapsedTimeMS;
		if (cDC->fireSpeedZoom.timer < 0)
		{
			cDC->fireSpeedZoom.timer = 0;
		}
	}
	else if (cDC->fireSpeedZoom.distanceOffset != 0)
	{
		cDC->fireSpeedZoom.distanceOffset -= GAME_TRACKER->elapsedTimeMS * 0x10;

		if (cDC->fireSpeedZoom.distanceOffset < 0)
		{
			cDC->fireSpeedZoom.distanceOffset = 0;
		}
	}
	scratchWork->camera.rot.z += cDC->fireSpeedZoom.distanceOffset >> 8;

	CTR_GteLoadRotMatrix(&scratchWork->camera.matrix);
	CTR_GteLoadSVec3V0(&scratchWork->camera.rot);
	CTR_GteLoadDelay();
	gte_rtv0();
	CTR_PSX_STORE_COP2_VEC3(&scratchWork->camera.pos.x, 25, 26, 27);

	scratchWork->camera.rot.x = 0;
	scratchWork->camera.rot.y = 0x40;
	scratchWork->camera.rot.z = 0;

	CTR_GteLoadSVec3V0(&scratchWork->camera.rot);
	CTR_GteLoadDelay();
	gte_rtv0();
	CTR_PSX_STORE_COP2_VEC3(&scratchWork->sideOffset.x, 25, 26, 27);

	scratchWork->camera.delta.x = CTR_MipsSra(d->posCurr.x, 8);
	scratchWork->camera.delta.y = CTR_MipsSra(d->posCurr.y, 8);
	scratchWork->camera.delta.z = CTR_MipsSra(d->posCurr.z, 8);

	scratchWork->camera.delta.x += scratchWork->sideOffset.x;
	scratchWork->camera.delta.z += scratchWork->sideOffset.z;
	scratchWork->camera.pos.x += scratchWork->camera.delta.x;


	// mask-grab
	if ((cDC->flags & CAMERA_FLAG_MASK_GRAB) != 0)
	{
		scratchWork->camera.pos.y = CTR_MipsSra(d->quadBlockHeight, 8) + (s32)cDC->maskGrabHeightOffset + (s32)zoom->vertDistance;
	}

	else
	{
		scratchWork->camera.pos.y += scratchWork->camera.delta.y + (s32)zoom->vertDistance;
	}

	scratchWork->camera.pos.z += scratchWork->camera.delta.z;

	uVar8 = 0;
	if (GAME_TRACKER->numPlyrCurrGame != 2)
	{
		uVar8 = 0xff9c;
	}

	// rotX
	scratchWork->camera.rot.x = uVar8;

	// rotY
	scratchWork->camera.rot.y = (((s16)(d->rotCurr.w + 0x800) + d->angle) + d->turnAngleCurr + sVar10) & 0xfff;

	// rotZ
	scratchWork->camera.rot.z = 0;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix(&scratchWork->camera.matrix, &scratchWork->camera.rot);

	scratchWork->camera.rot.x = 0;
	scratchWork->camera.rot.y = 0;

	// if racer is damaged,
	// straight-forward angle
	if ((d->actionsFlagSet & ACTION_WARP) != 0)
	{
		cDC->damagePitchOffset += 8;
		if (cDC->damagePitchOffset > 0)
		{
			cDC->damagePitchOffset = 0;
		}
	}

	// if racer is not damaged,
	// slight-down view angle
	else
	{
		cDC->damagePitchOffset -= 8;
		if (cDC->damagePitchOffset < -0x20)
		{
			cDC->damagePitchOffset = -0x20;
		}
	}

	// Z
	scratchWork->camera.rot.z = cDC->damagePitchOffset;

	CTR_GteLoadRotMatrix(&scratchWork->camera.matrix);

	CTR_GteLoadSVec3V0(&scratchWork->camera.rot);
	CTR_GteLoadDelay();
	gte_rtv0();
	gte_stsv(&scratchWork->camera.rot);

	scratchWork->camera.delta.x += (s32)scratchWork->camera.rot.x;
	scratchWork->camera.delta.y += (s32)scratchWork->camera.rot.y + (s32)zoom->angle[2];
	scratchWork->camera.delta.z += (s32)scratchWork->camera.rot.z;

	cDC->desiredRot.x = ((zoom->angle[1] * (s32)cDC->desiredRot.x) + ((0x100 - (s32)zoom->angle[1]) * (s32)d->rotCurr.z)) >> 8;


	if (d->kartState == KS_MASK_GRABBED)
	{
		// pushBuffer position
		scratchWork->camera.pos.x = (s32)pb->pos.x;
		scratchWork->camera.pos.y = (s32)pb->pos.y;
		scratchWork->camera.pos.z = (s32)pb->pos.z;

		cDC->heightSmoothing.currentOffset = 0;
		cDC->heightSmoothing.startOffset = 0;
		cDC->heightSmoothing.framesRemaining = 0;
	}

	if (d->kartState == KS_ENGINE_REVVING)
	{
		cDC->heightSmoothing.currentOffset = 0;
		cDC->heightSmoothing.startOffset = 0;
		cDC->heightSmoothing.framesRemaining = 0;
	}

	if ((d->kartState != KS_BLASTED) && ((d->actionsFlagSet & ACTION_TOUCH_GROUND) != 0) && (cDC->BlastedLerp.boolLerpPending != 0))
	{
		cDC->BlastedLerp.desiredPos.x = cDC->cameraPos.x - (s16)scratchWork->camera.pos.x;
		cDC->BlastedLerp.desiredPos.y = cDC->cameraPos.y - (s16)scratchWork->camera.pos.y;
		cDC->BlastedLerp.desiredPos.z = cDC->cameraPos.z - (s16)scratchWork->camera.pos.z;

		cDC->BlastedLerp.desiredRot.x = cDC->lookAtPos.x - (s16)scratchWork->camera.delta.x;
		cDC->BlastedLerp.desiredRot.y = cDC->lookAtPos.y - (s16)scratchWork->camera.delta.y;
		cDC->BlastedLerp.desiredRot.z = cDC->lookAtPos.z - (s16)scratchWork->camera.delta.z;

		cDC->BlastedLerp.framesRemaining = 8;
		cDC->BlastedLerp.boolLerpPending = 0;
	}

	// if this is arcade end-of-race
	if (((cDC->flags & CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED) != 0) || (cDC->cameraMode != 0))
	{
		if (cDC->BlastedLerp.boolLerpPending != 0)
		{
			cDC->BlastedLerp.desiredPos.x = cDC->cameraPos.x - (s16)scratchWork->camera.pos.x;
			cDC->BlastedLerp.desiredPos.y = cDC->cameraPos.y - (s16)scratchWork->camera.pos.y;
			cDC->BlastedLerp.desiredPos.z = cDC->cameraPos.z - (s16)scratchWork->camera.pos.z;

			cDC->BlastedLerp.desiredRot.x = cDC->lookAtPos.x - (s16)scratchWork->camera.delta.x;
			cDC->BlastedLerp.desiredRot.y = cDC->lookAtPos.y - (s16)scratchWork->camera.delta.y;
			cDC->BlastedLerp.desiredRot.z = cDC->lookAtPos.z - (s16)scratchWork->camera.delta.z;

			cDC->BlastedLerp.framesRemaining = 8;
			cDC->BlastedLerp.boolLerpPending = 0;
		}
	}
	else
	{
		if ((d->kartState == KS_BLASTED) || (cDC->BlastedLerp.boolLerpPending != 0))
		{
			if (cDC->BlastedLerp.boolLerpPending == 0)
			{
				cDC->BlastedLerp.unkOffset[0] = cDC->lookAtPos.y - cDC->cameraPos.y;
				cDC->BlastedLerp.unkOffset[1] = cDC->cameraPos.y - CTR_MipsSra(d->quadBlockHeight, 8);
			}

			cDC->BlastedLerp.boolLerpPending = 1;

			if (scratchWork->camera.pos.y > cDC->cameraPos.y)
			{
				s32 maxY = (s32)cDC->BlastedLerp.unkOffset[1] + CTR_MipsSra(d->quadBlockHeight, 8);
				if (maxY < scratchWork->camera.pos.y)
				{
					scratchWork->camera.pos.y = maxY;
				}
			}
		}
	}

	// if frame countdown is not finished
	if (cDC->BlastedLerp.boolLerpPending == 0 && cDC->BlastedLerp.framesRemaining != 0)
	{
		scratchWork->camera.pos.x += (cDC->BlastedLerp.desiredPos.x * cDC->BlastedLerp.framesRemaining) >> 3;
		scratchWork->camera.pos.y += (cDC->BlastedLerp.desiredPos.y * cDC->BlastedLerp.framesRemaining) >> 3;
		scratchWork->camera.pos.z += (cDC->BlastedLerp.desiredPos.z * cDC->BlastedLerp.framesRemaining) >> 3;

		scratchWork->camera.delta.x += (cDC->BlastedLerp.desiredRot.x * cDC->BlastedLerp.framesRemaining) >> 3;
		scratchWork->camera.delta.y += (cDC->BlastedLerp.desiredRot.y * cDC->BlastedLerp.framesRemaining) >> 3;
		scratchWork->camera.delta.z += (cDC->BlastedLerp.desiredRot.z * cDC->BlastedLerp.framesRemaining) >> 3;

		// decrease frame countdown
		cDC->BlastedLerp.framesRemaining--;
	}

	CAM_FindClosestQuadblock((struct ScratchpadStruct *)scratchWork, cDC, d, &scratchWork->camera.pos);

	{
		if ((((struct ScratchpadStruct *)scratchWork)->boolDidTouchQuadblock != 0) &&
		    ((((struct ScratchpadStruct *)scratchWork)->hit.ptrQuadblock->quadFlags & CAM_FOLLOW_DRIVER_QUAD_FLAGS_SKIP_TERRAIN_HEIGHT) == 0))
		{
			u8 terrain = ((struct ScratchpadStruct *)scratchWork)->hit.ptrQuadblock->terrain_type;
			s32 minY;
			s32 smoothFrames;

			// Mud, Water, or FastWater
			if (((terrain == 0xe) || (terrain == 4)) || (terrain == 0xd))
			{
				scratchWork->collision.terrainHeightFloor = 0;
			}

			minY = (s32)scratchWork->collision.terrainHeightFloor + (s32)zoom->vertDistance;
			if (scratchWork->camera.pos.y < minY)
			{
				scratchWork->camera.pos.y = minY;
			}

			smoothFrames = cDC->heightSmoothing.framesRemaining;
			if (smoothFrames != 0)
			{
				scratchWork->camera.pos.y =
				    ((8 - smoothFrames) * scratchWork->camera.pos.y + smoothFrames * ((s32)cDC->heightSmoothing.startOffset + CTR_MipsSra(d->posCurr.y, 8))) >>
				    3;

				cDC->heightSmoothing.framesRemaining += -1;
			}
		}
		else
		{
			if (scratchWork->camera.pos.y < (s32)cDC->heightSmoothing.currentOffset + CTR_MipsSra(d->posCurr.y, 8))
			{
				cDC->heightSmoothing.framesRemaining = 8;
				cDC->heightSmoothing.startOffset = cDC->heightSmoothing.currentOffset;
				scratchWork->camera.pos.y = (s32)cDC->heightSmoothing.startOffset + CTR_MipsSra(d->posCurr.y, 8);
				goto LAB_8001ab04;
			}

			cDC->heightSmoothing.framesRemaining = 8;
			cDC->heightSmoothing.startOffset = (s16)scratchWork->camera.pos.y - (s16)CTR_MipsSra(d->posCurr.y, 8);
		}
		cDC->heightSmoothing.currentOffset = (s16)scratchWork->camera.pos.y - CTR_MipsSra(d->posCurr.y, 8);
	LAB_8001ab04:

		// if mask grabs you when you're underwater
		if (((GAME_TRACKER->level1->configFlags & 2) != 0) && (scratchWork->camera.pos.y < zoom->vertDistance))
		{
			scratchWork->camera.pos.y = zoom->vertDistance;
		}

		if (cDC->BlastedLerp.boolLerpPending != 0)
		{
			scratchWork->camera.delta.y = scratchWork->camera.pos.y + (s32) * (s16 *)((s32)cDC + 0xc8);
		}

		if (d->kartState == KS_MASK_GRABBED)
		{
			pb->rot.z -= (pb->rot.z >> 3);

			// camera dirX, cameraPosX minus driverPosX
			scratchWork->camera.dir.x = (s32)pb->pos.x - CTR_MipsSra(d->posCurr.x, 8);

			// camera dirY, cameraPosY minus driverPosY, plus something else
			scratchWork->camera.dir.y = (s32)pb->pos.y - (CTR_MipsSra(d->posCurr.y, 8) + (s32)zoom->angle[2]);

			// camera dirZ, cameraPosZ minus driverPosZ
			scratchWork->camera.dir.z = (s32)pb->pos.z - CTR_MipsSra(d->posCurr.z, 8);

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
			s32 dist;

			// camera direction
			scratchWork->camera.dir.x = scratchWork->camera.pos.x - scratchWork->camera.delta.x;
			scratchWork->camera.dir.y = scratchWork->camera.pos.y - scratchWork->camera.delta.y;
			scratchWork->camera.dir.z = scratchWork->camera.pos.z - scratchWork->camera.delta.z;

			// camera rotation
			pb->rot.y = (s16)ratan2(scratchWork->camera.dir.x, scratchWork->camera.dir.z);

			dist = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsMulLo(scratchWork->camera.dir.x, scratchWork->camera.dir.x),
			                                      CTR_MipsMulLo(scratchWork->camera.dir.z, scratchWork->camera.dir.z)));

			pb->rot.x = 0x800 - (s16)ratan2(scratchWork->camera.dir.y, dist);

			pb->rot.z = (s16)CTR_MipsSra(CTR_MipsMulLo((s32)zoom->angle[0], (s32)cDC->desiredRot.x), 8);
		}

		// something with pushBuffer position
		scratchWork->camera.posCopy.x = scratchWork->camera.pos.x - (s32)pb->pos.x;
		scratchWork->camera.posCopy.y = scratchWork->camera.pos.y - (s32)pb->pos.y;
		scratchWork->camera.posCopy.z = scratchWork->camera.pos.z - (s32)pb->pos.z;

		cDC->pushBufferPosCorrection.x -= (scratchWork->camera.pos.x - cDC->cameraPos.x);
		cDC->pushBufferPosCorrection.y -= (scratchWork->camera.pos.y - cDC->cameraPos.y);
		cDC->pushBufferPosCorrection.z -= (scratchWork->camera.pos.z - cDC->cameraPos.z);

		if (cDC->pushBufferPosCorrection.x > 2)
		{
			cDC->pushBufferPosCorrection.x = 2;
		}
		if (cDC->pushBufferPosCorrection.x < -2)
		{
			cDC->pushBufferPosCorrection.x = -2;
		}
		if (cDC->pushBufferPosCorrection.y > 2)
		{
			cDC->pushBufferPosCorrection.y = 2;
		}
		if (cDC->pushBufferPosCorrection.y < -2)
		{
			cDC->pushBufferPosCorrection.y = -2;
		}
		if (cDC->pushBufferPosCorrection.z > 2)
		{
			cDC->pushBufferPosCorrection.z = 2;
		}
		if (cDC->pushBufferPosCorrection.z < -2)
		{
			cDC->pushBufferPosCorrection.z = -2;
		}

		if (d->kartState != KS_MASK_GRABBED)
		{
			// pushBuffer position
			pb->pos.x += (s16)scratchWork->camera.posCopy.x + cDC->pushBufferPosCorrection.x;
			pb->pos.y += (s16)scratchWork->camera.posCopy.y + cDC->pushBufferPosCorrection.y;
			pb->pos.z += (s16)scratchWork->camera.posCopy.z + cDC->pushBufferPosCorrection.z;
		}

		cDC->cameraPos.x = scratchWork->camera.pos.x;
		cDC->cameraPos.y = scratchWork->camera.pos.y;
		cDC->cameraPos.z = scratchWork->camera.pos.z;
		cDC->lookAtPos.x = scratchWork->camera.delta.x;
		cDC->lookAtPos.y = scratchWork->camera.delta.y;
		cDC->lookAtPos.z = scratchWork->camera.delta.z;

		// backup flags (again)
		backupFlags = cDC->flags;

		cDC->flags &= ~CAMERA_FLAG_MASK_GRAB;

		if (
		    // transitioning, end-race battle, intro-race
		    ((backupFlags & (CAMERA_FLAG_TRANSITION_AWAY | CAMERA_FLAG_BATTLE_END_OF_RACE)) == 0) && ((GAME_TRACKER->gameMode1 & START_OF_RACE) == 0))
		{
			return;
		}


		// if end-of-race battle
		if ((backupFlags & CAMERA_FLAG_BATTLE_END_OF_RACE) != 0)
		{
			CAM_FollowDriver_Spin360(cDC, scratchWork, d, &local_40, &local_38);

			// reverse interpolation of fly-in [0x1000 to 0]
			blend = 0x1000 - cDC->transitionBlend;
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
				blend = cDC->transitionBlend;
			}

			// if startline camera
			else
			{
				struct SpawnType1 *st1 = GAME_TRACKER->level1->ptrSpawnType1;
				s16 flyInDone = 0;

				// run fly-in animation
				if (st1->count >= 4)
				{
					u8 *cameraPath = ((void **)ST1_GETPOINTERS(st1))[ST1_CAMERA_PATH];
					s16 flyInFrame;

					flyInData.frameCount2 = 0x8e;
					flyInData.ptrStart = cameraPath;
					flyInData.ptrEnd = cameraPath + 0x354;
					flyInData.frameCount1 = 0x96;

					// which frame of fly-in you are in
					flyInFrame = 0xa5 - cDC->transitionFrame;

					if (flyInFrame > 0x96)
					{
						flyInFrame = 0x96;
					}

					CAM_StartLine_FlyIn(&flyInData, 0x96, flyInFrame, &local_40, &local_38);

					blend = (s32)cDC->transitionBlend;
				}
				// No camera + No ghosts (battle maps)
				else
				{
					// startline fly-in is done
					flyInDone = 1;
					blend = 0x1000;
				}

				if (cDC->transitionFrame < 1)
				{
					flyInDone = 1;
				}

				// Press Triangle
				if ((GAMEPADS->gamepad[d->driverID].buttonsTapped & BTN_TRIANGLE) != 0)
				{
					cDC->flags |= CAMERA_FLAG_RESET_RAIN_POS | CAMERA_FLAG_DIRECTION_CHANGED;
					flyInDone = 1;
				}

				// if startline fly-in is done
				if (flyInDone)
				{
					struct GameTracker *gGT = GAME_TRACKER;
					gGT->hudFlags |= HUD_FLAG_RACE_HUD;
					gGT->gameMode1 &= ~(START_OF_RACE);
					GAME_TRACKER->hudFlags |= HUD_FLAG_RENDER_LESS;
				}
			}
		}

		// use camera pos+rot, TransitionTo pos+rot, camera pos+rot, and interpolation
		CAM_ProcessTransition(&pb->pos, &pb->rot, &local_40, &local_38, &pb->pos, &pb->rot, blend);

		scratchWork->camera.pos.x = (s32)pb->pos.x;
		scratchWork->camera.pos.y = (s32)pb->pos.y;
		scratchWork->camera.pos.z = (s32)pb->pos.z;

		CAM_FindClosestQuadblock((struct ScratchpadStruct *)scratchWork, cDC, d, &scratchWork->camera.pos);

		{
			s32 totalFrames = (s32)cDC->transitionFrameCount;

			if (totalFrames != 0)
			{
				s32 curFrame = (s32)cDC->transitionFrame;

				if (curFrame <= totalFrames)
				{
					s32 halfFrames = totalFrames / 2;

					if (curFrame < halfFrames)
					{
						// Sine(angle)
						s32 s = MATH_Sin(0x400 - (curFrame << 10) / halfFrames);

						cDC->transitionBlend = (s16)(s / 2) + 0x800;
					}
					else
					{
						// Cosine(angle)
						s32 c = MATH_Cos(((curFrame - totalFrames) * 0x400) / halfFrames);

						cDC->transitionBlend = 0x800 - (s16)(c / 2);
					}
				}
			}
		}

		// backup  flags
		backupFlags = cDC->flags;

		// if transition is a round-trip,
		// like Load/Save that moves and comes back
		if ((backupFlags & CAMERA_FLAG_TRANSITION_AWAY) != 0)
		{
			// if transitioning back to player
			if ((backupFlags & CAMERA_FLAG_TRANSITION_BACK) != 0)
			{
				if (cDC->transitionFrame > 0)
				{
					cDC->transitionFrame--;
				}
				else
				{
					cDC->flags &= ~(CAMERA_FLAG_TRANSITION_AWAY | CAMERA_FLAG_TRANSITION_BACK | CAMERA_FLAG_TRANSITION_HOLD);
				}
			}
			// if not transitioning back to player
			else
			{
				if (cDC->transitionFrame < cDC->transitionFrameCount)
				{
					cDC->transitionFrame++;
				}
				else
				{
					cDC->flags |= CAMERA_FLAG_TRANSITION_HOLD;
				}
			}
		}

		// if not a round-trip,
		// like startline camera
		else
		{
			// If game is not paused
			if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
			{
				if (cDC->transitionFrame > 0)
				{
					cDC->transitionFrame--;
				}
			}
		}
	}
#undef pb
}

// NOTE(aalhendi): The stack vectors have word-aligned XY pairs. Retail loads
// each pair with Z before transferring them to the GTE, using t4/t5. This
// writes the full second rotation word, not a row update preserving its high
// half. Native keeps the existing typed register transfers.
static inline void CAM_LoadProjectionInputs(const SVec3 *vector, const SVec3 *row)
{
#ifdef CTR_NATIVE
	CTC2(CTR_PackS16Pair(row->x, row->y), 0);
	CTC2((s32)row->z, 1);
	CTR_GteLoadSVec3V0(vector);
#else
	__asm__ volatile("lw $12,0(%1)\n\t"
	                 "lh $13,4(%1)\n\t"
	                 "ctc2 $12,$0\n\t"
	                 "ctc2 $13,$1\n\t"
	                 "lw $12,0(%0)\n\t"
	                 "lh $13,4(%0)\n\t"
	                 "mtc2 $12,$0\n\t"
	                 "mtc2 $13,$1"
	                 :
	                 : "r"(vector), "r"(row)
	                 : "$12", "$13", "memory");
#endif
}

s32 CAM_MapRange_PosPoints(SVec3 *pos1, SVec3 *pos2, SVec3 *currPos)
{
	SVec3 pathDelta;
	pathDelta.x = pos1->x - pos2->x;
	pathDelta.y = pos1->y - pos2->y;
	pathDelta.z = pos1->z - pos2->z;

	MATH_VectorNormalize(&pathDelta);

	{
		SVec3 currDelta;
		currDelta.x = currPos->x - pos1->x;
		currDelta.y = currPos->y - pos1->y;
		currDelta.z = currPos->z - pos1->z;

		CAM_LoadProjectionInputs(&currDelta, &pathDelta);
		CTR_GteLoadDelay();
		gte_mvmva(0, 0, 0, 3, 0);

		return MFC2_S(25) >> 12;
	}
}

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
	{
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

		{
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
				pb->pos.x = (s16)d->instSelf->matrix.t[0];
				pb->pos.y = (s16)d->instSelf->matrix.t[1];
				pb->pos.z = (s16)d->instSelf->matrix.t[2];
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
				CTR_WriteU16LE(&cDC->action, (u16)*psVar19);
				CTR_WriteU16LE((u8 *)&cDC->action + 2, (u16)psVar21[2]);
				cDC->mode = psVar21[3];
				CTR_WriteU16LE(&cDC->unk0xC, (u16)psVar21[4]);
				CTR_WriteU16LE((u8 *)&cDC->unk0xC + 2, (u16)psVar21[5]);
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
				sVar6 = (s16)pb->distanceToScreen_CURR;
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

								{
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

									iVar24 = SquareRoot0_stub(CTR_MipsAddLo(
									    CTR_MipsAddLo(CAM_MulLo((s32)stackMemPos.x, (s32)stackMemPos.x), CAM_MulLo((s32)stackMemPos.y, (s32)stackMemPos.y)),
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
									psVar21 = CTR_VECTOR_DATA(&((cDC->transitionTo).rot));
									if (iVar17 < 0)
									{
										psVar21 = CTR_VECTOR_DATA(&(cDC->eorModeData.pointPath.endPos));
									}
									pb->pos.x = psVar21[0] + (s16)((stackMemPos.x * iVar25) >> 0xc);
									pb->pos.y = psVar21[1] + (s16)((stackMemPos.y * iVar25) >> 0xc);
									pb->pos.z = psVar21[2] + (s16)((stackMemPos.z * iVar25) >> 0xc);
									goto LAB_8001c11c;
								}
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
										CAM_FollowDriver_AngleAxis(cDC, d, CameraScratchWork_AsAngleAxis(scratchWork), &pb->pos, &pb->rot);
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

										{
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

					{
						Vec3 cameraProbePos;
						cameraProbePos.x = (s32)pb->pos.x;
						cameraProbePos.y = (s32)pb->pos.y;
						cameraProbePos.z = (s32)pb->pos.z;

						CAM_FindClosestQuadblock((struct ScratchpadStruct *)scratchWork, cDC, d, &cameraProbePos);
						goto LAB_8001c150;
					}
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
	}
}
