#include <common.h>
#include <level_info.h>
#include <platform/native_custom_cup_storage.h>
#include <platform/native_track_library.h>
#include <track_text.h>
#include "MM_FullScreenMenu.c"

enum CustomCupSelectPage
{
	CUSTOM_CUP_SELECT_STANDARD,
	CUSTOM_CUP_SELECT_KIND,
	CUSTOM_CUP_SELECT_LIST,
	CUSTOM_CUP_SELECT_ACTIONS,
	CUSTOM_CUP_SELECT_DELETE,
};

enum
{
	CUSTOM_CUP_ACTION_PLAY,
	CUSTOM_CUP_ACTION_EDIT,
	CUSTOM_CUP_ACTION_DELETE,
	CUSTOM_CUP_ACTION_COUNT,
	CUSTOM_CUP_DELETE_NO = 0,
	CUSTOM_CUP_DELETE_YES,
	CUSTOM_CUP_DELETE_COUNT,
	CUSTOM_CUP_SELECT_RETAIL_TRACK_COUNT = 18,
	CUSTOM_CUP_SELECT_KIND_COUNT = 2,
	CUSTOM_CUP_SELECT_CAROUSEL_X = 0,
	CUSTOM_CUP_SELECT_CAROUSEL_Y = 0,
	CUSTOM_CUP_SELECT_MENU_X = 16,
	CUSTOM_CUP_SELECT_MENU_WIDTH = 256,
	CUSTOM_CUP_SELECT_MENU_PADDING = 8,
	CUSTOM_CUP_SELECT_PREVIEW_WIDTH = 204,
	CUSTOM_CUP_SELECT_PREVIEW_X = 298,
	CUSTOM_CUP_SELECT_PREVIEW_Y = 34,
	CUSTOM_CUP_SELECT_PREVIEW_TOP = 15,
	CUSTOM_CUP_SELECT_PREVIEW_SEPARATOR_Y = 29,
	CUSTOM_CUP_SELECT_PREVIEW_BORDER_HEIGHT = 2,
	CUSTOM_CUP_SELECT_PREVIEW_STEP = 15,
	CUSTOM_CUP_SELECT_PREVIEW_COLUMN_GAP = 4,
	CUSTOM_CUP_SELECT_PREVIEW_PADDING = 3,
	CUSTOM_CUP_SELECT_PREVIEW_LEFT_INSET = 2,
};

struct CustomCupSelectEntry
{
	char id[NATIVE_CUSTOM_CUP_ID_SIZE];
	char name[CUSTOM_CUP_NAME_BYTES];
};

static enum CustomCupSelectPage s_cupSelectPage;
static struct CustomCupSelectEntry *s_cupSelectEntries;
static size_t s_cupSelectCount;
static int s_cupKindRow;
static struct FullScreenMenu_Scroll s_cupKindScroll;
static s16 s_cupActionRow;
static s16 s_cupDeleteRow;
static struct FullScreenMenu_Scroll s_cupActionScroll;
static struct FullScreenMenu_Scroll s_cupDeleteScroll;
static struct MM_Carousel s_cupCarousel;
static bool s_cupSelectError;
static bool s_cupCreateUnavailable;
static bool s_cupEditUnavailable;
static struct CustomCup s_cupSelectPreview;

static void MM_CustomCupSelect_OnSelected(void *context, int index);
static void MM_CustomCupSelect_OnConfirm(void *context, int index);
static void MM_CustomCupSelect_DrawScreen(struct GameTracker *gGT);

static const char *MM_CustomCupSelect_Label(void *context, int index, char *buffer, size_t capacity)
{
	(void)context;
	(void)buffer;
	(void)capacity;
	return index == 0 ? "CREATE NEW" : s_cupSelectEntries[index - 1].name;
}

static struct MM_CarouselSource MM_CustomCupSelect_Source(void)
{
	struct MM_CarouselSource source = {
		.count = (int)s_cupSelectCount + 1,
		.label = MM_CustomCupSelect_Label,
		.selected = MM_CustomCupSelect_OnSelected,
		.confirm = MM_CustomCupSelect_OnConfirm,
	};
	return source;
}

static void MM_CustomCupSelect_Clear(void)
{
	free(s_cupSelectEntries);
	s_cupSelectEntries = NULL;
	s_cupSelectCount = 0;
	CustomCup_Clear(&s_cupSelectPreview);
}

static void MM_CustomCupSelect_Visit(void *context, const char *id,
                                    enum NativeCustomCupStorageResult result, const struct CustomCup *cup)
{
	(void)context;
	if (result != NATIVE_CUSTOM_CUP_OK || cup == NULL)
	{
		s_cupSelectError = true;
		return;
	}
	if (s_cupSelectCount >= 0x7fff)
	{
		s_cupSelectError = true;
		return;
	}
	struct CustomCupSelectEntry *entries = realloc(s_cupSelectEntries, (s_cupSelectCount + 1) * sizeof(*entries));
	if (entries == NULL)
	{
		s_cupSelectError = true;
		return;
	}
	s_cupSelectEntries = entries;
	struct CustomCupSelectEntry *entry = &entries[s_cupSelectCount++];
	memcpy(entry->id, id, sizeof(entry->id));
	memcpy(entry->name, cup->name, sizeof(entry->name));
}

static void MM_CustomCupSelect_Preview(int index)
{
	CustomCup_Clear(&s_cupSelectPreview);
	if (index > 0 && (size_t)index <= s_cupSelectCount &&
	    NativeCustomCup_Load(s_cupSelectEntries[index - 1].id, &s_cupSelectPreview) != NATIVE_CUSTOM_CUP_OK)
	{
		s_cupSelectError = true;
	}
}

static void MM_CustomCupSelect_OnSelected(void *context, int index)
{
	(void)context;
	s_cupCreateUnavailable = false;
	MM_CustomCupSelect_Preview(index);
}

static void MM_CustomCupSelect_OpenList(int selected)
{
	MM_CustomCupSelect_Clear();
	s_cupSelectError = false;
	s_cupCreateUnavailable = false;
	if (NativeCustomCup_Scan(MM_CustomCupSelect_Visit, NULL) != NATIVE_CUSTOM_CUP_OK)
	{
		s_cupSelectError = true;
	}
	struct MM_CarouselSource source = MM_CustomCupSelect_Source();
	if (selected > (int)s_cupSelectCount) { selected = (int)s_cupSelectCount; }
	MM_Carousel_Init(&s_cupCarousel, &source, selected, (u32)MM_FRAME_COUNTER);
	s_cupSelectPage = CUSTOM_CUP_SELECT_LIST;
}

static void MM_CustomCupSelect_Init(void)
{
	MM_CustomCupSelect_Clear();
	s_cupSelectPage = g_config.customCups && (GAME_TRACKER->gameMode1 & ARCADE_MODE) != 0
	                      ? CUSTOM_CUP_SELECT_KIND : CUSTOM_CUP_SELECT_STANDARD;
	s_cupKindRow = 0;
	s_cupKindScroll.offset = 0;
	s_cupSelectError = false;
	if (s_cupSelectPage != CUSTOM_CUP_SELECT_STANDARD) { RECTMENU_ClearInput(); }
}

static bool MM_CustomCupSelect_TracksOpen(const struct CustomCup *cup)
{
	for (int i = 0; i < cup->trackCount; i++)
	{
		int level;
		if (!CustomCup_ResolveTrack(cup->tracks[i], NativeTrackLibrary_Current(), &level)) { return false; }
		for (int j = 0; j < CUSTOM_CUP_SELECT_RETAIL_TRACK_COUNT; j++)
		{
			if (MM_ARCADE_TRACKS[j].levID == level && !MM_TrackSelect_boolTrackOpen(&MM_ARCADE_TRACKS[j]))
			{
				return false;
			}
		}
	}
	return true;
}

static bool MM_CustomCupSelect_UpdateKind(struct RectMenu *menu, u32 tapped)
{
	static const char *const labels[CUSTOM_CUP_SELECT_KIND_COUNT] = {"STANDARD", "CUSTOM"};
	struct GameTracker *gGT = GAME_TRACKER;
	if (FullScreenMenu_Back(tapped))
	{
		RECTMENU_ClearInput();
		MM_CustomCupSelect_Clear();
		MM_DESIRED_MENU = &MM_MENU_CHARACTER_SELECT;
		MM_Characters_RestoreIDs();
		return true;
	}

	s16 selected = (s16)s_cupKindRow;
	FullScreenMenu_NavTapped(tapped, &s_cupKindScroll, &selected, CUSTOM_CUP_SELECT_KIND_COUNT);
	s_cupKindRow = selected;
	if (FullScreenMenu_Confirm(tapped))
	{
		RECTMENU_ClearInput();
		if (selected == 0)
		{
			s_cupSelectPage = CUSTOM_CUP_SELECT_STANDARD;
			MM_CUP_TRANSITION_FRAME = MM_CUP_SELECT_INITIAL_TRANSITION_FRAMES;
			MM_CUP_TRANSITION_STATE = ENTERING_MENU;
			menu->rowSelected = 0;
			return false;
		}
		MM_CustomCupSelect_OpenList(0);
		MM_CustomCupSelect_DrawScreen(gGT);
		return true;
	}

	bool wasShaded = s_frameDrawStyle == FULLSCREEN_MENU_DRAW_STYLE_SHADED;
	FullScreenMenu_SetShadedFrame(true);
	FullScreenMenu_DrawSectionScreen("SELECT CUP TYPE", labels, &s_cupKindScroll,
	                                 CUSTOM_CUP_SELECT_KIND_COUNT, selected, SCREEN_WIDTH / 2,
	                                 gGT, gGT->backBuffer->otMem.uiOT);
	FullScreenMenu_SetShadedFrame(wasShaded);
	return true;
}

static void MM_CustomCupSelect_DrawPreview(struct GameTracker *gGT)
{
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
	int numberWidth = DecalFont_GetLineWidth("99", FONT_SMALL);
	int positionX = CUSTOM_CUP_SELECT_PREVIEW_X - CUSTOM_CUP_SELECT_PREVIEW_PADDING + CUSTOM_CUP_SELECT_PREVIEW_LEFT_INSET;
	int titleX = positionX + numberWidth + CUSTOM_CUP_SELECT_PREVIEW_COLUMN_GAP;
	int lapsX = CUSTOM_CUP_SELECT_PREVIEW_X + CUSTOM_CUP_SELECT_PREVIEW_WIDTH - numberWidth;
	int titleWidth = lapsX - CUSTOM_CUP_SELECT_PREVIEW_COLUMN_GAP - titleX;
	for (int i = 0; i < s_cupSelectPreview.trackCount; i++)
	{
		int level;
		char title[128], text[128], position[12], laps[12];
		const char *name = CustomCup_ResolveTrack(s_cupSelectPreview.tracks[i], NativeTrackLibrary_Current(), &level)
		                       ? LevelInfo_DisplayTitle(level, sdata->lngStrings, title, sizeof(title)) : "TRACK UNAVAILABLE";
		snprintf(text, sizeof(text), "%s", name);
		snprintf(position, sizeof(position), "%d", i + 1);
		snprintf(laps, sizeof(laps), "%d", s_cupSelectPreview.laps[i]);
		TrackText_Scroll(text, titleWidth, FONT_SMALL, gGT->timer);
		int y = CUSTOM_CUP_SELECT_PREVIEW_Y + i * CUSTOM_CUP_SELECT_PREVIEW_STEP;
		DecalFont_DrawLineOT(position, positionX + numberWidth, y, FONT_SMALL, JUSTIFY_RIGHT | ORANGE, ot);
		DecalFont_DrawLineOT(text, titleX, y, FONT_SMALL, ORANGE, ot);
		DecalFont_DrawLineOT(laps, lapsX + numberWidth, y, FONT_SMALL, JUSTIFY_RIGHT | ORANGE, ot);
	}
	if (s_cupCarousel.selected > 0)
	{
		int headerTop = CUSTOM_CUP_SELECT_PREVIEW_TOP + CUSTOM_CUP_SELECT_PREVIEW_BORDER_HEIGHT;
		int headerY = headerTop + (CUSTOM_CUP_SELECT_PREVIEW_SEPARATOR_Y - headerTop - data.font_charPixHeight[FONT_SMALL]) / 2;
		DecalFont_DrawLineOT("N.", positionX, headerY, FONT_SMALL, ORANGE, ot);
		DecalFont_DrawLineOT("TRACK", titleX, headerY, FONT_SMALL, ORANGE, ot);
		DecalFont_DrawLineOT("L.", lapsX, headerY, FONT_SMALL, ORANGE, ot);
		RECT separator = {CUSTOM_CUP_SELECT_PREVIEW_X - CUSTOM_CUP_SELECT_PREVIEW_PADDING + 3,
		                  CUSTOM_CUP_SELECT_PREVIEW_SEPARATOR_Y,
		                  CUSTOM_CUP_SELECT_PREVIEW_WIDTH + 2 * CUSTOM_CUP_SELECT_PREVIEW_PADDING - 6,
		                  CUSTOM_CUP_SELECT_PREVIEW_BORDER_HEIGHT};
		Color borderColor;
		memcpy(&borderColor, &sdata->battleSetup_Color_UI_1, sizeof(borderColor));
		RECTMENU_DrawOuterRect_Edge(&separator, &borderColor, 0x20, ot);
		RECT preview = {CUSTOM_CUP_SELECT_PREVIEW_X - CUSTOM_CUP_SELECT_PREVIEW_PADDING,
		                CUSTOM_CUP_SELECT_PREVIEW_TOP,
		                CUSTOM_CUP_SELECT_PREVIEW_WIDTH + 2 * CUSTOM_CUP_SELECT_PREVIEW_PADDING,
		                CUSTOM_CUP_MAX_TRACKS * CUSTOM_CUP_SELECT_PREVIEW_STEP + CUSTOM_CUP_SELECT_PREVIEW_PADDING +
		                    CUSTOM_CUP_SELECT_PREVIEW_Y - CUSTOM_CUP_SELECT_PREVIEW_TOP};
		RECTMENU_DrawInnerRect(&preview, 0, ot);
	}
}

static void MM_CustomCupSelect_DrawScreen(struct GameTracker *gGT)
{
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
	struct MM_CarouselSource source = MM_CustomCupSelect_Source();
	MM_Carousel_Draw(&s_cupCarousel, &source, gGT, CUSTOM_CUP_SELECT_CAROUSEL_X,
	                 CUSTOM_CUP_SELECT_CAROUSEL_Y, (u32)MM_FRAME_COUNTER);
	MM_CustomCupSelect_DrawPreview(gGT);
	if (s_cupSelectError) { FullScreenMenu_DrawPrompt("CUP FILE ERROR OR TRACK UNAVAILABLE", ot); }
	else if (s_cupCreateUnavailable) { FullScreenMenu_DrawPrompt("CUP CREATION IS NOT YET AVAILABLE", ot); }
}

static void MM_CustomCupSelect_OnConfirm(void *context, int index)
{
	(void)context;
	if (index == 0)
	{
		s_cupCreateUnavailable = true;
		return;
	}
	if (index < 1 || (size_t)index > s_cupSelectCount || !CustomCup_IsValid(&s_cupSelectPreview))
	{
		s_cupSelectError = true;
		return;
	}
	s_cupSelectError = false;
	s_cupEditUnavailable = false;
	s_cupActionRow = CUSTOM_CUP_ACTION_PLAY;
	s_cupActionScroll.offset = 0;
	s_cupSelectPage = CUSTOM_CUP_SELECT_ACTIONS;
}

static void MM_CustomCupSelect_Play(void)
{
	struct GameTracker *gGT = GAME_TRACKER;
	int level, laps;
	if (MM_CustomCupSelect_TracksOpen(&s_cupSelectPreview) &&
	    CustomCupRace_Start(&sdata->customCupRace, &s_cupSelectPreview, NativeTrackLibrary_Current()) &&
	    CustomCupRace_Current(&sdata->customCupRace, &level, &laps))
	{
		gGT->cup.cupID = 0;
		gGT->cup.trackIndex = 0;
		memset(gGT->cup.points, 0, sizeof(gGT->cup.points));
		gGT->currLEV = level;
		gGT->numLaps = laps;
		MM_CustomCupSelect_Clear();
		MM_DESIRED_MENU = &MM_MENU_QUEUE_LOAD_TRACK;
		return;
	}
	s_cupSelectError = true;
}

static void MM_CustomCupSelect_DrawActions(struct GameTracker *gGT, const char *const *labels,
                                           int count, int selected, bool deleting)
{
	static struct MenuRow headerRows[] = {{.stringIndex = 0}, {.stringIndex = -1}};
	static struct MenuRow actionRows[] = {{.stringIndex = 0}, {.stringIndex = 0}, {.stringIndex = 0}, {.stringIndex = -1}};
	static struct MenuRow confirmRows[] = {{.stringIndex = 0}, {.stringIndex = 0}, {.stringIndex = -1}};
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
	int rowHeight = data.font_charPixHeight[FONT_BIG] + 3;
	int previewHeight = CUSTOM_CUP_MAX_TRACKS * CUSTOM_CUP_SELECT_PREVIEW_STEP + CUSTOM_CUP_SELECT_PREVIEW_PADDING +
	                    CUSTOM_CUP_SELECT_PREVIEW_Y - CUSTOM_CUP_SELECT_PREVIEW_TOP;
	int menuHeight = (1 + count + (deleting ? 1 : 0)) * rowHeight + (deleting ? 24 : 12) + 8;
	int menuY = CUSTOM_CUP_SELECT_PREVIEW_TOP + (previewHeight - menuHeight) / 2 + 4;
	int actionY = menuY + rowHeight + 12;
	int rowsY = deleting ? actionY + rowHeight + 12 : actionY;
	int centerX = CUSTOM_CUP_SELECT_MENU_X + CUSTOM_CUP_SELECT_MENU_WIDTH / 2;
	char title[CUSTOM_CUP_NAME_BYTES];
	TrackText_Copy(title, sizeof(title), s_cupSelectPreview.name);
	TrackText_Scroll(title, CUSTOM_CUP_SELECT_MENU_WIDTH - 2 * CUSTOM_CUP_SELECT_MENU_PADDING,
	                 FONT_BIG, gGT->timer);
	DecalFont_DrawLineOT(title, centerX, menuY + 2, FONT_BIG, JUSTIFY_CENTER | ORANGE, ot);
	if (deleting)
	{
		DecalFont_DrawLineOT("DELETE", centerX, actionY + 2, FONT_BIG, JUSTIFY_CENTER | ORANGE, ot);
	}
	for (int i = 0; i < count; i++)
	{
		int y = rowsY + 2 + i * rowHeight;
		DecalFont_DrawLineOT((char *)labels[i], centerX, y, FONT_BIG, JUSTIFY_CENTER | ORANGE, ot);
	}
	struct RectMenu confirmation = {.stringIndexTitle = -1, .rows = confirmRows, .rowSelected = selected};
	struct RectMenu actions = {.stringIndexTitle = -1, .rows = actionRows, .rowSelected = deleting ? CUSTOM_CUP_ACTION_DELETE : selected};
	struct RectMenu header = {.stringIndexTitle = -1, .rows = headerRows,
	                          .state = HIDE_ROW_HIGHLIGHT | DRAW_NEXT_MENU_IN_HIERARCHY,
	                          .ptrNextBox_InHierarchy = &actions};
	if (deleting)
	{
		actions.state = SHOW_ONLY_HIGHLIT_ROW | HIDE_ROW_HIGHLIGHT | DRAW_NEXT_MENU_IN_HIERARCHY;
		actions.ptrNextBox_InHierarchy = &confirmation;
	}
	RECTMENU_DrawSelf(&header, CUSTOM_CUP_SELECT_MENU_X, menuY, CUSTOM_CUP_SELECT_MENU_WIDTH);
	MM_CustomCupSelect_DrawPreview(gGT);
}

static void MM_CustomCupSelect_UpdateActions(u32 tapped, struct GameTracker *gGT)
{
	static const char *const actions[] = {"PLAY", "EDIT", "DELETE"};
	static const char *const confirmation[] = {"NO", "YES"};
	bool deleting = s_cupSelectPage == CUSTOM_CUP_SELECT_DELETE;
	s16 *row = deleting ? &s_cupDeleteRow : &s_cupActionRow;
	struct FullScreenMenu_Scroll *scroll = deleting ? &s_cupDeleteScroll : &s_cupActionScroll;
	int count = deleting ? CUSTOM_CUP_DELETE_COUNT : CUSTOM_CUP_ACTION_COUNT;
	if (FullScreenMenu_Back(tapped))
	{
		RECTMENU_ClearInput();
		s_cupSelectPage = deleting ? CUSTOM_CUP_SELECT_ACTIONS : CUSTOM_CUP_SELECT_LIST;
		s_cupSelectError = false;
		s_cupEditUnavailable = false;
		if (deleting) { MM_CustomCupSelect_UpdateActions(0, gGT); }
		else { MM_CustomCupSelect_DrawScreen(gGT); }
		return;
	}
	FullScreenMenu_NavTapped(tapped, scroll, row, count);
	if (FullScreenMenu_Confirm(tapped))
	{
		RECTMENU_ClearInput();
		if (deleting)
		{
			if (*row == CUSTOM_CUP_DELETE_YES)
			{
				int selected = s_cupCarousel.selected;
				if (NativeCustomCup_Delete(s_cupSelectEntries[selected - 1].id) == NATIVE_CUSTOM_CUP_OK)
				{
					MM_CustomCupSelect_OpenList(selected);
					MM_CustomCupSelect_DrawScreen(gGT);
					return;
				}
				s_cupSelectError = true;
			}
			s_cupSelectPage = CUSTOM_CUP_SELECT_ACTIONS;
			MM_CustomCupSelect_UpdateActions(0, gGT);
			return;
		}
		s_cupSelectError = false;
		s_cupEditUnavailable = false;
		if (*row == CUSTOM_CUP_ACTION_PLAY)
		{
			MM_CustomCupSelect_Play();
			if (MM_DESIRED_MENU == &MM_MENU_QUEUE_LOAD_TRACK) { return; }
		}
		else if (*row == CUSTOM_CUP_ACTION_EDIT) { s_cupEditUnavailable = true; }
		else if (*row == CUSTOM_CUP_ACTION_DELETE)
		{
			s_cupDeleteRow = CUSTOM_CUP_DELETE_NO;
			s_cupDeleteScroll.offset = 0;
			s_cupSelectPage = CUSTOM_CUP_SELECT_DELETE;
			MM_CustomCupSelect_UpdateActions(0, gGT);
			return;
		}
	}
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
	MM_CustomCupSelect_DrawActions(gGT, deleting ? confirmation : actions, count, *row, deleting);
	if (s_cupSelectError) { FullScreenMenu_DrawPrompt("CUP FILE ERROR OR TRACK UNAVAILABLE", ot); }
	else if (s_cupEditUnavailable) { FullScreenMenu_DrawPrompt("CUP EDITING IS NOT YET AVAILABLE", ot); }
}

/* Returns false once the original cup selector should take over. */
static bool MM_CustomCupSelect_Update(struct RectMenu *menu)
{
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_STANDARD) { return false; }
	struct GameTracker *gGT = GAME_TRACKER;
	u32 tapped = (u32)sdata->gGamepads->gamepad[0].buttonsTapped;
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_KIND) { return MM_CustomCupSelect_UpdateKind(menu, tapped); }
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_ACTIONS || s_cupSelectPage == CUSTOM_CUP_SELECT_DELETE)
	{
		MM_CustomCupSelect_UpdateActions(tapped, gGT);
		return true;
	}
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_LIST)
	{
		struct MM_CarouselSource source = MM_CustomCupSelect_Source();
		bool moving = s_cupCarousel.changeFrames > 0;
		if (MM_Carousel_Update(&s_cupCarousel, &source, tapped, (u32)MM_FRAME_COUNTER))
		{
			RECTMENU_ClearInput();
			if (MM_DESIRED_MENU == &MM_MENU_QUEUE_LOAD_TRACK) { return true; }
			if (s_cupSelectPage != CUSTOM_CUP_SELECT_LIST)
			{
				MM_CustomCupSelect_UpdateActions(0, gGT);
				return true;
			}
			tapped = 0;
		}
		if (moving) { tapped = 0; }
	}
	if (FullScreenMenu_Back(tapped))
	{
		RECTMENU_ClearInput();
		if (s_cupSelectPage == CUSTOM_CUP_SELECT_LIST)
		{
			MM_CustomCupSelect_Clear();
			s_cupSelectPage = CUSTOM_CUP_SELECT_KIND;
			s_cupKindRow = 1;
			MM_CustomCupSelect_UpdateKind(menu, 0);
		}
		return true;
	}
	MM_CustomCupSelect_DrawScreen(gGT);
	return true;
}
