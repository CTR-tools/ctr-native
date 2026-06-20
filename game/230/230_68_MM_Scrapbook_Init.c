#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3fe4-0x800b4014.
void MM_Scrapbook_Init(void)
{
	D230.scrapbookState = SCRAP_INIT;

	// change checkered flag
	RaceFlag_SetDrawOrder(1);

	// clear gamepad input (for menus)
	RECTMENU_ClearInput();
}
