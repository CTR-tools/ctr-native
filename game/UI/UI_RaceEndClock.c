#include <common.h>

enum UIRaceEndClockConstants
{
	UI_RACE_END_DRIVER_COUNT = 8,
	UI_RACE_END_AVG_SPEED_SCALE = 100,
	UI_RACE_END_MIN_MISSILES_FOR_RATIO = 4,
	UI_RACE_END_ATTACK_RATIO_SHIFT = 0xc,
	UI_RACE_END_ATTACK_RATIO_INVALID = -1,
	UI_RACE_END_FIRST_PLACE_RANK = 0,
};

void UI_RaceEnd_GetDriverClock(struct Driver *driver)
{
	s32 missilesLaunched;
	s32 scaledDistance;
	s32 timeElapsed;
	s32 attacksReceived;

	if ((driver->actionsFlagSet & ACTION_RACE_TIMER_FROZEN) == 0)
	{
		timeElapsed = driver->timeElapsedInRace;
		driver->actionsFlagSet |= ACTION_RACE_TIMER_FROZEN;

		if (timeElapsed != 0)
		{
			// NOTE(aalhendi): Retail keeps the low word of distance * 100.
			scaledDistance = (s32)((u32)driver->distanceDriven * UI_RACE_END_AVG_SPEED_SCALE);
			driver->distanceDriven = scaledDistance / timeElapsed;
		}

		if (driver->numTimesMissileLaunched >= UI_RACE_END_MIN_MISSILES_FOR_RATIO)
		{
			missilesLaunched = driver->numTimesMissileLaunched;
			driver->NumMissilesComparedToNumAttacks = (driver->numTimesAttacking << UI_RACE_END_ATTACK_RATIO_SHIFT) / missilesLaunched;
		}
		else
		{
			// Fewer than four launches does not produce an accuracy ratio.
			driver->NumMissilesComparedToNumAttacks = UI_RACE_END_ATTACK_RATIO_INVALID;
		}

		attacksReceived = 0;
		{
			s32 i;

			for (i = 0; i < UI_RACE_END_DRIVER_COUNT; i++)
			{
				attacksReceived += driver->numTimesAttackedByPlayer[i];
			}
		}

		driver->numTimesAttacked = attacksReceived;

		if (driver->driverRank == UI_RACE_END_FIRST_PLACE_RANK)
		{
			driver->TimeWinningDriverSpentLastPlace = driver->timeSpentInLastPlace;
		}
	}
}
