#include <common.h>


void QueueLoadTrack_MenuProc(struct RectMenu *menu)
{
	// NOTE(aalhendi): Retail reacquires shared owners after each state-writing phase.
	// If you're in Time Trial,
	// set P3 to N Tropy, and P4 to N Oxide
	if ((GAME_TRACKER->gameMode1 & TIME_TRIAL) != 0)
	{
		GAME_CHARACTER_IDS[2] = N_TROPY;
		GAME_CHARACTER_IDS[3] = NITROS_OXIDE;
	}

	// If you're in Adventure Mode,
	// go to Adventure Arena from Title Screen
	if ((GAME_TRACKER->gameMode1 & ADVENTURE_MODE) != 0)
	{
		GAME_TRACKER->gameMode1 |= ADVENTURE_ARENA;
	}

	// If you're not in Battle Mode,
	// clear battle related variables
	if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) == 0)
	{
		GAME_TRACKER->originalEventTime = TITLE_INITIAL_EVENT_TIME;
		GAME_TRACKER->gameMode1 &= ~(POINT_LIMIT | LIFE_LIMIT | TIME_LIMIT);
	}

	MainRaceTrack_RequestLoad(GAME_TRACKER->currLEV);

	RECTMENU_Hide(menu);
}


struct RectMenu *QueueLoadTrack_GetMenuPtr()
{
	return &data.menuQueueLoadTrack;
}
