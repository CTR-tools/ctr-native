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
	CUSTOM_CUP_SELECT_DELETE,
};

enum
{
	CUSTOM_CUP_SELECT_RETAIL_TRACK_COUNT = 18,
	CUSTOM_CUP_SELECT_KIND_COUNT = 2,
	CUSTOM_CUP_SELECT_CAROUSEL_X = 0,
	CUSTOM_CUP_SELECT_CAROUSEL_Y = 0,
	CUSTOM_CUP_SELECT_PREVIEW_WIDTH = 200,
	CUSTOM_CUP_SELECT_PREVIEW_X = 300,
	CUSTOM_CUP_SELECT_PREVIEW_Y = 34,
	CUSTOM_CUP_SELECT_PREVIEW_STEP = 15,
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
static struct MM_Carousel s_cupCarousel;
static bool s_cupSelectError;
static bool s_cupCreateUnavailable;
static struct CustomCup s_cupSelectPreview;

static void MM_CustomCupSelect_OnSelected(void *context, int index);
static void MM_CustomCupSelect_OnConfirm(void *context, int index);

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

static void MM_CustomCupSelect_OpenList(void)
{
	MM_CustomCupSelect_Clear();
	s_cupSelectError = false;
	s_cupCreateUnavailable = false;
	if (NativeCustomCup_Scan(MM_CustomCupSelect_Visit, NULL) != NATIVE_CUSTOM_CUP_OK)
	{
		s_cupSelectError = true;
	}
	struct MM_CarouselSource source = MM_CustomCupSelect_Source();
	MM_Carousel_Init(&s_cupCarousel, &source, 0, (u32)MM_FRAME_COUNTER);
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
		MM_CustomCupSelect_OpenList();
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
	for (int i = 0; i < s_cupSelectPreview.trackCount; i++)
	{
		int level;
		char title[128], text[160];
		const char *name = CustomCup_ResolveTrack(s_cupSelectPreview.tracks[i], NativeTrackLibrary_Current(), &level)
		                       ? LevelInfo_DisplayTitle(level, sdata->lngStrings, title, sizeof(title)) : "TRACK UNAVAILABLE";
		snprintf(text, sizeof(text), "%d. %s (%d)", i + 1, name, s_cupSelectPreview.laps[i]);
		TrackText_Scroll(text, CUSTOM_CUP_SELECT_PREVIEW_WIDTH, FONT_SMALL, gGT->timer);
		DecalFont_DrawLineOT(text, CUSTOM_CUP_SELECT_PREVIEW_X, CUSTOM_CUP_SELECT_PREVIEW_Y + i * CUSTOM_CUP_SELECT_PREVIEW_STEP,
		                     FONT_SMALL, ORANGE, ot);
	}
	if (s_cupCarousel.selected > 0)
	{
		RECT preview = {CUSTOM_CUP_SELECT_PREVIEW_X - 6, CUSTOM_CUP_SELECT_PREVIEW_Y - 6,
		                CUSTOM_CUP_SELECT_PREVIEW_WIDTH + 12, CUSTOM_CUP_MAX_TRACKS * CUSTOM_CUP_SELECT_PREVIEW_STEP + 12};
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
	else if (s_cupCarousel.selected > 0) { FullScreenMenu_DrawPrompt("SQUARE: DELETE", ot); }
}

static void MM_CustomCupSelect_OnConfirm(void *context, int index)
{
	(void)context;
	if (index == 0)
	{
		s_cupCreateUnavailable = true;
		return;
	}
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

/* Returns false once the original cup selector should take over. */
static bool MM_CustomCupSelect_Update(struct RectMenu *menu)
{
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_STANDARD) { return false; }
	struct GameTracker *gGT = GAME_TRACKER;
	u32 tapped = (u32)sdata->gGamepads->gamepad[0].buttonsTapped;
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_KIND) { return MM_CustomCupSelect_UpdateKind(menu, tapped); }
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_LIST)
	{
		struct MM_CarouselSource source = MM_CustomCupSelect_Source();
		bool moving = s_cupCarousel.changeFrames > 0;
		if (MM_Carousel_Update(&s_cupCarousel, &source, tapped, (u32)MM_FRAME_COUNTER))
		{
			RECTMENU_ClearInput();
			if (MM_DESIRED_MENU == &MM_MENU_QUEUE_LOAD_TRACK) { return true; }
			tapped = 0;
		}
		if (moving) { tapped = 0; }
	}
	if (FullScreenMenu_Back(tapped))
	{
		RECTMENU_ClearInput();
		if (s_cupSelectPage == CUSTOM_CUP_SELECT_DELETE)
		{
			s_cupSelectPage = CUSTOM_CUP_SELECT_LIST;
		}
		else if (s_cupSelectPage == CUSTOM_CUP_SELECT_LIST)
		{
			MM_CustomCupSelect_Clear();
			s_cupSelectPage = CUSTOM_CUP_SELECT_KIND;
			s_cupKindRow = 1;
		}
		return true;
	}
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_LIST && s_cupCarousel.selected > 0 && (tapped & BTN_SQUARE_one) != 0)
	{
		s_cupSelectPage = CUSTOM_CUP_SELECT_DELETE;
		RECTMENU_ClearInput();
	}
	else if (s_cupSelectPage == CUSTOM_CUP_SELECT_DELETE && FullScreenMenu_Confirm(tapped))
	{
		RECTMENU_ClearInput();
		if (NativeCustomCup_Delete(s_cupSelectEntries[s_cupCarousel.selected - 1].id) == NATIVE_CUSTOM_CUP_OK)
		{
			MM_CustomCupSelect_OpenList();
		}
		else
		{
			s_cupSelectError = true;
			s_cupSelectPage = CUSTOM_CUP_SELECT_LIST;
		}
	}
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_DELETE)
	{
		bool wasShaded = s_frameDrawStyle == FULLSCREEN_MENU_DRAW_STYLE_SHADED;
		FullScreenMenu_SetShadedFrame(true);
		FullScreenMenu_DrawMessage("CUSTOM CUPS", "DELETE? CONFIRM OR BACK", gGT, gGT->backBuffer->otMem.uiOT);
		FullScreenMenu_SetShadedFrame(wasShaded);
	}
	else
	{
		MM_CustomCupSelect_DrawScreen(gGT);
	}
	return true;
}
