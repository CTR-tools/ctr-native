#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043b30-0x80043c04.
void QueueLoadTrack_MenuProc(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;

	// If you're in Time Trial,
	// set P3 to N Tropy, and P4 to N Oxide
	if ((gGT->gameMode1 & TIME_TRIAL) != 0)
	{
		data.characterIDs[2] = N_TROPY;
		data.characterIDs[3] = NITROS_OXIDE;
	}

	// If you're in Adventure Mode,
	// go to Adventure Arena from Title Screen
	if ((gGT->gameMode1 & ADVENTURE_MODE) != 0)
	{
		gGT->gameMode1 |= ADVENTURE_ARENA;
	}

	// If you're not in Battle Mode,
	// clear battle related variables
	if ((gGT->gameMode1 & BATTLE_MODE) == 0)
	{
		gGT->originalEventTime = 0x2a300;
		gGT->gameMode1 &= ~(POINT_LIMIT | LIFE_LIMIT | TIME_LIMIT);
	}

	MainRaceTrack_RequestLoad(gGT->currLEV);

	RECTMENU_Hide(menu);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043c04-0x80043c10.
struct RectMenu *QueueLoadTrack_GetMenuPtr()
{
	return &data.menuQueueLoadTrack;
}
