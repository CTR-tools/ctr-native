#ifndef CTR_TRACK_RELIC_MENU_IMPL
#define CTR_TRACK_RELIC_MENU_IMPL

static int s_relicPage, s_relicRow, s_relicEditing;
static const char *s_relicMessage;

/* Returns 1 after a save, -1 on exit, and 0 while the submenu remains open. */
static int TrackRelic_Update(const struct NativeTrackMetadata *track, u32 buttons, const char **message)
{
	if (s_relicEditing)
	{
		enum NativeTextEditResult result = NativeTextEdit_Result();
		if (result == NATIVE_TEXT_EDIT_ACTIVE) { return 0; }
		int saved = 0;
		if (result == NATIVE_TEXT_EDIT_ACCEPTED)
		{
			const char *value = NativeTextEdit_Value();
			if (!NativeTrackLibrary_ParseRelicTime(value, NULL)) { *message = LANG_GetString(LNG_EXPANDED_TRACKS_RELIC_INVALID); OtherFX_Play(2, 1); }
			else if (!NativeTrackLibrary_SetRelicTime(track, s_relicRow, value)) { *message = LANG_GetString(LNG_EXPANDED_TRACKS_SAVE_DETAILS_FAILED); OtherFX_Play(2, 1); }
			else { *message = NULL; saved = 1; }
		}
		NativeTextEdit_Close();
		s_relicEditing = 0;
		return saved;
	}
	struct FullScreenMenu_Scroll scroll = {0};
	s16 selected = (s16)s_relicRow;
	FullScreenMenu_NavTapped(buttons, &scroll, &selected, NATIVE_TRACK_RELIC_COUNT + 1);
	s_relicRow = selected;
	if (FullScreenMenu_Back(buttons) || (FullScreenMenu_Confirm(buttons) && s_relicRow == NATIVE_TRACK_RELIC_COUNT)) { return -1; }
	if (FullScreenMenu_Confirm(buttons))
	{
		char value[NATIVE_TRACK_RELIC_TEXT_BYTES];
		NativeTrackLibrary_FormatRelicTime(NativeTrackLibrary_RelicTime(track, s_relicRow), value, sizeof(value));
		s_relicEditing = NativeTextEdit_BeginForGame(value, sizeof(value));
		if (s_relicEditing) { NativeTextEdit_LimitCharacters(NATIVE_TRACK_RELIC_TEXT_BYTES - 1); }
		else { *message = LANG_GetString(LNG_EXPANDED_TRACKS_EDITOR_FAILED); }
	}
	return 0;
}

#endif
