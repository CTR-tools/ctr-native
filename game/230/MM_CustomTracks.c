#include <common.h>
#include <platform/native_track_library.h>
#include <stdio.h>

#include "MM_FullScreenMenu.c"

enum CustomTracksPage
{
	CUSTOM_TRACKS_CLOSED,
	CUSTOM_TRACKS_ROOT,
	CUSTOM_TRACKS_AVAILABLE,
	CUSTOM_TRACKS_IMPORT,
};

enum
{
	CUSTOM_TRACKS_TITLE_BYTES = 128,
	CUSTOM_TRACKS_DETAIL_BYTES = 256,
	CUSTOM_TRACKS_TOGGLE_WIDTH = 70,
	CUSTOM_TRACKS_DETAIL_INDENT = 12,
};

static const struct { unsigned int bit; const char *name; } s_customTrackModes[] = {
	{NATIVE_TRACK_DETECTED_TIME_TRIAL, "Time Trial"}, {NATIVE_TRACK_DETECTED_VS, "Versus"},
	{NATIVE_TRACK_DETECTED_ARCADE, "Arcade"}, {NATIVE_TRACK_DETECTED_RELIC, "Relic Race"},
	{NATIVE_TRACK_DETECTED_CTR, "CTR Challenge"}, {NATIVE_TRACK_DETECTED_CRYSTAL, "Crystal Challenge"},
};

enum { CUSTOM_TRACKS_BASE_ROWS = 3, CUSTOM_TRACKS_MODE_INDENT = 24 };

static int CustomTracks_ModeCount(const struct NativeTrackMetadata *track)
{
	if (!track->detectionKnown)
	{
		return 1;
	}
	int count = 0;
	for (size_t i = 0; i < len(s_customTrackModes); i++)
	{
		if ((track->detectedModes & s_customTrackModes[i].bit) != 0) { count++; }
	}
	return count;
}

static int CustomTracks_TrackRows(const struct NativeTrackMetadata *track)
{
	return CUSTOM_TRACKS_BASE_ROWS + CustomTracks_ModeCount(track);
}

static enum CustomTracksPage s_customTracksPage;
static struct FullScreenMenu_Scroll s_customTracksScroll;
static s16 s_customTracksReturnRow;
static b32 s_customTracksImportEnabled;

void MM_CustomTracks_ResetNavigation(void)
{
	s_customTracksPage = CUSTOM_TRACKS_CLOSED;
	s_customTracksScroll.offset = 0;
}

void MM_CustomTracks_Open(struct RectMenu *menu, b32 allowImport)
{
	s_customTracksImportEnabled = allowImport;
	s_customTracksReturnRow = menu->rowSelected;
	s_customTracksPage = CUSTOM_TRACKS_ROOT;
	s_customTracksScroll.offset = 0;
	menu->rowSelected = 0;
}

static void CustomTracks_DrawMenuRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	const char *const *labels = context;
	(void)selected;
	const int color = s_customTracksPage == CUSTOM_TRACKS_ROOT && row == 1 && !s_customTracksImportEnabled ? GRAY : ORANGE;
	DecalFont_DrawLineOT((char *)labels[row], FULLSCREEN_MENU_LABEL_X, y, FONT_SMALL, color, ot);
}

// Metadata is free-form UTF-8; the retail font also reserves ASCII glyphs for buttons.
static void CustomTracks_FormatText(char *out, size_t capacity, const char *source, int width)
{
	size_t length = 0;
	for (const unsigned char *p = (const unsigned char *)source; *p && length + 1 < capacity; p++)
	{
		if (*p >= 0x80 && *p < 0xc0)
		{
			continue;
		}
		out[length++] = *p < 0x20 || *p >= 0x7f || *p == '@' || *p == '[' || *p == '^' || *p == '*' ? '?' : (char)*p;
	}
	out[length] = '\0';
	while (length > 0 && DecalFont_GetLineWidth(out, FONT_SMALL) > width)
	{
		out[--length] = '\0';
	}
}

static void CustomTracks_DrawDetail(const char *label, const char *value, int y, uint32_t *ot)
{
	char text[CUSTOM_TRACKS_DETAIL_BYTES], safe[CUSTOM_TRACKS_DETAIL_BYTES];
	snprintf(text, sizeof(text), "%s: %s", label, value != NULL && value[0] != '\0' ? value : "Unknown");
	CustomTracks_FormatText(safe, sizeof(safe), text, FULLSCREEN_MENU_CONTENT_W - 2 * CUSTOM_TRACKS_DETAIL_INDENT);
	DecalFont_DrawLineOT(safe, FULLSCREEN_MENU_LABEL_X + CUSTOM_TRACKS_DETAIL_INDENT, y, FONT_SMALL, WHITE, ot);
}

static void CustomTracks_DrawRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	(void)selected;
	const struct NativeTrackRegistry *registry = context;
	const struct NativeTrackMetadata *track = NULL;
	for (size_t i = 0; i < NativeTrackRegistry_Count(registry); i++)
	{
		track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE + (int)i);
		int rows = CustomTracks_TrackRows(track);
		if (row < rows) { break; }
		row -= rows;
		track = NULL;
	}
	if (track == NULL)
	{
		return;
	}
	switch (row)
	{
	case 0:
	{
		char title[CUSTOM_TRACKS_TITLE_BYTES];
		CustomTracks_FormatText(title, sizeof(title), track->title, FULLSCREEN_MENU_CONTENT_W - CUSTOM_TRACKS_TOGGLE_WIDTH);
		FullScreenMenu_DrawRowLabel(title, y, ot);
		FullScreenMenu_DrawOnOff(NativeTrackLibrary_IsEnabled(track), y, ot);
		break;
	}
	case 1:
		CustomTracks_DrawDetail("Author", track->author, y, ot);
		break;
	case 2:
	{
		DecalFont_DrawLineOT("Available modes:", FULLSCREEN_MENU_LABEL_X + CUSTOM_TRACKS_DETAIL_INDENT, y, FONT_SMALL, WHITE, ot);
		break;
	}
	default:
	{
		const char *name = "Unknown";
		int modeRow = row - CUSTOM_TRACKS_BASE_ROWS;
		if (track->detectionKnown)
		{
			for (size_t i = 0; i < len(s_customTrackModes); i++)
			{
				if ((track->detectedModes & s_customTrackModes[i].bit) != 0 && modeRow-- == 0)
				{
					name = s_customTrackModes[i].name;
					break;
				}
			}
		}
		DecalFont_DrawLineOT((char *)name, FULLSCREEN_MENU_LABEL_X + CUSTOM_TRACKS_MODE_INDENT, y, FONT_SMALL, WHITE, ot);
		break;
	}
	}
}

static void CustomTracks_DrawAvailable(struct RectMenu *menu, struct GamepadBuffer *pad, struct GameTracker *gGT, uint32_t *ot)
{
	const struct NativeTrackRegistry *registry = NativeTrackLibrary_Current();
	int count = (int)NativeTrackRegistry_Count(registry);
	if (count == 0)
	{
		FullScreenMenu_DrawMessage("Available Tracks", "No tracks imported", gGT, ot);
		return;
	}
	FullScreenMenu_NavScroll(pad, &s_customTracksScroll, &menu->rowSelected, count);
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE + menu->rowSelected);
	if ((pad->buttonsTapped & (BTN_CROSS | BTN_CIRCLE)) != 0 && track != NULL)
	{
		bool changed = NativeTrackLibrary_SetEnabled(track, !NativeTrackLibrary_IsEnabled(track));
		OtherFX_Play(changed ? 1 : 2, 1);
	}
	int totalRows = 0, selectedRow = 0, selectedEnd = 0;
	for (int i = 0; i < count; i++)
	{
		const struct NativeTrackMetadata *item = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE + i);
		int rows = CustomTracks_TrackRows(item);
		if (i == menu->rowSelected)
		{
			selectedRow = totalRows;
			selectedEnd = totalRows + rows - 1;
		}
		totalRows += rows;
	}
	struct FullScreenMenu_Scroll scroll = {0};
	scroll.offset = FullScreenMenu_ScrollOffsetFor(&scroll, selectedEnd, totalRows);
	FullScreenMenu_DrawScreen("Available Tracks", 0, &scroll, totalRows, selectedRow, CustomTracks_DrawRow, registry, gGT, ot);
}

// Uses the caller's row while open, restoring it when returning to Options.
b32 MM_CustomTracks_Update(struct RectMenu *menu)
{
	if (s_customTracksPage == CUSTOM_TRACKS_CLOSED)
	{
		return false;
	}
	struct GameTracker *gGT = sdata->gGT;
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
	struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[0];
	if (MM_TrackImport_Update((u32)pad->buttonsTapped))
	{
		FullScreenMenu_DrawMessage("Import Track", MM_TrackImport_Message(), gGT, ot);
		return true;
	}
	if (FullScreenMenu_Back((u32)pad->buttonsTapped))
	{
		if (s_customTracksPage == CUSTOM_TRACKS_AVAILABLE || s_customTracksPage == CUSTOM_TRACKS_IMPORT)
		{
			menu->rowSelected = s_customTracksPage == CUSTOM_TRACKS_IMPORT ? 1 : 0;
			s_customTracksPage = CUSTOM_TRACKS_ROOT;
		}
		else
		{
			s_customTracksPage = CUSTOM_TRACKS_CLOSED;
			menu->rowSelected = s_customTracksReturnRow;
		}
		s_customTracksScroll.offset = 0;
		return true;
	}
	if (s_customTracksPage == CUSTOM_TRACKS_AVAILABLE)
	{
		CustomTracks_DrawAvailable(menu, pad, gGT, ot);
		return true;
	}
	static const char *rootItems[] = {"Available Tracks", "Import"};
	static const char *importItems[] = {"Folder", "Xdelta patch"};
	const bool choosingImport = s_customTracksPage == CUSTOM_TRACKS_IMPORT;
	const char *const *items = choosingImport ? importItems : rootItems;
	const int count = choosingImport ? (int)len(importItems) : (int)len(rootItems);
	FullScreenMenu_NavScroll(pad, &s_customTracksScroll, &menu->rowSelected, count);
	const bool canOpen = choosingImport || menu->rowSelected != 1 || s_customTracksImportEnabled;
	if (canOpen && FullScreenMenu_Confirm((u32)pad->buttonsTapped))
	{
		if (choosingImport && menu->rowSelected == 0)
		{
			MM_TrackImport_Open();
		}
		else if (choosingImport)
		{
			MM_TrackImport_OpenPatch();
		}
		else
		{
			s_customTracksPage = menu->rowSelected == 1 ? CUSTOM_TRACKS_IMPORT : CUSTOM_TRACKS_AVAILABLE;
			menu->rowSelected = 0;
			s_customTracksScroll.offset = 0;
		}
		return true;
	}
	FullScreenMenu_DrawScreen(choosingImport ? "Import" : "Custom Tracks", 0, &s_customTracksScroll, count,
	                          menu->rowSelected, CustomTracks_DrawMenuRow, items, gGT, ot);
	return true;
}
