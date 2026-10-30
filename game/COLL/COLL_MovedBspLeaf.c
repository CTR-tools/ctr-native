#include "common.h"

#define COLL_BSPLEAF_TEST_QUAD COLL_MOVED_QUADBLK_TestTriangles
void COLL_MOVED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps)
{
#include "COLL_BspLeafTraversal.inc"
}
#undef COLL_BSPLEAF_TEST_QUAD
