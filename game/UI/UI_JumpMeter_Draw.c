#include <common.h>

enum
{
	UI_JUMP_METER_TIME_UNIT = 0x3c0,
	UI_JUMP_METER_TENTH_PRE_DIVISOR = 6,
	UI_JUMP_METER_TENTH_SHIFT = 4,
	UI_JUMP_METER_TENTH_UNIT = 0x60,
	UI_JUMP_METER_PERCENT_SCALE = 100,
	UI_JUMP_METER_NUMBER_Y_OFFSET = -45,
	UI_JUMP_METER_NUMBER_Y_BIAS = 2,
	UI_JUMP_METER_SECONDS_X_OFFSET = -0x10,
	UI_JUMP_METER_TENTHS_X_OFFSET = -4,
	UI_JUMP_METER_HUNDREDTHS_X_OFFSET = 4,
	UI_JUMP_METER_NUMBER_BOX_X_OFFSET = -0x14,
	UI_JUMP_METER_NUMBER_BOX_W = 0x22,
	UI_JUMP_METER_NUMBER_BOX_H = 10,
	UI_JUMP_METER_NUMBER_FILL_RIGHT_OFFSET = 14,
	UI_JUMP_METER_BAR_W = 0xc,
	UI_JUMP_METER_BAR_H = 38,
	UI_JUMP_METER_POLY_F4_OT_TAG = 0x5000000,
	UI_JUMP_METER_NUMBER_FILL_COLOR = 0x28ffffff,
	UI_JUMP_METER_BAR_BLUE = 0x28ff0000,
	UI_JUMP_METER_BAR_GREEN = 0x2800ff00,
	UI_JUMP_METER_BAR_YELLOW = 0x2800ffff,
	UI_JUMP_METER_BAR_RED = 0x280000ff,
	UI_JUMP_METER_BAR_EMPTY_COLOR = 0x28808080,
};


void UI_JumpMeter_Draw(s16 posX, s16 posY, struct Driver *driver)
{
	s16 currentJumpMeter;
	s32 wholeSeconds;
	u32 *primMemCursor;
	POLY_F4 *p;
	s32 numbersY;
	s16 numberBoxX;
	s16 numberBoxY;
	s16 numberFillRightX;
	s16 barRightX;
	s16 barTopY;
	s32 tenths;
	s32 posXInt;
	RECT box;
	Color color;
	s32 jumpMeterHeight;
	s32 jumpMeterRemainder;
	s32 hundredths;

	wholeSeconds = ((s32)driver->jumpMeter / UI_JUMP_METER_TIME_UNIT) * 0x10000 >> 0x10;
	jumpMeterRemainder = (s32)driver->jumpMeter + wholeSeconds * -UI_JUMP_METER_TIME_UNIT;
	tenths =
	    (((jumpMeterRemainder / UI_JUMP_METER_TENTH_PRE_DIVISOR + (jumpMeterRemainder >> 0x1f)) >> UI_JUMP_METER_TENTH_SHIFT) - (jumpMeterRemainder >> 0x1f)) *
	        0x10000 >>
	    0x10;
	posXInt = (s32)posX;
	numbersY = (s32)posY + UI_JUMP_METER_NUMBER_Y_OFFSET + UI_JUMP_METER_NUMBER_Y_BIAS;
	hundredths = ((jumpMeterRemainder + tenths * -UI_JUMP_METER_TENTH_UNIT) * UI_JUMP_METER_PERCENT_SCALE) / UI_JUMP_METER_TIME_UNIT;

	DebugFont_DrawNumbers(wholeSeconds, posXInt + UI_JUMP_METER_SECONDS_X_OFFSET, numbersY);
	DebugFont_DrawNumbers(tenths, posXInt + UI_JUMP_METER_TENTHS_X_OFFSET, numbersY);
	DebugFont_DrawNumbers(hundredths * 0x10000 >> 0x10, posXInt + UI_JUMP_METER_HUNDREDTHS_X_OFFSET, numbersY);

	numberBoxX = posX + UI_JUMP_METER_NUMBER_BOX_X_OFFSET;
	numberBoxY = posY + UI_JUMP_METER_NUMBER_Y_OFFSET;
	box.x = numberBoxX;
	box.y = numberBoxY;
	box.w = UI_JUMP_METER_NUMBER_BOX_W;
	box.h = UI_JUMP_METER_NUMBER_BOX_H;

	memset(&color, 0, sizeof(color));
	CTR_Box_DrawWireBox(&box, &color, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);

	{
		struct DB *allocDB;
		u32 *allocCursor;

		allocDB = GAME_TRACKER->backBuffer;
		allocCursor = allocDB->primMem.cursor;
		p = 0;

		if (allocCursor <= (u32 *)allocDB->primMem.guardEnd)
		{
			p = (POLY_F4 *)allocCursor;
			allocDB->primMem.cursor = (u32 *)(p + 1);
		}
	}

	if (p != 0)
	{
		numberFillRightX = posX + UI_JUMP_METER_NUMBER_FILL_RIGHT_OFFSET;
		CtrGpu_WriteColorCode(&p->r0, UI_JUMP_METER_NUMBER_FILL_COLOR);
		p->x0 = numberBoxX;
		p->y0 = numberBoxY;
		p->x1 = numberFillRightX;
		p->y1 = numberBoxY;
		p->x2 = numberBoxX;
		p->y2 = posY + UI_JUMP_METER_NUMBER_Y_OFFSET + UI_JUMP_METER_NUMBER_BOX_H;
		p->x3 = numberFillRightX;
		p->y3 = posY + UI_JUMP_METER_NUMBER_Y_OFFSET + UI_JUMP_METER_NUMBER_BOX_H;

		primMemCursor = GAME_TRACKER->pushBuffer_UI.ptrOT;

		p->tag = (*primMemCursor | UI_JUMP_METER_POLY_F4_OT_TAG);
		*primMemCursor = CtrGpu_PrimToOTLink24(p);

		{
			// NOTE(aalhendi): Keep the initialized RECT before its Color in this
			// call scope; their lifetimes determine the retail stack layout.
			RECT box2 = {posX, posY - UI_JUMP_METER_BAR_H, UI_JUMP_METER_BAR_W, UI_JUMP_METER_BAR_H};
			Color color2;

			memset(&color2, 0, sizeof(color2));
			CTR_Box_DrawWireBox(&box2, &color2, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
		}

		{
			struct DB *allocDB;
			u32 *allocCursor;

			allocDB = GAME_TRACKER->backBuffer;
			allocCursor = allocDB->primMem.cursor;
			p = 0;

			if (allocCursor <= (u32 *)allocDB->primMem.guardEnd)
			{
				p = (POLY_F4 *)allocCursor;
				allocDB->primMem.cursor = (u32 *)(p + 1);
			}
		}

		if (p != 0)
		{
			currentJumpMeter = driver->jumpMeter;
			if (currentJumpMeter < UI_JUMP_METER_HANG_TIME_SMALL)
			{
				CtrGpu_WriteColorCode(&p->r0, UI_JUMP_METER_BAR_BLUE);
			}
			else
			{
				if (currentJumpMeter < UI_JUMP_METER_HANG_TIME_MEDIUM)
				{
					CtrGpu_WriteColorCode(&p->r0, UI_JUMP_METER_BAR_GREEN);
				}
				else
				{
					if (currentJumpMeter < UI_JUMP_METER_HANG_TIME_LARGE)
					{
						CtrGpu_WriteColorCode(&p->r0, UI_JUMP_METER_BAR_YELLOW);
					}
					else
					{
						CtrGpu_WriteColorCode(&p->r0, UI_JUMP_METER_BAR_RED);
					}
				}
			}
			jumpMeterHeight = (s32)currentJumpMeter * UI_JUMP_METER_BAR_H;
			barRightX = posX + UI_JUMP_METER_BAR_W;
			p->x0 = posX;
			p->x1 = barRightX;
			p->x2 = posX;
			p->y2 = posY;
			p->x3 = barRightX;
			p->y3 = posY;
			barTopY = posY - ((s16)(jumpMeterHeight / UI_JUMP_METER_MAX));
			p->y0 = barTopY;
			p->y1 = barTopY;

			primMemCursor = GAME_TRACKER->pushBuffer_UI.ptrOT;

			p->tag = (*primMemCursor | UI_JUMP_METER_POLY_F4_OT_TAG);
			*primMemCursor = CtrGpu_PrimToOTLink24(p);

			{
				struct DB *allocDB;
				u32 *allocCursor;

				allocDB = GAME_TRACKER->backBuffer;
				allocCursor = allocDB->primMem.cursor;
				p = 0;

				if (allocCursor <= (u32 *)allocDB->primMem.guardEnd)
				{
					p = (POLY_F4 *)allocCursor;
					allocDB->primMem.cursor = (u32 *)(p + 1);
				}
			}

			if (p != 0)
			{
				CtrGpu_WriteColorCode(&p->r0, UI_JUMP_METER_BAR_EMPTY_COLOR);
				p->x0 = posX;
				p->y0 = posY - UI_JUMP_METER_BAR_H;
				p->x1 = barRightX;
				p->y1 = posY - UI_JUMP_METER_BAR_H;
				p->x2 = posX;
				p->y2 = posY;
				p->x3 = barRightX;
				p->y3 = posY;

				primMemCursor = GAME_TRACKER->pushBuffer_UI.ptrOT;

				p->tag = (*primMemCursor | UI_JUMP_METER_POLY_F4_OT_TAG);
				*primMemCursor = CtrGpu_PrimToOTLink24(p);
			}
		}
	}
	return;
}
