#include <common.h>

enum PodiumConstants
{
	PODIUM_DRIVER_COUNT = 8,
	PODIUM_RANK_FIRST = 0,
	PODIUM_RANK_SECOND = 1,
	PODIUM_RANK_THIRD = 2,
};


void Podium_InitModels(struct GameTracker *gGT)
{
	s32 i;
	struct Driver *driver;
	struct Driver **drivers;

	gGT->podium_modelIndex_First = 0;
	gGT->podium_modelIndex_Second = 0;
	gGT->podium_modelIndex_Third = 0;
	gGT->podium_modelIndex_tawna = STATIC_TAWNA1;

	for (i = 0, drivers = gGT->drivers; i < PODIUM_DRIVER_COUNT; i++, drivers++)
	{
		driver = *drivers;
		if (driver != 0)
		{
			// NOTE(aalhendi): Each rank writes a named byte field, not an element of a C array.
			switch (driver->driverRank)
			{
			case PODIUM_RANK_FIRST:
			{
				gGT->podium_modelIndex_First = (u8)GAME_CHARACTER_IDS[driver->driverID] + STATIC_CRASHDANCE;
				// NOTE(aalhendi): Select Tawna from the stored model byte; unlisted winners keep the current selection.
				switch (gGT->podium_modelIndex_First)
				{
				case STATIC_CRASHDANCE + CRASH_BANDICOOT:
				case STATIC_CRASHDANCE + COCO_BANDICOOT:
				{
					gGT->podium_modelIndex_tawna = STATIC_TAWNA2;
					break;
				}
				case STATIC_CRASHDANCE + POLAR:
				case STATIC_CRASHDANCE + PURA:
				{
					gGT->podium_modelIndex_tawna = STATIC_TAWNA3;
					break;
				}
				case STATIC_CRASHDANCE + NEO_CORTEX:
				case STATIC_CRASHDANCE + N_GIN:
				{
					gGT->podium_modelIndex_tawna = STATIC_TAWNA4;
					break;
				}
				}
				break;
			}
			case PODIUM_RANK_SECOND:
			{
				gGT->podium_modelIndex_Second = (u8)GAME_CHARACTER_IDS[driver->driverID] + STATIC_CRASHDANCE;
				break;
			}
			case PODIUM_RANK_THIRD:
			{
				gGT->podium_modelIndex_Third = (u8)GAME_CHARACTER_IDS[driver->driverID] + STATIC_CRASHDANCE;
				break;
			}
			}
		}
	}
}
