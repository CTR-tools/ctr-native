#include <common.h>

void UI_JumpMeter_Update(struct Driver *driver)
{
	if ((driver->actionsFlagSet & ACTION_AIRBORNE) != 0)
	{
		if ((UI_JUMP_METER_LANDING_VOICELINE_MIN < driver->jump_LandingBoost) && (driver->jumpMeter < UI_JUMP_METER_LANDING_VOICELINE_CUTOFF))
		{
			Voiceline_RequestPlay(UI_JUMP_METER_LANDING_VOICELINE_ID, GAME_CHARACTER_IDS[driver->driverID], UI_JUMP_METER_LANDING_VOICELINE_FLAGS);
		}

		// NOTE(aalhendi): Sample landing boost after the voice provider, not before it.
		driver->jumpMeter = driver->jump_LandingBoost;
		if (UI_JUMP_METER_MAX < driver->jumpMeter)
		{
			driver->jumpMeter = UI_JUMP_METER_MAX;
		}
		driver->jumpMeterTimer = UI_JUMP_METER_DISPLAY_TIMER;
	}
	else if ((driver->actionsFlagSetPrevFrame & ACTION_AIRBORNE) != 0)
	{
		if (UI_JUMP_METER_SIGNIFICANT_THRESHOLD < driver->jumpMeter)
		{
			// NOTE(aalhendi): Retail adds the low word even when the signed statistic wraps.
			driver->timeSpentJumping = CTR_MipsAddLo(driver->timeSpentJumping, driver->jumpMeter);
		}

		if (driver->jumpMeter > driver->highestJump)
		{
			driver->highestJump = driver->jumpMeter;
		}

		if (driver->jumpMeter >= UI_JUMP_METER_HANG_TIME_LARGE)
		{
			VehFire_Increment(driver, UI_JUMP_METER_HANG_TIME_RESERVES, POWER_SLIDE_HANG_TIME, UI_JUMP_METER_HANG_TIME_FIRE_LARGE);
		}
		else if (driver->jumpMeter >= UI_JUMP_METER_HANG_TIME_MEDIUM)
		{
			VehFire_Increment(driver, UI_JUMP_METER_HANG_TIME_RESERVES, POWER_SLIDE_HANG_TIME, UI_JUMP_METER_HANG_TIME_FIRE_MEDIUM);
		}
		else if (driver->jumpMeter >= UI_JUMP_METER_HANG_TIME_SMALL)
		{
			VehFire_Increment(driver, UI_JUMP_METER_HANG_TIME_RESERVES, POWER_SLIDE_HANG_TIME, UI_JUMP_METER_HANG_TIME_FIRE_SMALL);
		}
	}
	else
	{
		if (driver->jumpMeterTimer != 0)
		{
			// NOTE(aalhendi): Narrow to the stored signed halfword before testing for negativity.
			driver->jumpMeterTimer -= (u32)GAME_TRACKER->elapsedTimeMS;
			if (driver->jumpMeterTimer < 0)
			{
				driver->jumpMeterTimer = 0;
			}
		}
		else
		{
			driver->jumpMeter = 0;
		}
	}
	return;
}
