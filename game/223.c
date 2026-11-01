#include <common.h>

enum RelicRaceEndMenuConstants
{
	RR_SAPPHIRE_RELIC_INDEX = 0,
	RR_GOLD_RELIC_INDEX = 1,
	RR_PLATINUM_RELIC_INDEX = 2,
	RR_RACE_TIME_ONE_SECOND = 0x3c0,
	RR_RACE_TIME_TEN_SECONDS = 0x2580,
	RR_RACE_TIME_ONE_MINUTE = 0xe100,
	RR_RESULT_MAX_FRAMES = CTR_SECONDS_TO_FRAMES(30),
	RR_HIGH_SCORE_REVEAL_FRAME = CTR_SECONDS_TO_FRAMES(17),
	RR_MISSED_CRATE_SKIP_BASE = 21,
	RR_MISSED_CRATE_SKIP_PERFECT_WINDOW = 59,
	RR_MISSED_CRATE_SKIP_RELIC_WINDOW = 229,
	RR_PERFECT_SKIP_FRAME = 140,
	RR_RELIC_SKIP_FRAME = 370,
	RR_FLYOUT_START_FRAME = 491,
	RR_FLYOUT_FRAME_OFFSET = 490,
	RR_LERP_FRAMES = 0x14,
	RR_RELIC_AWARD_START_FRAME = 250,
	RR_RELIC_GROW_START_FRAME = 251,
	RR_PERFECT_START_FRAME = 80,
	RR_PERFECT_FLYOUT_OFFSET = 170,
	RR_COUNTDOWN_START_FRAME = 140,
	RR_COUNTDOWN_TICK_START_FRAME = 160,
	RR_COUNTDOWN_WINDOW_FRAMES = 110,
	RR_COUNTDOWN_STEP_FRAMES = 5,
	RR_HIGH_SCORE_BANNER_START_FRAME = 370,
	RR_HIGH_SCORE_BANNER_HOLD_FRAMES = CTR_SECONDS_TO_FRAMES(4),
	RR_TIMEBOX_SCALE = 0x300,
	RR_RELIC_FULL_SCALE = 0xc00,
	RR_RELIC_GROW_STEP = 0x80,
	RR_SCREEN_DEPTH = 0x100,
	RR_PLATINUM_RELIC_COLOR = INST_COLOR_PLATINUM_RELIC,
	RR_GOLD_RELIC_COLOR = INST_COLOR_GOLD_RELIC,
	RR_RELIC_AWARD_SFX = 0x67,
	RR_COUNTDOWN_TICK_SFX = 99,
	RR_CONFIRM_BUTTON_MASK = BTN_CROSS_one | BTN_CIRCLE,
	RR_MENU_READY_FLAG = 1,
	RR_SCORE_MODE_TIME_TRIAL = 0,
	RR_SCORE_MODE_RELIC_RACE = 1,
	RR_HIGH_SCORE_VISIBLE_ROWS = 5,
	RR_HIGH_SCORE_FIRST_VISIBLE_ENTRY = 1,
	RR_HIGH_SCORE_ROW_SPACING = 0x1a,
	RR_HIGH_SCORE_DRIVER_COLOR_OFFSET = 5,
	RR_HIGH_SCORE_FLASH_TIMER_BIT = 2,
	RR_HIGH_SCORE_ICON_SCALE = 0x1000,
};

// NOTE(aalhendi): Retail stores this writable one-character string before the
// overlay code.
global_variable s32 s_rankString223 CTR_PSX_MATCH_SECTION(".rodata") = 0x20;
global_variable const s32 s_timeCrateXString223 = 'x';
global_variable const char s_crateCountFormat223[12] = "%2.02d/%ld";
global_variable const char s_countdownStartFormat223[4] = "-10";
global_variable const char s_countdownFormat223[4] = "-%d";
global_variable Color s_highScoreIconColor223;

#define RR_RELIC            (sdata->ptrRelic)
#define RR_TIMEBOX1         (sdata->ptrTimebox1)
#define RR_RELIC_TIME_1MIN  (sdata->relicTime_1min)
#define RR_RELIC_TIME_10SEC (sdata->relicTime_10sec)
#define RR_RELIC_TIME_1SEC  (sdata->relicTime_1sec)
#define RR_RELIC_TIME_10MS  (sdata->relicTime_10ms)
#define RR_RELIC_TIME_1MS   (sdata->relicTime_1ms)

#define RR_DRAW_POLY_GT4(icon, posX, posY, primMem, ot, color0, color1, color2, color3, transparency, scale) \
	RECTMENU_DrawPolyGT4((icon), (s16)(posX), (s32)(posY), (primMem), (ot), (color0), (color1), (color2), (color3), (s8)(transparency), (s16)(scale))
#define RR_DRAW_CLEAR_BOX CTR_Box_DrawClearBox

// NOTE(aalhendi): Native uses the shared s16 declaration; retail preserves
// overlay 223's wider caller-side x argument through its private ABI binding.
#define RR_DRAW_LINE_WIDE_X DecalFont_DrawLine

// The tier time the team has to beat. Reading the shared accessor keeps the goal
// shown on the results screen equal to the one the award was decided on.
static s32 RR_TeamTargetTime(struct GameTracker *gGT, s32 tier)
{
	return GAMEPROG_RelicTargetTime(gGT->levelID, tier);
}

void RR_EndEvent_UnlockAward(void)
{
	// One clock and one crate pool decide the pair's relic, so both the tier
	// comparison and the all-crates bonus are read off the team, not off player one.
	s32 timeDeduct;
	s32 teamTime;
	s32 goldTarget;
	s32 platinumTarget;

	{
		struct GameTracker *gGT = GAME_TRACKER;

		timeDeduct = MainGameEnd_GotAllTimeCrates(gGT) ? RR_RACE_TIME_TEN_SECONDS : 0;
		teamTime = MainGameEnd_GradeDriver(gGT)->timeElapsedInRace - timeDeduct;
		goldTarget = RR_TeamTargetTime(gGT, RR_GOLD_RELIC_INDEX);
		platinumTarget = RR_TeamTargetTime(gGT, RR_PLATINUM_RELIC_INDEX);
	}

	{
		struct GameTracker *gGT;
		u32 *rewards;
		s32 rewardBit;

		gGT = GAME_TRACKER;

		if (teamTime <= RR_TeamTargetTime(gGT, RR_SAPPHIRE_RELIC_INDEX))
		{
			rewards = GAME_ADV_PROGRESS.rewards;
			rewardBit = gGT->levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC;
			if (!CHECK_ADV_BIT(rewards, rewardBit))
			{
				UNLOCK_ADV_BIT(rewards, rewardBit);
				gGT->podiumRewardID = STATIC_RELIC;
				gGT->gameModeEnd |= NEW_RELIC;

				if (gGT->levelID == TURBO_TRACK)
				{
					GAME_PROGRESS.unlocks[0] |= GAME_UNLOCK_TURBO_TRACK_MASK;
				}
			}
		}
	}

	{
		struct GameTracker *gGT;
		u32 *rewards;
		s32 rewardBit;

		gGT = GAME_TRACKER;
		if (teamTime <= goldTarget)
		{
			rewards = GAME_ADV_PROGRESS.rewards;
			rewardBit = gGT->levelID + ADV_REWARD_FIRST_GOLD_RELIC;
			if (!CHECK_ADV_BIT(rewards, rewardBit))
			{
				UNLOCK_ADV_BIT(rewards, rewardBit);
				gGT->podiumRewardID = STATIC_RELIC;
				gGT->gameModeEnd |= NEW_RELIC;

				RR_RELIC_TIME_1MIN = goldTarget / RR_RACE_TIME_ONE_MINUTE;
				RR_RELIC_TIME_10SEC = (goldTarget / RR_RACE_TIME_TEN_SECONDS) % 6;
				RR_RELIC_TIME_1SEC = (goldTarget / RR_RACE_TIME_ONE_SECOND) % 10;
				RR_RELIC_TIME_10MS = (goldTarget / (RR_RACE_TIME_ONE_SECOND / 10)) % 10;
				RR_RELIC_TIME_1MS = ((goldTarget * 100) / RR_RACE_TIME_ONE_SECOND) % 10;
			}
		}
	}

	{
		struct GameTracker *gGT;
		u32 *rewards;
		s32 rewardBit;

		gGT = GAME_TRACKER;
		if (teamTime <= platinumTarget)
		{
			rewards = GAME_ADV_PROGRESS.rewards;
			rewardBit = gGT->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC;
			if (!CHECK_ADV_BIT(rewards, rewardBit))
			{
				UNLOCK_ADV_BIT(rewards, rewardBit);
				gGT->podiumRewardID = STATIC_RELIC;
				gGT->gameModeEnd |= NEW_RELIC;

				RR_RELIC_TIME_1MIN = platinumTarget / RR_RACE_TIME_ONE_MINUTE;
				RR_RELIC_TIME_10SEC = (platinumTarget / RR_RACE_TIME_TEN_SECONDS) % 6;
				RR_RELIC_TIME_1SEC = (platinumTarget / RR_RACE_TIME_ONE_SECOND) % 10;
				RR_RELIC_TIME_10MS = (platinumTarget / (RR_RACE_TIME_ONE_SECOND / 10)) % 10;
				RR_RELIC_TIME_1MS = ((platinumTarget * 100) / RR_RACE_TIME_ONE_SECOND) % 10;
			}
		}
	}
}

// same in TT and RR, but not the same in Main Menu
void RR_EndEvent_DrawHighScore(s16 startX, s32 startY, s16 scoreMode)
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
		driver = MainGameEnd_GradeDriver(GAME_TRACKER);
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
		UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), startX, startYCopy, startX, startYCopy, GAME_FRAMES_SINCE_RACE_ENDED, RR_LERP_FRAMES);

		rowIndex = 0;
		scoreEntryOffset = sizeof(struct HighScoreEntry);
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_BEST_TIMES], pos.x, pos.y, FONT_BIG, JUSTIFY_CENTER | ORANGE);

		iconYBase = (s16)iconYBaseSource;
		timeboxX = timeboxXSource;
	}
	// NOTE(aalhendi): This lifetime boundary preserves retail's spill order.
	rankString = (char *)&s_rankString223;

	// Draw the icon, name, and time for the five best scores in the selected mode.
	for (; rowIndex < RR_HIGH_SCORE_VISIBLE_ROWS; scoreEntryOffset += sizeof(struct HighScoreEntry), rowIndex++, rowOffsetY += RR_HIGH_SCORE_ROW_SPACING)
	{
		scoreEntry = (struct HighScoreEntry *)((u8 *)scoreEntries + scoreEntryOffset);
		rowOffsetYCopy = (u16)rowOffsetY;
		flashMask = RR_HIGH_SCORE_FLASH_TIMER_BIT << 1;

		// If this loop index is a new high score
		if ((s8)GAME_TRACKER->newHighScoreIndex == rowIndex)
		{
			// make name color flash every odd frame
			nameColor = (GAME_TRACKER->timer & RR_HIGH_SCORE_FLASH_TIMER_BIT) ? WHITE : scoreEntry->characterID + RR_HIGH_SCORE_DRIVER_COLOR_OFFSET;

			// flash color of time
			timeColorSource = (GAME_TRACKER->timer << 1) & flashMask;
			timeColor = timeColorSource;
		}
		else
		{
			timeColor = 0;
			nameColor = scoreEntry->characterID + RR_HIGH_SCORE_DRIVER_COLOR_OFFSET;
		}

		// Make a rank on the high score list ('1', '2', '3', '4', '5')
		// by taking the binary value of the rank (0, 1, 2, 3, 4),
		// and adding the ascii value of '1'
		currentY = timeboxYBase + rowOffsetYCopy;
		*rankString = (char)rowIndex + '1';

		// Draw String for Rank ('1', '2', '3', '4', '5')
		DecalFont_DrawLine(rankString, iconX + 0x20, currentY - 1, FONT_SMALL, WHITE);

		// Draw Character Icon
		RR_DRAW_POLY_GT4(GAME_TRACKER->ptrIcons[GAME_CHARACTER_METADATA[(s16)scoreEntry->characterID].iconID], iconX, iconYBase + (s16)rowOffsetYCopy,
		                 &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT,
		                 // color of each corner
		                 s_highScoreIconColor223, s_highScoreIconColor223, s_highScoreIconColor223, s_highScoreIconColor223, 1, RR_HIGH_SCORE_ICON_SCALE);

		// Draw Name
		RR_DRAW_LINE_WIDE_X(scoreEntry->name, timeboxX, currentY, FONT_CREDITS, nameColor);

		// Draw time
		RR_DRAW_LINE_WIDE_X(RECTMENU_DrawTime(scoreEntry->time), timeboxX, currentY + 0x11, FONT_SMALL, timeColor);

		// If this loop index is a new high score
		if ((s8)GAME_TRACKER->newHighScoreIndex == rowIndex)
		{
			box.x = iconX - 4;
			highlightY = rowOffsetYCopy - 1;
			box.y = timeboxYBase + highlightY;
			box.w = 0xab;
			box.h = 0x1a;

			// Draw a rectangle to highlight your time on the "Best Times" list
			RR_DRAW_CLEAR_BOX(&box, &GAME_MENU_HIGHLIGHT, TRANS_50_DECAL, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
		}
	}

	if (scoreModeCopy == RR_SCORE_MODE_TIME_TRIAL)
	{
		// Change the way text flickers
		timeColor = JUSTIFY_CENTER | ORANGE;

		// If you got a new best lap
		if (((GAME_TRACKER->gameModeEnd & NEW_BEST_LAP) != 0) && ((GAME_TRACKER->timer & RR_HIGH_SCORE_FLASH_TIMER_BIT) != 0))
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

void RR_EndEvent_DrawMenu(void)
{
	RECT box;
	SVec2 pos;
	SVec2 promptPos;
	s32 rewardBit;

	register s16 textColor CTR_PSX_REGISTER("$19");
	char crateCountText[16];
	char countdownText[24];

	// The screen reports the clock the relic was graded on and the crates the pair
	// picked up between them, so a co-op result reads as one shared attempt.
	struct Driver *driver = MainGameEnd_GradeDriver(GAME_TRACKER);
	UI_RaceEnd_SetMenuLayout(&data.menuRetryExit, &promptPos);

	// change color
	textColor = (GAME_TRACKER->timer & 1) ? (JUSTIFY_CENTER | ORANGE) : (JUSTIFY_CENTER | WHITE);

	rewardBit = GAME_TRACKER->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC;

	// check if platinum is unlocked, set platinum color
	if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, rewardBit))
	{
		RR_RELIC->colorRGBA = RR_PLATINUM_RELIC_COLOR;
	}

	// check if gold is unlocked, set gold color
	else if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, GAME_TRACKER->levelID + ADV_REWARD_FIRST_GOLD_RELIC))
	{
		RR_RELIC->colorRGBA = RR_GOLD_RELIC_COLOR;
	}

	{
		s32 framesSinceRaceEnded;
		struct Instance *timebox1;

		timebox1 = RR_TIMEBOX1;
		framesSinceRaceEnded = GAME_FRAMES_SINCE_RACE_ENDED;

		timebox1->scale.z = RR_TIMEBOX_SCALE;
		timebox1->scale.y = RR_TIMEBOX_SCALE;
		timebox1->scale.x = RR_TIMEBOX_SCALE;

		if (framesSinceRaceEnded < RR_RESULT_MAX_FRAMES)
		{
			GAME_FRAMES_SINCE_RACE_ENDED = framesSinceRaceEnded + 1;
		}
	}

	if (GAME_FRAMES_SINCE_RACE_ENDED >= RR_HIGH_SCORE_REVEAL_FRAME)
	{
		GAME_TRACKER->gameModeEnd |= DRAW_HIGH_SCORES;
	}

	// if race ended 59-80 frames ago and not all crates were collected
	if (((u32)(GAME_FRAMES_SINCE_RACE_ENDED - RR_MISSED_CRATE_SKIP_BASE) < RR_MISSED_CRATE_SKIP_PERFECT_WINDOW) &&
	    !MainGameEnd_GotAllTimeCrates(GAME_TRACKER))
	{
		// advance timer to 140 frames, since we can skip the amount of time
		// that would have been taken to draw "PERFECT" text
		GAME_FRAMES_SINCE_RACE_ENDED = RR_PERFECT_SKIP_FRAME;
	}

	// if race ended 229-250 frames ago, no relic was won, and not all crates were collected
	if (((u32)(GAME_FRAMES_SINCE_RACE_ENDED - RR_MISSED_CRATE_SKIP_BASE) < RR_MISSED_CRATE_SKIP_RELIC_WINDOW) &&
	    ((GAME_TRACKER->gameModeEnd & NEW_RELIC) == 0) && !MainGameEnd_GotAllTimeCrates(GAME_TRACKER))
	{
		// advance timer to 370 frames, since we can skip the amount of time
		// that would have been taken to draw the animation
		// to deduct 10 seconds from the relic timer
		GAME_FRAMES_SINCE_RACE_ENDED = RR_RELIC_SKIP_FRAME;
	}


	// Draw Race Clock,
	// Reset local frame counter
	{
		s32 elapsedFrames;

		elapsedFrames = GAME_FRAMES_SINCE_RACE_ENDED;
		if (elapsedFrames >= RR_FLYOUT_START_FRAME)
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x100, 0x32, 0x100, -0x32, elapsedFrames - RR_FLYOUT_FRAME_OFFSET, RR_LERP_FRAMES);
		}
		else
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), -0x96, 0x32, 0x100, 0x32, elapsedFrames, RR_LERP_FRAMES);
		}

		UI_DrawRaceClock(pos.x, pos.y - 8, UI_RACE_CLOCK_SHOW_RESULTS, driver);
	}


	// Draw Relic,
	// Reset local frame counter
	{
		if ((GAME_TRACKER->gameModeEnd & NEW_RELIC) != 0)
		{
			if (GAME_FRAMES_SINCE_RACE_ENDED >= RR_FLYOUT_START_FRAME)
			{
				UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), (s16)UI_ConvertX_2(0x100, RR_SCREEN_DEPTH), (s16)UI_ConvertY_2(0xa2, RR_SCREEN_DEPTH),
				                 (s16)UI_ConvertX_2(-0x64, RR_SCREEN_DEPTH), (s16)UI_ConvertY_2(0xa2, RR_SCREEN_DEPTH),
				                 GAME_FRAMES_SINCE_RACE_ENDED - RR_FLYOUT_FRAME_OFFSET, RR_LERP_FRAMES);
			}
			else
			{
				if (GAME_FRAMES_SINCE_RACE_ENDED < RR_RELIC_GROW_START_FRAME)
				{
					goto draw_relic_done;
				}

				// on exactly the 251st frame after race ends
				if (GAME_FRAMES_SINCE_RACE_ENDED - 1 == RR_RELIC_AWARD_START_FRAME)
				{
					// play sound of unlocking relic
					OtherFX_Play(RR_RELIC_AWARD_SFX, 1);
				}

				if (RR_RELIC->scale.x < RR_RELIC_FULL_SCALE)
				{
					RR_RELIC->scale.x += RR_RELIC_GROW_STEP;
					RR_RELIC->scale.y += RR_RELIC_GROW_STEP;
					RR_RELIC->scale.z += RR_RELIC_GROW_STEP;
				}

				UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), (s16)UI_ConvertX_2(0x100, RR_SCREEN_DEPTH), (s16)UI_ConvertY_2(0xa2, RR_SCREEN_DEPTH),
				                 (s16)UI_ConvertX_2(0x100, RR_SCREEN_DEPTH), (s16)UI_ConvertY_2(0xa2, RR_SCREEN_DEPTH),
				                 GAME_FRAMES_SINCE_RACE_ENDED - RR_RELIC_AWARD_START_FRAME, RR_LERP_FRAMES);
			}
		}
	}

draw_relic_done:
{
	s32 elapsedFrames;
	b16 beforeFlyout;

	RR_RELIC->matrix.t[0] = pos.x;
	elapsedFrames = GAME_FRAMES_SINCE_RACE_ENDED;
	beforeFlyout = elapsedFrames < RR_FLYOUT_START_FRAME;
	RR_RELIC->matrix.t[1] = pos.y;

	if (!beforeFlyout)
	{
		UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x100, 0x20, 0x100, -0x44, elapsedFrames - RR_FLYOUT_FRAME_OFFSET, RR_LERP_FRAMES);
	}
	else
	{
		UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x28a, 0x20, 0x100, 0x20, elapsedFrames, RR_LERP_FRAMES);
	}
}

	// Draw Time Crates
	// Reset local frame counter
	{
		s32 elapsedFrames;

		elapsedFrames = GAME_FRAMES_SINCE_RACE_ENDED;

		if (elapsedFrames >= RR_FLYOUT_START_FRAME)
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 200, 0x79, 0x264, 0x79, elapsedFrames - RR_FLYOUT_FRAME_OFFSET, RR_LERP_FRAMES);
		}
		else
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 200, 0x79, 200, 0x79, elapsedFrames, RR_LERP_FRAMES);
		}

		RR_TIMEBOX1->matrix.t[0] = UI_ConvertX_2(pos.x, RR_SCREEN_DEPTH);
		RR_TIMEBOX1->matrix.t[1] = UI_ConvertY_2(pos.y, RR_SCREEN_DEPTH);

		DecalFont_DrawLine((char *)&s_timeCrateXString223, pos.x + 0x14, pos.y - 10, 2, 0);
		sprintf(crateCountText, s_crateCountFormat223, MainGameEnd_TimeCratesCollected(GAME_TRACKER), CTR_PRINTF_PSX_LONG(GAME_TRACKER->timeCratesInLEV));
		DecalFont_DrawLine(crateCountText, pos.x + 0x21, pos.y - 0xe, 1, 0);
	}


	// if collected all time boxes in level
	if (MainGameEnd_GotAllTimeCrates(GAME_TRACKER))
	{
		sprintf(countdownText, s_countdownStartFormat223);

		{
			s32 elapsedFrames;

			elapsedFrames = GAME_FRAMES_SINCE_RACE_ENDED;

			if (elapsedFrames >= RR_PERFECT_FLYOUT_OFFSET + RR_PERFECT_START_FRAME)
			{
				UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x100, 0x8a, 0x296, 0x8a, elapsedFrames - (RR_PERFECT_FLYOUT_OFFSET + RR_PERFECT_START_FRAME),
				                 RR_LERP_FRAMES);
				goto draw_perfect;
			}

			if (elapsedFrames >= RR_PERFECT_START_FRAME)
			{
				UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), -0x96, 0x8a, 0x100, 0x8a, elapsedFrames - RR_PERFECT_START_FRAME, RR_LERP_FRAMES);

				if (GAME_FRAMES_SINCE_RACE_ENDED == RR_PERFECT_START_FRAME)
				{
					OtherFX_Play(HOWL_SFX_SWISH, 1);
				}

			draw_perfect:
				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_PERFECT], pos.x, pos.y, FONT_BIG, textColor);
			}
		}

		{
			s32 elapsedFrames;

			elapsedFrames = GAME_FRAMES_SINCE_RACE_ENDED;

			if (elapsedFrames >= RR_FLYOUT_FRAME_OFFSET)
			{
				UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x199, 0x32, 0x199, -0x32, elapsedFrames - RR_COUNTDOWN_START_FRAME, RR_LERP_FRAMES);
			}
			else
			{
				s32 countdownDelta;
				s16 minusSeconds;

				if ((u32)(elapsedFrames - RR_COUNTDOWN_START_FRAME) >= RR_COUNTDOWN_WINDOW_FRAMES)
				{
					goto skip_countdown;
				}

				if (elapsedFrames >= RR_COUNTDOWN_TICK_START_FRAME)
				{
					countdownDelta = RR_COUNTDOWN_TICK_START_FRAME - elapsedFrames;
					minusSeconds = 10 + countdownDelta / RR_COUNTDOWN_STEP_FRAMES;

					if (minusSeconds < 0)
					{
						minusSeconds = 0;
					}
					else if ((minusSeconds != 10) && (countdownDelta == (countdownDelta / RR_COUNTDOWN_STEP_FRAMES) * RR_COUNTDOWN_STEP_FRAMES))
					{
						// The all-crates bonus is the team's, so the visible deduction
						// comes off the clock the screen and high score entry report.
						driver->timeElapsedInRace -= RR_RACE_TIME_ONE_SECOND;
						OtherFX_Play(RR_COUNTDOWN_TICK_SFX, 1);
					}

					sprintf(countdownText, s_countdownFormat223, minusSeconds);
				}

				UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x296, 0x2a, 0x199, 0x2a, GAME_FRAMES_SINCE_RACE_ENDED - RR_COUNTDOWN_START_FRAME, RR_LERP_FRAMES);
			}

			DecalFont_DrawLine(countdownText, pos.x, pos.y, FONT_BIG, textColor);
		}
	}

skip_countdown:

	// Draw RELIC AWARDED
	if (((GAME_TRACKER->gameModeEnd & (NEW_RELIC | NEW_HIGH_SCORE)) == NEW_RELIC) && (GAME_FRAMES_SINCE_RACE_ENDED >= RR_FLYOUT_FRAME_OFFSET))
	{
		UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x100, 0x50, 0x296, 0x50, GAME_FRAMES_SINCE_RACE_ENDED - RR_FLYOUT_FRAME_OFFSET, RR_LERP_FRAMES);
		goto draw_relic_awarded;
	}
	else
	{
		if (((GAME_TRACKER->gameModeEnd & (NEW_RELIC | NEW_HIGH_SCORE)) == (NEW_RELIC | NEW_HIGH_SCORE)) &&
		    (GAME_FRAMES_SINCE_RACE_ENDED >= RR_HIGH_SCORE_BANNER_START_FRAME))
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x100, 0x50, 0x296, 0x50, GAME_FRAMES_SINCE_RACE_ENDED - RR_HIGH_SCORE_BANNER_START_FRAME,
			                 RR_LERP_FRAMES);
			goto draw_relic_awarded;
		}

		if (((GAME_TRACKER->gameModeEnd & NEW_RELIC) != 0) && (GAME_FRAMES_SINCE_RACE_ENDED >= RR_RELIC_AWARD_START_FRAME))
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), -0x96, 0x50, 0x100, 0x50, GAME_FRAMES_SINCE_RACE_ENDED - RR_RELIC_AWARD_START_FRAME, RR_LERP_FRAMES);
			goto draw_relic_awarded;
		}
	}

	goto skip_relic_awarded;

draw_relic_awarded:
	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_RELIC_AWARDED], pos.x, pos.y, FONT_BIG, textColor);

skip_relic_awarded:

	// Draw NEW HIGH SCORE
	if ((GAME_TRACKER->gameModeEnd & NEW_HIGH_SCORE) != 0)
	{
		if (GAME_FRAMES_SINCE_RACE_ENDED >= RR_FLYOUT_FRAME_OFFSET)
		{
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), 0x100, 0x50, 0x296, 0x50, GAME_FRAMES_SINCE_RACE_ENDED - RR_FLYOUT_FRAME_OFFSET, RR_LERP_FRAMES);
		}
		else
		{
			if (GAME_FRAMES_SINCE_RACE_ENDED < RR_HIGH_SCORE_BANNER_START_FRAME)
			{
				goto skip_new_high_score;
			}

			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), -0x96, 0x50, 0x100, 0x50, GAME_FRAMES_SINCE_RACE_ENDED - RR_HIGH_SCORE_BANNER_START_FRAME,
			                 RR_LERP_FRAMES);
			goto draw_new_high_score;
		}

	draw_new_high_score:
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_NEW_HIGH_SCORE], pos.x, pos.y, FONT_BIG, textColor);
		// NOTE(aalhendi): Preserve retail's text-color register lifetime across
		// the control-flow join below.
		CTR_PSX_KEEP_VALUE(textColor);
	}

skip_new_high_score:

	// copy to local frame counter
	{
		s32 elapsedFrames;

		elapsedFrames = GAME_FRAMES_SINCE_RACE_ENDED;

		pos.x = 0;
		pos.y = 0xc;

		// if race ended more than 490 frames ago
		if (elapsedFrames >= RR_FLYOUT_START_FRAME)
		{
			// Interpolate, vertical fly-out
			UI_Lerp2D_Linear(CTR_VECTOR_DATA(&(pos)), -0xa, 0xc, -0xa, -0x58, elapsedFrames - RR_FLYOUT_FRAME_OFFSET, RR_LERP_FRAMES);
		}
	}

	// This is actually a RECT on the stack
	box.x = -0xa;
	box.y = pos.y;
	box.w = 0x214;
	box.h = 0x3b;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&box, 0, GAME_TRACKER->backBuffer->otMem.uiOT);


	if ( // If you have not pressed X to continue
	    ((GAME_MENU_READY & RR_MENU_READY_FLAG) == 0) &&

	    (GAME_FRAMES_SINCE_RACE_ENDED >= RR_HIGH_SCORE_REVEAL_FRAME) &&

	    ((GAME_TRACKER->gameModeEnd & NEW_HIGH_SCORE) == 0))
	{
		RR_EndEvent_DrawHighScore(0x100, 10, RR_SCORE_MODE_RELIC_RACE);

		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_PRESS_TO_CONTINUE], promptPos.x, promptPos.y, FONT_BIG, JUSTIFY_CENTER | ORANGE);

		if ((GAME_ANY_PLAYER_TAP & RR_CONFIRM_BUTTON_MASK) != 0)
		{
			RECTMENU_ClearInput();
			RECTMENU_Show(&data.menuRetryExit);

			// record that you have pressed X to continue
			GAME_MENU_READY |= RR_MENU_READY_FLAG;
		}
	}
}
// NOTE(aalhendi): Retail places this initialized color after the overlay code.
global_variable Color s_highScoreIconColor223 = COLOR_CODE_PACKED_INIT(0x808080);
