#include "common.h"

#define COLL_BSPLEAF_TEST_QUAD COLL_FIXED_QUADBLK_TestTriangles
void COLL_FIXED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps)
{
#include "COLL_BspLeafTraversal.inc"
}
#undef COLL_BSPLEAF_TEST_QUAD
