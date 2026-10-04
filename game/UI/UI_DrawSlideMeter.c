#include <common.h>

enum
{
	UI_SLIDE_METER_BAR_W = 49,
	UI_SLIDE_METER_BAR_H_FULL = 7,
	UI_SLIDE_METER_BAR_H_SPLIT = 3,
	UI_SLIDE_METER_SPLIT_PLAYER_COUNT = 3,
	UI_SLIDE_METER_READY_COLOR = 0x280000ff,
	UI_SLIDE_METER_WAIT_COLOR = 0x2800ff00,
	UI_SLIDE_METER_EMPTY_COLOR = 0x28808080,
	UI_SLIDE_METER_POLY_F4_OT_TAG = 0x05000000,
};

void UI_DrawSlideMeter(s16 posX, s16 posY, struct Driver *driver)
{
	s32 barHeight;
	s32 meterLength;
	s16 leftX;
	s16 topY;
	RECT box;
	Color black;
	struct DB *backDB;
	u32 *primMemCursor;
	u32 *ot;
	POLY_F4 *p;

	meterLength = 0;
	barHeight = UI_SLIDE_METER_BAR_H_FULL;
	if (GAME_TRACKER->numPlyrCurrGame >= UI_SLIDE_METER_SPLIT_PLAYER_COUNT)
	{
		barHeight = UI_SLIDE_METER_BAR_H_SPLIT;
	}
	if (driver->turbo_MeterRoomLeft != 0)
	{
		meterLength = UI_SLIDE_METER_BAR_W - (driver->turbo_MeterRoomLeft * UI_SLIDE_METER_BAR_W) / (driver->const_turboMaxRoom * ELAPSED_MS);
	}

	box.x = posX - UI_SLIDE_METER_BAR_W;
	box.y = posY - barHeight;
	box.w = UI_SLIDE_METER_BAR_W;
	box.h = barHeight;
	memset(&black, 0, sizeof(black));
	CTR_Box_DrawWireBox(&box, &black, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);

	backDB = GAME_TRACKER->backBuffer;
	primMemCursor = backDB->primMem.cursor;
	p = 0;
	// NOTE(aalhendi): Retail accepts equality; the reserved tail holds the packet.
	if (primMemCursor <= (u32 *)backDB->primMem.guardEnd)
	{
		p = (POLY_F4 *)primMemCursor;
		backDB->primMem.cursor = (u32 *)(p + 1);
	}
	if (p != 0)
	{
		// NOTE(aalhendi): Branch-local stores preserve the old compiler's retail control flow.
		if (driver->const_turboLowRoomWarning * ELAPSED_MS < driver->turbo_MeterRoomLeft)
		{
			CtrGpu_WriteColorCode(&p->r0, UI_SLIDE_METER_WAIT_COLOR);
		}
		else
		{
			CtrGpu_WriteColorCode(&p->r0, UI_SLIDE_METER_READY_COLOR);
		}
		leftX = posX - meterLength;
		topY = posY - barHeight;
		p->x0 = leftX;
		p->y0 = topY;
		p->x1 = posX;
		p->y1 = topY;
		p->y2 = posY;
		p->x3 = posX;
		p->y3 = posY;
		p->x2 = leftX;
		ot = GAME_TRACKER->pushBuffer_UI.ptrOT;
		p->tag = *ot | UI_SLIDE_METER_POLY_F4_OT_TAG;
		*ot = CtrGpu_PrimToOTLink24(p);

		// NOTE(aalhendi): Exhausting the empty quad must retain the filled quad already linked.
		backDB = GAME_TRACKER->backBuffer;
		primMemCursor = backDB->primMem.cursor;
		p = 0;
		if (primMemCursor <= (u32 *)backDB->primMem.guardEnd)
		{
			p = (POLY_F4 *)primMemCursor;
			backDB->primMem.cursor = (u32 *)(p + 1);
		}
		if (p != 0)
		{
			CtrGpu_WriteColorCode(&p->r0, UI_SLIDE_METER_EMPTY_COLOR);
			leftX = posX - UI_SLIDE_METER_BAR_W;
			p->x0 = leftX;
			p->y0 = topY;
			p->x1 = posX;
			p->y1 = topY;
			p->y2 = posY;
			p->x3 = posX;
			p->y3 = posY;
			p->x2 = leftX;
			ot = GAME_TRACKER->pushBuffer_UI.ptrOT;
			p->tag = *ot | UI_SLIDE_METER_POLY_F4_OT_TAG;
			*ot = CtrGpu_PrimToOTLink24(p);
		}
	}
}
