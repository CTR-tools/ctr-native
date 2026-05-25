#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046458-0x80046534.
void DECOMP_RECTMENU_CollectInput()
{
	int i;
	int numListen;
	struct RectMenu *activeSub;

	sdata->AnyPlayerTap = 0;
	sdata->AnyPlayerHold = 0;
	activeSub = sdata->activeSubMenu;

	numListen = sdata->gGT->numPlyrNextGame;

	if ((activeSub != NULL) && ((activeSub->state & ALL_PLAYERS_USE_MENU) != 0))
	{
		numListen = 4;
	}

	struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[0];

#ifdef REBUILD_PS1
	if ((gb[0].buttonsTapped & BTN_CROSS) != 0)
	{
		gb[1].buttonsTapped = BTN_CROSS;
		gb[2].buttonsTapped = BTN_CROSS;
		gb[3].buttonsTapped = BTN_CROSS;
	}
	else
	{
		gb[1].buttonsTapped = 0;
		gb[2].buttonsTapped = 0;
		gb[3].buttonsTapped = 0;
	}

	sdata->gameProgress.unlocks[0] = -1;
#endif

	for (i = 0; i < numListen; i++)
	{
		sdata->buttonTapPerPlayer[i] = gb->buttonsTapped;
		sdata->buttonHeldPerPlayer[i] = gb->buttonsHeldCurrFrame;
		gb++;

		sdata->AnyPlayerTap |= sdata->buttonTapPerPlayer[i];
		sdata->AnyPlayerHold |= sdata->buttonHeldPerPlayer[i];
	}
}
