#include <common.h>

enum TimeTrialEndMenuConstants
{
	TT_RACE_CLOCK_HOLD_FRAMES = CTR_SECONDS_TO_FRAMES(3),
	TT_RACE_CLOCK_FLYOUT_FRAME_OFFSET = 65,
	TT_RESULT_MAX_FRAMES = CTR_SECONDS_TO_FRAMES(30),
	TT_RESULT_MESSAGE_STEP_FRAMES = CTR_SECONDS_TO_FRAMES(1),
	TT_NEW_HIGH_SCORE_START_FRAME = TT_RACE_CLOCK_HOLD_FRAMES + TT_RESULT_MESSAGE_STEP_FRAMES,
	TT_NEW_BEST_LAP_START_FRAME = TT_NEW_HIGH_SCORE_START_FRAME + TT_RESULT_MESSAGE_STEP_FRAMES,
	TT_NTROPY_MESSAGE_START_FRAME = TT_NEW_BEST_LAP_START_FRAME + TT_RESULT_MESSAGE_STEP_FRAMES,
	TT_HIGH_SCORE_MENU_START_FRAME = TT_RESULT_MAX_FRAMES + 1,
	TT_HIGH_SCORE_EXIT_START_FRAME = 1001,
	TT_FINAL_MENU_START_FRAME = 1017,
	TT_HIGH_SCORE_EXIT_DONE_FRAME = 1018,
	TT_LERP_FRAMES = 0x14,
	TT_CLOCK_DISPLAY_FLAG = 1,
	TT_TOTAL_TIME_FLASH_FLAG = 1 << 2,
	TT_BEST_LAP_FLASH_FLAG_FIRST = 3,
	TT_CONFIRM_BUTTON_MASK = BTN_CROSS_one | BTN_CIRCLE,
	TT_MENU_READY_SHOW_MENU = 1,
	TT_MENU_READY_HIGH_SCORE_EXIT = 0x10,
	TT_HIGH_SCORE_VISIBLE_ROWS = 5,
	TT_HIGH_SCORE_FIRST_VISIBLE_ENTRY = 1,
	TT_HIGH_SCORE_ROW_SPACING = 0x1a,
	TT_HIGH_SCORE_DRIVER_COLOR_OFFSET = 5,
	TT_HIGH_SCORE_FLASH_TIMER_BIT = 2,
	TT_HIGH_SCORE_ICON_TRANSPARENCY = 1,
	TT_HIGH_SCORE_ICON_SCALE = 0x1000,
	TT_SCORE_MODE_TIME_TRIAL = 0,
};

// NOTE(aalhendi): Retail stores this writable one-character string before the
// overlay code.
global_variable s32 s_rankString224 CTR_PSX_MATCH_SECTION(".rodata") = 0x20;
global_variable Color s_highScoreIconColor224;

extern struct RectMenu menu224;
extern struct RectMenu menu224NoSave;

#define TT_FLAGS         (sdata->flags_timeTrialEndOfRace)
#define TT_GHOST_TOO_BIG (sdata->boolGhostTooBigToSave)

#define TT_DRAW_POLY_GT4(icon, posX, posY, primMem, ot, color0, color1, color2, color3, transparency, scale) \
	RECTMENU_DrawPolyGT4((icon), (s16)(posX), (s32)(posY), (primMem), (ot), (color0), (color1), (color2), (color3), (s8)(transparency), (s16)(scale))
#define TT_DRAW_CLEAR_BOX CTR_Box_DrawClearBox

// NOTE(aalhendi): Native uses the shared s16 declaration; retail preserves
// overlay 224's wider caller-side x argument through its private ABI binding.
#define TT_DRAW_LINE_WIDE_X DecalFont_DrawLine

void TT_EndEvent_DisplayTime(s32 paramX, s16 paramY, u32 raceClockFlags)
{
	SVec2 pos;
	RECT rectangle;
	s32 startX;
	s32 endX;
	s16 textWidth;

	register struct Driver *d CTR_PSX_REGISTER("$22");

	d = GAME_TRACKER->drivers[0];

	textWidth = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[LNG_TOTAL], FONT_BIG);
	startX = (s16)(paramX - (0x88 - textWidth) / 2);
	textWidth = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[LNG_TOTAL], FONT_BIG);
	endX = (s16)(paramX - (0x88 - textWidth) / 2);

	// === Naughty Dog Bug ===
	// Start and End is the same
	UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), startX, paramY, endX, paramY, GAME_FRAMES_SINCE_RACE_ENDED, TT_LERP_FRAMES);

	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_YOUR_TIME], paramX, ((u32)pos.y - 0x4c), FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	UI_DrawRaceClock(pos.x, pos.y, raceClockFlags, d);

	rectangle.x = (pos.x - DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[LNG_TOTAL], FONT_BIG)) - 6;
	rectangle.y = pos.y - 0x50;

	rectangle.w = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[LNG_TOTAL], FONT_BIG) + 0x94;
	rectangle.h = 99;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&rectangle, 4, GAME_TRACKER->backBuffer->otMem.uiOT);

	return;
}

void TT_EndEvent_DrawHighScore(s16 startX, s32 startY, s16 scoreMode)
{
	// This is different from High Score in Main Menu because Main Menu
	// does not show the rank icons '1', '2', '3', '4', '5'
	char *timeString;
	s16 timeColor;
	u32 timeColorSource;
	SVec2 pos;
	RECT box;
	s16 startXCopy;
	char flashMask;
	s16 scoreModeCopy;
	s16 startYCopy;
	s16 timeboxXSource;
	struct Driver *driver;
	s32 rowIndex;
	struct HighScoreEntry *scoreEntries;
	u16 iconX;
	s32 iconYBase;
	struct HighScoreEntry *scoreEntry;
	register s32 timeboxYBase CTR_PSX_REGISTER("$22");
	s32 timeboxX;
	u32 rowOffsetY;
	s32 rowOffsetYCopy;
	s32 scoreEntryOffset;
	char *rankString;
	s32 currentY;
	s32 highlightY;
	s16 nameColor;

	{
		register s32 iconYBaseSource CTR_PSX_REGISTER("$17");

		iconX = startX - 0x52;
		iconYBaseSource = startY + 0x11;
		timeboxYBase = iconYBaseSource;
		startYCopy = (s16)startY;
		rowOffsetY = 0;
		driver = GAME_TRACKER->drivers[0];
		timeboxXSource = (s16)(startX - 0x1f);
		// NOTE(aalhendi): Removing this otherwise dead retail temporary changes
		// GCC 2.8.1's register allocation.
		startXCopy = startX;
		(void)startXCopy;
		scoreModeCopy = scoreMode;

		// 12 entries per track, 6 for Time Trial and 6 for Relic Race
		struct HighScoreTrack *customTrack = GAMEPROG_CustomHighScoreTrack(GAME_TRACKER->levelID);
		scoreEntries = customTrack != NULL ? customTrack->scoreEntry[scoreModeCopy] :
		    GAME_PROGRESS.highScoreTracks[GAME_TRACKER->levelID].scoreEntry[scoreModeCopy];

		// NOTE(aalhendi): Retail passes identical start and end points.
		UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), startX, startYCopy, startX, startYCopy, GAME_FRAMES_SINCE_RACE_ENDED, TT_LERP_FRAMES);

		rowIndex = 0;
		scoreEntryOffset = sizeof(struct HighScoreEntry) * TT_HIGH_SCORE_FIRST_VISIBLE_ENTRY;
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_BEST_TIMES], pos.x, pos.y, FONT_BIG, JUSTIFY_CENTER | ORANGE);

		iconYBase = (s16)iconYBaseSource;
		timeboxX = timeboxXSource;
	}
	// NOTE(aalhendi): This lifetime boundary preserves retail's spill order.
	rankString = (char *)&s_rankString224;

	// Draw the icon, name, and time for the five best scores in the selected mode.
	for (; rowIndex < TT_HIGH_SCORE_VISIBLE_ROWS; scoreEntryOffset += sizeof(struct HighScoreEntry), rowIndex++, rowOffsetY += TT_HIGH_SCORE_ROW_SPACING)
	{
		scoreEntry = (struct HighScoreEntry *)((u8 *)scoreEntries + scoreEntryOffset);
		rowOffsetYCopy = (u16)rowOffsetY;
		flashMask = TT_HIGH_SCORE_FLASH_TIMER_BIT << 1;

		// If this loop index is a new high score
		if ((s8)GAME_TRACKER->newHighScoreIndex == rowIndex)
		{
			// make name color flash every odd frame
			nameColor = (GAME_TRACKER->timer & TT_HIGH_SCORE_FLASH_TIMER_BIT) ? WHITE : scoreEntry->characterID + TT_HIGH_SCORE_DRIVER_COLOR_OFFSET;

			// flash color of time
			timeColorSource = (GAME_TRACKER->timer << 1) & flashMask;
			timeColor = timeColorSource;
		}
		else
		{
			timeColor = 0;
			nameColor = scoreEntry->characterID + TT_HIGH_SCORE_DRIVER_COLOR_OFFSET;
		}

		// Make a rank on the high score list ('1', '2', '3', '4', '5')
		// by taking the binary value of the rank (0, 1, 2, 3, 4),
		// and adding the ascii value of '1'
		currentY = timeboxYBase + rowOffsetYCopy;
		*rankString = (char)rowIndex + '1';

		// Draw String for Rank ('1', '2', '3', '4', '5')
		DecalFont_DrawLine(rankString, iconX + 0x20, currentY - 1, FONT_SMALL, WHITE);

		// Draw Character Icon
		TT_DRAW_POLY_GT4(GAME_TRACKER->ptrIcons[GAME_CHARACTER_METADATA[(s16)scoreEntry->characterID].iconID], iconX, iconYBase + (s16)rowOffsetYCopy,
		                 &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT,
		                 // color of each corner
		                 s_highScoreIconColor224, s_highScoreIconColor224, s_highScoreIconColor224, s_highScoreIconColor224, TT_HIGH_SCORE_ICON_TRANSPARENCY,
		                 TT_HIGH_SCORE_ICON_SCALE);

		// Draw Name
		TT_DRAW_LINE_WIDE_X(scoreEntry->name, timeboxX, currentY, FONT_CREDITS, nameColor);

		// Draw time
		TT_DRAW_LINE_WIDE_X(RECTMENU_DrawTime(scoreEntry->time), timeboxX, currentY + 0x11, FONT_SMALL, timeColor);

		// If this loop index is a new high score
		if ((s8)GAME_TRACKER->newHighScoreIndex == rowIndex)
		{
			box.x = iconX - 4;
			highlightY = rowOffsetYCopy - 1;
			box.y = timeboxYBase + highlightY;
			box.w = 0xab;
			box.h = 0x1a;

			// Draw a rectangle to highlight your time on the "Best Times" list
			TT_DRAW_CLEAR_BOX(&box, &GAME_MENU_HIGHLIGHT, TRANS_50_DECAL, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
		}
	}

	if (scoreModeCopy == TT_SCORE_MODE_TIME_TRIAL)
	{
		// Change the way text flickers
		timeColor = JUSTIFY_CENTER | ORANGE;

		// If you got a new best lap
		if (((GAME_TRACKER->gameModeEnd & NEW_BEST_LAP) != 0) && ((GAME_TRACKER->timer & TT_HIGH_SCORE_FLASH_TIMER_BIT) != 0))
		{
			timeColor = JUSTIFY_CENTER | WHITE;
		}

		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_BEST_LAP], startX, timeboxYBase + 0x84, FONT_BIG, JUSTIFY_CENTER | ORANGE);

		// make a string for best lap
		timeString = RECTMENU_DrawTime(scoreEntries[0].time);
		DecalFont_DrawLine(timeString, startX, timeboxYBase + 0x95, FONT_SMALL, timeColor);
	}
	else
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_YOUR_TIME], startX, timeboxYBase + 0x84, FONT_BIG, JUSTIFY_CENTER | ORANGE);

		// make a string for your current track time
		timeString = RECTMENU_DrawTime(driver->timeElapsedInRace);

		// color
		timeColor = JUSTIFY_CENTER | ORANGE;
		DecalFont_DrawLine(timeString, startX, timeboxYBase + 0x95, FONT_SMALL, timeColor);
	}

	box.x = pos.x - 0x60;
	box.y = pos.y - 4;
	box.w = 0xc0;
	box.h = 0xb4;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&box, 4, GAME_TRACKER->backBuffer->otMem.uiOT);
}


void TT_EndEvent_DrawMenu(void)
{
	u32 gameModeEnd;
	s32 elapsedFrames;
	struct GameTracker *gGT;
	struct GameProgress *gameProgress;
	struct HighScoreTrack *highScoreTracks;
	SVec2 pos;

	TT_FLAGS |= TT_CLOCK_DISPLAY_FLAG;
	gGT = GAME_TRACKER;
	highScoreTracks = GAME_PROGRESS.highScoreTracks;

	// If you just beat N Tropy && N Tropy was beaten on all tracks
	if (((gGT->gameModeEnd & NTROPY_JUST_BEAT) != 0) && (s16)GAMEPROG_CheckGhostsBeaten(1))
	{
		gameProgress = (struct GameProgress *)((u8 *)highScoreTracks - OFFSETOF(struct GameProgress, highScoreTracks));
		gameProgress->unlocks[0] |= UNLOCK_TROPY;
	}

	if (GAME_FRAMES_SINCE_RACE_ENDED < TT_RESULT_MAX_FRAMES)
	{
		GAME_FRAMES_SINCE_RACE_ENDED++;
	}
	else if ((GAME_TRACKER->gameModeEnd & NEW_HIGH_SCORE) == 0)
	{
		if ((GAME_FRAMES_SINCE_RACE_ENDED < TT_HIGH_SCORE_EXIT_DONE_FRAME) && ((GAME_MENU_READY & TT_MENU_READY_HIGH_SCORE_EXIT) != 0))
		{
			GAME_FRAMES_SINCE_RACE_ENDED++;
		}
		else if (GAME_FRAMES_SINCE_RACE_ENDED < TT_HIGH_SCORE_EXIT_START_FRAME)
		{
			GAME_FRAMES_SINCE_RACE_ENDED++;
		}
	}

	elapsedFrames = GAME_FRAMES_SINCE_RACE_ENDED;

	if (elapsedFrames > TT_RESULT_MAX_FRAMES)
	{
		gameModeEnd = GAME_TRACKER->gameModeEnd | DRAW_HIGH_SCORES;
		GAME_TRACKER->gameModeEnd = gameModeEnd;
		goto draw_high_scores;
	}

	if (elapsedFrames < TT_RACE_CLOCK_HOLD_FRAMES + 1)
	{
		goto draw_race_clock;
	}

	elapsedFrames = GAME_FRAMES_SINCE_RACE_ENDED;
	if (elapsedFrames >= TT_NEW_HIGH_SCORE_START_FRAME + 1)
	{
		UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x264, 122, 0x100, 122, elapsedFrames - TT_NEW_HIGH_SCORE_START_FRAME, TT_LERP_FRAMES);

		if ((s8)GAME_TRACKER->newHighScoreIndex >= 0)
		{
			char **bannerLanguageStrings;

			// NOTE(aalhendi): This local preserves retail's branch-delay
			// scheduling around the banner guard.
			bannerLanguageStrings = GAME_LANGUAGE_STRINGS;
			DecalFont_DrawLine(bannerLanguageStrings[LNG_NEW_HIGH_SCORE], pos.x, pos.y, FONT_BIG,
			                   (GAME_TRACKER->timer & 1) ? (JUSTIFY_CENTER | ORANGE) : (JUSTIFY_CENTER | WHITE));
			TT_FLAGS |= TT_TOTAL_TIME_FLASH_FLAG;
		}
	}

	{
		s32 messageFrames;

		// NOTE(aalhendi): Retail carries each banner's frame snapshot through
		// its lerp setup; keeping the lifetimes local reproduces that schedule.
		messageFrames = GAME_FRAMES_SINCE_RACE_ENDED;
		if (messageFrames >= TT_NEW_BEST_LAP_START_FRAME + 1)
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), -0x64, 142, 0x100, 142, messageFrames - TT_NEW_BEST_LAP_START_FRAME, TT_LERP_FRAMES);

			if ((GAME_TRACKER->gameModeEnd & NEW_BEST_LAP) != 0)
			{
				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_NEW_BEST_LAP], pos.x, pos.y, FONT_BIG,
				                   (GAME_TRACKER->timer & 1) ? (JUSTIFY_CENTER | ORANGE) : (JUSTIFY_CENTER | WHITE));

				if (GAME_TRACKER->lapIndexNewBest == 0)
				{
					TT_FLAGS |= 1 << TT_BEST_LAP_FLASH_FLAG_FIRST;
				}
				else if (GAME_TRACKER->lapIndexNewBest == 1)
				{
					TT_FLAGS |= 1 << (TT_BEST_LAP_FLASH_FLAG_FIRST + 1);
				}

				if (GAME_TRACKER->lapIndexNewBest == 2)
				{
					TT_FLAGS |= 1 << (TT_BEST_LAP_FLASH_FLAG_FIRST + 2);
				}
			}
		}
	}

	{
		s32 messageFrames;

		messageFrames = GAME_FRAMES_SINCE_RACE_ENDED;
		if (messageFrames >= TT_NTROPY_MESSAGE_START_FRAME + 1)
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x264, 162, 0x100, 162, messageFrames - TT_NTROPY_MESSAGE_START_FRAME, TT_LERP_FRAMES);

			if ((GAME_TRACKER->gameModeEnd & NTROPY_JUST_OPENED) != 0)
			{
				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_N_TROPY_OPENED], pos.x, pos.y, FONT_BIG,
				                   (GAME_TRACKER->timer & 1) ? (JUSTIFY_CENTER | ORANGE) : (JUSTIFY_CENTER | WHITE));
			}
			else
			{
				if ((GAME_TRACKER->gameModeEnd & NTROPY_JUST_BEAT) == 0)
				{
					goto draw_result_time;
				}

				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_N_TROPY_BEATEN], pos.x, pos.y, FONT_BIG,
				                   (GAME_TRACKER->timer & 1) ? (JUSTIFY_CENTER | ORANGE) : (JUSTIFY_CENTER | WHITE));
			}
		}
	}

draw_result_time:
	UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), -0x64, 90, 0x100, 90, GAME_FRAMES_SINCE_RACE_ENDED - TT_RACE_CLOCK_HOLD_FRAMES, TT_LERP_FRAMES);
	TT_EndEvent_DisplayTime(pos.x, pos.y, TT_FLAGS);
	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_PRESS_TO_CONTINUE], 0x100, 0xbe, FONT_BIG, JUSTIFY_CENTER | ORANGE);

	if ((GAME_ANY_PLAYER_TAP & TT_CONFIRM_BUTTON_MASK) != 0)
	{
		GAME_FRAMES_SINCE_RACE_ENDED = TT_HIGH_SCORE_MENU_START_FRAME;
	}

	goto show_menu;

draw_race_clock:
	if (elapsedFrames >= TT_RACE_CLOCK_FLYOUT_FRAME_OFFSET + 1)
	{
		UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x14, 8, -0x96, 8, elapsedFrames - TT_RACE_CLOCK_FLYOUT_FRAME_OFFSET, TT_LERP_FRAMES);
	}
	else
	{
		UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x14, 8, 0x14, 8, elapsedFrames, TT_LERP_FRAMES);
	}

	UI_DrawRaceClock(pos.x, pos.y, UI_RACE_CLOCK_SHOW_CURRENT_TIME, GAME_TRACKER->drivers[0]);
	goto show_menu;

draw_high_scores:
	if ((elapsedFrames < TT_FINAL_MENU_START_FRAME) && ((gameModeEnd & NEW_HIGH_SCORE) == 0))
	{
		if (elapsedFrames >= TT_HIGH_SCORE_EXIT_START_FRAME + 1)
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x80, 10, -0x96, 10, elapsedFrames - TT_HIGH_SCORE_EXIT_START_FRAME, TT_LERP_FRAMES);
		}
		else
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), -0x96, 10, 0x80, 10, elapsedFrames - TT_HIGH_SCORE_MENU_START_FRAME, TT_LERP_FRAMES);
		}

		TT_EndEvent_DrawHighScore(pos.x, pos.y, TT_SCORE_MODE_TIME_TRIAL);

		{
			s32 highScoreElapsedFrames;

			highScoreElapsedFrames = GAME_FRAMES_SINCE_RACE_ENDED;
			if (highScoreElapsedFrames >= TT_HIGH_SCORE_EXIT_START_FRAME + 1)
			{
				UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x180, 0x82, 0x296, 0x82, highScoreElapsedFrames - TT_HIGH_SCORE_EXIT_START_FRAME, TT_LERP_FRAMES);
			}
			else
			{
				UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x296, 0x82, 0x180, 0x82, highScoreElapsedFrames - TT_HIGH_SCORE_MENU_START_FRAME, TT_LERP_FRAMES);
			}
		}

		TT_EndEvent_DisplayTime(pos.x, pos.y, TT_FLAGS);
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_PRESS_TO_CONTINUE], 0x100, 0xbe, FONT_BIG, JUSTIFY_CENTER | ORANGE);

		if (((GAME_ANY_PLAYER_TAP & TT_CONFIRM_BUTTON_MASK) != 0) && (GAME_FRAMES_SINCE_RACE_ENDED < TT_HIGH_SCORE_EXIT_START_FRAME + 1))
		{
			GAME_FRAMES_SINCE_RACE_ENDED = TT_HIGH_SCORE_EXIT_START_FRAME;
			GAME_MENU_READY |= TT_MENU_READY_HIGH_SCORE_EXIT;
		}
	}

show_menu:
	if (((GAME_MENU_READY & TT_MENU_READY_SHOW_MENU) == 0) && (GAME_FRAMES_SINCE_RACE_ENDED >= TT_FINAL_MENU_START_FRAME))
	{
		GAME_MENU_READY = (GAME_MENU_READY | TT_MENU_READY_SHOW_MENU) & ~TT_MENU_READY_HIGH_SCORE_EXIT;
		TT_FLAGS = 0;

		if (TT_GHOST_TOO_BIG == 0)
		{
			RECTMENU_Show(&menu224);
		}
		else
		{
			RECTMENU_Show(&menu224NoSave);
		}
	}
}

struct MenuRow rowsWithSave[6] = {
    // Retry
    {
        .stringIndex = LNG_RETRY,
        .rowOnPressUp = 0,
        .rowOnPressDown = 1,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    },

    // Change Level
    {
        .stringIndex = LNG_CHANGE_LEVEL,
        .rowOnPressUp = 0,
        .rowOnPressDown = 2,
        .rowOnPressLeft = 1,
        .rowOnPressRight = 1,
    },

    // Change Character
    {
        .stringIndex = LNG_CHANGE_CHARACTER,
        .rowOnPressUp = 1,
        .rowOnPressDown = 3,
        .rowOnPressLeft = 2,
        .rowOnPressRight = 2,
    },

    // Save Ghost
    {
        .stringIndex = LNG_SAVE_GHOST,
        .rowOnPressUp = 2,
        .rowOnPressDown = 4,
        .rowOnPressLeft = 3,
        .rowOnPressRight = 3,
    },

    // Quit
    {
        .stringIndex = LNG_QUIT,
        .rowOnPressUp = 3,
        .rowOnPressDown = 4,
        .rowOnPressLeft = 4,
        .rowOnPressRight = 4,
    },

    // NULL, end of menu
    {
        .stringIndex = RECTMENU_STRING_NONE,
        .rowOnPressUp = 0,
        .rowOnPressDown = 0,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    }};

struct RectMenu menu224 = {
    .stringIndexTitle = RECTMENU_STRING_NONE,
    .posX_curr = 0x100,
    .posY_curr = 0xA0,

    .unk1 = 0,

    .state = RECTMENU_STATE_SMALL_EXEC_CENTERED,
    .rows = rowsWithSave,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,

    // rest of variables all default zero
};

struct MenuRow rowsNoSave[5] = {
    // Retry
    {
        .stringIndex = LNG_RETRY,
        .rowOnPressUp = 0,
        .rowOnPressDown = 1,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    },

    // Change Level
    {
        .stringIndex = LNG_CHANGE_LEVEL,
        .rowOnPressUp = 0,
        .rowOnPressDown = 2,
        .rowOnPressLeft = 1,
        .rowOnPressRight = 1,
    },

    // Change Character
    {
        .stringIndex = LNG_CHANGE_CHARACTER,
        .rowOnPressUp = 1,
        .rowOnPressDown = 3,
        .rowOnPressLeft = 2,
        .rowOnPressRight = 2,
    },

    // Quit
    {
        .stringIndex = LNG_QUIT,
        .rowOnPressUp = 2,
        .rowOnPressDown = 3,
        .rowOnPressLeft = 3,
        .rowOnPressRight = 3,
    },

    // NULL, end of menu
    {
        .stringIndex = RECTMENU_STRING_NONE,
        .rowOnPressUp = 0,
        .rowOnPressDown = 0,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    }};

struct RectMenu menu224NoSave = {
    .stringIndexTitle = RECTMENU_STRING_NONE,
    .posX_curr = 0x100,
    .posY_curr = 0xA0,

    .unk1 = 0,

    .state = RECTMENU_STATE_SMALL_EXEC_CENTERED,
    .rows = rowsNoSave,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,

    // rest of variables all default zero
};

global_variable Color s_highScoreIconColor224 = COLOR_CODE_PACKED_INIT(0x808080);
