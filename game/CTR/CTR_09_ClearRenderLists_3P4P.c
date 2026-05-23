#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021c2c-0x80021c8c
void CTR_ClearRenderLists_3P4P(struct GameTracker *gGT, int numPlyrCurrGame)
{
	if (numPlyrCurrGame <= 0)
		return;

	for (int i = 0; i < numPlyrCurrGame; i++)
	{
		void *quadBlocksRendered = data.ptrRenderedQuadblockDestination_again[i];

		for (int listIndex = 0; listIndex < 4; listIndex++)
		{
			gGT->LevRenderLists[i].list[listIndex].bspListStart = 0;
			gGT->LevRenderLists[i].list[listIndex].ptrQuadBlocksRendered = quadBlocksRendered;
		}

		gGT->LevRenderLists[i].list[4].ptrQuadBlocksRendered = 0;
	}
}
