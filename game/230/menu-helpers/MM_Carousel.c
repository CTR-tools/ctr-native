#include <common.h>
#include "MM_Carousel.h"
#include <track_text.h>

enum { MM_CAROUSEL_CENTER_ROW = 4, MM_CAROUSEL_CHANGE_FRAMES = 3 };

static bool MM_Carousel_Enabled(const struct MM_CarouselSource *source, int index)
{
	return source->enabled == NULL || source->enabled(source->context, index);
}

static int MM_Carousel_Find(const struct MM_CarouselSource *source, int index, int direction)
{
	for (int i = 0; i < source->count; i++)
	{
		index = (index + direction + source->count) % source->count;
		if (MM_Carousel_Enabled(source, index)) { return index; }
	}
	return -1;
}

void MM_Carousel_Init(struct MM_Carousel *carousel, const struct MM_CarouselSource *source, int selected, u32 frame)
{
	memset(carousel, 0, sizeof(*carousel));
	carousel->selected = -1;
	carousel->next = -1;
	carousel->scrollStart = frame;
	if (source == NULL || source->count <= 0) { return; }
	if (selected < 0 || selected >= source->count) { selected = 0; }
	if (!MM_Carousel_Enabled(source, selected)) { selected = MM_Carousel_Find(source, selected, 1); }
	carousel->selected = selected;
	carousel->next = selected;
	if (selected >= 0 && source->selected != NULL) { source->selected(source->context, selected); }
}

bool MM_Carousel_Update(struct MM_Carousel *carousel, const struct MM_CarouselSource *source, u32 buttons, u32 frame)
{
	if (source == NULL || source->count <= 0 || carousel->selected < 0 || carousel->selected >= source->count) { return false; }
	if (carousel->changeFrames > 0)
	{
		if (--carousel->changeFrames == 0)
		{
			carousel->selected = carousel->next;
			carousel->scrollStart = frame;
			if (source->selected != NULL) { source->selected(source->context, carousel->selected); }
		}
		return false;
	}
	if ((buttons & (BTN_DOWN | BTN_UP)) != 0)
	{
		int direction = (buttons & BTN_DOWN) != 0 ? 1 : -1;
		int next = MM_Carousel_Find(source, carousel->selected, direction);
		if (next >= 0 && next != carousel->selected)
		{
			carousel->next = next;
			carousel->direction = -direction;
			carousel->changeFrames = MM_CAROUSEL_CHANGE_FRAMES;
			OtherFX_Play(0, 1);
		}
		return true;
	}
	if ((buttons & (BTN_CROSS_one | BTN_CIRCLE)) != 0)
	{
		OtherFX_Play(1, 1);
		if (source->confirm != NULL) { source->confirm(source->context, carousel->selected); }
		return true;
	}
	return false;
}

int MM_Carousel_RowIndex(const struct MM_Carousel *carousel, const struct MM_CarouselSource *source, int row)
{
	if (source == NULL || source->count <= 0 || carousel->selected < 0) { return -1; }
	int index = carousel->selected;
	int offset = row - MM_CAROUSEL_CENTER_ROW;
	int direction = offset < 0 ? -1 : 1;
	for (int i = 0; i < (offset < 0 ? -offset : offset); i++)
	{
		index = MM_Carousel_Find(source, index, direction);
		if (index < 0) { return -1; }
	}
	return index;
}

enum
{
	MM_CAROUSEL_ANGLE_STEP = 0x73,
	MM_CAROUSEL_WIDTH = 0x100,
	MM_CAROUSEL_HEIGHT = 0x19,
	MM_CAROUSEL_X_RADIUS = 0x19,
	MM_CAROUSEL_X_SHIFT = 9,
	MM_CAROUSEL_X_OFFSET = -0xb4,
	MM_CAROUSEL_Y_RADIUS = 200,
	MM_CAROUSEL_Y_SHIFT = 12,
	MM_CAROUSEL_Y_OFFSET = 0x60,
	MM_CAROUSEL_TEXT_X = 8,
	MM_CAROUSEL_TEXT_Y = 5,
	MM_CAROUSEL_HIGHLIGHT_X = 6,
	MM_CAROUSEL_HIGHLIGHT_Y = 4,
};

static RECT16 MM_Carousel_RowRect(int rowIndex, int changeFrames, int direction, int x, int y)
{
	int angle = (rowIndex - MM_CAROUSEL_CENTER_ROW) * MM_CAROUSEL_ANGLE_STEP;
	if (changeFrames > 0)
	{
		angle += ((MM_CAROUSEL_CHANGE_FRAMES - changeFrames) * MM_CAROUSEL_ANGLE_STEP / MM_CAROUSEL_CHANGE_FRAMES) * direction;
	}
	u32 packed = CTR_ReadU32AlignedLE((u32 *)((u32)MM_TRIG_APPROX + (angle & 0x3ff) * sizeof(*MM_TRIG_APPROX)));
	int cosine, sine;
	if ((angle & 0x400) != 0)
	{
		cosine = (s16)packed;
		sine = (s16)(packed >> 16);
		if ((angle & 0x800) != 0) { sine = -sine; }
		else { cosine = -cosine; }
	}
	else
	{
		cosine = (s16)(packed >> 16);
		sine = (s16)packed;
		if ((angle & 0x800) != 0) { cosine = -cosine; sine = -sine; }
	}
	RECT16 rect = {
		.x = (s16)((u16)x + ((cosine * MM_CAROUSEL_X_RADIUS) >> MM_CAROUSEL_X_SHIFT) + MM_CAROUSEL_X_OFFSET),
		.y = (s16)((u16)y + ((sine * (MM_CAROUSEL_Y_RADIUS >> 3) * 8) >> MM_CAROUSEL_Y_SHIFT) + MM_CAROUSEL_Y_OFFSET),
		.w = MM_CAROUSEL_WIDTH, .h = MM_CAROUSEL_HEIGHT,
	};
	return rect;
}

static void MM_Carousel_DrawRow(RECT16 *row, const char *title, bool selected, struct GameTracker *gGT)
{
	MM_DRAW_LINE_WIDE_FLAGS((char *)title, (s16)(row->x + MM_CAROUSEL_TEXT_X), (s16)(row->y + MM_CAROUSEL_TEXT_Y), FONT_BIG, ORANGE);
	if (selected)
	{
		RECT16 highlight = {row->x + MM_CAROUSEL_HIGHLIGHT_X, row->y + MM_CAROUSEL_HIGHLIGHT_Y,
		                    row->w - 2 * MM_CAROUSEL_HIGHLIGHT_X, row->h - 2 * MM_CAROUSEL_HIGHLIGHT_Y};
		MM_DRAW_CLEAR_BOX(&highlight, &GAME_MENU_HIGHLIGHT, TRANS_50_DECAL, gGT->backBuffer->otMem.uiOT, &gGT->backBuffer->primMem);
	}
	RECTMENU_DrawInnerRect(row, 0, gGT->backBuffer->otMem.uiOT);
}

void MM_Carousel_Draw(const struct MM_Carousel *carousel, const struct MM_CarouselSource *source,
                      struct GameTracker *tracker, int x, int y, u32 frame)
{
	enum { VISIBLE_ROWS = 9, LABEL_BYTES = 256, LABEL_MARGIN = 16 };
	if (source == NULL || source->count <= 0 || source->label == NULL || carousel->selected < 0) { return; }
	for (int rowIndex = 0; rowIndex < VISIBLE_ROWS; rowIndex++)
	{
		int index = MM_Carousel_RowIndex(carousel, source, rowIndex);
		if (index < 0) { continue; }
		bool selected = rowIndex == MM_CAROUSEL_CENTER_ROW && carousel->changeFrames == 0;
		u32 scrollFrame = selected ? frame - carousel->scrollStart : 0;
		RECT16 row = MM_Carousel_RowRect(rowIndex, carousel->changeFrames, carousel->direction, x, y);
		char label[LABEL_BYTES];
		const char *text = source->label(source->context, index, label, sizeof(label));
		if (text == NULL) { text = ""; }
		if (source->labelIsFontEncoded == NULL || !source->labelIsFontEncoded(source->context, index))
		{
			if (text != label) { TrackText_Copy(label, sizeof(label), text); }
			TrackText_Scroll(label, MM_CAROUSEL_WIDTH - LABEL_MARGIN, FONT_BIG, scrollFrame);
			text = label;
		}
		if (source->decorate != NULL) { source->decorate(source->context, index, &row, selected, scrollFrame); }
		MM_Carousel_DrawRow(&row, text, selected, tracker);
	}
}
