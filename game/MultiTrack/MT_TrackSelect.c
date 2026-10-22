// MultiTrack: scrolling list of stock + custom tracks, replacing the retail
// track carousel (MM_TrackSelect_MenuProc_Retail). Port of
// mod/src/track_select.c.

#define MT_ROWS_VISIBLE 10
#define MT_ROW_HEIGHT   0xE
#define MT_LIST_X       0x24
#define MT_LIST_Y       0x26
#define MT_LIST_W       0x1B8

// Rows are the unlocked stock tracks of this mode, then every custom track
// whose host slot belongs to this mode. Returns the number of rows.
static int MT_RowInfo(struct MainMenu_LevelRow *stock, int numStock, int battle, int row, int *levID, struct MTEntry **custom)
{
	int n = 0;
	*custom = NULL;

	for (int i = 0; i < numStock; i++)
	{
		if (!MM_TrackSelect_boolTrackOpen(&stock[i]))
			continue;
		if (n == row)
			*levID = stock[i].levID;
		n++;
	}

	struct MTHeader *table = MT_Table();
	for (int i = 0; i < table->count; i++)
	{
		struct MTEntry *e = &table->entries[i];
		if (((e->flags & MT_FLAG_BATTLE) != 0) != battle)
			continue;
		if (n == row)
		{
			*levID = e->slot;
			*custom = e;
		}
		n++;
	}
	return n;
}

// arcade fills the grid with bots, which need the lev's nav paths
static int MT_NoBots(struct GameTracker *gGT, struct MTEntry *e)
{
	return e && (e->flags & MT_FLAG_NO_AI) && (gGT->gameMode1 & ARCADE_MODE);
}

void MM_TrackSelect_MenuProc(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;
	s16 elapsedFrames = MM_TRACK_TRANSITION_FRAME;
	int battle = (gGT->gameMode1 & BATTLE_MODE) != 0;
	(void)menu;

	if (MT_STATE->magic != MT_MAGIC)
	{
		MT_STATE->magic = MT_MAGIC;
		MT_STATE->redirectLevel = -1;
		MT_STATE->cursor[0] = 0;
		MT_STATE->cursor[1] = 0;
	}

	if (MM_TRACK_TRANSITION_STATE == ENTERING_MENU)
	{
		MM_ERROR_MESSAGE_POS_INDEX = battle ? 2 : 1;
		MM_TransitionInOut(MM_TRACK_TRANSITIONS, elapsedFrames, MM_TRACK_SELECT_SLIDE_FRAMES);
		if (elapsedFrames == 0)
			MM_TRACK_TRANSITION_STATE = IN_MENU;
		else
			elapsedFrames--;
	}
	else if (MM_TRACK_TRANSITION_STATE == EXITING_MENU)
	{
		MM_TransitionInOut(MM_TRACK_TRANSITIONS, elapsedFrames, MM_TRACK_SELECT_SLIDE_FRAMES);
		elapsedFrames++;

		if (elapsedFrames > MM_TRACK_SELECT_TRANSITION_FRAMES)
		{
			MM_TRACK_TRANSITION_FRAME = elapsedFrames;
			MM_ERROR_MESSAGE_POS_INDEX = 0;

			if (MM_TRACK_TRANSITION_START_AFTER_EXIT == 0)
			{
				MT_STATE->redirectLevel = -1;
				MM_DESIRED_MENU = &MM_MENU_CHARACTER_SELECT;
				MM_Characters_RestoreIDs();
				return;
			}

			if (battle)
			{
				MM_DESIRED_MENU = &MM_MENU_BATTLE_WEAPONS;
				MM_Battle_Init();
				return;
			}

			if ((gGT->gameMode1 & TIME_TRIAL) != 0)
			{
				MM_GHOST_TAPE_PLAYING = MM_ALLOC_HIGH_MEM(MM_TRACK_SELECT_GHOST_TAPE_ALLOC_SIZE, MM_LOADED_GHOST_DATA);
				memset(MM_GHOST_TAPE_PLAYING, 0, MM_TRACK_SELECT_GHOST_TAPE_CLEAR_SIZE);
				MM_REPLAY_HUMAN_GHOST = 0;
				SelectProfile_ToggleMode(SELECT_PROFILE_SCREEN_GHOST);
				MM_DESIRED_MENU = &MM_MENU_GHOST_SELECTION;
				return;
			}

			MM_DESIRED_MENU = &MM_MENU_QUEUE_LOAD_TRACK;
			return;
		}
	}
	MM_TRACK_TRANSITION_FRAME = elapsedFrames;

	struct MainMenu_LevelRow *stock = battle ? &MM_BATTLE_TRACKS[0] : &MM_ARCADE_TRACKS[0];
	int numStock = battle ? 7 : 18;

	int levID = 0;
	struct MTEntry *custom;
	int numRows = MT_RowInfo(stock, numStock, battle, -1, &levID, &custom);
	int cursor = MT_STATE->cursor[battle];
	if (cursor >= numRows)
		cursor = 0;

	struct MTEntry *cursorEntry;
	MT_RowInfo(stock, numStock, battle, cursor, &levID, &cursorEntry);

	if (MM_TRACK_LAP_BOX_OPEN == 0)
	{
		int tap = MM_GAME_BUTTON_TAPS[0] & (BTN_UP | BTN_DOWN | BTN_L1 | BTN_R1 | BTN_TRIANGLE | BTN_SQUARE_one | BTN_CROSS_one | BTN_CIRCLE);

		if ((MM_TRACK_TRANSITION_STATE == IN_MENU) && (tap != 0))
		{
			if (tap & BTN_UP)
				cursor = (cursor == 0) ? numRows - 1 : cursor - 1;
			else if (tap & BTN_DOWN)
				cursor = (cursor + 1 >= numRows) ? 0 : cursor + 1;
			else if (tap & BTN_L1)
				cursor = (cursor < MT_ROWS_VISIBLE) ? 0 : cursor - MT_ROWS_VISIBLE;
			else if (tap & BTN_R1)
				cursor = (cursor + MT_ROWS_VISIBLE >= numRows) ? numRows - 1 : cursor + MT_ROWS_VISIBLE;

			if (tap & (BTN_UP | BTN_DOWN | BTN_L1 | BTN_R1))
				OtherFX_Play(0, 1);

			if ((tap & (BTN_CROSS_one | BTN_CIRCLE)) && MT_NoBots(gGT, cursorEntry))
			{
				OtherFX_Play(2, 1);
			}
			else if (tap & (BTN_CROSS_one | BTN_CIRCLE))
			{
				OtherFX_Play(1, 1);
				if ((gGT->gameMode1 & (BATTLE_MODE | TIME_TRIAL)) == 0)
				{
					MM_TRACK_LAP_BOX_OPEN = MM_TRACK_TRANSITION_STATE;
				}
				else
				{
					MM_TRACK_TRANSITION_START_AFTER_EXIT = MM_TRACK_TRANSITION_STATE;
					MM_TRACK_TRANSITION_STATE = EXITING_MENU;
				}
			}
			else if (tap & (BTN_TRIANGLE | BTN_SQUARE_one))
			{
				OtherFX_Play(2, 1);
				MM_TRACK_TRANSITION_START_AFTER_EXIT = 0;
				MM_TRACK_TRANSITION_STATE = EXITING_MENU;
			}

			RECTMENU_ClearInput();
		}
	}
	else
	{
		s32 lapState = 0;
		MM_MENU_LAP_SELECT.rowSelected = MM_USELESS_LAP_ROW_COPY;

		if (MM_TRACK_TRANSITION_STATE == IN_MENU)
			lapState = RECTMENU_ProcessInput(&MM_MENU_LAP_SELECT);

		RECTMENU_DrawSelf(&MM_MENU_LAP_SELECT, MM_TRACK_TRANSITIONS[2].currX, MM_TRACK_TRANSITIONS[2].currY, MM_TRACK_SELECT_LAP_MENU_WIDTH);

		MM_USELESS_LAP_ROW_COPY = MM_MENU_LAP_SELECT.rowSelected;
		gGT->numLaps = MM_LAP_COUNT_BY_ROW[MM_MENU_LAP_SELECT.rowSelected].lapCount;

		if (lapState == 1)
		{
			MM_TRACK_TRANSITION_STATE = EXITING_MENU;
			MM_TRACK_TRANSITION_START_AFTER_EXIT = 1;
		}
		else if (lapState == -1)
		{
			MM_TRACK_LAP_BOX_OPEN = 0;
		}

		if ((gGT->gameMode2 & CHEAT_ONELAP) != 0)
			gGT->numLaps = 1;
	}

	MT_STATE->cursor[battle] = cursor;

	// highlighted row decides what QueueLoadTrack will load
	MT_RowInfo(stock, numStock, battle, cursor, &levID, &custom);
	gGT->currLEV = levID;
	MT_STATE->redirectBase = custom ? custom->bigBase : 0;
	MT_STATE->redirectLevel = custom ? custom->slot : -1;

	// no track preview video: custom tracks have none, and it would show the host track's
	MM_TrackSelect_Video_State(1);

	int x = MM_TRACK_TRANSITIONS[0].currX;
	int y = MM_TRACK_TRANSITIONS[0].currY;
	int top = cursor - MT_ROWS_VISIBLE / 2;
	if (top > numRows - MT_ROWS_VISIBLE)
		top = numRows - MT_ROWS_VISIBLE;
	if (top < 0)
		top = 0;

	char header[32];
	sprintf(header, "%d/%d  L1/R1 PAGE", cursor + 1, numRows);
	DecalFont_DrawLine(header, x + MT_LIST_X + MT_LIST_W - 8, y + MT_LIST_Y - 0xE, FONT_SMALL, (JUSTIFY_RIGHT | PERIWINKLE));
	DecalFont_DrawLine(battle ? "SELECT ARENA" : "SELECT TRACK", x + MT_LIST_X + 8, y + MT_LIST_Y - 0xE, FONT_SMALL, ORANGE);

	for (int i = 0; (i < MT_ROWS_VISIBLE) && (top + i < numRows); i++)
	{
		int row = top + i;
		int rowLev;
		struct MTEntry *e;
		MT_RowInfo(stock, numStock, battle, row, &rowLev, &e);

		char *name = e ? e->name : sdata->lngStrings[data.metaDataLEV[rowLev].name_LNG];
		int color = (row == cursor) ? ORANGE : (e ? WHITE : PERIWINKLE);
		DecalFont_DrawLine(name, x + MT_LIST_X + 8, y + MT_LIST_Y + 4 + i * MT_ROW_HEIGHT, FONT_SMALL, color);

		if (row == cursor)
		{
			RECT q = {x + MT_LIST_X + 2, y + MT_LIST_Y + 1 + i * MT_ROW_HEIGHT, MT_LIST_W - 4, MT_ROW_HEIGHT - 1};
			CTR_Box_DrawClearBox(&q, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, gGT->backBuffer->otMem.uiOT,
			                     &gGT->backBuffer->primMem);
		}
	}

	if (MT_NoBots(gGT, custom))
	{
		DecalFont_DrawLine("NO AI PATHS - USE TIME TRIAL OR VS", x + MT_LIST_X + MT_LIST_W / 2, y + MT_LIST_Y + MT_ROWS_VISIBLE * MT_ROW_HEIGHT + 0x16,
		                   FONT_SMALL, (JUSTIFY_CENTER | RED));
	}

	if (custom)
	{
		char info[64];
		sprintf(info, "BY %s  -  ON %s", custom->author, sdata->lngStrings[data.metaDataLEV[custom->slot].name_LNG]);
		DecalFont_DrawLine(info, x + MT_LIST_X + MT_LIST_W / 2, y + MT_LIST_Y + MT_ROWS_VISIBLE * MT_ROW_HEIGHT + 0xA, FONT_SMALL,
		                   (JUSTIFY_CENTER | WHITE));
	}

	RECT r = {x + MT_LIST_X, y + MT_LIST_Y, MT_LIST_W, MT_ROWS_VISIBLE * MT_ROW_HEIGHT + 4};
	RECTMENU_DrawInnerRect(&r, 0, gGT->backBuffer->otMem.uiOT);
}
