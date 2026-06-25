#include <common.h>

// used for both finished lap time and current race time
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004edac-0x8004f894
void UI_DrawRaceClock(u16 paramX, u16 paramY, u32 flags, struct Driver *driver)
{
	// flag parameter bits
	// despite being a u32 only 6 bits are ever used
	// 0: Display in-race time (if 0) / Display time trial results (if 1)
	// 1: Enable or disable flashing
	// 2: Only used to check if you're in a relic race
	// 3: Only used to check if lap 1 should flash (which is when it's the fastest lap)
	// 4: Only used to check if lap 2 should flash (which is when it's the fastest lap)
	// 5: Only used to check if lap 3 should flash (which is when it's the fastest lap)
	struct GameTracker *gGT;
	s16 sVar1;
	int stringColor;
	u32 lapIndex;
	u32 uVar3;
	u8 *totalTimeString;
	int iVar5;
	int numParamY;
	int iVar7;
	u32 fontType;

	int msElapsed;
	int str;
	int posX;
	int numLaps;
	int levID;

	int strOffset;

	char msOnes;
	u16 uVar11;
	s16 *lapTextHeight;
	char msTens;
	char secondsOnes;
	int lapFontType;
	char secondsTens;
	s16 strFlags_but_its_also_posY;
	u16 stringColor_but_its_also_relicColor;
	char minutesOnes;
	char minutesTens;
	char acStack80[8];

	u16 textPosX;
	u16 textPosY;

	char *local_38;
	int unbitshiftTextPosX;
	int bitshiftTextPosX;

	gGT = sdata->gGT;

	minutesTens = '\0';

	// if pointer is nullptr
	if (driver == 0)
	{
		// quit the function
		return;
	}

	// set default time to 99:59:99
	minutesTens = 9;
	minutesOnes = 9;
	secondsTens = 5;
	secondsOnes = 9;
	msTens = 9;
	msOnes = 9;

	// milliseconds elapsed in race
	msElapsed = driver->timeElapsedInRace;

	if (gGT->numLaps == 7)
	{
		// less than 99:59:99
		if (msElapsed / 0x8ca00 < 10)
		{
			minutesTens = (char)(msElapsed / 0x8ca00) % 10;

			goto setRestOfTime;
		}
	}

	// if numLaps is not 7
	else
	{
		// less than 9:59:99
		if (msElapsed / 0xe100 < 10)
		{
		setRestOfTime:
			minutesOnes = (char)(msElapsed / 0xe100) % 10;
			secondsTens = (char)((msElapsed / 0x2580) % 6);
			secondsOnes = (char)((msElapsed / 0x3c0) % 10);
			msTens = (char)(((msElapsed * 10) / 0x3c0) % 10);
			msOnes = (char)(((msElapsed * 100) / 0x3c0) % 10);
		}
	}

	if ((flags & 1) == 0)
	{
		// TIME
		str = 0x12;

		// If you're in Time Trial
		if ((gGT->gameMode1 & TIME_TRIAL) != 0)
		{
			// TIME TRIAL
			str = 0x4d;
		}

		// Draw small string
		fontType = FONT_SMALL;

		strFlags_but_its_also_posY = 0;
	}

	else
	{
		// TOTAL
		str = 0xc4;

		// If you're in a Relic Race
		if ((gGT->gameMode1 & RELIC_RACE) != 0)
		{
			// YOUR TIME
			str = 0xc5;
		}

		// Draw big string
		fontType = FONT_BIG;

		// this particular snippet is for drawing the TOTAL text in the time trial lap results screen before it flashes, at which point it moves from the left
		// to the center of the screen if global game timer is on an odd number (in which case this condition doesn't execute) then set string to end at posX of
		// next DrawLine and use data.ptrColor[WHITE]
		if (((flags & 4) == 0) || (strFlags_but_its_also_posY = (JUSTIFY_RIGHT | WHITE), (gGT->timer & 2) != 0))
		{
			strFlags_but_its_also_posY = (JUSTIFY_RIGHT | ORANGE);
		}
	}

	textPosX = paramX;
	textPosY = paramY;

	// str = 0x12: TIME
	// str = 0x4d: TIME TRIAL
	// str = 0xc4: TOTAL
	// str = 0xc5: YOUR TIME
	DecalFont_DrawLine(sdata->lngStrings[str], (int)(s16)paramX, (int)(s16)paramY, fontType, (int)strFlags_but_its_also_posY);

	// set string to use data.ptrColor[1], which is the periwinkle gradient seen in the LAP text on the HUD
	// particularly used for relic race when the time is frozen
	strFlags_but_its_also_posY = PERIWINKLE;

	if (
	    // if time isn't frozen or we're not in relic race
	    (gGT->frozenTimeRemaining == 0) && (
	                                           // set color to orange if the string shouldn't flash
	                                           strFlags_but_its_also_posY = ORANGE,

	                                           // if the string (which is probably the total time text in the time trial lap results screen) should flash
	                                           (flags & 4) != 0))
	{
		// use timer to change color on even and odd frames
		// strFlags_but_its_also_posY equals either 4 (white) or 0 (orange)
		strFlags_but_its_also_posY = (u16)((gGT->timer & 2) == 0) << 2;
	}

	if (gGT->numLaps == 7)
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
	if ((flags & 1) == 0)
	{
		posX = (int)(s16)textPosX;
		numParamY = ((u32)textPosY + 8) * 0x10000;
	}

	// If in the time trial results screen, reuse X and Y positions used for the rightmost margin of the "TOTAL" text used for displaying the total time, and
	// then adjust them accordingly
	else
	{
		posX = (int)(((u32)textPosX + 0x11) * 0x10000) >> 0x10;
		numParamY = (u32)textPosY << 0x10;
	}

	// Draw String
	DecalFont_DrawLine(totalTimeString, posX, numParamY >> 0x10, FONT_BIG, (int)strFlags_but_its_also_posY);

	if (
	    // If you're not in a Relic Race
	    ((gGT->gameMode1 & RELIC_RACE) == 0) || ((flags & 2) != 0))
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

		local_38 = acStack80;
		numParamY = 1;
		bitshiftTextPosX = (u32)textPosX << 0x10;
		unbitshiftTextPosX = bitshiftTextPosX >> 0x10;
		do
		{
			if ((numLaps <= (int)lapIndex) && (numLaps < (char)gGT->numLaps))
			{
				UI_SaveLapTime(lapIndex, gGT->elapsedEventTime - driver->lapTime, (u32)driver->driverID);

				// custom code for optimization using this unrelated variable
				iVar5 = (u32)driver->driverID * 7 + numLaps;

				// set slot for the tens place of a minute to nothing
				rdata.s_timeString_empty[0] = ' ';

				// Convert each number from the binary version of the number to the ascii version of the number by adding ascii value of '0'
				// This is dynamically programmed to handle time for more than one player, see UI_SaveLapTime

				rdata.s_timeString_empty[1] = sdata->LapTimes.p1_Min1s[iVar5] + '0';
				rdata.s_timeString_empty[3] = sdata->LapTimes.p1_Sec10s[iVar5] + '0';
				rdata.s_timeString_empty[4] = sdata->LapTimes.p1_Sec1s[iVar5] + '0';
				rdata.s_timeString_empty[6] = sdata->LapTimes.p1_Ms10s[iVar5] + '0';
				rdata.s_timeString_empty[7] = sdata->LapTimes.p1_Ms1s[iVar5] + '0';

				// default
				stringColor_but_its_also_relicColor = PERIWINKLE;

				if (
				    // if this is lap 1, and if lap 1 should flash
				    ((numLaps == 0) && ((flags & 8) != 0)) ||

				    // if this is lap 2, and if lap 2 should flash
				    ((numLaps == 1) && ((flags & 0x10) != 0)) ||

				    // if this is lap 3, and if lap 3 should flash
				    ((numLaps == 2) && ((flags & 0x20) != 0)))
				{
					// Change color based on frame counter
					stringColor_but_its_also_relicColor = ((u16)(gGT->timer >> 1) ^ 1) & 1;
				}

				// Otherwise, color is white by default, you can see that in "stringColor_but_its_also_relicColor = 1" near lap 3 check

				if ((flags & 1) == 0)
				{
					// If you're in Arcade Mode
					if ((gGT->gameMode1 & ARCADE_MODE) != 0)
					{
						goto LAB_8004f84c;
					}

					// Set lap number in "Ln" string
					sdata->s_Ln[1] = (char)numLaps + '1';

					iVar7 = (int)(((u32)textPosY + numParamY * 8 + 0x10) * 0x10000) >> 0x10;

					// draw "Ln" string
					DecalFont_DrawLine(&sdata->s_Ln[0], bitshiftTextPosX >> 0x10, iVar7, FONT_SMALL, RED);

					lapFontType = FONT_SMALL;
					stringColor = PERIWINKLE;
					iVar5 = (int)(((u32)textPosX + 0x1a) * 0x10000) >> 0x10;
				}
				else
				{
					// draw big text for time in each lap
					lapFontType = FONT_BIG;

					// if number of laps is more than 3
					if ('\x03' < (char)gGT->numLaps)
					{
						// draw small text for time in each lap
						lapFontType = FONT_SMALL;
					}

					// DAT_8008d510
					// %d

					sprintf(local_38, &sdata->s_int[0], numParamY);
					lapTextHeight = (s16 *)(&data.font_charPixHeight[lapFontType]);

					// draw string
					DecalFont_DrawLine(local_38, unbitshiftTextPosX,
					                   (int)(((u32)textPosY - ((char)gGT->numLaps - numLaps) * (int)*lapTextHeight) * 0x10000) >> 0x10, lapFontType,
					                   (JUSTIFY_RIGHT | RED));

					DecalFont_DrawLine(sdata->lngStrings[LNG_LAP], (int)(((u32)textPosX - (u32)data.font_charPixWidth[lapFontType])),
					                   (int)(((u32)textPosY - ((char)gGT->numLaps - numLaps) * (int)*lapTextHeight) * 0x10000) >> 0x10, lapFontType,
					                   (JUSTIFY_RIGHT | RED));

					stringColor = (int)(s16)stringColor_but_its_also_relicColor;
					iVar7 = (int)(((u32)textPosY - ((char)gGT->numLaps - numLaps) * (int)*lapTextHeight) * 0x10000) >> 0x10;
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
		// Level ID
		levID = gGT->levelID;

		// if you have gold or platinum, draw platinum
		if ((CHECK_ADV_BIT(rewardsSet, gGT->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC) != 0) ||
		    (CHECK_ADV_BIT(rewardsSet, gGT->levelID + ADV_REWARD_FIRST_GOLD_RELIC) != 0))
		{
		DrawPlatinum:
			str = 200;
			stringColor_but_its_also_relicColor = SILVER;
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
		if (CHECK_ADV_BIT(rewardsSet, gGT->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC) != 0)
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
		str = 0xc6;

		// blue color
		stringColor_but_its_also_relicColor = TROPY_LIGHT_BLUE;
	}

	else
	{
		// GOLD
		str = 199;

		// yellow color
		stringColor_but_its_also_relicColor = PAPU_YELLOW;
	}

LAB_8004f378:
	fontType = FONT_BIG;
	if ((flags & 1) == 0)
	{
		fontType = FONT_SMALL;
		sVar1 = textPosY + 0x18;
		strFlags_but_its_also_posY = textPosY + 0x20;
		uVar11 = textPosX;
	}

	else
	{
		stringColor_but_its_also_relicColor = stringColor_but_its_also_relicColor | JUSTIFY_RIGHT;
		sVar1 = textPosY - 0x11;
		strFlags_but_its_also_posY = sVar1;
		uVar11 = textPosX + 0x11;
	}

	// str 0xc6: SAPPHIRE
	// str 199 (c7): GOLD
	// str 0xc8: PLATINUM
	DecalFont_DrawLine(sdata->lngStrings[str], (int)(s16)textPosX, (int)sVar1, fontType, (int)(s16)stringColor_but_its_also_relicColor);

	// Convert each number from the binary
	// version of Relic Time to the ascii version
	// of the number by adding ascii value of '0'
	sdata->raceClockStr[0] = sdata->relicTime_1min + '0';
	sdata->raceClockStr[2] = sdata->relicTime_10sec + '0';
	sdata->raceClockStr[3] = sdata->relicTime_1sec + '0';
	sdata->raceClockStr[5] = sdata->relicTime_10ms + '0';
	sdata->raceClockStr[6] = sdata->relicTime_1ms + '0';
	DecalFont_DrawLine(sdata->raceClockStr, (int)(s16)uVar11, (int)strFlags_but_its_also_posY, FONT_BIG,
	                   (int)(s16)(stringColor_but_its_also_relicColor & (0xffff ^ JUSTIFY_RIGHT)));
}

// countdown clock, used for Battle Mode and Crystal Challenge
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004f894-0x8004f9d8.
void UI_DrawLimitClock(s16 posX, s16 posY, s16 fontType)
{
	struct GameTracker *gGT;
	char *str;
	struct Driver *d;
	u32 flags;
	int i;
	int timeRemaining;
	gGT = sdata->gGT;

	// amount of time event should last, minus, time elapsed in the event.
	// basically, time remaining in the event
	timeRemaining = gGT->originalEventTime - gGT->elapsedEventTime;

	// if you run out of time
	if (timeRemaining < 0)
	{
		// Make a time string with zero milliseconds on the clock
		str = RECTMENU_DrawTime(0);

		// If you're not in End-Of-Race menu
		if ((gGT->gameMode1 & END_OF_RACE) == 0)
		{
			// dont check if numPlyrCurrGame != 0,
			// when would that ever be false

			// end race for all players
			for (i = 0; i < (u8)gGT->numPlyrCurrGame; i++)
			{
				// pointer of each player (P1, P2, P3, P4)
				d = gGT->drivers[i];
				d->actionsFlagSet |= ACTION_RACE_FINISHED;
			};

			MainGameEnd_Initialize();
		}
	}

	// if you have not run out of time
	else
	{
		// make a string with the remaining time
		str = RECTMENU_DrawTime(timeRemaining);
	}

	// default color is dark red
	flags = DARK_RED;

	if (
	    // if less than 15 seconds remain
	    (timeRemaining < 0x3840) &&

	    // if number of frames is an even number
	    ((gGT->timer & 1) == 0))
	{
		// set color to white
		flags = WHITE;
	}

	// put the time string on the screen
	DecalFont_DrawLine(str, (int)posX, (int)posY, (int)fontType, flags);
	return;
}
