#include <common.h>

enum
{
	UI_LIMIT_CLOCK_FLASH_THRESHOLD = 0x3840,
};

// Countdown clock for Battle Mode and Crystal Challenge.
void UI_DrawLimitClock(s16 posX, s16 posY, s16 fontType)
{
	char *str;
	u32 flags;
	s32 i;

	// NOTE(aalhendi): Retail subtracts words without trapping on signed overflow.
	s32 timeRemaining = CTR_MipsSubLo(GAME_TRACKER->originalEventTime, GAME_TRACKER->elapsedEventTime);

	if (timeRemaining < 0)
	{
		struct GameTracker *gGT;

		str = RECTMENU_DrawTime(0);
		gGT = GAME_TRACKER;

		if ((gGT->gameMode1 & END_OF_RACE) == 0)
		{
			// Finish each active player before starting the end-of-race transition.
			for (i = 0; i < gGT->numPlyrCurrGame; i++)
			{
				struct Driver *d = gGT->drivers[i];
				d->actionsFlagSet |= ACTION_RACE_FINISHED;
			}

			MainGameEnd_Initialize();
		}
	}

	else
	{
		str = RECTMENU_DrawTime(timeRemaining);
	}

	// Flash only during the last 15 seconds.
	if (timeRemaining < UI_LIMIT_CLOCK_FLASH_THRESHOLD)
	{
		flags = WHITE;
		if ((GAME_TRACKER->timer & 1) != 0)
		{
			flags = DARK_RED;
		}
	}
	else
	{
		flags = DARK_RED;
	}

	DecalFont_DrawLine(str, (int)posX, (int)posY, (int)fontType, flags);
}
