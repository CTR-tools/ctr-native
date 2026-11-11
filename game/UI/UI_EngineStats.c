#include <common.h>

enum
{
	ENGINE_STATS_SEGMENTS = 6,
	ENGINE_STATS_SEGMENT_WIDTH = 13,
	ENGINE_STATS_BAR_HEIGHT = 7,
	ENGINE_STATS_SCALE_ONE = 4096,
	ENGINE_STATS_POLY_CODE = 0x38000000,
};

static const int kEngineStatsNames[NUM_CLASSES] = {
	LNG_EXPANDED_ENGINE_BALANCED, LNG_EXPANDED_ENGINE_ACCELERATION,
	LNG_EXPANDED_ENGINE_SPEED, LNG_EXPANDED_ENGINE_TURN, LNG_EXPANDED_ENGINE_MAXED_OUT,
};

static const int kEngineStatsAbbreviations[NUM_CLASSES] = {
	LNG_EXPANDED_ENGINE_BALANCED_SHORT, LNG_EXPANDED_ENGINE_ACCELERATION_SHORT,
	LNG_EXPANDED_ENGINE_SPEED_SHORT, LNG_EXPANDED_ENGINE_TURN_SHORT, LNG_EXPANDED_ENGINE_MAXED_OUT_SHORT,
};

static const u8 kEngineStatsValues[NUM_CLASSES][3] = {
	{55, 55, 55}, {48, 80, 32}, {80, 32, 10}, {28, 48, 80}, {80, 80, 80},
};

static const int kEngineStatsLabels[3] = {LNG_SPEED, LNG_ACCEL, LNG_TURN};
static const int kEngineStatsColors[3] = {ORANGE_RED, LIME_GREEN, BLUE};

const char *UI_EngineStats_ClassName(int engine)
{
	if (engine < 0 || engine >= NUM_CLASSES)
	{
		return NULL;
	}
	return LANG_GetString(kEngineStatsNames[engine]);
}

const char *UI_EngineStats_ClassAbbreviation(int engine)
{
	if (engine < 0 || engine >= NUM_CLASSES)
	{
		return NULL;
	}
	return LANG_GetString(kEngineStatsAbbreviations[engine]);
}

static void UI_EngineStats_DrawBar(int x, int y, int length, int scale)
{
	struct DB *buffer = GAME_TRACKER->backBuffer;
	u32 *ot = GAME_TRACKER->pushBuffer_UI.ptrOT;
	const int width = length * scale / ENGINE_STATS_SCALE_ONE;
	const int height = (ENGINE_STATS_BAR_HEIGHT * scale + ENGINE_STATS_SCALE_ONE - 1) / ENGINE_STATS_SCALE_ONE;
	if (width <= 0 || height <= 0)
	{
		return;
	}
	RECT outline = {x, y, width, height};
	const Color white = {.r = 255, .g = 255, .b = 255};
	const Color black = {.r = 0, .g = 0, .b = 0};
	CTR_Box_DrawWireBox(&outline, &white, ot, &buffer->primMem);
	if (width > 2 && height > 2)
	{
		RECT shadow = {x + 1, y + 1, width - 2, height - 2};
		CTR_Box_DrawWireBox(&shadow, &black, ot, &buffer->primMem);
	}
	for (int segment = 0; segment < ENGINE_STATS_SEGMENTS; segment++)
	{
		const int start = segment * ENGINE_STATS_SEGMENT_WIDTH;
		if (start >= length)
		{
			break;
		}
		const int end = length < start + ENGINE_STATS_SEGMENT_WIDTH ? length : start + ENGINE_STATS_SEGMENT_WIDTH;
		if (buffer->primMem.cursor > buffer->primMem.guardEnd)
		{
			return;
		}
		POLY_G4 *p = buffer->primMem.cursor;
		buffer->primMem.cursor = p + 1;
		const u32 left = CS_GARAGE_STAT_COLORS[segment] | ENGINE_STATS_POLY_CODE;
		const u32 right = CS_GARAGE_STAT_COLORS[segment + 1] | ENGINE_STATS_POLY_CODE;
		CtrGpu_WriteColorCode(&p->r0, left);
		CtrGpu_WriteColorCode(&p->r1, right);
		CtrGpu_WriteColorCode(&p->r2, left);
		CtrGpu_WriteColorCode(&p->r3, right);
		p->x0 = p->x2 = x + start * scale / ENGINE_STATS_SCALE_ONE;
		p->x1 = p->x3 = x + end * scale / ENGINE_STATS_SCALE_ONE;
		p->y0 = p->y1 = y;
		p->y2 = p->y3 = y + height;
		p->tag = *ot | 0x08000000;
		*ot = CtrGpu_PrimToOTLink24(p);
	}
}

struct EngineStatsLayout
{
	int titleX;
	int labelX;
	int barX;
	RECT box;
};

static struct EngineStatsLayout UI_EngineStats_SmallLayout(int x, int y, const char *className, int labelScale, int titleScale, int barScale)
{
	struct EngineStatsLayout layout = {.box = {x, y, 112, 40}};
	int labelWidth = 0;
	for (int row = 0; row < 3; row++)
	{
		const int width = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[kEngineStatsLabels[row]], FONT_SMALL) * labelScale / ENGINE_STATS_SCALE_ONE;
		if (width > labelWidth)
		{
			labelWidth = width;
		}
	}
	layout.labelX = x + labelWidth;
	layout.barX = layout.labelX + 3;
	const int contentWidth = labelWidth + 3 + 80 * barScale / ENGINE_STATS_SCALE_ONE;
	const int titleWidth = DecalFont_GetLineWidth((char *)className, FONT_SMALL) * titleScale / ENGINE_STATS_SCALE_ONE;
	const int totalWidth = titleWidth > contentWidth ? titleWidth : contentWidth;
	layout.titleX = x + totalWidth / 2;
	return layout;
}

static struct EngineStatsLayout UI_EngineStats_LargeLayout(int x, int y, const char *className)
{
	struct EngineStatsLayout layout = {.labelX = x - 1, .barX = x + 9};
	int labelWidth = 0;
	for (int row = 0; row < 3; row++)
	{
		const int width = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[kEngineStatsLabels[row]], FONT_BIG);
		if (width > labelWidth)
		{
			labelWidth = width;
		}
	}
	const int titleWidth = DecalFont_GetLineWidth((char *)className, FONT_BIG);
	int left = x - (titleWidth + 1) / 2;
	int right = x + (titleWidth + 1) / 2;
	if (layout.labelX - labelWidth < left)
	{
		left = layout.labelX - labelWidth;
	}
	if (layout.barX + 80 > right)
	{
		right = layout.barX + 80;
	}
	left -= 6;
	right += 6;
	const int shift = left < 0 ? -left : (right > 512 ? 512 - right : 0);
	layout.labelX += shift;
	layout.barX += shift;
	layout.box = (RECT){left + shift, y, right - left, 68};
	layout.titleX = layout.box.x + layout.box.w / 2;
	return layout;
}

void UI_EngineStats_Draw(int x, int y, int engine, enum EngineStatsFormat format, bool retailClassNames)
{
	if (engine < 0 || engine >= NUM_CLASSES)
	{
		return;
	}
	const int classStringIndex = engine == TURN ? 0 : (engine < SPEED ? 1 : 2);
	const char *className = retailClassNames ? GAME_LANGUAGE_STRINGS[CS_GARAGE_CLASS_STRINGS[classStringIndex]] : UI_EngineStats_ClassName(engine);
	u32 *ot = GAME_TRACKER->backBuffer->otMem.uiOT;
	const bool large = format == ENGINE_STATS_LARGE;
	const int font = large ? FONT_BIG : FONT_SMALL;
	const int labelScale = large ? ENGINE_STATS_SCALE_ONE : 0x700;
	const int titleScale = large ? ENGINE_STATS_SCALE_ONE : 0x900;
	const int rowStep = large ? 15 : 9;
	const int barScale = large ? ENGINE_STATS_SCALE_ONE : (51 * ENGINE_STATS_SCALE_ONE / 80);
	const struct EngineStatsLayout layout = large ? UI_EngineStats_LargeLayout(x, y, className)
	                                              : UI_EngineStats_SmallLayout(x, y, className, labelScale, titleScale, barScale);
	DecalFont_DrawLineScaledOT((char *)className, layout.titleX, y + (large ? 4 : 1), font, JUSTIFY_CENTER | ORANGE, titleScale, ot);
	for (int row = 0; row < 3; row++)
	{
		const int rowY = y + (large ? 19 : 9) + row * rowStep;
		const int barY = rowY + (large ? 3 : -2);
		DecalFont_DrawLineScaledOT(GAME_LANGUAGE_STRINGS[kEngineStatsLabels[row]], layout.labelX, rowY, font, JUSTIFY_RIGHT | kEngineStatsColors[row], labelScale, ot);
		const int length = large ? CS_GARAGE_STAT_LENGTHS[row] : kEngineStatsValues[engine][row];
		UI_EngineStats_DrawBar(layout.barX, barY, length, barScale);
	}
	if (large)
	{
		RECT box = layout.box;
		RECTMENU_DrawInnerRect(&box, 4, ot);
	}
}
