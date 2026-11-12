#include "MM_FullScreenMenu.c"

enum CustomCupEditPage
{
	CUP_EDIT_LIST,
	CUP_EDIT_TRACK_ACTIONS,
	CUP_EDIT_MOVE,
	CUP_EDIT_DISCARD,
	CUP_EDIT_PICK_TRACK,
};

enum
{
	CUP_EDIT_REPLACE,
	CUP_EDIT_LAPS_ACTION,
	CUP_EDIT_MOVE_ACTION,
	CUP_EDIT_REMOVE,
	CUP_EDIT_ACTION_COUNT,
	CUP_EDIT_MIN_LAPS = 1,
	CUP_EDIT_MAX_LAPS = 99,
};

static struct CustomCup s_editCup;
static enum CustomCupEditPage s_editPage;
static s16 s_editRow, s_editActionRow, s_editConfirmRow;
static int s_editMoveOrigin;
static struct FullScreenMenu_Scroll s_editScroll, s_editActionScroll, s_editConfirmScroll;
static bool s_editAdding, s_editError;

static bool MM_CustomCupEdit_Dirty(void)
{
	if (s_editCup.trackCount != s_cupSelectPreview.trackCount) { return true; }
	for (int i = 0; i < s_editCup.trackCount; i++)
	{
		if (s_editCup.laps[i] != s_cupSelectPreview.laps[i] ||
		    strcmp(s_editCup.tracks[i], s_cupSelectPreview.tracks[i]) != 0) { return true; }
	}
	return false;
}

static bool MM_CustomCupEdit_Open(void)
{
	CustomCup_Clear(&s_editCup);
	if (!CustomCup_Copy(&s_editCup, &s_cupSelectPreview)) { return false; }
	s_editPage = CUP_EDIT_LIST;
	s_editRow = 0;
	s_editScroll.offset = 0;
	s_editError = false;
	return true;
}

static void MM_CustomCupEdit_TrackConfirm(int level)
{
	char *identity = CustomCup_TrackIdentity(level, NativeTrackLibrary_Current());
	bool ok = false;
	if (identity != NULL)
	{
		if (s_editAdding)
		{
			ok = CustomCup_InsertTrack(&s_editCup, s_editCup.trackCount, identity, CUSTOM_CUP_DEFAULT_LAPS);
			if (ok) { s_editRow = s_editCup.trackCount - 1; }
		}
		else { ok = CustomCup_ReplaceTrack(&s_editCup, s_editRow, identity, s_editCup.laps[s_editRow]); }
		free(identity);
	}
	s_editError = !ok;
	s_editPage = CUP_EDIT_LIST;
}

static void MM_CustomCupEdit_OpenPicker(bool adding)
{
	s_editAdding = adding;
	int level = -1;
	if (!adding) { CustomCup_ResolveTrack(s_editCup.tracks[s_editRow], NativeTrackLibrary_Current(), &level); }
	MM_TrackSelect_OpenPicker(level);
	s_editPage = CUP_EDIT_PICK_TRACK;
}

static void MM_CustomCupEdit_DrawRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	(void)context;
	if (row < s_editCup.trackCount)
	{
		int level;
		char title[128], text[160], label[176], laps[20];
		const char *name = CustomCup_ResolveTrack(s_editCup.tracks[row], NativeTrackLibrary_Current(), &level)
		                       ? LevelInfo_DisplayTitle(level, sdata->lngStrings, title, sizeof(title)) : "TRACK UNAVAILABLE";
		snprintf(text, sizeof(text), "%s", name);
		TrackText_Scroll(text, FULLSCREEN_MENU_CONTENT_W - 120, FONT_SMALL, selected ? GAME_TRACKER->timer : 0);
		snprintf(label, sizeof(label), "%d. %s", row + 1, text);
		snprintf(laps, sizeof(laps), "%d LAPS", s_editCup.laps[row]);
		FullScreenMenu_DrawRowLabel(label, y, ot);
		FullScreenMenu_DrawValueText(laps, y, ot);
		return;
	}
	bool adding = row == s_editCup.trackCount;
	bool enabled = adding ? s_editCup.trackCount < CUSTOM_CUP_MAX_TRACKS : s_editCup.trackCount >= CUSTOM_CUP_MIN_TRACKS;
	DecalFont_DrawLineOT(adding ? "ADD TRACK" : "SAVE", FULLSCREEN_MENU_LABEL_X, y, FONT_SMALL,
	                     enabled ? ORANGE : PERIWINKLE, ot);
}

static void MM_CustomCupEdit_DrawList(struct GameTracker *gGT)
{
	char title[CUSTOM_CUP_NAME_BYTES];
	TrackText_Copy(title, sizeof(title), s_editCup.name);
	TrackText_Scroll(title, FULLSCREEN_MENU_CONTENT_W - 2 * FULLSCREEN_MENU_ROW_PADDING, FONT_BIG, gGT->timer);
	bool shaded = s_frameDrawStyle == FULLSCREEN_MENU_DRAW_STYLE_SHADED;
	FullScreenMenu_SetShadedFrame(true);
	FullScreenMenu_DrawScreen(title, 0, &s_editScroll, s_editCup.trackCount + 2, s_editRow,
	                          MM_CustomCupEdit_DrawRow, NULL, gGT, gGT->backBuffer->otMem.uiOT);
	FullScreenMenu_SetShadedFrame(shaded);
	if (s_editPage == CUP_EDIT_MOVE) { FullScreenMenu_DrawPrompt("UP / DOWN: MOVE", gGT->backBuffer->otMem.uiOT); }
	else if (s_editError) { FullScreenMenu_DrawPrompt("CUP FILE ERROR", gGT->backBuffer->otMem.uiOT); }
}

static void MM_CustomCupEdit_DrawActionRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	const char *const *labels = context;
	(void)selected;
	bool disabled = row == CUP_EDIT_REMOVE && s_editCup.trackCount <= CUSTOM_CUP_MIN_TRACKS;
	DecalFont_DrawLineOT((char *)labels[row], FULLSCREEN_MENU_LABEL_X, y, FONT_SMALL,
	                     disabled ? PERIWINKLE : ORANGE, ot);
	if (row == CUP_EDIT_LAPS_ACTION) { FullScreenMenu_DrawValueWithSuffix(s_editCup.laps[s_editRow], "", y, ot); }
}

/* Returns true when the editor has closed. */
static bool MM_CustomCupEdit_Update(u32 tapped, struct GameTracker *gGT)
{
	static const char *const actionLabels[] = {"REPLACE", "LAPS", "MOVE", "REMOVE"};
	static const char *const discardLabels[] = {"NO", "YES"};
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
	if (s_editPage == CUP_EDIT_PICK_TRACK)
	{
		MM_TrackSelect_MenuProc(&MM_MENU_TRACK_SELECT);
		if (s_trackPicker.done)
		{
			if (s_trackPicker.confirmed) { MM_CustomCupEdit_TrackConfirm(s_trackPicker.result); }
			else { s_editPage = CUP_EDIT_LIST; }
			s_trackPicker.done = false;
			MM_CustomCupEdit_DrawList(gGT);
		}
		return false;
	}
	if (s_editPage == CUP_EDIT_MOVE)
	{
		if (FullScreenMenu_Back(tapped))
		{
			CustomCup_MoveTrack(&s_editCup, s_editRow, s_editMoveOrigin);
			s_editRow = s_editMoveOrigin;
			s_editPage = CUP_EDIT_LIST;
			RECTMENU_ClearInput();
		}
		else if (FullScreenMenu_Confirm(tapped)) { s_editPage = CUP_EDIT_LIST; RECTMENU_ClearInput(); }
		else
		{
			int target = s_editRow + ((tapped & BTN_UP) != 0 ? -1 : (tapped & BTN_DOWN) != 0 ? 1 : 0);
			if (target != s_editRow && target >= 0 && target < s_editCup.trackCount && CustomCup_MoveTrack(&s_editCup, s_editRow, target))
			{
				s_editRow = target;
				OtherFX_Play(0, 1);
			}
		}
		FullScreenMenu_FollowScroll(&s_editScroll, &s_editRow, s_editCup.trackCount + 2);
		MM_CustomCupEdit_DrawList(gGT);
		return false;
	}
	if (s_editPage == CUP_EDIT_TRACK_ACTIONS || s_editPage == CUP_EDIT_DISCARD)
	{
		bool discard = s_editPage == CUP_EDIT_DISCARD;
		s16 *row = discard ? &s_editConfirmRow : &s_editActionRow;
		struct FullScreenMenu_Scroll *scroll = discard ? &s_editConfirmScroll : &s_editActionScroll;
		int count = discard ? 2 : CUP_EDIT_ACTION_COUNT;
		if (FullScreenMenu_Back(tapped)) { s_editPage = CUP_EDIT_LIST; RECTMENU_ClearInput(); MM_CustomCupEdit_DrawList(gGT); return false; }
		FullScreenMenu_NavTapped(tapped, scroll, row, count);
		if (!discard && *row == CUP_EDIT_LAPS_ACTION)
		{
			struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[0];
			int held = pad->buttonsHeldCurrFrame;
			int direction = (tapped & BTN_LEFT) != 0 ? -1 : (tapped & BTN_RIGHT) != 0 ? 1 : 0;
			if (direction == 0 && FullScreenMenu_OptionStepDue(held, -1)) { direction = -1; }
			else if (direction == 0 && FullScreenMenu_OptionStepDue(held, 1)) { direction = 1; }
			int laps = s_editCup.laps[s_editRow];
			if (direction != 0 && FullScreenMenu_OptionStep(&laps, direction, CUP_EDIT_MIN_LAPS, CUP_EDIT_MAX_LAPS, 1))
			{
				CustomCup_SetLaps(&s_editCup, s_editRow, laps);
				OtherFX_Play(0, 1);
			}
		}
		if (FullScreenMenu_Confirm(tapped))
		{
			RECTMENU_ClearInput();
			if (discard)
			{
				if (*row == 1) { CustomCup_Clear(&s_editCup); return true; }
				s_editPage = CUP_EDIT_LIST;
			}
			else if (*row == CUP_EDIT_REPLACE) { MM_CustomCupEdit_OpenPicker(false); }
			else if (*row == CUP_EDIT_LAPS_ACTION) { }
			else if (*row == CUP_EDIT_MOVE_ACTION) { s_editMoveOrigin = s_editRow; s_editPage = CUP_EDIT_MOVE; }
			else if (s_editCup.trackCount > CUSTOM_CUP_MIN_TRACKS && CustomCup_RemoveTrack(&s_editCup, s_editRow)) { s_editPage = CUP_EDIT_LIST; }
		}
		if (s_editPage != CUP_EDIT_TRACK_ACTIONS && s_editPage != CUP_EDIT_DISCARD) { return MM_CustomCupEdit_Update(0, gGT); }
		if (discard)
		{
			FullScreenMenu_DrawSectionScreen("DISCARD CHANGES?", discardLabels, scroll, count, *row, SCREEN_WIDTH / 2, gGT, ot);
		}
		else
		{
			int level;
			char title[160];
			const char *name = CustomCup_ResolveTrack(s_editCup.tracks[s_editRow], NativeTrackLibrary_Current(), &level)
			                       ? LevelInfo_DisplayTitle(level, sdata->lngStrings, title, sizeof(title)) : "TRACK UNAVAILABLE";
			char display[160];
			snprintf(display, sizeof(display), "%s", name);
			TrackText_Scroll(display, FULLSCREEN_MENU_CONTENT_W - 2 * FULLSCREEN_MENU_ROW_PADDING, FONT_BIG, gGT->timer);
			FullScreenMenu_DrawScreen(display, 0, scroll, count, *row, MM_CustomCupEdit_DrawActionRow, actionLabels, gGT, ot);
		}
		return false;
	}
	if (FullScreenMenu_Back(tapped))
	{
		RECTMENU_ClearInput();
		if (!MM_CustomCupEdit_Dirty()) { CustomCup_Clear(&s_editCup); return true; }
		s_editConfirmRow = 0;
		s_editConfirmScroll.offset = 0;
		s_editPage = CUP_EDIT_DISCARD;
		return MM_CustomCupEdit_Update(0, gGT);
	}
	FullScreenMenu_NavTapped(tapped, &s_editScroll, &s_editRow, s_editCup.trackCount + 2);
	if (FullScreenMenu_Confirm(tapped))
	{
		RECTMENU_ClearInput();
		s_editError = false;
		if (s_editRow < s_editCup.trackCount)
		{
			s_editActionRow = 0;
			s_editActionScroll.offset = 0;
			s_editPage = CUP_EDIT_TRACK_ACTIONS;
		}
		else if (s_editRow == s_editCup.trackCount && s_editCup.trackCount < CUSTOM_CUP_MAX_TRACKS) { MM_CustomCupEdit_OpenPicker(true); }
		else if (s_editRow == s_editCup.trackCount + 1 && s_editCup.trackCount >= CUSTOM_CUP_MIN_TRACKS)
		{
			char *id = s_cupSelectEntries[s_cupCarousel.selected - 1].id;
			if (NativeCustomCup_Save(id, &s_editCup) == NATIVE_CUSTOM_CUP_OK)
			{
				CustomCup_Clear(&s_editCup);
				MM_CustomCupSelect_Preview(s_cupCarousel.selected);
				return true;
			}
			s_editError = true;
		}
		if (s_editPage != CUP_EDIT_LIST) { return MM_CustomCupEdit_Update(0, gGT); }
	}
	MM_CustomCupEdit_DrawList(gGT);
	return false;
}
