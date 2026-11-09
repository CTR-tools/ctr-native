#include <common.h>
#include <platform/native_assets.h>
#include <platform/native_track_library.h>
#include <platform/native_text_edit.h>
#include <track_text.h>
#include <track_import_view.h>
#include <stdio.h>

#include "MM_FullScreenMenu.c"

CTR_STATIC_ASSERT((int)NATIVE_TRACK_DEFAULT_RETAIL_MUSIC == (int)CRASH_COVE);

enum CustomTracksPage
{
	CUSTOM_TRACKS_CLOSED,
	CUSTOM_TRACKS_ROOT,
	CUSTOM_TRACKS_AVAILABLE,
	CUSTOM_TRACKS_DETAIL,
	CUSTOM_TRACKS_DELETE,
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
static int s_customTracksIndex;
static const char *s_customTracksMessage;
static int s_customTracksEditing;
static int s_customTracksPreview;

static void CustomTracks_PreviewMusic(const struct NativeTrackRegistry *registry, const struct NativeTrackMetadata *track);

static void CustomTracks_CloseDetail(void)
{
	if (s_customTracksEditing) { NativeTextEdit_Close(); s_customTracksEditing = 0; }
	if (s_customTracksPreview) { MusicPreview_Restore(); s_customTracksPreview = 0; }
}

void MM_CustomTracks_ResetNavigation(void)
{
	MM_TrackImport_Reset();
	CustomTracks_CloseDetail();
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
	int totalRows = count * CUSTOM_TRACKS_BASE_ROWS;
	int selectedRow = menu->rowSelected * CUSTOM_TRACKS_BASE_ROWS;
	int selectedEnd = selectedRow + CUSTOM_TRACKS_BASE_ROWS - 1;
	struct FullScreenMenu_Scroll scroll = {0};
	scroll.offset = FullScreenMenu_ScrollOffsetFor(&scroll, selectedEnd, totalRows);
	FullScreenMenu_DrawScreen("Available Tracks", 0, &scroll, totalRows, selectedRow, CustomTracks_DrawRow, registry, gGT, ot);
	if (track != NULL && FullScreenMenu_Confirm((u32)pad->buttonsTapped))
	{
		s_customTracksIndex = menu->rowSelected;
		menu->rowSelected = TRACK_REVIEW_INCLUDE;
		s_customTracksMessage = NULL;
		s_customTracksPage = CUSTOM_TRACKS_DETAIL;
		if (s_customTracksImportEnabled) { CustomTracks_PreviewMusic(registry, track); }
	}
}

static void CustomTracks_DrawImportReviewRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	(void)selected;
	const struct TrackImportReviewView *view = context;
	static const char *const labels[] = {"Track", "Import", "Name", "Author", "Music source", "Music track", "Confirm import", "Cancel import"};
	static const char *const installedLabels[] = {"Track", "Active", "Name", "Author", "Music source", "Music track", "Delete track", "Back"};
	if ((!view->installed && row == TRACK_REVIEW_CONFIRM && view->includedCount == 0) ||
	    (view->installed && !s_customTracksImportEnabled && row >= TRACK_REVIEW_TITLE && row <= TRACK_REVIEW_CONFIRM))
	{
		DecalFont_DrawLineOT((char *)(view->installed ? installedLabels[row] : labels[row]), FULLSCREEN_MENU_LABEL_X, y, FONT_SMALL, GRAY, ot);
	}
	else { FullScreenMenu_DrawRowLabel(view->installed ? installedLabels[row] : labels[row], y, ot); }
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
	else if (row == TRACK_REVIEW_MUSIC_SOURCE || row == TRACK_REVIEW_MUSIC_TRACK)
	{
		if (row == TRACK_REVIEW_MUSIC_TRACK && view->musicIndex > 0)
		{
			snprintf(value, sizeof(value), "%d", view->musicIndex);
		}
		else
		{
			const char *text = row == TRACK_REVIEW_MUSIC_SOURCE ? view->musicSource : view->musicTrack;
			CustomTracks_FormatText(value, sizeof(value), text != NULL ? text : "", FULLSCREEN_MENU_CONTENT_W / 2);
		}
	}
	else if (row == TRACK_REVIEW_INCLUDE)
	{
		snprintf(value, sizeof(value), "%s", view->included ? "Yes" : "No");
	}
	else if (row == TRACK_REVIEW_CONFIRM && !view->installed)
	{
		snprintf(value, sizeof(value), "%d / %d", view->includedCount, view->count);
		FullScreenMenu_DrawValueTextColored(value, y, view->includedCount == 0 ? GRAY : WHITE, ot);
		return;
	}
	else { return; }
	FullScreenMenu_DrawValueText(value, y, ot);
}

static void CustomTracks_DrawReviewRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	const struct TrackImportReviewView *view = context;
	CustomTracks_DrawImportReviewRow(context, row + view->installed, y, selected, ot);
}

static void CustomTracks_DrawImportReview(const struct TrackImportReviewView *view, struct GameTracker *gGT, uint32_t *ot)
{
	struct FullScreenMenu_Scroll scroll = {0};
	if (view->editing)
	{
		FullScreenMenu_DrawPrompt("Enter: save  Esc: cancel", ot);
	}
	else if (view->message != NULL)
	{
		FullScreenMenu_DrawPrompt(view->message, ot);
	}
	FullScreenMenu_DrawScreen(view->installed ? "Track Details" : "Review Import", 0, &scroll,
	                          TRACK_REVIEW_ROWS - view->installed, view->row - view->installed,
	                          CustomTracks_DrawReviewRow, view, gGT, ot);
}

static int CustomTracks_Rescan(const char *identity)
{
	char saved[CUSTOM_TRACKS_DETAIL_BYTES + 16], path[CUSTOM_TRACKS_PATH_BYTES];
	int id;
	int length = snprintf(path, sizeof(path), "%s/tracks", NativeAssets_GetAssetDir());
	snprintf(saved, sizeof(saved), "%s", identity);
	if (length < 0 || (size_t)length >= sizeof(path) || !NativeTrackLibrary_Refresh(path, NULL) ||
	    !NativeTrackRegistry_Find(NativeTrackLibrary_Current(), saved, &id)) { return 0; }
	s_customTracksIndex = id - NATIVE_TRACK_CUSTOM_ID_BASE;
	return 1;
}

static int CustomTracks_MusicChoice(const struct NativeTrackRegistry *registry, const struct NativeTrackMetadata *track, int *custom)
{
	(void)registry;
	int retail = NativeTrackLibrary_Music(track);
	*custom = retail < 0 && track->scaPath != NULL;
	if (!*custom) { return retail >= 0 ? retail : CRASH_COVE; }
	return 0;
}

static void CustomTracks_PreviewMusic(const struct NativeTrackRegistry *registry, const struct NativeTrackMetadata *track)
{
	int custom;
	int choice = CustomTracks_MusicChoice(registry, track, &custom);
	MusicPreview_Request(custom ? -1 : choice, custom ? track : NULL);
	s_customTracksPreview = 1;
}

static void CustomTracks_Detail(struct RectMenu *menu, struct GamepadBuffer *pad, struct GameTracker *gGT, uint32_t *ot)
{
	const struct NativeTrackRegistry *registry = NativeTrackLibrary_Current();
	int count = (int)NativeTrackRegistry_Count(registry);
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE + s_customTracksIndex);
	if (track == NULL) { CustomTracks_CloseDetail(); s_customTracksPage = CUSTOM_TRACKS_AVAILABLE; menu->rowSelected = 0; return; }
	u32 buttons = (u32)pad->buttonsTapped;
	if (s_customTracksEditing)
	{
		enum NativeTextEditResult result = NativeTextEdit_Result();
		if (result != NATIVE_TEXT_EDIT_ACTIVE)
		{
			if (result == NATIVE_TEXT_EDIT_ACCEPTED)
			{
				const char *value = NativeTextEdit_Value();
				if (!NativeTrackLibrary_SetDetails(track, menu->rowSelected == TRACK_REVIEW_TITLE ? value : track->title,
				    menu->rowSelected == TRACK_REVIEW_AUTHOR ? value : (track->author != NULL ? track->author : "")))
				{ s_customTracksMessage = "Cannot save track details"; OtherFX_Play(2, 1); }
				else if (!CustomTracks_Rescan(track->identity)) { s_customTracksMessage = "Cannot refresh track details"; }
				else { s_customTracksMessage = NULL; }
			}
			NativeTextEdit_Close();
			s_customTracksEditing = 0;
			goto draw;
		}
	}
	else
	{
		struct FullScreenMenu_Scroll scroll = {0};
		s16 selected = menu->rowSelected - 1;
		FullScreenMenu_NavTapped(buttons, &scroll, &selected, TRACK_REVIEW_ROWS - 1);
		menu->rowSelected = selected + 1;
		int back = FullScreenMenu_Back(buttons);
		int confirm = !back && FullScreenMenu_Confirm(buttons);
		if (back || (confirm && menu->rowSelected == TRACK_REVIEW_CANCEL))
		{
			CustomTracks_CloseDetail();
			s_customTracksPage = CUSTOM_TRACKS_AVAILABLE;
			menu->rowSelected = s_customTracksIndex;
			return;
		}
		if (menu->rowSelected == TRACK_REVIEW_INCLUDE && ((buttons & (BTN_LEFT | BTN_RIGHT)) || confirm))
		{
			if (!NativeTrackLibrary_SetEnabled(track, !NativeTrackLibrary_IsEnabled(track))) { s_customTracksMessage = "Cannot change track status"; OtherFX_Play(2, 1); }
			else { s_customTracksMessage = NULL; if (!confirm) { OtherFX_Play(0, 1); } }
		}
		else if (confirm && menu->rowSelected == TRACK_REVIEW_CONFIRM && s_customTracksImportEnabled)
		{
			s_customTracksPage = CUSTOM_TRACKS_DELETE;
			return;
		}
		else if (confirm && s_customTracksImportEnabled && (menu->rowSelected == TRACK_REVIEW_TITLE || menu->rowSelected == TRACK_REVIEW_AUTHOR))
		{
			const char *value = menu->rowSelected == TRACK_REVIEW_TITLE ? track->title : track->author;
			s_customTracksEditing = NativeTextEdit_BeginForGame(value != NULL ? value : "", 256);
			if (s_customTracksEditing) { NativeTextEdit_LimitCharacters(TRACK_REVIEW_TEXT_CHARACTERS); }
			if (!s_customTracksEditing) { s_customTracksMessage = "Cannot open text editor"; }
		}
		else if (s_customTracksImportEnabled && (menu->rowSelected == TRACK_REVIEW_MUSIC_SOURCE || menu->rowSelected == TRACK_REVIEW_MUSIC_TRACK) &&
		         ((buttons & (BTN_LEFT | BTN_RIGHT)) || confirm))
		{
			int custom, choice = CustomTracks_MusicChoice(registry, track, &custom);
			if (menu->rowSelected == TRACK_REVIEW_MUSIC_SOURCE)
			{
				if (track->scaPath == NULL) { goto draw; }
				custom = !custom;
				choice = custom ? 0 : CRASH_COVE;
			}
			else
			{
				if (custom) { goto draw; }
				int choices = TURBO_TRACK + 1;
				choice = (choice + choices + ((buttons & BTN_LEFT) ? -1 : 1)) % choices;
			}
			int ok = NativeTrackLibrary_SetMusic(track, custom ? -1 : choice, custom);
			if (!ok) { s_customTracksMessage = "Cannot save music selection"; OtherFX_Play(2, 1); }
			else if (!CustomTracks_Rescan(track->identity)) { s_customTracksMessage = "Cannot refresh track details"; }
			else
			{
				s_customTracksMessage = NULL;
				registry = NativeTrackLibrary_Current();
				track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE + s_customTracksIndex);
				if (track != NULL) { CustomTracks_PreviewMusic(registry, track); }
				if (!confirm) { OtherFX_Play(0, 1); }
			}
			goto draw;
		}
	}
draw:
	registry = NativeTrackLibrary_Current();
	count = (int)NativeTrackRegistry_Count(registry);
	track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE + s_customTracksIndex);
	if (track == NULL) { CustomTracks_CloseDetail(); s_customTracksPage = CUSTOM_TRACKS_AVAILABLE; menu->rowSelected = 0; return; }
	int custom, music = CustomTracks_MusicChoice(registry, track, &custom);
	struct TrackImportReviewView view = {
		.title = track->title, .author = track->author, .index = s_customTracksIndex, .count = count,
		.row = menu->rowSelected, .included = NativeTrackLibrary_IsEnabled(track), .installed = 1,
		.editing = s_customTracksEditing, .editingValue = s_customTracksEditing ? NativeTextEdit_Value() : NULL,
		.message = s_customTracksMessage, .musicSource = custom ? "Custom" : "Retail",
		.musicTrack = custom ? "Track audio" : sdata->lngStrings[data.metaDataLEV[music].name_LNG],
	};
	CustomTracks_DrawImportReview(&view, gGT, ot);
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
	if (s_customTracksPage == CUSTOM_TRACKS_DETAIL)
	{
		CustomTracks_Detail(menu, pad, gGT, ot);
		return true;
	}
	if (s_customTracksPage == CUSTOM_TRACKS_DELETE)
	{
		if (FullScreenMenu_Back((u32)pad->buttonsTapped)) { s_customTracksPage = CUSTOM_TRACKS_DETAIL; }
		else if (FullScreenMenu_Confirm((u32)pad->buttonsTapped))
		{
			CustomTracks_CloseDetail();
			char path[CUSTOM_TRACKS_PATH_BYTES];
			const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), NATIVE_TRACK_CUSTOM_ID_BASE + s_customTracksIndex);
			int length = snprintf(path, sizeof(path), "%s/tracks", NativeAssets_GetAssetDir());
			int ok = length >= 0 && (size_t)length < sizeof(path) && NativeTrackLibrary_Delete(track, path);
			s_customTracksPage = CUSTOM_TRACKS_AVAILABLE;
			int count = (int)NativeTrackRegistry_Count(NativeTrackLibrary_Current());
			menu->rowSelected = count > 0 && s_customTracksIndex >= count ? count - 1 : s_customTracksIndex;
			if (!ok) { OtherFX_Play(2, 1); s_customTracksPage = CUSTOM_TRACKS_REFRESH_RESULT; snprintf(s_customTracksRefreshMessage, sizeof(s_customTracksRefreshMessage), "Cannot delete track"); }
		}
		else { FullScreenMenu_DrawMessage("Delete Track", "Confirm: delete  Back: cancel", gGT, ot); }
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
	FullScreenMenu_DrawScreen(choosingImport ? "Import" : "Custom Tracks", 0, &s_customTracksScroll, count,
	                          menu->rowSelected, CustomTracks_DrawMenuRow, items, gGT, ot);
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
	return true;
}
