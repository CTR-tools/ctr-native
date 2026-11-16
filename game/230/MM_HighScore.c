#include <common.h>
#include <widescreen.h>

enum
{
	MM_HIGHSCORE_MAIN_TRANSITION_MAX_FRAME = 0xc,
	MM_HIGHSCORE_SLIDE_TRANSITION_FRAMES = 8,
	MM_HIGHSCORE_LAST_ARCADE_TRACK = 0x11,
	MM_HIGHSCORE_TRACK_SLIDE_STEP_X = 0x40,
	MM_HIGHSCORE_ROW_SLIDE_STEP_Y = 0x1b,
	MM_HIGHSCORE_OFFSCREEN_X = 0x200,
	MM_HIGHSCORE_OFFSCREEN_Y = 0xd8,
	MM_HIGHSCORE_WIPE_RECT_W = 0x228,
	MM_HIGHSCORE_WIPE_RECT_H = 0x19,
	MM_HIGHSCORE_WIPE_RECT_X_OFFSET = -0x14,
	MM_HIGHSCORE_WIPE_RECT_Y_OFFSET = 9,
	MM_HIGHSCORE_TEXT_SHADOW_X = 3,
	MM_HIGHSCORE_TEXT_SHADOW_Y = 1,
	MM_HIGHSCORE_ARROW_ICON_GROUP = 4,
	MM_HIGHSCORE_ARROW_ICON_ID = 0x38,
	MM_HIGHSCORE_ARROW_LEFT_X_OFFSET = 0xec,
	MM_HIGHSCORE_ARROW_RIGHT_X_OFFSET = 0x112,
	MM_HIGHSCORE_ARROW_Y_OFFSET = 0x15,
	MM_HIGHSCORE_ARROW_SCALE = 0x1000,
	MM_HIGHSCORE_ARROW_LEFT_ROTATION = 0x800,
	MM_HIGHSCORE_TITLE_X_OFFSET = 0x100,
	MM_HIGHSCORE_TITLE_Y_OFFSET = 0xe,
	MM_HIGHSCORE_FLASH_TIMER_BIT = 4,
	MM_HIGHSCORE_BEST_TRACK_LABEL_X_OFFSET = 0x20,
	MM_HIGHSCORE_BEST_TRACK_LABEL_Y_OFFSET = 0x2b,
	MM_HIGHSCORE_SCORE_MODE_TIME_TRIAL = 0,
	MM_HIGHSCORE_GHOST_STAR_COUNT = 2,
	MM_HIGHSCORE_GHOST_STAR_ICON_GROUP = 5,
	MM_HIGHSCORE_GHOST_STAR_ICON_ID = 0x37,
	MM_HIGHSCORE_GHOST_STAR_X_OFFSET = 0xf0,
	MM_HIGHSCORE_GHOST_STAR_X_STEP = 0x10,
	MM_HIGHSCORE_GHOST_STAR_Y_OFFSET = 4,
	MM_HIGHSCORE_GHOST_STAR_SCALE = 0x1000,
	MM_HIGHSCORE_TITLE_META_INDEX = 0,
	MM_HIGHSCORE_BEST_TRACK_META_INDEX = 1,
	MM_HIGHSCORE_BEST_LAP_LABEL_META_INDEX = 7,
	MM_HIGHSCORE_BEST_LAP_ENTRY_META_INDEX = 8,
	MM_HIGHSCORE_BEST_LAP_LABEL_X_OFFSET = 0x124,
	MM_HIGHSCORE_BEST_LAP_LABEL_Y_OFFSET = 0x2b,
	MM_HIGHSCORE_BEST_LAP_TEXT_X_OFFSET = 0x160,
	MM_HIGHSCORE_BEST_LAP_NAME_Y_OFFSET = 0x39,
	MM_HIGHSCORE_BEST_LAP_TIME_Y_OFFSET = 0x4a,
	MM_HIGHSCORE_BEST_LAP_ICON_X_OFFSET = 0x124,
	MM_HIGHSCORE_BEST_LAP_ICON_Y_OFFSET = 0x38,
	MM_HIGHSCORE_DRIVER_COLOR_OFFSET = 5,
	MM_HIGHSCORE_ICON_TRANSPARENCY = 1,
	MM_HIGHSCORE_ICON_SCALE = 0x1000,
	MM_HIGHSCORE_VISIBLE_SCORE_ROWS = 5,
	MM_HIGHSCORE_FIRST_VISIBLE_ENTRY = 1,
	MM_HIGHSCORE_FIRST_VISIBLE_META_INDEX = 2,
	MM_HIGHSCORE_SCORE_ROW_Y_STEP = 0x1f,
	MM_HIGHSCORE_SCORE_ICON_X_OFFSET = 0x20,
	MM_HIGHSCORE_SCORE_NAME_X_OFFSET = 0x5c,
	MM_HIGHSCORE_SCORE_NAME_Y_OFFSET = 0x39,
	MM_HIGHSCORE_SCORE_TIME_Y_OFFSET = 0x4a,
	MM_HIGHSCORE_VIDEO_META_INDEX = 9,
	MM_HIGHSCORE_MENU_META_INDEX = 10,
	MM_HIGHSCORE_VIDEO_BOX_W = 0xb0,
	MM_HIGHSCORE_VIDEO_BOX_H = 0x4b,
	MM_HIGHSCORE_VIDEO_BOX_X_OFFSET = 0x124,
	MM_HIGHSCORE_VIDEO_BOX_Y_OFFSET = 0x5a,
	MM_HIGHSCORE_MENU_WIDTH = 0xa4,
	MM_HIGHSCORE_TEXT_JUSTIFICATION_MASK = 0xffffc000u,
};

static int MM_HighScore_IconOffset(struct Icon *icon, int iconX, int textX)
{
	int width = icon != NULL ? icon->texLayout.u1 - icon->texLayout.u0 : 0;
	return iconX + Widescreen_FixedEdgeShift(textX - iconX) - Widescreen_CenteredEdgeShift(width);
}

void MM_HighScore_Text3D(char *string, s32 posX, s32 posY, s16 font, u32 flags)
{
	u32 justificationMask;

	// draw a string
	DecalFont_DrawLine(string, posX, posY, font, flags);

	// draw the same string in a different place
	justificationMask = MM_HIGHSCORE_TEXT_JUSTIFICATION_MASK;
	DecalFont_DrawLine(string, posX + MM_HIGHSCORE_TEXT_SHADOW_X, posY + MM_HIGHSCORE_TEXT_SHADOW_Y, font, (flags & justificationMask) | BLACK);
}

#define MM_HIGHSCORE_COLOR(colorSlot, index) (((Color *)*(colorSlot))[index])

// GCC 2.8.1 derives the retail stack and register layout from
// this C body, but its C-only output matches only 255 of 598 words.
void MM_HighScore_Draw(s16 trackIndex, s32 rowIndex, s32 posX, s32 posY)
{
	register s16 trackIndexArg CTR_PSX_REGISTER("$4") = trackIndex;
	register s32 rowIndexArg CTR_PSX_REGISTER("$5") = rowIndex;
	register s32 posYArg CTR_PSX_REGISTER("$7") = posY;
	register s16 offsetY CTR_PSX_REGISTER("$22");
	struct MetaDataLEV *levelMetadata;
	register struct MainMenu_LevelRow *displayData CTR_PSX_REGISTER("$20");
	register u32 sharedS1 CTR_PSX_REGISTER("$17") = 0;
	register u32 sharedS2 CTR_PSX_REGISTER("$18");
	register u32 sharedS5 CTR_PSX_REGISTER("$21");
	struct TransitionMeta *sharedS8;
	s16 lineWidth;
	s16 bestLapLabelY;
	s32 trackOffsetWork;
	s32 zeroValue;
	s32 leftArrowY;
	s32 titleOffsetX;
	s32 titleOffsetY;
	register s16 scoreRowIndex CTR_PSX_REGISTER("$19");
	struct TransitionMeta *bestLapTransitions;
	register u16 scoreModeWork CTR_PSX_REGISTER("$10");
	u16 videoTransitionY;
	register u32 frameCounterPage CTR_PSX_REGISTER("$2");
	register u16 frameCounterWork CTR_PSX_REGISTER("$2");
	struct
	{
		RECT videoBox;
		u16 trackIndex;
		u8 padAfterTrackIndex[6];
		u16 scoreMode;
		u8 padAfterScoreMode[6];
		s32 rowIndex;
		s32 posX;
		s32 posY;
	} draw;
	register s16 offsetX CTR_PSX_REGISTER("$23");

	CTR_PSX_DEPEND_VALUE(rowIndexArg, sharedS1);
	draw.rowIndex = rowIndexArg;
	scoreModeWork = (u16)draw.rowIndex;
	CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(frameCounterPage, MM_FRAME_COUNTER_ASM_NAME, scoreModeWork);
	draw.posX = posX;
	offsetX = (s16)draw.posX;
	CTR_PSX_DEPEND_VALUE(frameCounterPage, offsetX);
	frameCounterWork = CTR_PSX_PAGE_LVALUE(u16, frameCounterPage, MM_FRAME_COUNTER_PAGE_OFFSET, MM_FRAME_COUNTER);
	CTR_PSX_DEPEND_VALUE(posYArg, frameCounterWork);
	draw.posY = posYArg;
	offsetY = (s16)draw.posY;
	draw.trackIndex = trackIndex;
	draw.scoreMode = scoreModeWork;

	// get color data
	if ((frameCounterWork & MM_HIGHSCORE_FLASH_TIMER_BIT) == 0)
	{
		sharedS1 = RED;
	}

	{
		register struct MainMenu_LevelRow *trackTable CTR_PSX_REGISTER("$11");
		register struct MetaDataLEV *lineMetadata CTR_PSX_REGISTER("$12");

		trackTable = MM_ARCADE_TRACKS;
		displayData = &trackTable[(s16)draw.trackIndex];
		lineMetadata = MM_LEVEL_METADATA;
		lineWidth = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[lineMetadata[displayData->levID].name_LNG], FONT_BIG);
	}

	// Draw arrow pointing Left
	{
		register u32 gameTrackerPage CTR_PSX_REGISTER("$10");
		register u32 workV1 CTR_PSX_REGISTER("$3");
		register u32 *orderingTable CTR_PSX_REGISTER("$4");
		register struct IconGroup *iconGroup CTR_PSX_REGISTER("$8");
		register struct DB *backBuffer CTR_PSX_REGISTER("$7");
		register u32 posYShift CTR_PSX_REGISTER("$12");

		CTR_PSX_LOAD_SYMBOL_PAGE(gameTrackerPage, RETAIL_GAME_TRACKER_ASM_NAME);
		sharedS1 <<= 2;
		workV1 = (u32)CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
		lineWidth /= 2;
		orderingTable = ((struct GameTracker *)workV1)->pushBuffer_UI.ptrOT;
		iconGroup = ((struct GameTracker *)workV1)->iconGroup[MM_HIGHSCORE_ARROW_ICON_GROUP];
		backBuffer = ((struct GameTracker *)workV1)->backBuffer;
		CTR_PSX_LOAD_SYMBOL_PAGE(workV1, MM_COLOR_POINTERS_ASM_NAME);
		CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(workV1, MM_COLOR_POINTERS_ASM_NAME, (u32)MM_COLOR_POINTERS);
		sharedS1 += workV1;
		CTR_PSX_KEEP_VALUE(sharedS1);
		posYShift = *(volatile u32 *)&draw.posY;
		sharedS2 = posYShift << 0x10;
		sharedS2 = (s32)sharedS2 >> 0x10;
		leftArrowY = MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_TITLE_META_INDEX].currY + sharedS2 + MM_HIGHSCORE_ARROW_Y_OFFSET;
		MM_DECALHUD_ARROW_2D((ICONGROUP_GETICONS(iconGroup))[MM_HIGHSCORE_ARROW_ICON_ID],
		                     ((scoreRowIndex = (s16)draw.posX) - lineWidth) + MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_TITLE_META_INDEX].currX +
		                         MM_HIGHSCORE_ARROW_LEFT_X_OFFSET,
		                     leftArrowY, &backBuffer->primMem, orderingTable, MM_HIGHSCORE_COLOR((u32 **)sharedS1, 0), MM_HIGHSCORE_COLOR((u32 **)sharedS1, 1),
		                     MM_HIGHSCORE_COLOR((u32 **)sharedS1, 2), MM_HIGHSCORE_COLOR((u32 **)sharedS1, 3), 0, (sharedS5 = MM_HIGHSCORE_ARROW_SCALE),
		                     MM_HIGHSCORE_ARROW_LEFT_ROTATION);
	}

	trackOffsetWork = MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_TITLE_META_INDEX].currY + (s16)sharedS2;
	zeroValue = 0;

	// Draw arrow pointing Right
	MM_DECALHUD_ARROW_2D((ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[MM_HIGHSCORE_ARROW_ICON_GROUP]))[MM_HIGHSCORE_ARROW_ICON_ID],
	                     MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_TITLE_META_INDEX].currX + (lineWidth + scoreRowIndex) + MM_HIGHSCORE_ARROW_RIGHT_X_OFFSET,
	                     trackOffsetWork + MM_HIGHSCORE_ARROW_Y_OFFSET, &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT,
	                     MM_HIGHSCORE_COLOR((u32 **)sharedS1, 0), MM_HIGHSCORE_COLOR((u32 **)sharedS1, 1), MM_HIGHSCORE_COLOR((u32 **)sharedS1, 2),
	                     MM_HIGHSCORE_COLOR((u32 **)sharedS1, 3), zeroValue, sharedS5, zeroValue);

	// draw track name
	levelMetadata = MM_LEVEL_METADATA;
	titleOffsetX = draw.posX + MM_HIGHSCORE_TITLE_X_OFFSET;
	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[levelMetadata[displayData->levID].name_LNG],
	                   MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_TITLE_META_INDEX].currX + (s16)titleOffsetX,
	                   MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_TITLE_META_INDEX].currY + (s16)(titleOffsetY = draw.posY + MM_HIGHSCORE_TITLE_Y_OFFSET), FONT_BIG,
	                   (s16)JUSTIFY_CENTER);

	if (((u32)draw.rowIndex << 0x10) == MM_HIGHSCORE_SCORE_MODE_TIME_TRIAL)
	{
		s16 ghostStarIndex;
		register u32 ghostV0 CTR_PSX_REGISTER("$2");

		ghostStarIndex = 0;
		CTR_PSX_KEEP_VALUE_RELAXED(ghostStarIndex);
		CTR_PSX_LOAD_SYMBOL_PAGE(sharedS1, RETAIL_GAME_TRACKER_ASM_NAME);
		CTR_PSX_LOAD_SYMBOL_PAGE(ghostV0, RETAIL_GAME_SAVE_ASM_NAME);
		CTR_PSX_ADD_SYMBOL_LOW(sharedS5, ghostV0, RETAIL_GAME_SAVE_ASM_NAME, (u32)&GAME_PROGRESS);
		CTR_PSX_LOAD_SYMBOL_PAGE(ghostV0, MM_HIGHSCORE_GHOST_STAR_FLAGS_ASM_NAME);
		CTR_PSX_ADD_SYMBOL_LOW(displayData, ghostV0, MM_HIGHSCORE_GHOST_STAR_FLAGS_ASM_NAME, (void *)MM_HIGHSCORE_GHOST_STAR_FLAGS);
		ghostV0 = (u32)CTR_PSX_PAGE_LVALUE(struct GameTracker *, sharedS1, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
		sharedS2 = (u32)MM_HIGHSCORE_TRANSITIONS;
		scoreRowIndex = ((struct GameTracker *)ghostV0)->levelID;

		// draw ghost stars
		do
		{
			u32 timeTrialFlags;
			register struct GameTracker *setTracker CTR_PSX_REGISTER("$3");
			register struct GameTracker *drawTracker CTR_PSX_REGISTER("$6");
			register s32 signedGhostStarIndex CTR_PSX_REGISTER("$9");
			register u32 ghostStarIndexShift CTR_PSX_REGISTER("$2");
			register s32 doubledGhostStarIndex CTR_PSX_REGISTER("$7");
			register u16 currentGhostFlag CTR_PSX_REGISTER("$5");
			register s32 flagWordIndex CTR_PSX_REGISTER("$4");
			register u32 trackPage CTR_PSX_REGISTER("$12");

			CTR_PSX_LOAD_SYMBOL_PAGE(trackPage, MM_ARCADE_TRACKS_ASM_NAME);
			CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(trackPage, MM_ARCADE_TRACKS_ASM_NAME, (u32)MM_ARCADE_TRACKS);
			setTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, sharedS1, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
			CTR_PSX_KEEP_VALUE_RELAXED(setTracker);
			setTracker->levelID = ((struct MainMenu_LevelRow *)trackPage)[(s16) * (volatile u16 *)&draw.trackIndex].levID;
			GAMEPROG_GetPtrHighScoreTrack();

			ghostStarIndexShift = (u32)ghostStarIndex << 0x10;
			signedGhostStarIndex = (s32)ghostStarIndexShift >> 0x10;
			CTR_PSX_KEEP_VALUE_RELAXED(signedGhostStarIndex);
			doubledGhostStarIndex = signedGhostStarIndex * sizeof(u16);
			CTR_PSX_KEEP_VALUE_RELAXED(doubledGhostStarIndex);
			currentGhostFlag = *(u16 *)((u8 *)displayData + doubledGhostStarIndex);
			CTR_PSX_KEEP_VALUE_RELAXED(currentGhostFlag);
			drawTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, sharedS1, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
			CTR_PSX_KEEP_VALUE_RELAXED(drawTracker);
			flagWordIndex = (((s16)currentGhostFlag) >> 2) >> 3;
			// NOTE(aalhendi): Retail derives the signed flag-word index in two shifts.
			timeTrialFlags = (&((struct GameProgress *)sharedS5)
			                       ->highScoreTracks[0]
			                       .timeTrialFlags)[drawTracker->levelID * (sizeof(struct HighScoreTrack) / sizeof(u32)) + flagWordIndex];
			if (((timeTrialFlags >> (currentGhostFlag & 0x1f)) & 1) != 0)
			{
				register s16 colorIndex CTR_PSX_REGISTER("$4");
				register u32 colorTablePage CTR_PSX_REGISTER("$2");
				register u32 **ghostColorPtr CTR_PSX_REGISTER("$4");
				register struct IconGroup *ghostIconGroup CTR_PSX_REGISTER("$8");
				register u32 *orderingTable CTR_PSX_REGISTER("$3");
				register struct DB *backBuffer CTR_PSX_REGISTER("$7");
				s32 ghostXBase;
				register s32 ghostX CTR_PSX_REGISTER("$5");
				register s32 ghostY CTR_PSX_REGISTER("$6");

				CTR_PSX_LOAD_SYMBOL_PAGE(colorTablePage, MM_HIGHSCORE_GHOST_STAR_COLORS_ASM_NAME);
				CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(colorTablePage, MM_HIGHSCORE_GHOST_STAR_COLORS_ASM_NAME, (u32)MM_HIGHSCORE_GHOST_STAR_COLORS);
				colorIndex = *(u16 *)((u8 *)colorTablePage + doubledGhostStarIndex);
				CTR_PSX_KEEP_VALUE_RELAXED(colorIndex);
				CTR_PSX_LOAD_SYMBOL_PAGE(colorTablePage, MM_COLOR_POINTERS_ASM_NAME);
				CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(colorTablePage, MM_COLOR_POINTERS_ASM_NAME, (u32)MM_COLOR_POINTERS);
				orderingTable = drawTracker->pushBuffer_UI.ptrOT;
				CTR_PSX_KEEP_VALUE_RELAXED(orderingTable);
				ghostIconGroup = drawTracker->iconGroup[MM_HIGHSCORE_GHOST_STAR_ICON_GROUP];
				CTR_PSX_KEEP_VALUE_RELAXED(ghostIconGroup);
				backBuffer = drawTracker->backBuffer;
				CTR_PSX_KEEP_VALUE_RELAXED(backBuffer);
				ghostColorPtr = (u32 **)(colorTablePage + (s16)colorIndex * sizeof(u32));
				CTR_PSX_DEPEND_VALUE(ghostColorPtr, orderingTable);
				CTR_PSX_ORDER_VALUES(ghostIconGroup, backBuffer);
				ghostX = signedGhostStarIndex * MM_HIGHSCORE_GHOST_STAR_X_STEP;
				ghostXBase = ((struct TransitionMeta *)sharedS2)[MM_HIGHSCORE_TITLE_META_INDEX].currX;
				ghostXBase += (s16)offsetX;
				ghostY = ((struct TransitionMeta *)sharedS2)[MM_HIGHSCORE_TITLE_META_INDEX].currY;
				ghostY += (s16)offsetY;
				ghostX += ghostXBase;
				ghostX += MM_HIGHSCORE_GHOST_STAR_X_OFFSET;

				MM_DECALHUD_DRAW_POLY_GT4((ICONGROUP_GETICONS(ghostIconGroup))[MM_HIGHSCORE_GHOST_STAR_ICON_ID], ghostX,
				                          ghostY + MM_HIGHSCORE_GHOST_STAR_Y_OFFSET, &backBuffer->primMem, orderingTable, MM_HIGHSCORE_COLOR(ghostColorPtr, 0),
				                          MM_HIGHSCORE_COLOR(ghostColorPtr, 1), MM_HIGHSCORE_COLOR(ghostColorPtr, 2), MM_HIGHSCORE_COLOR(ghostColorPtr, 3), 0,
				                          MM_HIGHSCORE_GHOST_STAR_SCALE);
			}

			ghostStarIndex++;
		} while (ghostStarIndex < MM_HIGHSCORE_GHOST_STAR_COUNT);

		GAME_TRACKER->levelID = scoreRowIndex;
		GAMEPROG_GetPtrHighScoreTrack();
	}

	{
		register s16 selectedScoreMode CTR_PSX_REGISTER("$16");

		sharedS1 = (u32)MM_HIGHSCORE_TRANSITIONS;
		selectedScoreMode = (s16)draw.scoreMode;
		sharedS5 = MM_HIGHSCORE_BEST_TRACK_LABEL_Y_OFFSET;
		CTR_PSX_KEEP_VALUE(sharedS5);

		// first entry: Time Trial or Relic
		displayData = (struct MainMenu_LevelRow *)&GAME_PROGRESS.highScoreTracks[MM_ARCADE_TRACKS[(s16)draw.trackIndex].levID].scoreEntry[selectedScoreMode][0];
		CTR_PSX_KEEP_VALUE_RELAXED(displayData);

		MM_HighScore_Text3D(
		    GAME_LANGUAGE_STRINGS[LNG_BEST_TRACK_TIMES],
		    (s16)(((struct TransitionMeta *)sharedS1)[MM_HIGHSCORE_BEST_TRACK_META_INDEX].currX + offsetX + MM_HIGHSCORE_BEST_TRACK_LABEL_X_OFFSET),
		    (s16)(((struct TransitionMeta *)sharedS1)[MM_HIGHSCORE_BEST_TRACK_META_INDEX].currY + offsetY + MM_HIGHSCORE_BEST_TRACK_LABEL_Y_OFFSET), FONT_SMALL,
		    zeroValue);

		scoreRowIndex = zeroValue;

		// if Time Trial
		// with ghost stars, and Best Lap
		if (selectedScoreMode == MM_HIGHSCORE_SCORE_MODE_TIME_TRIAL)
		{
			bestLapLabelY = ((struct TransitionMeta *)sharedS1)[MM_HIGHSCORE_BEST_LAP_LABEL_META_INDEX].currY + offsetY;
			MM_HighScore_Text3D(
			    GAME_LANGUAGE_STRINGS[LNG_BEST_LAP_TIME],
			    (s16)(((struct TransitionMeta *)sharedS1)[MM_HIGHSCORE_BEST_LAP_LABEL_META_INDEX].currX + offsetX + MM_HIGHSCORE_BEST_LAP_LABEL_X_OFFSET),
			    (s16)(bestLapLabelY + sharedS5), FONT_SMALL, 0);

			// Character Name
			MM_HighScore_Text3D(
			    ((struct HighScoreEntry *)displayData)[0].name,
			    (s16)(((struct TransitionMeta *)sharedS1)[MM_HIGHSCORE_BEST_LAP_ENTRY_META_INDEX].currX + offsetX + MM_HIGHSCORE_BEST_LAP_TEXT_X_OFFSET),
			    (s16)(((struct TransitionMeta *)sharedS1)[MM_HIGHSCORE_BEST_LAP_ENTRY_META_INDEX].currY + offsetY + MM_HIGHSCORE_BEST_LAP_NAME_Y_OFFSET),
			    FONT_BIG, (s16)(((struct HighScoreEntry *)displayData)[0].characterID + MM_HIGHSCORE_DRIVER_COLOR_OFFSET));
			bestLapTransitions = (struct TransitionMeta *)sharedS1;

			// Draw time string
			// NOTE(aalhendi): Retail also uses currX as the Y transition base here.
			MM_HighScore_Text3D(
			    RECTMENU_DrawTime(((struct HighScoreEntry *)displayData)[0].time),
			    (s16)(bestLapTransitions[MM_HIGHSCORE_BEST_LAP_ENTRY_META_INDEX].currX + offsetX + MM_HIGHSCORE_BEST_LAP_TEXT_X_OFFSET),
			    (s16)(((struct TransitionMeta *)sharedS1)[MM_HIGHSCORE_BEST_LAP_ENTRY_META_INDEX].currX + offsetY + MM_HIGHSCORE_BEST_LAP_TIME_Y_OFFSET),
			    FONT_SMALL, zeroValue);

			// Character Icon
			struct Icon *icon = GAME_TRACKER->ptrIcons[GAME_CHARACTER_METADATA[(s16)((struct HighScoreEntry *)displayData)[0].characterID].iconID];
			MM_RECTMENU_DRAW_POLY_GT4(icon,
			                          bestLapTransitions[MM_HIGHSCORE_BEST_LAP_ENTRY_META_INDEX].currX + (s16)offsetX +
			                              MM_HighScore_IconOffset(icon, MM_HIGHSCORE_BEST_LAP_ICON_X_OFFSET, MM_HIGHSCORE_BEST_LAP_TEXT_X_OFFSET),
			                          ((struct TransitionMeta *)sharedS1)[MM_HIGHSCORE_BEST_LAP_ENTRY_META_INDEX].currY + (s16)offsetY +
			                              MM_HIGHSCORE_BEST_LAP_ICON_Y_OFFSET,
			                          &GAME_TRACKER->backBuffer->primMem, (GAME_TRACKER->pushBuffer_UI).ptrOT, MM_HIGHSCORE_ICON_COLOR, MM_HIGHSCORE_ICON_COLOR,
			                          MM_HIGHSCORE_ICON_COLOR, MM_HIGHSCORE_ICON_COLOR, MM_HIGHSCORE_ICON_TRANSPARENCY, MM_HIGHSCORE_ICON_SCALE);
		}
	}

	// Draw five "best track times"
	// Icon, Name, and Time
	CTR_PSX_COPY_VALUE(sharedS8, (struct TransitionMeta *)sharedS1);
	CTR_PSX_KEEP_VALUE(sharedS8);
	for (scoreRowIndex = 0; scoreRowIndex <= MM_HIGHSCORE_VISIBLE_SCORE_ROWS - 1;)
	{
		struct HighScoreEntry *rowEntry;
		s32 currentRowIndex = (s16)scoreRowIndex;
		s32 metaIndex;
		metaIndex = currentRowIndex + MM_HIGHSCORE_FIRST_VISIBLE_META_INDEX;
		sharedS1 = (u32)&sharedS8[metaIndex];
		sharedS2 = currentRowIndex * MM_HIGHSCORE_SCORE_ROW_Y_STEP;
		rowEntry = (struct HighScoreEntry *)displayData;
		rowEntry += currentRowIndex + MM_HIGHSCORE_FIRST_VISIBLE_ENTRY;
		CTR_PSX_KEEP_VALUE(sharedS1);

		// Character Icon
		struct Icon *icon = GAME_TRACKER->ptrIcons[GAME_CHARACTER_METADATA[(s16)rowEntry->characterID].iconID];
		MM_RECTMENU_DRAW_POLY_GT4(icon,
		                          ((struct TransitionMeta *)sharedS1)[0].currX + (s16)offsetX +
		                              MM_HighScore_IconOffset(icon, MM_HIGHSCORE_SCORE_ICON_X_OFFSET, MM_HIGHSCORE_SCORE_NAME_X_OFFSET),
		                          ((struct TransitionMeta *)sharedS1)[0].currY + (s16)offsetY + sharedS5 + (s16)sharedS2 +
		                              (MM_HIGHSCORE_SCORE_NAME_Y_OFFSET - MM_HIGHSCORE_BEST_TRACK_LABEL_Y_OFFSET),
		                          &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT, MM_HIGHSCORE_ICON_COLOR, MM_HIGHSCORE_ICON_COLOR,
		                          MM_HIGHSCORE_ICON_COLOR, MM_HIGHSCORE_ICON_COLOR, MM_HIGHSCORE_ICON_TRANSPARENCY, MM_HIGHSCORE_ICON_SCALE);

		// draw the name string
		{
			s32 nameRowYOffset = (s32)sharedS2 + (MM_HIGHSCORE_SCORE_NAME_Y_OFFSET - MM_HIGHSCORE_BEST_TRACK_LABEL_Y_OFFSET);

			MM_HighScore_Text3D(rowEntry->name, (s16)(((struct TransitionMeta *)sharedS1)[0].currX + offsetX + MM_HIGHSCORE_SCORE_NAME_X_OFFSET),
			                    (s16)(((struct TransitionMeta *)sharedS1)[0].currY + offsetY + sharedS5 + nameRowYOffset), FONT_BIG,
			                    (s16)(rowEntry->characterID + MM_HIGHSCORE_DRIVER_COLOR_OFFSET));
		}

		// draw the Time string
		{
			char *timeString = RECTMENU_DrawTime(rowEntry->time);

			sharedS2 = sharedS2 + MM_HIGHSCORE_SCORE_ROW_Y_STEP;
			CTR_PSX_KEEP_VALUE(sharedS2);
			MM_HighScore_Text3D(timeString, (s16)(((struct TransitionMeta *)sharedS1)[0].currX + offsetX + MM_HIGHSCORE_SCORE_NAME_X_OFFSET),
			                    (s16)(((struct TransitionMeta *)sharedS1)[0].currY + offsetY + sharedS5 + sharedS2), FONT_SMALL, zeroValue);
		}

		{
			register s16 nextScoreRowIndex CTR_PSX_REGISTER("$2");
			nextScoreRowIndex = scoreRowIndex + 1;
			scoreRowIndex = nextScoreRowIndex;
		}
	}

	draw.videoBox.w = MM_HIGHSCORE_VIDEO_BOX_W;
	draw.videoBox.h = MM_HIGHSCORE_VIDEO_BOX_H;
	draw.videoBox.x = (u16)MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_VIDEO_META_INDEX].currX + (u16)offsetX + MM_HIGHSCORE_VIDEO_BOX_X_OFFSET;
	videoTransitionY = (u16)MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_VIDEO_META_INDEX].currY;
	draw.videoBox.y = videoTransitionY + (u16)offsetY + MM_HIGHSCORE_VIDEO_BOX_Y_OFFSET;

	MM_TrackSelect_Video_Draw(&draw.videoBox, &MM_ARCADE_TRACKS[0], (s16) * (volatile u16 *)&draw.trackIndex, (MM_HIGHSCORE_TRANSITION_STATE == EXITING_MENU),
	                          0);
}

#undef MM_HIGHSCORE_COLOR

void MM_HighScore_Init(void)
{
	MM_HIGHSCORE_TRANSITION_STATE = ENTERING_MENU;
	MM_HIGHSCORE_TRANSITION_MAIN_FRAME = MM_HIGHSCORE_MAIN_TRANSITION_MAX_FRAME;
	MM_HIGHSCORE_TARGET_ROW = 0;
	MM_HIGHSCORE_CURRENT_ROW = 0;

	// reset all video variables
	MM_TrackSelect_Video_SetDefaults();
}

void MM_HighScore_MenuProc(struct RectMenu *menu_unused)
{
	b32 videoResetRequested;
	s32 currOffsetY;
	s32 currOffsetX;
	s32 nextOffsetX;
	s32 nextOffsetY;
	register s32 compareOffsetX CTR_PSX_REGISTER("$7");
	register s32 compareOffsetY CTR_PSX_REGISTER("$5");
	register s32 currentTransitionTrackFrame CTR_PSX_REGISTER("$4");
	register s32 sharedV0 CTR_PSX_REGISTER("$2");
	register s32 sharedV1 CTR_PSX_REGISTER("$3");
	register const struct TransitionMeta *titleMeta CTR_PSX_REGISTER("$4");
	register const s16 *activeVerticalMoveAddress CTR_PSX_REGISTER("$2");
	RECT wipeRect;
	s16 pendingVerticalMove;

	(void)menu_unused;

	videoResetRequested = false;
	if (MM_HIGHSCORE_TRANSITION_STATE == IN_MENU)
	{
		goto LAB_HIGHSCORE_PROCESS_INPUT;
	}
	if (MM_HIGHSCORE_TRANSITION_STATE < EXITING_MENU)
	{
		if (MM_HIGHSCORE_TRANSITION_STATE == ENTERING_MENU)
		{
			goto LAB_HIGHSCORE_ENTERING;
		}
		goto LAB_HIGHSCORE_TRANSITION_DONE;
	}
	if (MM_HIGHSCORE_TRANSITION_STATE == EXITING_MENU)
	{
		goto LAB_HIGHSCORE_EXITING;
	}
	goto LAB_HIGHSCORE_TRANSITION_DONE;

LAB_HIGHSCORE_ENTERING:
	MM_TransitionInOut(MM_HIGHSCORE_TRANSITIONS, MM_HIGHSCORE_TRANSITION_MAIN_FRAME, MM_HIGHSCORE_SLIDE_TRANSITION_FRAMES);
	if (MM_HIGHSCORE_TRANSITION_MAIN_FRAME != 0)
	{
		MM_HIGHSCORE_TRANSITION_MAIN_FRAME--;
		goto LAB_HIGHSCORE_TRANSITION_DONE;
	}
	MM_HIGHSCORE_TRANSITION_STATE = IN_MENU;
	goto LAB_HIGHSCORE_TRANSITION_DONE;

LAB_HIGHSCORE_EXITING:
	if ((MM_HIGHSCORE_TRANSITION_TRACK_FRAME == 0) && (MM_HIGHSCORE_TRANSITION_ROW_FRAME == 0))
	{
		MM_TransitionInOut(MM_HIGHSCORE_TRANSITIONS, MM_HIGHSCORE_TRANSITION_MAIN_FRAME, MM_HIGHSCORE_SLIDE_TRANSITION_FRAMES);
		MM_HIGHSCORE_TRANSITION_MAIN_FRAME++;
		if (MM_HIGHSCORE_MAIN_TRANSITION_MAX_FRAME < MM_HIGHSCORE_TRANSITION_MAIN_FRAME)
		{
			MM_JumpTo_Title_Returning();
			return;
		}
	}

LAB_HIGHSCORE_TRANSITION_DONE:
	if (MM_HIGHSCORE_TRANSITION_STATE != IN_MENU)
	{
		goto LAB_OVR_230__800b3c78;
	}

LAB_HIGHSCORE_PROCESS_INPUT:
	if ((MM_GAME_BUTTON_TAPS[0] & BTN_UP) != 0)
	{
		if (MM_HIGHSCORE_MENU.rowSelected == 1)
		{
			videoResetRequested = true;
		}
	}
	else if (((MM_GAME_BUTTON_TAPS[0] & BTN_DOWN) != 0) && (MM_HIGHSCORE_MENU.rowSelected < 1))
	{
		videoResetRequested = true;
	}

	if ((MM_GAME_BUTTON_TAPS[0] & (BTN_SQUARE_one | BTN_TRIANGLE)) != 0)
	{
		videoResetRequested = true;
		OtherFX_Play(2, 1);
		MM_HIGHSCORE_TRANSITION_STATE = EXITING_MENU;
	}
	else if ((MM_GAME_BUTTON_TAPS[0] & BTN_LEFT) != 0)
	{
		b32 trackOpen;
		videoResetRequested = true;
		MM_HIGHSCORE_PENDING_HORIZONTAL_MOVE = -1;

		do
		{
			MM_HIGHSCORE_TARGET_TRACK = MM_HIGHSCORE_TARGET_TRACK - 1;
			if (MM_HIGHSCORE_TARGET_TRACK < 0)
			{
				MM_HIGHSCORE_TARGET_TRACK = MM_HIGHSCORE_LAST_ARCADE_TRACK;
			}
			trackOpen = MM_TrackSelect_boolTrackOpen(MM_ARCADE_TRACKS + MM_HIGHSCORE_TARGET_TRACK);
		} while (!trackOpen);
	}
	else if ((MM_GAME_BUTTON_TAPS[0] & BTN_RIGHT) != 0)
	{
		b32 trackOpen;
		videoResetRequested = true;
		MM_HIGHSCORE_PENDING_HORIZONTAL_MOVE = 1;

		do
		{
			MM_HIGHSCORE_TARGET_TRACK = MM_HIGHSCORE_TARGET_TRACK + 1;
			if (MM_HIGHSCORE_LAST_ARCADE_TRACK < MM_HIGHSCORE_TARGET_TRACK)
			{
				MM_HIGHSCORE_TARGET_TRACK = 0;
			}
			trackOpen = MM_TrackSelect_boolTrackOpen(MM_ARCADE_TRACKS + MM_HIGHSCORE_TARGET_TRACK);
		} while (!trackOpen);
	}
	else
	{
		s32 menuResult = RECTMENU_ProcessInput(&MM_HIGHSCORE_MENU);
		if ((s16)menuResult == -1)
		{
			goto LAB_HIGHSCORE_MENU_BACK;
		}
		if ((s16)menuResult == 1)
		{
			goto LAB_HIGHSCORE_MENU_CONFIRM;
		}
		goto LAB_HIGHSCORE_MENU_RESULT_DONE;

	LAB_HIGHSCORE_MENU_BACK:
		MM_HIGHSCORE_TRANSITION_STATE = EXITING_MENU;
		goto LAB_HIGHSCORE_MENU_RESULT_DONE;

	LAB_HIGHSCORE_MENU_CONFIRM:
		if (MM_HIGHSCORE_MENU.rowSelected == 2)
		{
			MM_HIGHSCORE_TRANSITION_STATE = EXITING_MENU;
		}

	LAB_HIGHSCORE_MENU_RESULT_DONE:
		if (((u16)MM_HIGHSCORE_MENU.rowSelected < 2) && (MM_HIGHSCORE_TARGET_ROW != MM_HIGHSCORE_MENU.rowSelected))
		{
			pendingVerticalMove = -1;
			if (MM_HIGHSCORE_MENU.rowSelected != 0)
			{
				pendingVerticalMove = 1;
			}
			MM_HIGHSCORE_PENDING_VERTICAL_MOVE = pendingVerticalMove;
			MM_HIGHSCORE_TARGET_ROW = MM_HIGHSCORE_MENU.rowSelected;
		}
	}

LAB_OVR_230__800b3c78:

{
	b32 videoState = (((videoResetRequested) || (MM_HIGHSCORE_TRANSITION_TRACK_FRAME != 0)) || (MM_HIGHSCORE_TRANSITION_ROW_FRAME != 0)) ||
	                 (MM_HIGHSCORE_TRANSITION_STATE == EXITING_MENU);
	MM_TrackSelect_Video_State(videoState);
}
	if (MM_HIGHSCORE_TRANSITION_TRACK_FRAME != 0)
	{
		MM_HIGHSCORE_TRANSITION_TRACK_FRAME--;
		if (MM_HIGHSCORE_TRANSITION_TRACK_FRAME == 0)
		{
			MM_HIGHSCORE_CURRENT_TRACK = MM_HIGHSCORE_TARGET_TRACK;
		}
	}
	else if (MM_HIGHSCORE_TRANSITION_ROW_FRAME == 0)
	{
		if (MM_HIGHSCORE_CURRENT_TRACK != MM_HIGHSCORE_TARGET_TRACK)
		{
			MM_HIGHSCORE_TRANSITION_TRACK_FRAME = MM_HIGHSCORE_SLIDE_TRANSITION_FRAMES;
			MM_HIGHSCORE_ACTIVE_HORIZONTAL_MOVE = MM_HIGHSCORE_PENDING_HORIZONTAL_MOVE;
		}
		else if (MM_HIGHSCORE_CURRENT_ROW != MM_HIGHSCORE_TARGET_ROW)
		{
			MM_HIGHSCORE_TRANSITION_ROW_FRAME = MM_HIGHSCORE_SLIDE_TRANSITION_FRAMES;
			MM_HIGHSCORE_ACTIVE_VERTICAL_MOVE = MM_HIGHSCORE_PENDING_VERTICAL_MOVE;
		}
	}
	else
	{
		MM_HIGHSCORE_TRANSITION_ROW_FRAME--;
		if (MM_HIGHSCORE_TRANSITION_ROW_FRAME == 0)
		{
			MM_HIGHSCORE_CURRENT_ROW = MM_HIGHSCORE_TARGET_ROW;
		}
	}

	RECTMENU_DrawSelf(&MM_HIGHSCORE_MENU, MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_MENU_META_INDEX].currX,
	                  MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_MENU_META_INDEX].currY, MM_HIGHSCORE_MENU_WIDTH);

	currOffsetX = 0;
	currOffsetY = 0;
	currentTransitionTrackFrame = MM_HIGHSCORE_TRANSITION_TRACK_FRAME;

	if (currentTransitionTrackFrame != 0)
	{
		sharedV1 = MM_HIGHSCORE_SLIDE_TRANSITION_FRAMES;
		sharedV0 = MM_HIGHSCORE_ACTIVE_HORIZONTAL_MOVE;
		sharedV1 -= currentTransitionTrackFrame;
		sharedV0 *= MM_HIGHSCORE_TRACK_SLIDE_STEP_X;
		currOffsetX = sharedV1 * sharedV0;
	}
	else
	{
		CTR_PSX_LOAD_SYMBOL_PAGE(sharedV1, MM_HIGHSCORE_TRANSITION_ROW_FRAME_ASM_NAME);
		compareOffsetY = MM_HIGHSCORE_SLIDE_TRANSITION_FRAMES;
		activeVerticalMoveAddress = &MM_HIGHSCORE_ACTIVE_VERTICAL_MOVE;
		currentTransitionTrackFrame = CTR_PSX_PAGE_LVALUE(s16, sharedV1, MM_HIGHSCORE_TRANSITION_ROW_FRAME_PAGE_OFFSET, MM_HIGHSCORE_TRANSITION_ROW_FRAME);
		CTR_PSX_KEEP_VALUE(currentTransitionTrackFrame);
		sharedV1 = *activeVerticalMoveAddress;
		compareOffsetY -= currentTransitionTrackFrame;
		sharedV0 = sharedV1 * MM_HIGHSCORE_ROW_SLIDE_STEP_Y;
		currOffsetY = compareOffsetY * sharedV0;
	}
	if (((currOffsetX != -MM_HIGHSCORE_OFFSCREEN_X) && (currOffsetX != MM_HIGHSCORE_OFFSCREEN_X)) &&
	    ((currOffsetY != -MM_HIGHSCORE_OFFSCREEN_Y && (currOffsetY != MM_HIGHSCORE_OFFSCREEN_Y))))
	{
		MM_HighScore_Draw((int)MM_HIGHSCORE_CURRENT_TRACK, (int)MM_HIGHSCORE_CURRENT_ROW, (int)(s16)currOffsetX, (int)(s16)currOffsetY);
		if (MM_HIGHSCORE_TRANSITION_ROW_FRAME != 0)
		{
			// draw rectangle
			sharedV0 = MM_HIGHSCORE_WIPE_RECT_W;
			CTR_PSX_KEEP_VALUE(sharedV0);
			titleMeta = &MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_TITLE_META_INDEX];
			sharedV1 = MM_HIGHSCORE_WIPE_RECT_H;
			wipeRect.w = sharedV0;
			sharedV0 = (u16)titleMeta->currX;
			compareOffsetY = currOffsetY + MM_HIGHSCORE_WIPE_RECT_Y_OFFSET;
			wipeRect.h = sharedV1;
			wipeRect.x = sharedV0 + MM_HIGHSCORE_WIPE_RECT_X_OFFSET;
			wipeRect.y = titleMeta->currY + (s16)compareOffsetY;
			RECTMENU_DrawInnerRect(&wipeRect, 0, sdata_static.gGT->backBuffer->otMem.uiOT);
		}
	}
	nextOffsetX = 0;
	nextOffsetY = 0;
	compareOffsetX = currOffsetX;
	compareOffsetY = currOffsetY;
	sharedV0 = MM_HIGHSCORE_TRANSITION_TRACK_FRAME;
	if (sharedV0 != 0)
	{
		sharedV0 *= MM_HIGHSCORE_TRACK_SLIDE_STEP_X;
		sharedV1 = MM_HIGHSCORE_ACTIVE_HORIZONTAL_MOVE;
		sharedV0 = -sharedV0;
		nextOffsetX = sharedV0 * sharedV1;
	}
	else
	{
		currentTransitionTrackFrame = MM_HIGHSCORE_TRANSITION_ROW_FRAME;
		sharedV1 = MM_HIGHSCORE_ACTIVE_VERTICAL_MOVE;
		currentTransitionTrackFrame *= -MM_HIGHSCORE_ROW_SLIDE_STEP_Y;
		nextOffsetY = currentTransitionTrackFrame * sharedV1;
	}
	if (((compareOffsetX != nextOffsetX) || (compareOffsetY != nextOffsetY)) &&
	    ((nextOffsetX != -MM_HIGHSCORE_OFFSCREEN_X &&
	      (((nextOffsetX != MM_HIGHSCORE_OFFSCREEN_X && (nextOffsetY != -MM_HIGHSCORE_OFFSCREEN_Y)) && (nextOffsetY != MM_HIGHSCORE_OFFSCREEN_Y))))))
	{
		MM_HighScore_Draw((int)MM_HIGHSCORE_TARGET_TRACK, (int)MM_HIGHSCORE_TARGET_ROW, (int)(s16)nextOffsetX, (int)(s16)nextOffsetY);
	}

	// draw rectangle
	sharedV0 = MM_HIGHSCORE_WIPE_RECT_W;
	CTR_PSX_KEEP_VALUE(sharedV0);
	titleMeta = &MM_HIGHSCORE_TRANSITIONS[MM_HIGHSCORE_TITLE_META_INDEX];
	sharedV1 = MM_HIGHSCORE_WIPE_RECT_H;
	wipeRect.w = sharedV0;
	sharedV0 = (u16)titleMeta->currX;
	compareOffsetY = nextOffsetY + MM_HIGHSCORE_WIPE_RECT_Y_OFFSET;
	wipeRect.h = sharedV1;
	wipeRect.x = sharedV0 + MM_HIGHSCORE_WIPE_RECT_X_OFFSET;
	wipeRect.y = titleMeta->currY + (s16)compareOffsetY;
	RECTMENU_DrawInnerRect(&wipeRect, 0, sdata_static.gGT->backBuffer->otMem.uiOT);

	return;
}
