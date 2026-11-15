#include <common.h>

#ifndef CTR_NATIVE_MM_CHOICE_PICKER_C
#define CTR_NATIVE_MM_CHOICE_PICKER_C

enum ChoicePickerResult
{
	CHOICE_PICKER_NONE,
	CHOICE_PICKER_CANCEL,
	CHOICE_PICKER_CONFIRM,
};

enum
{
	CHOICE_PICKER_ROW_SPACING = 14,
	CHOICE_PICKER_TITLE_HEIGHT = 14,
	CHOICE_PICKER_PADDING_X = 12,
	CHOICE_PICKER_PADDING_Y = 8,
	CHOICE_PICKER_MAX_WIDTH = 0x1a0,
	CHOICE_PICKER_CENTER_Y = 0x7e,
};

typedef const char *(*ChoicePickerLabel)(const void *context, int index);
typedef bool (*ChoicePickerApply)(const void *context, int selected);

static enum ChoicePickerResult ChoicePicker_Update(int *selected, int count, u32 tapped)
{
	if (count > 0 && (tapped & BTN_UP) != 0)
	{
		*selected = (*selected > 0) ? *selected - 1 : count - 1;
		OtherFX_Play(0, 1);
	}
	else if (count > 0 && (tapped & BTN_DOWN) != 0)
	{
		*selected = (*selected < count - 1) ? *selected + 1 : 0;
		OtherFX_Play(0, 1);
	}
	if ((tapped & (BTN_CIRCLE | BTN_TRIANGLE | BTN_START)) != 0)
	{
		return CHOICE_PICKER_CANCEL;
	}
	if (count > 0 && (tapped & BTN_CROSS) != 0)
	{
		return CHOICE_PICKER_CONFIRM;
	}
	return CHOICE_PICKER_NONE;
}

static enum ChoicePickerResult ChoicePicker_UpdateAndApply(bool *open, int *selected, int count, u32 tapped,
                                                            ChoicePickerApply apply, const void *context)
{
	enum ChoicePickerResult result = ChoicePicker_Update(selected, count, tapped);
	if (result != CHOICE_PICKER_NONE)
	{
		bool applied = result == CHOICE_PICKER_CONFIRM && apply(context, *selected);
		*open = false;
		OtherFX_Play(applied ? 1 : 2, 1);
	}
	return result;
}

static void ChoicePicker_Draw(const char *title, int count, int selected, ChoicePickerLabel label, const void *context,
                              struct GameTracker *gGT, u32 *ot)
{
	int contentWidth = DecalFont_GetLineWidth((char *)title, FONT_SMALL);
	for (int i = 0; i < count; i++)
	{
		int width = DecalFont_GetLineWidth((char *)label(context, i), FONT_SMALL);
		if (width > contentWidth)
		{
			contentWidth = width;
		}
	}
	const int maxContent = CHOICE_PICKER_MAX_WIDTH - 2 * CHOICE_PICKER_PADDING_X;
	if (contentWidth > maxContent)
	{
		contentWidth = maxContent;
	}
	const int width = contentWidth + 2 * CHOICE_PICKER_PADDING_X;
	const int height = 2 * CHOICE_PICKER_PADDING_Y + CHOICE_PICKER_TITLE_HEIGHT + count * CHOICE_PICKER_ROW_SPACING;
	const int x = (0x200 - width) / 2;
	const int y = CHOICE_PICKER_CENTER_Y - height / 2;
	const int titleY = y + CHOICE_PICKER_PADDING_Y;
	DecalFont_DrawLineOT((char *)title, x + width / 2, titleY, FONT_SMALL, JUSTIFY_CENTER | ORANGE, ot);
	for (int i = 0; i < count; i++)
	{
		int rowY = titleY + CHOICE_PICKER_TITLE_HEIGHT + i * CHOICE_PICKER_ROW_SPACING;
		DecalFont_DrawLineOT((char *)label(context, i), x + CHOICE_PICKER_PADDING_X, rowY, FONT_SMALL, ORANGE, ot);
		if (i == selected)
		{
			RECT selection = {x + 4, rowY - 2, width - 8, 12};
			CTR_Box_DrawClearBox(&selection, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, ot, &gGT->backBuffer->primMem);
		}
	}
	// The ordering table prepends primitives: the background is submitted last.
	RECT popup = {x, y, width, height};
	RECTMENU_DrawInnerRect(&popup, 4, ot);
}

#endif
