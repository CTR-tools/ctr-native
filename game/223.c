#include <common.h>

enum RelicRaceEndMenuConstants
{
	RR_RELIC_TIERS = 3,
	RR_SAPPHIRE_RELIC_INDEX = 0,
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
	RR_PLATINUM_RELIC_COLOR = 0xffede90,
	RR_GOLD_RELIC_COLOR = 0xd8d2090,
	RR_RELIC_AWARD_SFX = 0x67,
	RR_PERFECT_SFX = 0x65,
	RR_COUNTDOWN_TICK_SFX = 99,
	RR_CONFIRM_BUTTON_MASK = BTN_CROSS_one | BTN_CIRCLE,
	RR_MENU_READY_FLAG = 1,
	RR_SCORE_MODE_TIME_TRIAL = 0,
	RR_SCORE_MODE_RELIC_RACE = 1,
	RR_HIGH_SCORE_ENTRIES_PER_MODE = 6,
	RR_HIGH_SCORE_VISIBLE_ROWS = 5,
	RR_HIGH_SCORE_FIRST_VISIBLE_ENTRY = 1,
	RR_HIGH_SCORE_ROW_SPACING = 0x1a,
	RR_HIGH_SCORE_DRIVER_COLOR_OFFSET = 5,
	RR_HIGH_SCORE_FLASH_TIMER_BIT = 2,
	RR_HIGH_SCORE_ICON_COLOR = 0x808080,
	RR_HIGH_SCORE_ICON_SCALE = 0x1000,
};

global_variable s32 s_rankString223 = 0x20;      // " \0"
global_variable s32 s_timeCrateXString223 = 'x'; // "x\0"
global_variable char s_crateCountFormat223[12] = "%2.02d/%ld";
global_variable char s_countdownStartFormat223[4] = "-10";
global_variable char s_countdownFormat223[4] = "-%d";

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009f71c-0x8009fcd0.
void RR_EndEvent_UnlockAward(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver = gGT->drivers[0];
	struct AdvProgress *adv = &sdata->advProgress;
	s32 levelID = gGT->levelID;
	s32 raceTime = driver->timeElapsedInRace;

	// 10 seconds for getting all crates
	if (driver->numTimeCrates == gGT->timeCratesInLEV)
	{
		raceTime -= RR_RACE_TIME_TEN_SECONDS;
	}

	for (s32 relicIndex = 0; relicIndex < RR_RELIC_TIERS; relicIndex++)
	{
		s32 relicTime = data.RelicTime[levelID * RR_RELIC_TIERS + relicIndex];

		// if driver did not beat relic time, check next relic
		if (raceTime > relicTime)
		{
			continue;
		}

		s32 rewardBit = ADV_REWARD_FIRST_SAPPHIRE_RELIC + ADV_REWARD_RELIC_TIER_STRIDE * relicIndex + levelID;

		// if relic already unlocked, check next relic
		if (CHECK_ADV_BIT(adv->rewards, rewardBit) != 0)
		{
			continue;
		}

		// == beat relic, and unlocked relic ==

		// unlock
		UNLOCK_ADV_BIT(adv->rewards, rewardBit);

		// relic model
		gGT->podiumRewardID = STATIC_RELIC;

		// won relic
		gGT->gameModeEnd |= NEW_RELIC;

		// unlocked sapphire
		// do not make this an AND (&&) if statement
		if (relicIndex == RR_SAPPHIRE_RELIC_INDEX)
		{
			if (gGT->levelID == TURBO_TRACK)
			{
				// unlock turbo track
				sdata->gameProgress.unlockFlags |= GAME_UNLOCK_TURBO_TRACK_MASK;
			}

			continue;
		}

		// == Gold or Platinum ==

		// store globally... 8008d9b0
		sdata->relicTime_1min = relicTime / RR_RACE_TIME_ONE_MINUTE;
		sdata->relicTime_10sec = (relicTime / RR_RACE_TIME_TEN_SECONDS) % 6;
		sdata->relicTime_1sec = (relicTime / RR_RACE_TIME_ONE_SECOND) % 10;
		sdata->relicTime_1ms = ((relicTime * 100) / RR_RACE_TIME_ONE_SECOND) % 10;

		// [Not Done]
		sdata->relicTime_10ms = 0;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800a01d8-0x800a0cb8.
void RR_EndEvent_DrawMenu(void)
{
	SVec2 pos;
	s32 elapsedFrames;
	s32 rewardBit;
	u32 textColor;
	RECT box;

	s16 startX;
	s16 endX;
	s16 endY;
	char crateCountText[16];
	char countdownText[24];
	b32 drawCountdown;

	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver = gGT->drivers[0];
	struct Instance *relic = sdata->ptrRelic;
	struct AdvProgress *adv = &sdata->advProgress;

	// testing
	// driver->numTimeCrates = gGT->timeCratesInLEV;

	// change color
	textColor = (gGT->timer & 1) ? 0xffff8000 : 0xffff8004;

	rewardBit = gGT->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC;

	// check if platinum is unlocked, set platinum color
	if (CHECK_ADV_BIT(adv->rewards, rewardBit))
	{
		relic->colorRGBA = RR_PLATINUM_RELIC_COLOR;
	}

	// check if gold is unlocked, set gold color
	else if (CHECK_ADV_BIT(adv->rewards, gGT->levelID + ADV_REWARD_FIRST_GOLD_RELIC))
	{
		relic->colorRGBA = RR_GOLD_RELIC_COLOR;
	}

	sdata->ptrTimebox1->scale = (SVec3){{RR_TIMEBOX_SCALE, RR_TIMEBOX_SCALE, RR_TIMEBOX_SCALE}};

	if (sdata->framesSinceRaceEnded < RR_RESULT_MAX_FRAMES)
	{
		sdata->framesSinceRaceEnded++;
	}

	if (sdata->framesSinceRaceEnded >= RR_HIGH_SCORE_REVEAL_FRAME)
	{
		gGT->gameModeEnd |= DRAW_HIGH_SCORES;
	}

	// Did not get all crates, prepare skips in the menus
	if (driver->numTimeCrates != gGT->timeCratesInLEV)
	{
		// if race ended 59-80 frames ago
		if ((u32)(sdata->framesSinceRaceEnded - RR_MISSED_CRATE_SKIP_BASE) < RR_MISSED_CRATE_SKIP_PERFECT_WINDOW)
		{
			// advance timer to 140 frames, since we can skip the amount of time
			// that would have been taken to draw "PERFECT" text
			sdata->framesSinceRaceEnded = RR_PERFECT_SKIP_FRAME;
		}

		// if race ended 229-250 frames ago, and no relic was won
		if (((gGT->gameModeEnd & NEW_RELIC) == 0) && ((u32)(sdata->framesSinceRaceEnded - RR_MISSED_CRATE_SKIP_BASE) < RR_MISSED_CRATE_SKIP_RELIC_WINDOW))
		{
			// advance timer to 370 frames, since we can skip the amount of time
			// that would have been taken to draw the animation
			// to deduct 10 seconds from the relic timer
			sdata->framesSinceRaceEnded = RR_RELIC_SKIP_FRAME;
		}
	}


	// Draw Race Clock,
	// Reset local frame counter
	elapsedFrames = sdata->framesSinceRaceEnded;
	if (elapsedFrames >= RR_FLYOUT_START_FRAME)
	{
		elapsedFrames -= RR_FLYOUT_FRAME_OFFSET;

		startX = 0x100;
		endY = -0x32;
	}
	else // 0 - 489
	{
		startX = -0x96;
		endY = 0x32;
	}


	// interpolate fly-in
	UI_Lerp2D_Linear(pos.v, startX, 0x32, 0x100, endY, elapsedFrames, RR_LERP_FRAMES);

	UI_DrawRaceClock(pos.x, pos.y - 8, 1, driver);


	// Draw Relic,
	// Reset local frame counter
	elapsedFrames = sdata->framesSinceRaceEnded;

	if ((gGT->gameModeEnd & NEW_RELIC) != 0)
	{
		if (elapsedFrames >= RR_FLYOUT_START_FRAME)
		{
			elapsedFrames -= RR_FLYOUT_FRAME_OFFSET;

			UI_Lerp2D_Linear(pos.v, UI_ConvertX_2(0x100, RR_SCREEN_DEPTH), UI_ConvertY_2(0xa2, RR_SCREEN_DEPTH), UI_ConvertX_2(-0x64, RR_SCREEN_DEPTH),
			                 UI_ConvertY_2(0xa2, RR_SCREEN_DEPTH), elapsedFrames, RR_LERP_FRAMES);
		}

		else if (elapsedFrames >= RR_RELIC_GROW_START_FRAME)
		{
			// on exactly the 251st frame after race ends
			if (elapsedFrames == RR_RELIC_GROW_START_FRAME)
			{
				// play sound of unlocking relic
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800a04cc-0x800a04d4 for relic unlock SFX.
				OtherFX_Play(RR_RELIC_AWARD_SFX, 1);
			}

			if (relic->scale.x < RR_RELIC_FULL_SCALE)
			{
				relic->scale.x += RR_RELIC_GROW_STEP;
				relic->scale.y += RR_RELIC_GROW_STEP;
				relic->scale.z += RR_RELIC_GROW_STEP;
			}

			UI_Lerp2D_Linear(pos.v, UI_ConvertX_2(0x100, RR_SCREEN_DEPTH), UI_ConvertY_2(0xa2, RR_SCREEN_DEPTH), UI_ConvertX_2(0x100, RR_SCREEN_DEPTH),
			                 UI_ConvertY_2(0xa2, RR_SCREEN_DEPTH), elapsedFrames - RR_RELIC_AWARD_START_FRAME, RR_LERP_FRAMES);
		}
	}

	relic->matrix.t[0] = pos.x;
	relic->matrix.t[1] = pos.y;

	// Draw Time Crates
	// Reset local frame counter
	elapsedFrames = sdata->framesSinceRaceEnded;
	{
		if (elapsedFrames >= RR_FLYOUT_START_FRAME)
		{
			elapsedFrames -= RR_FLYOUT_FRAME_OFFSET;

			// interpolate fly-in
			UI_Lerp2D_Linear(pos.v, 200, 0x79, 0x264, 0x79, elapsedFrames, RR_LERP_FRAMES);
		}

		else
		{
			UI_Lerp2D_Linear(pos.v, 200, 0x79, 200, 0x79, elapsedFrames, RR_LERP_FRAMES);
		}

		sdata->ptrTimebox1->matrix.t[0] = UI_ConvertX_2(pos.x, RR_SCREEN_DEPTH);
		sdata->ptrTimebox1->matrix.t[1] = UI_ConvertY_2(pos.y, RR_SCREEN_DEPTH);

		DecalFont_DrawLine((char *)&s_timeCrateXString223, pos.x + 0x14, pos.y - 10, 2, 0);
		sprintf(crateCountText, s_crateCountFormat223, driver->numTimeCrates, CTR_PRINTF_PSX_LONG(gGT->timeCratesInLEV));
		DecalFont_DrawLine(crateCountText, pos.x + 0x21, pos.y - 0xe, 1, 0);
	}


	// if collected all time boxes in level
	if (driver->numTimeCrates == gGT->timeCratesInLEV)
	{
		// copy to local frame counter
		elapsedFrames = sdata->framesSinceRaceEnded;

		// PERFECT text, fade-in and fade-out
		if (elapsedFrames >= RR_PERFECT_START_FRAME)
		{
			elapsedFrames -= RR_PERFECT_START_FRAME;

			// fade-out PERFECT
			// 170 frames after the first 80
			if (elapsedFrames >= RR_PERFECT_FLYOUT_OFFSET)
			{
				startX = 0x100;
				endX = 0x296;
			}

			// === fade-in PERFECT >=80 ===
			else
			{
				startX = -0x96;
				endX = 0x100;

				// 0 frames after the first 80
				if (elapsedFrames == 0)
				{
					// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800a07e8-0x800a07f0 for PERFECT fly-in SFX.
					OtherFX_Play(RR_PERFECT_SFX, 1);
				}
			}

			UI_Lerp2D_Linear(pos.v, startX, 0, endX, 0, elapsedFrames, RR_LERP_FRAMES);

			DecalFont_DrawLine(sdata->lngStrings[LNG_PERFECT], pos.x, 0x8a, 1, textColor);
		}

		// copy to local frame counter
		elapsedFrames = sdata->framesSinceRaceEnded;

		// fade-in COUNTDOWN (-10, -9, -8...)
		if (elapsedFrames >= RR_COUNTDOWN_START_FRAME)
		{
			char *str = countdownText;
			sprintf(str, s_countdownStartFormat223);

			drawCountdown = 0;

			if (elapsedFrames >= RR_FLYOUT_FRAME_OFFSET)
			{
				// interpolate fly-out
				UI_Lerp2D_Linear(pos.v, 0x199, 0x32, 0x199, -0x32, elapsedFrames - RR_COUNTDOWN_START_FRAME, RR_LERP_FRAMES);
				drawCountdown = 1;
			}

			else if ((u32)(elapsedFrames - RR_COUNTDOWN_START_FRAME) < RR_COUNTDOWN_WINDOW_FRAMES)
			{
				// 20 frames after fly-in starts, do the countdown
				if (elapsedFrames >= RR_COUNTDOWN_TICK_START_FRAME)
				{
					s32 countdownDelta = RR_COUNTDOWN_TICK_START_FRAME - elapsedFrames;

					// 10, 9, 8, 7...
					// changes once every 5 frames
					s32 minusSeconds = 10 + (countdownDelta / RR_COUNTDOWN_STEP_FRAMES);

					if (minusSeconds < 0)
					{
						minusSeconds = 0;
					}

					// "if != 10" means "if text is not -10"
					else if ((minusSeconds != 10) && (countdownDelta == ((countdownDelta / RR_COUNTDOWN_STEP_FRAMES) * RR_COUNTDOWN_STEP_FRAMES)))
					{
						// subtract a second
						driver->timeElapsedInRace -= RR_RACE_TIME_ONE_SECOND;
						// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800a08e0-0x800a08f4 for relic countdown tick SFX.
						OtherFX_Play(RR_COUNTDOWN_TICK_SFX, 1);
					}

					sprintf(str, s_countdownFormat223, minusSeconds);
				}

				// interpolate fly-in
				UI_Lerp2D_Linear(pos.v, 0x296, 0x2a, 0x199, 0x2a, elapsedFrames - RR_COUNTDOWN_START_FRAME, RR_LERP_FRAMES);
				drawCountdown = 1;
			}

			// Draw String
			if (drawCountdown)
			{
				DecalFont_DrawLine(str, pos.x, pos.y, 1, textColor);
			}
		}
	}


	// Draw RELIC AWARDED
	// copy to local frame counter
	elapsedFrames = sdata->framesSinceRaceEnded;

	if ((gGT->gameModeEnd & NEW_RELIC) != 0)
	{
		if (((gGT->gameModeEnd & (NEW_RELIC | NEW_HIGH_SCORE)) == NEW_RELIC) && (elapsedFrames >= RR_FLYOUT_FRAME_OFFSET))
		{
			startX = 0x100;
			endX = 0x296;
			elapsedFrames -= RR_FLYOUT_FRAME_OFFSET;
		}

		// Fade-out early, so "NEW HIGH SCORE" can fade-in
		else if (((gGT->gameModeEnd & (NEW_RELIC | NEW_HIGH_SCORE)) == (NEW_RELIC | NEW_HIGH_SCORE)) && (elapsedFrames >= RR_HIGH_SCORE_BANNER_START_FRAME))
		{
			startX = 0x100;
			endX = 0x296;
			elapsedFrames -= RR_HIGH_SCORE_BANNER_START_FRAME;
		}

		// Fade-In
		else if (elapsedFrames >= RR_RELIC_AWARD_START_FRAME)
		{
			startX = -0x96;
			endX = 0x100;
			elapsedFrames -= RR_RELIC_AWARD_START_FRAME;
		}

		else
		{
			goto skipRelicAwarded;
		}

		// interpolate fly-in
		UI_Lerp2D_Linear(pos.v, startX, 0x50, endX, 0x50, elapsedFrames, RR_LERP_FRAMES);

		DecalFont_DrawLine(sdata->lngStrings[LNG_RELIC_AWARDED], pos.x, pos.y, 1, textColor);
	}

skipRelicAwarded:

	// copy to local frame counter
	elapsedFrames = sdata->framesSinceRaceEnded;

	if ((elapsedFrames >= RR_HIGH_SCORE_BANNER_START_FRAME) && ((gGT->gameModeEnd & NEW_HIGH_SCORE) != 0))
	{
		elapsedFrames -= RR_HIGH_SCORE_BANNER_START_FRAME;

		// 4 seconds after the 370 initial frames
		if (elapsedFrames >= RR_HIGH_SCORE_BANNER_HOLD_FRAMES)
		{
			elapsedFrames -= RR_HIGH_SCORE_BANNER_HOLD_FRAMES;

			startX = 0x100;
			endX = 0x296;
		}

		else
		{
			startX = -0x96;
			endX = 0x100;
		}

		// Interpolate fly-in
		UI_Lerp2D_Linear(pos.v, startX, 0x50, endX, 0x50, elapsedFrames, RR_LERP_FRAMES);

		DecalFont_DrawLine(sdata->lngStrings[LNG_NEW_HIGH_SCORE], pos.x, pos.y, 1, textColor);
	}


	// copy to local frame counter
	elapsedFrames = sdata->framesSinceRaceEnded;

	pos.y = 0xc;

	// if race ended more than 490 frames ago
	if (elapsedFrames >= RR_FLYOUT_START_FRAME)
	{
		elapsedFrames -= RR_FLYOUT_FRAME_OFFSET;

		// Interpolate, vertical fly-out
		UI_Lerp2D_Linear(pos.v, -0xa, 0xc, -0xa, -0x58, elapsedFrames, RR_LERP_FRAMES);
	}


	// This is actually a RECT on the stack
	box.x = -0xa;
	box.y = pos.y;
	box.w = 0x214;
	box.h = 0x3b;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&box, 0, gGT->backBuffer->otMem.uiOT);


	if ( // If you have not pressed X to continue
	    ((sdata->menuReadyToPass & RR_MENU_READY_FLAG) == 0) &&

	    (sdata->framesSinceRaceEnded >= RR_HIGH_SCORE_REVEAL_FRAME) &&

	    ((gGT->gameModeEnd & NEW_HIGH_SCORE) == 0))
	{
		RR_EndEvent_DrawHighScore(0x100, 10, RR_SCORE_MODE_RELIC_RACE);

		DecalFont_DrawLine(sdata->lngStrings[LNG_PRESS_TO_CONTINUE], 0x100, 0xbe, 1, 0xffff8000);

		if ((sdata->AnyPlayerTap & RR_CONFIRM_BUTTON_MASK) != 0)
		{
			RECTMENU_ClearInput();
			RECTMENU_Show(&data.menuRetryExit);

			// record that you have pressed X to continue
			sdata->menuReadyToPass |= RR_MENU_READY_FLAG;
		}
	}
}

// same in TT and RR, but not the same in Main Menu
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009fcd0-0x800a01d8.
void RR_EndEvent_DrawHighScore(s16 startX, int startY, s16 scoreMode)
{
	// This is different from High Score in Main Menu because Main Menu
	// does not show the rank icons '1', '2', '3', '4', '5'
	char *timeString;
	u32 timeColor;
	SVec2 pos;
	RECT box;

	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver = gGT->drivers[0];
	s16 timeboxX = startX - 0x1f;
	u16 rowOffsetY = 0;

	// 12 entries per track, 6 for Time Trial and 6 for Relic Race
	struct HighScoreEntry *scoreEntries = &sdata->gameProgress.highScoreTracks[gGT->levelID].scoreEntry[RR_HIGH_SCORE_ENTRIES_PER_MODE * scoreMode];

	// === Naughty Dog Bug ===
	// Start and End are the same

	// interpolate fly-in
	UI_Lerp2D_Linear(pos.v, startX, startY, startX, startY, sdata->framesSinceRaceEnded, RR_LERP_FRAMES);

	DecalFont_DrawLine(sdata->lngStrings[LNG_BEST_TIMES], pos.x, pos.y, 1, 0xffff8000);

	// Draw icon, name, and time of the
	// 5 best times in Time Trial
	for (s32 rowIndex = 0; rowIndex < RR_HIGH_SCORE_VISIBLE_ROWS; rowIndex++)
	{
		s32 scoreEntryIndex = rowIndex + RR_HIGH_SCORE_FIRST_VISIBLE_ENTRY;
		s16 timeboxY = startY + 0x11 + rowOffsetY;

		// default color, not flashing
		timeColor = 0;
		s16 nameColor = scoreEntries[scoreEntryIndex].characterID + RR_HIGH_SCORE_DRIVER_COLOR_OFFSET;

		// If this loop index is a new high score
		if (gGT->newHighScoreIndex == rowIndex)
		{
			// make name color flash every odd frame
			nameColor = (gGT->timer & RR_HIGH_SCORE_FLASH_TIMER_BIT) ? 4 : nameColor;

			// flash color of time
			timeColor = ((gGT->timer & RR_HIGH_SCORE_FLASH_TIMER_BIT) << 1);
		}

		// Make a rank on the high score list ('1', '2', '3', '4', '5')
		// by taking the binary value of the rank (0, 1, 2, 3, 4),
		// and adding the ascii value of '1'
		s_rankString223 = (char)rowIndex + '1';

		// Draw String for Rank ('1', '2', '3', '4', '5')
		DecalFont_DrawLine((char *)&s_rankString223, startX - 0x32, timeboxY - 1, 2, 4);

		// Draw Character Icon
		RECTMENU_DrawPolyGT4(gGT->ptrIcons[data.MetaDataCharacters[scoreEntries[scoreEntryIndex].characterID].iconID], startX - 0x52, timeboxY,

		                     // pointer to PrimMem struct
		                     &gGT->backBuffer->primMem,

		                     // pointer to OT mem
		                     gGT->pushBuffer_UI.ptrOT,

		                     // color of each corner
		                     RR_HIGH_SCORE_ICON_COLOR, RR_HIGH_SCORE_ICON_COLOR, RR_HIGH_SCORE_ICON_COLOR, RR_HIGH_SCORE_ICON_COLOR,

		                     1, RR_HIGH_SCORE_ICON_SCALE);

		// Draw Name
		DecalFont_DrawLine(scoreEntries[scoreEntryIndex].name, timeboxX, timeboxY, 3, nameColor);

		// Draw time
		DecalFont_DrawLine(RECTMENU_DrawTime(scoreEntries[scoreEntryIndex].time), timeboxX, timeboxY + 0x11, 2, timeColor);

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
		rowOffsetY += RR_HIGH_SCORE_ROW_SPACING;
	}

	if (scoreMode == RR_SCORE_MODE_TIME_TRIAL)
	{
		// Change the way text flickers
		timeColor = 0xffff8000;

		DecalFont_DrawLine(sdata->lngStrings[LNG_BEST_LAP], startX, startY + 0x95, 1, timeColor);

		// If you got a new best lap
		if (((gGT->gameModeEnd & NEW_BEST_LAP) != 0) && ((gGT->timer & RR_HIGH_SCORE_FLASH_TIMER_BIT) != 0))
		{
			timeColor = 0xffff8004;
		}

		// make a string for best lap
		timeString = RECTMENU_DrawTime(scoreEntries[0].time);
	}
	else
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_YOUR_TIME], startX, startY + 0x95, 1, 0xffff8000);

		// make a string for your current track time
		timeString = RECTMENU_DrawTime(driver->timeElapsedInRace);

		// color
		timeColor = 0xffff8000;
	}

	// Print amount of time, for whichever purpose
	DecalFont_DrawLine(timeString, startX, startY + 0xa6, 2, timeColor);

	box.x = pos.x - 0x60;
	box.y = pos.y - 4;
	box.w = 0xc0;
	box.h = 0xb4;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&box, 4, gGT->backBuffer->otMem.uiOT);
}
