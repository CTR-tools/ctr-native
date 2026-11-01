#include <common.h>

void UI_DrawSpeedNeedle(s16 posX, s16 posY, struct Driver *driver)
{
	s32 minScale = 0;
	s16 accelSpeed = driver->const_AccelSpeed_ClassStat;
	s16 sacredSpeed = driver->const_SacredFireSpeed;
	s32 maxScale = (((accelSpeed + sacredSpeed) >> 8) * 108000) / 64000;
	s16 speed = driver->speedometerNeedleValue;
	s32 speedScale;
	s32 mappedAngle;
	s16 angle1;
	s32 angle2;
	PolyG3 *p;
	PrimCode primCode;
	const s32 halfNeedleHeight = 30;
	const s32 halfNeedleWidth = 3;
	const s32 needleCenterX = 65;
	const s32 needleCenterY = 41;
	s16 xLen;
	s32 yLen;
	s16 yPixel;

	if (accelSpeed >= speed)
	{
		speedScale = ((speed >> 8) * 108000) / 64000;
		maxScale = ((accelSpeed >> 8) * 108000) / 64000;
		mappedAngle = VehCalc_MapToRange(speedScale, 0, maxScale, ANG(305.20), ANG(213.75));
	}
	else
	{
		speedScale = ((speed >> 8) * 108000) / 64000;
		minScale = ((accelSpeed >> 8) * 108000) / 64000;
		mappedAngle = VehCalc_MapToRange(speedScale, minScale, maxScale, ANG(213.75), ANG(157.5));
	}

	{
		struct DB *backBuffer = GAME_TRACKER->backBuffer;
		u8 *next = backBuffer->primMem.cursor;
		p = NULL;
#ifdef CTR_NATIVE
		if (next == NULL)
			return;
#endif
		if (next <= (u8 *)backBuffer->primMem.guardEnd)
		{
			p = (void *)next;
			backBuffer->primMem.cursor = next + sizeof(*p);
		}
	}
	if (p == nullptr)
	{
		return;
	}

	primCode.code = (RenderCode_Polygon << 5) | (1 << 4);

	ColorCode_SetPacked(&p->v[0].color, MakeColorPacked(91, 91, 0) | ((u32)primCode.code << 24));
	ColorCode_SetPacked(&p->v[1].color, MakeColorPacked(50, 43, 1) | ((u32)primCode.code << 24));
	ColorCode_SetPacked(&p->v[2].color, MakeColorPacked(255, 187, 0) | ((u32)primCode.code << 24));

	angle1 = (s16)mappedAngle;
	angle2 = angle1 + ANG(90);

	// NOTE(aalhendi): Q11 half-dimensions retain the original geometry. Keep the
	// fixed-point Y calculation wide until its signed pixel-offset phase.
	xLen = ((MATH_Sin(angle2) * halfNeedleWidth) >> 11);
	xLen += needleCenterX;
	p->v[2].pos.x = posX + xLen;

	yLen = ((MATH_Cos(angle2) * (halfNeedleWidth + 1)) >> 11) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yPixel = (s16)CTR_MipsSra(yLen, 9);
	yPixel += needleCenterY;
	p->v[2].pos.y = posY + yPixel;

	xLen = ((MATH_Sin(angle1) * halfNeedleWidth) >> 11);
	xLen -= needleCenterX;
	p->v[1].pos.x = posX - xLen;

	yLen = ((MATH_Cos(angle1) * halfNeedleWidth) >> 11) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yPixel = (s16)CTR_MipsSra(yLen, 9);
	yPixel -= needleCenterY;
	p->v[1].pos.y = posY - yPixel;

	xLen = ((MATH_Sin(angle1) * halfNeedleHeight) >> 11);
	xLen += needleCenterX;
	p->v[0].pos.x = posX + xLen;

	yLen = ((MATH_Cos(angle1) * halfNeedleHeight) >> 11) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yPixel = (s16)CTR_MipsSra(yLen, 9);
	yPixel += needleCenterY;
	p->v[0].pos.y = posY + yPixel;

	{
		u32 *ot = GAME_TRACKER->pushBuffer_UI.ptrOT;
		p->tag.self = *ot | (((sizeof(*p) - sizeof(Tag)) / sizeof(u32)) << 24);
		*ot = CtrGpu_PrimToOTLink24(p);
	}
	{
		struct DB *backBuffer = GAME_TRACKER->backBuffer;
		u8 *next = backBuffer->primMem.cursor;
		p = NULL;
#ifdef CTR_NATIVE
		if (next == NULL)
			return;
#endif
		if (next <= (u8 *)backBuffer->primMem.guardEnd)
		{
			p = (void *)next;
			backBuffer->primMem.cursor = next + sizeof(*p);
		}
	}
	if (p == nullptr)
	{
		return;
	}

	ColorCode_SetPacked(&p->v[0].color, MakeColorPacked(255, 255, 255) | ((u32)primCode.code << 24));
	ColorCode_SetPacked(&p->v[1].color, MakeColorPacked(156, 105, 0) | ((u32)primCode.code << 24));
	ColorCode_SetPacked(&p->v[2].color, MakeColorPacked(255, 255, 0) | ((u32)primCode.code << 24));

	xLen = ((MATH_Sin(angle2) * halfNeedleWidth) >> 11);
	xLen -= needleCenterX;
	p->v[2].pos.x = posX - xLen;

	yLen = ((MATH_Cos(angle2) * (halfNeedleWidth + 1)) >> 11) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yPixel = (s16)CTR_MipsSra(yLen, 9);
	yPixel -= needleCenterY;
	p->v[2].pos.y = posY - yPixel;

	xLen = ((MATH_Sin(angle1) * halfNeedleWidth) >> 11);
	xLen -= needleCenterX;
	p->v[1].pos.x = posX - xLen;

	yLen = ((MATH_Cos(angle1) * halfNeedleWidth) >> 11) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yPixel = (s16)CTR_MipsSra(yLen, 9);
	yPixel -= needleCenterY;
	p->v[1].pos.y = posY - yPixel;

	xLen = ((MATH_Sin(angle1) * halfNeedleHeight) >> 11);
	xLen += needleCenterX;
	p->v[0].pos.x = posX + xLen;

	yLen = ((MATH_Cos(angle1) * halfNeedleHeight) >> 11) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yPixel = (s16)CTR_MipsSra(yLen, 9);
	yPixel += needleCenterY;
	p->v[0].pos.y = posY + yPixel;

	{
		u32 *ot = GAME_TRACKER->pushBuffer_UI.ptrOT;
		p->tag.self = *ot | (((sizeof(*p) - sizeof(Tag)) / sizeof(u32)) << 24);
		*ot = CtrGpu_PrimToOTLink24(p);
	}
}

void UI_DrawSpeedBG(s16 posX, s16 posY, struct Driver *driver)
{
	typedef struct TPage_PolyG3
	{
		TPage t;
		PolyG3 p;
	} TPage_PolyG3;

	enum
	{
		pointCount = (s32)(sizeof(GAME_DATA_NAME(speedometerBG_vertData)) / (sizeof(Point) * 2)),
		gouraudQuadCode = (RenderCode_Polygon << 5) | (1 << 4) | (1 << 3)
	};
	const s16 xOffset = 480;
	const s16 yOffset = 190;
	s32 i;
	// NOTE(aalhendi): Each OT value is assigned only at a final link phase.
	// Its MIPS A3 live range ends at publication, before any further wire call.
	register u32 *ot CTR_PSX_REGISTER("$7");

	// NOTE(aalhendi): Retail callers supply the HUD position and driver,
	// but this background uses fixed screen coordinates.
	(void)posX;
	(void)posY;
	(void)driver;

	/* Draw the horizontal lines - they're a bit wider than the speedometer width */
	{
		for (i = 0; i < pointCount; i += 2)
		{
			s32 nextIndex = i + 1;
			Point *first = &GAME_DATA_NAME(speedometerBG_vertData)[1][i];
			Point *second = &GAME_DATA_NAME(speedometerBG_vertData)[1][nextIndex];
			CTR_Box_DrawWirePrims(first->x + xOffset, first->y + yOffset, second->x + xOffset, second->y + yOffset, 0xff, 0xff, 0xff,
			                      GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
			CTR_Box_DrawWirePrims(first->x + xOffset + 1, first->y + yOffset + 1, second->x + xOffset + 1, second->y + yOffset + 1, 0, 0, 0,
			                      GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
		}
	}

	/* Draw the vertical lines and colors */
	{
		for (i = 0; i < pointCount - 2; i += 2)
		{
			PolyG4 *p;
			u32 colorBottom, colorTop;
			Point *bottomLeft = &GAME_DATA_NAME(speedometerBG_vertData)[0][i];
			s32 topLeftIndex = i + 2;
			Point *topLeft = &GAME_DATA_NAME(speedometerBG_vertData)[0][topLeftIndex];
			CTR_Box_DrawWirePrims(bottomLeft->x + xOffset, bottomLeft->y + yOffset, topLeft->x + xOffset, topLeft->y + yOffset, 0xff, 0xff, 0xff,
			                      GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
			{
				s32 bottomRightIndex = i + 1;
				Point *bottomRight = &GAME_DATA_NAME(speedometerBG_vertData)[0][bottomRightIndex];
				s32 topRightIndex = i + 3;
				Point *topRight = &GAME_DATA_NAME(speedometerBG_vertData)[0][topRightIndex];
				CTR_Box_DrawWirePrims(bottomRight->x + xOffset, bottomRight->y + yOffset, topRight->x + xOffset, topRight->y + yOffset, 0xff, 0xff, 0xff,
				                      GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
				CTR_Box_DrawWirePrims(bottomLeft->x + xOffset + 1, bottomLeft->y + yOffset + 1, topLeft->x + xOffset + 1, topLeft->y + yOffset + 1, 0, 0, 0,
				                      GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
				CTR_Box_DrawWirePrims(bottomRight->x + xOffset + 1, bottomRight->y + yOffset + 1, topRight->x + xOffset + 1, topRight->y + yOffset + 1, 0, 0, 0,
				                      GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
			}
			{
				struct DB *backBuffer = GAME_TRACKER->backBuffer;
				u8 *next = backBuffer->primMem.cursor;
				p = NULL;
#ifdef CTR_NATIVE
				if (next == NULL)
					return;
#endif
				if (next <= (u8 *)backBuffer->primMem.guardEnd)
				{
					p = (void *)next;
					backBuffer->primMem.cursor = next + sizeof(*p);
				}
			}
			if (p == nullptr)
			{
				return;
			}

			if (i < 3)
			{
				colorBottom = 0x00b500;
				colorTop = 0x00b500;
			}
			else if (i < 5)
			{
				colorBottom = 0x00b500;
				colorTop = 0x00d1ff;
			}
			else if (i < 7)
			{
				colorBottom = 0x00d1ff;
				colorTop = 0x0000db;
			}
			else
			{
				colorBottom = 0x0000db;
				colorTop = 0x0000db;
			}
			ColorCode_SetPacked(&p->v[0].color, colorBottom | ((u32)gouraudQuadCode << 24));
			ColorCode_SetPacked(&p->v[1].color, colorBottom | ((u32)gouraudQuadCode << 24));
			ColorCode_SetPacked(&p->v[2].color, colorTop | ((u32)gouraudQuadCode << 24));
			ColorCode_SetPacked(&p->v[3].color, colorTop | ((u32)gouraudQuadCode << 24));
			p->v[0].pos.x = GAME_DATA_NAME(speedometerBG_vertData)[0][i + 0].x + xOffset;
			p->v[0].pos.y = GAME_DATA_NAME(speedometerBG_vertData)[0][i + 0].y + yOffset;
			p->v[1].pos.x = GAME_DATA_NAME(speedometerBG_vertData)[0][i + 1].x + xOffset;
			p->v[1].pos.y = GAME_DATA_NAME(speedometerBG_vertData)[0][i + 1].y + yOffset;
			p->v[2].pos.x = GAME_DATA_NAME(speedometerBG_vertData)[0][i + 2].x + xOffset;
			p->v[2].pos.y = GAME_DATA_NAME(speedometerBG_vertData)[0][i + 2].y + yOffset;
			p->v[3].pos.x = GAME_DATA_NAME(speedometerBG_vertData)[0][i + 3].x + xOffset;
			p->v[3].pos.y = GAME_DATA_NAME(speedometerBG_vertData)[0][i + 3].y + yOffset;
			{
				ot = GAME_TRACKER->pushBuffer_UI.ptrOT;
				p->tag.self = *ot | (((sizeof(*p) - sizeof(Tag)) / sizeof(u32)) << 24);
				*ot = CtrGpu_PrimToOTLink24(p);
			}
		}
	}

	/* Draw transparent background */
	{
		for (i = 0; i < pointCount - 2; i += 2)
		{
			TPage_PolyG3 *p;
			PrimCode primCode;
			{
				struct DB *backBuffer = GAME_TRACKER->backBuffer;
				u8 *next = backBuffer->primMem.cursor;
				p = NULL;
#ifdef CTR_NATIVE
				if (next == NULL)
					return;
#endif
				if (next <= (u8 *)backBuffer->primMem.guardEnd)
				{
					p = (void *)next;
					backBuffer->primMem.cursor = next + sizeof(*p);
				}
			}
			if (p == nullptr)
			{
				return;
			}

			{
				s32 pointIndex = i + 1;
				Point *point = &GAME_DATA_NAME(speedometerBG_vertData)[0][pointIndex];
				p->p.v[0].pos.x = point->x + xOffset;
				p->p.v[0].pos.y = point->y + yOffset;
			}
			{
				s32 pointIndex = i + 3;
				Point *point = &GAME_DATA_NAME(speedometerBG_vertData)[0][pointIndex];
				p->p.v[1].pos.x = point->x + xOffset;
				p->p.v[1].pos.y = point->y + yOffset;
			}
			p->p.v[2].pos.x = GAME_DATA_NAME(speedometerBG_vertData)[0][pointCount - 1].x + xOffset;
			p->p.v[2].pos.y = GAME_DATA_NAME(speedometerBG_vertData)[0][1].y + yOffset;

			p->t.tag.bits.size = (sizeof(*p) - sizeof(Tag)) / sizeof(u32);
			primCode.code = (RenderCode_Polygon << 5) | (1 << 4) | (1 << 1);
			p->p.gPolyCode = primCode;
			// NOTE(aalhendi): Secondary color padding is not another command;
			// retail writes RGB only and the GPU ignores those two bytes.
			p->p.v[1].color.r = 0;
			p->p.v[1].color.g = 0;
			p->p.v[1].color.b = 0;
			p->p.v[0].color.r = 0;
			p->p.v[0].color.g = 0;
			p->p.v[0].color.b = 0;
			p->p.v[2].color.r = 0;
			p->p.v[2].color.g = 0;
			p->p.v[2].color.b = 0;
			p->t.texpage.self = 0xe1000a00;
			p->p.tag.self = 0;

// NOTE(aalhendi): CTR_NATIVE keeps PsyCross display-area drawing enabled.
#ifdef CTR_NATIVE
			((TPage *)p)->texpage.bits.drawDisplayArea = 1;
#endif

			{
				ot = GAME_TRACKER->pushBuffer_UI.ptrOT;
				p->t.tag.self = *ot | (((sizeof(*p) - sizeof(Tag)) / sizeof(u32)) << 24);
				*ot = CtrGpu_PrimToOTLink24(p);
			}
		}
	}
}
