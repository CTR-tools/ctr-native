#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046404-0x80046458.
void DECOMP_RECTMENU_ClearInput()
{
	int i;

	sdata->AnyPlayerTap = 0;
	sdata->AnyPlayerHold = 0;

	for (i = 0; i < 4; i++)
	{
		sdata->buttonTapPerPlayer[i] = 0;
		sdata->buttonHeldPerPlayer[i] = 0;
	}
}
