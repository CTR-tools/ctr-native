#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80034960-0x800349c4.
int MainDB_GetClipSize(u32 levelID, int numPlyrCurrGame)
{
	switch (levelID)
	{
	case ADVENTURE_GARAGE:
		return 24000;

	case MAIN_MENU_LEVEL:
		return 16;

	case SEWER_SPEEDWAY:
		return 6000;

	case MYSTERY_CAVES:
		return 2500;

	case PAPU_PYRAMID:
	case POLAR_PASS:
		if (numPlyrCurrGame < 3)
			return 3000;

		return 2500;

	default:
		return 3000;
	}
}
