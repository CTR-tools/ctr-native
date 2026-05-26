#include <common.h>

// NOTE(aalhendi): Native mirrors retail rdata 0x80011180 because REBUILD_PC
// does not expose the retail rdata object.
#if defined(CTR_NATIVE)
static const int s_advHubConnectedLevID[5][3] = {
    {N_SANITY_BEACH, THE_LOST_RUINS, -1},
    {GEM_STONE_VALLEY, GLACIER_PARK, -1},
    {GEM_STONE_VALLEY, GLACIER_PARK, -1},
    {N_SANITY_BEACH, THE_LOST_RUINS, CITADEL_CITY},
    {GLACIER_PARK, -1, -1},
};
#define LOAD_HUB_CONNECTED_LEV(hub, index) s_advHubConnectedLevID[(hub)][(index)]
#else
#define LOAD_HUB_CONNECTED_LEV(hub, index) rdata.MetaDataHubs[(hub)].connectedHub_LevID[(index)]
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033318-0x80033474.
void LOAD_Hub_Main(int bigfilePtr)
{
	struct GameTracker *gGT;

	// quit if already loading
	if (sdata->Loading.stage != -1)
		return;

	gGT = sdata->gGT;

	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		int stepFlagSet = gGT->drivers[i]->stepFlagSet;
		int nextLevelID = (stepFlagSet & 0x30) >> 4;
		int needSwapNow = (stepFlagSet & 0xc0) >> 6;

		// if new level does not need to load
		if (nextLevelID == 0)
		{
			if ((needSwapNow != 0) || (gGT->bool_AdvHub_NeedToSwapLEV != 0))
			{
				gGT->bool_AdvHub_NeedToSwapLEV = 0;
				LOAD_Hub_SwapNow();
			}
		}

		// if new level needs to load
		else
		{
			// only in AdvHub, or else the game
			// crashes in 4P Nitro Court Life Limit
			u32 currLevelID = gGT->levelID - GEM_STONE_VALLEY;

			// ctr hubs are 0-4
			if (currLevelID >= 5)
				return;

			LOAD_Hub_ReadFile(bigfilePtr, LOAD_HUB_CONNECTED_LEV(currLevelID, nextLevelID - 1), 3 - gGT->activeMempackIndex);
		}
	}
}
