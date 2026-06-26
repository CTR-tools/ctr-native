#include <common.h>

enum TimeTrialEndMenuConstants
{
	TT_RACE_CLOCK_HOLD_FRAMES = CTR_SECONDS_TO_FRAMES(3),
	TT_RACE_CLOCK_FLYOUT_FRAME_OFFSET = 65,
	TT_RESULT_MAX_FRAMES = CTR_SECONDS_TO_FRAMES(30),
	TT_RESULT_MESSAGE_STEP_FRAMES = CTR_SECONDS_TO_FRAMES(1),
	TT_HIGH_SCORE_MENU_START_FRAME = TT_RESULT_MAX_FRAMES + 1,
	TT_HIGH_SCORE_HOLD_END_FRAME = 1000,
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
	TT_HIGH_SCORE_ENTRIES_PER_MODE = 6,
	TT_HIGH_SCORE_VISIBLE_ROWS = 5,
	TT_HIGH_SCORE_FIRST_VISIBLE_ENTRY = 1,
	TT_HIGH_SCORE_ROW_SPACING = 0x1a,
	TT_HIGH_SCORE_DRIVER_COLOR_OFFSET = 5,
	TT_HIGH_SCORE_FLASH_TIMER_BIT = 2,
	TT_HIGH_SCORE_ICON_COLOR = 0x808080,
	TT_HIGH_SCORE_ICON_TRANSPARENCY = 1,
	TT_HIGH_SCORE_ICON_SCALE = 0x1000,
	TT_SCORE_MODE_TIME_TRIAL = 0,
};

global_variable s32 s_rankString224 = 0x20; // " \0"

extern struct RectMenu menu224;
extern struct RectMenu menu224NoSave;

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009fdc8-0x800a04d4.
void TT_EndEvent_DrawMenu(void)
{
	s32 elapsedFrames;
	s32 startX;
	s16 endX;
	SVec2 pos;
	struct GameTracker *gGT = sdata->gGT;
	char **lngStrings = sdata->lngStrings;
	u32 gameModeEnd = gGT->gameModeEnd;

	sdata->flags_timeTrialEndOfRace |= TT_CLOCK_DISPLAY_FLAG;

	// If you just beat N Tropy && N Tropy was beaten on all tracks
	if (((gameModeEnd & NTROPY_JUST_BEAT) != 0) && ((GAMEPROG_CheckGhostsBeaten(1) & 0xffff) != 0))
	{
		sdata->gameProgress.unlockFlags |= UNLOCK_TROPY;
	}

	// copy the frame counter variable
	elapsedFrames = sdata->framesSinceRaceEnded;

	// Conditions to increment frame
	if ((elapsedFrames < TT_RESULT_MAX_FRAMES) ||

	    (((gameModeEnd & NEW_HIGH_SCORE) == 0) &&

	     (
	         // Transition the high scores on-screen
	         (elapsedFrames <= TT_HIGH_SCORE_HOLD_END_FRAME) ||

	         (
	             // Wait until press X, then transition off-screen
	             ((sdata->menuReadyToPass & TT_MENU_READY_HIGH_SCORE_EXIT) != 0) && (elapsedFrames < TT_HIGH_SCORE_EXIT_DONE_FRAME)))))
	{
		elapsedFrames++;
	}

	// copy "back" the frame counter variable
	sdata->framesSinceRaceEnded = elapsedFrames;

	// First 90 frames (0-3 seconds)
	// Return at bottom of IF block
	if (elapsedFrames <= TT_RACE_CLOCK_HOLD_FRAMES)
	{
		// no lerp, just sit on-screen
		endX = 0x14;

		if (elapsedFrames > TT_RACE_CLOCK_FLYOUT_FRAME_OFFSET)
		{
			endX = -0x96;
			elapsedFrames -= TT_RACE_CLOCK_FLYOUT_FRAME_OFFSET;
		}

		// draw race clock in top-left corner
		UI_Lerp2D_Linear(pos.v, 0x14, 8, endX, 8, elapsedFrames, TT_LERP_FRAMES);

		UI_DrawRaceClock(pos.x, pos.y, 0, gGT->drivers[0]);

		return;
	}

	// between 91 and 900 frames (3-30)
	// Return at bottom of IF block
	if (elapsedFrames <= TT_RESULT_MAX_FRAMES)
	{
		// first transition is race clock
		elapsedFrames -= TT_RACE_CLOCK_HOLD_FRAMES;

		// race time
		UI_Lerp2D_Linear(pos.v, -0x64, 90, 0x100, 90, elapsedFrames, TT_LERP_FRAMES);

		TT_EndEvent_DisplayTime(pos.x, pos.y, sdata->flags_timeTrialEndOfRace);


		// Blink Orange/White
		s32 textColor = (gGT->timer & 1) ? 0xffff8000 : 0xffff8004;


		// "new high score" 1 second later
		elapsedFrames -= TT_RESULT_MESSAGE_STEP_FRAMES;

		if ((elapsedFrames > 0) &&

		    // if there is a new high score
		    gGT->newHighScoreIndex > -1)
		{
			UI_Lerp2D_Linear(pos.v, 0x264, 122, 0x100, 122, elapsedFrames, TT_LERP_FRAMES);

			DecalFont_DrawLine(lngStrings[LNG_NEW_HIGH_SCORE], pos.x, pos.y, FONT_BIG, textColor);

			// Total time should flash
			sdata->flags_timeTrialEndOfRace |= TT_TOTAL_TIME_FLASH_FLAG;
		}


		// "new best lap" 1 second later
		elapsedFrames -= TT_RESULT_MESSAGE_STEP_FRAMES;

		if ((elapsedFrames > 0) &&

		    // if got new best lap
		    ((gameModeEnd & NEW_BEST_LAP) != 0))
		{
			UI_Lerp2D_Linear(pos.v, -0x64, 142, 0x100, 142, elapsedFrames, TT_LERP_FRAMES);

			DecalFont_DrawLine(lngStrings[LNG_NEW_BEST_LAP], pos.x, pos.y, FONT_BIG, textColor);

			if ((u32)gGT->lapIndexNewBest < 3)
			{
				// make the best row start flashing
				sdata->flags_timeTrialEndOfRace |= 1 << (TT_BEST_LAP_FLASH_FLAG_FIRST + gGT->lapIndexNewBest);
			}
		}


		// "n tropy" 1 second later
		elapsedFrames -= TT_RESULT_MESSAGE_STEP_FRAMES;

		s32 nTropyEventFlags = NTROPY_JUST_BEAT | NTROPY_JUST_OPENED;

		if ((elapsedFrames > 0) &&

		    // if just open, or beat, n tropy
		    ((gameModeEnd & nTropyEventFlags) != 0))
		{
			UI_Lerp2D_Linear(pos.v, 0x264, 162, 0x100, 162, elapsedFrames, TT_LERP_FRAMES);

			char *nTropyString;
			if ((gameModeEnd & NTROPY_JUST_OPENED) != 0)
			{
				nTropyString = lngStrings[LNG_N_TROPY_OPENED];
			}
			else
			{
				nTropyString = lngStrings[LNG_N_TROPY_BEATEN];
			}

			DecalFont_DrawLine(nTropyString, pos.x, pos.y, FONT_BIG, textColor);
		}

		DecalFont_DrawLine(lngStrings[LNG_PRESS_TO_CONTINUE], 0x100, 0xbe, FONT_BIG, 0xffff8000);

		// If you press Cross or Circle
		if ((sdata->AnyPlayerTap & TT_CONFIRM_BUTTON_MASK) != 0)
		{
			// advance timer, quickly skip to see high scores
			sdata->framesSinceRaceEnded = TT_HIGH_SCORE_MENU_START_FRAME;
		}

		return;
	}

	// Return at bottom of IF block
	if (elapsedFrames < TT_FINAL_MENU_START_FRAME)
	{
		// start drawing the high score menu that shows the top 5 best times
		gGT->gameModeEnd |= DRAW_HIGH_SCORES;

		if ((gameModeEnd & NEW_HIGH_SCORE) == 0)
		{
			// ====== Draw High Score ===========

			if (elapsedFrames > TT_HIGH_SCORE_EXIT_START_FRAME)
			{
				elapsedFrames -= TT_HIGH_SCORE_EXIT_START_FRAME;

				startX = 0x80;
				endX = -0x96;
			}

			else
			{
				elapsedFrames -= TT_HIGH_SCORE_MENU_START_FRAME;

				startX = -0x96;
				endX = 0x80;
			}


			UI_Lerp2D_Linear(pos.v, startX, 10, endX, 10, elapsedFrames, TT_LERP_FRAMES);

			TT_EndEvent_DrawHighScore(pos.x, pos.y, TT_SCORE_MODE_TIME_TRIAL);


			// ====== Draw Your Time ===========


			if (endX == 0x80)
			{
				startX = 0x296;
				endX = 0x180;
			}

			else
			{
				startX = 0x180;
				endX = 0x296;
			}

			UI_Lerp2D_Linear(pos.v, startX, 0x82, endX, 0x82, elapsedFrames, TT_LERP_FRAMES);

			TT_EndEvent_DisplayTime(pos.x, pos.y, sdata->flags_timeTrialEndOfRace);

			DecalFont_DrawLine(lngStrings[LNG_PRESS_TO_CONTINUE], 0x100, 0xbe, FONT_BIG, 0xffff8000);

			// ==== Pause Timer until Press X =======
			// Cross or Circle, or if timer drags on too long
			if (((sdata->AnyPlayerTap & TT_CONFIRM_BUTTON_MASK) != 0) && (sdata->framesSinceRaceEnded <= TT_HIGH_SCORE_EXIT_START_FRAME))
			{
				sdata->framesSinceRaceEnded = TT_HIGH_SCORE_EXIT_START_FRAME;

				// unpause frame counter,
				// which then counts up to the transition-out completion frame
				sdata->menuReadyToPass |= TT_MENU_READY_HIGH_SCORE_EXIT;
			}
		}

		return;
	}

	// if not showing menu yet
	if ((sdata->menuReadyToPass & TT_MENU_READY_SHOW_MENU) == 0)
	{
		// start showing menu
		sdata->menuReadyToPass = (sdata->menuReadyToPass & ~TT_MENU_READY_HIGH_SCORE_EXIT) | TT_MENU_READY_SHOW_MENU;

		sdata->flags_timeTrialEndOfRace = 0;

		RECTMENU_Show(sdata->boolGhostTooBigToSave ? &menu224NoSave : &menu224);
	}

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009f704-0x8009f8c0.
void TT_EndEvent_DisplayTime(int paramX, s16 paramY, u32 UI_DrawRaceClockFlags)
{
	SVec2 pos;
	RECT rectangle;

	struct GameTracker *gGT = sdata->gGT;
	struct Driver *d = gGT->drivers[0];
	s16 startTextWidth = DecalFont_GetLineWidth(sdata->lngStrings[LNG_TOTAL], FONT_BIG);
	s16 endTextWidth = DecalFont_GetLineWidth(sdata->lngStrings[LNG_TOTAL], FONT_BIG);

	// === Naughty Dog Bug ===
	// Start and End is the same
	UI_Lerp2D_Linear(pos.v, (paramX - (0x88 - startTextWidth) / 2), paramY, (paramX - (0x88 - endTextWidth) / 2), paramY, sdata->framesSinceRaceEnded,
	                 TT_LERP_FRAMES);

	DecalFont_DrawLine(sdata->lngStrings[LNG_YOUR_TIME], paramX, ((u32)pos.y - 0x4c), FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	UI_DrawRaceClock(pos.x, pos.y, UI_DrawRaceClockFlags, d);

	rectangle.x = (pos.x - DecalFont_GetLineWidth(sdata->lngStrings[LNG_TOTAL], FONT_BIG)) - 6;
	rectangle.y = pos.y - 0x50;

	rectangle.w = DecalFont_GetLineWidth(sdata->lngStrings[LNG_TOTAL], FONT_BIG) + 0x94;
	rectangle.h = 99;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&rectangle, 4, gGT->backBuffer->otMem.uiOT);

	return;
}

// same in TT and RR, but not the same in Main Menu
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009f8c0-0x8009fdc8.
void TT_EndEvent_DrawHighScore(s16 startX, int startY, s16 scoreMode)
{
	// This is different from High Score in Main Menu because Main Menu
	// does not show the rank icons '1', '2', '3', '4', '5'
	char *timeString;
	u32 timeColor;
	SVec2 pos;
	RECT box;

	struct GameTracker *gGT = sdata->gGT;
	struct Driver *d = gGT->drivers[0];
	s16 timeboxX = startX - 0x1f;
	u16 rowOffsetY = 0;

	// 12 entries per track, 6 for Time Trial and 6 for Relic Race
	struct HighScoreEntry *scoreEntries = &sdata->gameProgress.highScoreTracks[gGT->levelID].scoreEntry[TT_HIGH_SCORE_ENTRIES_PER_MODE * scoreMode];

	// === Naughty Dog Bug ===
	// Start and End is the same

	// interpolate fly-in
	UI_Lerp2D_Linear(pos.v, startX, startY, startX, startY, sdata->framesSinceRaceEnded, TT_LERP_FRAMES);

	DecalFont_DrawLine(sdata->lngStrings[LNG_BEST_TIMES], pos.x, pos.y, FONT_BIG, 0xffff8000);

	// Draw icon, name, and time of the
	// 5 best times in Time Trial
	for (s32 rowIndex = 0; rowIndex < TT_HIGH_SCORE_VISIBLE_ROWS; rowIndex++)
	{
		s32 scoreEntryIndex = rowIndex + TT_HIGH_SCORE_FIRST_VISIBLE_ENTRY;
		s16 timeboxY = startY + 0x11 + rowOffsetY;

		// default color, not flashing
		timeColor = 0;
		s16 nameColor = scoreEntries[scoreEntryIndex].characterID + TT_HIGH_SCORE_DRIVER_COLOR_OFFSET;

		// If this loop index is a new high score
		if (gGT->newHighScoreIndex == rowIndex)
		{
			// make name color flash every odd frame
			nameColor = (gGT->timer & TT_HIGH_SCORE_FLASH_TIMER_BIT) ? WHITE : nameColor;

			// flash color of time
			timeColor = ((gGT->timer & TT_HIGH_SCORE_FLASH_TIMER_BIT) << 1);
		}

		// Make a rank on the high score list ('1', '2', '3', '4', '5')
		// by taking the binary value of the rank (0, 1, 2, 3, 4),
		// and adding the ascii value of '1'
		s_rankString224 = (char)rowIndex + '1';

		// Draw String for Rank ('1', '2', '3', '4', '5')
		DecalFont_DrawLine((char *)&s_rankString224, startX - 0x32, timeboxY - 1, FONT_SMALL, WHITE);

		// Draw Character Icon
		RECTMENU_DrawPolyGT4(gGT->ptrIcons[data.MetaDataCharacters[scoreEntries[scoreEntryIndex].characterID].iconID], startX - 0x52, timeboxY,
		                     &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT,
		                     // color of each corner
		                     TT_HIGH_SCORE_ICON_COLOR, TT_HIGH_SCORE_ICON_COLOR, TT_HIGH_SCORE_ICON_COLOR, TT_HIGH_SCORE_ICON_COLOR,
		                     TT_HIGH_SCORE_ICON_TRANSPARENCY, TT_HIGH_SCORE_ICON_SCALE);

		// Draw Name
		DecalFont_DrawLine(scoreEntries[scoreEntryIndex].name, timeboxX, timeboxY, FONT_CREDITS, nameColor);

		// Draw time
		DecalFont_DrawLine(RECTMENU_DrawTime(scoreEntries[scoreEntryIndex].time), timeboxX, timeboxY + 0x11, FONT_SMALL, timeColor);

		// If this loop index is a new high score
		if (gGT->newHighScoreIndex == rowIndex)
		{
			box.x = startX - 0x56;
			box.y = timeboxY - 1;
			box.w = 0xab;
			box.h = 0x1a;

			// Draw a rectangle to highlight your time on the "Best Times" list
			CTR_Box_DrawClearBox(&box, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, gGT->pushBuffer_UI.ptrOT);
		}
		rowOffsetY += TT_HIGH_SCORE_ROW_SPACING;
	}

	if (scoreMode == TT_SCORE_MODE_TIME_TRIAL)
	{
		// Change the way text flickers
		timeColor = 0xffff8000;

		DecalFont_DrawLine(sdata->lngStrings[LNG_BEST_LAP], startX, startY + 0x95, FONT_BIG, timeColor);

		// If you got a new best lap
		if (((gGT->gameModeEnd & NEW_BEST_LAP) != 0) && ((gGT->timer & TT_HIGH_SCORE_FLASH_TIMER_BIT) != 0))
		{
			timeColor = 0xffff8004;
		}
		// make a string for best lap
		timeString = RECTMENU_DrawTime(scoreEntries[0].time);
	}
	else
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_YOUR_TIME], startX, startY + 0x95, FONT_BIG, 0xffff8000);

		timeString = RECTMENU_DrawTime(d->timeElapsedInRace);
		timeColor = 0xffff8000;
	}

	// Print amount of time, for whichever purpose
	DecalFont_DrawLine(timeString, startX, startY + 0xa6, FONT_SMALL, timeColor);

	box.x = pos.x - 0x60;
	box.y = pos.y - 4;
	box.w = 0xc0;
	box.h = 0xb4;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&box, 4, gGT->backBuffer->otMem.uiOT);
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
        .stringIndex = 0xFFFF,
        .rowOnPressUp = 0,
        .rowOnPressDown = 0,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    }};

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
        .stringIndex = 0xFFFF,
        .rowOnPressUp = 0,
        .rowOnPressDown = 0,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    }};

struct RectMenu menu224 = {
    .stringIndexTitle = 0xFFFF,
    .posX_curr = 0x100,
    .posY_curr = 0xA0,

    .unk1 = 0,

    .state = 0xC83,
    .rows = rowsWithSave,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,

    // rest of variables all default zero
};

struct RectMenu menu224NoSave = {
    .stringIndexTitle = 0xFFFF,
    .posX_curr = 0x100,
    .posY_curr = 0xA0,

    .unk1 = 0,

    .state = 0xC83,
    .rows = rowsNoSave,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,

    // rest of variables all default zero
};
