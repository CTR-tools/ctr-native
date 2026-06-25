#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021bbc-0x80021c2c
void CTR_ClearRenderLists_1P2P(struct GameTracker *gGT, int numPlyrCurrGame)
{
	if (numPlyrCurrGame <= 0)
	{
		return;
	}

	for (int i = 0; i < numPlyrCurrGame; i++)
	{
		void *quadBlocksRendered = data.ptrRenderedQuadblockDestination_forEachPlayer[i];

		for (int listIndex = 0; listIndex < 5; listIndex++)
		{
			gGT->LevRenderLists[i].list[listIndex].bspListStart = 0;
			gGT->LevRenderLists[i].list[listIndex].ptrQuadBlocksRendered = quadBlocksRendered;
		}

		gGT->LevRenderLists[i].bspListStart_FullDynamic = 0;
		gGT->LevRenderLists[i].ptrQuadBlocksRendered_FullDynamic = 0;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021c2c-0x80021c8c
void CTR_ClearRenderLists_3P4P(struct GameTracker *gGT, int numPlyrCurrGame)
{
	if (numPlyrCurrGame <= 0)
	{
		return;
	}

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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021c8c-0x80021c94.
void CTR_EmptyFunc_MainFrame_ResetDB(void)
{
}
