#include <common.h>

enum
{
	COLL_SCRUB_DEPTH_REPEAT_STEP = 0x100,
	COLL_SCRUB_DEPTH_REPEAT_LIMIT = 0x401,
};

void COLL_MOVED_FindScrub(struct QuadBlock *qb, s32 triangleID, struct ScratchpadStruct *sps)
{
	struct ScratchpadStructExtended *ext = (struct ScratchpadStructExtended *)sps;
	// NOTE(aalhendi): Retail sign-extends this halfword before preserving its bits.
	s32 searchFlags = (s16)ext->scratchpadStruct.Union.QuadBlockColl.searchFlags;
	struct BspSearchTriangle *tri;
	s32 i;

	if (qb == NULL)
	{
		ext->scratchpadStruct.Union.QuadBlockColl.searchFlags = searchFlags & ~COLL_SEARCH_REPEAT_SCRUB;
		ext->scratchpadStruct.Input1.scrubDepth = 0;
		ext->numTriangles = 0;
		return;
	}

	for (i = ext->numTriangles - 1; i >= 0; i--)
	{
		tri = &ext->bspSearchTriangle[i];

		if ((tri->quadblock == qb) && (tri->triangleID == triangleID))
		{
			s32 scrubDepth = tri->scrubDepth;

			if (scrubDepth < COLL_SCRUB_DEPTH_REPEAT_LIMIT)
			{
				// The guard bounds this signed addition, including negative depths.
				scrubDepth += COLL_SCRUB_DEPTH_REPEAT_STEP;
				tri->scrubDepth = scrubDepth;
			}

			ext->scratchpadStruct.Union.QuadBlockColl.searchFlags = searchFlags | COLL_SEARCH_REPEAT_SCRUB;
			ext->scratchpadStruct.Input1.scrubDepth = scrubDepth;
			return;
		}
	}

	tri = &ext->bspSearchTriangle[ext->numTriangles];
	tri->quadblock = qb;
	tri->triangleID = triangleID;
	tri->scrubDepth = 0;

	ext->scratchpadStruct.Union.QuadBlockColl.searchFlags = searchFlags & ~COLL_SEARCH_REPEAT_SCRUB;
	ext->scratchpadStruct.Input1.scrubDepth = 0;
	ext->numTriangles++;
}
