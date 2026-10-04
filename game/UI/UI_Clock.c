#include <common.h>

enum
{
	UI_RACE_CLOCK_EXTENDED_MINUTE_LAP_COUNT = 7,
	UI_RACE_CLOCK_TICKS_PER_TENTH_SECOND = 96,
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


// Both display ranges use this quotient chain. minuteCarry is an input;
// the remaining digit arguments are output lvalues; centiseconds is local scratch.
// NOTE(aalhendi): Keep quotients wide until the string stores. The hundredfold
// tick product wraps to 32 bits before signed division, as it does on PSX.
// NOTE(aalhendi): Own the decimal reciprocal from the start of the chain;
// making it const folds that early owner away. The final digit temporarily
// owns its quotient before the signed remainder is formed.
#define UI_RACE_CLOCK_SET_REMAINING_DIGITS(elapsed, minuteCarry, minuteUnits, secTens, secUnits, tenths, hundredths)  \
	do                                                                                                                \
	{                                                                                                                 \
		s32 clockMinutes;                                                                                             \
		s32 clockTenSeconds;                                                                                          \
		s32 clockSeconds;                                                                                             \
		s32 clockTenths;                                                                                              \
		s32 clockCentiseconds;                                                                                        \
		s32 clockCentisecondHigh;                                                                                     \
		s32 clockDecimalMultiplier = 0x66666667;                                                                      \
		clockMinutes = (elapsed) / UI_RACE_CLOCK_TICKS_PER_MINUTE;                                                    \
		(minuteUnits) = clockMinutes - (minuteCarry) * UI_RACE_CLOCK_DECIMAL_BASE;                                    \
		clockTenSeconds = (elapsed) / UI_RACE_CLOCK_TICKS_PER_TEN_SECONDS;                                            \
		(secTens) = clockTenSeconds % UI_RACE_CLOCK_SECONDS_TENS_MOD;                                                 \
		clockSeconds = (elapsed) / UI_RACE_CLOCK_TICKS_PER_SECOND;                                                    \
		(secUnits) = clockSeconds % UI_RACE_CLOCK_DECIMAL_BASE;                                                       \
		clockTenths = (elapsed) / UI_RACE_CLOCK_TICKS_PER_TENTH_SECOND;                                               \
		(tenths) = clockTenths % UI_RACE_CLOCK_DECIMAL_BASE;                                                          \
		clockCentiseconds = (s32)((u32)(elapsed) * UI_RACE_CLOCK_CENTISECOND_SCALE) / UI_RACE_CLOCK_TICKS_PER_SECOND; \
		clockCentisecondHigh = CTR_MipsMulHiS(clockCentiseconds, clockDecimalMultiplier);                             \
		(hundredths) = (clockCentisecondHigh >> 2) - (clockCentiseconds >> 31);                                       \
		(hundredths) = clockCentiseconds - (hundredths) * UI_RACE_CLOCK_DECIMAL_BASE;                                 \
	} while (0)


// used for both finished lap time and current race time
void UI_DrawRaceClock(s16 labelPosX, s16 labelPosY, u32 flags, struct Driver *driver)
{
	s16 relicLabelY;
	// NOTE(aalhendi): One packed style word serves each successive text phase.
	s16 textFlags;
	s16 relicTimeY;
	char *totalTimeString;
	u32 fontType;
	u32 *rewardsSet;

	s32 languageIndex;

	s32 msOnes;
	u16 relicTimeX;
	s32 msTens;
	s32 secondsOnes;
	s32 secondsTens;
	s32 minutesOnes;
	s32 minutesTens;

	u16 textPosX;
	u16 textPosY;

	textPosX = labelPosX;
	textPosY = labelPosY;
	minutesTens = 0;

	// if pointer is nullptr
	if (driver == NULL)
	{
		// quit the function
		return;
	}

	if (GAME_TRACKER->numLaps == UI_RACE_CLOCK_EXTENDED_MINUTE_LAP_COUNT)
	{
		s32 timeElapsed;

		timeElapsed = driver->timeElapsedInRace;
		minutesTens = timeElapsed / UI_RACE_CLOCK_TICKS_PER_TEN_MINUTES;
		if (minutesTens >= UI_RACE_CLOCK_MAX_DISPLAY_DIGIT)
		{
			minutesTens = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;
			goto clockSaturated;
		}
		UI_RACE_CLOCK_SET_REMAINING_DIGITS(timeElapsed, minutesTens, minutesOnes, secondsTens, secondsOnes, msTens, msOnes);
	}
	else
	{
		s32 timeElapsed;

		timeElapsed = driver->timeElapsedInRace;
		minutesOnes = timeElapsed / UI_RACE_CLOCK_TICKS_PER_MINUTE;
		if (minutesOnes >= UI_RACE_CLOCK_MAX_DISPLAY_DIGIT)
		{
		clockSaturated:
			// Saturated display: 9:59:99, or 99:59:99 with the separate minute carry.
			minutesOnes = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;
			secondsTens = UI_RACE_CLOCK_DEFAULT_SECONDS_TENS;
			secondsOnes = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;
			msTens = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;
			msOnes = UI_RACE_CLOCK_DEFAULT_MINUTE_DIGIT;
			goto clockDigitsReady;
		}
		UI_RACE_CLOCK_SET_REMAINING_DIGITS(timeElapsed, minutesTens, minutesOnes, secondsTens, secondsOnes, msTens, msOnes);
	}
clockDigitsReady:

	if ((flags & UI_RACE_CLOCK_SHOW_RESULTS) != 0)
	{
		// TOTAL
		languageIndex = LNG_TOTAL;

		// If you're in a Relic Race
		if ((GAME_TRACKER->gameMode1 & RELIC_RACE) != 0)
		{
			// YOUR TIME
			languageIndex = LNG_YOUR_TIME;
		}

		// Draw big string
		fontType = FONT_BIG;

		// Results screens draw the label right-justified and flash it when requested.
		if (((flags & UI_RACE_CLOCK_FLASH_TOTAL) == 0) || (textFlags = (JUSTIFY_RIGHT | WHITE), (GAME_TRACKER->timer & 2) != 0))
		{
			textFlags = (JUSTIFY_RIGHT | ORANGE);
		}
	}
	else
	{
		// TIME
		languageIndex = LNG_TIME;

		// If you're in Time Trial
		if ((GAME_TRACKER->gameMode1 & TIME_TRIAL) != 0)
		{
			// TIME TRIAL
			languageIndex = LNG_TIME_TRIAL;
		}

		// Draw small string
		fontType = FONT_SMALL;

		textFlags = 0;
	}

	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[languageIndex], (s32)(s16)textPosX, (s32)(s16)textPosY, fontType, (s32)textFlags);

	// set string to use data.ptrColor[1], which is the periwinkle gradient seen in the LAP text on the HUD
	// particularly used for relic race when the time is frozen
	textFlags = PERIWINKLE;

	if (
	    // if time isn't frozen or we're not in relic race
	    (GAME_TRACKER->frozenTimeRemaining == 0) &&
	    (
	        // set color to orange if the string shouldn't flash
	        textFlags = ORANGE,

	        // if the string (which is probably the total time text in the time trial lap results screen) should flash
	        (flags & UI_RACE_CLOCK_FLASH_TOTAL) != 0))
	{
		// use timer to change color on even and odd frames
		// textFlags equals either 4 (white) or 0 (orange)
		textFlags = (u16)((GAME_TRACKER->timer & 2) == 0) << 2;
	}

	// Both writable strings retain their separators; only the digit slots differ.
#define UI_RACE_CLOCK_WRITE_SECOND_DIGITS(buffer, firstDigit) \
	do                                                        \
	{                                                         \
		(buffer)[(firstDigit) + 0] = secondsTens + '0';       \
		(buffer)[(firstDigit) + 1] = secondsOnes + '0';       \
		(buffer)[(firstDigit) + 3] = msTens + '0';            \
		(buffer)[(firstDigit) + 4] = msOnes + '0';            \
	} while (0)

	if (GAME_TRACKER->numLaps == UI_RACE_CLOCK_EXTENDED_MINUTE_LAP_COUNT)
	{
		// String for amount of time in total race
		GAME_RDATA_NAME(s_timeString_empty)[0] = minutesTens + '0';
		totalTimeString = GAME_RDATA_NAME(s_timeString_empty);
		totalTimeString[1] = minutesOnes + '0';
		UI_RACE_CLOCK_WRITE_SECOND_DIGITS(totalTimeString, 3);
	}

	// if number of laps is not 7
	else
	{
		// String for amount of time in lap
		GAME_SDATA_NAME(raceClockStr)[0] = minutesOnes + '0';
		totalTimeString = GAME_SDATA_NAME(raceClockStr);
		UI_RACE_CLOCK_WRITE_SECOND_DIGITS(totalTimeString, 2);
	}

#undef UI_RACE_CLOCK_WRITE_SECOND_DIGITS

	// If in the time trial results screen, reuse X and Y positions used for the rightmost margin of the "TOTAL" text used for displaying the total time, and
	// then adjust them accordingly
	if ((flags & UI_RACE_CLOCK_SHOW_RESULTS) != 0)
	{
		DecalFont_DrawLine(totalTimeString, (s16)(textPosX + UI_RACE_CLOCK_RESULTS_TIME_X_OFFSET), (s16)textPosY, FONT_BIG, textFlags);
	}

	// If in-race, reuse X and Y positions used for the TIME/TIME TRIAL text in the top left corner of the HUD for the actual time itself
	// Except set the Y position to be lower
	else
	{
		DecalFont_DrawLine(totalTimeString, (s16)textPosX, (s16)(textPosY + UI_RACE_CLOCK_HUD_TIME_Y_OFFSET), FONT_BIG, textFlags);
	}

	if (((GAME_TRACKER->gameMode1 & RELIC_RACE) != 0) && ((flags & UI_RACE_CLOCK_DRAW_LAPS_IN_RELIC) == 0))
	{
		s32 relicFirstChar;
		s32 relicSecondChar;
		char *relicTimeString;

		// === Relic Race Only ===

		// If did not unlock relic, draw NEXT goal
		if ((GAME_TRACKER->gameModeEnd & NEW_RELIC) == 0)
		{
			rewardsSet = GAME_ADV_PROGRESS.rewards;

			// if you have gold or platinum, draw platinum
			if (CHECK_ADV_BIT(rewardsSet, GAME_TRACKER->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC) ||
			    CHECK_ADV_BIT(rewardsSet, GAME_TRACKER->levelID + ADV_REWARD_FIRST_GOLD_RELIC))
			{
				goto DrawPlatinum;
			}

			// If you have not unlocked Gold or Plat relic on this track

			// if no blue relic, draw blue,
			// if owned blue relic, draw gold
			if (CHECK_ADV_BIT(rewardsSet, GAME_TRACKER->levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC))
			{
				goto DrawGold;
			}
			goto DrawSapphire;
		}

		// Draw (blue,gold,plat) based on which you have unlocked
		else
		{
			rewardsSet = GAME_ADV_PROGRESS.rewards;

			// if owned plat, draw plat
			if (CHECK_ADV_BIT(rewardsSet, GAME_TRACKER->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC))
			{
			DrawPlatinum:
				languageIndex = LNG_PLATINUM;
				textFlags = SILVER;
				goto relicRewardSelected;
			}

			// if own gold, draw gold,
			// if own blue, draw blue
			if (!CHECK_ADV_BIT(rewardsSet, GAME_TRACKER->levelID + ADV_REWARD_FIRST_GOLD_RELIC))
			{
				goto DrawSapphire;
			}
		}

	DrawGold:
		languageIndex = LNG_GOLD;
		textFlags = PAPU_YELLOW;
		goto relicRewardSelected;

	DrawSapphire:
		languageIndex = LNG_SAPPHIRE;
		textFlags = TROPY_LIGHT_BLUE;

	relicRewardSelected:
		if ((flags & UI_RACE_CLOCK_SHOW_RESULTS) != 0)
		{
			fontType = FONT_BIG;
			textFlags = textFlags | JUSTIFY_RIGHT;
			relicLabelY = textPosY - UI_RACE_CLOCK_RELIC_RESULTS_Y_OFFSET;
			relicTimeX = textPosX + UI_RACE_CLOCK_RESULTS_TIME_X_OFFSET;
			relicTimeY = relicLabelY;
		}

		else
		{
			fontType = FONT_SMALL;
			relicLabelY = textPosY + UI_RACE_CLOCK_RELIC_HUD_LABEL_Y_OFFSET;
			relicTimeY = textPosY + UI_RACE_CLOCK_RELIC_HUD_TIME_Y_OFFSET;
			relicTimeX = textPosX;
		}

		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[languageIndex], (s32)(s16)textPosX, (s32)relicLabelY, fontType, (s32)(s16)textFlags);

		// NOTE(aalhendi): Keep converted characters until their stores. The
		// source fields are signed words, but retail reads only their low bytes.
		relicFirstChar = (u8)GAME_SDATA_NAME(relicTime_1min) + '0';
		relicSecondChar = (u8)GAME_SDATA_NAME(relicTime_10sec) + '0';
		GAME_SDATA_NAME(raceClockStr)[0] = relicFirstChar;
		relicTimeString = GAME_SDATA_NAME(raceClockStr);
		relicFirstChar = (u8)GAME_SDATA_NAME(relicTime_1sec) + '0';
		relicTimeString[2] = relicSecondChar;
		relicTimeString[3] = relicFirstChar;
		relicSecondChar = (u8)GAME_SDATA_NAME(relicTime_10ms) + '0';
		relicFirstChar = (u8)GAME_SDATA_NAME(relicTime_1ms) + '0';
		relicTimeString[5] = relicSecondChar;
		relicTimeString[6] = relicFirstChar;
		DecalFont_DrawLine(relicTimeString, (s32)(s16)relicTimeX, (s32)relicTimeY, FONT_BIG, (s32)(s16)(textFlags & (0xffff ^ JUSTIFY_RIGHT)));
	}
	else
	{
		char *lapTimeString;
		char *lapNumberText;
		const s32 *lapSecondUnits;
		const s32 *lapSecondDigit;
		// NOTE(aalhendi): Retail holds lap flags in s3 and row Y in s0 across
		// font calls. These are local allocation choices, not a caller ABI.
		register s32 lapTextFlags CTR_PSX_REGISTER("$19");
		u32 lapIndex;
		s32 lapTimeX;
		s32 lapNumber;
		register s32 lapRowY CTR_PSX_REGISTER("$16");
		s32 lapRow;
		s16 *lapTextHeight;
		s32 lapFontType;
		char lapNumberString[8];
		s32 lapLabelX;

		// If you're not in a Relic Race
		// If you're not in Arcade mode,
		// nor Time Trial, nor adventure mode
		if ((GAME_TRACKER->gameMode1 & (ARCADE_MODE | TIME_TRIAL | ADVENTURE_MODE)) == 0)
		{
			return;
		}

		lapIndex = (u32)driver->lapIndex;
		lapRow = 0;

		if (lapIndex + 1 == 0)
		{
			return;
		}

		lapNumberText = lapNumberString;
		lapNumber = 1;
		lapLabelX = (s16)textPosX;
		lapSecondUnits = GAME_LAP_SECOND_UNITS;
		do
		{
			if ((lapRow <= (s32)lapIndex) && (lapRow < GAME_TRACKER->numLaps))
			{
				char *lapLabelText = GAME_SDATA_NAME(s_Ln);
#ifdef CTR_NATIVE
				struct GameTracker *gGT = GAME_TRACKER;

				// NOTE(aalhendi): A finished counter equals the lap count, not a live
				// lap slot. Native preserves the saved digits instead of writing
				// the next slot (out of bounds after P2's seventh lap).
				if (lapIndex < (u32)gGT->numLaps)
#endif
				{
					UI_SaveLapTime(lapIndex, GAME_TRACKER->elapsedEventTime - driver->lapTime, (u32)driver->driverID);
				}

				// set slot for the tens place of a minute to nothing
				GAME_RDATA_NAME(s_timeString_empty)[0] = ' ';
				lapTimeString = GAME_RDATA_NAME(s_timeString_empty);

				// Convert each number from the binary version of the number to the ascii version of the number by adding ascii value of '0'
				// This is dynamically programmed to handle time for more than one player, see UI_SaveLapTime

				// NOTE(aalhendi): Observe the live driver slot after each shared string store.
				lapTimeString[1] = (GAME_LAP_MINUTE_UNITS + lapRow)[(u32)driver->driverID * UI_RACE_CLOCK_LAP_TIME_SLOTS_PER_PLAYER] + '0';
				lapTimeString[3] = (GAME_LAP_SECOND_TENS + lapRow)[(u32)driver->driverID * UI_RACE_CLOCK_LAP_TIME_SLOTS_PER_PLAYER] + '0';
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(lapSecondDigit, lapSecondUnits,
				                                        (u32)lapRow * sizeof(*lapSecondUnits) +
				                                            (u32)driver->driverID * UI_RACE_CLOCK_LAP_TIME_SLOTS_PER_PLAYER * sizeof(*lapSecondUnits));
				lapTimeString[4] = *lapSecondDigit + '0';
				lapTimeString[6] = (GAME_LAP_CENTISECOND_TENS + lapRow)[(u32)driver->driverID * UI_RACE_CLOCK_LAP_TIME_SLOTS_PER_PLAYER] + '0';
				lapTimeString[7] = (GAME_LAP_CENTISECOND_UNITS + lapRow)[(u32)driver->driverID * UI_RACE_CLOCK_LAP_TIME_SLOTS_PER_PLAYER] + '0';

				if (
				    // if this is lap 1, and if lap 1 should flash
				    ((lapRow == 0) && ((flags & UI_RACE_CLOCK_FLASH_LAP_1) != 0)) ||

				    // if this is lap 2, and if lap 2 should flash
				    ((lapRow == 1) && ((flags & UI_RACE_CLOCK_FLASH_LAP_2) != 0)) ||

				    // if this is lap 3, and if lap 3 should flash
				    (textFlags = PERIWINKLE, ((lapRow == 2) && ((flags & UI_RACE_CLOCK_FLASH_LAP_3) != 0))))
				{
					// Change color based on frame counter
					textFlags = ((u16)((u32)GAME_TRACKER->timer >> 1) ^ 1) & 1;
				}

				if ((flags & UI_RACE_CLOCK_SHOW_RESULTS) != 0)
				{
					// draw big text for time in each lap
					lapFontType = FONT_BIG;

					// if number of laps is more than 3
					if ('\x03' < GAME_TRACKER->numLaps)
					{
						// draw small text for time in each lap
						lapFontType = FONT_SMALL;
					}

					// DAT_8008d510
					// %d

					sprintf(lapNumberText, GAME_SDATA_NAME(s_int), lapNumber);
					lapTextHeight = GAME_DATA_NAME(font_charPixHeight);
					lapTextHeight += lapFontType;
					lapTextFlags = JUSTIFY_RIGHT | RED;

					// draw string
					DecalFont_DrawLine(lapNumberText, lapLabelX, (s16)(textPosY - (GAME_TRACKER->numLaps - lapRow) * (s32)*lapTextHeight), lapFontType,
					                   lapTextFlags);

					DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_LAP], (s32)(((u32)textPosX - (u32)GAME_DATA_NAME(font_charPixWidth)[lapFontType])),
					                   (s16)(textPosY - (GAME_TRACKER->numLaps - lapRow) * (s32)*lapTextHeight), lapFontType, lapTextFlags);

					lapTextFlags = (s32)(s16)textFlags;
					lapRowY = (s16)(textPosY - (GAME_TRACKER->numLaps - lapRow) * (s32)*lapTextHeight);
					lapTimeX = lapLabelX;

					// draw string for total amount of time in race
					DecalFont_DrawLine(lapTimeString, lapTimeX, lapRowY, lapFontType, lapTextFlags);
				}
				else
				{
					// If you're in Arcade Mode
					if ((GAME_TRACKER->gameMode1 & ARCADE_MODE) != 0)
					{
						goto nextLapRow;
					}

					// Set lap number in "Ln" string
					lapLabelText[1] = (char)lapRow + '1';

					lapRowY = lapNumber * UI_RACE_CLOCK_LAP_ROW_Y_STEP + UI_RACE_CLOCK_LAP_ROW_Y_OFFSET;
					lapRowY = (s16)((u32)textPosY + (u32)lapRowY);

					// draw "Ln" string
					DecalFont_DrawLine(lapLabelText, lapLabelX, lapRowY, FONT_SMALL, RED);

					lapFontType = FONT_SMALL;
					lapTimeX = (s16)((u32)textPosX + UI_RACE_CLOCK_LAP_TIME_X_OFFSET);

					// draw string for total amount of time in race
					DecalFont_DrawLine(lapTimeString, lapTimeX, lapRowY, lapFontType, PERIWINKLE);
				}
			}
		nextLapRow:
			// lap counter
			lapIndex = (u32)driver->lapIndex;

			lapRow = lapRow + 1;
			lapNumber = lapNumber + 1;
			if ((s32)(lapIndex + 1) <= lapRow)
			{
				return;
			}
		} while (1);
	}
}

#undef UI_RACE_CLOCK_SET_REMAINING_DIGITS
