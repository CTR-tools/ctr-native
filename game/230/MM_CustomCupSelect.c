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
	CUSTOM_CUP_SELECT_VISIBLE_ROWS = 8,
	CUSTOM_CUP_SELECT_ROW_Y = 58,
	CUSTOM_CUP_SELECT_ROW_STEP = 16,
	CUSTOM_CUP_SELECT_RETAIL_TRACK_COUNT = 18,
	CUSTOM_CUP_SELECT_KIND_COUNT = 2,
};

struct CustomCupSelectEntry
{
	char id[NATIVE_CUSTOM_CUP_ID_SIZE];
	char name[CUSTOM_CUP_NAME_BYTES];
};

static enum CustomCupSelectPage s_cupSelectPage;
static struct CustomCupSelectEntry *s_cupSelectEntries;
static size_t s_cupSelectCount;
static int s_cupSelectRow;
static int s_cupSelectFirst;
static struct FullScreenMenu_Scroll s_cupKindScroll;
static bool s_cupSelectError;
static struct CustomCup s_cupSelectPreview;

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

static void MM_CustomCupSelect_Preview(void)
{
	CustomCup_Clear(&s_cupSelectPreview);
	if (s_cupSelectCount != 0 &&
	    NativeCustomCup_Load(s_cupSelectEntries[s_cupSelectRow].id, &s_cupSelectPreview) != NATIVE_CUSTOM_CUP_OK)
	{
		s_cupSelectError = true;
	}
}

static void MM_CustomCupSelect_OpenList(void)
{
	MM_CustomCupSelect_Clear();
	s_cupSelectError = false;
	s_cupSelectRow = 0;
	s_cupSelectFirst = 0;
	if (NativeCustomCup_Scan(MM_CustomCupSelect_Visit, NULL) != NATIVE_CUSTOM_CUP_OK)
	{
		s_cupSelectError = true;
	}
	MM_CustomCupSelect_Preview();
	s_cupSelectPage = CUSTOM_CUP_SELECT_LIST;
}

static void MM_CustomCupSelect_Init(void)
{
	MM_CustomCupSelect_Clear();
	s_cupSelectPage = g_config.customCups && (GAME_TRACKER->gameMode1 & ARCADE_MODE) != 0
	                      ? CUSTOM_CUP_SELECT_KIND : CUSTOM_CUP_SELECT_STANDARD;
	s_cupSelectRow = 0;
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

	s16 selected = (s16)s_cupSelectRow;
	FullScreenMenu_NavTapped(tapped, &s_cupKindScroll, &selected, CUSTOM_CUP_SELECT_KIND_COUNT);
	s_cupSelectRow = selected;
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

/* Returns false once the original cup selector should take over. */
static bool MM_CustomCupSelect_Update(struct RectMenu *menu)
{
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_STANDARD) { return false; }
	struct GameTracker *gGT = GAME_TRACKER;
	u32 tapped = (u32)sdata->gGamepads->gamepad[0].buttonsTapped;
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_KIND) { return MM_CustomCupSelect_UpdateKind(menu, tapped); }
	bool confirm = (tapped & (BTN_CROSS_one | BTN_CIRCLE)) != 0;
	bool back = (tapped & BTN_TRIANGLE) != 0;
	int count = (int)s_cupSelectCount;
	int oldRow = s_cupSelectRow;
	if (s_cupSelectPage != CUSTOM_CUP_SELECT_DELETE)
	{
		if ((tapped & BTN_UP) != 0 && s_cupSelectRow > 0) { s_cupSelectRow--; }
		if ((tapped & BTN_DOWN) != 0 && s_cupSelectRow + 1 < count) { s_cupSelectRow++; }
	}
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_LIST && s_cupSelectRow != oldRow)
	{
		MM_CustomCupSelect_Preview();
	}
	if (back)
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
			s_cupSelectRow = 1;
		}
		return true;
	}
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_LIST && count != 0 && (tapped & BTN_SQUARE_one) != 0)
	{
		s_cupSelectPage = CUSTOM_CUP_SELECT_DELETE;
		RECTMENU_ClearInput();
	}
	else if (confirm)
	{
		RECTMENU_ClearInput();
		if (s_cupSelectPage == CUSTOM_CUP_SELECT_DELETE)
		{
			if (NativeCustomCup_Delete(s_cupSelectEntries[s_cupSelectRow].id) == NATIVE_CUSTOM_CUP_OK)
			{
				MM_CustomCupSelect_OpenList();
			}
			else
			{
				s_cupSelectError = true;
				s_cupSelectPage = CUSTOM_CUP_SELECT_LIST;
			}
		}
		else if (s_cupSelectCount != 0)
		{
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
				return true;
			}
			s_cupSelectError = true;
		}
	}
	if (s_cupSelectRow < s_cupSelectFirst) { s_cupSelectFirst = s_cupSelectRow; }
	if (s_cupSelectRow >= s_cupSelectFirst + CUSTOM_CUP_SELECT_VISIBLE_ROWS)
	{
		s_cupSelectFirst = s_cupSelectRow - CUSTOM_CUP_SELECT_VISIBLE_ROWS + 1;
	}
	DecalFont_DrawLine("CUSTOM CUPS", 256, 24, FONT_BIG, JUSTIFY_CENTER | ORANGE);
	count = (int)s_cupSelectCount;
	for (int i = s_cupSelectFirst; i < count && i < s_cupSelectFirst + CUSTOM_CUP_SELECT_VISIBLE_ROWS; i++)
	{
		char name[CUSTOM_CUP_NAME_BYTES];
		const char *label = s_cupSelectEntries[i].name;
		TrackText_Copy(name, sizeof(name), label);
		TrackText_Scroll(name, 210, FONT_SMALL, i == s_cupSelectRow ? gGT->timer : 0);
		DecalFont_DrawLine(name, 40, CUSTOM_CUP_SELECT_ROW_Y + (i - s_cupSelectFirst) * CUSTOM_CUP_SELECT_ROW_STEP,
		                   FONT_SMALL, i == s_cupSelectRow ? WHITE : ORANGE);
	}
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_LIST || s_cupSelectPage == CUSTOM_CUP_SELECT_DELETE)
	{
		if (count == 0) { DecalFont_DrawLine("NO SAVED CUPS", 40, CUSTOM_CUP_SELECT_ROW_Y, FONT_SMALL, ORANGE); }
		for (int i = 0; i < s_cupSelectPreview.trackCount; i++)
		{
			int level;
			char title[128], text[160];
			const char *name = CustomCup_ResolveTrack(s_cupSelectPreview.tracks[i], NativeTrackLibrary_Current(), &level)
			                       ? LevelInfo_DisplayTitle(level, sdata->lngStrings, title, sizeof(title)) : "TRACK UNAVAILABLE";
			snprintf(text, sizeof(text), "%d. %s (%d)", i + 1, name, s_cupSelectPreview.laps[i]);
			TrackText_Scroll(text, 220, FONT_SMALL, gGT->timer);
			DecalFont_DrawLine(text, 270, 45 + i * 14, FONT_SMALL, ORANGE);
		}
	}
	if (s_cupSelectPage == CUSTOM_CUP_SELECT_DELETE)
	{
		DecalFont_DrawLine("DELETE? CONFIRM OR BACK", 256, 190, FONT_SMALL, JUSTIFY_CENTER | WHITE);
	}
	else if (s_cupSelectPage == CUSTOM_CUP_SELECT_LIST && count != 0)
	{
		DecalFont_DrawLine("SQUARE: DELETE", 256, 190, FONT_SMALL, JUSTIFY_CENTER | ORANGE);
	}
	if (s_cupSelectError) { DecalFont_DrawLine("CUP FILE ERROR OR TRACK UNAVAILABLE", 256, 205, FONT_SMALL, JUSTIFY_CENTER | RED); }
	return true;
}
