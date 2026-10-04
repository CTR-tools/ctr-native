#include <common.h>

enum
{
	UI_RACE_CLOCK_EXTENDED_MINUTE_LAP_COUNT = 7,
	UI_RACE_CLOCK_TICKS_PER_SECOND = 0x3c0,
	UI_RACE_CLOCK_TICKS_PER_TEN_SECONDS = 0x2580,
	UI_RACE_CLOCK_TICKS_PER_MINUTE = 0xe100,
	UI_RACE_CLOCK_TICKS_PER_TEN_MINUTES = 0x8ca00,
	UI_RACE_CLOCK_LAP_TIME_SLOTS_PER_PLAYER = 7,
	UI_RACE_CLOCK_DECIMAL_BASE = 10,
	UI_RACE_CLOCK_CENTISECOND_SCALE = 100,
	UI_RACE_CLOCK_SECONDS_TENS_MOD = 6,
	UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT = 9,
	UI_RACE_CLOCK_DEFAULT_SECONDS_TENS = 5,
	UI_RACE_CLOCK_MAX_DISPLAY_DIGIT = 10,
	UI_RACE_CLOCK_RESULTS_TIME_X_OFFSET = 0x11,
	UI_RACE_CLOCK_HUD_TIME_Y_OFFSET = 8,
	UI_RACE_CLOCK_LAP_ROW_Y_STEP = 8,
	UI_RACE_CLOCK_LAP_ROW_Y_OFFSET = 0x10,
	UI_RACE_CLOCK_LAP_TIME_X_OFFSET = 0x1a,
	UI_RACE_CLOCK_RELIC_HUD_LABEL_Y_OFFSET = 0x18,
	UI_RACE_CLOCK_RELIC_HUD_TIME_Y_OFFSET = 0x20,
	UI_RACE_CLOCK_RELIC_RESULTS_Y_OFFSET = 0x11,
};


// used for both finished lap time and current race time
void UI_DrawRaceClock(s16 labelPosX, s16 labelPosY, u32 flags, struct Driver *driver)
{
	struct GameTracker *gGT;
	s16 relicLabelY;
	s16 labelFlags;
	s16 timeColor;
	s16 relicTimeY;
	int stringColor;
	u32 lapIndex;
	char *totalTimeString;
	int iVar5;
	int numParamY;
	int iVar7;
	u32 fontType;

	int timeElapsed;
	int lngIndex;
	int posX;
	int numLaps;

	int strOffset;

	char msOnes;
	u16 relicTimeX;
	s16 *lapTextHeight;
	char msTens;
	char secondsOnes;
	int lapFontType;
	char secondsTens;
	u16 lapOrRelicColor;
	char minutesOnes;
	char minutesTens;
	char lapNumberString[8];

	u16 textPosX;
	u16 textPosY;

	int unbitshiftTextPosX;
	int bitshiftTextPosX;

	gGT = sdata->gGT;

	minutesTens = '\0';

	// if pointer is nullptr
	if (driver == NULL)
	{
		// quit the function
		return;
	}

	// set default time to 99:59:99
	minutesTens = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;
	minutesOnes = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;
	secondsTens = UI_RACE_CLOCK_DEFAULT_SECONDS_TENS;
	secondsOnes = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;
	msTens = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;
	msOnes = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;

	// Race timer units elapsed.
	timeElapsed = driver->timeElapsedInRace;

	if (gGT->numLaps == UI_RACE_CLOCK_EXTENDED_MINUTE_LAP_COUNT)
	{
		// less than 99:59:99
		if (timeElapsed / UI_RACE_CLOCK_TICKS_PER_TEN_MINUTES < UI_RACE_CLOCK_MAX_DISPLAY_DIGIT)
		{
			minutesTens = (char)(timeElapsed / UI_RACE_CLOCK_TICKS_PER_TEN_MINUTES) % UI_RACE_CLOCK_DECIMAL_BASE;

			goto setRestOfTime;
		}
	}

	// if numLaps is not 7
	else
	{
		// less than 9:59:99
		if (timeElapsed / UI_RACE_CLOCK_TICKS_PER_MINUTE < UI_RACE_CLOCK_MAX_DISPLAY_DIGIT)
		{
		setRestOfTime:
			minutesOnes = (char)(timeElapsed / UI_RACE_CLOCK_TICKS_PER_MINUTE) % UI_RACE_CLOCK_DECIMAL_BASE;
			secondsTens = (char)((timeElapsed / UI_RACE_CLOCK_TICKS_PER_TEN_SECONDS) % UI_RACE_CLOCK_SECONDS_TENS_MOD);
			secondsOnes = (char)((timeElapsed / UI_RACE_CLOCK_TICKS_PER_SECOND) % UI_RACE_CLOCK_DECIMAL_BASE);
			msTens = (char)(((timeElapsed * UI_RACE_CLOCK_DECIMAL_BASE) / UI_RACE_CLOCK_TICKS_PER_SECOND) % UI_RACE_CLOCK_DECIMAL_BASE);
			msOnes = (char)(((timeElapsed * UI_RACE_CLOCK_CENTISECOND_SCALE) / UI_RACE_CLOCK_TICKS_PER_SECOND) % UI_RACE_CLOCK_DECIMAL_BASE);
		}
	}

	if ((flags & UI_RACE_CLOCK_SHOW_RESULTS) == 0)
	{
		// TIME
		lngIndex = LNG_TIME;

		// If you're in Time Trial
		if ((gGT->gameMode1 & TIME_TRIAL) != 0)
		{
			// TIME TRIAL
			lngIndex = LNG_TIME_TRIAL;
		}

		// Draw small string
		fontType = FONT_SMALL;

		labelFlags = 0;
	}

	else
	{
		// TOTAL
		lngIndex = LNG_TOTAL;

		// If you're in a Relic Race
		if ((gGT->gameMode1 & RELIC_RACE) != 0)
		{
			// YOUR TIME
			lngIndex = LNG_YOUR_TIME;
		}

		// Draw big string
		fontType = FONT_BIG;

		// Results screens draw the label right-justified and flash it when requested.
		if (((flags & UI_RACE_CLOCK_FLASH_TOTAL) == 0) || (labelFlags = (JUSTIFY_RIGHT | WHITE), (gGT->timer & 2) != 0))
		{
			labelFlags = (JUSTIFY_RIGHT | ORANGE);
		}
	}

	textPosX = labelPosX;
	textPosY = labelPosY;

	DecalFont_DrawLine(sdata->lngStrings[lngIndex], (int)(s16)labelPosX, (int)(s16)labelPosY, fontType, (int)labelFlags);

	// set string to use data.ptrColor[1], which is the periwinkle gradient seen in the LAP text on the HUD
	// particularly used for relic race when the time is frozen
	timeColor = PERIWINKLE;

	if (
	    // if time isn't frozen or we're not in relic race
	    (gGT->frozenTimeRemaining == 0) && (
	                                           // set color to orange if the string shouldn't flash
	                                           timeColor = ORANGE,

	                                           // if the string (which is probably the total time text in the time trial lap results screen) should flash
	                                           (flags & UI_RACE_CLOCK_FLASH_TOTAL) != 0))
	{
		// use timer to change color on even and odd frames
		// timeColor equals either 4 (white) or 0 (orange)
		timeColor = (u16)((gGT->timer & 2) == 0) << 2;
	}

	if (gGT->numLaps == UI_RACE_CLOCK_EXTENDED_MINUTE_LAP_COUNT)
	{
		// String for amount of time in total race
		totalTimeString = rdata.s_timeString_empty;

		// Convert each number from the binary version of the number to the ascii version of the number by adding ascii value of '0'
		totalTimeString[0] = minutesTens + '0';

		strOffset = 1;
	}

	// if number of laps is not 7
	else
	{
		// String for amount of time in lap
		totalTimeString = &sdata->raceClockStr[0];

		strOffset = 0;
	}

	// Convert each number from the binary version of the number to the ascii version of the number by adding ascii value of '0'
	totalTimeString[strOffset + 0] = minutesOnes + '0';
	totalTimeString[strOffset + 2] = secondsTens + '0';
	totalTimeString[strOffset + 3] = secondsOnes + '0';
	totalTimeString[strOffset + 5] = msTens + '0';
	totalTimeString[strOffset + 6] = msOnes + '0';

	// If in-race, reuse X and Y positions used for the TIME/TIME TRIAL text in the top left corner of the HUD for the actual time itself
	// Except set the Y position to be lower
	if ((flags & UI_RACE_CLOCK_SHOW_RESULTS) == 0)
	{
		posX = (int)(s16)textPosX;
		numParamY = ((u32)textPosY + UI_RACE_CLOCK_HUD_TIME_Y_OFFSET) * 0x10000;
	}

	// If in the time trial results screen, reuse X and Y positions used for the rightmost margin of the "TOTAL" text used for displaying the total time, and
	// then adjust them accordingly
	else
	{
		posX = (int)(((u32)textPosX + UI_RACE_CLOCK_RESULTS_TIME_X_OFFSET) * 0x10000) >> 0x10;
		numParamY = (u32)textPosY << 0x10;
	}

	// Draw String
	DecalFont_DrawLine(totalTimeString, posX, numParamY >> 0x10, FONT_BIG, (int)timeColor);

	if (
	    // If you're not in a Relic Race
	    ((gGT->gameMode1 & RELIC_RACE) == 0) || ((flags & UI_RACE_CLOCK_DRAW_LAPS_IN_RELIC) != 0))
	{
		// If you're not in Arcade mode,
		// nor Time Trial, nor adventure mode
		if ((gGT->gameMode1 & (ARCADE_MODE | TIME_TRIAL | ADVENTURE_MODE)) == 0)
		{
			return;
		}

		lapIndex = (u32)driver->lapIndex;
		numLaps = 0;

		if (lapIndex == 0xffffffff)
		{
			return;
		}

		numParamY = 1;
		bitshiftTextPosX = (u32)textPosX << 0x10;
		unbitshiftTextPosX = bitshiftTextPosX >> 0x10;
		do
		{
			if ((numLaps <= (int)lapIndex) && (numLaps < gGT->numLaps))
			{
				UI_SaveLapTime(lapIndex, gGT->elapsedEventTime - driver->lapTime, (u32)driver->driverID);

				// custom code for optimization using this unrelated variable
				iVar5 = (u32)driver->driverID * UI_RACE_CLOCK_LAP_TIME_SLOTS_PER_PLAYER + numLaps;

				// set slot for the tens place of a minute to nothing
				rdata.s_timeString_empty[0] = ' ';

				// Convert each number from the binary version of the number to the ascii version of the number by adding ascii value of '0'
				// This is dynamically programmed to handle time for more than one player, see UI_SaveLapTime

				rdata.s_timeString_empty[1] = GAME_LAP_MINUTE_UNITS[iVar5] + '0';
				rdata.s_timeString_empty[3] = GAME_LAP_SECOND_TENS[iVar5] + '0';
				rdata.s_timeString_empty[4] = GAME_LAP_SECOND_UNITS[iVar5] + '0';
				rdata.s_timeString_empty[6] = GAME_LAP_CENTISECOND_TENS[iVar5] + '0';
				rdata.s_timeString_empty[7] = GAME_LAP_CENTISECOND_UNITS[iVar5] + '0';

				// default
				lapOrRelicColor = PERIWINKLE;

				if (
				    // if this is lap 1, and if lap 1 should flash
				    ((numLaps == 0) && ((flags & UI_RACE_CLOCK_FLASH_LAP_1) != 0)) ||

				    // if this is lap 2, and if lap 2 should flash
				    ((numLaps == 1) && ((flags & UI_RACE_CLOCK_FLASH_LAP_2) != 0)) ||

				    // if this is lap 3, and if lap 3 should flash
				    ((numLaps == 2) && ((flags & UI_RACE_CLOCK_FLASH_LAP_3) != 0)))
				{
					// Change color based on frame counter
					lapOrRelicColor = ((u16)(gGT->timer >> 1) ^ 1) & 1;
				}

				// Otherwise, color is white by default, you can see that in "lapOrRelicColor = 1" near lap 3 check

				if ((flags & UI_RACE_CLOCK_SHOW_RESULTS) == 0)
				{
					// If you're in Arcade Mode
					if ((gGT->gameMode1 & ARCADE_MODE) != 0)
					{
						goto LAB_8004f84c;
					}

					// Set lap number in "Ln" string
					sdata->s_Ln[1] = (char)numLaps + '1';

					iVar7 = (int)(((u32)textPosY + numParamY * UI_RACE_CLOCK_LAP_ROW_Y_STEP + UI_RACE_CLOCK_LAP_ROW_Y_OFFSET) * 0x10000) >> 0x10;

					// draw "Ln" string
					DecalFont_DrawLine(&sdata->s_Ln[0], bitshiftTextPosX >> 0x10, iVar7, FONT_SMALL, RED);

					lapFontType = FONT_SMALL;
					stringColor = PERIWINKLE;
					iVar5 = (int)(((u32)textPosX + UI_RACE_CLOCK_LAP_TIME_X_OFFSET) * 0x10000) >> 0x10;
				}
				else
				{
					// draw big text for time in each lap
					lapFontType = FONT_BIG;

					// if number of laps is more than 3
					if ('\x03' < gGT->numLaps)
					{
						// draw small text for time in each lap
						lapFontType = FONT_SMALL;
					}

					// DAT_8008d510
					// %d

					sprintf(lapNumberString, &sdata->s_int[0], numParamY);
					lapTextHeight = (s16 *)(&data.font_charPixHeight[lapFontType]);

					// draw string
					DecalFont_DrawLine(lapNumberString, unbitshiftTextPosX,
					                   (int)(((u32)textPosY - (gGT->numLaps - numLaps) * (int)*lapTextHeight) * 0x10000) >> 0x10, lapFontType,
					                   (JUSTIFY_RIGHT | RED));

					DecalFont_DrawLine(sdata->lngStrings[LNG_LAP], (int)(((u32)textPosX - (u32)data.font_charPixWidth[lapFontType])),
					                   (int)(((u32)textPosY - (gGT->numLaps - numLaps) * (int)*lapTextHeight) * 0x10000) >> 0x10, lapFontType,
					                   (JUSTIFY_RIGHT | RED));

					stringColor = (int)(s16)lapOrRelicColor;
					iVar7 = (int)(((u32)textPosY - (gGT->numLaps - numLaps) * (int)*lapTextHeight) * 0x10000) >> 0x10;
					iVar5 = unbitshiftTextPosX;
				}

				// draw string for total amount of time in race
				DecalFont_DrawLine(rdata.s_timeString_empty, iVar5, iVar7, lapFontType, stringColor);
			}
		LAB_8004f84c:
			// lap counter
			lapIndex = (u32)driver->lapIndex;

			numLaps = numLaps + 1;
			numParamY = numParamY + 1;
			if ((int)(lapIndex + 1) <= numLaps)
			{
				return;
			}
		} while (1);
	}


	// === Relic Race Only ===

	u32 *rewardsSet = sdata->advProgress.rewards;

	// If did not unlock relic, draw NEXT goal
	if ((gGT->gameModeEnd & NEW_RELIC) == 0)
	{
		// if you have gold or platinum, draw platinum
		if (CHECK_ADV_BIT(rewardsSet, gGT->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC) || CHECK_ADV_BIT(rewardsSet, gGT->levelID + ADV_REWARD_FIRST_GOLD_RELIC))
		{
		DrawPlatinum:
			lngIndex = LNG_PLATINUM;
			lapOrRelicColor = SILVER;
			goto LAB_8004f378;
		}

		// If you have not unlocked Gold or Plat relic on this track

		// if no blue relic, draw blue,
		// if owned blue relic, draw gold
		lapIndex = CHECK_ADV_BIT(rewardsSet, gGT->levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC);
	}

	// Draw (blue,gold,plat) based on which you have unlocked
	else
	{
		// if owned plat, draw plat
		if (CHECK_ADV_BIT(rewardsSet, gGT->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC))
		{
			goto DrawPlatinum;
		}

		// if own gold, draw gold,
		// if own blue, draw blue
		lapIndex = CHECK_ADV_BIT(rewardsSet, gGT->levelID + ADV_REWARD_FIRST_GOLD_RELIC);
	}

	if ((lapIndex & 1) == 0)
	{
		// SAPPHIRE
		lngIndex = LNG_SAPPHIRE;

		// blue color
		lapOrRelicColor = TROPY_LIGHT_BLUE;
	}

	else
	{
		// GOLD
		lngIndex = LNG_GOLD;

		// yellow color
		lapOrRelicColor = PAPU_YELLOW;
	}

LAB_8004f378:
	fontType = FONT_BIG;
	if ((flags & UI_RACE_CLOCK_SHOW_RESULTS) == 0)
	{
		fontType = FONT_SMALL;
		relicLabelY = textPosY + UI_RACE_CLOCK_RELIC_HUD_LABEL_Y_OFFSET;
		relicTimeY = textPosY + UI_RACE_CLOCK_RELIC_HUD_TIME_Y_OFFSET;
		relicTimeX = textPosX;
	}

	else
	{
		lapOrRelicColor = lapOrRelicColor | JUSTIFY_RIGHT;
		relicLabelY = textPosY - UI_RACE_CLOCK_RELIC_RESULTS_Y_OFFSET;
		relicTimeY = relicLabelY;
		relicTimeX = textPosX + UI_RACE_CLOCK_RESULTS_TIME_X_OFFSET;
	}

	DecalFont_DrawLine(sdata->lngStrings[lngIndex], (int)(s16)textPosX, (int)relicLabelY, fontType, (int)(s16)lapOrRelicColor);

	// Convert each number from the binary
	// version of Relic Time to the ascii version
	// of the number by adding ascii value of '0'
	sdata->raceClockStr[0] = sdata->relicTime_1min + '0';
	sdata->raceClockStr[2] = sdata->relicTime_10sec + '0';
	sdata->raceClockStr[3] = sdata->relicTime_1sec + '0';
	sdata->raceClockStr[5] = sdata->relicTime_10ms + '0';
	sdata->raceClockStr[6] = sdata->relicTime_1ms + '0';
	DecalFont_DrawLine(sdata->raceClockStr, (int)(s16)relicTimeX, (int)relicTimeY, FONT_BIG, (int)(s16)(lapOrRelicColor & (0xffff ^ JUSTIFY_RIGHT)));
}

#include "UI_LimitClock.c"
