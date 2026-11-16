#include <common.h>
#include <widescreen.h>

#define UI_NUM_X      sdata->s_x
#define UI_NUM_INT    sdata->s_int
#define UI_NUM_LONG   sdata->s_longInt
#define UI_NUM_COLORS data.ptrColor

#define UI_NUM_DRAW_POLY_GT4(icon, x, y, prim, ot, c0, c1, c2, c3, transparency, scale)                                                              \
	DecalHUD_DrawPolyGT4((icon), (x), (y), (prim), (ot), ColorCode_Load(&(c0)), ColorCode_Load(&(c1)), ColorCode_Load(&(c2)), ColorCode_Load(&(c3)), \
	                     (transparency), (scale))

enum
{
	UI_DRAWNUM_TEXT_PLAYER_MAX = 2,
	UI_DRAWNUM_WUMPA_TEXT_BUFFER_SIZE = 16,
	UI_DRAWNUM_COUNTER_TEXT_BUFFER_SIZE = 16,
	UI_DRAWNUM_SMALL_X_OFFSET_Y = 4,
	UI_DRAWNUM_BIG_TEXT_OFFSET_X = 0xd,
	UI_DRAWNUM_WUMPA_DIGIT_SPACING_X = 0xc,
	UI_DRAWNUM_DIGIT_ICON_GROUP = 5,
	UI_DRAWNUM_DECIMAL_BASE = 10,
	UI_DRAWNUM_TIMEBOX_X_OFFSET_X = 0x14,
	UI_DRAWNUM_TIMEBOX_X_OFFSET_Y = -10,
	UI_DRAWNUM_TIMEBOX_TEXT_OFFSET_X = 0x21,
	UI_DRAWNUM_TIMEBOX_TEXT_OFFSET_Y = -0xe,
};

void UI_DrawNumWumpa(s16 posX, s16 posY, struct Driver *d)
{
	struct GameTracker *gGT = GAME_TRACKER;
	char message[UI_DRAWNUM_WUMPA_TEXT_BUFFER_SIZE];

	if (gGT->numPlyrCurrGame <= UI_DRAWNUM_TEXT_PLAYER_MAX)
	{
		DecalFont_DrawLine(UI_NUM_X, posX, posY + UI_DRAWNUM_SMALL_X_OFFSET_Y, FONT_SMALL, ORANGE);

		sprintf(message, UI_NUM_INT, d->numWumpas);
		DecalFont_DrawLine(message, posX + Widescreen_ScaleWidth(UI_DRAWNUM_BIG_TEXT_OFFSET_X), posY, FONT_BIG, ORANGE);
	}
	else
	{
		// NOTE(aalhendi): Advance the icon-group base before reading its appended pointer array.
		// Its retail argument register preserves the order of the address addition.
		register u8 *firstIcons CTR_PSX_REGISTER("$4") = (u8 *)gGT->iconGroup[UI_DRAWNUM_DIGIT_ICON_GROUP];
		u8 rawWumpa = d->numWumpas;
		s32 currWumpa = (s8)rawWumpa;
		s32 tens = (s8)(currWumpa / UI_DRAWNUM_DECIMAL_BASE);
		s32 ones = currWumpa - tens * UI_DRAWNUM_DECIMAL_BASE;

		firstIcons += tens * sizeof(struct Icon *);
		UI_NUM_DRAW_POLY_GT4(*(struct Icon **)(firstIcons + sizeof(struct IconGroup)), posX, posY, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT,
		                     ((Color *)UI_NUM_COLORS[ORANGE])[0], ((Color *)UI_NUM_COLORS[ORANGE])[1], ((Color *)UI_NUM_COLORS[ORANGE])[2],
		                     ((Color *)UI_NUM_COLORS[ORANGE])[3], 0, FP(1.0));

		// NOTE(aalhendi): Each digit takes a fresh tracker and palette snapshot.
		{
			struct GameTracker *nextGT = GAME_TRACKER;
			s32 byteOffset = ones * sizeof(struct Icon *);
			struct IconGroup *nextGroup = nextGT->iconGroup[UI_DRAWNUM_DIGIT_ICON_GROUP];
			UI_NUM_DRAW_POLY_GT4(*(struct Icon **)((u8 *)nextGroup + byteOffset + sizeof(struct IconGroup)), posX + Widescreen_ScaleWidth(UI_DRAWNUM_WUMPA_DIGIT_SPACING_X), posY,
			                     &nextGT->backBuffer->primMem, nextGT->pushBuffer_UI.ptrOT, ((Color *)UI_NUM_COLORS[ORANGE])[0],
			                     ((Color *)UI_NUM_COLORS[ORANGE])[1], ((Color *)UI_NUM_COLORS[ORANGE])[2], ((Color *)UI_NUM_COLORS[ORANGE])[3], 0, FP(1.0));
		}
	}
}

// The collected count is passed in rather than read from the driver, because a
// co-op relic race shares one crate pool between both racers.
void UI_DrawNumTimebox(s16 posX, s16 posY, int collected)
{
	char string[UI_DRAWNUM_COUNTER_TEXT_BUFFER_SIZE];

	DecalFont_DrawLine(UI_NUM_X, posX + Widescreen_ScaleWidth(UI_DRAWNUM_TIMEBOX_X_OFFSET_X), posY + UI_DRAWNUM_TIMEBOX_X_OFFSET_Y, FONT_SMALL, ORANGE);

	sprintf(string, rdata.s_lapString, collected, CTR_PRINTF_PSX_LONG(GAME_TRACKER->timeCratesInLEV));

	DecalFont_DrawLine(string, posX + Widescreen_ScaleWidth(UI_DRAWNUM_TIMEBOX_TEXT_OFFSET_X), posY + UI_DRAWNUM_TIMEBOX_TEXT_OFFSET_Y, FONT_BIG, ORANGE);
}

// NOTE(aalhendi): Retail callers pass the driver through the shared HUD
// interface, but these three counters read the adventure profile instead.
void UI_DrawNumRelic(s16 posX, s16 posY, struct Driver *driver)
{
	char string[UI_DRAWNUM_COUNTER_TEXT_BUFFER_SIZE];
	struct GameTracker *gGT;
	s32 num;
	(void)driver;

	DecalFont_DrawLine(UI_NUM_X, posX, posY + UI_DRAWNUM_SMALL_X_OFFSET_Y, FONT_SMALL, ORANGE);

	gGT = GAME_TRACKER;
	num = gGT->currAdvProfile.numRelics;
	if ((gGT->gameMode2 & INC_RELIC) != 0)
	{
		num--;
	}

	sprintf(string, UI_NUM_LONG, CTR_PRINTF_PSX_LONG(num));
	DecalFont_DrawLine(string, posX + Widescreen_ScaleWidth(UI_DRAWNUM_BIG_TEXT_OFFSET_X), posY, FONT_BIG, ORANGE);
}

void UI_DrawNumKey(s16 posX, s16 posY, struct Driver *driver)
{
	char string[UI_DRAWNUM_COUNTER_TEXT_BUFFER_SIZE];
	struct GameTracker *gGT;
	s32 num;
	(void)driver;

	DecalFont_DrawLine(UI_NUM_X, posX, posY + UI_DRAWNUM_SMALL_X_OFFSET_Y, FONT_SMALL, ORANGE);

	gGT = GAME_TRACKER;
	num = gGT->currAdvProfile.numKeys;
	if ((gGT->gameMode2 & INC_KEY) != 0)
	{
		num--;
	}

	sprintf(string, UI_NUM_LONG, CTR_PRINTF_PSX_LONG(num));
	DecalFont_DrawLine(string, posX + Widescreen_ScaleWidth(UI_DRAWNUM_BIG_TEXT_OFFSET_X), posY, FONT_BIG, ORANGE);
}

void UI_DrawNumTrophy(s16 posX, s16 posY, struct Driver *driver)
{
	char string[UI_DRAWNUM_COUNTER_TEXT_BUFFER_SIZE];
	struct GameTracker *gGT;
	s32 num;
	(void)driver;

	DecalFont_DrawLine(UI_NUM_X, posX, posY + UI_DRAWNUM_SMALL_X_OFFSET_Y, FONT_SMALL, ORANGE);

	gGT = GAME_TRACKER;
	num = gGT->currAdvProfile.numTrophies;
	if ((gGT->gameMode2 & INC_TROPHY) != 0)
	{
		num--;
	}

	sprintf(string, UI_NUM_LONG, CTR_PRINTF_PSX_LONG(num));
	DecalFont_DrawLine(string, posX + Widescreen_ScaleWidth(UI_DRAWNUM_BIG_TEXT_OFFSET_X), posY, FONT_BIG, ORANGE);
}

// Crystal Challenge has a single shared counter, so co-op players pool pickups.
int UI_Crystal_CountCollected(const struct GameTracker *gGT)
{
	int collected = 0;

	for (int playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		const struct Driver *driver = gGT->drivers[playerIndex];

		if (driver != NULL)
		{
			collected += driver->numCrystals;
		}
	}

	return collected;
}

void UI_DrawNumCrystal(s16 posX, s16 posY)
{
	char string[UI_DRAWNUM_COUNTER_TEXT_BUFFER_SIZE];

	DecalFont_DrawLine(UI_NUM_X, posX, posY + UI_DRAWNUM_SMALL_X_OFFSET_Y, FONT_SMALL, ORANGE);

	sprintf(string, rdata.s_lapString, UI_Crystal_CountCollected(GAME_TRACKER), CTR_PRINTF_PSX_LONG(GAME_TRACKER->numCrystalsInLEV));

	DecalFont_DrawLine(string, posX + Widescreen_ScaleWidth(UI_DRAWNUM_BIG_TEXT_OFFSET_X), posY, FONT_BIG, ORANGE);
}
