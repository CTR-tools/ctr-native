#include <common.h>
#include <widescreen.h>

enum
{
	UI_JUMP_METER_SIGNIFICANT_THRESHOLD = 0x150,
	UI_JUMP_METER_HANG_TIME_SMALL = 640,
	UI_JUMP_METER_HANG_TIME_MEDIUM = 960,
	UI_JUMP_METER_HANG_TIME_LARGE = 1440,
	UI_JUMP_METER_HANG_TIME_RESERVES = 960,
	UI_JUMP_METER_HANG_TIME_FIRE_SMALL = 0,
	UI_JUMP_METER_HANG_TIME_FIRE_MEDIUM = 0x80,
	UI_JUMP_METER_HANG_TIME_FIRE_LARGE = 0x100,
	UI_JUMP_METER_LANDING_VOICELINE_MIN = 0x480,
	UI_JUMP_METER_LANDING_VOICELINE_CUTOFF = 0x481,
	UI_JUMP_METER_LANDING_VOICELINE_ID = 7,
	UI_JUMP_METER_LANDING_VOICELINE_FLAGS = 0x10,
	UI_JUMP_METER_MAX = 0x960,
	UI_JUMP_METER_DISPLAY_TIMER = 0x5a0,
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
	UI_JUMP_METER_BAR_RED = 0x28ff0000,
	UI_JUMP_METER_BAR_GREEN = 0x2800ff00,
	UI_JUMP_METER_BAR_CYAN = 0x2800ffff,
	UI_JUMP_METER_BAR_BLUE = 0x280000ff,
	UI_JUMP_METER_BAR_EMPTY_COLOR = 0x28808080,
	UI_SLIDE_METER_BAR_W = 49,
	UI_SLIDE_METER_BAR_H_FULL = 7,
	UI_SLIDE_METER_BAR_H_SPLIT = 3,
	UI_SLIDE_METER_SPLIT_PLAYER_COUNT = 3,
	UI_SLIDE_METER_PRIM_COUNT = 2,
	UI_SLIDE_METER_READY_R = 0xff,
	UI_SLIDE_METER_READY_G = 0,
	UI_SLIDE_METER_READY_B = 0,
	UI_SLIDE_METER_WAIT_R = 0,
	UI_SLIDE_METER_WAIT_G = 0xff,
	UI_SLIDE_METER_WAIT_B = 0,
	UI_SLIDE_METER_EMPTY_R = 0x80,
	UI_SLIDE_METER_EMPTY_G = 0x80,
	UI_SLIDE_METER_EMPTY_B = 0x80,
};


void UI_JumpMeter_Update(struct Driver *driver)
{
	if ((driver->actionsFlagSet & ACTION_AIRBORNE) == 0)
	{
		if ((driver->actionsFlagSetPrevFrame & ACTION_AIRBORNE) == 0)
		{
			if (driver->jumpMeterTimer == 0)
			{
				driver->jumpMeter = 0;
			}
			else
			{
				driver->jumpMeterTimer -= sdata->gGT->elapsedTimeMS;
				if (driver->jumpMeterTimer < 0)
				{
					driver->jumpMeterTimer = 0;
				}
			}
		}
		else
		{
			if (UI_JUMP_METER_SIGNIFICANT_THRESHOLD < driver->jumpMeter)
			{
				driver->timeSpentJumping += driver->jumpMeter;
			}

			if (driver->highestJump < driver->jumpMeter)
			{
				driver->highestJump = driver->jumpMeter;
			}

			if (driver->jumpMeter >= UI_JUMP_METER_HANG_TIME_SMALL)
			{
				int fireLevel = UI_JUMP_METER_HANG_TIME_FIRE_SMALL;
				if (driver->jumpMeter >= UI_JUMP_METER_HANG_TIME_MEDIUM)
				{
					fireLevel = UI_JUMP_METER_HANG_TIME_FIRE_MEDIUM;
				}
				if (driver->jumpMeter >= UI_JUMP_METER_HANG_TIME_LARGE)
				{
					fireLevel = UI_JUMP_METER_HANG_TIME_FIRE_LARGE;
				}

				VehFire_Increment(driver, UI_JUMP_METER_HANG_TIME_RESERVES, POWER_SLIDE_HANG_TIME, fireLevel);
			}
		}
	}
	else
	{
		if ((UI_JUMP_METER_LANDING_VOICELINE_MIN < driver->jump_LandingBoost) && (driver->jumpMeter < UI_JUMP_METER_LANDING_VOICELINE_CUTOFF))
		{
			Voiceline_RequestPlay(UI_JUMP_METER_LANDING_VOICELINE_ID, data.characterIDs[driver->driverID], UI_JUMP_METER_LANDING_VOICELINE_FLAGS);
		}

		driver->jumpMeter = driver->jump_LandingBoost;

		if (UI_JUMP_METER_MAX < driver->jump_LandingBoost)
		{
			driver->jumpMeter = UI_JUMP_METER_MAX;
		}

		driver->jumpMeterTimer = UI_JUMP_METER_DISPLAY_TIMER;
	}
	return;
}

void UI_JumpMeter_Draw(s16 posX, s16 posY, struct Driver *driver)
{
	struct GameTracker *gGT = sdata->gGT;
	u32 barColorAndCode;
	s16 currentJumpMeter;
	struct DB *backDB;
	int wholeSeconds;
	u32 *primMemCursor;
	POLY_F4 *p;
	int numbersY;
	s16 numberBoxX;
	s16 barRightX;
	s16 barTopY;
	int tenths;
	int posXInt;
	RECT box;
	RECT box2;
	int jumpMeterHeight;
	int jumpMeterRemainder;

	wholeSeconds = ((int)driver->jumpMeter / UI_JUMP_METER_TIME_UNIT) * 0x10000 >> 0x10;
	jumpMeterRemainder = (int)driver->jumpMeter + wholeSeconds * -UI_JUMP_METER_TIME_UNIT;
	tenths =
	    (((jumpMeterRemainder / UI_JUMP_METER_TENTH_PRE_DIVISOR + (jumpMeterRemainder >> 0x1f)) >> UI_JUMP_METER_TENTH_SHIFT) - (jumpMeterRemainder >> 0x1f)) *
	        0x10000 >>
	    0x10;
	posXInt = (int)posX;
	numbersY = (int)posY + UI_JUMP_METER_NUMBER_Y_OFFSET + UI_JUMP_METER_NUMBER_Y_BIAS;

	DebugFont_DrawNumbers(wholeSeconds, posXInt + Widescreen_ScaleWidth(UI_JUMP_METER_SECONDS_X_OFFSET), numbersY);
	DebugFont_DrawNumbers(tenths, posXInt + Widescreen_ScaleWidth(UI_JUMP_METER_TENTHS_X_OFFSET), numbersY);
	DebugFont_DrawNumbers((((jumpMeterRemainder + tenths * -UI_JUMP_METER_TENTH_UNIT) * UI_JUMP_METER_PERCENT_SCALE) / UI_JUMP_METER_TIME_UNIT) * 0x10000 >>
	                          0x10,
	                      posXInt + Widescreen_ScaleWidth(UI_JUMP_METER_HUNDREDTHS_X_OFFSET), numbersY);

	numberBoxX = posX + Widescreen_ScaleWidth(UI_JUMP_METER_NUMBER_BOX_X_OFFSET);
	box.w = Widescreen_ScaleWidth(UI_JUMP_METER_NUMBER_FILL_RIGHT_OFFSET) - Widescreen_ScaleWidth(UI_JUMP_METER_NUMBER_BOX_X_OFFSET);
	box.h = UI_JUMP_METER_NUMBER_BOX_H;
	box.x = numberBoxX;
	box.y = posY + UI_JUMP_METER_NUMBER_Y_OFFSET;

	Color color;
	ColorCode_SetPacked(&color, data.colors[BLACK][0]);
	CTR_Box_DrawWireBox(&box, &color, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);

	backDB = gGT->backBuffer;
	primMemCursor = backDB->primMem.cursor;
	p = 0;

	if (primMemCursor <= (u32 *)backDB->primMem.guardEnd)
	{
		backDB->primMem.cursor = &primMemCursor[6];
		p = (POLY_F4 *)primMemCursor;
	}

	if (p != 0)
	{
		CtrGpu_WriteColorCode(&p->r0, UI_JUMP_METER_NUMBER_FILL_COLOR);
		p->x1 = posX + Widescreen_ScaleWidth(UI_JUMP_METER_NUMBER_FILL_RIGHT_OFFSET);
		p->x3 = p->x1;
		p->x0 = box.x;
		p->y0 = box.y;
		p->y1 = box.y;
		p->x2 = box.x;
		p->y2 = box.y + UI_JUMP_METER_NUMBER_BOX_H;
		p->y3 = box.y + UI_JUMP_METER_NUMBER_BOX_H;

		primMemCursor = gGT->pushBuffer_UI.ptrOT;

		p->tag = CtrGpu_PackOTTag(*primMemCursor, UI_JUMP_METER_POLY_F4_OT_TAG);
		*primMemCursor = CtrGpu_PrimToOTLink24(p);

		box2.y = posY - UI_JUMP_METER_BAR_H;
		box2.w = Widescreen_ScaleWidth(UI_JUMP_METER_BAR_W);
		box2.h = UI_JUMP_METER_BAR_H;
		box2.x = posX;

		CTR_Box_DrawWireBox(&box2, &color, gGT->pushBuffer_UI.ptrOT, &backDB->primMem);

		backDB = gGT->backBuffer;
		primMemCursor = backDB->primMem.cursor;
		p = 0;

		if (primMemCursor <= (u32 *)backDB->primMem.guardEnd)
		{
			backDB->primMem.cursor = &primMemCursor[6];
			p = (POLY_F4 *)primMemCursor;
		}

		if (p != 0)
		{
			currentJumpMeter = driver->jumpMeter;
			barColorAndCode = UI_JUMP_METER_BAR_RED;
			if (UI_JUMP_METER_HANG_TIME_SMALL <= currentJumpMeter)
			{
				if (currentJumpMeter < UI_JUMP_METER_HANG_TIME_MEDIUM)
				{
					barColorAndCode = UI_JUMP_METER_BAR_GREEN;
				}
				else
				{
					if (currentJumpMeter < UI_JUMP_METER_HANG_TIME_LARGE)
					{
						barColorAndCode = UI_JUMP_METER_BAR_CYAN;
					}
					else
					{
						barColorAndCode = UI_JUMP_METER_BAR_BLUE;
					}
				}
			}
			CtrGpu_WriteColorCode(&p->r0, barColorAndCode);
			jumpMeterHeight = (int)currentJumpMeter * UI_JUMP_METER_BAR_H;
			barRightX = posX + box2.w;
			p->x0 = posX;
			p->x1 = barRightX;
			p->x2 = posX;
			p->y2 = posY;
			p->x3 = barRightX;
			p->y3 = posY;
			barTopY = posY - ((s16)(jumpMeterHeight / UI_JUMP_METER_MAX));
			p->y0 = barTopY;
			p->y1 = barTopY;

			primMemCursor = gGT->pushBuffer_UI.ptrOT;

			p->tag = CtrGpu_PackOTTag(*primMemCursor, UI_JUMP_METER_POLY_F4_OT_TAG);
			*primMemCursor = CtrGpu_PrimToOTLink24(p);

			backDB = gGT->backBuffer;
			primMemCursor = backDB->primMem.cursor;
			p = 0;

			if (primMemCursor <= (u32 *)backDB->primMem.guardEnd)
			{
				backDB->primMem.cursor = &primMemCursor[6];
				p = (POLY_F4 *)primMemCursor;
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

				primMemCursor = gGT->pushBuffer_UI.ptrOT;

				p->tag = CtrGpu_PackOTTag(*primMemCursor, UI_JUMP_METER_POLY_F4_OT_TAG);
				*primMemCursor = CtrGpu_PrimToOTLink24(p);
			}
		}
	}
	return;
}

void UI_DrawSlideMeter(s16 posX, s16 posY, struct Driver *driver)
{
	const struct GameTracker *gGT = sdata->gGT;
	const int barWidth = UI_SLIDE_METER_BAR_W;
	const int displayWidth = barWidth - Widescreen_FixedEdgeShift(barWidth);
	int barHeight = gGT->numPlyrCurrGame >= UI_SLIDE_METER_SPLIT_PLAYER_COUNT ? UI_SLIDE_METER_BAR_H_SPLIT : UI_SLIDE_METER_BAR_H_FULL;
	if (gGT->numPlyrCurrGame >= UI_SLIDE_METER_SPLIT_PLAYER_COUNT && (driver->driverID & 1) != 0)
	{
		// Right-side viewports anchor the bar's left edge beside the lap count.
		posX -= barWidth - displayWidth;
	}

	int meterLength = 0;
	if (driver->turbo_MeterRoomLeft != 0)
	{
		int currentRoomRemaining = driver->turbo_MeterRoomLeft * barWidth;
		int maxRoom = driver->const_turboMaxRoom * ELAPSED_MS;
		meterLength = barWidth - (currentRoomRemaining / maxRoom);
	}

	RECT box;
	s16 topX = posX - displayWidth;
	s16 topY = posY - barHeight;
	box.x = topX;
	box.y = topY;
	box.w = displayWidth;
	box.h = barHeight;
	Color black = MakeColor(0, 0, 0);
	CTR_Box_DrawWireBox(&box, &black, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);

	const PrimCode primCode = {.kind.poly = {.quad = 1, .renderCode = RenderCode_Polygon}};
	ColorCode colorCode = MakeColorCode(UI_SLIDE_METER_READY_R, UI_SLIDE_METER_READY_G, UI_SLIDE_METER_READY_B, primCode);

	if (driver->const_turboLowRoomWarning * ELAPSED_MS < driver->turbo_MeterRoomLeft)
	{
		colorCode = MakeColorCode(UI_SLIDE_METER_WAIT_R, UI_SLIDE_METER_WAIT_G, UI_SLIDE_METER_WAIT_B, primCode);
	}

	for (int i = 0; i < UI_SLIDE_METER_PRIM_COUNT; i++)
	{
		PolyF4 *p;
		GetPrimMem(p);
		if (p == nullptr)
		{
			return;
		}

		p->colorCode = colorCode;

		p->v[0].pos.y = topY;
		p->v[1].pos.y = topY;
		p->v[2].pos.y = posY;
		p->v[3].pos.y = posY;

		int leftX = posX - meterLength + Widescreen_FixedEdgeShift(meterLength);
		p->v[0].pos.x = leftX;
		p->v[1].pos.x = posX;
		p->v[2].pos.x = leftX;
		p->v[3].pos.x = posX;

		AddPrimitive(p, gGT->pushBuffer_UI.ptrOT);
		colorCode = MakeColorCode(UI_SLIDE_METER_EMPTY_R, UI_SLIDE_METER_EMPTY_G, UI_SLIDE_METER_EMPTY_B, primCode);
		meterLength = barWidth;
	}
}

// Ported from CTR-ModSDK's Retro-Fueled_and_ReservesMeter mod pack.
void UI_DrawReservesMeter(s16 posX, s16 posY, struct Driver *driver)
{
	const struct GameTracker *gGT = sdata->gGT;
	const int barWidth = 49;
	const int displayWidth = barWidth - Widescreen_FixedEdgeShift(barWidth);
	const int barHeight = 3;
	s16 topX = posX - displayWidth;
	s16 topY = posY - barHeight;

	RECT box = {topX, topY, displayWidth, barHeight};
	Color black = MakeColor(0, 0, 0);
	CTR_Box_DrawWireBox(&box, &black, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);

	const PrimCode primCode = {.kind.poly = {.quad = 1, .renderCode = RenderCode_Polygon}};
	ColorCode colorCode = MakeColorCode(0xff, 0, 0, primCode);
	if (driver->reserves > 1600)
	{
		colorCode = driver->reserves < 3840
		                ? MakeColorCode(0xff, 0xff, 0, primCode)
		                : MakeColorCode(0, 0xff, 0, primCode);
	}

	int meterLength = (driver->reserves * 14) / 2400;
	if (meterLength > barWidth)
	{
		meterLength = barWidth;
		colorCode = MakeColorCode(0, 0, 0xff, primCode);
	}

	for (int i = 0; i < 2; i++)
	{
		PolyF4 *p;
		GetPrimMem(p);
		if (p == NULL)
		{
			return;
		}

		p->colorCode = colorCode;
		p->v[0].pos.y = topY;
		p->v[1].pos.y = topY;
		p->v[2].pos.y = posY;
		p->v[3].pos.y = posY;
		int leftX = posX - meterLength + Widescreen_FixedEdgeShift(meterLength);
		p->v[0].pos.x = leftX;
		p->v[1].pos.x = posX;
		p->v[2].pos.x = leftX;
		p->v[3].pos.x = posX;

		AddPrimitive(p, gGT->pushBuffer_UI.ptrOT);
		colorCode = MakeColorCode(0x80, 0x80, 0x80, primCode);
		meterLength = barWidth;
	}
}
