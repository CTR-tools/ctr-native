#include <common.h>
#include <platform/native_track_library.h>
#include <stdio.h>

#include "MM_FullScreenMenu.c"

enum CustomTracksPage
{
	CUSTOM_TRACKS_CLOSED,
	CUSTOM_TRACKS_ROOT,
	CUSTOM_TRACKS_AVAILABLE,
};

enum
{
	CUSTOM_TRACKS_TITLE_BYTES = 128,
	CUSTOM_TRACKS_DETAIL_BYTES = 256,
	CUSTOM_TRACKS_TOGGLE_WIDTH = 70,
	CUSTOM_TRACKS_DETAIL_INDENT = 12,
	CUSTOM_TRACKS_ROWS_PER_TRACK = 3,
};

static enum CustomTracksPage s_customTracksPage;
static struct FullScreenMenu_Scroll s_customTracksScroll;
static s16 s_customTracksReturnRow;

void MM_CustomTracks_ResetNavigation(void)
{
	s_customTracksPage = CUSTOM_TRACKS_CLOSED;
	s_customTracksScroll.offset = 0;
}

void MM_CustomTracks_Open(struct RectMenu *menu)
{
	s_customTracksReturnRow = menu->rowSelected;
	s_customTracksPage = CUSTOM_TRACKS_ROOT;
	s_customTracksScroll.offset = 0;
	menu->rowSelected = 0;
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
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(context,
		NATIVE_TRACK_CUSTOM_ID_BASE + row / CUSTOM_TRACKS_ROWS_PER_TRACK);
	if (track == NULL)
	{
		return;
	}
	switch (row % CUSTOM_TRACKS_ROWS_PER_TRACK)
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
		char modes[CUSTOM_TRACKS_DETAIL_BYTES];
		if (track->modes != NULL)
		{
			snprintf(modes, sizeof(modes), "%s", track->modes);
			for (char *p = modes; *p; p++)
			{
				if (*p == '_')
				{
					*p = ' ';
				}
			}
		}
		CustomTracks_DrawDetail("Game modes", track->modes != NULL ? modes : NULL, y, ot);
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
	FullScreenMenu_DrawGroupedScreen("Available Tracks", CUSTOM_TRACKS_ROWS_PER_TRACK, count, menu->rowSelected,
	                                 CustomTracks_DrawRow, registry, gGT, ot);
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
		if (s_customTracksPage == CUSTOM_TRACKS_AVAILABLE)
		{
			s_customTracksPage = CUSTOM_TRACKS_ROOT;
			menu->rowSelected = 0;
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
	static const char *items[] = {"Available Tracks", "Import"};
	FullScreenMenu_NavScroll(pad, &s_customTracksScroll, &menu->rowSelected, (int)len(items));
	if (FullScreenMenu_Confirm((u32)pad->buttonsTapped))
	{
		if (menu->rowSelected == 1)
		{
			MM_TrackImport_Open();
		}
		else
		{
			s_customTracksPage = CUSTOM_TRACKS_AVAILABLE;
			menu->rowSelected = 0;
			s_customTracksScroll.offset = 0;
		}
		return true;
	}
	FullScreenMenu_DrawScreen("Custom Tracks", 0, &s_customTracksScroll, (int)len(items),
	                          menu->rowSelected, FullScreenMenu_DrawTextListRow, items, gGT, ot);
	return true;
}
