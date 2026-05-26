#include <common.h>

void PickupBots_Init(void)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80040850-0x800408b8.
	int hub;
	int lev = sdata->gGT->levelID;

	// get hubID of level
	hub = data.metaDataLEV[lev].hubID;

	// If Level ID is Oxide Station
	if (lev == OXIDE_STATION)
	{
		hub = 0;
	}

	if (hub > -1)
	{
		// set pointer to boss weapon meta
		sdata->bossWeaponMeta = data.bossWeaponMetaPtr[hub];
	}
	return;
}
