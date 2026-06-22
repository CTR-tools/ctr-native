#include <common.h>

enum RaceFlagScratchConstants
{
	RACE_FLAG_SCREEN_ROWS = 10,
	RACE_FLAG_SCREEN_POINTS_PER_ROW = 3,
};

struct RaceFlagProjectedRow
{
	u32 xy[RACE_FLAG_SCREEN_POINTS_PER_ROW];
};

union RaceFlagScreenBuffer
{
	struct RaceFlagProjectedRow row[RACE_FLAG_SCREEN_ROWS];
	u32 xy[RACE_FLAG_SCREEN_ROWS * RACE_FLAG_SCREEN_POINTS_PER_ROW];
};

struct RaceFlagScratch
{
	union RaceFlagScreenBuffer screen[2];
};

_Static_assert(sizeof(union RaceFlagScreenBuffer) == 0x78);
_Static_assert(sizeof(struct RaceFlagScratch) == 0xf0);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043e34-0x80043f1c.
int RaceFlag_MoveModels(int frameIndex, int numFrames)
{
	// need a better prefix than TitleFlag,
	// all this does is move the intro logo models
	// from the center of the screen, to the right

	// also used for transitioning driver models
	// on and off the screen in character selection

	int angle;
	int midpoint;
	int result;

	if (frameIndex < 0)
		return 0;

	if (frameIndex > numFrames)
		return 0x1000;

	// cut in half
	midpoint = numFrames / 2;

	// if less than half done
	if (frameIndex < midpoint)
	{
		angle = (midpoint - frameIndex) * 0x400;

		// 50% - sin(angle) / 2
		result = 0x800 - MATH_Sin(angle / midpoint) / 2;
	}
	// if more than half done
	else
	{
		angle = (frameIndex - midpoint) * 0x400;

		// sin(angle) / 2 + 50%
		result = MATH_Sin(angle / midpoint) / 2 + 0x800;
	}
	return result;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043f1c-0x80043f28.
int RaceFlag_IsFullyOnScreen(void)
{
	// return true if flag is fully on screen
	// return false if flag is not fully on screen
	return (sdata->RaceFlag_Position == 0);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043f28-0x80043f44.
int RaceFlag_IsFullyOffScreen(void)
{
	// return false, "not true", if flag is < 5000, partially on-screen
	// return true, "not false", if flag is >= 5000, fully off-screen
	return ((((u16)sdata->RaceFlag_Position + 4999U) & 0xffff) < 9999) ^ 1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043f44-0x80043f8c.
int RaceFlag_IsTransitioning()
{
	int pos = sdata->RaceFlag_Position;

	return
	    // if checkered flag is not fully on-screen and not fully off-screen
	    (pos != 0) && (pos != -5000) && (pos != 5000) &&

	    // is allowed to render
	    ((sdata->gGT->renderFlags & 0x1000) != 0);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043f8c-0x80043fb0.
void RaceFlag_SetDrawOrder(int drawOrder)
{
	sdata->RaceFlag_DrawOrder = (drawOrder != 0) ? 1 : -1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043fb0-0x8004402c.
void RaceFlag_BeginTransition(int direction)
{
	// Begin Transition on-screen
	if (direction == 1)
	{
		sdata->RaceFlag_LoadingTextAnimFrame = -1;

		sdata->RaceFlag_Position = 5000;

		sdata->RaceFlag_AnimationType = 0;
	}

	// Begin Transition off-screen
	else if (direction == 2)
	{
		RaceFlag_SetDrawOrder(0);

		sdata->RaceFlag_Position = 0;

		sdata->RaceFlag_AnimationType = direction;
	}

	// enable loading screen's checkered flag
	sdata->gGT->renderFlags |= 0x1000;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004402c-0x80044058.
void RaceFlag_SetFullyOnScreen()
{
	sdata->RaceFlag_AnimationType = 0;
	sdata->RaceFlag_LoadingTextAnimFrame = -1;

	// flag is now fully on-screen
	sdata->RaceFlag_Position = 0;

	// enable loading screen's checkered flag
	sdata->gGT->renderFlags |= 0x1000;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80044058-0x80044088.
void RaceFlag_SetFullyOffScreen()
{
	sdata->RaceFlag_AnimationType = 0;
	sdata->RaceFlag_LoadingTextAnimFrame = -1;

	// flag is now fully off-screen
	sdata->RaceFlag_Position = 5000;

	// disable loading screen's checkered flag
	sdata->gGT->renderFlags &= ~(0x1000);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80044088-0x80044094.
void RaceFlag_SetCanDraw(s16 param_1)
{
	sdata->RaceFlag_CanDraw = param_1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80044094-0x800440a0.
int RaceFlag_GetCanDraw(void)
{
	return sdata->RaceFlag_CanDraw;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800440a0-0x80044290.
u32 *RaceFlag_GetOT(void)
{
	s16 sVar1;
	int iVar2;
	struct GameTracker *gGT = sdata->gGT;

	u32 *otDrawFirst_FarthestDepth;
	u32 *otDrawLast_ClosestDepth;

	otDrawFirst_FarthestDepth = (u32 *)&gGT->pushBuffer[0].ptrOT[0x3FF];
	otDrawLast_ClosestDepth = gGT->otSwapchainDB[gGT->swapchainIndex];

	if (sdata->RaceFlag_DrawInitialized == 0)
	{
		sdata->RaceFlag_DrawInitialized = 1;
	}

	// transitioning on-screen
	if (sdata->RaceFlag_AnimationType == 0)
	{
		// set fully "off" to start transition "on"
		if (sdata->RaceFlag_Position < 0)
			sdata->RaceFlag_Position = 5000;

		sdata->RaceFlag_TransitionSpeed = 300;

		iVar2 = sdata->RaceFlag_Position;

		// if transitioning
		if (iVar2 != 0)
		{
			// skip last 8 frames to zero
			if (iVar2 < 8)
				sdata->RaceFlag_Position = 0;

			// transition for frame >= 8
			else
			{
				// rate of transition

				iVar2 = ((u16)sdata->RaceFlag_Position >> 3) * gGT->elapsedTimeMS;
				iVar2 = iVar2 >> 5;

				sVar1 = -(s16)iVar2;
				if (iVar2 < 1)
				{
					sVar1 = -1;
				}

				sdata->RaceFlag_Position += sVar1;
			}
		}

		// transition is finished
		else
		{
			if (sdata->RaceFlag_DrawOrder != 1)
			{
				if (sdata->RaceFlag_DrawOrder != -1)
					return otDrawFirst_FarthestDepth;

				sdata->RaceFlag_DrawOrder = 0;
			}
		}
	}

	// transition off-screen
	if (sdata->RaceFlag_AnimationType == 2)
	{
		if (sdata->RaceFlag_TransitionSpeed < 1000)
		{
			sdata->RaceFlag_TransitionSpeed += (s16)((gGT->elapsedTimeMS * 10) >> 5);
		}

		// If transitioning "off"
		if (sdata->RaceFlag_Position > -5000)
		{
			sdata->RaceFlag_Position -= (((u32)sdata->RaceFlag_TransitionSpeed >> 2) * gGT->elapsedTimeMS) >> 5;
		}

		// finished transitioning off
		else
		{
			sdata->RaceFlag_Position = 5000;
			sdata->RaceFlag_AnimationType = 0;
			gGT->renderFlags &= ~(0x1000);
		}
	}

	return otDrawLast_ClosestDepth;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80044290-0x800442a0.
void RaceFlag_ResetTextAnim(void)
{
	sdata->RaceFlag_LoadingTextAnimFrame = -1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800442a0-0x800444e8.
void RaceFlag_DrawLoadingString(void)
{
	struct GameTracker *gGT = sdata->gGT;
	int iVar2;
	int iVar3;
	int iVar4;
	int uVar5;
	int iVar6;
	char *pbVar7;
	char *pbVar8;
	int iVar9;
	int iVar10;
	u32 *uVar11;
	char local_30;
	char local_2f;

	pbVar7 = sdata->lngStrings[LNG_LOADING];

	// pointer to OT mem
	uVar11 = (u32 *)gGT->pushBuffer_UI.ptrOT;

	// pointer to OT mem
	gGT->pushBuffer_UI.ptrOT = gGT->otSwapchainDB[gGT->swapchainIndex];

	// get length of "LOADING..." string
	iVar2 = 10;

	iVar3 = DecalFont_GetLineWidth(pbVar7, 1);

	// loop counter
	iVar6 = 0;

	// if game is not loading
	if (sdata->Loading.stage == LOAD_IDLE)
	{
		if (-1000 < (int)sdata->RaceFlag_Transition)
		{
			sdata->RaceFlag_Transition -= 0x28;
		}
	}
	else
	{
		sdata->RaceFlag_Transition = 0;
	}

	iVar10 = (sdata->RaceFlag_Transition & 0xffff) - (iVar3 >> 1);

	iVar3 = sdata->RaceFlag_LoadingTextAnimFrame;

	if (0 < iVar2)
	{
		iVar9 = iVar3 * -0x3c + 0x23c;

		// for iVar6 = 0; iVar6 < strlen("LOADING..."); iVar6++)
		do
		{
			if (iVar3 < 0)
			{
			LAB_800443c4:

				// draw text off screen
				iVar4 = 0x23c;
			}
			else
			{
				iVar4 = iVar9;
				if (
				    // if frame > 4,
				    // if text starts moving on-screen?
				    (4 < iVar3) && (
				                       // draw letter at midpoint of screen
				                       iVar4 = 0x100,

				                       // if frame > 0x4a,
				                       // if text starts moving off-screen?
				                       0x4a < iVar3))
				{
					// if frame > 0x4f,
					// if letter is fully off-screen
					if (0x4f < iVar3)
						goto LAB_800443c4;

					// letter is moving off-screen
					iVar4 = (0x4b - iVar3) * 0x3c + 0x100;
				}
			}
			local_30 = *pbVar7;
			pbVar8 = pbVar7 + 1;
			uVar5 = 1;
			if (local_30 < 4)
			{
				local_2f = *pbVar8;
				pbVar8 = pbVar7 + 2;

				// increment loop counter
				iVar6 = iVar6 + 1;

				uVar5 = 2;
			}
			if ((s16)iVar4 != 0x23c)
			{
				DecalFont_DrawLineStrlen(&local_30, uVar5, (iVar10 + iVar4), 0x6c, 1, 0);
			}

			iVar4 = DecalFont_GetLineWidthStrlen(&local_30, uVar5, 1);

			iVar10 = iVar10 + iVar4;
			iVar9 = iVar9 + 0xf0;

			// increment loop counter
			iVar6 = iVar6 + 1;

			// treat all letters with 4 frame difference
			iVar3 = iVar3 + -4;

			pbVar7 = pbVar8;
		} while (iVar6 < iVar2);
	}

	// pointer to OT mem
	gGT->pushBuffer_UI.ptrOT = (u_long *)uVar11;

	if (iVar3 < 0x50)
	{
		iVar2 = gGT->elapsedTimeMS >> 5;

		if (iVar2 < 1)
		{
			iVar2 = 1;
		}

		sdata->RaceFlag_LoadingTextAnimFrame += iVar2;
	}

	else
	{
		sdata->RaceFlag_LoadingTextAnimFrame = -1;
		if ((u32)(sdata->Loading.stage - 6U) < 2)
		{
			sdata->RaceFlag_LoadingTextAnimFrame = 0;
		}
	}
	return;
}

force_inline char RaceFlag_CalculateBrightness(u32 sine, u8 darkTile)
{
	if (darkTile)
	{
		return ((sine * -55 + 0x140000) >> 0xD);
	}
	return ((sine * -125 + 0x1fe000) >> 0xD);
}

// inline Sine operation
// drops clock from ~130 to
force_inline int MathSinInline(u32 param_1)
{
	int iVar1;

	// approximate trigonometry
	iVar1 = *(int *)&data.trigApprox[param_1 & 0x3ff];

	if ((param_1 & 0x400) == 0)
	{
		iVar1 = iVar1 << 0x10;
	}

	iVar1 = iVar1 >> 0x10;

	if ((param_1 & 0x800) != 0)
	{
		// make negative
		iVar1 = -iVar1;
	}
	return iVar1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800444e8-0x80044ef8.
void RaceFlag_DrawSelf()
{
	int i, j;
	int column, row;
	int toggle;

	s16 flagPos;
	u_long *ot;
	u32 screenlimit;
	u32 dimensions;

	int var2;
	int var3;
	u32 var1;

	POLY_G4 *p;
	struct GameTracker *gGT = sdata->gGT;

	int time;
	int lightL;
	int lightR;

	struct RaceFlagScratch *scratch;
	s32 local[5];
	SVECTOR pos[3] = {0};

	if (sdata->RaceFlag_CanDraw == 0)
		return;

	if (sdata->RaceFlag_LoadingTextAnimFrame < 0)
	{
		if ((5 < sdata->Loading.stage) && (sdata->Loading.stage < 8))
		{
			sdata->RaceFlag_LoadingTextAnimFrame = 0;
		}

		if (sdata->RaceFlag_LoadingTextAnimFrame < 0)
			goto SKIP_LOADING_TEXT;
	}

	RaceFlag_DrawLoadingString();

SKIP_LOADING_TEXT:

	sdata->RaceFlag_CopyLoadStage = sdata->Loading.stage;
	ot = (u_long *)RaceFlag_GetOT();

	gte_SetRotMatrix(&data.matrixTitleFlag);
	gte_SetTransMatrix(&data.matrixTitleFlag);
	gte_SetGeomOffset(0x100, 0x78);
	gte_SetGeomScreen(0x100);

	p = (POLY_G4 *)gGT->backBuffer->primMem.cursor;

	scratch = CTR_SCRATCHPAD_PTR(struct RaceFlagScratch, 0);

	dimensions = 0xd80200;
	screenlimit = 0x80008000;

	toggle = 0;

	// === First Loop Iteration ===
	// Remove 36*10 branching instructions,
	// Reduces clock from ~150 to ~130
	{
		union RaceFlagScreenBuffer *writeScreen = &scratch->screen[toggle];
		toggle = toggle ^ 1;

		local[0] = data.checkerFlagVariables[0];
		local[1] = data.checkerFlagVariables[1];
		local[2] = data.checkerFlagVariables[2];
		local[3] = data.checkerFlagVariables[3];
		local[4] = data.checkerFlagVariables[4];

		// === Step 1 ===
		int stepRate = gGT->elapsedTimeMS;
		local[4] += local[3] * stepRate;
		var1 = (int)local[4] >> 5;

		// === Step 2 ===
		if (0xfff < var1)
		{
			// reset counter
			local[4] &= 0x1ffff;
			var1 = (int)local[4] >> 5;

			local[0] += 0x200;
			local[2] += 200;

			int sin0 = MathSinInline(local[0]) + 0xfff;
			int sin2 = MathSinInline(local[2]) + 0xfff;

			// reset based on trig
			local[1] = (sin0 * 0x20 >> 0xd) + 0x96;
			local[3] = (sin2 * 0x40 >> 0xd) + 0xb4;
		}

		// === Step 3 ===
		var2 = MathSinInline(var1) + 0xfff;
		var2 = var2 * local[1];
		var2 = (var2 >> 0xd) + 0x280;

		// === Step 4 ===
		var1 += 0xc80;
		lightL = MathSinInline(var1) + 0xfff;

		// === Step 5 ===
		pos[0].vy = 0xfc72;
		pos[1].vy = 0xfcd0;
		pos[2].vy = 0xfd2e;

		// === Step 6 ===
		data.checkerFlagVariables[0] = local[0];
		data.checkerFlagVariables[1] = local[1];
		data.checkerFlagVariables[2] = local[2];
		data.checkerFlagVariables[3] = local[3];
		data.checkerFlagVariables[4] = local[4];

		time = sdata->RaceFlag_ElapsedTime >> 5;
		var1 = time;

		flagPos = sdata->RaceFlag_Position;
		flagPos = -0xbbe - flagPos;
		pos[0].vx = flagPos;
		pos[1].vx = flagPos;
		pos[2].vx = flagPos;

		i = 0;
		// === Step 7 ===
		for (row = 0; row < 10; row++)
		{
			SVECTOR *vect;
			for (vect = &pos[0]; vect < &pos[3]; vect++)
			{
				// Range: [1.0, 2.0]
				var3 = MathSinInline(var1) + 0xfff;
				var1 += 300;

				// change all vector posZ
				vect->vz = (s16)var2 + (s16)(var3 * 0x20 >> 0xd);
			}

			CTR_GteLoadSV3WithPad(&pos[0], &pos[1], &pos[2]);
			gte_rtpt();

			pos[0].vy += 0x11a;
			pos[1].vy += 0x11a;
			pos[2].vy += 0x11a;

			CTR_GteStoreSXY3(&writeScreen->row[row].xy[0], &writeScreen->row[row].xy[1], &writeScreen->row[row].xy[2]);
		}

		lightR = lightL;
	}


	// === Rest of Iterations ===
	// Now executing without branching
	for (column = 1; column < 36; column++)
	{
		union RaceFlagScreenBuffer *writeScreen = &scratch->screen[toggle];
		toggle = toggle ^ 1;
		union RaceFlagScreenBuffer *readScreen = &scratch->screen[toggle];

		// === Step 1 ===
		int stepRate = 0x40;
		local[4] += local[3] * stepRate;
		var1 = (int)local[4] >> 5;

		// === Step 2 ===
		if (0xfff < var1)
		{
			// reset counter
			local[4] &= 0x1ffff;
			var1 = (int)local[4] >> 5;

			local[0] += 0x200;
			local[2] += 200;

			int sin0 = MathSinInline(local[0]) + 0xfff;
			int sin2 = MathSinInline(local[2]) + 0xfff;

			// reset based on trig
			local[1] = (sin0 * 0x20 >> 0xd) + 0x96;
			local[3] = (sin2 * 0x40 >> 0xd) + 0xb4;
		}

		// === Step 3 ===
		var2 = MathSinInline(var1) + 0xfff;
		var2 = var2 * local[1];
		var2 = (var2 >> 0xd) + 0x280;

		// === Step 4 ===
		var1 += 0xc80;
		lightL = MathSinInline(var1) + 0xfff;

		// === Step 5 ===
		pos[0].vy = 0xfc72;
		pos[1].vy = 0xfcd0;
		pos[2].vy = 0xfd2e;

		// === Step 6 ===
		time += 0x100;
		var1 = time;

		pos[0].vx += 100;
		pos[1].vx += 100;
		pos[2].vx += 100;

		i = 0;
		// === Step 7 ===
		for (row = 0; row < 10; row++)
		{
			SVECTOR *vect;
			for (vect = &pos[0]; vect < &pos[3]; vect++)
			{
				// Range: [1.0, 2.0]
				var3 = MathSinInline(var1) + 0xfff;
				var1 += 300;

				// change all vector posZ
				vect->vz = (s16)var2 + (s16)(var3 * 0x20 >> 0xd);
			}

			CTR_GteLoadSV3WithPad(&pos[0], &pos[1], &pos[2]);
			gte_rtpt();

			pos[0].vy += 0x11a;
			pos[1].vy += 0x11a;
			pos[2].vy += 0x11a;

			CTR_GteStoreSXY3(&writeScreen->row[row].xy[0], &writeScreen->row[row].xy[1], &writeScreen->row[row].xy[2]);

			int firstSegment = (row == 0) ? 1 : 0;

			for (j = firstSegment; j < RACE_FLAG_SCREEN_POINTS_PER_ROW; j++, i++)
			{
				int pointIndex = row * RACE_FLAG_SCREEN_POINTS_PER_ROW + j - 1;
				u32 read0 = readScreen->xy[pointIndex];
				u32 read1 = readScreen->xy[pointIndex + 1];
				u32 write0 = writeScreen->xy[pointIndex];
				u32 write1 = writeScreen->xy[pointIndex + 1];

				if (((read0 & read1 & write0 & write1 & screenlimit) == 0) &&
				    (((dimensions - read0) & (dimensions - read1) & (dimensions - write0) & (dimensions - write1) & screenlimit) == 0))
				{
					// TRUE for gray, FALSE for white
					u8 boolDark = (((column >> 2) + (i >> 2) & 1U) != 0);

					u8 colorR = RaceFlag_CalculateBrightness(lightR, boolDark);
					setRGB0(p, colorR, colorR, colorR);
					*(int *)&p->r2 = *(int *)&p->r0;

					u8 colorL = RaceFlag_CalculateBrightness(lightL, boolDark);
					setRGB1(p, colorL, colorL, colorL);
					*(int *)&p->r3 = *(int *)&p->r1;

					// positions
					*(int *)&p->x0 = read0;
					*(int *)&p->x2 = read1;
					*(int *)&p->x1 = write0;
					*(int *)&p->x3 = write1;

					// prim/code
					setPolyG4(p);

					// Prim/OT
					// addPrim(ot, p); works but uses more instructions.
					*(int *)p = CtrGpu_PackOTTag(*ot, 0x8000000);
					*ot = CtrGpu_PrimToOTLink24(p);

					p++;
				}
			}
		}

		lightR = lightL;
	}

	gGT->backBuffer->primMem.cursor = p;
	sdata->RaceFlag_ElapsedTime += gGT->elapsedTimeMS * 100;
}
