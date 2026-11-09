#include <common.h>
#include <platform/native_assets.h>
#include <platform/native_track_library.h>
#include <track_text.h>
#include <track_import_view.h>
#include <stdio.h>

#include "MM_FullScreenMenu.c"

enum CustomTracksPage
{
	CUSTOM_TRACKS_CLOSED,
	CUSTOM_TRACKS_ROOT,
	CUSTOM_TRACKS_AVAILABLE,
	CUSTOM_TRACKS_IMPORT,
	CUSTOM_TRACKS_REFRESH_RESULT,
};

enum
{
	CUSTOM_TRACKS_TITLE_BYTES = 128,
	CUSTOM_TRACKS_DETAIL_BYTES = 256,
	CUSTOM_TRACKS_TOGGLE_WIDTH = 70,
	CUSTOM_TRACKS_DETAIL_INDENT = 12,
	CUSTOM_TRACKS_PATH_BYTES = 1024,
};

static const struct { unsigned int bit; const char *name; } s_customTrackModes[] = {
	{NATIVE_TRACK_DETECTED_TIME_TRIAL, "TT"}, {NATIVE_TRACK_DETECTED_ARCADE, "ARC"}, {NATIVE_TRACK_DETECTED_VS, "VS"},
	{NATIVE_TRACK_DETECTED_CTR, "CTR"}, {NATIVE_TRACK_DETECTED_RELIC, "REL"}, {NATIVE_TRACK_DETECTED_CRYSTAL, "CRY"},
};

enum { CUSTOM_TRACKS_BASE_ROWS = 3 };

static enum CustomTracksPage s_customTracksPage;
static struct FullScreenMenu_Scroll s_customTracksScroll;
static s16 s_customTracksReturnRow;
static b32 s_customTracksImportEnabled;
static char s_customTracksRefreshMessage[128];

void MM_CustomTracks_ResetNavigation(void)
{
	MM_TrackImport_Reset();
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
	const int color = s_customTracksPage == CUSTOM_TRACKS_ROOT && row != 0 && !s_customTracksImportEnabled ? GRAY : ORANGE;
	DecalFont_DrawLineOT((char *)labels[row], FULLSCREEN_MENU_LABEL_X, y, FONT_SMALL, color, ot);
}

static void CustomTracks_FormatText(char *out, size_t capacity, const char *source, int width)
{
	TrackText_Copy(out, capacity, source);
	TrackText_Trim(out, width, FONT_SMALL);
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
	if (row < 0) { return; }
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE + row / CUSTOM_TRACKS_BASE_ROWS);
	row %= CUSTOM_TRACKS_BASE_ROWS;
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
		char modes[CUSTOM_TRACKS_DETAIL_BYTES] = "";
		if (track->detectionKnown)
		{
			for (size_t i = 0; i < len(s_customTrackModes); i++)
			{
				if ((track->detectedModes & s_customTrackModes[i].bit) != 0)
				{
					if (modes[0] != '\0') { strcat(modes, ", "); }
					strcat(modes, s_customTrackModes[i].name);
				}
			}
		}
		CustomTracks_DrawDetail("Modes", modes, y, ot);
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
	int totalRows = count * CUSTOM_TRACKS_BASE_ROWS;
	int selectedRow = menu->rowSelected * CUSTOM_TRACKS_BASE_ROWS;
	int selectedEnd = selectedRow + CUSTOM_TRACKS_BASE_ROWS - 1;
	struct FullScreenMenu_Scroll scroll = {0};
	scroll.offset = FullScreenMenu_ScrollOffsetFor(&scroll, selectedEnd, totalRows);
	FullScreenMenu_DrawScreen("Available Tracks", 0, &scroll, totalRows, selectedRow, CustomTracks_DrawRow, registry, gGT, ot);
}

static void CustomTracks_DrawImportReviewRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	(void)selected;
	const struct TrackImportReviewView *view = context;
	static const char *const labels[] = {"Track", "Name", "Author", "Confirm import", "Cancel import"};
	FullScreenMenu_DrawRowLabel(labels[row], y, ot);
	char value[CUSTOM_TRACKS_DETAIL_BYTES];
	if (row == TRACK_REVIEW_TRACK)
	{
		snprintf(value, sizeof(value), "%d / %d", view->index + 1, view->count);
	}
	else if (row == TRACK_REVIEW_TITLE || row == TRACK_REVIEW_AUTHOR)
	{
		const char *text = view->editing && view->row == row ? view->editingValue :
		                   row == TRACK_REVIEW_TITLE ? view->title : view->author;
		if (view->editing && view->row == row)
		{
			TrackText_Copy(value, sizeof(value), text != NULL ? text : "");
			FullScreenMenu_DrawEditableValue(value, FULLSCREEN_MENU_CONTENT_W / 2, y, ot);
			return;
		}
		CustomTracks_FormatText(value, sizeof(value), text != NULL ? text : "", FULLSCREEN_MENU_CONTENT_W / 2);
	}
	else { return; }
	FullScreenMenu_DrawValueText(value, y, ot);
}

static void CustomTracks_DrawImportReview(const struct TrackImportReviewView *view, struct GameTracker *gGT, uint32_t *ot)
{
	struct FullScreenMenu_Scroll scroll = {0};
	FullScreenMenu_DrawScreen("Review Import", 0, &scroll,
	                          TRACK_REVIEW_ROWS, view->row, CustomTracks_DrawImportReviewRow, view, gGT, ot);
	if (view->editing)
	{
		FullScreenMenu_DrawPrompt("Enter: save  Esc: cancel", ot);
	}
	else if (view->message != NULL)
	{
		FullScreenMenu_DrawPrompt(view->message, ot);
	}
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
		struct TrackImportReviewView view;
		if (MM_TrackImport_GetReview(&view)) { CustomTracks_DrawImportReview(&view, gGT, ot); }
		else { FullScreenMenu_DrawMessage("Import Track", MM_TrackImport_Message(), gGT, ot); }
		return true;
	}
	if (s_customTracksPage == CUSTOM_TRACKS_REFRESH_RESULT)
	{
		if (FullScreenMenu_Back((u32)pad->buttonsTapped) || FullScreenMenu_Confirm((u32)pad->buttonsTapped))
		{
			s_customTracksPage = CUSTOM_TRACKS_ROOT;
			menu->rowSelected = 2;
		}
		else
		{
			FullScreenMenu_DrawMessage("Refresh", s_customTracksRefreshMessage, gGT, ot);
		}
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
	static const char *rootItems[] = {"Available Tracks", "Import", "Refresh"};
	static const char *importItems[] = {"Folder", "Xdelta patch"};
	const bool choosingImport = s_customTracksPage == CUSTOM_TRACKS_IMPORT;
	const char *const *items = choosingImport ? importItems : rootItems;
	const int count = choosingImport ? (int)len(importItems) : (int)len(rootItems);
	FullScreenMenu_NavScroll(pad, &s_customTracksScroll, &menu->rowSelected, count);
	const bool canOpen = choosingImport || menu->rowSelected == 0 || s_customTracksImportEnabled;
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
		else if (menu->rowSelected == 2)
		{
			char path[CUSTOM_TRACKS_PATH_BYTES];
			struct NativeTrackScanResult result;
			int length = snprintf(path, sizeof(path), "%s/tracks", NativeAssets_GetAssetDir());
			if (length >= 0 && (size_t)length < sizeof(path) && NativeTrackLibrary_Refresh(path, &result))
			{
				snprintf(s_customTracksRefreshMessage, sizeof(s_customTracksRefreshMessage), "Tracks: %zu  Rejected: %zu", result.installed, result.rejected);
			}
			else
			{
				snprintf(s_customTracksRefreshMessage, sizeof(s_customTracksRefreshMessage), "Refresh failed");
			}
			s_customTracksPage = CUSTOM_TRACKS_REFRESH_RESULT;
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
