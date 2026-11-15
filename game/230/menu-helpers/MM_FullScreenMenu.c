#include <common.h>

#ifndef CTR_NATIVE_MM_FULLSCREEN_MENU_C
#define CTR_NATIVE_MM_FULLSCREEN_MENU_C

#include <stdio.h>

// ================= shared full-screen menu layout and navigation =================
//
// Any menu that is a full-screen box: an opaque frame, a title, and a scrolling list of
// label and value pairs.

// The box the menu sits in: the title screen's menus cast a drop shadow, the pause
// menu's are flat. Screens opened from either follow the menu they came from so they
// look like a sibling of the rows beside them.
enum
{
	FULLSCREEN_MENU_DRAW_STYLE_SHADED = 0,
	FULLSCREEN_MENU_DRAW_STYLE_FLAT = 4,
};

// Row geometry.
#define FULLSCREEN_MENU_TITLE_X     0x100
#define FULLSCREEN_MENU_MAIN_X      0x180
#define FULLSCREEN_MENU_TITLE_Y     0x18
#define FULLSCREEN_MENU_CENTER_Y    0x6C
#define FULLSCREEN_MENU_ROW_START_Y 0x3C
#define FULLSCREEN_MENU_ROW_SPACING 0x0E
#define FULLSCREEN_MENU_CONTENT_X   0x20
#define FULLSCREEN_MENU_CONTENT_W   0x1C0
#define FULLSCREEN_MENU_ROW_PADDING 8
#define FULLSCREEN_MENU_LABEL_X     (FULLSCREEN_MENU_CONTENT_X + FULLSCREEN_MENU_ROW_PADDING)
#define FULLSCREEN_MENU_VALUE_X     (FULLSCREEN_MENU_CONTENT_X + FULLSCREEN_MENU_CONTENT_W - FULLSCREEN_MENU_ROW_PADDING)

// Vertically centres FONT_SMALL hints alongside a FONT_BIG title.
#define FULLSCREEN_MENU_TITLE_BASELINE_NUDGE_Y \
	((data.font_charPixHeight[FONT_BIG] - data.font_charPixHeight[FONT_SMALL]) / 2)

// Rows visible at once; the rest are reached by scrolling.
#define FULLSCREEN_MENU_MAX_VISIBLE_ROWS 11

// Centred layout for section lists and settings lists, sized to the content row count.
static int FullScreenMenu_VisibleRows(int numRows);
static int FullScreenMenu_Height(int numRows);
static int FullScreenMenu_TitleY(int numRows);

// Matches the box of the menu that opened the screen; flat until a caller says otherwise.
static int s_frameDrawStyle = FULLSCREEN_MENU_DRAW_STYLE_FLAT;

static inline void FullScreenMenu_SetShadedFrame(bool shaded)
{
	s_frameDrawStyle = shaded ? FULLSCREEN_MENU_DRAW_STYLE_SHADED : FULLSCREEN_MENU_DRAW_STYLE_FLAT;
}

// Background, separator and the highlighted row. Shared so that every screen of this kind
// sits in the same box, whichever menu it belongs to.
//
// Note the OT draws back to front: a primitive added later sits behind the ones already
// in the list, which is why the frame can be drawn after every row and still leave the
// text visible.
static void FullScreenMenu_DrawFrame(int numRows)
{
	struct RectMenu frame = {
		.stringIndexTitle = LNG_OPTIONS,
		.state = USE_SMALL_FONT | BIG_TEXT_IN_TITLE,
		.drawStyle = s_frameDrawStyle,
	};
	RECT bounds = {FULLSCREEN_MENU_CONTENT_X - 6, FullScreenMenu_TitleY(numRows) - 4,
	               FULLSCREEN_MENU_CONTENT_W + 12, FullScreenMenu_Height(numRows)};
	RECTMENU_DrawFullRect(&frame, &bounds);
}

static void FullScreenMenu_DrawRowHighlight(struct GameTracker *gGT, uint32_t *ot, int y)
{
	RECT sel = {FULLSCREEN_MENU_CONTENT_X, y - 2, FULLSCREEN_MENU_CONTENT_W, 0x0C};
	CTR_Box_DrawClearBox(&sel, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, ot, &gGT->backBuffer->primMem);
}

// The screen's title, centred above the rows. Every screen draws one, so the font, the
// colour and the column are written once here rather than per screen.
static void FullScreenMenu_DrawTitle(const char *title, int numRows, int nudgeY, uint32_t *ot)
{
	DecalFont_DrawLineOT((char *)title, FULLSCREEN_MENU_TITLE_X, FullScreenMenu_TitleY(numRows) + nudgeY, FONT_BIG, JUSTIFY_CENTER | ORANGE, ot);
}

/* Keep the end of the edited text visible and underline its last character. */
static inline void FullScreenMenu_DrawEditableText(const char *text, int width, int x, int y,
                                                  int font, int justify, int color, uint32_t *ot)
{
	while (*text != '\0' && DecalFont_GetLineWidth((char *)text, font) > width)
	{
		text++;
	}
	int cursorX = x;
	if (justify == JUSTIFY_CENTER) { cursorX += DecalFont_GetLineWidth((char *)text, font) / 2; }
	DecalFont_DrawLineOT((char *)text, x, y, font, justify | color, ot);
	enum { CURSOR_PERIOD = 30, CURSOR_VISIBLE = 15, CURSOR_OFFSET_Y = 3 };
	if ((unsigned int)sdata->frameCounter % CURSOR_PERIOD < CURSOR_VISIBLE)
	{
		DecalFont_DrawLineOT("_", cursorX, y + CURSOR_OFFSET_Y, font, JUSTIFY_RIGHT | ORANGE, ot);
	}
}

static inline void FullScreenMenu_DrawEditableTitle(const char *text, int numRows, uint32_t *ot)
{
	FullScreenMenu_DrawEditableText(text, FULLSCREEN_MENU_CONTENT_W - 2 * FULLSCREEN_MENU_ROW_PADDING,
	                                FULLSCREEN_MENU_TITLE_X, FullScreenMenu_TitleY(numRows),
	                                FONT_BIG, JUSTIFY_CENTER, ORANGE, ot);
}

// A row's label, in the left column. Shared so a row looks the same whichever screen it is
// on; the value beside it is left to the caller, because what a value looks like is what
// separates one kind of row from another.
static void FullScreenMenu_DrawRowLabel(const char *label, int y, uint32_t *ot)
{
	DecalFont_DrawLineOT((char *)label, FULLSCREEN_MENU_LABEL_X, y, FONT_SMALL, ORANGE, ot);
}

// ================= option rows: for menus that edit settings =================

// ================= value column =================
//
// Everything that can sit in the value column goes through here, so the column, the font
// and the colour are written once. What a value looks like is the only thing that differs
// between rows, and that is what these four are for.

// A piece of text: an option's name, or what a row that has no number of its own shows.
static void FullScreenMenu_DrawValueTextColored(const char *text, int y, int color, uint32_t *ot)
{
	DecalFont_DrawLineOT((char *)text, FULLSCREEN_MENU_VALUE_X, y, FONT_SMALL, JUSTIFY_RIGHT | color, ot);
}

static void FullScreenMenu_DrawValueText(const char *text, int y, uint32_t *ot)
{
	FullScreenMenu_DrawValueTextColored(text, y, WHITE, ot);
}

static inline void FullScreenMenu_DrawEditableValue(const char *text, int width, int y, uint32_t *ot)
{
	FullScreenMenu_DrawEditableText(text, width, FULLSCREEN_MENU_VALUE_X, y, FONT_SMALL, JUSTIFY_RIGHT, WHITE, ot);
}

static void FullScreenMenu_DrawOnOffColored(bool on, int y, int color, uint32_t *ot)
{
	FullScreenMenu_DrawValueTextColored(LANG_GetString(on ? LNG_EXPANDED_ON : LNG_EXPANDED_OFF), y, color, ot);
}

static void FullScreenMenu_DrawOnOff(bool on, int y, uint32_t *ot)
{
	FullScreenMenu_DrawOnOffColored(on, y, WHITE, ot);
}

// A number with a unit after it, for a setting that is not a percentage: seconds, frames,
// megabytes. The unit is what tells two settings apart when both are plain numbers.
static void FullScreenMenu_DrawValueWithSuffixColored(int value, const char *suffix, int y, int color, uint32_t *ot)
{
	char text[16];

	snprintf(text, sizeof(text), "%d%s", value, suffix);
	FullScreenMenu_DrawValueTextColored(text, y, color, ot);
}

static void FullScreenMenu_DrawValueWithSuffix(int value, const char *suffix, int y, uint32_t *ot)
{
	FullScreenMenu_DrawValueWithSuffixColored(value, suffix, y, WHITE, ot);
}

static inline void FullScreenMenu_DrawPercentColored(int percent, int y, int color, uint32_t *ot)
{
	FullScreenMenu_DrawValueWithSuffixColored(percent, "%", y, color, ot);
}

static inline void FullScreenMenu_DrawPercent(int percent, int y, uint32_t *ot)
{
	FullScreenMenu_DrawPercentColored(percent, y, WHITE, ot);
}

// One name out of a list, drawn where its value goes. A value outside the list falls back
// to the first name, so a setting restored from a file cannot leave its row blank.
static const char *FullScreenMenu_ChoiceName(const char *const *choices, int count, int value)
{
	if ((choices == NULL) || (count <= 0))
	{
		return "";
	}

	if ((value < 0) || (value >= count))
	{
		return choices[0];
	}

	return choices[value];
}

// Frames a held Left/Right waits before it steps again, so one press moves a single step
// and holding runs the value through its range at a pace that can be seen.
#define FULLSCREEN_MENU_STEP_REPEAT_FRAMES 3

// True when a held direction should step this frame. Takes the button mask rather than a
// pad, because a screen may answer for more than one player and each has its own mask.
static inline bool FullScreenMenu_OptionStepDue(int held, int direction)
{
	const int button = (direction < 0) ? BTN_LEFT : BTN_RIGHT;

	if ((held & button) == 0)
	{
		return false;
	}

	return (sdata->frameCounter % FULLSCREEN_MENU_STEP_REPEAT_FRAMES) == 0;
}

// Applies one clamped step. Reports whether the value moved, so a caller can commit only
// on a real change instead of once per frame while a direction is held.
static bool FullScreenMenu_OptionStep(int *value, int direction, int min, int max, int step)
{
	const int before = *value;
	int wanted = before + (direction * step);

	if (wanted < min)
	{
		wanted = min;
	}
	else if (wanted > max)
	{
		wanted = max;
	}

	*value = wanted;

	return *value != before;
}

// ================= shared list navigation =================

// First visible row, given the current selection and how many rows there are.
static int FullScreenMenu_ScrollOffsetFor(const struct FullScreenMenu_Scroll *scroll, int selected, int numRows)
{
	int offset = scroll->offset;

	if (selected < offset)
	{
		offset = selected;
	}
	else if (selected >= offset + FULLSCREEN_MENU_MAX_VISIBLE_ROWS)
	{
		offset = selected - FULLSCREEN_MENU_MAX_VISIBLE_ROWS + 1;
	}

	if (offset > numRows - FULLSCREEN_MENU_MAX_VISIBLE_ROWS)
	{
		offset = numRows - FULLSCREEN_MENU_MAX_VISIBLE_ROWS;
	}

	if (offset < 0)
	{
		offset = 0;
	}

	return offset;
}

// Row a screen should stop on, for a list of this many rows. Keeps the selection inside
// the list even when a row it was pointing at has just gone away.
static int FullScreenMenu_ClampRow(int selected, int numRows)
{
	if (numRows <= 0)
	{
		return 0;
	}

	if (selected < 0)
	{
		return 0;
	}

	if (selected > numRows - 1)
	{
		return numRows - 1;
	}

	return selected;
}

// Where the list ends on screen: the last visible row, clamped to the end of the list so
// a short list does not report rows that are not there.
static int FullScreenMenu_VisibleEnd(const struct FullScreenMenu_Scroll *scroll, int numRows)
{
	int end = scroll->offset + FULLSCREEN_MENU_MAX_VISIBLE_ROWS;

	if (end > numRows)
	{
		end = numRows;
	}

	return end;
}

static int FullScreenMenu_VisibleRows(int numRows)
{
	return numRows < FULLSCREEN_MENU_MAX_VISIBLE_ROWS ? numRows : FULLSCREEN_MENU_MAX_VISIBLE_ROWS;
}

static int FullScreenMenu_Height(int numRows)
{
	return data.font_charPixHeight[FONT_BIG] + 9 +
	       FullScreenMenu_VisibleRows(numRows) * FULLSCREEN_MENU_ROW_SPACING + 8;
}

// Title centred vertically inside the content-sized frame. Row Ys start right below it.
static int FullScreenMenu_TitleY(int numRows)
{
	return FULLSCREEN_MENU_CENTER_Y - FullScreenMenu_Height(numRows) / 2 + 4;
}

// Y of the row at visible index `i`, counted from the top of the list.
static int FullScreenMenu_RowY(int i, int numRows)
{
	return FullScreenMenu_TitleY(numRows) + data.font_charPixHeight[FONT_BIG] + 9 +
	       i * FULLSCREEN_MENU_ROW_SPACING;
}

// Moves the selection by one row, wrapping at either end, and follows it with the scroll
// so the selected row is always on screen. One direction per press, so holding does not
// run: lists step per tap, unlike a value's Left/Right, which runs on a timer.
static int FullScreenMenu_MoveRow(struct FullScreenMenu_Scroll *scroll, int selected, int direction, int numRows)
{
	if (numRows <= 0)
	{
		return 0;
	}

	if (direction < 0)
	{
		selected = (selected > 0) ? selected - 1 : numRows - 1;
	}
	else
	{
		selected = (selected < numRows - 1) ? selected + 1 : 0;
	}

	scroll->offset = FullScreenMenu_ScrollOffsetFor(scroll, selected, numRows);

	return selected;
}

// ================= settings list =================
//
// Row metadata and value operations, independent of how a screen stores its settings.

enum FullScreenMenu_RowKind
{
	FULLSCREEN_ROW_TOGGLE, // ON or OFF
	FULLSCREEN_ROW_CHOICE, // one name out of a list
	FULLSCREEN_ROW_STEP,   // a number in a range, drawn with a unit
	FULLSCREEN_ROW_ACTION, // no value of its own; the screen answers it
};

struct FullScreenMenu_Row
{
	const char *label;
	enum FullScreenMenu_RowKind kind;
	const char *const *choices; // FULLSCREEN_ROW_CHOICE names
	int numChoices;             // FULLSCREEN_ROW_CHOICE
	int min;                    // FULLSCREEN_ROW_STEP
	int max;                    // FULLSCREEN_ROW_STEP
	int step;                   // FULLSCREEN_ROW_STEP
	const char *suffix;         // FULLSCREEN_ROW_STEP unit
};

static inline void FullScreenMenu_DrawRowValue(const struct FullScreenMenu_Row *row, int value, int y, uint32_t *ot)
{
	switch (row->kind)
	{
	case FULLSCREEN_ROW_TOGGLE:
		FullScreenMenu_DrawOnOff(value != 0, y, ot);
		break;

	case FULLSCREEN_ROW_CHOICE:
		FullScreenMenu_DrawValueText(FullScreenMenu_ChoiceName(row->choices, row->numChoices, value), y, ot);
		break;

	case FULLSCREEN_ROW_STEP:
		FullScreenMenu_DrawValueWithSuffix(value, row->suffix, y, ot);
		break;

	case FULLSCREEN_ROW_ACTION:
		// An action row has no value to put in the column.
		break;
	}
}

// What a direction does to a row. Only a row with a range to move within answers one, which
// is what the options menu does with its sliders: a choice and a toggle are changed by the
// confirm button and not by the directions, so a held direction cannot run them. Reports
// whether the value moved.
static inline bool FullScreenMenu_RowStep(const struct FullScreenMenu_Row *row, int *value, int direction)
{
	if (row->kind != FULLSCREEN_ROW_STEP)
	{
		return false;
	}

	return FullScreenMenu_OptionStep(value, direction, row->min, row->max, row->step);
}

// What the confirm button does to a row that holds a value: a toggle flips and a choice
// moves to the next name, round the end of the list and back to the first. A row with a
// range has no single next value, so the button leaves it to the directions, and neither
// touches a row the screen answers itself. Reports whether the value moved.
static inline bool FullScreenMenu_RowActivate(const struct FullScreenMenu_Row *row, int *value)
{
	switch (row->kind)
	{
	case FULLSCREEN_ROW_TOGGLE:
		*value = (*value != 0) ? 0 : 1;
		return true;

	case FULLSCREEN_ROW_CHOICE:
	{
		if (row->numChoices <= 0)
		{
			return false;
		}

		*value = (*value + 1) % row->numChoices;
		return true;
	}

	case FULLSCREEN_ROW_STEP:
	case FULLSCREEN_ROW_ACTION:
		return false;
	}

	return false;
}

// Keeps the selection inside the list and the list scrolled to it, for when rows have been
// added or taken away since the last frame.
static void FullScreenMenu_FollowScroll(struct FullScreenMenu_Scroll *scroll, s16 *selected, int numRows)
{
	*selected = (s16)FullScreenMenu_ClampRow(*selected, numRows);
	scroll->offset = FullScreenMenu_ScrollOffsetFor(scroll, *selected, numRows);
}

// ================= driving a full-screen list =================

static void FullScreenMenu_NavTapped(u32 buttons, struct FullScreenMenu_Scroll *scroll, s16 *selected, int numRows)
{
	FullScreenMenu_FollowScroll(scroll, selected, numRows);

	if ((buttons & BTN_UP) != 0)
	{
		*selected = (s16)FullScreenMenu_MoveRow(scroll, *selected, -1, numRows);
		OtherFX_Play(0, 1);
	}

	if ((buttons & BTN_DOWN) != 0)
	{
		*selected = (s16)FullScreenMenu_MoveRow(scroll, *selected, 1, numRows);
		OtherFX_Play(0, 1);
	}
}

static inline void FullScreenMenu_NavScroll(struct GamepadBuffer *pad, struct FullScreenMenu_Scroll *scroll, s16 *selected, int numRows)
{
	FullScreenMenu_NavTapped((u32)pad->buttonsTapped, scroll, selected, numRows);
}

// Submit prompts before other menu primitives so they appear in front of them.
static inline void FullScreenMenu_DrawPrompt(const char *text, uint32_t *ot)
{
	int y = SCREEN_HEIGHT - data.font_charPixHeight[FONT_SMALL] - 4;
	int height = DecalFont_DrawMultiLine((char *)text, 0x100, y, 400, FONT_SMALL, JUSTIFY_CENTER | ORANGE);
	RECT rect = {-10, y - 4, 0x214, height + 8};
	RECTMENU_DrawInnerRect(&rect, 4, ot);
}

static inline bool FullScreenMenu_Confirm(u32 tapped)
{
	if ((tapped & (BTN_CROSS | BTN_CIRCLE)) == 0)
	{
		return false;
	}
	OtherFX_Play(1, 1);
	return true;
}

static inline bool FullScreenMenu_Back(u32 tapped)
{
	if ((tapped & (BTN_TRIANGLE | BTN_START)) == 0)
	{
		return false;
	}
	OtherFX_Play(2, 1);
	return true;
}

// Menus supply row content; clipping, placement and highlighting stay shared.
typedef void (*FullScreenMenu_DrawRow)(const void *context, int row, int y, bool selected, uint32_t *ot);

static inline void FullScreenMenu_DrawTextListRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	const char *const *labels = context;
	(void)selected;
	FullScreenMenu_DrawRowLabel(labels[row], y, ot);
}

static void FullScreenMenu_DrawRows(const struct FullScreenMenu_Scroll *scroll, int numRows, int selected,
                                   FullScreenMenu_DrawRow drawRow, const void *context,
                                   struct GameTracker *gGT, uint32_t *ot)
{
	const int visibleEnd = FullScreenMenu_VisibleEnd(scroll, numRows);

	for (int i = scroll->offset; i < visibleEnd; i++)
	{
		const int y = FullScreenMenu_RowY(i - scroll->offset, numRows);

		drawRow(context, i, y, i == selected, ot);

		if (i == selected)
		{
			FullScreenMenu_DrawRowHighlight(gGT, ot, y);
		}
	}
}

static void FullScreenMenu_DrawScreen(const char *title, int nudgeY, const struct FullScreenMenu_Scroll *scroll,
                                     int numRows, int selected, FullScreenMenu_DrawRow drawRow, const void *context,
                                     struct GameTracker *gGT, uint32_t *ot)
{
	FullScreenMenu_DrawTitle(title, numRows, nudgeY, ot);
	FullScreenMenu_DrawRows(scroll, numRows, selected, drawRow, context, gGT, ot);
	FullScreenMenu_DrawFrame(numRows);
}

// Large, content-sized section list: title menus sit right, pause menus centre.
static inline void FullScreenMenu_DrawSectionScreen(const char *title, const char *const *labels,
                                            const struct FullScreenMenu_Scroll *scroll,
                                            int numRows, int selected, int columnX,
                                            struct GameTracker *gGT, uint32_t *ot)
{
	const int rowHeight = data.font_charPixHeight[FONT_BIG] + 3;
	const int visibleEnd = FullScreenMenu_VisibleEnd(scroll, numRows);
	const int visibleRows = visibleEnd - scroll->offset;
	const int height = (visibleRows + 1) * rowHeight + 6;
	const int titleY = FULLSCREEN_MENU_CENTER_Y - height / 2 + 2;
	int width = DecalFont_GetLineWidth((char *)title, FONT_BIG);
	for (int row = 0; row < numRows; row++)
	{
		const int labelWidth = DecalFont_GetLineWidth((char *)labels[row], FONT_BIG);
		if (labelWidth > width) { width = labelWidth; }
	}
	width += 2 * FULLSCREEN_MENU_ROW_PADDING;
	if (width > FULLSCREEN_MENU_CONTENT_W) { width = FULLSCREEN_MENU_CONTENT_W; }
	const int maxWidth = 2 * (SCREEN_WIDTH - 6 - columnX);
	if (width > maxWidth) { width = maxWidth; }
	DecalFont_DrawLineOT((char *)title, columnX, titleY, FONT_BIG, JUSTIFY_CENTER | ORANGE, ot);
	for (int row = scroll->offset; row < visibleEnd; row++)
	{
		const int y = titleY + rowHeight + 6 + (row - scroll->offset) * rowHeight;
		DecalFont_DrawLineOT((char *)labels[row], columnX, y, FONT_BIG, JUSTIFY_CENTER | ORANGE, ot);
		if (row == selected)
		{
			RECT highlight = {columnX - width / 2, y - 1, width, rowHeight - 3};
			CTR_Box_DrawClearBox(&highlight, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, ot, &gGT->backBuffer->primMem);
		}
	}
	struct RectMenu frame = {.stringIndexTitle = LNG_OPTIONS, .drawStyle = s_frameDrawStyle};
	RECT bounds = {columnX - width / 2 - 6, titleY - 6, width + 12, height + 8};
	RECTMENU_DrawFullRect(&frame, &bounds);
}

static inline void FullScreenMenu_DrawMessage(const char *title, const char *message, struct GameTracker *gGT, uint32_t *ot)
{
	const struct FullScreenMenu_Scroll scroll = {0};
	const char *labels[] = {message};
	FullScreenMenu_DrawScreen(title, 0, &scroll, 1, -1, FullScreenMenu_DrawTextListRow, labels, gGT, ot);
}

// Keep each selectable row together with its informational subrows while scrolling.
static inline void FullScreenMenu_DrawGroupedScreen(const char *title, int rowsPerItem, int numItems, int selected,
                                                    FullScreenMenu_DrawRow drawRow, const void *context,
                                                    struct GameTracker *gGT, uint32_t *ot)
{
	int numRows = numItems * rowsPerItem;
	int lastRow = selected * rowsPerItem + rowsPerItem - 1;
	struct FullScreenMenu_Scroll scroll = {0};
	scroll.offset = FullScreenMenu_ScrollOffsetFor(&scroll, lastRow, numRows);
	FullScreenMenu_DrawScreen(title, 0, &scroll, numRows, selected * rowsPerItem, drawRow, context, gGT, ot);
}

#endif // CTR_NATIVE_MM_FULLSCREEN_MENU_C
